/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace tinyxml2;

typedef std::pair<std::string, std::string> ZapChoiceKey;

// The last build of an output path to a data folder
struct BuildRecord {
	// Slider value that differed from the preset, in percent
	struct SliderValue {
		std::string name;
		bool isSmall = false;
		int value = 0;
	};

	// Zap that differed from the preset
	struct ZapValue {
		std::string name;
		bool zapped = false;
	};

	std::string path;
	std::string dataFolder;
	std::string set;
	std::string preset;
	std::string time; // Local time with the offset to UTC, ISO 8601
	std::vector<SliderValue> sliders;
	std::vector<ZapValue> zaps;

	// Built with slider or zap values that differ from the preset
	bool IsModified() const { return !sliders.empty() || !zaps.empty(); }

	// Adds a slider or zap value of the build if it differs from the preset value (both 0-1)
	void AddPresetChange(const std::string& name, bool isSmall, bool zap, float value, float presetValue);

	// Absolute data folder with a trailing separator, so records of the same folder compare equal
	static std::string NormalizeFolder(const std::string& folder);
	static bool SameFolder(const std::string& a, const std::string& b);
};

class BuildSelection {
	std::map<std::string, std::string> outputChoice;
	std::map<ZapChoiceKey, bool> zapChoice;
	std::map<std::string, std::vector<BuildRecord>> buildRecords;

public:
	BuildSelection() {}
	BuildSelection(XMLElement* srcElement) { LoadBuildSelection(srcElement); }

	bool HasOutputPath(const std::string& search);
	bool HasZapChoice(const std::string& project, const std::string& zap);

	std::map<std::string, std::string> GetOutputChoices();
	std::string GetOutputChoice(const std::string& outputPath);
	void SetOutputChoice(const std::string& outputPath, const std::string& choice);
	void RemoveOutputChoice(const std::string& outputPath);

	std::map<ZapChoiceKey, bool> GetZapChoices();
	bool GetZapChoice(const std::string& project, const std::string& zap);
	void SetZapChoice(const std::string& project, const std::string& zap, bool choice);
	void RemoveZapChoice(const std::string& project, const std::string& zap);

	// Last build of the output path to the data folder, or nullptr if none was recorded
	const BuildRecord* GetBuildRecord(const std::string& outputPath, const std::string& dataFolder) const;

	int LoadBuildSelection(XMLElement* srcElement);
};


class BuildSelectionFile {
	XMLDocument doc;
	XMLElement* root = nullptr;
	int error = 0;

public:
	std::string fileName;
	BuildSelectionFile() {}
	BuildSelectionFile(const std::string& srcFileName);
	~BuildSelectionFile(){};

	bool fail() { return error != 0; }

	int GetError() { return error; }

	// Clears all data of the file
	void Clear();

	// Creates a new empty document structure
	void New(const std::string& newFileName);

	// Loads the XML document. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save build selections as a new file without altering the original.
	void Rename(const std::string& newFileName);

	// Writes the XML file using the internal fileName (use Rename() to change the name).
	bool Save();

	// Get the build selection in the file
	void Get(BuildSelection& outBuildSel);

	// Updates or adds output choices in the XML document
	int UpdateOutputChoices(BuildSelection& inBuildSel);

	// Updates or adds zap choices in the XML document
	int UpdateZapChoices(BuildSelection& inBuildSel);

	// Removes a single element from the XML document
	void RemoveOutputChoice(const std::string& path);

	// Removes a single element from the XML document
	void RemoveZapChoice(const std::string& project, const std::string& zap);

	// Adds or replaces build records, matched by output path and data folder
	void SetBuildRecords(const std::vector<BuildRecord>& records);

	// Removes the build records of the output paths in the data folder
	void RemoveBuildRecords(const std::vector<std::string>& paths, const std::string& dataFolder);
};
