/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <Object3d.hpp>

#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class AnimInfo;
class GLSurface;

namespace nifly {
class NifFile;
}

namespace bsos {
// Resolves a physics XML path as referenced by a NiStringExtraData (e.g.
// "SKSE\Plugins\hdtSkinnedMeshConfigs\outfit.xml") to a readable stream, or
// nullptr when it cannot be found.
using XmlStreamResolver = std::function<std::unique_ptr<std::istream>(const std::string& xmlPath)>;

// Bone name -> replacement pose-to-global transform produced by the
// simulation, consumed by the skinning code instead of
// AnimBone::xformPoseToGlobal for bones driven by physics. Spelled out again
// instead of including Anim.h, which would pull the application skeleton into
// every user of this header; PhysicsController.cpp asserts that this stays the
// same type as AnimPoseOverrideMap.
using PoseOverrideMap = std::unordered_map<std::string, nifly::MatTransform>;

// Shape name -> physics XML paths linked to that shape by the NIF it was
// loaded from, for shapes merged into a work NIF that no longer holds the
// node hierarchy the link came from.
using ShapePhysicsFileMap = std::unordered_map<std::string, std::vector<std::string>>;

// Direction the wind blows in. Named as the viewport shows it, which looks
// at the mesh from the front, but anchored to the mesh: the wind keeps
// blowing at the same side of it no matter how far the camera has turned it
// inside the physics world.
enum class WindDirection { Up, Down, Forward, Backward, Left, Right };

/*
App-facing facade over the physics core ported from Faster HDT-SMP (see
src/physics/hdt/README.md for its origin, license and the port notes). Owns
the Bullet world and all physics systems built from the XMLs referenced by a
NIF's shapes. App-agnostic: depends only on nifly, AnimInfo/AnimSkeleton and
Bullet, never on OutfitProject or any UI type.

Without USE_BULLET every method is an inert stub so call sites stay valid in
builds without Bullet.
*/
class PhysicsController {
public:
	PhysicsController();
	~PhysicsController();

	PhysicsController(const PhysicsController&) = delete;
	PhysicsController& operator=(const PhysicsController&) = delete;

	// Scans all shapes and nodes of the NIF for NiStringExtraData named
	// "HDT Skinned Mesh Physics Object" and adds the links of
	// "shapePhysicsFiles", which cover shapes whose original node hierarchy
	// is not part of this NIF. Resolves each XML through "resolver" and
	// builds one physics system per (xml, shape set).
	// Bones referenced by an XML but missing from the skeleton and the NIF
	// are reported in "outWarnings" and skipped.
	// Returns the number of systems built.
	size_t BuildFromNif(nifly::NifFile* nif,
						AnimInfo* anim,
						const XmlStreamResolver& resolver,
						const ShapePhysicsFileMap& shapePhysicsFiles,
						std::vector<std::string>& outWarnings);

	// Tears down all systems and the physics world.
	void Clear();

	// True when at least one system was built.
	bool IsActive() const;

	// Re-seats all dynamic bodies on the current kinematic pose. Call after
	// enabling physics, seeking an animation or any other teleport.
	void ResetDynamics();

	// Advances the simulation by the given wall-clock time. Simulates whole
	// 1/60 ticks only and carries the remainder over to the next call, so the
	// result does not depend on the rate this is called at. Time beyond a few
	// ticks is dropped rather than simulated, so a stall cannot explode the
	// simulation.
	void Step(float dtSeconds);

	// Reports a horizontal camera turntable rotation in degrees. Accumulated
	// into a root yaw offset so cloth/hair reacts as if the character turned
	// under a fixed camera.
	void InjectCameraYaw(float deltaDegrees);

	// World wind strength, 0 (off) to 1. Per-bone and per-system wind factors
	// from the XMLs still apply.
	void SetWindStrength(float strength);

	// Direction the wind blows in, relative to the mesh.
	void SetWindDirection(WindDirection direction);

	// Simulated replacement transforms for physics-driven bones.
	const PoseOverrideMap& PoseOverrides() const;

	// Names of shapes whose skinning uses at least one physics-driven bone.
	const std::unordered_set<std::string>& AffectedShapes() const;

	// Draws (or removes, when disabled) collider and constraint overlays.
	void UpdateDebugVis(GLSurface& gls, bool enabled);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};
}
