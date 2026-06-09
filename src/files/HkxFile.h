/*
BodySlide and Outfit Studio
See the included LICENSE file

Native reader for Havok HKX packfile skeletons and spline-compressed
animations as shipped by Bethesda for Skyrim LE, Skyrim SE/VR and Fallout 4.

Supports:
  - Skyrim LE        hk_2010.2.0-r1, file version 8, 32-bit pointers
  - Skyrim SE / VR   hk_2010.2.0-r1, file version 8, 64-bit pointers
  - Fallout 4 / VR   hk_2014.1.0-r1, file version 11, 64-bit pointers

Only enough of the format is decoded to load skeletons (bones, parent
indices, local reference pose) and animations (per-frame TRS for every
transform track plus the track-to-bone binding). Physics, ragdolls,
floats and textual annotations are ignored.

Spline decompression is ported from PyNifly's anim_fo4.py (GPLv3),
which itself is derived from Dagobaking's skyrim-fo4-animation-conversion
and PredatorCZ/HavokLib (both GPLv3).
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace HKX {

enum class Format {
	Unknown,
	Skyrim32, // hk_2010, version 8, 32-bit pointers (Skyrim LE)
	Skyrim64, // hk_2010, version 8, 64-bit pointers (Skyrim SE/VR)
	Fallout64 // hk_2014, version 11, 64-bit pointers (Fallout 4/VR)
};

struct Bone {
	std::string name;
	int16_t parentIndex = -1;
	bool lockTranslation = false;
};

// Local-space TRS as stored in an hkQsTransform. Quaternion is xyzw.
struct Transform {
	float translation[3] = {0.0f, 0.0f, 0.0f};
	float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
};

struct Skeleton {
	std::string name;
	std::vector<Bone> bones;
	// Parallel to bones; size matches bones.size() (or empty if missing).
	std::vector<Transform> referencePose;
};

struct AnimationBinding {
	std::string originalSkeletonName;
	// Maps transform-track index -> bone index in the bound skeleton.
	// May be empty, in which case track index equals bone index.
	std::vector<int16_t> transformTrackToBoneIndices;
	int32_t blendHint = 0; // 0 = NORMAL, 1 = ADDITIVE
};

struct Animation {
	float duration = 0.0f;
	float frameDuration = 1.0f / 30.0f;
	uint32_t numFrames = 0;
	uint32_t numTransformTracks = 0;
	// Per-track names sourced from annotationTracks. May be empty if the
	// file has no annotation tracks.
	std::vector<std::string> trackNames;
	// Decompressed per-frame, per-track local TRS in row-major
	// [frame * numTransformTracks + track] order.
	// size() == numFrames * numTransformTracks.
	std::vector<Transform> trackTransforms;
	AnimationBinding binding;

	// Convenience accessor.
	const Transform& At(uint32_t frame, uint32_t track) const {
		return trackTransforms[frame * numTransformTracks + track];
	}
};

struct SaveAnimationOptions {
	std::string originalSkeletonName;
	std::string containerName;
};

class File {
public:
	// Parse the given .hkx file. Returns false on failure; if errorOut is
	// non-null it receives a short human-readable description.
	bool Load(const std::string& path, std::string* errorOut = nullptr);

	// Serialize a single-frame HKX animation packfile using static tracks.
	// `transforms` are written positionally as transform tracks 0..N-1.
	// The output format is selected explicitly via `format`.
	static bool SavePoseAnimation(const std::string& path,
							 Format format,
							 const std::vector<Transform>& transforms,
							 const SaveAnimationOptions& options = {},
							 std::string* errorOut = nullptr);

	Format GetFormat() const { return format; }
	const std::vector<Skeleton>& GetSkeletons() const { return skeletons; }
	const std::vector<Animation>& GetAnimations() const { return animations; }

private:
	Format format = Format::Unknown;
	std::vector<Skeleton> skeletons;
	std::vector<Animation> animations;
};

} // namespace HKX
