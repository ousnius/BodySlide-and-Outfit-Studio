/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Controller.h"

// The wind direction list is part of the interface both applications build
// their controls from, so it exists with or without Bullet.
namespace Physics {
const std::vector<std::string>& WindDirectionNames() {
	static const std::vector<std::string> names{"Up", "Down", "Forward", "Backward", "Left", "Right"};
	return names;
}

WindDirection WindDirectionFromIndex(int index) {
	static const WindDirection directions[]
		= {WindDirection::Up, WindDirection::Down, WindDirection::Forward, WindDirection::Backward, WindDirection::Left, WindDirection::Right};

	if (index < 0 || index >= static_cast<int>(sizeof(directions) / sizeof(directions[0])))
		return WindDirection::Right;

	return directions[index];
}
}

#ifdef USE_BULLET

#include "NiflyBullet.h"
#include "DebugDraw.h"
#include "SystemBuilder.h"
#include "hdt/hdtSkinnedMeshShape.h"
#include "hdt/hdtSkinnedMeshWorld.h"

#include "../components/Anim.h"

#include <ExtraData.hpp>
#include <NifFile.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <type_traits>

namespace Physics {
// The skinning code takes the override map through Anim.h, which cannot depend
// on this module; keep the two spellings of the type interchangeable.
static_assert(std::is_same<PoseOverrideMap, AnimPoseOverrideMap>::value, "PoseOverrideMap must match AnimPoseOverrideMap");

namespace {
	// The NiStringExtraData name the game scans for (case-insensitive, like
	// BSFixedString comparison in hdtSMP64's DefaultBBP scan).
	const hdt::IDStr physicsExtraDataName("HDT Skinned Mesh Physics Object");

	// Same world units as hdtConvertNi.h upstream
	constexpr float scaleRealWorld = 0.01425f;
	constexpr float scaleSkyrim = 1.0f / scaleRealWorld;

	// Wind strength 1 is well past the strongest vanilla weather wind (which
	// is scaleSkyrim): exaggerated wind is the point of a test tool, and the
	// in-game maximum barely moves stiffer setups.
	constexpr float maxWindScale = 3.0f;

	// How long the grab spring would take to close the gap to the cursor on its
	// own. Short enough that the mesh feels attached to the cursor, long enough
	// that the constraints of the XML still get to resist it - and that the
	// spring stays well inside what a 1/60 explicit step integrates stably,
	// including the stiffening a lever arm longer than the bone adds.
	constexpr float grabResponseTime = 0.08f;

	// The spring only ever pulls as if the cursor were this far away (a quarter
	// meter), so throwing the cursor across the screen cannot feed the
	// simulation unbounded energy - it just pulls at full strength for longer.
	constexpr float grabMaxOffset = 0.25f * scaleSkyrim;

	// NIF axes: +X is the mesh's own right, +Y forward, +Z up. Left and right
	// are swapped against that because the viewport looks at the mesh from the
	// front, where its right side is on the viewer's left - the labels name
	// what the user sees, not the mesh's anatomy.
	btVector3 WindDirectionVector(WindDirection direction) {
		switch (direction) {
			case WindDirection::Up: return btVector3(0, 0, 1);
			case WindDirection::Down: return btVector3(0, 0, -1);
			case WindDirection::Forward: return btVector3(0, 1, 0);
			case WindDirection::Backward: return btVector3(0, -1, 0);
			case WindDirection::Left: return btVector3(1, 0, 0);
			case WindDirection::Right: return btVector3(-1, 0, 0);
		}

		return btVector3(-1, 0, 0);
	}

	// Thin subclass so the controller can drive the protected per-frame hooks
	// in the same order hdtSMP64's SkyrimPhysicsWorld did, and set the same
	// default gravity (hdtSkyrimPhysicsWorld.cpp: (0, 0, -9.8 * scaleSkyrim)).
	class PreviewWorld : public hdt::SkinnedMeshWorld {
	public:
		PreviewWorld() { setGravity(btVector3(0, 0, -9.8f * scaleSkyrim)); }

		using hdt::SkinnedMeshWorld::readTransform;
		using hdt::SkinnedMeshWorld::writeTransform;

		// Ports of SkyrimPhysicsWorld::applyTranslationOffset /
		// restoreTranslationOffset: recenter all rigid bodies around their
		// average position during the step to keep float precision stable.
		btVector3 applyTranslationOffset() {
			btVector3 center;
			center.setZero();
			int count = 0;
			for (int i = 0; i < m_collisionObjects.size(); ++i) {
				auto rig = btRigidBody::upcast(m_collisionObjects[i]);
				if (rig) {
					center += rig->getWorldTransform().getOrigin();
					++count;
				}
			}

			if (count > 0) {
				center /= static_cast<btScalar>(count);
				for (int i = 0; i < m_collisionObjects.size(); ++i) {
					auto rig = btRigidBody::upcast(m_collisionObjects[i]);
					if (rig)
						rig->getWorldTransform().getOrigin() -= center;
				}
			}
			return center;
		}

		void restoreTranslationOffset(const btVector3& offset) {
			for (int i = 0; i < m_collisionObjects.size(); ++i) {
				auto rig = btRigidBody::upcast(m_collisionObjects[i]);
				if (rig)
					rig->getWorldTransform().getOrigin() += offset;
			}
		}
	};

	// One system per distinct XML path, applied to the union of the shapes it
	// was found on.
	struct XmlEntry {
		std::string path; // first-seen spelling
		std::vector<nifly::NiShape*> shapes;
	};

	// Collects the physics XML paths attached to a block via NiStringExtraData
	// named "HDT Skinned Mesh Physics Object".
	std::vector<std::string> GetPhysicsXmlPaths(nifly::NifFile* nif, nifly::NiObjectNET* obj) {
		std::vector<std::string> result;

		for (auto& extraDataRef : obj->extraDataRefs) {
			auto stringExtraData = nif->GetHeader().GetBlock<nifly::NiStringExtraData>(extraDataRef);
			if (!stringExtraData)
				continue;

			if (hdt::IDStr(stringExtraData->name.get()) != physicsExtraDataName)
				continue;

			const std::string& xmlPath = stringExtraData->stringData.get();
			if (!xmlPath.empty())
				result.push_back(xmlPath);
		}

		return result;
	}

	// Extra data on a shape applies to that shape; extra data on a NiNode
	// applies to all shapes under that node. "shapePhysicsFiles" adds the links
	// captured when each NIF was loaded: Outfit Studio merges the shapes of
	// every file after the first into one work NIF, dropping the root nodes the
	// links live on, so those shapes are only covered by that map.
	std::vector<XmlEntry> CollectPhysicsXmlEntries(nifly::NifFile* nif, const ShapePhysicsFileMap& shapePhysicsFiles) {
		std::vector<XmlEntry> entries;
		if (!nif)
			return entries;

		auto addEntry = [&entries](const std::string& path, nifly::NiShape* shape) {
			const hdt::IDStr pathId(path);
			for (auto& e : entries) {
				if (hdt::IDStr(e.path) == pathId) {
					if (std::find(e.shapes.begin(), e.shapes.end(), shape) == e.shapes.end())
						e.shapes.push_back(shape);
					return;
				}
			}
			entries.push_back({path, {shape}});
		};

		const auto shapes = nif->GetShapes();

		for (auto* shape : shapes) {
			for (const auto& xmlPath : GetPhysicsXmlPaths(nif, shape))
				addEntry(xmlPath, shape);
		}

		for (auto* node : nif->GetNodes()) {
			const auto xmlPaths = GetPhysicsXmlPaths(nif, node);
			if (xmlPaths.empty())
				continue;

			for (auto* shape : shapes) {
				bool underNode = false;
				for (nifly::NiNode* p = nif->GetParentNode(shape); p; p = nif->GetParentNode(p)) {
					if (p == node) {
						underNode = true;
						break;
					}
				}

				if (underNode) {
					for (const auto& xmlPath : xmlPaths)
						addEntry(xmlPath, shape);
				}
			}
		}

		for (auto& shapePhysicsFile : shapePhysicsFiles) {
			auto* shape = nif->FindBlockByName<nifly::NiShape>(shapePhysicsFile.first);
			if (!shape)
				continue;

			for (const auto& xmlPath : shapePhysicsFile.second)
				addEntry(xmlPath, shape);
		}

		return entries;
	}
}

std::vector<SystemInfo> CollectPhysicsXmlLinks(nifly::NifFile* nif, const ShapePhysicsFileMap& shapePhysicsFiles) {
	std::vector<SystemInfo> result;

	for (auto& entry : CollectPhysicsXmlEntries(nif, shapePhysicsFiles)) {
		SystemInfo info;
		info.xmlPath = entry.path;
		for (auto* shape : entry.shapes)
			info.shapeNames.push_back(shape->name.get());

		result.push_back(std::move(info));
	}

	return result;
}

bool HasPhysicsLinks(nifly::NifFile* nif, const ShapePhysicsFileMap& shapePhysicsFiles) {
	if (!nif)
		return false;

	for (auto& shapePhysicsFile : shapePhysicsFiles) {
		if (!shapePhysicsFile.second.empty() && nif->FindBlockByName<nifly::NiShape>(shapePhysicsFile.first))
			return true;
	}

	// Answering "is there anything at all" does not need the shape sets
	// CollectPhysicsXmlEntries builds, which is what makes this cheap enough to
	// call whenever the loaded meshes change. A link on a node with no shapes
	// under it counts here but builds nothing - it does not exist in practice,
	// since the game only reads the link from the model root.
	const auto shapes = nif->GetShapes();
	if (shapes.empty())
		return false;

	for (auto* shape : shapes) {
		if (!GetPhysicsXmlPaths(nif, shape).empty())
			return true;
	}

	for (auto* node : nif->GetNodes()) {
		if (!GetPhysicsXmlPaths(nif, node).empty())
			return true;
	}

	return false;
}

struct Controller::Impl {
	// One bone held by the cursor for as long as a grab lasts.
	struct GrabbedBone {
		PreviewSystem* system = nullptr;
		btRigidBody* body = nullptr;
		// Where the grab took hold, in NIF global space and in the body's own
		// frame. The body carries the local one along as it moves, which is
		// what makes the grab stick to the spot it started on.
		btVector3 anchor = btVector3(0, 0, 0);
		btVector3 localAnchor = btVector3(0, 0, 0);
		float weight = 1.0f;
	};

	PreviewWorld world;
	std::vector<hdt::Ref<PreviewSystem>> systems;
	// What each entry of "systems" was built from, same order and size
	std::vector<SystemInfo> systemInfos;
	PoseOverrideMap poseOverrides;
	std::unordered_set<std::string> affectedShapes;
	DebugVis debugVis;
	float rootYawRad = 0.0f;
	// Wall-clock time that has not been simulated yet, kept below one tick
	float leftoverTime = 0.0f;
	float windStrength = 0.0f;
	WindDirection windDirection = WindDirection::Right;
	std::vector<GrabbedBone> grabbed;
	// How far the cursor has dragged the grab, in NIF global space
	btVector3 grabOffset = btVector3(0, 0, 0);

	// The cursor-driven collision sphere, deliberately kept out of "systems" so
	// that IsActive and AffectedShapes keep meaning "there is cloth to simulate"
	hdt::Ref<PreviewSystem> probe;
	float probeRadius = 0.0f;

	// Pulls every grabbed bone towards where the cursor wants its anchor, as a
	// force rather than a teleport: the solver still has to get the bone there
	// past its constraints, its collisions and gravity, so a stiff setup barely
	// gives and a loose one follows the cursor almost exactly.
	//
	// Call once per tick, before the world recenters itself in
	// applyTranslationOffset - the spring works in absolute positions.
	void applyGrabForces() {
		// Critically damped, with both gains per unit mass so that the pull
		// feels the same whatever masses the XML handed its bones.
		const btScalar omega = 1.0f / grabResponseTime;

		for (auto& grabbedBone : grabbed) {
			const btScalar inverseMass = grabbedBone.body->getInvMass();
			if (!(inverseMass > 0.0f))
				continue;

			const btTransform& bodyTransform = grabbedBone.body->getWorldTransform();
			const btVector3 anchorNow = bodyTransform * grabbedBone.localAnchor;
			const btVector3 anchorWanted = grabbedBone.system->m_rootMotion * (grabbedBone.anchor + grabOffset);

			btVector3 offset = anchorWanted - anchorNow;
			const btScalar distance = offset.length();
			if (distance > grabMaxOffset)
				offset *= grabMaxOffset / distance;

			// Both the lever arm the force acts on and the velocity to damp are
			// those of the anchor, not of the body's center: pulling a bone by
			// its edge has to turn it, or a grabbed skirt slides instead of
			// folding.
			const btVector3 leverArm = anchorNow - grabbedBone.body->getCenterOfMassPosition();
			const btVector3 anchorVelocity = grabbedBone.body->getVelocityInLocalPoint(leverArm);

			// Bones never sleep (addSkinnedMeshSystem disables deactivation for
			// all of them), so the force always lands.
			const btScalar strength = grabbedBone.weight / inverseMass;
			grabbedBone.body->applyForce(strength * (omega * omega * offset - 2.0f * omega * anchorVelocity), leverArm);
		}
	}

	// The root motion turns the mesh inside the physics world, so a wind
	// vector fixed in world space would blow at a different side of the mesh
	// after every camera turn. Turning it along with the mesh keeps it where
	// the user aimed it. Call after anything that changes yaw, strength or
	// direction.
	void applyWind() {
		const btVector3 direction = btMatrix3x3(btQuaternion(btVector3(0, 0, 1), rootYawRad)) * WindDirectionVector(windDirection);

		world.m_enableWind = windStrength > 0.0f;
		world.getWind() = direction * (windStrength * maxWindScale * scaleSkyrim);
	}
};

Controller::Controller()
	: impl(std::make_unique<Impl>()) {}

Controller::~Controller() {
	Clear();
}

size_t Controller::BuildFromNif(nifly::NifFile* nif,
									   AnimInfo* anim,
									   const XmlStreamResolver& resolver,
									   const ShapePhysicsFileMap& shapePhysicsFiles,
									   std::vector<std::string>& outWarnings) {
	Clear();

	if (!nif || !anim || !resolver)
		return 0;

	for (auto& entry : CollectPhysicsXmlEntries(nif, shapePhysicsFiles)) {
		auto stream = resolver(entry.path);
		if (!stream) {
			outWarnings.push_back("physics XML not found: " + entry.path);
			continue;
		}

		std::ostringstream contents;
		contents << stream->rdbuf();
		const std::string xmlData = contents.str();

		BuildInput input;
		input.xmlData = &xmlData;
		input.xmlName = entry.path;
		input.nif = nif;
		input.anim = anim;
		input.shapes = entry.shapes;

		SystemBuilder builder;
		auto system = builder.Build(input, outWarnings);
		if (!system)
			continue;

		system->m_poseOverrides = &impl->poseOverrides;
		impl->world.addSkinnedMeshSystem(system.get());
		impl->systems.push_back(system);

		SystemInfo info;
		info.xmlPath = entry.path;
		for (auto* shape : entry.shapes)
			info.shapeNames.push_back(shape->name.get());

		impl->systemInfos.push_back(std::move(info));

		// A shape needs re-skinning against pose overrides when its skin
		// references at least one dynamic (non-kinematic) bone of the system.
		for (auto* shape : entry.shapes) {
			const std::string shapeName = shape->name.get();
			auto skinIt = anim->shapeSkinning.find(shapeName);
			if (skinIt == anim->shapeSkinning.end())
				continue;

			bool affected = false;
			for (const auto& bn : skinIt->second.boneNames) {
				const hdt::IDStr boneName(bn.first);
				for (const auto& bone : system->getBones()) {
					if (bone->m_name == boneName && !bone->m_rig.isStaticOrKinematicObject()) {
						affected = true;
						break;
					}
				}
				if (affected)
					break;
			}

			if (affected)
				impl->affectedShapes.insert(shapeName);
		}
	}

	return impl->systems.size();
}

void Controller::Clear() {
	if (!impl)
		return;

	// The grab points straight at the rigid bodies about to go away
	EndGrab();
	ClearProbe();

	for (auto& system : impl->systems)
		impl->world.removeSkinnedMeshSystem(system.get());

	impl->systems.clear();
	impl->systemInfos.clear();
	impl->poseOverrides.clear();
	impl->affectedShapes.clear();
	impl->rootYawRad = 0.0f;
	impl->leftoverTime = 0.0f;
}

const std::vector<SystemInfo>& Controller::Systems() const {
	return impl->systemInfos;
}

bool Controller::IsActive() const {
	return impl && !impl->systems.empty();
}

void Controller::ResetDynamics() {
	if (!IsActive())
		return;

	// Everything is about to be teleported back onto the pose, which would
	// leave the grab holding on to a spot that no longer exists
	EndGrab();

	// Mirrors hdtSMP64 SkyrimPhysicsWorld::resetSystems (and the reset call in
	// SkinnedMeshWorld::addSkinnedMeshSystem).
	for (auto& system : impl->systems)
		system->readTransform(system->prepareForRead(hdt::RESET_PHYSICS));

	impl->leftoverTime = 0.0f;
}

namespace {
	// Reading a ValueVariant the way the property it belongs to is spelled.
	// An int where a float is wanted is accepted: the schema calls some of
	// these flexFloat and a file may hold either.
	bool PatchFloat(const ValueVariant& value, float& out) {
		if (const float* number = std::get_if<float>(&value)) {
			out = *number;
			return true;
		}
		if (const int* number = std::get_if<int>(&value)) {
			out = static_cast<float>(*number);
			return true;
		}
		return false;
	}

	bool PatchInt(const ValueVariant& value, int& out) {
		if (const int* number = std::get_if<int>(&value)) {
			out = *number;
			return true;
		}
		return false;
	}

	bool PatchBool(const ValueVariant& value, bool& out) {
		if (const bool* flag = std::get_if<bool>(&value)) {
			out = *flag;
			return true;
		}
		return false;
	}

	bool PatchVector(const ValueVariant& value, btVector3& out) {
		if (const nifly::Vector3* vector = std::get_if<nifly::Vector3>(&value)) {
			out = ToBt(*vector);
			return true;
		}
		return false;
	}

	// A repeated text element comes back as a list; one written once comes
	// back as the single string, and means a list of one.
	bool PatchStrings(const ValueVariant& value, std::vector<std::string>& out) {
		if (const auto* list = std::get_if<std::vector<std::string>>(&value)) {
			out = *list;
			return true;
		}
		if (const auto* text = std::get_if<std::string>(&value)) {
			out.assign(1, *text);
			return true;
		}
		// An absent list is an empty one, which is a meaningful value here
		if (std::holds_alternative<std::monostate>(value)) {
			out.clear();
			return true;
		}
		return false;
	}

	bool PatchBone(PreviewSystem* system, const PatchRequest& request) {
		auto* bone = system->findBone(hdt::IDStr(request.elementName));
		if (!bone)
			return false;

		btRigidBody& rig = bone->m_rig;
		const std::string& property = request.property;
		float number = 0.0f;

		if (property == "mass") {
			if (!PatchFloat(request.value, number) || number < 0.0f)
				return false;

			// Crossing 0 turns the body between kinematic and dynamic, which
			// changes its collision flags, whether it publishes a pose
			// override at all and which shapes count as affected by physics.
			// None of that can be patched; it is a rebuild.
			if ((rig.getInvMass() > 0.0f) != (number > 0.0f))
				return false;

			rig.setMassProps(number, rig.getLocalInertia());
			rig.updateInertiaTensor();
			return true;
		}

		if (property == "inertia") {
			btVector3 inertia(0, 0, 0);
			if (!PatchVector(request.value, inertia))
				return false;

			const btScalar mass = rig.getInvMass() > 0.0f ? 1.0f / rig.getInvMass() : 0.0f;
			rig.setMassProps(mass, inertia);
			rig.updateInertiaTensor();
			return true;
		}

		if (property == "linearDamping" || property == "angularDamping") {
			if (!PatchFloat(request.value, number))
				return false;

			const bool linear = property == "linearDamping";
			rig.setDamping(linear ? number : rig.getLinearDamping(), linear ? rig.getAngularDamping() : number);
			return true;
		}

		if (!PatchFloat(request.value, number))
			return false;

		// The same clamps readBoneTemplate applies, so a patched value behaves
		// exactly like the same value after a rebuild.
		if (property == "friction")
			rig.setFriction(number);
		else if (property == "rollingFriction")
			rig.setRollingFriction(number);
		else if (property == "restitution")
			rig.setRestitution(number);
		else if (property == "margin-multiplier")
			bone->m_marginMultipler = number;
		else if (property == "gravity-factor")
			bone->m_gravityFactor = btClamped(number, 0.0f, 1.0f);
		else if (property == "wind-factor")
			bone->m_windFactor = std::max(number, 0.0f);
		else
			return false;

		return true;
	}

	bool PatchBody(PreviewSystem* system, const PatchRequest& request) {
		auto* body = static_cast<PreviewBody*>(system->findBody(hdt::IDStr(request.elementName)));
		if (!body)
			return false;

		const std::string& property = request.property;

		if (property == "margin" || property == "penetration") {
			float number = 0.0f;
			if (!PatchFloat(request.value, number))
				return false;

			auto* perTriangle = body->m_shape ? body->m_shape->asPerTriangleShape() : nullptr;
			auto* perVertex = body->m_shape ? body->m_shape->asPerVertexShape() : nullptr;

			if (property == "penetration") {
				if (!perTriangle)
					return false;

				perTriangle->m_shapeProp.penetration = number;
				return true;
			}

			if (perTriangle)
				perTriangle->m_shapeProp.margin = number;

			// PerTriangleShape::finishBuild copies the margin into the
			// per-vertex colliders it generates alongside itself, so writing
			// only the outer shape would leave those at the old size.
			if (perVertex)
				perVertex->m_shapeProp.margin = number;

			return perTriangle || perVertex;
		}

		if (property == "tag" || property == "can-collide-with-tag" || property == "no-collide-with-tag") {
			std::vector<std::string> names;
			if (!PatchStrings(request.value, names))
				return false;

			if (property == "tag") {
				body->m_tags.clear();
				for (const std::string& name : names)
					body->m_tags.push_back(hdt::IDStr(name));

				return true;
			}

			auto& tags = property == "can-collide-with-tag" ? body->m_canCollideWithTags : body->m_noCollideWithTags;
			tags.clear();
			for (const std::string& name : names)
				tags.insert(hdt::IDStr(name));

			return true;
		}

		if (property == "can-collide-with-bone" || property == "no-collide-with-bone") {
			std::vector<std::string> names;
			if (!PatchStrings(request.value, names))
				return false;

			// The builder creates a bone it cannot find; a patch can only bind
			// to one that already exists, so an unknown name is a rebuild.
			std::vector<hdt::SkinnedMeshBone*> bones;
			for (const std::string& name : names) {
				auto* bone = system->findBone(hdt::IDStr(name));
				if (!bone)
					return false;

				bones.push_back(bone);
			}

			auto& target = property == "can-collide-with-bone" ? body->m_canCollideWithBones : body->m_noCollideWithBones;
			target.clear();
			for (auto* bone : bones)
				target.insert(bone);

			return true;
		}

		if (property == "shared") {
			const std::string* text = std::get_if<std::string>(&request.value);
			if (!text)
				return false;

			const hdt::IDStr shared(*text);
			if (shared == hdt::IDStr("public"))
				body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;
			else if (shared == hdt::IDStr("internal"))
				body->m_shared = PreviewBody::SharedType::SHARED_INTERNAL;
			else if (shared == hdt::IDStr("external"))
				body->m_shared = PreviewBody::SharedType::SHARED_EXTERNAL;
			else if (shared == hdt::IDStr("private"))
				body->m_shared = PreviewBody::SharedType::SHARED_PRIVATE;
			else
				body->m_shared = PreviewBody::SharedType::SHARED_PUBLIC;

			return true;
		}

		if (property == "disable-tag") {
			const std::string* text = std::get_if<std::string>(&request.value);
			if (!text)
				return false;

			body->m_disableTag = hdt::IDStr(*text);
			return true;
		}

		if (property == "disable-priority") {
			int priority = 0;
			if (!PatchInt(request.value, priority))
				return false;

			body->m_disablePriority = priority;
			return true;
		}

		return false;
	}

	// The constraint joining bodyA to bodyB, or null when there is no such
	// constraint or more than one: the XSD puts no key on the pair, so it is a
	// label rather than an address, and an ambiguous one has to rebuild.
	hdt::BoneScaleConstraint* FindConstraint(PreviewSystem* system, const PatchRequest& request) {
		const hdt::IDStr bodyA(request.bodyA);
		const hdt::IDStr bodyB(request.bodyB);

		hdt::BoneScaleConstraint* found = nullptr;
		bool ambiguous = false;

		auto consider = [&](hdt::BoneScaleConstraint* constraint) {
			if (!constraint || !constraint->m_boneA || !constraint->m_boneB)
				return;
			if (constraint->m_boneA->m_name != bodyA || constraint->m_boneB->m_name != bodyB)
				return;

			if (found)
				ambiguous = true;
			else
				found = constraint;
		};

		for (const auto& constraint : system->constraints())
			consider(constraint.get());

		// A constraint inside a constraint-group is held by the group, not by
		// the system's own list
		for (const auto& group : system->constraintGroups()) {
			for (const auto& constraint : group->m_constraints)
				consider(constraint.get());
		}

		return ambiguous ? nullptr : found;
	}

	bool PatchGenericConstraint(hdt::Generic6DofConstraint* constraint, const PatchRequest& request) {
		const std::string& property = request.property;

		// Every property of this constraint is either a vector applied per
		// axis or a flag applied to all six, with the linear axes at 0..2 and
		// the angular ones at 3..5 - the same layout readGenericConstraint uses.
		const bool angular = property.rfind("angular", 0) == 0;
		const int base = angular ? 3 : 0;

		auto* linearMotor = constraint->getTranslationalLimitMotor();

		btVector3 vector(0, 0, 0);
		bool flag = false;
		float number = 0.0f;

		if (property == "linearLowerLimit" || property == "angularLowerLimit" || property == "linearUpperLimit" || property == "angularUpperLimit") {
			if (!PatchVector(request.value, vector))
				return false;

			const bool lower = property.find("Lower") != std::string::npos;
			if (angular) {
				if (lower)
					constraint->setAngularLowerLimit(vector);
				else
					constraint->setAngularUpperLimit(vector);
			}
			else {
				if (lower)
					constraint->setLinearLowerLimit(vector);
				else
					constraint->setLinearUpperLimit(vector);
			}

			return true;
		}

		if (property == "linearStiffness" || property == "angularStiffness") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				const bool limited = angular ? constraint->getRotationalLimitMotor(axis)->m_springStiffnessLimited
											 : linearMotor->m_springStiffnessLimited[axis];
				constraint->setStiffness(base + axis, vector[axis], limited);
			}

			return true;
		}

		if (property == "linearDamping" || property == "angularDamping") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				const bool limited = angular ? constraint->getRotationalLimitMotor(axis)->m_springDampingLimited
											 : linearMotor->m_springDampingLimited[axis];
				constraint->setDamping(base + axis, vector[axis], limited);
			}

			return true;
		}

		if (property == "linearEquilibrium" || property == "angularEquilibrium") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis) {
				constraint->setEquilibriumPoint(base + axis, vector[axis]);
				// readGenericConstraint aims the servos at the equilibrium too
				constraint->setServoTarget(base + axis, vector[axis]);
			}

			return true;
		}

		if (property == "linearTargetVelocity" || property == "angularTargetVelocity") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setTargetVelocity(base + axis, vector[axis]);

			return true;
		}

		if (property == "linearMaxMotorForce" || property == "angularMaxMotorForce") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setMaxMotorForce(base + axis, vector[axis]);

			return true;
		}

		if (property == "linearBounce") {
			if (!PatchVector(request.value, vector))
				return false;

			linearMotor->m_bounce = vector;
			return true;
		}

		if (property == "angularBounce") {
			if (!PatchVector(request.value, vector))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->getRotationalLimitMotor(axis)->m_bounce = vector[axis];

			return true;
		}

		if (property == "enableLinearSprings" || property == "enableAngularSprings") {
			if (!PatchBool(request.value, flag))
				return false;

			const int springBase = property == "enableAngularSprings" ? 3 : 0;
			for (int axis = 0; axis < 3; ++axis)
				constraint->enableSpring(springBase + axis, flag);

			return true;
		}

		if (property == "linearMotors" || property == "angularMotors") {
			if (!PatchBool(request.value, flag))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->enableMotor(base + axis, flag);

			return true;
		}

		if (property == "linearServoMotors" || property == "angularServoMotors") {
			if (!PatchBool(request.value, flag))
				return false;

			for (int axis = 0; axis < 3; ++axis)
				constraint->setServo(base + axis, flag);

			return true;
		}

		if (property == "linearStiffnessLimited" || property == "angularStiffnessLimited") {
			if (!PatchBool(request.value, flag))
				return false;

			// Only the flag changes; the stiffness it qualifies stays put
			for (int axis = 0; axis < 3; ++axis) {
				const btScalar stiffness = angular ? constraint->getRotationalLimitMotor(axis)->m_springStiffness
												   : linearMotor->m_springStiffness[axis];
				constraint->setStiffness(base + axis, stiffness, flag);
			}

			return true;
		}

		if (property == "springDampingLimited") {
			if (!PatchBool(request.value, flag))
				return false;

			// This one flag qualifies both halves
			for (int axis = 0; axis < 3; ++axis) {
				constraint->setDamping(axis, linearMotor->m_springDamping[axis], flag);
				constraint->setDamping(axis + 3, constraint->getRotationalLimitMotor(axis)->m_springDamping, flag);
			}

			return true;
		}

		if (property == "motorERP" || property == "motorCFM" || property == "stopERP" || property == "stopCFM") {
			if (!PatchFloat(request.value, number))
				return false;

			// readGenericConstraint sets the linear axes through setParam and
			// writes the angular motors directly; mirror both.
			const int param = property == "motorERP"	? BT_CONSTRAINT_ERP
							  : property == "motorCFM"	? BT_CONSTRAINT_CFM
							  : property == "stopERP"	? BT_CONSTRAINT_STOP_ERP
														: BT_CONSTRAINT_STOP_CFM;

			for (int axis = 0; axis < 3; ++axis) {
				constraint->setParam(param, number, axis);

				auto* rotationMotor = constraint->getRotationalLimitMotor(axis);
				if (!rotationMotor)
					continue;

				if (property == "motorERP")
					rotationMotor->m_motorERP = number;
				else if (property == "motorCFM")
					rotationMotor->m_motorCFM = number;
				else if (property == "stopERP")
					rotationMotor->m_stopERP = number;
				else
					rotationMotor->m_stopCFM = number;
			}

			return true;
		}

		return false;
	}

	bool PatchStiffSpringConstraint(hdt::StiffSpringConstraint* constraint, const PatchRequest& request) {
		float number = 0.0f;
		if (!PatchFloat(request.value, number))
			return false;

		// The distances are the ones the two bones happened to be apart when
		// the constraint was built, times the factors from the XML; the
		// factors themselves are not kept, so re-applying one would compound.
		if (request.property == "stiffness")
			constraint->m_stiffness = number;
		else if (request.property == "damping")
			constraint->m_damping = number;
		else
			return false;

		return true;
	}

	bool PatchConeTwistConstraint(hdt::ConeTwistConstraint* constraint, const PatchRequest& request) {
		float number = 0.0f;
		if (!PatchFloat(request.value, number))
			return false;

		// btConeTwistConstraint takes all six together and recomputes from
		// them, so the five that did not change are read back out first.
		float swingSpan1 = constraint->getSwingSpan1();
		float swingSpan2 = constraint->getSwingSpan2();
		float twistSpan = constraint->getTwistSpan();
		float softness = constraint->getLimitSoftness();
		float bias = constraint->getBiasFactor();
		float relaxation = constraint->getRelaxationFactor();

		const std::string& property = request.property;
		if (property == "swingSpan1")
			swingSpan1 = number;
		else if (property == "swingSpan2")
			swingSpan2 = number;
		else if (property == "twistSpan")
			twistSpan = number;
		else if (property == "limitSoftness")
			softness = number;
		else if (property == "biasFactor")
			bias = number;
		else if (property == "relaxationFactor")
			relaxation = number;
		else
			return false;

		constraint->setLimit(swingSpan1, swingSpan2, twistSpan, softness, bias, relaxation);
		return true;
	}
}

bool Controller::PatchProperty(const PatchRequest& request) {
	if (!IsActive())
		return false;

	// Clearing a property hands it back to the ambient *-default cascade, and
	// what that resolves to is not something this end knows; rebuild instead.
	if (std::holds_alternative<std::monostate>(request.value))
		return false;

	PreviewSystem* system = nullptr;
	for (size_t i = 0; i < impl->systemInfos.size(); ++i) {
		if (impl->systemInfos[i].xmlPath == request.xmlPath) {
			system = impl->systems[i].get();
			break;
		}
	}

	if (!system)
		return false;

	switch (request.kind) {
		case ElementKind::Bone: return PatchBone(system, request);

		case ElementKind::PerVertexShape:
		case ElementKind::PerTriangleShape: return PatchBody(system, request);

		case ElementKind::GenericConstraint:
		case ElementKind::StiffSpringConstraint:
		case ElementKind::ConeTwistConstraint: break;

		// A *-default declares a template rather than an object, a <shape> is
		// baked into the bone that carries it, and a constraint-group is only
		// a bracket. None of them has anything live to write to.
		default: return false;
	}

	hdt::BoneScaleConstraint* constraint = FindConstraint(system, request);
	if (!constraint)
		return false;

	if (auto* generic = dynamic_cast<hdt::Generic6DofConstraint*>(constraint))
		return PatchGenericConstraint(generic, request);
	if (auto* stiffSpring = dynamic_cast<hdt::StiffSpringConstraint*>(constraint))
		return PatchStiffSpringConstraint(stiffSpring, request);
	if (auto* coneTwist = dynamic_cast<hdt::ConeTwistConstraint*>(constraint))
		return PatchConeTwistConstraint(coneTwist, request);

	return false;
}

DynamicState Controller::CaptureDynamicState() const {
	DynamicState state;
	if (!IsActive())
		return state;

	constexpr float radToDeg = 180.0f / 3.14159265358979323846f;
	state.rootYawDegrees = impl->rootYawRad * radToDeg;

	for (size_t i = 0; i < impl->systems.size(); ++i) {
		const std::string& xmlPath = impl->systemInfos[i].xmlPath;

		for (auto& bone : impl->systems[i]->getBones()) {
			// The pose puts kinematic bones where they belong on its own, and
			// their velocity is derived from it again every frame
			if (bone->m_rig.isStaticOrKinematicObject())
				continue;

			DynamicState::BoneMotion motion;
			motion.xmlPath = xmlPath;
			motion.boneName = bone->m_name.str();
			// Recorded as the bone's own frame rather than the rigid body's,
			// so an edited center of mass does not displace it on the way back
			motion.transform = FromBt(bone->m_rig.getWorldTransform() * bone->m_rigToLocal);
			motion.linearVelocity = FromBt(bone->m_rig.getLinearVelocity());
			motion.angularVelocity = FromBt(bone->m_rig.getAngularVelocity());
			state.bones.push_back(std::move(motion));
		}
	}

	return state;
}

void Controller::RestoreDynamicState(const DynamicState& state) {
	if (!IsActive())
		return;

	// The bodies the grab held on to belonged to the systems this replaces
	EndGrab();

	// The captured transforms are in the world the root motion had turned the
	// mesh into, so the yaw has to come back with them. Clear() reset it.
	constexpr float degToRad = 3.14159265358979323846f / 180.0f;
	impl->rootYawRad = state.rootYawDegrees * degToRad;
	impl->applyWind();

	for (const auto& motion : state.bones) {
		// One system per distinct XML path, spelled the same way on both sides
		// of the rebuild because both read the link out of the same NIF
		PreviewSystem* system = nullptr;
		for (size_t i = 0; i < impl->systemInfos.size(); ++i) {
			if (impl->systemInfos[i].xmlPath == motion.xmlPath) {
				system = impl->systems[i].get();
				break;
			}
		}

		if (!system)
			continue;

		auto* bone = system->findBone(hdt::IDStr(motion.boneName));

		// A bone the edit turned kinematic is driven by the pose from now on
		if (!bone || bone->m_rig.isStaticOrKinematicObject())
			continue;

		const btTransform rigTransform = ToBt(motion.transform) * bone->m_localToRig;
		const btVector3 linearVelocity = ToBt(motion.linearVelocity);
		const btVector3 angularVelocity = ToBt(motion.angularVelocity);

		bone->m_rig.setWorldTransform(rigTransform);
		bone->m_rig.setInterpolationWorldTransform(rigTransform);
		bone->m_rig.setLinearVelocity(linearVelocity);
		bone->m_rig.setAngularVelocity(angularVelocity);
		bone->m_rig.setInterpolationLinearVelocity(linearVelocity);
		bone->m_rig.setInterpolationAngularVelocity(angularVelocity);
		bone->m_rig.updateInertiaTensor();
		bone->m_rig.activate();
	}

	// Whatever was left of the tick the rebuild interrupted is not owed to a
	// simulation that no longer exists
	impl->leftoverTime = 0.0f;
}

void Controller::Step(float dtSeconds) {
	if (!IsActive())
		return;

	// Like hdtSMP64's SkyrimPhysicsWorld::doUpdate, the simulation only ever
	// advances in whole ticks of a fixed length, so its behavior does not
	// depend on the frame rate. Time that does not fill a tick is carried over
	// to the next call; time beyond maxTicksPerStep is dropped so a stall
	// (breakpoint, window drag) cannot turn into a burst of catch-up steps.
	constexpr float fixedTimeStep = 1.0f / 60.0f;
	constexpr int maxTicksPerStep = 3;

	if (dtSeconds > 0.0f)
		impl->leftoverTime += dtSeconds;

	int ticks = static_cast<int>(impl->leftoverTime / fixedTimeStep);
	if (ticks <= 0)
		return;

	impl->leftoverTime -= ticks * fixedTimeStep;
	ticks = std::min(ticks, maxTicksPerStep);

	// Root motion from the accumulated camera yaw: rotate the character about
	// the world Z axis under a fixed camera.
	const btTransform rootMotion(btQuaternion(btVector3(0, 0, 1), impl->rootYawRad));
	for (auto& system : impl->systems)
		system->m_rootMotion = rootMotion;

	// The probe is placed in the same space the mesh is modelled in, so it has
	// to turn with the mesh or it would slide off as the camera goes around.
	if (impl->probe)
		impl->probe->m_rootMotion = rootMotion;

	// Keep the wind aimed at the same side of the mesh as it turns
	impl->applyWind();

	// Same call order as hdtSMP64's SkyrimPhysicsWorld::doUpdate /
	// doUpdate2ndStep: readTransform (which runs prepareForRead per system) ->
	// translation offset -> stepSimulation -> restore offset ->
	// writeTransform. SkinnedMeshWorld::stepSimulation does not call
	// readTransform/writeTransform itself.
	for (int i = 0; i < ticks; ++i) {
		impl->world.readTransform(fixedTimeStep);
		// After readTransform, which is what moves the kinematic bones the grab
		// is pulling against, and before the world recenters itself
		impl->applyGrabForces();
		const btVector3 offset = impl->world.applyTranslationOffset();
		impl->world.stepSimulation(fixedTimeStep, 1, fixedTimeStep);
		impl->world.restoreTranslationOffset(offset);
		impl->world.writeTransform();
	}
}

void Controller::InjectCameraYaw(float deltaDegrees) {
	if (!IsActive())
		return;

	// The camera orbits the mesh, so the mesh turns with the camera rather
	// than against it: same sign, or the cloth swings the wrong way.
	constexpr float degToRad = 3.14159265358979323846f / 180.0f;
	impl->rootYawRad += deltaDegrees * degToRad;

	// Only ever used as a rotation, so wrapping is exact and prevents float
	// precision loss from unbounded accumulation.
	constexpr float twoPi = 2.0f * 3.14159265358979323846f;
	impl->rootYawRad = std::fmod(impl->rootYawRad, twoPi);
}

void Controller::SetWindStrength(float strength) {
	impl->windStrength = std::max(0.0f, std::min(strength, 1.0f));
	impl->applyWind();
}

void Controller::SetWindDirection(WindDirection direction) {
	impl->windDirection = direction;
	impl->applyWind();
}

bool Controller::BeginGrab(const std::vector<GrabTarget>& targets) {
	EndGrab();

	if (!IsActive())
		return false;

	for (const auto& target : targets) {
		if (!(target.weight > 0.0f))
			continue;

		const hdt::IDStr boneName(target.boneName);
		for (auto& system : impl->systems) {
			auto* bone = system->findBone(boneName);
			if (!bone)
				continue;

			// A kinematic bone is an input to the simulation, not part of it:
			// it goes where the pose says and there is nothing to pull it off.
			if (bone->m_rig.isStaticOrKinematicObject())
				break;

			Impl::GrabbedBone grabbedBone;
			grabbedBone.system = system.get();
			grabbedBone.body = &bone->m_rig;
			grabbedBone.anchor = ToBt(target.anchor);
			// The bodies stand in root motion space and carry the anchor along
			// from here on, so bind it in the frame they are in right now.
			grabbedBone.localAnchor = bone->m_rig.getWorldTransform().inverse() * (system->m_rootMotion * grabbedBone.anchor);
			grabbedBone.weight = std::max(0.0f, std::min(target.weight, 1.0f));
			impl->grabbed.push_back(grabbedBone);
			break;
		}
	}

	return !impl->grabbed.empty();
}

void Controller::UpdateGrab(const nifly::Vector3& offset) {
	if (!impl)
		return;

	impl->grabOffset = ToBt(offset);
}

void Controller::EndGrab() {
	if (!impl)
		return;

	impl->grabbed.clear();
	impl->grabOffset.setZero();
}

bool Controller::IsGrabbing() const {
	return impl && !impl->grabbed.empty();
}

void Controller::SetProbe(const nifly::Vector3& position, float radius) {
	if (!IsActive() || !(radius > 0.0f))
		return;

	if (!impl->probe) {
		impl->probe = SystemBuilder::BuildProbe(radius);
		if (!impl->probe)
			return;

		impl->probeRadius = radius;
		impl->probe->m_rootMotion = btTransform(btQuaternion(btVector3(0, 0, 1), impl->rootYawRad));
		static_cast<ProbeBone*>(impl->probe->getBones()[0].get())->m_position = position;

		// Only now: addSkinnedMeshSystem ends with a RESET_PHYSICS readTransform,
		// which seats the sphere wherever the bone currently says it is. Adding
		// it before the position is set would spawn it at the origin and sweep it
		// to the cursor over the first tick, flinging everything on the way.
		impl->world.addSkinnedMeshSystem(impl->probe.get());
		return;
	}

	static_cast<ProbeBone*>(impl->probe->getBones()[0].get())->m_position = position;

	if (radius != impl->probeRadius) {
		impl->probeRadius = radius;
		SystemBuilder::SetProbeRadius(impl->probe.get(), radius);
	}
}

void Controller::ClearProbe() {
	if (!impl || !impl->probe)
		return;

	impl->world.removeSkinnedMeshSystem(impl->probe.get());
	impl->probe.reset();
	impl->probeRadius = 0.0f;
}

bool Controller::IsProbeActive() const {
	return impl && impl->probe;
}

const PoseOverrideMap& Controller::PoseOverrides() const {
	return impl->poseOverrides;
}

const std::unordered_set<std::string>& Controller::AffectedShapes() const {
	return impl->affectedShapes;
}

void Controller::UpdateDebugVis(GLSurface& gls, bool enabled) {
	if (enabled && IsActive())
		impl->debugVis.Update(gls, impl->systems);
	else
		impl->debugVis.Clear(gls);
}
}

#else  // USE_BULLET

// Inert stubs keep call sites valid in builds without Bullet.
namespace Physics {
// Nothing can be simulated, so the applications keep their physics controls
// hidden.
bool HasPhysicsLinks(nifly::NifFile*, const ShapePhysicsFileMap&) {
	return false;
}

std::vector<SystemInfo> CollectPhysicsXmlLinks(nifly::NifFile*, const ShapePhysicsFileMap&) {
	return std::vector<SystemInfo>();
}

struct Controller::Impl {};

Controller::Controller() = default;
Controller::~Controller() = default;

size_t Controller::BuildFromNif(nifly::NifFile*, AnimInfo*, const XmlStreamResolver&, const ShapePhysicsFileMap&, std::vector<std::string>&) {
	return 0;
}

const std::vector<SystemInfo>& Controller::Systems() const {
	static const std::vector<SystemInfo> empty;
	return empty;
}

void Controller::Clear() {}

bool Controller::IsActive() const {
	return false;
}

void Controller::ResetDynamics() {}

bool Controller::PatchProperty(const PatchRequest&) {
	return false;
}

DynamicState Controller::CaptureDynamicState() const {
	return DynamicState();
}

void Controller::RestoreDynamicState(const DynamicState&) {}
void Controller::Step(float) {}
void Controller::InjectCameraYaw(float) {}
void Controller::SetWindStrength(float) {}
void Controller::SetWindDirection(WindDirection) {}

bool Controller::BeginGrab(const std::vector<GrabTarget>&) {
	return false;
}

void Controller::UpdateGrab(const nifly::Vector3&) {}
void Controller::EndGrab() {}

bool Controller::IsGrabbing() const {
	return false;
}

void Controller::SetProbe(const nifly::Vector3&, float) {}
void Controller::ClearProbe() {}

bool Controller::IsProbeActive() const {
	return false;
}

const PoseOverrideMap& Controller::PoseOverrides() const {
	static const PoseOverrideMap empty;
	return empty;
}

const std::unordered_set<std::string>& Controller::AffectedShapes() const {
	static const std::unordered_set<std::string> empty;
	return empty;
}

void Controller::UpdateDebugVis(GLSurface&, bool) {}
}

#endif	// USE_BULLET
