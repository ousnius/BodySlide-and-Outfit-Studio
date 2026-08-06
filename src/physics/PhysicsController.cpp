/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsController.h"

#ifdef USE_BULLET

#include "NiflyBullet.h"
#include "PhysicsDebugDraw.h"
#include "PhysicsSystemBuilder.h"
#include "hdt/hdtSkinnedMeshWorld.h"

#include "../components/Anim.h"

#include <ExtraData.hpp>
#include <NifFile.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <type_traits>

namespace bsos {
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
	class BSOSWorld : public hdt::SkinnedMeshWorld {
	public:
		BSOSWorld() { setGravity(btVector3(0, 0, -9.8f * scaleSkyrim)); }

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
}

struct PhysicsController::Impl {
	BSOSWorld world;
	std::vector<hdt::Ref<BSOSSystem>> systems;
	PoseOverrideMap poseOverrides;
	std::unordered_set<std::string> affectedShapes;
	PhysicsDebugVis debugVis;
	float rootYawRad = 0.0f;
	// Wall-clock time that has not been simulated yet, kept below one tick
	float leftoverTime = 0.0f;
	float windStrength = 0.0f;
	WindDirection windDirection = WindDirection::Right;

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

PhysicsController::PhysicsController()
	: impl(std::make_unique<Impl>()) {}

PhysicsController::~PhysicsController() {
	Clear();
}

size_t PhysicsController::BuildFromNif(nifly::NifFile* nif,
									   AnimInfo* anim,
									   const XmlStreamResolver& resolver,
									   const ShapePhysicsFileMap& shapePhysicsFiles,
									   std::vector<std::string>& outWarnings) {
	Clear();

	if (!nif || !anim || !resolver)
		return 0;

	// One system per distinct XML path, applied to the union of the shapes it
	// was found on. Extra data on a shape applies to that shape; extra data on
	// a NiNode applies to all shapes under that node.
	struct XmlEntry {
		std::string path; // first-seen spelling
		std::vector<nifly::NiShape*> shapes;
	};
	std::vector<XmlEntry> entries;

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

	// Links captured when each NIF was loaded. Outfit Studio merges shapes of
	// every file after the first into one work NIF, dropping the root nodes
	// the links live on, so those shapes are only covered by this map.
	for (auto& shapePhysicsFile : shapePhysicsFiles) {
		auto* shape = nif->FindBlockByName<nifly::NiShape>(shapePhysicsFile.first);
		if (!shape)
			continue;

		for (const auto& xmlPath : shapePhysicsFile.second)
			addEntry(xmlPath, shape);
	}

	for (auto& entry : entries) {
		auto stream = resolver(entry.path);
		if (!stream) {
			outWarnings.push_back("physics XML not found: " + entry.path);
			continue;
		}

		std::ostringstream contents;
		contents << stream->rdbuf();
		const std::string xmlData = contents.str();

		PhysicsBuildInput input;
		input.xmlData = &xmlData;
		input.xmlName = entry.path;
		input.nif = nif;
		input.anim = anim;
		input.shapes = entry.shapes;

		PhysicsSystemBuilder builder;
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

void PhysicsController::Clear() {
	if (!impl)
		return;

	for (auto& system : impl->systems)
		impl->world.removeSkinnedMeshSystem(system.get());

	impl->systems.clear();
	impl->poseOverrides.clear();
	impl->affectedShapes.clear();
	impl->rootYawRad = 0.0f;
	impl->leftoverTime = 0.0f;
}

bool PhysicsController::IsActive() const {
	return impl && !impl->systems.empty();
}

void PhysicsController::ResetDynamics() {
	if (!IsActive())
		return;

	// Mirrors hdtSMP64 SkyrimPhysicsWorld::resetSystems (and the reset call in
	// SkinnedMeshWorld::addSkinnedMeshSystem).
	for (auto& system : impl->systems)
		system->readTransform(system->prepareForRead(hdt::RESET_PHYSICS));

	impl->leftoverTime = 0.0f;
}

void PhysicsController::Step(float dtSeconds) {
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
		const btVector3 offset = impl->world.applyTranslationOffset();
		impl->world.stepSimulation(fixedTimeStep, 1, fixedTimeStep);
		impl->world.restoreTranslationOffset(offset);
		impl->world.writeTransform();
	}
}

void PhysicsController::InjectCameraYaw(float deltaDegrees) {
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

void PhysicsController::SetWindStrength(float strength) {
	impl->windStrength = std::max(0.0f, std::min(strength, 1.0f));
	impl->applyWind();
}

void PhysicsController::SetWindDirection(WindDirection direction) {
	impl->windDirection = direction;
	impl->applyWind();
}

const PoseOverrideMap& PhysicsController::PoseOverrides() const {
	return impl->poseOverrides;
}

const std::unordered_set<std::string>& PhysicsController::AffectedShapes() const {
	return impl->affectedShapes;
}

void PhysicsController::UpdateDebugVis(GLSurface& gls, bool enabled) {
	if (enabled && IsActive())
		impl->debugVis.Update(gls, impl->systems);
	else
		impl->debugVis.Clear(gls);
}
}

#else  // USE_BULLET

// Inert stubs keep call sites valid in builds without Bullet.
namespace bsos {
struct PhysicsController::Impl {};

PhysicsController::PhysicsController() = default;
PhysicsController::~PhysicsController() = default;

size_t PhysicsController::BuildFromNif(nifly::NifFile*, AnimInfo*, const XmlStreamResolver&, const ShapePhysicsFileMap&, std::vector<std::string>&) {
	return 0;
}

void PhysicsController::Clear() {}

bool PhysicsController::IsActive() const {
	return false;
}

void PhysicsController::ResetDynamics() {}
void PhysicsController::Step(float) {}
void PhysicsController::InjectCameraYaw(float) {}
void PhysicsController::SetWindStrength(float) {}
void PhysicsController::SetWindDirection(WindDirection) {}

const PoseOverrideMap& PhysicsController::PoseOverrides() const {
	static const PoseOverrideMap empty;
	return empty;
}

const std::unordered_set<std::string>& PhysicsController::AffectedShapes() const {
	static const std::unordered_set<std::string> empty;
	return empty;
}

void PhysicsController::UpdateDebugVis(GLSurface&, bool) {}
}

#endif	// USE_BULLET
