/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseData.h"
#include "../files/HkxFile.h"

#include <algorithm>
#include <cmath>

namespace {
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
