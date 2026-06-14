#include "../src/files/HkxFile.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {

HKX::Transform MakeTransform(float tx, float ty, float tz, float qx, float qy, float qz, float qw, float sx, float sy, float sz) {
	HKX::Transform transform{};
	transform.translation[0] = tx;
	transform.translation[1] = ty;
	transform.translation[2] = tz;
	transform.rotation[0] = qx;
	transform.rotation[1] = qy;
	transform.rotation[2] = qz;
	transform.rotation[3] = qw;
	transform.scale[0] = sx;
	transform.scale[1] = sy;
	transform.scale[2] = sz;
	return transform;
}

void RequireFloatNear(float actual, float expected, const char* label) {
	INFO(label);
	REQUIRE(std::fabs(actual - expected) < 1e-4f);
}

void RequireQuaternionNear(const float actual[4], const float expected[4]) {
	float aligned[4] = {actual[0], actual[1], actual[2], actual[3]};
	float dot = aligned[0] * expected[0] + aligned[1] * expected[1] + aligned[2] * expected[2] + aligned[3] * expected[3];
	if (dot < 0.0f) {
		aligned[0] = -aligned[0];
		aligned[1] = -aligned[1];
		aligned[2] = -aligned[2];
		aligned[3] = -aligned[3];
	}

	RequireFloatNear(aligned[0], expected[0], "rotation x");
	RequireFloatNear(aligned[1], expected[1], "rotation y");
	RequireFloatNear(aligned[2], expected[2], "rotation z");
	RequireFloatNear(aligned[3], expected[3], "rotation w");
}

void RequireTransformNear(const HKX::Transform& actual, const HKX::Transform& expected) {
	RequireFloatNear(actual.translation[0], expected.translation[0], "translation x");
	RequireFloatNear(actual.translation[1], expected.translation[1], "translation y");
	RequireFloatNear(actual.translation[2], expected.translation[2], "translation z");
	RequireQuaternionNear(actual.rotation, expected.rotation);
	RequireFloatNear(actual.scale[0], expected.scale[0], "scale x");
	RequireFloatNear(actual.scale[1], expected.scale[1], "scale y");
	RequireFloatNear(actual.scale[2], expected.scale[2], "scale z");
}

std::string MakeTempPath(const std::string& suffix) {
	auto dir = std::filesystem::temp_directory_path();
	auto file = dir / ("bsos_hkx_pose_" + suffix + ".hkx");
	return file.string();
}

} // namespace

TEST_CASE("HKX pose animation serialization round-trips supported formats", "[HkxFile]") {
	const float halfAngle = 0.35f;
	const float sinHalf = std::sin(halfAngle);
	const float cosHalf = std::cos(halfAngle);

	const std::vector<HKX::Transform> tracks = {
		MakeTransform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
		MakeTransform(1.25f, -2.5f, 3.75f, 0.0f, 0.0f, sinHalf, cosHalf, 1.0f, 1.0f, 1.0f),
		MakeTransform(0.0f, 0.125f, 0.0f, 0.1f, 0.2f, 0.0f, 0.9746794f, 1.1f, 1.1f, 1.1f),
	};

	const struct {
		HKX::Format format;
		const char* suffix;
	} cases[] = {
		{HKX::Format::Skyrim32, "le"},
		{HKX::Format::Skyrim64, "se"},
		{HKX::Format::Fallout64, "fo4"},
	};

	for (const auto& testCase : cases) {
		INFO(testCase.suffix);
		const std::string path = MakeTempPath(testCase.suffix);
		std::string error;

		HKX::SaveAnimationOptions options;
		options.originalSkeletonName = "UnitTestSkeleton";
		options.containerName = "Merged Animation Container";

		REQUIRE(HKX::File::SavePoseAnimation(path, testCase.format, tracks, options, &error));

		HKX::File file;
		const bool loaded = file.Load(path, &error);
		INFO(error);
		REQUIRE(loaded);
		REQUIRE(file.GetFormat() == testCase.format);
		REQUIRE(file.GetAnimations().size() == 1u);

		const HKX::Animation& animation = file.GetAnimations().front();
		REQUIRE(animation.binding.originalSkeletonName == "UnitTestSkeleton");
		REQUIRE(animation.numFrames == 1u);
		REQUIRE(animation.numTransformTracks == tracks.size());

		for (uint32_t trackIndex = 0; trackIndex < animation.numTransformTracks; ++trackIndex)
			RequireTransformNear(animation.At(0u, trackIndex), tracks[trackIndex]);

		std::remove(path.c_str());
	}
}