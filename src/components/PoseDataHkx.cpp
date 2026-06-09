/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../files/HkxFile.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <wx/filename.h>

namespace {

struct HkxQuaternion {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 1.0f;
};

// Convert a unit quaternion (w, x, y, z) to a standard column-vector
// rotation matrix (R such that R*v rotates v by the quaternion). This
// matches what nifly stores in AnimBone::xformToParent.rotation (populated
// directly from the NIF file's 3x3 rotation matrix, which is also a
// standard column-vector rotation). It does NOT match the internal
// convention of nifly::RotVecToMat / RotMatToVec, but those are only used
// to serialize a rotation to/from a rotation-vector representation; when
// combined via MatTransform::ComposeTransforms the stored matrices are
// multiplied as standard rotations, so the two sides of the composition
// must agree on that convention.
static nifly::Matrix3 QuatToNiflyMat(float qw, float qx, float qy, float qz) {
	float w = qw, x = qx, y = qy, z = qz;

	float len = std::sqrt(w * w + x * x + y * y + z * z);
	if (len <= 1e-8f)
		return nifly::Matrix3();
	w /= len;
	x /= len;
	y /= len;
	z /= len;

	const float xx = x * x;
	const float yy = y * y;
	const float zz = z * z;
	const float xy = x * y;
	const float xz = x * z;
	const float yz = y * z;
	const float wx = w * x;
	const float wy = w * y;
	const float wz = w * z;

	nifly::Matrix3 m;
	m[0][0] = 1.0f - 2.0f * (yy + zz);
	m[0][1] = 2.0f * (xy - wz);
	m[0][2] = 2.0f * (xz + wy);
	m[1][0] = 2.0f * (xy + wz);
	m[1][1] = 1.0f - 2.0f * (xx + zz);
	m[1][2] = 2.0f * (yz - wx);
	m[2][0] = 2.0f * (xz - wy);
	m[2][1] = 2.0f * (yz + wx);
	m[2][2] = 1.0f - 2.0f * (xx + yy);
	return m;
}

static HkxQuaternion MatrixToHkxQuaternion(const nifly::Matrix3& matrix) {
	HkxQuaternion quat;

	const float trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
	if (trace > 0.0f) {
		const float scale = std::sqrt(trace + 1.0f) * 2.0f;
		quat.w = 0.25f * scale;
		quat.x = (matrix[2][1] - matrix[1][2]) / scale;
		quat.y = (matrix[0][2] - matrix[2][0]) / scale;
		quat.z = (matrix[1][0] - matrix[0][1]) / scale;
	}
	else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2]) {
		const float scale = std::sqrt(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2.0f;
		quat.w = (matrix[2][1] - matrix[1][2]) / scale;
		quat.x = 0.25f * scale;
		quat.y = (matrix[0][1] + matrix[1][0]) / scale;
		quat.z = (matrix[0][2] + matrix[2][0]) / scale;
	}
	else if (matrix[1][1] > matrix[2][2]) {
		const float scale = std::sqrt(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2.0f;
		quat.w = (matrix[0][2] - matrix[2][0]) / scale;
		quat.x = (matrix[0][1] + matrix[1][0]) / scale;
		quat.y = 0.25f * scale;
		quat.z = (matrix[1][2] + matrix[2][1]) / scale;
	}
	else {
		const float scale = std::sqrt(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2.0f;
		quat.w = (matrix[1][0] - matrix[0][1]) / scale;
		quat.x = (matrix[0][2] + matrix[2][0]) / scale;
		quat.y = (matrix[1][2] + matrix[2][1]) / scale;
		quat.z = 0.25f * scale;
	}

	const float length = std::sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
	if (length <= 1e-8f)
		return HkxQuaternion();

	const float invLength = 1.0f / length;
	quat.x *= invLength;
	quat.y *= invLength;
	quat.z *= invLength;
	quat.w *= invLength;
	return quat;
}

static bool SaveHkxPoseInternal(const std::string& skeletonHkxPath,
							 const std::string& poseHkxPath,
							 HKX::Format hkxFormat,
							 const PoseData& pose,
							 std::string* errorOut) {
	if (hkxFormat == HKX::Format::Unknown) {
		if (errorOut)
			*errorOut = "Saving HKX poses is not supported for the current target game.";
		return false;
	}

	if (skeletonHkxPath.empty()) {
		if (errorOut)
			*errorOut = "No HKX skeleton path was provided for pose export.";
		return false;
	}

	if (!pose.absoluteLocal) {
		if (errorOut)
			*errorOut = "HKX export requires absolute local pose data.";
		return false;
	}

	std::string skeletonError;
	HKX::File skeletonFile;
	if (!skeletonFile.Load(skeletonHkxPath, &skeletonError) || skeletonFile.GetSkeletons().empty()) {
		if (errorOut)
			*errorOut = skeletonError.empty() ? "Failed to parse the HKX skeleton data." : "Failed to parse the HKX skeleton data.\n\n" + skeletonError;
		return false;
	}

	const HKX::Skeleton& skeleton = skeletonFile.GetSkeletons().front();
	if (skeleton.bones.empty()) {
		if (errorOut)
			*errorOut = "The configured HKX skeleton does not contain any bones.";
		return false;
	}

	std::unordered_map<std::string, const PoseBoneData*> poseBones;
	poseBones.reserve(pose.boneData.size());
	for (const auto& boneData : pose.boneData)
		poseBones[boneData.name] = &boneData;

	std::vector<HKX::Transform> trackTransforms(skeleton.bones.size());
	for (size_t boneIndex = 0; boneIndex < skeleton.bones.size(); ++boneIndex) {
		HKX::Transform track = (boneIndex < skeleton.referencePose.size()) ? skeleton.referencePose[boneIndex] : HKX::Transform{};
		auto it = poseBones.find(skeleton.bones[boneIndex].name);
		if (it != poseBones.end()) {
			const PoseBoneData& boneData = *it->second;
			HkxQuaternion quat = MatrixToHkxQuaternion(nifly::RotVecToMat(boneData.rotation));
			track.translation[0] = boneData.translation.x;
			track.translation[1] = boneData.translation.y;
			track.translation[2] = boneData.translation.z;
			track.rotation[0] = quat.x;
			track.rotation[1] = quat.y;
			track.rotation[2] = quat.z;
			track.rotation[3] = quat.w;
			float scale = (boneData.scale != 0.0f) ? boneData.scale : 1.0f;
			track.scale[0] = scale;
			track.scale[1] = scale;
			track.scale[2] = scale;
		}
		trackTransforms[boneIndex] = track;
	}

	HKX::SaveAnimationOptions saveOptions;
	saveOptions.originalSkeletonName = skeleton.name;
	saveOptions.containerName = "Merged Animation Container";

	std::string saveError;
	if (!HKX::File::SavePoseAnimation(poseHkxPath, hkxFormat, trackTransforms, saveOptions, &saveError)) {
		if (errorOut)
			*errorOut = saveError.empty() ? "Failed to serialize the HKX pose data." : "Failed to serialize the HKX pose data.\n\n" + saveError;
		return false;
	}

	return true;
}
} // namespace

bool PoseDataCollection::LoadHkxPose(const std::string& skeletonHkxPath, const std::string& animHkxPath, PoseData& outPose, uint32_t frameIndex) {
	HKX::File skelFile;
	if (!skelFile.Load(skeletonHkxPath, nullptr) || skelFile.GetSkeletons().empty())
		return false;

	HKX::File animFile;
	if (!animFile.Load(animHkxPath, nullptr) || animFile.GetAnimations().empty())
		return false;

	const HKX::Skeleton& skel = skelFile.GetSkeletons().front();
	const HKX::Animation& anim = animFile.GetAnimations().front();

	if (anim.numTransformTracks == 0 || anim.numFrames == 0)
		return false;

	uint32_t frame = std::min(frameIndex, anim.numFrames - 1);

	outPose.boneData.clear();
	outPose.absoluteLocal = true;

	const size_t nTracks = anim.numTransformTracks;
	const size_t nBones = skel.bones.size();
	const auto& binding = anim.binding.transformTrackToBoneIndices;

	outPose.boneData.reserve(nTracks);

	for (size_t track = 0; track < nTracks; ++track) {
		// Resolve the bone this track addresses. If a binding map is
		// provided, use it; otherwise fall back to positional matching.
		size_t boneIdx;
		if (!binding.empty() && track < binding.size()) {
			int16_t bi = binding[track];
			if (bi < 0 || size_t(bi) >= nBones)
				continue;
			boneIdx = size_t(bi);
		}
		else {
			if (track >= nBones)
				break;
			boneIdx = track;
		}

		const HKX::Transform& xf = anim.At(frame, uint32_t(track));

		PoseBoneData bd{};
		bd.name = skel.bones[boneIdx].name;
		bd.translation = nifly::Vector3(xf.translation[0], xf.translation[1], xf.translation[2]);
		// HKX quaternion is xyzw; QuatToNiflyMat takes (w, x, y, z).
		bd.rotation = nifly::RotMatToVec(QuatToNiflyMat(xf.rotation[3], xf.rotation[0], xf.rotation[1], xf.rotation[2]));
		bd.scale = (xf.scale[0] != 0.0f) ? xf.scale[0] : 1.0f;
		outPose.boneData.push_back(std::move(bd));
	}

	return !outPose.boneData.empty();
}

bool PoseDataCollection::LoadPoseFile(const std::string& filePath,
						 PoseData& outPose,
						 const std::string& skeletonHkxPath,
						 std::string* errorOut) {
	PoseFileFormat format = GetPoseFileFormat(filePath);
	wxFileName fileName(wxString::FromUTF8(filePath.c_str()));

	switch (format) {
	case PoseFileFormat::Hkx:
		if (skeletonHkxPath.empty()) {
			if (errorOut)
				*errorOut = "No HKX skeleton path was provided for pose import.";
			return false;
		}
		outPose.name = std::string("HKX: ") + std::string(fileName.GetName().ToUTF8().data());
		if (!LoadHkxPose(skeletonHkxPath, filePath, outPose)) {
			if (errorOut)
				*errorOut = "Failed to parse the HKX pose data.";
			return false;
		}
		return true;

	case PoseFileFormat::Json:
		outPose.name = std::string("SAM: ") + std::string(fileName.GetName().ToUTF8().data());
		if (!LoadJsonPose(filePath, outPose)) {
			if (errorOut)
				*errorOut = "Failed to parse the SAM JSON pose data.";
			return false;
		}
		return true;

	case PoseFileFormat::Yaml:
		outPose.name = std::string("SAM: ") + std::string(fileName.GetName().ToUTF8().data());
		if (!LoadYamlPose(filePath, outPose)) {
			if (errorOut)
				*errorOut = "Failed to parse the SAM YAML pose data.";
			return false;
		}
		return true;

	default:
		if (errorOut)
			*errorOut = "Please choose a pose file with a .hkx, .json, .yaml or .yml extension.";
		return false;
	}
}

bool PoseDataCollection::SavePoseFile(const std::string& filePath,
						 const PoseData& pose,
						 const std::string& skeletonHkxPath,
						 HKX::Format hkxFormat,
						 std::string* errorOut) {
	switch (GetPoseFileFormat(filePath)) {
	case PoseFileFormat::Hkx:
		return SaveHkxPoseInternal(skeletonHkxPath, filePath, hkxFormat, pose, errorOut);

	case PoseFileFormat::Json:
		if (!SaveJsonPose(filePath, pose)) {
			if (errorOut)
				*errorOut = "Failed to serialize the SAM JSON pose data.";
			return false;
		}
		return true;

	case PoseFileFormat::Yaml:
		if (!SaveYamlPose(filePath, pose)) {
			if (errorOut)
				*errorOut = "Failed to serialize the SAM YAML pose data.";
			return false;
		}
		return true;

	default:
		if (errorOut)
			*errorOut = "Please save the pose with a .hkx, .json, .yaml or .yml extension.";
		return false;
	}
}
