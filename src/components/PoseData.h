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
	float scale;
};

class PoseData {
public:
	std::string name;
	std::vector<PoseBoneData> boneData;
	bool readOnly = false;
	// When true, rotation/translation/scale in boneData are the absolute
	// local-to-parent transform at frame 0 (as stored by Havok HKX), not an
	// Outfit-Studio-style delta on top of the bind pose. The caller must
	// convert to a delta against the live skeleton's xformToParent before
	// assigning poseRotVec/poseTranVec/poseScale.
	bool absoluteLocal = false;

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
	// Stored as a deque so that pointers/references to individual entries
	// remain stable across subsequent insertions. The combobox in Outfit
	// Studio holds raw PoseData* in its ClientData; with a vector every
	// push_back would invalidate every previously stored pointer.
	std::deque<PoseData> poseData;

	// Loads all pose data in the specified folder.
	int LoadData(const std::string& basePath);

	// Appends a pose to the collection and returns a stable pointer to it.
	PoseData* AddPose(PoseData pose);

	// Loads a single pose from a Havok HKX skeleton + animation pair.
	// Both files are parsed natively (no external tools required) for all
	// supported variants: Skyrim LE, Skyrim SE/VR and Fallout 4. The bones
	// parsed from skeletonHkxPath are matched to the animation's transform
	// tracks via the animation binding (when present), otherwise positionally.
	// `frameIndex` selects which frame of the animation to extract. On
	// success, outPose.boneData is populated and absoluteLocal is set.
	// Returns false if either file cannot be parsed.
	static bool LoadHkxPose(const std::string& skeletonHkxPath, const std::string& animHkxPath, PoseData& outPose, uint32_t frameIndex = 0);
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
