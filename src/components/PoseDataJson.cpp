/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../utils/PlatformUtil.h"

#include <wx/filename.h>

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>

namespace {
// SAF pose JSON reader modeled after ScreenArcherMenu's SAF/io.cpp
// (ReadTransformJson / LoadPosePath). A SAF pose file is a JSON document
// containing:
//   - "version" (unsigned int, optional; 0 means legacy transposed format)
//   - "skeleton", "name" (strings, ignored here)
//   - "transforms" (object mapping bone name -> transform object)
// Each transform object has the optional fields x, y, z (translation),
// yaw, pitch, roll (rotation Euler angles in degrees) and scale (float,
// default 1.0). If the file has no "transforms" key, the root object is
// treated as the bone map directly (older files).

static float GetJsonFloat(const nlohmann::json& obj, const char* key, float defaultValue = 0.0f) {
	if (!obj.is_object())
		return defaultValue;
	auto it = obj.find(key);
	if (it == obj.end())
		return defaultValue;
	try {
		if (it->is_number())
			return it->get<float>();
		if (it->is_string())
			return std::stof(it->get<std::string>());
	}
	catch (...) {
	}
	return defaultValue;
}

// Convert SAF yaw/pitch/roll (degrees) to an axis-angle rotation vector,
// reproducing SAM/SAF's MatrixFromEulerYPR (version >= 1) and the legacy
// transposed MatrixFromEulerYPRTransposed with negated angles (version 0,
// used by MatrixFromDegree). Both variants are taken directly from
// ScreenArcherMenu's SAF/conversions.cpp.
static nifly::Vector3 SafEulerToRotVec(float yawDeg, float pitchDeg, float rollDeg, unsigned int version) {
	const float deg2rad = 3.14159265358979323846f / 180.0f;

	float x = yawDeg * deg2rad;
	float y = pitchDeg * deg2rad;
	float z = rollDeg * deg2rad;

	if (version == 0) {
		// Legacy MatrixFromDegree -> MatrixFromEulerYPRTransposed with negated angles.
		x = -x;
		y = -y;
		z = -z;
	}

	float sinX = std::sin(x);
	float cosX = std::cos(x);
	float sinY = std::sin(y);
	float cosY = std::cos(y);
	float sinZ = std::sin(z);
	float cosZ = std::cos(z);

	nifly::Matrix3 m;
	if (version == 0) {
		// SAF::MatrixFromEulerYPRTransposed layout.
		m[0][0] = cosY * cosZ;
		m[0][1] = -cosY * sinZ;
		m[0][2] = sinY;
		m[1][0] = sinX * sinY * cosZ + sinZ * cosX;
		m[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		m[1][2] = -sinX * cosY;
		m[2][0] = sinX * sinZ - cosX * sinY * cosZ;
		m[2][1] = cosX * sinY * sinZ + sinX * cosZ;
		m[2][2] = cosX * cosY;
	}
	else {
		// SAF::MatrixFromEulerYPR layout. SAF stores this into an
		// NiMatrix43 whose `data[i][j]` is indexed as [column][row]
		// (see RotateMatrix in SAF/conversions.cpp: out.x accumulates
		// data[0][0]*x + data[1][0]*y + data[2][0]*z, meaning
		// M[0][j] == data[j][0]). Converting the raw assignments via
		// that mapping yields the standard row-major rotation matrix
		// below. Copying them verbatim into a row-major Matrix3 would
		// store the transpose, i.e. the inverse rotation.
		m[0][0] = cosY * cosZ;
		m[0][1] = -cosY * sinZ;
		m[0][2] = sinY;
		m[1][0] = sinX * sinY * cosZ + sinZ * cosX;
		m[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		m[1][2] = -sinX * cosY;
		m[2][0] = sinX * sinZ - cosX * sinY * cosZ;
		m[2][1] = cosX * sinY * sinZ + sinX * cosZ;
		m[2][2] = cosX * cosY;
	}

	return nifly::RotMatToVec(m);
}

static bool ParseSafPoseJson(const std::string& filePath, PoseData& outPose) {
	nlohmann::json root;
	try {
#ifdef _WINDOWS
		std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(filePath);
		std::ifstream ifs(winFileName.c_str());
#else
		std::ifstream ifs(filePath.c_str());
#endif
		if (!ifs.is_open())
			return false;
		root = nlohmann::json::parse(ifs, nullptr, false, true);
	}
	catch (...) {
		return false;
	}

	if (root.is_null() || root.is_discarded() || !root.is_object())
		return false;

	unsigned int version = 0;
	auto versionIt = root.find("version");
	if (versionIt != root.end() && versionIt->is_number_unsigned()) {
		try {
			version = versionIt->get<unsigned int>();
		}
		catch (...) {
			version = 0;
		}
	}

	// Newer files wrap bones in a "transforms" object. Older files put the
	// bone map at the root directly.
	const nlohmann::json* transforms = &root;
	auto transformsIt = root.find("transforms");
	if (transformsIt != root.end() && transformsIt->is_object())
		transforms = &(*transformsIt);

	if (!transforms->is_object())
		return false;

	try {
		for (auto it = transforms->begin(); it != transforms->end(); ++it) {
			if (!it.value().is_object())
				continue;

			// Skip non-bone metadata keys that appear on older root-level files.
			const std::string& boneName = it.key();
			if (boneName == "version" || boneName == "skeleton" || boneName == "name" || boneName == "type")
				continue;

			PoseBoneData bd{};
			bd.name = boneName;
			bd.translation.x = GetJsonFloat(it.value(), "x");
			bd.translation.y = GetJsonFloat(it.value(), "y");
			bd.translation.z = GetJsonFloat(it.value(), "z");
			float yawDeg = GetJsonFloat(it.value(), "yaw");
			float pitchDeg = GetJsonFloat(it.value(), "pitch");
			float rollDeg = GetJsonFloat(it.value(), "roll");
			bd.rotation = SafEulerToRotVec(yawDeg, pitchDeg, rollDeg, version);
			bd.scale = GetJsonFloat(it.value(), "scale", 1.0f);

			outPose.boneData.push_back(std::move(bd));
		}
	}
	catch (...) {
		return false;
	}

	return !outPose.boneData.empty();
}
} // namespace

int PoseDataCollection::LoadJsonData(const std::string& basePath, const std::string& namePrefix) {
	wxString wxBase = wxString::FromUTF8(basePath.c_str());
	if (!wxDirExists(wxBase))
		return 0;

	wxArrayString files;
	wxDir::GetAllFiles(wxBase, &files, "*.json", wxDIR_FILES | wxDIR_DIRS);

	int loaded = 0;
	for (auto& file : files) {
		PoseData pd;
		pd.readOnly = true;
		pd.absoluteLocal = true;
		wxFileName fn(file);
		pd.name = namePrefix + std::string(fn.GetName().ToUTF8().data());

		if (ParseSafPoseJson(file.ToUTF8().data(), pd)) {
			AddPose(std::move(pd));
			++loaded;
		}
	}

	return loaded;
}
