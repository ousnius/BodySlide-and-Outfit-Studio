/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include <map>
#include <wx/dir.h>

using namespace tinyxml2;

typedef std::pair<std::string, std::string> ZapChoiceKey;

class BuildSelection {
	std::map<std::string, std::string> outputChoice;
	std::map<ZapChoiceKey, bool> zapChoice;

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
};
