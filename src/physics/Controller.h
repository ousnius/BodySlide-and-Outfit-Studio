/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "XmlSchema.h"

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

namespace Physics {
// Resolves a physics XML path as referenced by a NiStringExtraData (e.g.
// "SKSE\Plugins\hdtSkinnedMeshConfigs\outfit.xml") to a readable stream, or
// nullptr when it cannot be found.
using XmlStreamResolver = std::function<std::unique_ptr<std::istream>(const std::string& xmlPath)>;

// Bone name -> replacement pose-to-global transform produced by the
// simulation, consumed by the skinning code instead of
// AnimBone::xformPoseToGlobal for bones driven by physics. Spelled out again
// instead of including Anim.h, which would pull the application skeleton into
// every user of this header; Controller.cpp asserts that this stays the
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

// The wind directions in the order both applications list them in their
// controls, so a selection index means the same thing in either.
const std::vector<std::string>& WindDirectionNames();
WindDirection WindDirectionFromIndex(int index);

// One bone of the mesh patch the cursor grabbed: where the patch holds on to
// that bone ("anchor", in NIF global space, same as PoseOverrideMap) and how
// much of the patch that bone carries. Weight 1 is the bone carrying the most
// of it, so the pull does not get stronger the more bones a patch spans.
struct GrabTarget {
	std::string boneName;
	nifly::Vector3 anchor;
	float weight = 1.0f;
};

// One physics system as it was built: the XML it came from, spelled the way
// the NIF link spells it (which is what the XmlStreamResolver is asked for),
// and the shapes that link was found on.
struct SystemInfo {
	std::string xmlPath;
	std::vector<std::string> shapeNames;
};

// One property change, addressed the way the XML addresses it, to be written
// into the running simulation instead of rebuilding it.
struct PatchRequest {
	std::string xmlPath;	 // which system, as the NIF link spells it
	ElementKind kind = ElementKind::Unknown;
	std::string elementName; // bone or shape name; empty for a constraint
	std::string bodyA;		 // constraints: the two bones they join
	std::string bodyB;
	std::string property; // schema child name, e.g. "linearStiffness"
	ValueVariant value;
};

// Where the simulated bones are and how they are moving at one instant, in NIF
// global space. Taken before a rebuild and put back afterwards so a structural
// edit does not snap cloth back onto the pose; bones are matched by name, so
// bones an edit added start from the pose and everything else carries on.
// Kinematic bones are not part of it: the pose drives them either way.
struct DynamicState {
	struct BoneMotion {
		std::string xmlPath;
		std::string boneName;
		nifly::MatTransform transform;
		nifly::Vector3 linearVelocity;
		nifly::Vector3 angularVelocity;
	};

	std::vector<BoneMotion> bones;
	// Camera turntable yaw the systems were simulating under
	float rootYawDegrees = 0.0f;
};

// True when "nif" or "shapePhysicsFiles" reference at least one physics XML,
// i.e. when a physics preview has anything to simulate. Only looks at extra
// data, so it is cheap enough to call whenever the loaded meshes change; the
// XMLs are neither resolved nor parsed. Always false without Bullet.
bool HasPhysicsLinks(nifly::NifFile* nif, const ShapePhysicsFileMap& shapePhysicsFiles);

// The physics XMLs "nif" and "shapePhysicsFiles" reference, and the shapes
// each of them was found on - the same grouping BuildFromNif would build one
// system per. Only reads extra data: nothing is resolved, parsed or simulated,
// so the editor can list and open the XMLs of a mesh that is not simulating.
// Always empty without Bullet, where nothing shows the physics UI anyway.
std::vector<SystemInfo> CollectPhysicsXmlLinks(nifly::NifFile* nif, const ShapePhysicsFileMap& shapePhysicsFiles);

/*
App-facing facade over the physics core ported from Faster HDT-SMP (see
src/physics/hdt/README.md for its origin, license and the port notes). Owns
the Bullet world and all physics systems built from the XMLs referenced by a
NIF's shapes. App-agnostic: depends only on nifly, AnimInfo/AnimSkeleton and
Bullet, never on OutfitProject or any UI type.

Without USE_BULLET every method is an inert stub so call sites stay valid in
builds without Bullet.
*/
class Controller {
public:
	Controller();
	~Controller();

	Controller(const Controller&) = delete;
	Controller& operator=(const Controller&) = delete;

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

	// The systems BuildFromNif built, in build order.
	const std::vector<SystemInfo>& Systems() const;

	// Tears down all systems and the physics world.
	void Clear();

	// True when at least one system was built.
	bool IsActive() const;

	// Re-seats all dynamic bodies on the current kinematic pose. Call after
	// enabling physics, seeking an animation or any other teleport.
	void ResetDynamics();

	// Writes one changed property straight into the running simulation, so a
	// value can be tuned while watching the cloth react instead of after a
	// rebuild that drops it back onto the pose.
	//
	// Returns false when the change cannot be made live - the value is baked
	// into the objects at build time, the element has no live counterpart, the
	// name it refers to does not resolve, or the address matches more than one
	// constraint - and the caller then has to rebuild the system. False is a
	// normal answer, not an error: it is the backstop that keeps the schema
	// table's PatchKind from having to be perfect.
	bool PatchProperty(const PatchRequest& request);

	// Takes a snapshot of the motion of every dynamic bone, to be handed back
	// to RestoreDynamicState after rebuilding the systems from edited XML.
	DynamicState CaptureDynamicState() const;

	// Puts the bones named by "state" back where they were and moving as they
	// were. Bones the state does not name (or that came back kinematic) keep
	// the pose the rebuild put them on. Ends any grab, which held on to rigid
	// bodies that no longer exist.
	void RestoreDynamicState(const DynamicState& state);

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

	// Starts dragging the given bones by their anchors, as if the user had
	// taken hold of the mesh there. Bones no system simulates and bones the
	// pose drives (kinematic ones) are dropped; returns false when that leaves
	// nothing to drag. Replaces a grab still in progress.
	//
	// Nothing is moved directly: each bone is pulled towards where the cursor
	// wants its anchor through a critically damped spring, so the constraints,
	// collisions and gravity of the physics XML decide what the mesh actually
	// does on the way, and how far it gets at all.
	bool BeginGrab(const std::vector<GrabTarget>& targets);

	// Moves the grab to "offset" away from where it started (NIF global space).
	void UpdateGrab(const nifly::Vector3& offset);

	// Lets go. The bones keep whatever velocity the drag gave them, so a fast
	// release flings them.
	void EndGrab();

	bool IsGrabbing() const;

	// Places a sphere the simulation collides with at "position" (NIF global
	// space), creating it on first use. It is kinematic: cloth and hair are
	// pushed out of its way through the normal contact solver, so the
	// constraints, collisions and gravity of the physics XML still decide the
	// result, and nothing the sphere hits can move it.
	void SetProbe(const nifly::Vector3& position, float radius);

	// Takes the sphere back out of the world.
	void ClearProbe();

	bool IsProbeActive() const;

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
