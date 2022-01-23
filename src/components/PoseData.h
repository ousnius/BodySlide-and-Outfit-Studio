/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include "Object3d.hpp"

#include <map>
#include <set>
#include <unordered_set>
#include <vector>
#include <wx/dir.h>

using namespace tinyxml2;

struct PoseBoneData {
	std::string name;
	nifly::Vector3 rotation;
	nifly::Vector3 translation;
};

class PoseData {
public:
	std::string name;
	std::vector<PoseBoneData> boneData;

	PoseData() {}

	PoseData(const std::string& name) { this->name = name; }

	PoseData(const std::string& name, const std::vector<PoseBoneData>& boneData) {
		this->name = name;
		this->boneData = boneData;
	}

	PoseData(XMLElement* srcElement) { LoadElement(srcElement); }

	bool LoadElement(XMLElement* srcElement);
	void WriteElement(XMLElement* element, bool append = false) const;
};

class PoseDataCollection {
public:
	std::vector<PoseData> poseData;

	// Loads all pose data in the specified folder.
	int LoadData(const std::string& basePath);
};

class PoseDataFile {
	XMLDocument doc;
	XMLElement* root = nullptr;
	int error = 0;

public:
	std::string fileName;

	PoseDataFile() {}
	PoseDataFile(const std::string& srcFileName);
	~PoseDataFile() {}

	bool fail() { return error != 0; }
	int GetError() { return error; }

	// Loads the XML document and identifies included pose. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Creates a new empty pose document structure, ready to add new pose to.
	void New(const std::string& newFileName);

	// Clears all data of the file.
	void Clear();

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save a pose as a new file without altering the original.
	void Rename(const std::string& newFileName);

	// Updates data in the XML document with the provided information.
	int SetData(const std::vector<PoseData>& data);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	bool Save();

	// Reads data of the pose element.
	int GetData(std::vector<PoseData>& outData);
};
