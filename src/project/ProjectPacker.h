/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "components/SliderSet.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <wx/string.h>
#include <wx/stream.h>

class wxFFileOutputStream;
class wxZipOutputStream;

// Where packed projects are written: a folder or a zip archive. Files get the same relative paths in both.
class PackTarget {
public:
	// What a packed file is, for the error messages
	enum class FileKind { Input, Data, Project, Group };

	virtual ~PackTarget() = default;

	// Adds the file at sourcePath under entryPath (relative, native separators).
	// Returns false and sets error to a message for the user if that fails.
	virtual bool AddFile(const wxString& sourcePath, const wxString& entryPath, FileKind kind, wxString& error) = 0;
};

// Copies the files into a folder
class FolderPackTarget : public PackTarget {
public:
	explicit FolderPackTarget(const wxString& inFolder)
		: folder(inFolder) {}

	bool AddFile(const wxString& sourcePath, const wxString& entryPath, FileKind kind, wxString& error) override;

private:
	wxString folder;
};

// Writes the files into a zip archive, which is complete once this is destroyed
class ZipPackTarget : public PackTarget {
public:
	explicit ZipPackTarget(const wxString& fileName);
	~ZipPackTarget() override;

	bool AddFile(const wxString& sourcePath, const wxString& entryPath, FileKind kind, wxString& error) override;

private:
	std::unique_ptr<wxFFileOutputStream> out;
	std::unique_ptr<wxZipOutputStream> zip;
};

struct PackResult {
	// Empty when packing worked, else what went wrong for the user
	wxString error;
	// Base projects of packed variants or add-ons that aren't packed themselves
	std::set<std::string> missingBaseProjects;
};

// Packs projects for release: one merged slider set file, the input NIFs and slider data of the projects and
// optionally a group file, all below baseDir ("CalienteTools/BodySlide" or "Tools/BodySlide").
// mergedFile is the merged slider set file's name, sources the projects by name and selected the ones to pack.
PackResult PackProjects(const std::map<std::string, SliderSet>& sources,
						const std::set<std::string>& selected,
						const std::string& mergedFile,
						const wxString& groupFilePath,
						const std::string& baseDir,
						PackTarget& target);

// Copies size bytes of a stream to another, or all of it for -1
bool CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size);
