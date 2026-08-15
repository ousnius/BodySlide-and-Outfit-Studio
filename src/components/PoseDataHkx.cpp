/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../files/HkxFile.h"
#include "../utils/StringStuff.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

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

static void ConvertOutfitStudioScaleToHavok(const HKX::Skeleton& skel, PoseData& outPose);

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

	// Outfit Studio's skeleton scales a bone's translation by its parents',
	// Havok's does not, so a pose with scaled bones has to be restated in
	// Havok's terms before it is written out.
	PoseData havokPose = pose;
	ConvertOutfitStudioScaleToHavok(skeleton, havokPose);

	// Keyed lowercase: the pose carries the NIF skeleton's bone names, which
	// differ in case from the HKX skeleton's for a few Fallout 4 bones
	// (HEAD/Head, SPINE1/Spine1, SPINE2/Spine2, WEAPON/Weapon).
	std::unordered_map<std::string, const PoseBoneData*> poseBones;
	poseBones.reserve(havokPose.boneData.size());
	for (const auto& boneData : havokPose.boneData)
		poseBones[ToLower(boneData.name)] = &boneData;

	std::vector<HKX::Transform> trackTransforms(skeleton.bones.size());
	for (size_t boneIndex = 0; boneIndex < skeleton.bones.size(); ++boneIndex) {
		HKX::Transform track = (boneIndex < skeleton.referencePose.size()) ? skeleton.referencePose[boneIndex] : HKX::Transform{};
		auto it = poseBones.find(ToLower(skeleton.bones[boneIndex].name));
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

// Extracts one frame of the animation into outPose, matching transform
// tracks to skeleton bones via the animation binding (when present),
// otherwise positionally.
static bool ExtractHkxFramePose(const HKX::Skeleton& skel, const HKX::Animation& anim, uint32_t frame, PoseData& outPose) {
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
		// Bone scales are deliberately dropped; see
		// ConvertOutfitStudioScaleToHavok for why.
		bd.scale = 1.0f;
		outPose.boneData.push_back(std::move(bd));
	}

	return !outPose.boneData.empty();
}

// Havok and Outfit Studio disagree about what a bone's scale does. Havok
// composes bones with hkQsTransform, whose scale does NOT apply to a child's
// translation, while nifly's MatTransform - and so AnimBone::UpdatePoseTransform
// - multiplies a child's translation by its parent's scale. Exported poses carry
// the user's own scales, so their translations are restated in Havok's terms by
// multiplying in the ancestors' accumulated scale.
//
// The import direction needs no such correction because it drops HKX bone scales
// entirely (see ExtractHkxFramePose): with every scale at 1.0 the two conventions
// coincide. Nothing is lost, because Bethesda's scale tracks are reciprocal
// bookkeeping pairs meant to cancel out - measured over 471 shipped Skyrim
// animations, the scale accumulated at the skinned bones is exactly 1.0 in 465 of
// them and physically impossible in the rest. Applying what survives quantisation
// made the character pulse about 1% in size, far more once frames were blended
// across the sign changes some root tracks contain.
static void ConvertOutfitStudioScaleToHavok(const HKX::Skeleton& skel, PoseData& outPose) {
	const size_t nBones = skel.bones.size();

	// Keyed lowercase; see SaveHkxPoseInternal for why.
	std::unordered_map<std::string, size_t> boneIndices;
	boneIndices.reserve(nBones);
	for (size_t i = 0; i < nBones; ++i)
		boneIndices.emplace(ToLower(skel.bones[i].name), i);

	auto usableScale = [](float scale) { return scale > 0.0f && std::isfinite(scale); };

	// Local scale per bone: the posed value where the pose drives the bone, the
	// skeleton's reference scale everywhere else.
	std::vector<float> localScale(nBones, 1.0f);
	for (size_t i = 0; i < nBones && i < skel.referencePose.size(); ++i) {
		if (usableScale(skel.referencePose[i].scale[0]))
			localScale[i] = skel.referencePose[i].scale[0];
	}
	for (const PoseBoneData& bd : outPose.boneData) {
		auto it = boneIndices.find(ToLower(bd.name));
		if (it != boneIndices.end() && usableScale(bd.scale))
			localScale[it->second] = bd.scale;
	}

	// Accumulated scale of each bone's ancestors. Havok skeletons store bones
	// parent before child, so one forward pass covers the whole hierarchy.
	std::vector<float> ancestorScale(nBones, 1.0f);
	for (size_t i = 0; i < nBones; ++i) {
		int parent = skel.bones[i].parentIndex;
		if (parent >= 0 && size_t(parent) < i)
			ancestorScale[i] = ancestorScale[parent] * localScale[parent];
	}

	for (PoseBoneData& bd : outPose.boneData) {
		auto it = boneIndices.find(ToLower(bd.name));
		if (it == boneIndices.end())
			continue;

		const float scale = ancestorScale[it->second];
		if (usableScale(scale) && std::fabs(scale - 1.0f) > 1e-6f)
			bd.translation *= scale;
	}
}

// Collects the bones that carry the character through the world: the roots of
// the skeleton's hierarchies, which is the only place a translation moves the
// whole character rather than posing a part of it.
//
// Note that this deliberately stops at the root and does not walk down the
// chain of bones whose translation is unlocked. On the Bethesda humanoid
// skeletons that chain continues into NPC COM [COM ] (Skyrim) / COM (Fallout
// 4), and that bone is not world motion: it carries the body's vertical bob
// and weight shift over the planted feet. Freezing it pins the pelvis in
// space while the legs keep swinging, which makes the feet float and shuffle.
// Measured over Skyrim's and Fallout 4's shipped animations, the COM track
// spans several units vertically in a large share of them (Fallout 4: over 4.5
// units in a quarter of them) while its net start-to-end displacement is zero
// in all but a handful, so freezing it costs the pose everywhere and prevents
// drift almost nowhere.
static std::unordered_set<std::string> CollectRootMotionBoneNames(const HKX::Skeleton& skel) {
	std::unordered_set<std::string> names;

	for (const HKX::Bone& bone : skel.bones) {
		if (bone.parentIndex < 0)
			names.insert(bone.name);
	}

	return names;
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
	return ExtractHkxFramePose(skel, anim, frame, outPose);
}

bool PoseDataCollection::LoadHkxAnimation(const std::string& skeletonHkxPath, const std::string& animHkxPath, AnimationData& outAnim, std::string* errorOut) {
	std::string skelError;
	auto setError = [errorOut](const std::string& message, const std::string& detail = std::string()) {
		if (errorOut)
			*errorOut = detail.empty() ? message : message + "\n\n" + detail;
	};

	HKX::File skelFile;
	if (!skelFile.Load(skeletonHkxPath, &skelError) || skelFile.GetSkeletons().empty()) {
		setError("Failed to parse the HKX skeleton data.", skelError);
		return false;
	}

	std::string animError;
	HKX::File animFile;
	if (!animFile.Load(animHkxPath, &animError) || animFile.GetAnimations().empty()) {
		setError("Failed to parse the HKX animation data.", animError);
		return false;
	}

	const HKX::Skeleton& skel = skelFile.GetSkeletons().front();
	const HKX::Animation& anim = animFile.GetAnimations().front();

	if (anim.numTransformTracks == 0 || anim.numFrames == 0) {
		setError("The HKX animation does not contain any frames.");
		return false;
	}

	outAnim.frameDuration = (anim.frameDuration > 0.0f && std::isfinite(anim.frameDuration)) ? anim.frameDuration : 1.0f / 30.0f;
	outAnim.framePoses.clear();
	outAnim.framePoses.resize(anim.numFrames);

	for (uint32_t frame = 0; frame < anim.numFrames; ++frame) {
		outAnim.framePoses[frame].name = outAnim.name;
		if (!ExtractHkxFramePose(skel, anim, frame, outAnim.framePoses[frame])) {
			setError("The HKX animation tracks could not be matched to the skeleton bones.");
			return false;
		}
	}

	// Strip root motion: in a mesh editor the character walking out of the
	// viewport is only in the way. Freezing the root motion bones at their
	// frame 0 translation keeps the animation's starting placement without the
	// world movement it would apply on top. Everything below the root is left
	// alone so the body still animates over its feet.
	const std::unordered_set<std::string> rootMotionBones = CollectRootMotionBoneNames(skel);
	if (!rootMotionBones.empty() && outAnim.framePoses.size() > 1) {
		std::unordered_map<std::string, nifly::Vector3> firstFrameTranslations;
		for (const PoseBoneData& bd : outAnim.framePoses.front().boneData) {
			if (rootMotionBones.count(bd.name))
				firstFrameTranslations[bd.name] = bd.translation;
		}

		for (size_t frame = 1; frame < outAnim.framePoses.size(); ++frame) {
			for (PoseBoneData& bd : outAnim.framePoses[frame].boneData) {
				auto it = firstFrameTranslations.find(bd.name);
				if (it != firstFrameTranslations.end())
					bd.translation = it->second;
			}
		}
	}

	return true;
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
