/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectPacker.h"
#include "utils/PlatformUtil.h"
#include "utils/ProjectUtil.h"

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

namespace {

// A packed variant or add-on refers to its base project in the merged file when that's packed along.
// Otherwise the base project has to be installed already, which is noted.
void PackBaseProject(SliderSet& set, const std::set<std::string>& packedProjects, const std::string& mergedFile, std::set<std::string>& missingBaseProjects) {
	if (!set.HasBaseProject())
		return;

	const std::string packedFile = "SliderSets" + std::string(PathSepStr) + mergedFile;
	if (packedProjects.find(set.GetBaseProjectName()) != packedProjects.end())
		set.SetBaseProject(packedFile, set.GetBaseProjectName(), set.GetBaseProjectID());
	else
		missingBaseProjects.insert(set.GetBaseProjectName());

	// Other base projects are optional, the ones that are packed too are in the merged file. An entry with the base
	// project's name is a copy of it in another file, e.g. in RefTemplates, which keeps its path.
	std::vector<BaseProjectRef> otherBases = set.GetOtherBaseProjects();
	for (auto& otherBase : otherBases)
		if (otherBase.name != set.GetBaseProjectName() && packedProjects.find(otherBase.name) != packedProjects.end())
			otherBase.file = packedFile;
	set.SetOtherBaseProjects(otherBases);
}

// The slider data files (.osd) and folders (.bsd) of a project, relative to the ShapeData folder
std::set<std::string> ProjectDataFiles(SliderSet& set, const std::string& sep) {
	std::set<std::string> dataFiles;

	for (size_t i = 0; i < set.size(); i++) {
		for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
			std::string target = set.ShapeToTarget(it->first);
			std::string targetDataName = set[i].TargetDataName(target);
			if (set[i].IsLocalData(targetDataName) && !set[i].IsInheritedData(targetDataName)) {
				std::string dataFileName = set[i].DataFileName(targetDataName);
				const bool isBSD = dataFileName.size() >= 4 && dataFileName.compare(dataFileName.size() - 4, 4, ".bsd") == 0;
				if (!isBSD) {
					// Split target file name to get OSD file name
					int split = dataFileName.find_last_of('/');
					if (split < 0)
						split = dataFileName.find_last_of('\\');
					if (split < 0)
						continue;

					dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName.substr(0, split));
				}
				else {
					dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName);
				}
			}
		}
	}

	return dataFiles;
}

// Logs a failure and returns the message for the user
wxString PackError(const char* format, const wxString& path) {
	wxLogError(format, path);
	return wxString::Format(wxGetTranslation(format), path);
}

wxString PackError(const char* message) {
	wxLogError(message);
	return wxGetTranslation(message);
}

const char* OpenErrorFormat(PackTarget::FileKind kind) {
	switch (kind) {
		case PackTarget::FileKind::Input: return wxTRANSLATE("Failed to open input file '%s'!");
		case PackTarget::FileKind::Data: return wxTRANSLATE("Failed to open data file '%s'!");
		case PackTarget::FileKind::Project: return wxTRANSLATE("Failed to open project file '%s'!");
		case PackTarget::FileKind::Group: return wxTRANSLATE("Failed to open group file '%s'!");
	}
	return "";
}

} // namespace

bool FolderPackTarget::AddFile(const wxString& sourcePath, const wxString& entryPath, FileKind kind, wxString& error) {
	wxFileInputStream inputStream(sourcePath);
	if (!inputStream.IsOk()) {
		error = PackError(OpenErrorFormat(kind), sourcePath);
		return false;
	}

	wxString dest = folder + wxFileName::GetPathSeparator() + entryPath;
	wxFileName::Mkdir(dest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	if (!wxCopyFile(sourcePath, dest)) {
		const char* format = "";
		switch (kind) {
			case FileKind::Input: format = wxTRANSLATE("Failed to copy input file '%s'!"); break;
			case FileKind::Data: format = wxTRANSLATE("Failed to copy data file '%s'!"); break;
			case FileKind::Project: format = wxTRANSLATE("Failed to copy merged project file '%s'!"); break;
			case FileKind::Group: format = wxTRANSLATE("Failed to copy group file '%s'!"); break;
		}
		error = PackError(format, sourcePath);
		return false;
	}

	return true;
}

ZipPackTarget::ZipPackTarget(const wxString& fileName)
	: out(std::make_unique<wxFFileOutputStream>(fileName))
	, zip(std::make_unique<wxZipOutputStream>(*out)) {}

ZipPackTarget::~ZipPackTarget() {
	// The archive's directory is written when the zip stream closes, before the file does
	zip.reset();
	out.reset();
}

bool ZipPackTarget::AddFile(const wxString& sourcePath, const wxString& entryPath, FileKind kind, wxString& error) {
	wxFileInputStream inputStream(sourcePath);
	if (!inputStream.IsOk()) {
		error = PackError(OpenErrorFormat(kind), sourcePath);
		return false;
	}

	if (!zip->PutNextEntry(entryPath, wxDateTime::Now(), inputStream.GetLength())) {
		error = PackError(wxTRANSLATE("Failed to put new entry into archive!"));
		return false;
	}

	if (!CopyStreamData(inputStream, *zip, inputStream.GetLength())) {
		error = PackError(wxTRANSLATE("Failed to copy file contents to archive!"));
		return false;
	}

	return true;
}

PackResult PackProjects(const std::map<std::string, SliderSet>& sources,
						const std::set<std::string>& selected,
						const std::string& mergedFile,
						const wxString& groupFilePath,
						const std::string& baseDir,
						PackTarget& target) {
	PackResult result;
	const std::string sep{wxString(wxFileName::GetPathSeparator()).ToUTF8()};
	const std::string shapeData = ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep;

	std::string mergedFilePath{wxFileName::CreateTempFileName("os").ToUTF8()};
	// The targets copy the merged project right away, so the temporary file goes when packing ends either way
	struct RemoveOnExit {
		const std::string& path;
		~RemoveOnExit() { wxRemoveFile(wxString::FromUTF8(path)); }
	} removeMerged{mergedFilePath};
	SliderSetFile projectFile;
	projectFile.New(mergedFilePath);

	for (auto& setName : selected) {
		auto source = sources.find(setName);
		if (source == sources.end())
			continue;

		SliderSet set = source->second;
		PackBaseProject(set, selected, mergedFile, result.missingBaseProjects);
		projectFile.UpdateSet(set);

		// Relative to the ShapeData folder. GetInputFileName starts with a separator, which Linux keeps doubled in archive entries.
		const std::string inputFile = set.GetDefaultDataFolder() + sep + set.GetInputFile();
		if (!target.AddFile(wxString::FromUTF8(shapeData + inputFile),
							wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + inputFile),
							PackTarget::FileKind::Input,
							result.error))
			return result;

		for (auto& df : ProjectDataFiles(set, sep)) {
			if (!target.AddFile(wxString::FromUTF8(shapeData + df), wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + df), PackTarget::FileKind::Data, result.error))
				return result;
		}
	}

	// Save new merged project file
	if (!projectFile.Save()) {
		result.error = PackError(wxTRANSLATE("Failed to save merged project file '%s'!"), wxString::FromUTF8(mergedFilePath));
		return result;
	}

	if (!target.AddFile(wxString::FromUTF8(mergedFilePath), wxString::FromUTF8(baseDir + sep + "SliderSets" + sep + mergedFile), PackTarget::FileKind::Project, result.error))
		return result;

	if (!groupFilePath.IsEmpty()) {
		std::string groupFileName{groupFilePath.AfterLast(wxFileName::GetPathSeparator()).ToUTF8()};
		if (!target.AddFile(groupFilePath, wxString::FromUTF8(baseDir + sep + "SliderGroups" + sep + groupFileName), PackTarget::FileKind::Group, result.error))
			return result;
	}

	return result;
}

bool CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size) {
	auto buf = std::make_unique<wxChar[]>(128 * 1024);
	int readSize = 128 * 1024;
	wxFileOffset copiedData = 0;

	for (;;) {
		if (size != -1 && copiedData + readSize > size)
			readSize = size - copiedData;

		inputStream.Read(buf.get(), readSize);

		size_t actuallyRead = inputStream.LastRead();
		outputStream.Write(buf.get(), actuallyRead);
		if (outputStream.LastWrite() != actuallyRead) {
			wxLogError("Failed to output data when copying stream.");
			return false;
		}

		if (size == -1) {
			if (inputStream.Eof())
				break;
		}
		else {
			copiedData += actuallyRead;
			if (copiedData >= size)
				break;
		}
	}

	return true;
}
