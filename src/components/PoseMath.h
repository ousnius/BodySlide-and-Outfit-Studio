/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Anim.h"

#include <map>
#include <string>
#include <vector>

// Posing the skeleton by directions in its global space (+X the character's right, +Y her front, +Z up): turning,
// aiming and twisting bones, two-bone IK, and reading back how a pose bends and twists the joints. Works on the
// AnimBone hierarchy only, with vector math that doesn't depend on how the matrices of a pose are composed.
//
// Skeletons of the games name and nest their bones differently: Skyrim's "NPC L Forearm [LLar]", Fallout 4's chain of
// LArm_ForeArm1 to 3, Starfield's L_Biceps and L_Wrist, and Oblivion's Bip01 bones with Scale nodes between the
// joints. Joints are found by the role of the main bones, looking through helper and scale nodes, not by parent and
// child alone.
namespace PoseMath {

// Rotates v about the axis of a rotation vector (axis times angle in radians), right-handed
nifly::Vector3 RotateVector(const nifly::Vector3& v, const nifly::Vector3& rotation);

// The rotation vector that turns the direction 'from' onto 'to' the shortest way
nifly::Vector3 SwingRotation(nifly::Vector3 from, nifly::Vector3 to);

// The angle in radians from a to b about the axis, right-handed, after projecting both across the axis
float SignedAngle(const nifly::Vector3& a, const nifly::Vector3& b, const nifly::Vector3& axis);

// The next joint a bone reaches, e.g. the elbow for the upper arm: the main bone below it, looking through helper and
// scale nodes, preferring one of the same role (spine to spine) and the middle for a bone in the middle. Without a main
// bone below, its child farthest away. Null when it reaches nothing or the middle of a left and right pair.
AnimBone* ReachChild(const AnimBone* bone);

// Where the next joint of a bone is with the pose, in global space: ReachChild, or the middle of a left and right
// pair (the pelvis reaches down between the hips). False when the bone reaches nothing.
bool ReachPosition(const AnimBone* bone, nifly::Vector3& outPosition);

// Which way a bone points with the pose: towards ReachPosition. Bones that reach nothing (twist bones) take the
// direction of their parent.
bool BoneAxis(const AnimBone* bone, nifly::Vector3& outAxis);

// Turns a bone about the skeleton's global axes by a rotation vector in radians, right-handed. Its children turn
// with it.
void TurnBoneGlobal(AnimBone* bone, const nifly::Vector3& rotation);

// Moves a bone by an offset in the skeleton's global space, with its children
void ShiftBoneGlobal(AnimBone* bone, const nifly::Vector3& offset);

// The rotation vector that points a bone in a global direction with the shortest turn from where it points now
nifly::Vector3 AimRotation(const AnimBone* bone, nifly::Vector3 direction);

// How far a bone is twisted about its own axis by its own pose rotation, in radians, right-handed about the
// direction it points: compared to the bone turned the shortest way from its unposed direction. Twist of parents
// doesn't count.
float LocalTwist(const AnimBone* bone);

// Turns a bone about its own axis so its LocalTwist becomes the given angle in radians
void SetLocalTwist(AnimBone* bone, float twist);

// Which way a bone points and how it's turned about that, in global space, to put it back after its parents moved
struct BoneOrientation {
	nifly::Vector3 axis;
	nifly::Vector3 across;
	nifly::Vector3 localAcross;
};
BoneOrientation CaptureOrientation(const AnimBone* bone);
void RestoreOrientation(AnimBone* bone, const BoneOrientation& orientation);

// The closest main ancestor, or else the parent: the joint a bone bends against
AnimBone* JointParent(const AnimBone* bone);

// The angle in radians between a bone and its joint parent, 0 when straight
float BendAngle(const AnimBone* bone);

// Whether a hinge joint (elbow, knee) bends the opposite way to the unposed skeleton's slight bend. False when the
// unposed skeleton is straight there or the joint barely bends.
bool BendsBackwards(const AnimBone* bone);

// The bones of a skeleton, and where the skin of each twist bone is: the middle of the vertices weighted to it, in the
// bone's own coordinates. A twist bone sits by its skin along a limb rather than by its origin, which some skeletons
// put at the joint (the forearm twist bones of Fallout 3 and Oblivion).
struct TwistContext {
	std::vector<AnimBone*> bones;
	std::map<const AnimBone*, nifly::Vector3> skinCenters;
};

// Twist bones that spread the twist of a bone over the skin of a limb segment, with how far along the segment each
// one is (0 at the joint the segment starts at, 1 at the end)
struct TwistShare {
	AnimBone* bone = nullptr;
	float fraction = 0.0f;
};

// The twist bones along the segment of a bone (from its joint to the next), or else along the limb segment that
// ends at its joint (the forearm for the hand). Bones of a limb in a chain, like Fallout 4's ForeArm2 and ForeArm3
// between the elbow and the hand, count as twist bones of the segment.
std::vector<TwistShare> SegmentTwistBones(const AnimBone* bone, const TwistContext& context);

// Sets the twist of a bone, relative to where its segment starts, and spreads it over the twist bones of its
// segment by their fraction, so the skin turns gradually. Returns the twist bones set, with their LocalTwist.
std::vector<std::pair<AnimBone*, float>> SetTwistSpread(AnimBone* bone, float twist, const TwistContext& context);

// The twist of a bone relative to where its segment starts: its LocalTwist plus that of the chain bones of the
// segment above it. The value SetTwistSpread takes.
float SegmentTwist(const AnimBone* bone, const TwistContext& context);

// How far the twist bones of a bone's segment are from the share SetTwistSpread gives them, in radians. 0 without
// twist bones.
float TwistSpreadError(const AnimBone* bone, const TwistContext& context);

// The upper and lower bone of the limb an end bone (hand, foot) belongs to: the lower bone is its closest main
// ancestor, or the top of a run of main ancestors with the same role (Fallout 4's three forearm bones); the upper
// bone is the main bone above that. False when there are none.
bool LimbChain(const AnimBone* end, AnimBone*& outUpper, AnimBone*& outLower);

struct ReachResult {
	// The end bone belongs to a limb, so it could be solved
	bool solved = false;
	// The target was in reach. When it wasn't, the limb points straight at it or folds fully.
	bool reachable = true;
	// How far the end joint ended up from the target
	float missed = 0.0f;
	// The upper bone was rolled so the joint bends in the plane of the pole, the way the unposed skeleton bends
	bool hingeAligned = false;
	AnimBone* upper = nullptr;
	AnimBone* lower = nullptr;
	nifly::Vector3 midPosition;
	nifly::Vector3 endPosition;
};

// Two-bone IK: puts the joint of 'end' (e.g. the hand) at the target by turning the upper and lower bone of its limb
// (LimbChain). The pole is the global direction the middle joint (elbow) points to. Without one, the middle joint
// points the way it bends in the unposed skeleton, or the way it pointed before. Both bones are solved from their
// unposed rotation, so no twist from earlier turns remains.
ReachResult ReachTwoBone(AnimBone* end, const nifly::Vector3& target, const nifly::Vector3* pole);

// Main bones of a body: pelvis, spine, neck, head, clavicles, arms, hands, legs and feet, by the names the games use.
// Twist, finger, helper, scale, physics, weapon and body part bones aren't.
bool IsMainBone(const std::string& name);

// The main bones of a skeleton. When it has "NPC " bones (Skyrim) or "Bip01 " bones (Oblivion, Fallout 3), only
// those, and only bones of the reference skeleton when it has any, so bones of other mods or custom bones that happen
// to share a name part don't count.
std::vector<AnimBone*> MainBones(const std::vector<AnimBone*>& skeleton);

// The closest ancestor of a bone that is a main bone, or null
AnimBone* MainParent(const AnimBone* bone);

// A short name for labels: the tag in brackets (LUar for "NPC L UpperArm [LUar]"), or else the name without a
// "Bip01 " prefix
std::string ShortBoneName(const std::string& name);

// What a main bone is, e.g. "upper arm" or "forearm (hinge)", and its natural range of motion as a hint. Empty
// for other bones.
struct BoneRole {
	std::string role;
	std::string limits;
};
BoneRole GetBoneRole(const std::string& name);

} // namespace PoseMath
