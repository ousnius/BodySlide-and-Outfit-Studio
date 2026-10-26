#include "../src/components/PoseMath.h"
#include "../src/utils/ConfigurationManager.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <deque>

using namespace nifly;

extern ConfigurationManager Config;

namespace {

// A small arm: shoulder -> upper arm -> forearm -> hand -> finger, with two twist bones along the forearm. The
// elbow bends slightly in the unposed skeleton, the hand towards -Y.
struct Arm {
	std::deque<AnimBone> bones;
	AnimBone* shoulder = nullptr;
	AnimBone* upper = nullptr;
	AnimBone* fore = nullptr;
	AnimBone* twist1 = nullptr;
	AnimBone* twist2 = nullptr;
	AnimBone* hand = nullptr;
	AnimBone* finger = nullptr;

	AnimBone* Add(const std::string& name, AnimBone* parent, const Vector3& translation) {
		AnimBone& bone = bones.emplace_back();
		bone.boneName = name;
		bone.parent = parent;
		bone.xformToParent.translation = translation;
		if (parent)
			parent->children.push_back(&bone);
		return &bone;
	}

	Arm() {
		shoulder = Add("NPC L Clavicle [LClv]", nullptr, Vector3(0.0f, 0.0f, 100.0f));
		upper = Add("NPC L UpperArm [LUar]", shoulder, Vector3(-5.0f, 0.0f, 0.0f));
		fore = Add("NPC L Forearm [LLar]", upper, Vector3(-20.0f, 0.0f, 0.0f));
		twist2 = Add("NPC L ForearmTwist2 [LLt2]", fore, Vector3(-6.0f, -0.3f, 0.0f));
		twist1 = Add("NPC L ForearmTwist1 [LLt1]", fore, Vector3(-12.0f, -0.6f, 0.0f));
		hand = Add("NPC L Hand [LHnd]", fore, Vector3(-18.0f, -1.0f, 0.0f));
		finger = Add("NPC L Finger20 [LF20]", hand, Vector3(-8.0f, 0.0f, 0.0f));
		shoulder->UpdateTransformToGlobal();
		shoulder->UpdatePoseTransform();
	}

	PoseMath::TwistContext Context() {
		PoseMath::TwistContext context;
		for (auto& bone : bones)
			context.bones.push_back(&bone);
		return context;
	}
};

Vector3 Position(const AnimBone* bone) {
	return bone->xformPoseToGlobal.translation;
}

bool Near(const Vector3& a, const Vector3& b, float tolerance = 1e-3f) {
	return (a - b).length() < tolerance;
}

} // namespace

TEST_CASE("Global turns are right-handed about the skeleton axes", "[posemath]") {
	Arm arm;
	// Turning the upper arm 90 degrees about +Z takes the elbow from -X to -Y
	PoseMath::TurnBoneGlobal(arm.upper, Vector3(0.0f, 0.0f, 90.0f * DEG2RAD));
	REQUIRE(Near(Position(arm.fore), Vector3(-5.0f, -20.0f, 100.0f)));

	// Rotating a vector agrees with the bones
	Vector3 turned = PoseMath::RotateVector(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 90.0f * DEG2RAD));
	REQUIRE(Near(turned, Vector3(0.0f, 1.0f, 0.0f)));
}

TEST_CASE("Aiming swings a bone without twisting it", "[posemath]") {
	Arm arm;
	PoseMath::TurnBoneGlobal(arm.upper, PoseMath::AimRotation(arm.upper, Vector3(0.0f, 1.0f, 1.0f)));

	Vector3 axis;
	REQUIRE(PoseMath::BoneAxis(arm.upper, axis));
	REQUIRE(Near(axis, Vector3(0.0f, std::sqrt(0.5f), std::sqrt(0.5f))));
	REQUIRE(PoseMath::LocalTwist(arm.upper) == Catch::Approx(0.0f).margin(1e-4));
}

TEST_CASE("Twist is measured about the bone and can be set", "[posemath]") {
	Arm arm;
	Vector3 axis;
	REQUIRE(PoseMath::BoneAxis(arm.upper, axis));

	PoseMath::TurnBoneGlobal(arm.upper, axis * (30.0f * DEG2RAD));
	REQUIRE(PoseMath::LocalTwist(arm.upper) == Catch::Approx(30.0f * DEG2RAD).margin(1e-4));
	// The forearm turns with it, but its own twist stays 0
	REQUIRE(PoseMath::LocalTwist(arm.fore) == Catch::Approx(0.0f).margin(1e-4));

	PoseMath::SetLocalTwist(arm.upper, -45.0f * DEG2RAD);
	REQUIRE(PoseMath::LocalTwist(arm.upper) == Catch::Approx(-45.0f * DEG2RAD).margin(1e-4));

	// Swinging after a twist keeps it
	PoseMath::TurnBoneGlobal(arm.upper, PoseMath::AimRotation(arm.upper, Vector3(0.0f, 0.0f, -1.0f)));
	REQUIRE(PoseMath::LocalTwist(arm.upper) == Catch::Approx(-45.0f * DEG2RAD).margin(1e-3));
}

TEST_CASE("Twist of the hand spreads over the forearm's twist bones", "[posemath]") {
	Arm arm;
	auto shares = PoseMath::SegmentTwistBones(arm.hand, arm.Context());
	REQUIRE(shares.size() == 2);

	auto set = PoseMath::SetTwistSpread(arm.hand, 60.0f * DEG2RAD, arm.Context());
	REQUIRE(set.size() == 2);
	REQUIRE(PoseMath::LocalTwist(arm.hand) == Catch::Approx(60.0f * DEG2RAD).margin(1e-3));
	// About a third and two thirds along the forearm
	REQUIRE(PoseMath::LocalTwist(arm.twist2) == Catch::Approx(20.0f * DEG2RAD).margin(0.02));
	REQUIRE(PoseMath::LocalTwist(arm.twist1) == Catch::Approx(40.0f * DEG2RAD).margin(0.02));
	REQUIRE(PoseMath::TwistSpreadError(arm.hand, arm.Context()) < 1e-3f);

	// Twisting the hand alone leaves the twist bones behind
	PoseMath::SetLocalTwist(arm.hand, 90.0f * DEG2RAD);
	REQUIRE(PoseMath::TwistSpreadError(arm.hand, arm.Context()) > 5.0f * DEG2RAD);
}

TEST_CASE("Two-bone IK reaches a target and bends towards the pole", "[posemath]") {
	Arm arm;
	const Vector3 shoulder = Position(arm.upper);

	SECTION("In reach") {
		const Vector3 target = shoulder + Vector3(-10.0f, 15.0f, 20.0f);
		const Vector3 pole(0.0f, 0.0f, -1.0f);
		auto result = PoseMath::ReachTwoBone(arm.hand, target, &pole);
		REQUIRE(result.reachable);
		REQUIRE(result.hingeAligned);
		REQUIRE(result.missed < 1e-3f);

		// The elbow is on the pole's side of the line from shoulder to hand
		Vector3 line = target - shoulder;
		line.Normalize();
		Vector3 elbow = Position(arm.fore) - shoulder;
		REQUIRE((elbow - line * elbow.dot(line)).dot(pole) > 0.0f);

		// Bent the way the unposed elbow bends, and without twisting the forearm
		REQUIRE_FALSE(PoseMath::BendsBackwards(arm.fore));
		REQUIRE(PoseMath::LocalTwist(arm.fore) == Catch::Approx(0.0f).margin(1e-3));
	}

	SECTION("Out of reach") {
		auto result = PoseMath::ReachTwoBone(arm.hand, shoulder + Vector3(-100.0f, 0.0f, 0.0f), nullptr);
		REQUIRE_FALSE(result.reachable);
		REQUIRE(result.missed > 50.0f);
	}

	SECTION("Solving again doesn't keep earlier roll") {
		PoseMath::TurnBoneGlobal(arm.upper, Vector3(-20.0f * DEG2RAD, 0.0f, 0.0f));
		PoseMath::TurnBoneGlobal(arm.fore, Vector3(0.0f, 0.0f, 40.0f * DEG2RAD));
		const Vector3 target = shoulder + Vector3(-15.0f, 0.0f, -25.0f);
		auto result = PoseMath::ReachTwoBone(arm.hand, target, nullptr);
		REQUIRE(result.missed < 1e-3f);
		REQUIRE(PoseMath::LocalTwist(arm.fore) == Catch::Approx(0.0f).margin(1e-3));
		REQUIRE_FALSE(PoseMath::BendsBackwards(arm.fore));
	}
}

TEST_CASE("Folding a hinge the wrong way is found", "[posemath]") {
	Arm arm;
	// The unposed hand sits towards -Y. Turning the forearm about -Z bends it towards +Y, backwards.
	PoseMath::TurnBoneGlobal(arm.fore, Vector3(0.0f, 0.0f, -40.0f * DEG2RAD));
	REQUIRE(PoseMath::BendsBackwards(arm.fore));
	REQUIRE(PoseMath::BendAngle(arm.fore) > 30.0f * DEG2RAD);

	// The other way is how it should bend
	PoseMath::TurnBoneGlobal(arm.fore, Vector3(0.0f, 0.0f, 80.0f * DEG2RAD));
	REQUIRE_FALSE(PoseMath::BendsBackwards(arm.fore));
}

TEST_CASE("Shifting moves a bone by a global offset", "[posemath]") {
	Arm arm;
	PoseMath::TurnBoneGlobal(arm.upper, Vector3(0.0f, 0.0f, 90.0f * DEG2RAD));
	const Vector3 before = Position(arm.hand);
	PoseMath::ShiftBoneGlobal(arm.fore, Vector3(1.0f, 2.0f, 3.0f));
	REQUIRE(Near(Position(arm.hand), before + Vector3(1.0f, 2.0f, 3.0f)));
}

TEST_CASE("A bone's orientation is put back after its parents turned", "[posemath]") {
	Arm arm;
	PoseMath::TurnBoneGlobal(arm.hand, Vector3(0.3f, -0.2f, 0.5f));
	const auto orientation = PoseMath::CaptureOrientation(arm.hand);
	const Vector3 fingerOffset = Position(arm.finger) - Position(arm.hand);
	const Matrix3 rotation = arm.hand->xformPoseToGlobal.rotation;

	PoseMath::TurnBoneGlobal(arm.upper, Vector3(0.0f, 0.7f, 0.2f));
	PoseMath::TurnBoneGlobal(arm.fore, Vector3(0.0f, 0.0f, 0.9f));
	PoseMath::RestoreOrientation(arm.hand, orientation);

	REQUIRE(Near(Position(arm.finger) - Position(arm.hand), fingerOffset));
	REQUIRE(arm.hand->xformPoseToGlobal.rotation.IsNearlyEqualTo(rotation));
}

TEST_CASE("Main bones and short names", "[posemath]") {
	REQUIRE(PoseMath::IsMainBone("NPC L UpperArm [LUar]"));
	REQUIRE(PoseMath::IsMainBone("NPC Spine2 [Spn2]"));
	REQUIRE_FALSE(PoseMath::IsMainBone("NPC L ForearmTwist1 [LLt1]"));
	REQUIRE_FALSE(PoseMath::IsMainBone("NPC L Finger20 [LF20]"));
	REQUIRE_FALSE(PoseMath::IsMainBone("NPC L Breast"));
	REQUIRE_FALSE(PoseMath::IsMainBone("NPC L FrontThigh 1"));
	REQUIRE_FALSE(PoseMath::IsMainBone("NPC L RearCalf [LrClf]"));
	REQUIRE(PoseMath::ShortBoneName("NPC L UpperArm [LUar]") == "LUar");
	REQUIRE(PoseMath::ShortBoneName("NPC R Foot [Rft ]") == "Rft");
	REQUIRE(PoseMath::ShortBoneName("Pelvis") == "Pelvis");
	REQUIRE(PoseMath::GetBoneRole("NPC L Forearm [LLar]").role == "forearm (hinge: elbow)");

	// Only NPC bones count when the skeleton has them
	AnimBone npc, other;
	npc.boneName = "NPC Head [Head]";
	other.boneName = "Ragdoll Head";
	auto main = PoseMath::MainBones({&npc, &other});
	REQUIRE(main.size() == 1);
	REQUIRE(main[0] == &npc);
	REQUIRE(PoseMath::MainBones({&other}).size() == 1);
}

namespace {

struct GameSkeleton {
	const char* file;
	const char* root;
	const char* hand;
	const char* forearm;
	const char* upperArm;
	const char* foot;
	const char* calf;
	const char* thigh;
	const char* pelvis;
	const char* chest;
	const char* head;
};

// The skeletons Outfit Studio ships, with the joints of their left limbs
const GameSkeleton GameSkeletons[] = {
	{"skeleton_female_sse.nif", "NPC Root [Root]", "NPC L Hand [LHnd]", "NPC L Forearm [LLar]", "NPC L UpperArm [LUar]", "NPC L Foot [Lft ]", "NPC L Calf [LClf]",
	 "NPC L Thigh [LThg]", "NPC Pelvis [Pelv]", "NPC Spine2 [Spn2]", "NPC Head [Head]"},
	{"skeleton_fo4.nif", "Root", "LArm_Hand", "LArm_ForeArm1", "LArm_UpperArm", "LLeg_Foot", "LLeg_Calf", "LLeg_Thigh", "Pelvis", "Chest", "HEAD"},
	{"skeleton_female_sf.nif", "Root", "L_Wrist", "L_Forearm", "L_Biceps", "L_Foot", "L_Calf", "L_Thigh", "C_Hips", "C_Chest", "C_Head"},
	{"skeleton_fo3nv.nif", "Bip01", "Bip01 L Hand", "Bip01 L Forearm", "Bip01 L UpperArm", "Bip01 L Foot", "Bip01 L Calf", "Bip01 L Thigh", "Bip01 Pelvis",
	 "Bip01 Spine2", "Bip01 Head"},
	{"skeleton_ob.nif", "Bip01", "Bip01 L Hand", "Bip01 L Forearm", "Bip01 L UpperArm", "Bip01 L Foot", "Bip01 L Calf", "Bip01 L Thigh", "Bip01 Pelvis",
	 "Bip01 Spine2", "Bip01 Head"},
};

AnimBone* LoadGameSkeleton(const GameSkeleton& game, PoseMath::TwistContext& context) {
	Config.SetValue("Anim/SkeletonRootName", game.root);
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	skeleton.Clear();
	REQUIRE(skeleton.LoadFromNif(std::string(BSOS_TEST_DATA_DIR) + "/../../res/" + game.file) == 0);

	std::vector<std::string> names;
	skeleton.GetBoneNames(names);
	for (auto& name : names)
		context.bones.push_back(skeleton.GetBonePtr(name));
	return skeleton.GetRootBonePtr();
}

AnimBone* Bone(const char* name) {
	AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
	INFO(name);
	REQUIRE(bone);
	return bone;
}

} // namespace

TEST_CASE("Posing works on the skeletons of every game", "[posemath]") {
	// Other tests expect the skeleton root setting and the skeleton as they were
	const std::string previousRoot = Config["Anim/SkeletonRootName"];

	for (auto& game : GameSkeletons) {
		INFO(game.file);
		PoseMath::TwistContext context;
		LoadGameSkeleton(game, context);

		// Main bones: the joints, and no helper, scale, twist or finger bones
		auto main = PoseMath::MainBones(context.bones);
		for (const char* name : {game.hand, game.forearm, game.upperArm, game.foot, game.calf, game.thigh, game.pelvis, game.chest, game.head})
			CHECK(std::find(main.begin(), main.end(), Bone(name)) != main.end());
		for (AnimBone* bone : main) {
			INFO(bone->boneName);
			std::string lower = bone->boneName;
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			for (const char* part : {"skin", "scale", "twist", "finger", "elbow", "knee", "weapon"})
				CHECK(lower.find(part) == std::string::npos);
		}

		// Limbs from the end bone, through Fallout 4's forearm chain and Oblivion's scale nodes
		AnimBone* upper = nullptr;
		AnimBone* lower = nullptr;
		REQUIRE(PoseMath::LimbChain(Bone(game.hand), upper, lower));
		CHECK(upper == Bone(game.upperArm));
		CHECK(lower == Bone(game.forearm));
		REQUIRE(PoseMath::LimbChain(Bone(game.foot), upper, lower));
		CHECK(upper == Bone(game.thigh));
		CHECK(lower == Bone(game.calf));

		// The pelvis points down between the hips, the chest up to the neck, the left upper arm to her left
		Vector3 axis;
		REQUIRE(PoseMath::BoneAxis(Bone(game.pelvis), axis));
		CHECK(axis.z < -0.8f);
		REQUIRE(PoseMath::BoneAxis(Bone(game.chest), axis));
		CHECK(axis.z > 0.8f);
		REQUIRE(PoseMath::BoneAxis(Bone(game.upperArm), axis));
		CHECK(axis.x < -0.3f);

		// The hand's twist spreads over the forearm
		CHECK_FALSE(PoseMath::SegmentTwistBones(Bone(game.hand), context).empty());

		// The hand reaches up in front of the face, with the elbow bending the right way and no twist left on it
		AnimBone* hand = Bone(game.hand);
		AnimBone* shoulder = Bone(game.upperArm);
		const float length = (Bone(game.forearm)->xformToGlobal.translation - shoulder->xformToGlobal.translation).length()
							 + (hand->xformToGlobal.translation - Bone(game.forearm)->xformToGlobal.translation).length();
		Vector3 direction(0.3f, 0.6f, 0.75f);
		direction.Normalize();
		const Vector3 target = shoulder->xformPoseToGlobal.translation + direction * (0.8f * length);
		const Vector3 pole(-1.0f, 0.0f, 0.0f);
		auto result = PoseMath::ReachTwoBone(hand, target, &pole);
		CHECK(result.solved);
		CHECK(result.reachable);
		CHECK(result.missed < 1e-3f * length);
		CHECK_FALSE(PoseMath::BendsBackwards(Bone(game.forearm)));
		CHECK(PoseMath::LocalTwist(Bone(game.forearm)) == Catch::Approx(0.0f).margin(1e-3));

		// Spread over the twist bones, the twist of the hand reads back whole
		PoseMath::SetTwistSpread(hand, 50.0f * DEG2RAD, context);
		CHECK(PoseMath::SegmentTwist(hand, context) == Catch::Approx(50.0f * DEG2RAD).margin(1e-2));
		CHECK(PoseMath::TwistSpreadError(hand, context) < 1e-2f);
	}
	Config.SetValue("Anim/SkeletonRootName", previousRoot);
	AnimSkeleton::getInstance().Clear();
}
