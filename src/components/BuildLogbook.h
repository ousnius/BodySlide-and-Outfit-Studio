/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include <map>
#include <string>
#include <vector>
#include <wx/dir.h>

using namespace tinyxml2;

class BuildLogbookEntry {
public:
	std::string path;
	std::string set;
	std::string preset;

	struct SliderValue {
		std::string name;
		std::string size;
		float value;
	};
	std::vector<SliderValue> sliders;

	std::string GetSummary();
};

class BuildLogbook {
	std::map<std::string, BuildLogbookEntry> entries;

public:
	BuildLogbook() {}
	BuildLogbook(XMLElement* srcElement) { LoadBuildLogbook(srcElement); }

	int LoadBuildLogbook(XMLElement* srcElement);

	void AddEntry(const BuildLogbookEntry& entry);
	void RemoveEntry(const std::string& path);
	bool HasEntry(const std::string& path);
	BuildLogbookEntry GetEntry(const std::string& path);

	const std::map<std::string, BuildLogbookEntry>& GetEntries() const;
};

class BuildLogbookFile {
	XMLDocument doc;
	XMLElement* root = nullptr;
	int error = 0;

public:
	std::string fileName;
	BuildLogbookFile() {}
	BuildLogbookFile(const std::string& srcFileName);
	~BuildLogbookFile() {}

	bool fail() { return error != 0; }
	int GetError() { return error; }

	// Clears all data of the file
	void Clear();

	// Creates a new empty document structure
	void New(const std::string& newFileName);

	// Loads the XML document. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	void Rename(const std::string& newFileName);

	// Writes the XML file using the internal fileName (use Rename() to change the name).
	bool Save();

	// Get the build logbook in the file
	void Get(BuildLogbook& outLogbook);

	// Updates or adds entries in the XML document
	int UpdateEntries(const BuildLogbook& inLogbook);

	// Removes a single element from the XML document
	void RemoveEntry(const std::string& path);
};
