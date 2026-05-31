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
	std::string fileName;

public:
	BuildLogbook() {}
	BuildLogbook(XMLElement* srcElement) { LoadBuildLogbook(srcElement); }

	int LoadBuildLogbook(XMLElement* srcElement);

	bool LoadFromFile(const std::string& path);
	bool SaveToFile();

	void AddEntry(const BuildLogbookEntry& entry);
	void RemoveEntry(const std::string& path);
	bool HasEntry(const std::string& path);
	BuildLogbookEntry GetEntry(const std::string& path);

	const std::map<std::string, BuildLogbookEntry>& GetEntries() const;
};
