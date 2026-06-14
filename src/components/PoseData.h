/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include "../files/HkxFile.h"
#include "Object3d.hpp"

class AnimBone;

#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include <wx/dir.h>

using namespace tinyxml2;

enum class PoseFileFormat {
	Unknown,
	Hkx,
	Json,
	Yaml,
};

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
	// When true, the pose was loaded from a read-only source (e.g. a SAM
	// YAML file) and must not be modified or deleted by Outfit Studio.
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

	// Applies this pose to all named bones in the skeleton. For each bone,
	// if a matching PoseBoneData entry exists, sets poseRotVec/poseTranVec/
	// poseScale (converting from absolute local-to-parent when absoluteLocal
	// is set). Bones without a matching entry are reset to the identity pose.
	// Calls UpdatePoseTransform on every bone.
	void ApplyToSkeleton() const;
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

	// Returns the on-disk pose format inferred from the file extension.
	static PoseFileFormat GetPoseFileFormat(const std::string& filePath);

	// Sanitizes a pose name for safe use as a Windows file stem.
	static std::string SanitizeFileStem(const std::string& name);

	// Captures the current live skeleton pose into outPose.
	// When absoluteLocal is true, transforms are stored in local-to-parent
	// space; otherwise Outfit-Studio relative deltas are stored.
	static void CaptureCurrentPose(const std::string& poseName, bool absoluteLocal, PoseData& outPose);

	// Loads all SAM pose YAML files from the specified folder (recursively).
	// The pose name is derived from the file name (without extension) and is
	// prefixed with namePrefix. Entries are appended to poseData.
	int LoadYamlData(const std::string& basePath, const std::string& namePrefix);

	// Loads all SAF pose JSON files from the specified folder (recursively).
	// SAF is the Fallout 4 companion of SAM, using a different on-disk format
	// (JSON instead of YAML) with yaw/pitch/roll rotation fields in degrees.
	// The pose name is derived from the file name (without extension) and is
	// prefixed with namePrefix. Entries are appended to poseData.
	int LoadJsonData(const std::string& basePath, const std::string& namePrefix);

	// Loads a single SAM pose YAML file into outPose.
	static bool LoadYamlPose(const std::string& filePath, PoseData& outPose);

	// Loads a single SAF/SAM pose JSON file into outPose.
	static bool LoadJsonPose(const std::string& filePath, PoseData& outPose);

	// Saves pose data in SAM pose YAML format.
	static bool SaveYamlPose(const std::string& filePath, const PoseData& pose);

	// Saves pose data in SAF pose JSON format.
	static bool SaveJsonPose(const std::string& filePath, const PoseData& pose);

	// Loads a single pose file based on its extension.
	// HKX files require skeletonHkxPath to point at the matching skeleton.
	// On failure, errorOut receives a short human-readable error.
	static bool LoadPoseFile(const std::string& filePath,
						 PoseData& outPose,
						 const std::string& skeletonHkxPath = std::string(),
						 std::string* errorOut = nullptr);

	// Saves a single pose file based on its extension.
	// HKX files require skeletonHkxPath and hkxFormat to be supplied.
	// On failure, errorOut receives a short human-readable error.
	static bool SavePoseFile(const std::string& filePath,
						 const PoseData& pose,
						 const std::string& skeletonHkxPath = std::string(),
						 HKX::Format hkxFormat = HKX::Format::Unknown,
						 std::string* errorOut = nullptr);

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
