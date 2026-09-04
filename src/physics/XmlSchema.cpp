/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "XmlSchema.h"

#include <cstring>

namespace Physics {
namespace {
bool NameEquals(const char* a, const char* b) {
	return a && b && std::strcmp(a, b) == 0;
}

// "coneLimit|limitZ" contains "limitZ"
bool AliasMatches(const char* aliases, const char* name) {
	if (!aliases || !name)
		return false;

	const std::size_t len = std::strlen(name);
	for (const char* p = aliases; *p;) {
		const char* end = std::strchr(p, '|');
		const std::size_t span = end ? static_cast<std::size_t>(end - p) : std::strlen(p);
		if (span == len && std::strncmp(p, name, span) == 0)
			return true;

		if (!end)
			break;
		p = end + 1;
	}
	return false;
}

const char* const kSharedValues[] = {"public", "private", "internal", "external"};
const EnumValues kSharedEnum{kSharedValues, 4};

const char* const kShapeTypeValues[] = {"ref", "box", "sphere", "capsule", "hull", "cylinder", "compound"};
const EnumValues kShapeTypeEnum{kShapeTypeValues, 7};

// Shorthand so the tables below stay readable.
constexpr float kInf = kUnbounded;
using VK = ValueKind;
using MU = Multiplicity;
using PS = PreviewSupport;
using PK = PatchKind;

/* ---------------------------------------------------------------- bone --- */

// Shared by <bone> and <bone-default>: the XSD gives them identical content.
const ChildDesc kBoneChildren[] = {
	{"mass", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, "kg", PS::Simulated, PK::Hot, nullptr,
	 "Rigid body mass. 0 makes the bone kinematic: it follows the pose and the simulation never moves it. "
	 "Changing the value rebuilds the system whenever it crosses 0, because that switches the body between "
	 "kinematic and dynamic."},
	{"inertia", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Diagonal of the local inertia tensor, in the bone's own frame. It is used as given rather than computed "
	 "from the collision shape, and 0 on an axis means infinite inertia about it, so the default of 0 0 0 is a "
	 "bone that cannot rotate at all. Larger values resist rotation more."},
	{"centerOfMassTransform", nullptr, VK::Transform, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Moves the center of mass and rotates the principal axes relative to the bone. Every constraint frame is "
	 "computed against this, so changing it rebuilds the system."},
	{"linearDamping", nullptr, VK::Factor, MU::Once, "0", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Velocity is multiplied by (1 - value) every second. On a bone this is a correction rather than real damping; "
	 "damping the constraint is usually the better tool. No effect when mass is 0."},
	{"angularDamping", nullptr, VK::Factor, MU::Once, "0", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "As linearDamping, for angular velocity. No effect when mass is 0."},
	{"gravity-factor", nullptr, VK::Factor, MU::Once, "1", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Scales gravity for this bone. 0 makes it weightless. The preview clamps this to 0..1. No effect when mass is 0."},
	{"wind-factor", nullptr, VK::PosFloat, MU::Once, "1", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Scales wind for this bone. The preview clamps negative values to 0. No effect when mass is 0."},
	{"friction", nullptr, VK::PosFloat, MU::Once, "0.5", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Sliding friction against everything this bone touches."},
	{"rollingFriction", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Torque opposing rolling. Keeps round colliders from rolling forever."},
	{"restitution", nullptr, VK::Float, MU::Once, "0", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Bounciness. 0 is fully inelastic, 1 loses no energy in a collision."},
	{"margin-multiplier", nullptr, VK::PosFloat, MU::Once, "1", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Scales the collision margin of every shape attached to this bone."},
	{"shape", nullptr, VK::None, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "The rigid collider carried by this bone."},
	{"can-collide-with-bone", nullptr, VK::BoneRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Whitelist of bones this bone may collide with. Read into the bone template but never transferred to the bone, "
	 "so it has no effect in the preview; the per-shape lists are the ones that work."},
	{"no-collide-with-bone", nullptr, VK::BoneRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Blacklist of bones this bone must not collide with. Same limitation as can-collide-with-bone."},
	{"collision-filter", nullptr, VK::Int, MU::Once, "0", -kInf, kInf, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Legacy, not part of the schema. Read and stored, then never used."},
};

const AttrDesc kBoneAttrs[] = {
	{"name", VK::BoneRef, true, nullptr, "Name of the skeleton node this bone drives. Must be unique in the file."},
	{"template", VK::String, false, nullptr, "Name of a bone-default to start from."},
};

const AttrDesc kDefaultAttrs[] = {
	{"name", VK::String, false, nullptr,
	 "Names this template so instances can select it with template=. Leave it out to change the ambient default for "
	 "everything that follows in the file."},
	{"extends", VK::String, false, nullptr, "Name of another template of the same kind to inherit from."},
};

/* --------------------------------------------------------------- shape --- */

const ChildDesc kShapeChildren[] = {
	{"halfExtend", nullptr, VK::Vec3, MU::Once, nullptr, -kInf, kInf, "units", PS::Simulated, PK::Structural, nullptr,
	 "Half the size of the box along each axis. For type box."},
	{"margin", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, "units", PS::Simulated, PK::Structural, nullptr,
	 "Collision margin. Applied for box, added to the radius for cylinder, read and discarded for hull, and not "
	 "supported at all for sphere or capsule."},
	{"radius", nullptr, VK::PosFloat, MU::Once, nullptr, 0.0f, kInf, "units", PS::Simulated, PK::Structural, nullptr,
	 "Radius. For types sphere, capsule and cylinder."},
	{"height", nullptr, VK::PosFloat, MU::Once, nullptr, 0.0f, kInf, "units", PS::Simulated, PK::Structural, nullptr,
	 "For capsule, the distance between the two cap centers along the local Y axis. For cylinder this value reaches "
	 "Bullet as a half extent, so the cylinder ends up twice as tall as the number reads."},
	{"point", nullptr, VK::Vec3, MU::List, nullptr, -kInf, kInf, "units", PS::Simulated, PK::Structural, nullptr,
	 "One vertex of a convex hull. For type hull; at least four are needed for a solid."},
	{"child", nullptr, VK::None, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "A transform plus a nested shape. For type compound."},
};

const AttrDesc kShapeAttrs[] = {
	{"type", VK::Enum, true, &kShapeTypeEnum, "Shape type."},
	{"name", VK::String, false, nullptr,
	 "Names the shape so type=\"ref\" can point at it. The schema marks it optional, but a top level <shape> without "
	 "a name aborts the whole file in this preview."},
};

const ChildDesc kShapeChildChildren[] = {
	{"transform", nullptr, VK::Transform, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Where the nested shape sits inside the compound."},
	{"shape", nullptr, VK::None, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "The nested shape."},
};

/* ------------------------------------------------- skinned mesh shapes --- */

// Everything per-vertex-shape and per-triangle-shape have in common. The
// per-triangle table repeats these and adds penetration.
#define BSOS_SKINNED_SHAPE_COMMON \
	{"margin", nullptr, VK::PosFloat, MU::Once, "1", 0.0f, kInf, "units", PS::Simulated, PK::Hot, nullptr, \
	 "Collision skin thickness around every collider of this shape. Larger values make it react earlier and look puffier."}, \
		{"tag", nullptr, VK::String, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr, \
		 "A label other shapes can name in their collide lists. A shape may carry several."}, \
		{"shared", nullptr, VK::Enum, MU::Once, "public", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, &kSharedEnum, \
		 "Which skeletons and meshes this shape may meet. Outfit Studio simulates a single actor, so internal behaves " \
		 "exactly like public here and external never collides with anything at all."}, \
		{"can-collide-with-tag", nullptr, VK::TagRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr, \
		 "Whitelist. As soon as this list has one entry it decides everything and no-collide-with-tag stops mattering."}, \
		{"no-collide-with-tag", nullptr, VK::TagRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr, \
		 "Blacklist, consulted only while can-collide-with-tag is empty."}, \
		{"can-collide-with-bone", nullptr, VK::BoneRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr, \
		 "Whitelist of bones this shape may collide with. A separate gate from the tag lists; both have to pass."}, \
		{"no-collide-with-bone", nullptr, VK::BoneRef, MU::List, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr, \
		 "Blacklist of bones, consulted only while can-collide-with-bone is empty."}, \
		{"weight-threshold", nullptr, VK::Float, MU::List, nullptr, 0.0f, 1.0f, nullptr, PS::Simulated, PK::Structural, nullptr, \
		 "Per bone: skin weights at or below this contribute nothing, neither moving the collider nor taking the " \
		 "collision response. Needs a bone attribute. Baked into the colliders at build time."}, \
		{"disable-tag", nullptr, VK::String, MU::Once, nullptr, -kInf, kInf, nullptr, PS::ParsedNotApplied, PK::Hot, nullptr, \
		 "Groups shapes that switch each other off in the game. Stored here but never acted on."}, \
		{"disable-priority", nullptr, VK::Int, MU::Once, "0", -kInf, kInf, nullptr, PS::ParsedNotApplied, PK::Hot, nullptr, \
		 "Within one disable-tag the lower priority is the one switched off. Stored here but never acted on."}, \
	{                                                                                                                  \
		"priority", nullptr, VK::Int, MU::Once, nullptr, -kInf, kInf, nullptr, PS::NotParsed, PK::Structural, nullptr,  \
			"Deprecated and no longer used; the parser warns about it."                                                 \
	}

const ChildDesc kPerVertexShapeChildren[] = {
	BSOS_SKINNED_SHAPE_COMMON,
};

const ChildDesc kPerTriangleShapeChildren[] = {
	BSOS_SKINNED_SHAPE_COMMON,
	{"penetration", "prenetration", VK::Float, MU::Once, "1", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Scales the penetration depth used for the collision response. Negative values push the wrong way on purpose. "
	 "prenetration is the older spelling of the same thing."},
};

#undef BSOS_SKINNED_SHAPE_COMMON

const AttrDesc kSkinnedShapeAttrs[] = {
	{"name", VK::NiShapeRef, true, nullptr,
	 "Name of the NiShape this collision body is built from. Every loaded shape of that name is merged into one body."},
	{"template", VK::String, false, nullptr,
	 "Name of a per-*-shape-default to start from. The preview does not implement those defaults, so this has no "
	 "effect here."},
};

const AttrDesc kWeightThresholdAttrs[] = {
	{"bone", VK::BoneRef, true, nullptr, "The skinned bone this threshold applies to."},
};

/* --------------------------------------------------------- constraints --- */

const AttrDesc kConstraintAttrs[] = {
	{"bodyA", VK::BoneRef, true, nullptr, "First bone. Created from the skeleton on the fly if the file never declares it."},
	{"bodyB", VK::BoneRef, true, nullptr, "Second bone."},
	{"template", VK::String, false, nullptr, "Name of a matching *-constraint-default to start from."},
};

// Frames, shared by generic-constraint and conetwist-constraint. All of them
// are resolved against the build time bone transforms, so all of them rebuild.
#define BSOS_CONSTRAINT_FRAMES \
	{"frameInA", nullptr, VK::Transform, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr, \
	 "Constraint frame given in bodyA's space. The last frame element in a block wins."}, \
		{"frameInB", nullptr, VK::Transform, MU::Once, nullptr, -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr, \
		 "Constraint frame given in bodyB's space. This is what is used when no frame is given at all."}, \
	{                                                                                                                     \
		"frameInLerp", nullptr, VK::Lerp, MU::Once, nullptr, 0.0f, 1.0f, nullptr, PS::Simulated, PK::Structural, nullptr,  \
			"Interpolates the frame between the two bodies: 0 is entirely bodyA, 1 entirely bodyB."                        \
	}

const ChildDesc kGenericConstraintChildren[] = {
	BSOS_CONSTRAINT_FRAMES,
	{"useLinearReferenceFrameA", nullptr, VK::Bool, MU::Once, "false", -kInf, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Builds the constraint with the two bodies swapped, so limits and springs are read in bodyA's frame."},

	{"linearLowerLimit", nullptr, VK::Vec3, MU::Once, "1 1 1", -kInf, kInf, "units", PS::Simulated, PK::Hot, nullptr,
	 "Lower translation limit per axis. An axis whose lower limit sits above its upper limit is free, which is exactly "
	 "what the default pair means."},
	{"linearUpperLimit", nullptr, VK::Vec3, MU::Once, "-1 -1 -1", -kInf, kInf, "units", PS::Simulated, PK::Hot, nullptr,
	 "Upper translation limit per axis."},
	{"angularLowerLimit", nullptr, VK::Vec3, MU::Once, "1 1 1", -kInf, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "Lower rotation limit per axis, in radians. Lower above upper leaves the axis free."},
	{"angularUpperLimit", nullptr, VK::Vec3, MU::Once, "-1 -1 -1", -kInf, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "Upper rotation limit per axis, in radians."},

	{"linearStiffness", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Spring constant per axis. k = mass * 4 * pi^2 * frequency^2, so 1 kg oscillating twice a second needs about 158."},
	{"angularStiffness", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Angular spring constant per axis."},
	{"linearDamping", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Damping per axis. Unlike the bone element of the same name this is real damping, and it is the one to reach for."},
	{"angularDamping", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Angular damping per axis."},
	{"linearEquilibrium", nullptr, VK::Vec3, MU::Once, "0 0 0", -kInf, kInf, "units", PS::Simulated, PK::Hot, nullptr,
	 "Rest position of the linear springs, and the target the linear servo motors aim at."},
	{"angularEquilibrium", nullptr, VK::Vec3, MU::Once, "0 0 0", -kInf, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "Rest orientation of the angular springs, and the target the angular servo motors aim at."},
	{"enableLinearSprings", nullptr, VK::Bool, MU::Once, "true", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Turns the three linear springs on or off."},
	{"enableAngularSprings", nullptr, VK::Bool, MU::Once, "true", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Turns the three angular springs on or off."},

	{"linearStiffnessLimited", nullptr, VK::Bool, MU::Once, "true", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Caps linear stiffness at what the timestep can solve. Usually better off, with the stiffness tuned properly instead."},
	{"angularStiffnessLimited", nullptr, VK::Bool, MU::Once, "true", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Caps angular stiffness at what the timestep can solve."},
	{"springDampingLimited", nullptr, VK::Bool, MU::Once, "true", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Caps damping at mass over timestep. Breaks down on scaled bodies, so usually better off."},
	{"linearSpringFrequency", nullptr, VK::Vec3, MU::Once, "0.25 0.25 0.25", 0.0f, 1.0f, nullptr, PS::NotParsed, PK::Structural, nullptr,
	 "Per axis coefficient for the linear stiffness cap. This preview's parser does not know the element."},
	{"angularSpringFrequency", nullptr, VK::Vec3, MU::Once, "0.25 0.25 0.25", 0.0f, 1.0f, nullptr, PS::NotParsed, PK::Structural, nullptr,
	 "Per axis coefficient for the angular stiffness cap. This preview's parser does not know the element."},

	{"linearNonHookeanStiffness", nullptr, VK::Vec3, MU::Once, "0 0 0", -0.75f, 0.9f, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Makes linear stiffness vary with distance from equilibrium. Needs the patched Bullet the game plugin ships with, "
	 "so it is read and then ignored here."},
	{"angularNonHookeanStiffness", nullptr, VK::Vec3, MU::Once, "0 0 0", -0.75f, 0.9f, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Angular counterpart, ignored here for the same reason."},
	{"linearNonHookeanDamping", nullptr, VK::Vec3, MU::Once, "0 0 0", -0.75f, 0.9f, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Makes linear damping vary with distance from equilibrium. Ignored here."},
	{"angularNonHookeanDamping", nullptr, VK::Vec3, MU::Once, "0 0 0", -0.75f, 0.9f, nullptr, PS::ParsedNotApplied, PK::Structural, nullptr,
	 "Angular counterpart, ignored here."},

	{"linearMotors", nullptr, VK::Bool, MU::Once, "false", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Drives the linear axes towards linearTargetVelocity."},
	{"angularMotors", nullptr, VK::Bool, MU::Once, "false", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Drives the angular axes towards angularTargetVelocity."},
	{"linearServoMotors", nullptr, VK::Bool, MU::Once, "false", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Turns the linear motors into servos aiming at linearEquilibrium. There is no separate servo target element."},
	{"angularServoMotors", nullptr, VK::Bool, MU::Once, "false", -kInf, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Turns the angular motors into servos aiming at angularEquilibrium."},
	{"linearTargetVelocity", nullptr, VK::Vec3, MU::Once, "0 0 0", -kInf, kInf, "units/s", PS::Simulated, PK::Hot, nullptr,
	 "Velocity the linear motors aim for."},
	{"angularTargetVelocity", nullptr, VK::Vec3, MU::Once, "0 0 0", -kInf, kInf, "rad/s", PS::Simulated, PK::Hot, nullptr,
	 "Angular velocity the angular motors aim for."},
	{"linearMaxMotorForce", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Force ceiling for the linear motors. 0 means no practical limit."},
	{"angularMaxMotorForce", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Torque ceiling for the angular motors."},

	{"motorERP", nullptr, VK::Factor, MU::Once, "0.9", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Share of the motor's error corrected each step."},
	{"motorCFM", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Motor softness. 0 is rigid."},
	{"stopERP", nullptr, VK::Factor, MU::Once, "0.2", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Share of a limit violation corrected each step."},
	{"stopCFM", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Limit softness. 0 is a hard wall."},

	{"linearBounce", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Restitution at the linear limits. Anything but 0 tends to jitter badly at these speeds."},
	{"angularBounce", nullptr, VK::Vec3, MU::Once, "0 0 0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Restitution at the angular limits. The same warning applies."},
};

const ChildDesc kStiffSpringConstraintChildren[] = {
	// The three factors below are multiplied into the distance the two bones
	// happened to be apart when the constraint was constructed, and that
	// distance is not kept, so re-applying them live would compound. They
	// rebuild.
	{"minDistanceFactor", nullptr, VK::PosFloat, MU::Once, "1", 0.0f, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Multiplies the distance the two bones started at to give the shortest allowed distance."},
	{"maxDistanceFactor", nullptr, VK::PosFloat, MU::Once, "1", 0.0f, kInf, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Multiplies the starting distance to give the longest allowed distance."},
	{"equilibrium", nullptr, VK::Factor, MU::Once, "0.5", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Structural, nullptr,
	 "Rest distance, mixed between the two limits. Note the mix runs the other way round than it reads: 0 is the "
	 "maximum distance and 1 the minimum."},
	{"stiffness", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Spring constant along the line between the two bones."},
	{"damping", nullptr, VK::PosFloat, MU::Once, "0", 0.0f, kInf, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "Damping along the line between the two bones."},
};

const ChildDesc kConeTwistConstraintChildren[] = {
	BSOS_CONSTRAINT_FRAMES,
	{"swingSpan1", "coneLimit|limitZ", VK::PosFloat, MU::Once, "0", 0.0f, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "First swing half angle, in radians. 0 locks the axis."},
	{"swingSpan2", "planeLimit|limitY", VK::PosFloat, MU::Once, "0", 0.0f, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "Second swing half angle, in radians."},
	{"twistSpan", "twistLimit|limitX", VK::PosFloat, MU::Once, "0", 0.0f, kInf, "rad", PS::Simulated, PK::Hot, nullptr,
	 "Twist half angle, in radians."},
	{"limitSoftness", nullptr, VK::Factor, MU::Once, "1", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "How much of the span stays free before the limit starts to bite. Bullet suggests 0.8 to 1."},
	{"biasFactor", nullptr, VK::Factor, MU::Once, "0.3", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "How hard the limit pushes back. Bullet suggests 0 to 0.6."},
	{"relaxationFactor", nullptr, VK::Factor, MU::Once, "1", 0.0f, 1.0f, nullptr, PS::Simulated, PK::Hot, nullptr,
	 "How fast a violated limit is corrected. Bullet suggests 0.8 to 1."},
};

#undef BSOS_CONSTRAINT_FRAMES

/* ------------------------------------------------------------ patterns --- */

// The pattern macro system is expanded by the game plugin before its own
// validator ever sees the file. Nothing here expands it, so these elements
// simulate as nothing at all; they exist in the table only so the editor can
// name them, keep them and warn about them.
const AttrDesc kPatternAttrs[] = {
	{"name", VK::String, true, nullptr, "Full pattern name, author.name when the pattern declares an author."},
	{"version", VK::String, false, nullptr, "Pattern version."},
};

const AttrDesc kPatternDefaultAttrs[] = {
	{"name", VK::String, true, nullptr, "Pattern name."},
	{"author", VK::String, false, nullptr, "Pattern author, which becomes the namespace of the full name."},
	{"version", VK::String, false, nullptr, "Pattern version."},
};

const AttrDesc kParamAttrs[] = {
	{"name", VK::String, true, nullptr, "Parameter name, referenced as ${name} inside the body."},
	{"default", VK::String, false, nullptr, "Value used when an instantiation leaves the parameter out. Without one the parameter is required."},
};

const AttrDesc kRepeatAttrs[] = {
	{"var", VK::String, true, nullptr, "Name of the loop variable."},
	{"count", VK::String, true, nullptr, "How many times to emit the content."},
	{"from", VK::String, false, nullptr, "First value of the loop variable. Defaults to 0."},
};

/* --------------------------------------------------------- the elements --- */

#define BSOS_SPAN(a) a, sizeof(a) / sizeof(a[0])
#define BSOS_NO_ATTRS nullptr, 0
#define BSOS_NO_CHILDREN nullptr, 0

const ElementDesc kElements[] = {
	{"system", ElementKind::System, BSOS_NO_ATTRS, BSOS_NO_CHILDREN,
	 "Root of a physics file. Its children may appear in any order, but the order they appear in matters: an unnamed "
	 "*-default applies to everything after it."},

	{"bone", ElementKind::Bone, BSOS_SPAN(kBoneAttrs), BSOS_SPAN(kBoneChildren),
	 "One rigid body, bound to the skeleton node of the same name. Bones named by a constraint or a collide list are "
	 "created on the fly from the current defaults, so not every bone needs declaring."},
	{"bone-default", ElementKind::BoneDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kBoneChildren),
	 "Default values for the bones that follow it."},

	{"per-vertex-shape", ElementKind::PerVertexShape, BSOS_SPAN(kSkinnedShapeAttrs), BSOS_SPAN(kPerVertexShapeChildren),
	 "A collision body built from the vertices of a NiShape: one sphere per vertex, sized by the vertex margin."},
	{"per-vertex-shape-default", ElementKind::PerVertexShapeDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kPerVertexShapeChildren),
	 "Default values for the per-vertex shapes that follow it. This preview does not implement these defaults."},

	{"per-triangle-shape", ElementKind::PerTriangleShape, BSOS_SPAN(kSkinnedShapeAttrs), BSOS_SPAN(kPerTriangleShapeChildren),
	 "A collision body built from the triangles of a NiShape, with a per-vertex shape generated alongside it."},
	{"per-triangle-shape-default", ElementKind::PerTriangleShapeDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kPerTriangleShapeChildren),
	 "Default values for the per-triangle shapes that follow it. This preview does not implement these defaults."},

	{"generic-constraint", ElementKind::GenericConstraint, BSOS_SPAN(kConstraintAttrs), BSOS_SPAN(kGenericConstraintChildren),
	 "A six degree of freedom constraint with a spring and a motor per axis. The usual choice."},
	{"generic-constraint-default", ElementKind::GenericConstraintDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kGenericConstraintChildren),
	 "Default values for the generic constraints that follow it."},

	{"stiffspring-constraint", ElementKind::StiffSpringConstraint, BSOS_SPAN(kConstraintAttrs), BSOS_SPAN(kStiffSpringConstraintChildren),
	 "A spring along the line between two bones, holding them a set distance apart."},
	{"stiffspring-constraint-default", ElementKind::StiffSpringConstraintDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kStiffSpringConstraintChildren),
	 "Default values for the stiff spring constraints that follow it."},

	{"conetwist-constraint", ElementKind::ConeTwistConstraint, BSOS_SPAN(kConstraintAttrs), BSOS_SPAN(kConeTwistConstraintChildren),
	 "A swing and twist constraint. The schema itself recommends a generic constraint instead."},
	{"conetwist-constraint-default", ElementKind::ConeTwistConstraintDefault, BSOS_SPAN(kDefaultAttrs), BSOS_SPAN(kConeTwistConstraintChildren),
	 "Default values for the cone twist constraints that follow it."},

	{"constraint-group", ElementKind::ConstraintGroup, BSOS_NO_ATTRS, BSOS_NO_CHILDREN,
	 "Groups constraints so they are solved together. Note that a *-default declared inside a group is not scoped to "
	 "it: it goes on applying to the rest of the file."},

	{"shape", ElementKind::Shape, BSOS_SPAN(kShapeAttrs), BSOS_SPAN(kShapeChildren),
	 "A rigid collider, either named at the top level for later reuse or given inline inside a bone."},
	{"child", ElementKind::ShapeChild, BSOS_NO_ATTRS, BSOS_SPAN(kShapeChildChildren),
	 "One part of a compound shape."},
	{"weight-threshold", ElementKind::WeightThreshold, BSOS_SPAN(kWeightThresholdAttrs), BSOS_NO_CHILDREN,
	 "The skin weight below which one bone stops contributing to a shape's colliders."},

	{"pattern", ElementKind::Pattern, BSOS_SPAN(kPatternAttrs), BSOS_NO_CHILDREN,
	 "Instantiates a shared pattern. Every extra attribute is a parameter value. Not expanded by this preview."},
	{"pattern-default", ElementKind::PatternDefault, BSOS_SPAN(kPatternDefaultAttrs), BSOS_NO_CHILDREN,
	 "Declares a reusable pattern. Not expanded by this preview."},
	{"repeat", ElementKind::Repeat, BSOS_SPAN(kRepeatAttrs), BSOS_NO_CHILDREN,
	 "Emits its content once per loop iteration. Not expanded by this preview."},
	{"param", ElementKind::Param, BSOS_SPAN(kParamAttrs), BSOS_NO_CHILDREN,
	 "One parameter of a pattern."},
	{"body", ElementKind::PatternBody, BSOS_NO_ATTRS, BSOS_NO_CHILDREN,
	 "The content a pattern emits, with ${placeholders} for its parameters."},
	{"patterns", ElementKind::Patterns, BSOS_NO_ATTRS, BSOS_NO_CHILDREN,
	 "Root of a shared pattern library file. Not read by this preview."},
};

#undef BSOS_SPAN
#undef BSOS_NO_ATTRS
#undef BSOS_NO_CHILDREN

constexpr std::size_t kElementCount = sizeof(kElements) / sizeof(kElements[0]);

const std::vector<const ElementDesc*>& ElementList() {
	static const std::vector<const ElementDesc*> list = [] {
		std::vector<const ElementDesc*> out;
		out.reserve(kElementCount);
		for (std::size_t i = 0; i < kElementCount; i++)
			out.push_back(&kElements[i]);
		return out;
	}();
	return list;
}
}

const AttrDesc* ElementDesc::Attr(const char* attrName) const {
	for (std::size_t i = 0; i < attrCount; i++) {
		if (NameEquals(attrs[i].name, attrName))
			return &attrs[i];
	}
	return nullptr;
}

const ChildDesc* ElementDesc::Child(const char* childName) const {
	for (std::size_t i = 0; i < childCount; i++) {
		if (NameEquals(children[i].name, childName) || AliasMatches(children[i].aliases, childName))
			return &children[i];
	}
	return nullptr;
}

const std::vector<const ElementDesc*>& AllElements() {
	return ElementList();
}

const ElementDesc* FindElement(const char* name) {
	for (std::size_t i = 0; i < kElementCount; i++) {
		if (NameEquals(kElements[i].name, name))
			return &kElements[i];
	}
	return nullptr;
}

const ElementDesc* FindElement(ElementKind kind) {
	for (std::size_t i = 0; i < kElementCount; i++) {
		if (kElements[i].kind == kind)
			return &kElements[i];
	}
	return nullptr;
}

ElementKind KindFromName(const char* name) {
	const ElementDesc* desc = FindElement(name);
	return desc ? desc->kind : ElementKind::Unknown;
}

const char* NameFromKind(ElementKind kind) {
	const ElementDesc* desc = FindElement(kind);
	return desc ? desc->name : "";
}

bool IsDefaultKind(ElementKind kind) {
	switch (kind) {
		case ElementKind::BoneDefault:
		case ElementKind::PerVertexShapeDefault:
		case ElementKind::PerTriangleShapeDefault:
		case ElementKind::GenericConstraintDefault:
		case ElementKind::StiffSpringConstraintDefault:
		case ElementKind::ConeTwistConstraintDefault: return true;
		default: return false;
	}
}

ElementKind InstanceKindOf(ElementKind kind) {
	switch (kind) {
		case ElementKind::BoneDefault: return ElementKind::Bone;
		case ElementKind::PerVertexShapeDefault: return ElementKind::PerVertexShape;
		case ElementKind::PerTriangleShapeDefault: return ElementKind::PerTriangleShape;
		case ElementKind::GenericConstraintDefault: return ElementKind::GenericConstraint;
		case ElementKind::StiffSpringConstraintDefault: return ElementKind::StiffSpringConstraint;
		case ElementKind::ConeTwistConstraintDefault: return ElementKind::ConeTwistConstraint;
		default: return kind;
	}
}

ElementKind DefaultKindOf(ElementKind kind) {
	switch (kind) {
		case ElementKind::Bone: return ElementKind::BoneDefault;
		case ElementKind::PerVertexShape: return ElementKind::PerVertexShapeDefault;
		case ElementKind::PerTriangleShape: return ElementKind::PerTriangleShapeDefault;
		case ElementKind::GenericConstraint: return ElementKind::GenericConstraintDefault;
		case ElementKind::StiffSpringConstraint: return ElementKind::StiffSpringConstraintDefault;
		case ElementKind::ConeTwistConstraint: return ElementKind::ConeTwistConstraintDefault;
		default: return IsDefaultKind(kind) ? kind : ElementKind::Unknown;
	}
}

bool IsSystemChildKind(ElementKind kind) {
	switch (kind) {
		case ElementKind::Bone:
		case ElementKind::BoneDefault:
		case ElementKind::PerVertexShape:
		case ElementKind::PerVertexShapeDefault:
		case ElementKind::PerTriangleShape:
		case ElementKind::PerTriangleShapeDefault:
		case ElementKind::GenericConstraint:
		case ElementKind::GenericConstraintDefault:
		case ElementKind::StiffSpringConstraint:
		case ElementKind::StiffSpringConstraintDefault:
		case ElementKind::ConeTwistConstraint:
		case ElementKind::ConeTwistConstraintDefault:
		case ElementKind::ConstraintGroup:
		case ElementKind::Shape:
		case ElementKind::Pattern:
		case ElementKind::PatternDefault:
		case ElementKind::Repeat: return true;
		default: return false;
	}
}
bool CascadeApplies(ElementKind instanceKind) {
	// SystemBuilder's parse loop handles bone-default and the three
	// *-constraint-default elements; per-vertex-shape-default and
	// per-triangle-shape-default fall through to "unknown element" and are
	// skipped, so nothing a shape template says reaches this preview.
	switch (InstanceKindOf(instanceKind)) {
		case ElementKind::Bone:
		case ElementKind::GenericConstraint:
		case ElementKind::StiffSpringConstraint:
		case ElementKind::ConeTwistConstraint: return true;
		default: return false;
	}
}


const char* PreviewSupportNote(PreviewSupport support) {
	switch (support) {
		case PreviewSupport::Simulated: return "";
		case PreviewSupport::ParsedNotApplied: return "read from the file and kept, but it changes nothing in this preview";
		case PreviewSupport::NotParsed: return "not understood by this preview's parser; kept in the file";
	}
	return "";
}
}
