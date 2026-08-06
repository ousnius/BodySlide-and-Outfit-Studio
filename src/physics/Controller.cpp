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

	for (auto& system : impl->systems)
		impl->world.removeSkinnedMeshSystem(system.get());

	impl->systems.clear();
	impl->poseOverrides.clear();
	impl->affectedShapes.clear();
	impl->rootYawRad = 0.0f;
	impl->leftoverTime = 0.0f;
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

struct Controller::Impl {};

Controller::Controller() = default;
Controller::~Controller() = default;

size_t Controller::BuildFromNif(nifly::NifFile*, AnimInfo*, const XmlStreamResolver&, const ShapePhysicsFileMap&, std::vector<std::string>&) {
	return 0;
}

void Controller::Clear() {}

bool Controller::IsActive() const {
	return false;
}

void Controller::ResetDynamics() {}
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
