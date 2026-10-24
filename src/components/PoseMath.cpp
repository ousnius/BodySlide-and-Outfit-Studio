/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PoseMath.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>

using namespace nifly;

namespace PoseMath {

namespace {

// Smallest sine of a bend that counts as bent, about 2 degrees
constexpr float BentSine = 0.035f;
// Bones closer than this, relative to the length of what they're compared with, are at the same spot
constexpr float SameSpot = 1e-4f;

Vector3 Normalized(Vector3 v) {
	v.Normalize();
	return v;
}

std::string Lower(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

bool Contains(const std::string& text, const char* part) {
	return text.find(part) != std::string::npos;
}

// Twist bones of their own, not the skin helpers that hang off them (Fallout 4's UpperTwist1_skin)
bool IsTwistBone(const AnimBone* bone) {
	const std::string lower = Lower(bone->boneName);
	return Contains(lower, "twist") && !Contains(lower, "skin");
}

// Nodes that hold things, drive animation or carry skin rather than being joints: they don't show which way a bone
// points (Fallout 4's HEAD has only Face_skin and Head_skin below it)
bool IsAttachment(const AnimBone* bone) {
	const std::string lower = Lower(bone->boneName);
	for (const char* part : {"weapon", "anim", "magic", "shield", "quiver", "camera", "handik", "pipboy", "skin"})
		if (Contains(lower, part))
			return true;
	return false;
}

bool IsDescendant(const AnimBone* bone, const AnimBone* ancestor) {
	for (const AnimBone* p = bone->parent; p; p = p->parent)
		if (p == ancestor)
			return true;
	return false;
}

int Depth(const AnimBone* bone) {
	int depth = 0;
	for (const AnimBone* p = bone->parent; p; p = p->parent)
		depth++;
	return depth;
}

const Vector3& Posed(const AnimBone* bone) {
	return bone->xformPoseToGlobal.translation;
}

// The bone's frame with its own pose rotation left out: its parent's pose, then its transform to the parent
MatTransform UnrotatedFrame(const AnimBone* bone) {
	if (bone->parent)
		return bone->parent->xformPoseToGlobal.ComposeTransforms(bone->xformToParent);
	return bone->xformToParent;
}

// A unit axis of the bone's coordinates that is far from parallel to the direction
Vector3 CrossAxis(const Vector3& direction) {
	const Vector3 axes[] = {Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)};
	Vector3 best = axes[0];
	float bestDot = 2.0f;
	for (auto& axis : axes) {
		float d = std::abs(axis.dot(direction));
		if (d < bestDot) {
			bestDot = d;
			best = axis;
		}
	}
	return best;
}

// The bones a bone reaches: one, or a left and right pair whose middle it reaches
std::vector<const AnimBone*> ReachBones(const AnimBone* bone) {
	const std::string role = GetBoneRole(bone->boneName).role;
	const Vector3 origin = bone->xformToGlobal.translation;

	// The pelvis points down between the knees: the hip joints are often level with it, and some skeletons hang the
	// spine below it
	if (role == "pelvis") {
		std::vector<const AnimBone*> knees;
		std::vector<const AnimBone*> level(bone->children.begin(), bone->children.end());
		for (int depth = 0; depth < 4 && !level.empty(); depth++) {
			std::vector<const AnimBone*> next;
			for (const AnimBone* b : level) {
				if (GetBoneRole(b->boneName).role.rfind("thigh", 0) == 0) {
					if (const AnimBone* knee = ReachChild(b))
						knees.push_back(knee);
				}
				else
					next.insert(next.end(), b->children.begin(), b->children.end());
			}
			level = std::move(next);
		}
		if (knees.size() == 2)
			return knees;
	}

	// The main bones below, level by level through helper and scale nodes, up to a few levels down
	std::vector<const AnimBone*> level(bone->children.begin(), bone->children.end());
	for (int depth = 0; depth < 4 && !level.empty(); depth++) {
		std::vector<const AnimBone*> candidates;
		float farthest = 0.0f;
		for (const AnimBone* child : level) {
			const float distance = (child->xformToGlobal.translation - origin).length();
			if (IsMainBone(child->boneName) && distance > SameSpot) {
				candidates.push_back(child);
				farthest = std::max(farthest, distance);
			}
		}

		if (!candidates.empty()) {
			// The same role continues the chain, e.g. one spine bone to the next
			if (!role.empty()) {
				for (const AnimBone* candidate : candidates)
					if (GetBoneRole(candidate->boneName).role == role)
						return {candidate};
			}

			// A bone in the middle reaches the middle: the neck and not a clavicle, or between a pair of hips
			const bool central = std::abs(origin.x) < 0.1f * farthest;
			if (central) {
				const AnimBone* middle = nullptr;
				const AnimBone* left = nullptr;
				const AnimBone* right = nullptr;
				for (const AnimBone* candidate : candidates) {
					const Vector3& p = candidate->xformToGlobal.translation;
					const float distance = (p - origin).length();
					if (std::abs(p.x - origin.x) < 0.25f * distance) {
						if (!middle || distance > (middle->xformToGlobal.translation - origin).length())
							middle = candidate;
					}
					else if (p.x < origin.x)
						left = candidate;
					else
						right = candidate;
				}
				if (middle)
					return {middle};
				if (left && right)
					return {left, right};
			}

			const AnimBone* best = candidates.front();
			for (const AnimBone* candidate : candidates)
				if ((candidate->xformToGlobal.translation - origin).length() > (best->xformToGlobal.translation - origin).length())
					best = candidate;
			return {best};
		}

		std::vector<const AnimBone*> next;
		for (const AnimBone* b : level)
			next.insert(next.end(), b->children.begin(), b->children.end());
		level = std::move(next);
	}

	// No main bone below: the child farthest away, looking through children at the bone's own spot
	const AnimBone* best = nullptr;
	float longest = SameSpot;
	std::vector<const AnimBone*> search(bone->children.begin(), bone->children.end());
	for (int depth = 0; depth < 3 && !best && !search.empty(); depth++) {
		std::vector<const AnimBone*> next;
		for (const AnimBone* child : search) {
			if (IsAttachment(child))
				continue;
			const float distance = (child->xformToGlobal.translation - origin).length();
			if (distance > longest) {
				longest = distance;
				best = child;
			}
			else if (distance <= SameSpot)
				next.insert(next.end(), child->children.begin(), child->children.end());
		}
		search = std::move(next);
	}
	if (best)
		return {best};
	return {};
}

Vector3 Middle(const std::vector<const AnimBone*>& bones, bool posed) {
	Vector3 sum;
	for (const AnimBone* b : bones)
		sum += posed ? b->xformPoseToGlobal.translation : b->xformToGlobal.translation;
	return sum / static_cast<float>(bones.size());
}

// Where the bone reaches in its own coordinates, from the unposed skeleton: fixed to the bone, however the bones
// below it are posed
bool LocalReachPoint(const AnimBone* bone, Vector3& outPoint) {
	auto reach = ReachBones(bone);
	if (reach.empty())
		return false;
	outPoint = bone->xformToGlobal.InverseTransform().ApplyTransform(Middle(reach, false));
	return outPoint.length() > SameSpot;
}

// The direction the bone reaches, in its own coordinates
bool LocalReach(const AnimBone* bone, Vector3& outDirection) {
	Vector3 point;
	if (LocalReachPoint(bone, point)) {
		outDirection = Normalized(point);
		return true;
	}

	// A bone that reaches nothing points like its parent: the parent's global direction in the bone's coordinates
	Vector3 axis;
	if (!bone->parent || !BoneAxis(bone->parent, axis))
		return false;
	outDirection = Normalized(bone->xformPoseToGlobal.InverseTransform().ApplyTransformToDir(axis));
	return outDirection.length() > 0.5f;
}

// The top of the run of main ancestors with the same role as the closest one: where the limb segment that ends at
// the bone's joint starts (Fallout 4's ForeArm1 for the hand)
AnimBone* SegmentStart(const AnimBone* bone) {
	AnimBone* start = MainParent(bone);
	if (!start)
		return bone->parent;

	const std::string role = GetBoneRole(start->boneName).role;
	while (AnimBone* above = MainParent(start)) {
		if (GetBoneRole(above->boneName).role != role)
			break;
		start = above;
	}
	return start;
}

// Where a twist bone's skin is with the pose: the middle of its weighted vertices, or else its origin
Vector3 SkinPosition(const AnimBone* bone, const TwistContext& context) {
	auto center = context.skinCenters.find(bone);
	if (center != context.skinCenters.end())
		return bone->xformPoseToGlobal.ApplyTransform(center->second);
	return Posed(bone);
}

// The twist bones along the segment from p0 to p1: the chain bones given, and bones named twist that hang off one of
// the owners, so a twist bone of another limb nearby doesn't count
std::vector<TwistShare> TwistBonesAlong(const Vector3& p0,
										const Vector3& p1,
										const AnimBone* bone,
										const std::vector<const AnimBone*>& chain,
										const std::vector<const AnimBone*>& owners,
										const TwistContext& context) {
	std::vector<TwistShare> shares;
	Vector3 segment = p1 - p0;
	const float length = segment.length();
	if (length < SameSpot)
		return shares;
	segment /= length;

	auto add = [&](AnimBone* candidate, bool inChain) {
		Vector3 offset = SkinPosition(candidate, context) - p0;
		const float along = offset.dot(segment);
		const float across = (offset - segment * along).length();
		const float fraction = along / length;
		// One at the end joint belongs to the next segment, like the upper arm's first twist bone for the clavicle
		if (fraction < -0.05f || fraction > 0.95f || (!inChain && across > 0.45f * length))
			return;
		shares.push_back({candidate, std::clamp(fraction, 0.0f, 1.0f)});
	};

	for (const AnimBone* member : chain)
		add(const_cast<AnimBone*>(member), true);
	for (AnimBone* candidate : context.bones) {
		if (!candidate || candidate == bone || !IsTwistBone(candidate) || std::find(chain.begin(), chain.end(), candidate) != chain.end())
			continue;
		const AnimBone* owner = MainParent(candidate);
		if (std::find(owners.begin(), owners.end(), owner) != owners.end())
			add(candidate, false);
	}

	std::sort(shares.begin(), shares.end(), [](const TwistShare& a, const TwistShare& b) { return Depth(a.bone) < Depth(b.bone); });
	return shares;
}

// Each bone that takes part in a twist of the bone, with the LocalTwist it needs: the bone and the twist bones of
// its segment, each turned by its share. A bone below the bone, or below another of them, turns with it already and
// only needs the difference.
std::vector<std::pair<AnimBone*, float>> SpreadShares(AnimBone* bone, float twist, const TwistContext& context) {
	std::vector<TwistShare> all = SegmentTwistBones(bone, context);
	all.push_back({bone, 1.0f});
	std::stable_sort(all.begin(), all.end(), [](const TwistShare& a, const TwistShare& b) { return Depth(a.bone) < Depth(b.bone); });

	std::map<const AnimBone*, float> segmentTwist;
	std::vector<std::pair<AnimBone*, float>> shares;
	for (auto& share : all) {
		float inherited = 0.0f;
		for (const AnimBone* p = share.bone->parent; p; p = p->parent) {
			auto found = segmentTwist.find(p);
			if (found != segmentTwist.end()) {
				inherited = found->second;
				break;
			}
		}

		const float wanted = share.fraction * twist;
		segmentTwist[share.bone] = wanted;
		shares.emplace_back(share.bone, wanted - inherited);
	}
	return shares;
}

} // namespace

Vector3 RotateVector(const Vector3& v, const Vector3& rotation) {
	const float angle = rotation.length();
	if (angle < 1e-8f)
		return v;

	// Rodrigues' formula
	const Vector3 k = rotation / angle;
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	return v * c + k.cross(v) * s + k * (k.dot(v) * (1.0f - c));
}

Vector3 SwingRotation(Vector3 from, Vector3 to) {
	from.Normalize();
	to.Normalize();
	const float cosine = std::clamp(from.dot(to), -1.0f, 1.0f);
	Vector3 axis = from.cross(to);
	if (axis.length() < 1e-6f) {
		if (cosine > 0.0f)
			return Vector3();
		// Opposite: any axis across the direction does
		axis = from.cross(CrossAxis(from));
	}

	axis.Normalize();
	return axis * std::acos(cosine);
}

float SignedAngle(const Vector3& a, const Vector3& b, const Vector3& axis) {
	const Vector3 n = Normalized(axis);
	const Vector3 pa = a - n * a.dot(n);
	const Vector3 pb = b - n * b.dot(n);
	if (pa.length() < 1e-6f || pb.length() < 1e-6f)
		return 0.0f;
	return std::atan2(pa.cross(pb).dot(n), pa.dot(pb));
}

AnimBone* ReachChild(const AnimBone* bone) {
	auto reach = ReachBones(bone);
	return reach.size() == 1 ? const_cast<AnimBone*>(reach.front()) : nullptr;
}

bool ReachPosition(const AnimBone* bone, Vector3& outPosition) {
	Vector3 point;
	if (!LocalReachPoint(bone, point))
		return false;
	outPosition = bone->xformPoseToGlobal.ApplyTransform(point);
	return true;
}

bool BoneAxis(const AnimBone* bone, Vector3& outAxis) {
	Vector3 reach;
	if (ReachPosition(bone, reach)) {
		outAxis = Normalized(reach - Posed(bone));
		if (outAxis.length() > 0.5f)
			return true;
	}
	return bone->parent && BoneAxis(bone->parent, outAxis);
}

void TurnBoneGlobal(AnimBone* bone, const Vector3& rotation) {
	// The bone's frame before its own pose rotation: parent pose, then the bone's rest transform
	Matrix3 frameRotation = bone->xformToParent.rotation;
	if (bone->parent)
		frameRotation = bone->parent->xformPoseToGlobal.rotation * frameRotation;

	// RotVecToMat matrices turn the opposite way round in this composition, so the rotation is negated to turn
	// right-handed about the global axis
	Matrix3 local = frameRotation.Transpose() * RotVecToMat(rotation * -1.0f) * frameRotation * RotVecToMat(bone->poseRotVec);
	bone->poseRotVec = RotMatToVec(local);
	bone->UpdatePoseTransform();
}

void ShiftBoneGlobal(AnimBone* bone, const Vector3& offset) {
	// The pose translation is in the bone's frame before its own pose rotation
	MatTransform frame = UnrotatedFrame(bone);
	Vector3 local = frame.InverseTransform().ApplyTransformToDir(offset);
	if (frame.scale > 1e-6f)
		local /= frame.scale;
	bone->poseTranVec += local;
	bone->UpdatePoseTransform();
}

Vector3 AimRotation(const AnimBone* bone, Vector3 direction) {
	Vector3 current;
	if (!BoneAxis(bone, current))
		return Vector3();
	return SwingRotation(current, direction);
}

float LocalTwist(const AnimBone* bone) {
	Vector3 localDirection;
	if (!LocalReach(bone, localDirection))
		return 0.0f;

	const MatTransform unrotated = UnrotatedFrame(bone);
	const MatTransform& posed = bone->xformPoseToGlobal;

	const Vector3 restDirection = Normalized(unrotated.ApplyTransformToDir(localDirection));
	const Vector3 direction = Normalized(posed.ApplyTransformToDir(localDirection));

	// A reference across the bone, turned the shortest way with the bone, against where the pose has it
	const Vector3 across = CrossAxis(localDirection);
	const Vector3 swung = RotateVector(unrotated.ApplyTransformToDir(across), SwingRotation(restDirection, direction));
	return SignedAngle(swung, posed.ApplyTransformToDir(across), direction);
}

void SetLocalTwist(AnimBone* bone, float twist) {
	Vector3 axis;
	if (!BoneAxis(bone, axis))
		return;

	float change = twist - LocalTwist(bone);
	if (std::abs(change) < 1e-6f)
		return;
	TurnBoneGlobal(bone, axis * change);
}

BoneOrientation CaptureOrientation(const AnimBone* bone) {
	BoneOrientation orientation;
	Vector3 localDirection;
	if (!BoneAxis(bone, orientation.axis) || !LocalReach(bone, localDirection))
		return orientation;
	orientation.localAcross = CrossAxis(localDirection);
	orientation.across = bone->xformPoseToGlobal.ApplyTransformToDir(orientation.localAcross);
	return orientation;
}

void RestoreOrientation(AnimBone* bone, const BoneOrientation& orientation) {
	Vector3 axis;
	if (orientation.axis.IsZero(true) || !BoneAxis(bone, axis))
		return;

	TurnBoneGlobal(bone, SwingRotation(axis, orientation.axis));
	const Vector3 across = bone->xformPoseToGlobal.ApplyTransformToDir(orientation.localAcross);
	TurnBoneGlobal(bone, Normalized(orientation.axis) * SignedAngle(across, orientation.across, orientation.axis));
}

AnimBone* JointParent(const AnimBone* bone) {
	AnimBone* main = MainParent(bone);
	return main ? main : bone->parent;
}

float BendAngle(const AnimBone* bone) {
	const AnimBone* parent = JointParent(bone);
	Vector3 reach;
	if (!parent || !ReachPosition(bone, reach))
		return 0.0f;

	Vector3 upper = Posed(bone) - Posed(parent);
	Vector3 lower = reach - Posed(bone);
	if (upper.length() < SameSpot || lower.length() < SameSpot)
		return 0.0f;
	return std::acos(std::clamp(Normalized(upper).dot(Normalized(lower)), -1.0f, 1.0f));
}

bool BendsBackwards(const AnimBone* bone) {
	const AnimBone* parent = JointParent(bone);
	auto reach = ReachBones(bone);
	if (!parent || reach.empty() || BendAngle(bone) < 10.0f * DEG2RAD)
		return false;

	// The unposed bend: towards this bone, then towards the next joint
	const Vector3 restUpper = Normalized(bone->xformToGlobal.translation - parent->xformToGlobal.translation);
	const Vector3 restLower = Normalized(Middle(reach, false) - bone->xformToGlobal.translation);
	const Vector3 restHinge = restUpper.cross(restLower);
	if (restHinge.length() < BentSine)
		return false;

	// Carried along by the joint parent's pose
	const Vector3 parentLocal = parent->xformToGlobal.InverseTransform().ApplyTransformToDir(restHinge);
	const Vector3 expected = Normalized(parent->xformPoseToGlobal.ApplyTransformToDir(parentLocal));
	Vector3 posedReach;
	ReachPosition(bone, posedReach);
	const Vector3 posedUpper = Normalized(Posed(bone) - Posed(parent));
	const Vector3 posedLower = Normalized(posedReach - Posed(bone));
	return posedUpper.cross(posedLower).dot(expected) < 0.0f;
}

std::vector<TwistShare> SegmentTwistBones(const AnimBone* bone, const TwistContext& context) {
	// Along the bone itself, e.g. the upper arm
	Vector3 reach;
	if (ReachPosition(bone, reach)) {
		auto shares = TwistBonesAlong(Posed(bone), reach, bone, {}, {bone}, context);
		if (!shares.empty())
			return shares;
	}

	// A hand or foot twists the limb segment up to its joint, e.g. the forearm, with the bones of a chain between.
	// Other bones only twist themselves: a clavicle doesn't turn the spine.
	const std::string role = GetBoneRole(bone->boneName).role;
	if (role.rfind("hand", 0) != 0 && role.rfind("foot", 0) != 0)
		return {};

	if (AnimBone* start = SegmentStart(bone)) {
		std::vector<const AnimBone*> chain;
		for (const AnimBone* p = bone->parent; p && p != start; p = p->parent)
			if (IsMainBone(p->boneName))
				chain.push_back(p);
		std::vector<const AnimBone*> owners = chain;
		owners.push_back(start);

		auto shares = TwistBonesAlong(Posed(start), Posed(bone), bone, chain, owners, context);
		shares.erase(std::remove_if(shares.begin(), shares.end(), [&](const TwistShare& s) { return s.bone == start || IsDescendant(s.bone, bone); }), shares.end());
		return shares;
	}

	return {};
}

std::vector<std::pair<AnimBone*, float>> SetTwistSpread(AnimBone* bone, float twist, const TwistContext& context) {
	auto shares = SpreadShares(bone, twist, context);
	for (auto& share : shares)
		SetLocalTwist(share.first, share.second);

	shares.erase(std::remove_if(shares.begin(), shares.end(), [&](const std::pair<AnimBone*, float>& s) { return s.first == bone; }), shares.end());
	return shares;
}

float SegmentTwist(const AnimBone* bone, const TwistContext& context) {
	float twist = LocalTwist(bone);
	for (auto& share : SegmentTwistBones(bone, context))
		if (IsDescendant(bone, share.bone))
			twist += LocalTwist(share.bone);
	return twist;
}

float TwistSpreadError(const AnimBone* bone, const TwistContext& context) {
	float error = 0.0f;
	for (auto& share : SpreadShares(const_cast<AnimBone*>(bone), SegmentTwist(bone, context), context)) {
		if (share.first == bone)
			continue;
		const float difference = std::remainder(LocalTwist(share.first) - share.second, 2.0f * PI);
		error = std::max(error, std::abs(difference));
	}
	return error;
}

bool LimbChain(const AnimBone* end, AnimBone*& outUpper, AnimBone*& outLower) {
	AnimBone* lower = MainParent(end);
	if (!lower)
		return false;

	const std::string role = GetBoneRole(lower->boneName).role;
	AnimBone* upper = MainParent(lower);
	while (upper && GetBoneRole(upper->boneName).role == role) {
		lower = upper;
		upper = MainParent(lower);
	}
	if (!upper)
		return false;

	outUpper = upper;
	outLower = lower;
	return true;
}

ReachResult ReachTwoBone(AnimBone* end, const Vector3& target, const Vector3* pole) {
	ReachResult result;
	AnimBone* root = nullptr;
	AnimBone* mid = nullptr;
	if (!LimbChain(end, root, mid))
		return result;
	result.solved = true;
	result.upper = root;
	result.lower = mid;

	// Where the middle joint pointed before, as a fallback pole
	const Vector3 beforeLine = Posed(end) - Posed(root);
	Vector3 beforePole = Posed(mid) - Posed(root);
	if (beforeLine.length() > SameSpot)
		beforePole -= Normalized(beforeLine) * beforePole.dot(Normalized(beforeLine));

	// Solve from the unposed rotations of both bones, so no roll from earlier turns stays
	mid->poseRotVec = Vector3();
	root->poseRotVec = Vector3();
	root->UpdatePoseTransform();

	const Vector3 s = Posed(root);
	const Vector3 m0 = Posed(mid);
	const Vector3 w0 = Posed(end);
	const float a = (m0 - s).length();
	const float b = (w0 - m0).length();
	if (a < SameSpot || b < SameSpot) {
		result.solved = false;
		return result;
	}

	const Vector3 restHinge = (m0 - s).cross(w0 - m0) / (a * b);
	const bool restBent = restHinge.length() > BentSine;

	Vector3 toTarget = target - s;
	float distance = toTarget.length();
	if (distance < SameSpot) {
		toTarget = m0 - s;
		distance = SameSpot;
	}
	const Vector3 u = Normalized(toTarget);

	// A thousandth of the limb keeps the solution off a fully stretched or folded limb
	const float margin = (a + b) * 1e-3f;
	const float shortest = std::abs(a - b) + margin;
	const float longest = a + b - margin;
	result.reachable = distance >= shortest && distance <= longest;
	const float d = std::clamp(distance, shortest, longest);

	// The plane the limb bends in, from the pole
	Vector3 p;
	if (pole)
		p = *pole;
	else if (restBent) {
		Vector3 line = Normalized(w0 - s);
		p = (m0 - s) - line * (m0 - s).dot(line);
	}
	else
		p = beforePole;

	Vector3 v = p - u * p.dot(u);
	if (v.length() < 1e-6f)
		v = u.cross(CrossAxis(u));
	v.Normalize();

	const float x = (a * a - b * b + d * d) / (2.0f * d);
	const float h = std::sqrt(std::max(a * a - x * x, 0.0f));
	const Vector3 midTarget = s + u * x + v * h;
	const Vector3 endTarget = s + u * d;

	// The upper bone towards the middle joint, rolled so the joint bends in the pole's plane like it does unposed
	const Vector3 upper = Normalized(midTarget - s);
	TurnBoneGlobal(root, SwingRotation(m0 - s, upper));
	if (restBent) {
		Vector3 hinge = (Posed(mid) - s).cross(Posed(end) - Posed(mid));
		Vector3 wanted = (midTarget - s).cross(endTarget - midTarget);
		if (hinge.length() > 1e-9f && wanted.length() > 1e-9f) {
			TurnBoneGlobal(root, upper * SignedAngle(hinge, wanted, upper));
			result.hingeAligned = true;
		}
	}

	// The lower bone towards the end
	TurnBoneGlobal(mid, SwingRotation(Posed(end) - Posed(mid), endTarget - midTarget));

	result.midPosition = Posed(mid);
	result.endPosition = Posed(end);
	result.missed = (result.endPosition - target).length();
	return result;
}

bool IsMainBone(const std::string& name) {
	const std::string lower = Lower(name);

	static const char* excluded[] = {
		"twist", "finger", "thumb", "magic", "weapon", "shield", "quiver", "anus", "genital", "breast", "butt", "belly", "vagina", "pussy",
		"clit", "labia", "penis", "scrotum", "balls", "sack", "hdt", "skirt", "cloak", "cape", "hair", "tail", "ponytail", "camera",
		"eye", "jaw", "tongue", "lip", "brow", "cheek", "pauldron", "physic",
		// Physics and collision bones of bodies like CBBE 3BA (FrontThigh, RearCalf, PreFrontThigh). Not just "rear":
		// that's in "forearm".
		"front", "rearthigh", "rearcalf", "back", "pre",
		// Helpers at or near the joints: Fallout 4's _skin and Fat bones, Starfield's Elbow, Knee, Mass, Deltoid, Cup
		// and Pecs, Oblivion's Scale nodes and Shoulder Helper, IK targets
		"skin", "fat", "helper", "anim", "mass", "deltoid", "cup", "pecs", "nipple", "elbow", "knee", "handik", "scale",
		"shoe", "drake", "wing", "whip"};
	for (const char* part : excluded)
		if (Contains(lower, part))
			return false;

	static const char* included[] = {"pelvis", "hips", "spine", "chest", "neck", "head", "clavicle", "collarbone", "upperarm", "biceps",
									 "forearm", "hand", "wrist", "thigh", "calf", "foot", "toe"};
	for (const char* part : included)
		if (Contains(lower, part))
			return true;
	return false;
}

std::vector<AnimBone*> MainBones(const std::vector<AnimBone*>& skeleton) {
	std::vector<AnimBone*> main;
	for (AnimBone* bone : skeleton)
		if (bone && IsMainBone(bone->boneName))
			main.push_back(bone);

	for (const char* prefix : {"NPC ", "Bip01 "}) {
		auto hasPrefix = [&](const AnimBone* b) { return b->boneName.rfind(prefix, 0) == 0; };
		if (std::any_of(main.begin(), main.end(), hasPrefix)) {
			main.erase(std::remove_if(main.begin(), main.end(), [&](const AnimBone* b) { return !hasPrefix(b); }), main.end());
			break;
		}
	}

	// Bones of the reference skeleton, not custom bones a project added with a joint's name part (ThighsMain)
	auto standard = [](const AnimBone* b) { return b->isStandardBone; };
	if (std::any_of(main.begin(), main.end(), standard))
		main.erase(std::remove_if(main.begin(), main.end(), [&](const AnimBone* b) { return !standard(b); }), main.end());
	return main;
}

AnimBone* MainParent(const AnimBone* bone) {
	for (AnimBone* p = bone->parent; p; p = p->parent)
		if (IsMainBone(p->boneName))
			return p;
	return nullptr;
}

std::string ShortBoneName(const std::string& name) {
	const size_t open = name.rfind('[');
	const size_t close = name.rfind(']');
	if (open != std::string::npos && close != std::string::npos && close > open + 1) {
		std::string tag = name.substr(open + 1, close - open - 1);
		tag.erase(tag.find_last_not_of(' ') + 1);
		if (!tag.empty())
			return tag;
	}
	if (name.rfind("Bip01 ", 0) == 0 && name.size() > 6)
		return name.substr(6);
	return name;
}

BoneRole GetBoneRole(const std::string& name) {
	if (!IsMainBone(name))
		return {};

	const std::string lower = Lower(name);
	if (Contains(lower, "pelvis") || Contains(lower, "hips"))
		return {"pelvis", "tilts about 20 degrees to the side and 25 forward or back; tilting it raises one hip. It points down between the hips."};
	if (Contains(lower, "spine") || Contains(lower, "chest"))
		return {"spine segment", "each segment bends about 15 degrees to the side, 30 forward and 10 back, and twists 15"};
	if (Contains(lower, "neck"))
		return {"neck", "bends about 40 degrees to the side, 50 forward and 30 back, and turns 70"};
	if (Contains(lower, "head"))
		return {"head", "nods about 20 degrees and tilts 20 on top of the neck"};
	if (Contains(lower, "clavicle") || Contains(lower, "collarbone"))
		return {"clavicle (shoulder girdle)", "raises about 30 degrees and moves 20 forward or back; it lifts with an arm raised above the shoulder"};
	if (Contains(lower, "upperarm") || Contains(lower, "biceps"))
		return {"upper arm (ball joint: shoulder)", "raises to about 170 degrees from hanging, 180 forward and 50 back; rolls about 90 either way"};
	if (Contains(lower, "forearm"))
		return {"forearm (hinge: elbow)", "bends 0 to 145 degrees one way only; the hand's twist spreads over the forearm's twist bones, about 80 degrees either way"};
	if (Contains(lower, "hand") || Contains(lower, "wrist"))
		return {"hand (wrist)", "bends about 70 degrees towards the palm or back and 25 to the side; twist comes from the forearm"};
	if (Contains(lower, "thigh"))
		return {"thigh (ball joint: hip)", "about 120 degrees forward, 20 back, 45 out and 25 in; rolls about 40 either way"};
	if (Contains(lower, "calf"))
		return {"calf (hinge: knee)", "bends 0 to 150 degrees backwards only"};
	if (Contains(lower, "foot"))
		return {"foot (ankle)", "about 20 degrees up, 45 down and 20 to the side"};
	if (Contains(lower, "toe"))
		return {"toes", "bend about 40 degrees up and 30 down"};
	return {};
}

} // namespace PoseMath
