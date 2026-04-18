/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../utils/PlatformUtil.h"

#include <wx/filename.h>

#include <fkYAML/node.hpp>

#include <cmath>
#include <fstream>

namespace {
// SAM pose YAML reader built on top of the fkYAML single-header library.
// The file format produced by ScreenArcherMenu is a small top-level mapping
// with at least a `transforms:` node. Each entry in `transforms` is keyed by
// a bone name and maps to a mapping of the scalar properties tx, ty, tz,
// rx, ry, rz, s. Other top-level keys of interest are `type` (must be
// `rltv` for relative poses) and `rotation` (Euler order, e.g. `exyz`).

static nifly::Matrix3 MakeAxisRotation(char axis, float angleRad) {
	float c = std::cos(angleRad);
	float s = std::sin(angleRad);
	switch (axis) {
	case 'x':
	case 'X':
		return nifly::Matrix3(1.0f, 0.0f, 0.0f, 0.0f, c, -s, 0.0f, s, c);
	case 'y':
	case 'Y':
		return nifly::Matrix3(c, 0.0f, s, 0.0f, 1.0f, 0.0f, -s, 0.0f, c);
	case 'z':
	case 'Z':
		return nifly::Matrix3(c, -s, 0.0f, s, c, 0.0f, 0.0f, 0.0f, 1.0f);
	}
	return nifly::Matrix3();
}

// Convert Euler angles (degrees) to an axis-angle rotation vector.
// The rotation order string is the SAM convention, e.g. "exyz" (leading
// optional 'e' for Euler). The three axis letters specify the intrinsic
// rotation order; the resulting matrix is R_axis1 * R_axis2 * R_axis3.
static nifly::Vector3 SamEulerToRotVec(float rxDeg, float ryDeg, float rzDeg, const std::string& order) {
	std::string ord = order;
	if (!ord.empty() && (ord.front() == 'e' || ord.front() == 'E'))
		ord.erase(0, 1);
	if (ord.size() < 3)
		ord = "xyz";

	const float deg2rad = 3.14159265358979323846f / 180.0f;
	nifly::Matrix3 mats[3];
	for (int i = 0; i < 3; ++i) {
		char a = ord[i];
		float ang = 0.0f;
		if (a == 'x' || a == 'X')
			ang = rxDeg * deg2rad;
		else if (a == 'y' || a == 'Y')
			ang = ryDeg * deg2rad;
		else if (a == 'z' || a == 'Z')
			ang = rzDeg * deg2rad;
		mats[i] = MakeAxisRotation(a, ang);
	}

	nifly::Matrix3 m = mats[0] * mats[1] * mats[2];
	return nifly::RotMatToVec(m);
}

static float GetYamlFloat(const fkyaml::node& map, const char* key, float defaultValue = 0.0f) {
	if (!map.is_mapping() || !map.contains(key))
		return defaultValue;
	const fkyaml::node& n = map[key];
	try {
		return n.get_value<float>();
	}
	catch (...) {
		return defaultValue;
	}
}

static std::string GetYamlString(const fkyaml::node& map, const char* key, const std::string& defaultValue = {}) {
	if (!map.is_mapping() || !map.contains(key))
		return defaultValue;
	const fkyaml::node& n = map[key];
	try {
		return n.get_value<std::string>();
	}
	catch (...) {
		return defaultValue;
	}
}

static bool ParseSamPoseYaml(const std::string& filePath, PoseData& outPose) {
	fkyaml::node root;
	try {
#ifdef _WINDOWS
		std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
		std::ifstream ifs(winFileName.c_str());
#else
		std::ifstream ifs(filePath.c_str());
#endif
		if (!ifs.is_open())
			return false;
		root = fkyaml::node::deserialize(ifs);
	}
	catch (...) {
		return false;
	}

	if (!root.is_mapping())
		return false;

	// SAM relative transforms map directly to Outfit Studio's relative pose
	// concept. If a file specifies something else (e.g. absolute), skip it
	// to avoid producing wrong results.
	std::string poseType = GetYamlString(root, "type");
	if (!poseType.empty() && poseType != "rltv")
		return false;

	std::string rotationOrder = GetYamlString(root, "rotation", "exyz");

	if (!root.contains("transforms"))
		return false;

	const fkyaml::node& transforms = root["transforms"];
	if (!transforms.is_mapping())
		return false;

	try {
		for (auto kv : transforms.map_items()) {
			const fkyaml::node& bone = kv.value();
			if (!bone.is_mapping())
				continue;

			PoseBoneData bd{};
			bd.name = kv.key().get_value<std::string>();
			bd.translation.x = GetYamlFloat(bone, "tx");
			bd.translation.y = GetYamlFloat(bone, "ty");
			bd.translation.z = GetYamlFloat(bone, "tz");
			float rxDeg = GetYamlFloat(bone, "rx");
			float ryDeg = GetYamlFloat(bone, "ry");
			float rzDeg = GetYamlFloat(bone, "rz");
			bd.rotation = SamEulerToRotVec(rxDeg, ryDeg, rzDeg, rotationOrder);
			bd.scale = GetYamlFloat(bone, "s", 1.0f);

			outPose.boneData.push_back(std::move(bd));
		}
	}
	catch (...) {
		return false;
	}

	return !outPose.boneData.empty();
}
} // namespace

int PoseDataCollection::LoadYamlData(const std::string& basePath, const std::string& namePrefix) {
	wxString wxBase = wxString::FromUTF8(basePath.c_str());
	if (!wxDirExists(wxBase))
		return 0;

	wxArrayString files;
	wxDir::GetAllFiles(wxBase, &files, "*.yaml", wxDIR_FILES);
	wxDir::GetAllFiles(wxBase, &files, "*.yml", wxDIR_FILES);

	int loaded = 0;
	for (auto& file : files) {
		PoseData pd;
		pd.readOnly = true;
		wxFileName fn(file);
		pd.name = namePrefix + std::string(fn.GetName().ToUTF8().data());

		if (ParseSamPoseYaml(file.ToUTF8().data(), pd)) {
			AddPose(std::move(pd));
			++loaded;
		}
	}

	return loaded;
}
