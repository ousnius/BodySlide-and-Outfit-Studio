#include "../src/components/PoseFit.h"
#include "../src/components/PoseMath.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <deque>

using namespace nifly;

namespace {

constexpr float Pi = 3.14159265358979f;

// A torso with one arm: spine -> clavicle -> upper arm -> forearm -> hand, the arm out along -X
struct Figure {
	std::deque<AnimBone> bones;
	AnimBone* spine = nullptr;
	AnimBone* clavicle = nullptr;
	AnimBone* upper = nullptr;
	AnimBone* fore = nullptr;
	AnimBone* hand = nullptr;

	AnimBone* Add(const std::string& name, AnimBone* parent, const Vector3& translation) {
		AnimBone& bone = bones.emplace_back();
		bone.boneName = name;
		bone.parent = parent;
		bone.xformToParent.translation = translation;
		if (parent)
			parent->children.push_back(&bone);
		return &bone;
	}

	Figure() {
		spine = Add("NPC Spine [Spn0]", nullptr, Vector3(0.0f, 0.0f, 80.0f));
		clavicle = Add("NPC L Clavicle [LClv]", spine, Vector3(-3.0f, 0.0f, 20.0f));
		upper = Add("NPC L UpperArm [LUar]", clavicle, Vector3(-8.0f, 0.0f, 0.0f));
		fore = Add("NPC L Forearm [LLar]", upper, Vector3(-20.0f, 0.0f, 0.0f));
		hand = Add("NPC L Hand [LHnd]", fore, Vector3(-16.0f, 0.0f, 0.0f));
		Add("NPC L Finger20 [LF20]", hand, Vector3(-8.0f, 0.0f, 0.0f));
		spine->UpdateTransformToGlobal();
		spine->UpdatePoseTransform();
	}

	void Reset() {
		for (auto& bone : bones) {
			bone.poseRotVec = Vector3();
			bone.poseTranVec = Vector3();
			bone.poseScale = 1.0f;
		}
		spine->UpdatePoseTransform();
	}
};

// A tube along the arm from the shoulder to the finger tip, weighted to the bone of each part with a blend at the
// joints, plus a box for the torso weighted to the spine
PoseFit::Template MakeTemplate(Figure& f) {
	PoseFit::Template t;
	const Vector3 shoulder = f.upper->xformToGlobal.translation;
	const int rings = 40, segments = 12;
	const float length = 52.0f, radius = 2.5f;
	for (int r = 0; r <= rings; r++) {
		float along = length * r / rings;
		for (int s = 0; s < segments; s++) {
			float phi = 2.0f * Pi * s / segments;
			t.verts.push_back(shoulder + Vector3(-along, radius * std::cos(phi), radius * std::sin(phi)));

			std::vector<PoseFit::Template::Influence> w;
			auto blend = [&](AnimBone* a, AnimBone* b, float at, float x) {
				float k = std::clamp((x - at) / 4.0f + 0.5f, 0.0f, 1.0f);
				if (k < 1.0f)
					w.push_back({a, 1.0f - k});
				if (k > 0.0f)
					w.push_back({b, k});
			};
			if (along < 20.0f)
				blend(f.upper, f.fore, 20.0f, along);
			else if (along < 36.0f)
				blend(f.fore, f.hand, 36.0f, along);
			else
				w.push_back({f.hand, 1.0f});
			t.influences.push_back(w);
		}
	}
	for (int r = 0; r < rings; r++)
		for (int s = 0; s < segments; s++) {
			auto at = [&](int rr, int ss) { return static_cast<uint16_t>(rr * segments + (ss % segments)); };
			t.tris.emplace_back(at(r, s), at(r, s + 1), at(r + 1, s + 1));
			t.tris.emplace_back(at(r, s), at(r + 1, s + 1), at(r + 1, s));
		}
	return t;
}

Vector3 Direction(const AnimBone* bone) {
	Vector3 axis;
	PoseMath::BoneAxis(bone, axis);
	return axis;
}

float AngleDegrees(const Vector3& a, const Vector3& b) {
	return std::acos(std::clamp(a.dot(b) / (a.length() * b.length()), -1.0f, 1.0f)) * 180.0f / Pi;
}

} // namespace

TEST_CASE("Unposing undoes a pose exactly", "[posefit]") {
	Figure f;
	PoseFit::Template t = MakeTemplate(f);

	PoseMath::TurnBoneGlobal(f.upper, Vector3(0.0f, 30.0f * DEG2RAD, 0.0f));
	PoseMath::TurnBoneGlobal(f.fore, Vector3(0.0f, 0.0f, 25.0f * DEG2RAD));
	f.fore->poseScale = 1.2f;
	f.fore->UpdatePoseTransform();

	std::vector<Vector3> posed = PoseFit::PoseVerts(t);
	std::vector<Vector3> rest = PoseFit::Unpose(posed, t.influences);
	float worst = 0.0f;
	for (size_t i = 0; i < rest.size(); i++)
		worst = std::max(worst, rest[i].DistanceTo(t.verts[i]));
	REQUIRE(worst < 1e-3f);
}

TEST_CASE("Weights come from the closest point of the posed templates", "[posefit]") {
	Figure f;
	PoseFit::Template t = MakeTemplate(f);
	PoseMath::TurnBoneGlobal(f.upper, Vector3(0.0f, 40.0f * DEG2RAD, 0.0f));
	std::vector<Vector3> posed = PoseFit::PoseVerts(t);

	// Points on the posed tube take the weights of the tube where they are, and unpose to where it is unposed
	std::vector<Vector3> points;
	for (size_t i = 0; i < posed.size(); i += 7)
		points.push_back(posed[i]);
	auto weights = PoseFit::WeightsFromTemplates({t}, points, 20.0f);
	REQUIRE(weights.size() == points.size());

	std::vector<Vector3> rest = PoseFit::Unpose(points, weights);
	float worst = 0.0f;
	for (size_t k = 0, i = 0; i < posed.size(); i += 7, k++)
		worst = std::max(worst, rest[k].DistanceTo(t.verts[i]));
	REQUIRE(worst < 0.05f);
}

TEST_CASE("The skeleton is fitted to an arm in another pose and length", "[posefit]") {
	Figure f;
	PoseFit::Template t = MakeTemplate(f);

	// The figure: the same arm raised 35 degrees, bent at the elbow and with a longer forearm
	PoseMath::TurnBoneGlobal(f.upper, Vector3(0.0f, -35.0f * DEG2RAD, 0.0f));
	PoseMath::TurnBoneGlobal(f.fore, Vector3(0.0f, 0.0f, 20.0f * DEG2RAD));
	f.fore->poseScale = 1.15f;
	f.fore->UpdatePoseTransform();
	const Vector3 upperWanted = Direction(f.upper);
	const Vector3 foreWanted = Direction(f.fore);
	std::vector<Vector3> figure = PoseFit::PoseVerts(t);
	f.Reset();
	REQUIRE(AngleDegrees(Direction(f.upper), upperWanted) > 30.0f);

	PoseFit::Options options;
	options.bones = {f.upper, f.fore, f.hand};
	options.moveRoot = false;
	options.iterations = 40;
	options.trim = 0.1f;
	PoseFit::Result result = PoseFit::Fit({t}, figure, t.tris, options);

	INFO("upper " << f.upper->poseScale << " fore " << f.fore->poseScale << " hand " << f.hand->poseScale << " distance " << result.distanceAfter);
	REQUIRE(result.distanceAfter < result.distanceBefore);
	REQUIRE(result.distanceAfter < 0.2f);
	REQUIRE(AngleDegrees(Direction(f.upper), upperWanted) < 3.0f);
	REQUIRE(AngleDegrees(Direction(f.fore), foreWanted) < 4.0f);
	// A smooth tube can't tell where along it the wrist is, but the hand ends up the size of the figure's
	REQUIRE(f.fore->poseScale > 1.0f);
	REQUIRE(std::fabs(f.fore->poseScale * f.hand->poseScale - 1.15f) < 0.05f);
}
