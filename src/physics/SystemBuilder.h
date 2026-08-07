/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#ifdef USE_BULLET

#include "Controller.h"

#include "hdt/hdtConeTwistConstraint.h"
#include "hdt/hdtGeneric6DofConstraint.h"
#include "hdt/hdtSkinnedMeshBody.h"
#include "hdt/hdtSkinnedMeshBone.h"
#include "hdt/hdtSkinnedMeshSystem.h"
#include "hdt/hdtStiffSpringConstraint.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class AnimBone;
class AnimInfo;

namespace nifly {
class NifFile;
class NiShape;
}

namespace hdt {
class XMLReader;
class PerVertexShape;
}

namespace Physics {
class PreviewSystem;

// Port of hdtSMP64's SkyrimBone: a physics bone bound to an AnimBone of the
// application skeleton instead of a NiNode of the game scene graph.
class PreviewBone : public hdt::SkinnedMeshBone {
public:
	PreviewBone(const hdt::IDStr& name, AnimBone* animBone, PreviewSystem* system, btRigidBody::btRigidBodyConstructionInfo& ci);

	void readTransform(float timeStep) override;
	void writeTransform() override;

	int m_depth = 0;
	AnimBone* m_animBone = nullptr;
	PreviewSystem* m_system = nullptr;
};

// The bone the collision probe hangs off. Unlike PreviewBone it is bound to no
// AnimBone and drives no skeleton bone: its kinematic pose is wherever the
// cursor last put it.
class ProbeBone : public hdt::SkinnedMeshBone {
public:
	ProbeBone(PreviewSystem* system, btRigidBody::btRigidBodyConstructionInfo& ci);

	void readTransform(float timeStep) override;
	void writeTransform() override {}

	PreviewSystem* m_system = nullptr;

	// Where the probe should be, in NIF global space.
	nifly::Vector3 m_position;
};

// Port of hdtSMP64's SkyrimBody. The game's runtime disable machinery
// (updateActiveState) is not ported, but the XML attributes feeding it are
// still parsed into the members below.
class PreviewBody : public hdt::SkinnedMeshBody {
public:
	enum class SharedType {
		SHARED_PUBLIC,
		SHARED_INTERNAL,
		SHARED_EXTERNAL,
		SHARED_PRIVATE,
	};

	PreviewSystem* m_mesh = nullptr;
	SharedType m_shared = SharedType::SHARED_PUBLIC;
	bool m_disabled = false;
	// The collision probe, which comes from no XML and so cannot be covered by
	// anything a shape declares it collides with
	bool m_isProbe = false;
	int m_disablePriority = 0;
	hdt::IDStr m_disableTag;

	bool canCollideWith(const hdt::SkinnedMeshBody* body) const override;
	void internalUpdate() override;
};

// Port of hdtSMP64's SkyrimSystem minus the game skeleton members. Root
// motion (camera turntable yaw) takes the role of the skeleton root
// transform the game watched.
class PreviewSystem : public hdt::SkinnedMeshSystem {
	friend class SystemBuilder;

public:
	hdt::SkinnedMeshBone* findBone(const hdt::IDStr& name);
	hdt::SkinnedMeshBody* findBody(const hdt::IDStr& name);
	int findBoneIdx(const hdt::IDStr& name);

	float prepareForRead(float timeStep) override;

	const std::vector<hdt::Ref<hdt::SkinnedMeshBody>>& meshes() const { return m_meshes; }
	const std::vector<hdt::Ref<hdt::BoneScaleConstraint>>& constraints() const { return m_constraints; }
	const std::vector<hdt::Ref<hdt::ConstraintGroup>>& constraintGroups() const { return m_constraintGroups; }

	// Set by the controller before each step; readTransform composes it in
	// front of every kinematic pose, writeTransform removes it again.
	btTransform m_rootMotion = btTransform::getIdentity();

	// Simulated replacement pose transforms, owned by the controller.
	PoseOverrideMap* m_poseOverrides = nullptr;

	bool m_initialized = false;

	// angular velocity damper (upstream: SkyrimPhysicsWorld config values)
	bool m_clampRotations = true;
	float m_rotationSpeedLimit = 10.0f;
	bool m_unclampedResets = true;
	float m_unclampedResetAngle = 120.0f;
	btQuaternion m_lastRootRotation = btQuaternion::getIdentity();
};

// Inputs for building one physics system from one XML file. Vertices,
// triangles, bone weights and skin transforms come from the NIF/AnimInfo
// instead of the game's NiSkinInstance.
struct BuildInput {
	const std::string* xmlData = nullptr; // whole XML file contents
	std::string xmlName;				  // diagnostics prefix
	nifly::NifFile* nif = nullptr;
	AnimInfo* anim = nullptr;
	std::vector<nifly::NiShape*> shapes;  // shapes this XML applies to
};

/*
Port of hdtSMP64's SkyrimSystemCreator: parses a physics XML and builds the
bone rigid bodies, constraints and collision meshes of one PreviewSystem, bound
to AnimSkeleton bones. Mesh data (vertices, triangles, weights, skin-to-bone
transforms) is sourced from nifly/AnimInfo. Diagnostics go to the warning
list passed to Build.
*/
class SystemBuilder {
public:
	hdt::Ref<PreviewSystem> Build(const BuildInput& input, std::vector<std::string>& outWarnings);

	// Builds the one-bone system behind Controller's collision probe: a single
	// kinematic bone carrying a single-vertex per-vertex shape. That is exactly
	// a sphere collider here, because the collider radius of a per-vertex shape
	// is the vertex margin multiplier times the shape margin - so the probe
	// needs no tessellation and no special case anywhere in the solver.
	static hdt::Ref<PreviewSystem> BuildProbe(float radius);

	// Resizes a system built by BuildProbe.
	static void SetProbeRadius(PreviewSystem* probe, float radius);

protected:
	std::unordered_map<hdt::IDStr, PreviewBone*> m_boneIndex;

	void indexBone(PreviewBone* bone);
	PreviewBone* findBoneFromIndex(const hdt::IDStr& name) const;

	struct DeferredBuild {
		hdt::SkinnedMeshBody* body;
		hdt::PerVertexShape* vertexShape;
	};

	std::vector<DeferredBuild> m_deferredBuilds;

	struct BoneTemplate : public btRigidBody::btRigidBodyConstructionInfo {
		static btEmptyShape emptyShape[1];

		BoneTemplate()
			: btRigidBodyConstructionInfo(0, nullptr, emptyShape) {
			m_centerOfMassTransform = btTransform::getIdentity();
			m_marginMultipler = 1.f;
		}

		std::shared_ptr<btCollisionShape> m_shape;
		std::vector<hdt::IDStr> m_canCollideWithBone;
		std::vector<hdt::IDStr> m_noCollideWithBone;
		btTransform m_centerOfMassTransform;
		float m_marginMultipler;
		float m_gravityFactor = 1.0f;
		float m_windFactor = 1.0f;
		hdt::U32 m_collisionFilter = 0;
	};

	enum FrameType {
		FrameInA,
		FrameInB,
		FrameInLerp,
		AWithXPointToB,
		AWithYPointToB,
		AWithZPointToB
	};

	struct GenericConstraintTemplate {
		FrameType frameType = FrameInB;
		bool useLinearReferenceFrameA = false;
		btTransform frame = btTransform::getIdentity();
		btVector3 linearLowerLimit = btVector3(1, 1, 1);
		btVector3 linearUpperLimit = btVector3(-1, -1, -1);
		btVector3 angularLowerLimit = btVector3(1, 1, 1);
		btVector3 angularUpperLimit = btVector3(-1, -1, -1);
		btVector3 linearStiffness = btVector3(0, 0, 0);
		btVector3 angularStiffness = btVector3(0, 0, 0);
		btVector3 linearDamping = btVector3(0, 0, 0);
		btVector3 angularDamping = btVector3(0, 0, 0);
		btVector3 linearEquilibrium = btVector3(0, 0, 0);
		btVector3 angularEquilibrium = btVector3(0, 0, 0);
		btVector3 linearBounce = btVector3(0, 0, 0);
		btVector3 angularBounce = btVector3(0, 0, 0);
		bool enableLinearSprings = true;
		bool enableAngularSprings = true;
		bool linearStiffnessLimited = true;
		bool angularStiffnessLimited = true;
		bool springDampingLimited = true;
		bool linearMotors = false;
		bool angularMotors = false;
		// TODO: Test if servo motors go to [0, 0, 0], or whatever equilibrium is. Provide option to set servo motor target. Hard coded to equilibrium right now.
		bool linearServoMotors = false;
		bool angularServoMotors = false;
		btVector3 linearNonHookeanDamping = btVector3(0, 0, 0);
		btVector3 angularNonHookeanDamping = btVector3(0, 0, 0);
		btVector3 linearNonHookeanStiffness = btVector3(0, 0, 0);
		btVector3 angularNonHookeanStiffness = btVector3(0, 0, 0);
		btVector3 linearTargetVelocity = btVector3(0, 0, 0);
		btVector3 angularTargetVelocity = btVector3(0, 0, 0);
		btVector3 linearMaxMotorForce = btVector3(0, 0, 0);
		btVector3 angularMaxMotorForce = btVector3(0, 0, 0);
		btScalar motorERP = 0.9f;
		btScalar motorCFM = 0;
		btScalar stopERP = 0.2f;
		btScalar stopCFM = 0;
	};

	struct StiffSpringConstraintTemplate {
		float minDistanceFactor = 1;
		float maxDistanceFactor = 1;
		float stiffness = 0;
		float damping = 0;
		float equilibriumFactor = 0.5;
	};

	struct ConeTwistConstraintTemplate {
		btTransform frame = btTransform::getIdentity();
		FrameType frameType = FrameInB;
		float swingSpan1 = 0;
		float swingSpan2 = 0;
		float twistSpan = 0;
		float limitSoftness = 1.0f;
		float biasFactor = 0.3f;
		float relaxationFactor = 1.0f;
	};

	// NIF shape -> offset of its first vertex within the merged body
	using VertexOffsetMap = std::vector<std::pair<nifly::NiShape*, int>>;

	hdt::Ref<PreviewSystem> m_mesh;
	nifly::NifFile* m_nif = nullptr;
	AnimInfo* m_anim = nullptr;
	const std::vector<nifly::NiShape*>* m_nifShapes = nullptr;
	hdt::XMLReader* m_reader = nullptr;

	std::string m_filePath;
	std::vector<std::string>* m_warnings = nullptr;

	void warn(const std::string& msg);

	// Body of Build once the reader is set up
	hdt::Ref<PreviewSystem> readSystem();

	AnimBone* findAnimBone(const hdt::IDStr& name);
	PreviewBone* getOrCreateBone(const hdt::IDStr& name);

	std::unordered_map<hdt::IDStr, BoneTemplate> m_boneTemplates;
	std::unordered_map<hdt::IDStr, GenericConstraintTemplate> m_genericConstraintTemplates;
	std::unordered_map<hdt::IDStr, StiffSpringConstraintTemplate> m_stiffSpringConstraintTemplates;
	std::unordered_map<hdt::IDStr, ConeTwistConstraintTemplate> m_coneTwistConstraintTemplates;
	std::unordered_map<hdt::IDStr, std::shared_ptr<btCollisionShape>> m_shapes;
	std::vector<std::shared_ptr<btCollisionShape>> m_shapeRefs;

	std::pair<hdt::Ref<PreviewBody>, VertexOffsetMap> generateMeshBody(const std::string& name);

	bool findBones(const hdt::IDStr& bodyAName, const hdt::IDStr& bodyBName, PreviewBone*& bodyA, PreviewBone*& bodyB);
	bool parseFrameType(const std::string& name, FrameType& type, btTransform& frame);
	static void calcFrame(FrameType type, const btTransform& frame, const hdt::btQsTransform& trA, const hdt::btQsTransform& trB, btTransform& frameA, btTransform& frameB);
	void readFrameLerp(btTransform& tr);
	void readBoneTemplate(BoneTemplate& dest);
	void readGenericConstraintTemplate(GenericConstraintTemplate& dest);
	void readStiffSpringConstraintTemplate(StiffSpringConstraintTemplate& dest);
	void readConeTwistConstraintTemplate(ConeTwistConstraintTemplate& dest);

	const BoneTemplate& getBoneTemplate(const hdt::IDStr& name);
	const GenericConstraintTemplate& getGenericConstraintTemplate(const hdt::IDStr& name);
	const StiffSpringConstraintTemplate& getStiffSpringConstraintTemplate(const hdt::IDStr& name);
	const ConeTwistConstraintTemplate& getConeTwistConstraintTemplate(const hdt::IDStr& name);

	PreviewBone* createBoneFromNodeName(const hdt::IDStr& bodyName, const hdt::IDStr& templateName = hdt::IDStr(""), const bool readTemplate = false);
	void readOrUpdateBone();
	hdt::Ref<PreviewBody> readPerVertexShape();
	hdt::Ref<PreviewBody> readPerTriangleShape();
	hdt::Ref<hdt::Generic6DofConstraint> readGenericConstraint();
	hdt::Ref<hdt::StiffSpringConstraint> readStiffSpringConstraint();
	hdt::Ref<hdt::ConeTwistConstraint> readConeTwistConstraint();
	hdt::Ref<hdt::ConstraintGroup> readConstraintGroup();
	std::shared_ptr<btCollisionShape> readShape();
};
}

#endif	// USE_BULLET
