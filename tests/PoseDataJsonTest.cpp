#include "../src/components/PoseData.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <random>

using nifly::Matrix3;
using nifly::Vector3;

namespace {

// The rotation SAF builds from yaw, pitch and roll in a version 1 pose file (MatrixFromEulerYPR)
Matrix3 SafMatrix(float yawDeg, float pitchDeg, float rollDeg) {
	const float x = yawDeg * nifly::DEG2RAD;
	const float y = pitchDeg * nifly::DEG2RAD;
	const float z = rollDeg * nifly::DEG2RAD;

	Matrix3 m;
	m[0][0] = std::cos(y) * std::cos(z);
	m[0][1] = -std::cos(y) * std::sin(z);
	m[0][2] = std::sin(y);
	m[1][0] = std::sin(x) * std::sin(y) * std::cos(z) + std::sin(z) * std::cos(x);
	m[1][1] = std::cos(x) * std::cos(z) - std::sin(x) * std::sin(y) * std::sin(z);
	m[1][2] = -std::sin(x) * std::cos(y);
	m[2][0] = std::sin(x) * std::sin(z) - std::cos(x) * std::sin(y) * std::cos(z);
	m[2][1] = std::cos(x) * std::sin(y) * std::sin(z) + std::sin(x) * std::cos(z);
	m[2][2] = std::cos(x) * std::cos(y);
	return m;
}

void CheckSameRotation(const Vector3& a, const Vector3& b) {
	Matrix3 ma = nifly::RotVecToMat(a);
	Matrix3 mb = nifly::RotVecToMat(b);
	for (int r = 0; r < 3; r++)
		for (int c = 0; c < 3; c++)
			CHECK(ma[r][c] == Catch::Approx(mb[r][c]).margin(1e-4));
}

} // namespace

TEST_CASE("JSON poses keep their rotations, also at gimbal lock", "[PoseData]") {
	PoseData pose("RoundTrip");
	pose.absoluteLocal = true;

	struct Angles {
		float yaw, pitch, roll;
	};
	std::vector<Angles> angles = {
		{0.0f, 0.0f, 0.0f},
		{30.0f, 20.0f, -45.0f},
		{-120.0f, 60.0f, 170.0f},
		// Pitch at +-90 degrees leaves only yaw + roll defined
		{25.0f, 90.0f, 0.0f},
		{25.0f, -90.0f, 0.0f},
		{-70.0f, 90.0f, 40.0f},
		{-70.0f, -90.0f, 40.0f},
		{10.0f, 89.999f, -5.0f},
	};

	std::mt19937 random(1234);
	std::uniform_real_distribution<float> angle(-180.0f, 180.0f);
	std::uniform_real_distribution<float> pitch(-90.0f, 90.0f);
	for (int i = 0; i < 50; i++)
		angles.push_back({angle(random), pitch(random), angle(random)});

	for (size_t i = 0; i < angles.size(); i++) {
		PoseBoneData bone;
		bone.name = "Bone" + std::to_string(i);
		bone.rotation = nifly::RotMatToVec(SafMatrix(angles[i].yaw, angles[i].pitch, angles[i].roll));
		bone.translation = Vector3(static_cast<float>(i), 1.0f, -2.0f);
		bone.scale = 1.0f;
		pose.boneData.push_back(bone);
	}

	auto dir = std::filesystem::temp_directory_path() / ("BSOSTests-pose-" + std::to_string(std::random_device()()));
	std::filesystem::create_directories(dir);
	std::string file = (dir / "RoundTrip.json").string();

	REQUIRE(PoseDataCollection::SaveJsonPose(file, pose));
	PoseData loaded;
	REQUIRE(PoseDataCollection::LoadJsonPose(file, loaded));

	std::error_code error;
	std::filesystem::remove_all(dir, error);

	REQUIRE(loaded.boneData.size() == pose.boneData.size());
	for (auto& bone : pose.boneData) {
		auto it = std::find_if(loaded.boneData.begin(), loaded.boneData.end(), [&](const PoseBoneData& b) { return b.name == bone.name; });
		REQUIRE(it != loaded.boneData.end());
		INFO(bone.name);
		CheckSameRotation(bone.rotation, it->rotation);
		CHECK(it->translation.x == Catch::Approx(bone.translation.x));
	}
}
