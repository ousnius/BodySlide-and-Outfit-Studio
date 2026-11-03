/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderSetIndex.h"
#include "SliderDataFileUtil.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <wx/dir.h>

namespace {
// A name is written as it is into the XML text, unless it has characters that get escaped there or
// that a case-insensitive comparison could fold differently
bool CanFindInText(const std::string& name) {
	if (name.empty())
		return false;

	for (unsigned char c : name)
		if (c >= 0x80 || c == '&' || c == '<' || c == '>' || c == '"' || c == '\'')
			return false;

	return true;
}

unsigned char LowerASCII(unsigned char c) {
	return c >= 'A' && c <= 'Z' ? static_cast<unsigned char>(c - 'A' + 'a') : c;
}

// Case-insensitive Boyer-Moore-Horspool search, which skips through the text instead of looking at every character
bool ContainsInsens(const std::string& text, const std::string& lowerName) {
	const size_t nameSize = lowerName.size();
	if (nameSize == 0 || text.size() < nameSize)
		return false;

	size_t skip[256];
	std::fill(std::begin(skip), std::end(skip), nameSize);
	for (size_t i = 0; i + 1 < nameSize; i++) {
		unsigned char c = static_cast<unsigned char>(lowerName[i]);
		skip[c] = nameSize - 1 - i;
		if (c >= 'a' && c <= 'z')
			skip[c - 'a' + 'A'] = nameSize - 1 - i;
	}

	const auto* textData = reinterpret_cast<const unsigned char*>(text.data());
	const auto* nameData = reinterpret_cast<const unsigned char*>(lowerName.data());
	for (size_t pos = 0; pos + nameSize <= text.size(); pos += skip[textData[pos + nameSize - 1]]) {
		size_t i = nameSize;
		while (i > 0 && LowerASCII(textData[pos + i - 1]) == nameData[i - 1])
			i--;

		if (i == 0)
			return true;
	}

	return false;
}

bool FileMentions(const std::string& fileName, const std::vector<std::string>& lowerNames) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);
	if (!file.is_open())
		return true;

	file.seekg(0, std::ios::end);
	std::streamoff size = file.tellg();
	if (size <= 0)
		return true;

	std::string text(static_cast<size_t>(size), '\0');
	file.seekg(0, std::ios::beg);
	file.read(text.data(), size);
	text.resize(static_cast<size_t>(file.gcount()));

	for (auto& name : lowerNames)
		if (ContainsInsens(text, name))
			return true;

	return false;
}
} // namespace

SliderSetIndex::SliderSetIndex(const std::string& inProjectPath) {
	Load(inProjectPath);
}

SliderSetIndex::SliderSetIndex(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames) {
	Load(inProjectPath, mentionedFileNames);
}

void SliderSetIndex::Load(const std::string& inProjectPath, const std::vector<std::string>& mentionedFileNames, const std::atomic<bool>* cancel) {
	projectPath = inProjectPath;
	entries.clear();
	filterNames.clear();

	// Without a name that can be found in the text, every file has to be loaded
	std::vector<std::string> lowerNames;
	bool filterFiles = !mentionedFileNames.empty();
	for (auto& name : mentionedFileNames) {
		if (!CanFindInText(name)) {
			filterFiles = false;
			break;
		}

		lowerNames.push_back(ToLower(name));
	}

	if (filterFiles)
		filterNames = lowerNames;

	wxString sliderSetsPath = wxString::FromUTF8(projectPath + PathSepStr + "SliderSets");
	if (!wxDir::Exists(sliderSetsPath))
		return;

	wxArrayString files;
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.osp");
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.xml");

	const std::string shapeDataPath = projectPath + PathSepStr + "ShapeData";

	for (auto& file : files) {
		if (cancel && *cancel)
			return;

		std::string fileName{file.ToUTF8()};
		if (filterFiles && !FileMentions(fileName, lowerNames))
			continue;

		SliderSetFile sliderSetFile(fileName);
		if (sliderSetFile.fail())
			continue;

		std::vector<std::string> setNames;
		sliderSetFile.GetSetNamesUnsorted(setNames, false);

		for (auto& setName : setNames) {
			if (cancel && *cancel)
				return;

			if (filterFiles && !sliderSetFile.SetMentions(setName, lowerNames))
				continue;

			SliderSetIndexEntry entry;
			if (sliderSetFile.GetSet(setName, entry.set, true, "", false) != 0)
				continue;

			entry.fileName = fileName;
			entry.setName = setName;
			entry.set.SetBaseDataPath(shapeDataPath);
			entries.push_back(std::move(entry));
		}
	}
}

const SliderSetIndexEntry* SliderSetIndex::Find(const std::string& setName) const {
	for (auto& entry : entries)
		if (entry.setName == setName)
			return &entry;

	return nullptr;
}

bool SliderSetIndex::Covers(const std::vector<std::string>& fileNames) const {
	if (filterNames.empty())
		return true;

	for (auto& fileName : fileNames)
		if (std::find(filterNames.begin(), filterNames.end(), ToLower(fileName)) == filterNames.end())
			return false;

	return true;
}

template<typename Use>
void SliderSetIndex::ForDataInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet, Use use) const {
	std::string osdName = std::filesystem::u8path(osdFilePath).filename().u8string();

	// Many sets name the same files, so paths are compared as text and each one is looked up once
	auto normalize = [](const std::string& path) {
		std::string normalized = std::filesystem::u8path(path).lexically_normal().u8string();
#ifdef _WINDOWS
		normalized = ToLower(normalized);
#endif
		return normalized;
	};

	std::map<std::string, bool> existingFiles;
	auto fileExists = [&existingFiles](const std::string& path) {
		auto existing = existingFiles.find(path);
		if (existing != existingFiles.end())
			return existing->second;

		return existingFiles[path] = PlatformUtil::FileExists(path);
	};

	const std::string osdPath = normalize(osdFilePath);

	for (auto& entry : entries) {
		if (entry.setName == exceptSet && SameFile(entry.fileName, exceptFile))
			continue;

		const SliderSet& set = entry.set;
		for (size_t i = 0; i < set.size(); i++) {
			for (auto& dataFile : set[i].dataFiles) {
				if (SliderDataFileIsBSD(dataFile.fileName))
					continue;

				std::string dataOSDFileName;
				std::string dataNameInFile;
				if (!SplitSliderDataFileName(dataFile.fileName, dataOSDFileName, dataNameInFile))
					continue;

				// Only resolve the paths of data in a file of the same name
				if (!StringsEqualInsens(dataOSDFileName.c_str(), osdName.c_str()))
					continue;

				// The data is in the first of its data folders that has the file, like ResolveSliderDataFile finds it
				std::vector<std::string> dataFolders;
				if (dataFile.bLocal)
					dataFolders.push_back(set.GetDefaultDataFolder());
				else
					dataFolders = set.GetTargetDataFolders(dataFile.targetName);

				for (auto& dataFolder : dataFolders) {
					std::string candidatePath = normalize(set.GetBaseDataPath() + PathSepStr + dataFolder + PathSepStr + dataOSDFileName);
					if (candidatePath == osdPath) {
						use(entry, dataNameInFile);
						break;
					}

					if (fileExists(candidatePath))
						break;
				}
			}
		}
	}
}

std::set<std::string> SliderSetIndex::DataUsedInFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const {
	std::set<std::string> dataNames;
	ForDataInFile(osdFilePath, exceptFile, exceptSet, [&dataNames](const SliderSetIndexEntry&, const std::string& dataName) { dataNames.insert(dataName); });
	return dataNames;
}

std::vector<const SliderSetIndexEntry*> SliderSetIndex::SetsUsingDataFile(const std::string& osdFilePath, const std::string& exceptFile, const std::string& exceptSet) const {
	std::vector<const SliderSetIndexEntry*> sets;
	ForDataInFile(osdFilePath, exceptFile, exceptSet, [&sets](const SliderSetIndexEntry& entry, const std::string&) {
		if (sets.empty() || sets.back() != &entry)
			sets.push_back(&entry);
	});
	return sets;
}

std::vector<const SliderSetIndexEntry*> SliderSetIndex::SetsUsingNif(const std::string& nifFilePath, const std::string& exceptFile, const std::string& exceptSet) const {
	std::vector<const SliderSetIndexEntry*> sets;

	std::string nifName = std::filesystem::u8path(nifFilePath).filename().u8string();

	for (auto& entry : entries) {
		if (entry.setName == exceptSet && SameFile(entry.fileName, exceptFile))
			continue;

		const SliderSet& set = entry.set;
		if (!StringsEqualInsens(set.GetInputFile().c_str(), nifName.c_str()))
			continue;

		if (SameFile(set.GetInputFileName(), nifFilePath))
			sets.push_back(&entry);
	}

	return sets;
}

bool SliderSetIndex::SameFile(const std::string& fileName1, const std::string& fileName2) {
	std::error_code error;
	return std::filesystem::equivalent(std::filesystem::u8path(fileName1), std::filesystem::u8path(fileName2), error);
}

SliderSetIndexTask::SliderSetIndexTask(const std::string& projectPath, const std::vector<std::string>& inFileNames)
	: fileNames(inFileNames) {
	thread = std::thread([this, projectPath] {
		index.Load(projectPath, fileNames, &cancel);
		if (!cancel)
			done = true;
	});
}

SliderSetIndexTask::~SliderSetIndexTask() {
	Cancel();
}

void SliderSetIndexTask::Cancel() {
	cancel = true;
	if (thread.joinable())
		thread.join();
}
