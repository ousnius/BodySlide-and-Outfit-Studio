/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "ExtraData.h"
#include "Animation.h"

typedef uint HavokMaterial;

struct HavokFilter {
	byte layer = 1;
	byte flagsAndParts = 0;
	ushort group = 0;
};

struct hkWorldObjCInfoProperty {
	uint data = 0;
	uint size = 0;
	uint capacityAndFlags = 0x80000000;
};

enum MotorType : byte {
	MOTOR_NONE = 0,
	MOTOR_POSITION = 1,
	MOTOR_VELOCITY = 2,
	MOTOR_SPRING = 3
};

struct bhkLimitedForceConstraintMotor {
	float minForce = -1000000.0f;
	float maxForce = 1000000.0f;
	bool motorEnabled = false;
};

struct bhkPositionConstraintMotor : bhkLimitedForceConstraintMotor {
	float tau = 0.8f;
	float damping = 1.0f;
	float proportionalRecoveryVelocity = 2.0f;
	float constantRecoveryVelocity = 1.0f;

	void Get(NiStream& stream) {
		stream >> minForce;
		stream >> maxForce;
		stream >> tau;
		stream >> damping;
		stream >> proportionalRecoveryVelocity;
		stream >> constantRecoveryVelocity;
		stream >> motorEnabled;
	}

	void Put(NiStream& stream) {
		stream << minForce;
		stream << maxForce;
		stream << tau;
		stream << damping;
		stream << proportionalRecoveryVelocity;
		stream << constantRecoveryVelocity;
		stream << motorEnabled;
	}
};

struct bhkVelocityConstraintMotor : bhkLimitedForceConstraintMotor {
	float tau = 0.0f;
	float velocityTarget = 0.0f;
	bool useVelocityTargetFromConstraintTargets = 0.0f;

	void Get(NiStream& stream) {
		stream >> minForce;
		stream >> maxForce;
		stream >> tau;
		stream >> velocityTarget;
		stream >> useVelocityTargetFromConstraintTargets;
		stream >> motorEnabled;
	}

	void Put(NiStream& stream) {
		stream << minForce;
		stream << maxForce;
		stream << tau;
		stream << velocityTarget;
		stream << useVelocityTargetFromConstraintTargets;
		stream << motorEnabled;
	}
};

struct bhkSpringDamperConstraintMotor : bhkLimitedForceConstraintMotor {
	float springConstant = 0.0f;
	float springDamping = 0.0f;

	void Get(NiStream& stream) {
		stream >> minForce;
		stream >> maxForce;
		stream >> springConstant;
		stream >> springDamping;
		stream >> motorEnabled;
	}

	void Put(NiStream& stream) {
		stream << minForce;
		stream << maxForce;
		stream << springConstant;
		stream << springDamping;
		stream << motorEnabled;
	}
};

struct MotorDesc {
	MotorType motorType = MOTOR_NONE;
	bhkPositionConstraintMotor motorPosition;
	bhkVelocityConstraintMotor motorVelocity;
	bhkSpringDamperConstraintMotor motorSpringDamper;

	void Get(NiStream& stream) {
		stream >> motorType;

		switch (motorType) {
		case MOTOR_POSITION:
			motorPosition.Get(stream);
			break;
		case MOTOR_VELOCITY:
			motorVelocity.Get(stream);
			break;
		case MOTOR_SPRING:
			motorSpringDamper.Get(stream);
			break;
		default:
			break;
		}
	}

	void Put(NiStream& stream) {
		stream << motorType;

		switch (motorType) {
		case MOTOR_POSITION:
			motorPosition.Put(stream);
			break;
		case MOTOR_VELOCITY:
			motorVelocity.Put(stream);
			break;
		case MOTOR_SPRING:
			motorSpringDamper.Put(stream);
			break;
		default:
			break;
		}
	}
};

struct HingeDesc {
	Vector4 axleA;
	Vector4 axleInA1;
	Vector4 axleInA2;
	Vector4 pivotA;
	Vector4 axleB;
	Vector4 axleInB1;
	Vector4 axleInB2;
	Vector4 pivotB;
};

struct LimitedHingeDesc {
	HingeDesc hinge;
	float minAngle = 0.0f;
	float maxAngle = 0.0f;
	float maxFriction = 0.0f;
	MotorDesc motorDesc;
};

struct RagdollDesc {
	Vector4 twistA;
	Vector4 planeA;
	Vector4 motorA;
	Vector4 pivotA;
	Vector4 twistB;
	Vector4 planeB;
	Vector4 motorB;
	Vector4 pivotB;
	float coneMaxAngle = 0.0f;
	float planeMinAngle = 0.0f;
	float planeMaxAngle = 0.0f;
	float twistMinAngle = 0.0f;
	float twistMaxAngle = 0.0f;
	float maxFriction = 0.0f;
	MotorDesc motorDesc;
};

struct StiffSpringDesc {
	Vector4 pivotA;
	Vector4 pivotB;
	float length = 0.0f;
};

struct BallAndSocketDesc {
	Vector4 translationA;
	Vector4 translationB;
};

struct PrismaticDesc {
	Vector4 slidingA;
	Vector4 rotationA;
	Vector4 planeA;
	Vector4 pivotA;
	Vector4 slidingB;
	Vector4 rotationB;
	Vector4 planeB;
	Vector4 pivotB;
	float minDistance = 0.0f;
	float maxDistance = 0.0f;
	float friction = 0.0f;
	MotorDesc motorDesc;
};

enum hkConstraintType : uint {
	BallAndSocket = 0,
	Hinge = 1,
	LimitedHinge = 2,
	Prismatic = 6,
	Ragdoll = 7,
	StiffSpring = 8
};

struct bhkCMSDMaterial {
	HavokMaterial material = 0;
	HavokFilter layer;
};

struct bhkCMSDBigTris {
	ushort triangle1 = 0;
	ushort triangle2 = 0;
	ushort triangle3 = 0;
	HavokMaterial material = 0;
	ushort weldingInfo = 0;
};

struct bhkCMSDTransform {
	Vector4 translation;
	QuaternionXYZW rotation;
};

struct bhkCMSDChunk {
	Vector4 translation;
	uint matIndex = 0;
	ushort reference = 0;
	ushort transformIndex = 0;

	uint numVerts = 0;
	std::vector<ushort> verts;
	uint numIndices = 0;
	std::vector<ushort> indices;
	uint numStrips = 0;
	std::vector<ushort> strips;
	uint numWeldingInfo = 0;
	std::vector<ushort> weldingInfo;
};

class NiAVObject;

class NiCollisionObject : public NiObject {
private:
	BlockRef<NiAVObject> targetRef;

public:
	static constexpr const char* BlockName = "NiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiCollisionObject* Clone() { return new NiCollisionObject(*this); }
};

enum PropagationMode : uint {
	PROPAGATE_ON_SUCCESS,
	PROPAGATE_ON_FAILURE,
	PROPAGATE_ALWAYS,
	PROPAGATE_NEVER
};

enum CollisionMode : uint {
	CM_USE_OBB,
	CM_USE_TRI,
	CM_USE_ABV,
	CM_NOTEST,
	CM_USE_NIBOUND
};

enum BoundVolumeType : uint {
	BASE_BV = 0xFFFFFFFF,
	SPHERE_BV = 0,
	BOX_BV = 1,
	CAPSULE_BV = 2,
	UNION_BV = 4,
	HALFSPACE_BV = 5
};

struct BoxBV {
	Vector3 center;
	Vector3 axis1;
	Vector3 axis2;
	Vector3 axis3;
	float extent1 = 0.0f;
	float extent2 = 0.0f;
	float extent3 = 0.0f;
};

struct CapsuleBV {
	Vector3 center;
	Vector3 origin;
	float unkFloat1 = 0.0f;
	float unkFloat2 = 0.0f;
};

struct HalfSpaceBV {
	Vector3 normal;
	Vector3 center;
	float unkFloat1 = 0.0f;
};

struct UnionBV;

struct BoundingVolume {
	BoundVolumeType collisionType = BASE_BV;
	BoundingSphere bvSphere;
	BoxBV bvBox;
	CapsuleBV bvCapsule;
	UnionBV* bvUnion = nullptr;
	HalfSpaceBV bvHalfSpace;

	BoundingVolume();
	~BoundingVolume();

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

struct UnionBV {
	uint numBV = 0;
	std::vector<BoundingVolume> boundingVolumes;

	void Get(NiStream& stream) {
		stream >> numBV;
		boundingVolumes.resize(numBV);
		for (int i = 0; i < numBV; i++)
			boundingVolumes[i].Get(stream);
	}

	void Put(NiStream& stream) {
		stream << numBV;
		for (int i = 0; i < numBV; i++)
			boundingVolumes[i].Put(stream);
	}
};

class NiCollisionData : public NiCollisionObject {
private:
	PropagationMode propagationMode = PROPAGATE_ON_SUCCESS;
	CollisionMode collisionMode = CM_USE_OBB;
	bool useABV = false;
	BoundingVolume boundingVolume;

public:
	static constexpr const char* BlockName = "NiCollisionData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiCollisionData* Clone() { return new NiCollisionData(*this); }
};

class bhkNiCollisionObject : public NiCollisionObject {
private:
	ushort flags = 1;
	BlockRef<NiObject> bodyRef;

public:
	static constexpr const char* BlockName = "bhkNiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkNiCollisionObject* Clone() { return new bhkNiCollisionObject(*this); }
};

class bhkCollisionObject : public bhkNiCollisionObject {
public:
	static constexpr const char* BlockName = "bhkCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkCollisionObject* Clone() { return new bhkCollisionObject(*this); }
};

class bhkNPCollisionObject : public bhkCollisionObject {
private:
	uint bodyID = 0;

public:
	static constexpr const char* BlockName = "bhkNPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkNPCollisionObject* Clone() { return new bhkNPCollisionObject(*this); }
};

class bhkPCollisionObject : public bhkNiCollisionObject {
public:
	static constexpr const char* BlockName = "bhkPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkPCollisionObject* Clone() { return new bhkPCollisionObject(*this); }
};

class bhkSPCollisionObject : public bhkPCollisionObject {
public:
	static constexpr const char* BlockName = "bhkSPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSPCollisionObject* Clone() { return new bhkSPCollisionObject(*this); }
};

class bhkBlendCollisionObject : public bhkCollisionObject {
private:
	float heirGain = 0.0f;
	float velGain = 0.0f;

public:
	static constexpr const char* BlockName = "bhkBlendCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkBlendCollisionObject* Clone() { return new bhkBlendCollisionObject(*this); }
};

class bhkPhysicsSystem : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	bhkPhysicsSystem(const uint size = 0);

	static constexpr const char* BlockName = "bhkPhysicsSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkPhysicsSystem* Clone() { return new bhkPhysicsSystem(*this); }
};

class bhkRagdollSystem : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	bhkRagdollSystem(const uint size = 0);

	static constexpr const char* BlockName = "bhkRagdollSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkRagdollSystem* Clone() { return new bhkRagdollSystem(*this); }
};

class bhkBlendController : public NiTimeController {
private:
	uint keys = 0;

public:
	static constexpr const char* BlockName = "bhkBlendController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkBlendController* Clone() { return new bhkBlendController(*this); }
};

class bhkRefObject : public NiObject {
};

class bhkSerializable : public bhkRefObject {
};

class bhkShape : public bhkSerializable {
};

class bhkHeightFieldShape : public bhkShape {
};

class bhkPlaneShape : public bhkHeightFieldShape {
private:
	HavokMaterial material = 0;
	Vector3 unkVec;
	Vector3 direction;
	float constant = 0.0f;
	Vector4 halfExtents;
	Vector4 center;

public:
	static constexpr const char* BlockName = "bhkPlaneShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkPlaneShape* Clone() { return new bhkPlaneShape(*this); }
};

class bhkSphereRepShape : public bhkShape {
private:
	HavokMaterial material = 0;
	float radius = 0.0f;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class bhkMultiSphereShape : public bhkSphereRepShape {
private:
	float unkFloat1 = 0.0f;
	float unkFloat2 = 0.0f;
	uint numSpheres = 0;
	std::vector<BoundingSphere> spheres;

public:
	static constexpr const char* BlockName = "bhkMultiSphereShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkMultiSphereShape* Clone() { return new bhkMultiSphereShape(*this); }
};

class bhkConvexShape : public bhkSphereRepShape {
};

class bhkConvexListShape : public bhkShape {
private:
	BlockRefArray<bhkConvexShape> shapeRefs;
	HavokMaterial material = 0;
	float radius = 0.0f;
	uint unkInt1 = 0;
	float unkFloat1 = 0.0f;
	hkWorldObjCInfoProperty childShapeProp;
	byte unkByte1 = 0;
	float unkFloat2 = 0.0f;

public:
	static constexpr const char* BlockName = "bhkConvexListShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkConvexListShape* Clone() { return new bhkConvexListShape(*this); }

	BlockRefArray<bhkConvexShape>& GetShapes();
};

class bhkConvexVerticesShape : public bhkConvexShape {
private:
	hkWorldObjCInfoProperty vertsProp;
	hkWorldObjCInfoProperty normalsProp;

	uint numVerts = 0;
	std::vector<Vector4> verts;

	uint numNormals = 0;
	std::vector<Vector4> normals;

public:
	static constexpr const char* BlockName = "bhkConvexVerticesShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkConvexVerticesShape* Clone() { return new bhkConvexVerticesShape(*this); }
};

class bhkBoxShape : public bhkConvexShape {
private:
	uint64_t padding = 0;
	Vector3 dimensions;
	float radius2 = 0.0f;

public:
	static constexpr const char* BlockName = "bhkBoxShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkBoxShape* Clone() { return new bhkBoxShape(*this); }
};

class bhkSphereShape : public bhkConvexShape {
public:
	static constexpr const char* BlockName = "bhkSphereShape";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSphereShape* Clone() { return new bhkSphereShape(*this); }
};

class bhkTransformShape : public bhkShape {
private:
	BlockRef<bhkShape> shapeRef;
	HavokMaterial material = 0;
	float radius = 0.0f;
	uint64_t padding = 0;
	Matrix4 xform;

public:
	static constexpr const char* BlockName = "bhkTransformShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkTransformShape* Clone() { return new bhkTransformShape(*this); }
};

class bhkConvexTransformShape : public bhkTransformShape {
public:
	static constexpr const char* BlockName = "bhkConvexTransformShape";
	virtual const char* GetBlockName() { return BlockName; }

	bhkConvexTransformShape* Clone() { return new bhkConvexTransformShape(*this); }
};

class bhkCapsuleShape : public bhkConvexShape {
private:
	uint64_t padding = 0;
	Vector3 point1;
	float radius1 = 0.0f;
	Vector3 point2;
	float radius2 = 0.0f;

public:
	static constexpr const char* BlockName = "bhkCapsuleShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkCapsuleShape* Clone() { return new bhkCapsuleShape(*this); }
};

class bhkBvTreeShape : public bhkShape {
};

class bhkMoppBvTreeShape : public bhkBvTreeShape {
private:
	BlockRef<bhkShape> shapeRef;
	uint userData = 0;
	uint shapeCollection = 0;
	uint code = 0;
	float scale = 0.0f;
	uint dataSize = 0;
	Vector4 offset;
	byte buildType = 2;						// User Version >= 12
	std::vector<byte> data;

public:
	static constexpr const char* BlockName = "bhkMoppBvTreeShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkMoppBvTreeShape* Clone() { return new bhkMoppBvTreeShape(*this); }
};

class NiTriStripsData;

class bhkNiTriStripsShape : public bhkShape {
private:
	HavokMaterial material = 0;
	float radius = 0.1f;
	uint unused1 = 0;
	uint unused2 = 0;
	uint unused3 = 0;
	uint unused4 = 0;
	uint unused5 = 0;
	uint growBy = 1;
	Vector4 scale = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	BlockRefArray<NiTriStripsData> partRefs;

	uint numFilters = 0;
	std::vector<uint> filters;

public:
	static constexpr const char* BlockName = "bhkNiTriStripsShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkNiTriStripsShape* Clone() { return new bhkNiTriStripsShape(*this); }

	BlockRefArray<NiTriStripsData>& GetParts();
};

class bhkShapeCollection : public bhkShape {
};

class bhkListShape : public bhkShapeCollection {
private:
	BlockRefArray<bhkShape> subShapeRefs;
	HavokMaterial material = 0;
	hkWorldObjCInfoProperty childShapeProp;
	hkWorldObjCInfoProperty childFilterProp;
	uint numUnkInts = 0;
	std::vector<uint> unkInts;

public:
	static constexpr const char* BlockName = "bhkListShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkListShape* Clone() { return new bhkListShape(*this); }

	BlockRefArray<bhkShape>& GetSubShapes();
};

struct hkTriangleData {
	Triangle tri;
	ushort weldingInfo = 0;
};

struct hkTriangleNormalData {
	Triangle tri;
	ushort weldingInfo = 0;
	Vector3 normal;
};

struct hkSubPartData {
	HavokFilter filter;
	uint numVerts = 0;
	HavokMaterial material = 0;
};

class hkPackedNiTriStripsData : public bhkShapeCollection {
private:
	uint keyCount = 0;
	std::vector<hkTriangleData> triData;
	std::vector<hkTriangleNormalData> triNormData;

	uint numVerts = 0;
	byte unkByte = 0;
	std::vector<Vector3> compressedVertData;

	ushort partCount = 0;
	std::vector<hkSubPartData> data;

public:
	static constexpr const char* BlockName = "hkPackedNiTriStripsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	hkPackedNiTriStripsData* Clone() { return new hkPackedNiTriStripsData(*this); }
};

class bhkPackedNiTriStripsShape : public bhkShapeCollection {
private:
	ushort partCount = 0;
	std::vector<hkSubPartData> data;

	uint userData = 0;
	uint unused1 = 0;
	float radius = 0.0f;
	uint unused2 = 0;
	Vector4 scaling;
	float radius2 = 0.0f;
	Vector4 scaling2;
	BlockRef<hkPackedNiTriStripsData> dataRef;

public:
	static constexpr const char* BlockName = "bhkPackedNiTriStripsShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkPackedNiTriStripsShape* Clone() { return new bhkPackedNiTriStripsShape(*this); }
};

class bhkLiquidAction : public bhkSerializable {
private:
	uint userData = 0;
	uint unkInt1 = 0;
	uint unkInt2 = 0;
	float initialStickForce = 0.0f;
	float stickStrength = 0.0f;
	float neighborDistance = 0.0f;
	float neighborStrength = 0.0f;

public:
	static constexpr const char* BlockName = "bhkLiquidAction";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkLiquidAction* Clone() { return new bhkLiquidAction(*this); }
};

class bhkOrientHingedBodyAction : public bhkSerializable {
private:
	BlockRef<NiObject> bodyRef;
	uint unkInt1 = 0;
	uint unkInt2 = 0;
	uint64_t padding = 0;
	Vector4 hingeAxisLS;
	Vector4 forwardLS;
	float strength = 0.0f;
	float damping = 0.0f;
	uint64_t padding2 = 0;

public:
	static constexpr const char* BlockName = "bhkOrientHingedBodyAction";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	bhkOrientHingedBodyAction* Clone() { return new bhkOrientHingedBodyAction(*this); }
};

class bhkWorldObject : public bhkSerializable {
private:
	BlockRef<bhkShape> shapeRef;
	HavokFilter collisionFilter;
	int unkInt1;
	byte broadPhaseType;
	byte unkBytes[3];
	hkWorldObjCInfoProperty prop;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkWorldObject* Clone() { return new bhkWorldObject(*this); }
};

class bhkPhantom : public bhkWorldObject {
};

class bhkShapePhantom : public bhkPhantom {
};

class bhkSimpleShapePhantom : public bhkShapePhantom {
private:
	uint64_t padding = 0;
	Matrix4 transform;

public:
	static constexpr const char* BlockName = "bhkSimpleShapePhantom";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkSimpleShapePhantom* Clone() { return new bhkSimpleShapePhantom(*this); }
};

class bhkAabbPhantom : public bhkShapePhantom {
private:
	uint64_t padding = 0;
	Vector4 aabbMin;
	Vector4 aabbMax;

public:
	static constexpr const char* BlockName = "bhkAabbPhantom";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkAabbPhantom* Clone() { return new bhkAabbPhantom(*this); }
};

class bhkEntity : public bhkWorldObject {
};

class bhkConstraint : public bhkSerializable {
private:
	BlockRefArray<bhkEntity> entityRefs;
	uint priority;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);

	BlockRefArray<bhkEntity>& GetEntities();
};

class bhkHingeConstraint : public bhkConstraint {
private:
	HingeDesc hinge;

public:
	static constexpr const char* BlockName = "bhkHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkHingeConstraint* Clone() { return new bhkHingeConstraint(*this); }
};

class bhkLimitedHingeConstraint : public bhkConstraint {
private:
	LimitedHingeDesc limitedHinge;

public:
	static constexpr const char* BlockName = "bhkLimitedHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkLimitedHingeConstraint* Clone() { return new bhkLimitedHingeConstraint(*this); }
};

class ConstraintData {
private:
	hkConstraintType type;
	BlockRefArray<bhkEntity> entityRefs;
	uint priority = 1;

	BallAndSocketDesc desc1;
	HingeDesc desc2;
	LimitedHingeDesc desc3;
	PrismaticDesc desc4;
	RagdollDesc desc5;
	StiffSpringDesc desc6;

	float strength = 0.0f;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);

	BlockRefArray<bhkEntity>& GetEntities();
};

class bhkBreakableConstraint : public bhkConstraint {
private:
	ConstraintData subConstraint;
	bool removeWhenBroken = false;

public:
	static constexpr const char* BlockName = "bhkBreakableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	bhkBreakableConstraint* Clone() { return new bhkBreakableConstraint(*this); }
};

class bhkRagdollConstraint : public bhkConstraint {
private:
	RagdollDesc ragdoll;

public:
	static constexpr const char* BlockName = "bhkRagdollConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkRagdollConstraint* Clone() { return new bhkRagdollConstraint(*this); }
};

class bhkStiffSpringConstraint : public bhkConstraint {
private:
	StiffSpringDesc stiffSpring;

public:
	static constexpr const char* BlockName = "bhkStiffSpringConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkStiffSpringConstraint* Clone() { return new bhkStiffSpringConstraint(*this); }
};

class bhkPrismaticConstraint : public bhkConstraint {
private:
	PrismaticDesc prismatic;

public:
	static constexpr const char* BlockName = "bhkPrismaticConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkPrismaticConstraint* Clone() { return new bhkPrismaticConstraint(*this); }
};

class bhkMalleableConstraint : public bhkConstraint {
private:
	ConstraintData subConstraint;

public:
	static constexpr const char* BlockName = "bhkMalleableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkMalleableConstraint* Clone() { return new bhkMalleableConstraint(*this); }
};

class bhkBallAndSocketConstraint : public bhkConstraint {
private:
	BallAndSocketDesc ballAndSocket;

public:
	static constexpr const char* BlockName = "bhkBallAndSocketConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkBallAndSocketConstraint* Clone() { return new bhkBallAndSocketConstraint(*this); }
};

class bhkBallSocketConstraintChain : public bhkSerializable {
private:
	uint numPivots = 0;
	std::vector<Vector4> pivots;

	float tau = 1.0f;
	float damping = 0.6f;
	float cfm = 1.1920929e-08f;
	float maxErrorDistance = 0.1f;

	BlockRefArray<bhkEntity> entityARefs;

	uint numEntities = 0;
	BlockRef<bhkEntity> entityARef;
	BlockRef<bhkEntity> entityBRef;
	uint priority = 0;

public:
	static constexpr const char* BlockName = "bhkBallSocketConstraintChain";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	bhkBallSocketConstraintChain* Clone() { return new bhkBallSocketConstraintChain(*this); }

	BlockRefArray<bhkEntity>& GetEntitiesA();

	int GetEntityARef();
	void SetEntityARef(int entityRef);

	int GetEntityBRef();
	void SetEntityBRef(int entityRef);
};

enum hkResponseType : byte {
	RESPONSE_INVALID,
	RESPONSE_SIMPLE_CONTACT,
	RESPONSE_REPORTING,
	RESPONSE_NONE
};

class bhkRigidBody : public bhkEntity {
private:
	hkResponseType collisionResponse = RESPONSE_SIMPLE_CONTACT;
	byte unusedByte1 = 0;
	ushort processContactCallbackDelay = 0xFFFF;
	uint unkInt1 = 0;
	HavokFilter collisionFilterCopy;
	ushort unkShorts2[6];
	Vector4 translation;
	QuaternionXYZW rotation;
	Vector4 linearVelocity;
	Vector4 angularVelocity;
	float inertiaMatrix[12];
	Vector4 center;
	float mass = 1.0f;
	float linearDamping = 0.1f;
	float angularDamping = 0.05f;
	float timeFactor = 1.0f;				// User Version >= 12
	float gravityFactor = 1.0f;				// User Version >= 12
	float friction = 0.5f;
	float rollingFrictionMult = 1.0f;		// User Version >= 12
	float restitution = 0.4f;
	float maxLinearVelocity = 104.4f;
	float maxAngularVelocity = 31.57f;
	float penetrationDepth = 0.15f;
	byte motionSystem = 1;
	byte deactivatorType = 1;
	byte solverDeactivation = 1;
	byte qualityType = 1;
	byte autoRemoveLevel = 0;
	byte responseModifierFlag = 0;
	byte numShapeKeysInContactPointProps = 0;
	bool forceCollideOntoPpu = false;
	uint unkInt2 = 0;
	uint unkInt3 = 0;
	uint unkInt4 = 0;						// User Version >= 12
	BlockRefArray<bhkSerializable> constraintRefs;
	uint unkInt5 = 0;						// User Version <= 11
	ushort unkShort3 = 0;					// User Version >= 12

public:
	static constexpr const char* BlockName = "bhkRigidBody";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkRigidBody* Clone() { return new bhkRigidBody(*this); }

	BlockRefArray<bhkSerializable>& GetConstraints();
};

class bhkRigidBodyT : public bhkRigidBody {
public:
	static constexpr const char* BlockName = "bhkRigidBodyT";
	virtual const char* GetBlockName() { return BlockName; }

	bhkRigidBodyT* Clone() { return new bhkRigidBodyT(*this); }
};

class bhkCompressedMeshShapeData : public bhkRefObject {
private:
	uint bitsPerIndex = 0;
	uint bitsPerWIndex = 0;
	uint maskWIndex = 0;
	uint maskIndex = 0;
	float error = 0.0f;
	Vector4 aabbBoundMin;
	Vector4 aabbBoundMax;
	byte weldingType = 0;
	byte materialType = 0;

	uint numMat32 = 0;
	std::vector<uint> mat32;
	uint numMat16 = 0;
	std::vector<uint> mat16;
	uint numMat8 = 0;
	std::vector<uint> mat8;

	uint numMaterials = 0;
	std::vector<bhkCMSDMaterial> materials;

	uint numNamedMat = 0;

	uint numTransforms = 0;
	std::vector<bhkCMSDTransform> transforms;

	uint numBigVerts = 0;
	std::vector<Vector4> bigVerts;

	uint numBigTris = 0;
	std::vector<bhkCMSDBigTris> bigTris;

	uint numChunks = 0;
	std::vector<bhkCMSDChunk> chunks;

	uint numConvexPieceA = 0;

public:
	static constexpr const char* BlockName = "bhkCompressedMeshShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	bhkCompressedMeshShapeData* Clone() { return new bhkCompressedMeshShapeData(*this); }
};

class bhkCompressedMeshShape : public bhkShape {
private:
	BlockRef<NiAVObject> targetRef;
	uint userData = 0;
	float radius = 0.005f;
	float unkFloat = 0.0f;
	Vector4 scaling = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	float radius2 = 0.005f;
	Vector4 scaling2 = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	BlockRef<bhkCompressedMeshShapeData> dataRef;

public:
	static constexpr const char* BlockName = "bhkCompressedMeshShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
	bhkCompressedMeshShape* Clone() { return new bhkCompressedMeshShape(*this); }
};

struct BoneMatrix {
	Vector3 translation;
	QuaternionXYZW rotation;
	Vector3 scale;
};

struct BonePose {
	uint numMatrices = 0;
	std::vector<BoneMatrix> matrices;

	void Get(NiStream& stream) {
		stream >> numMatrices;
		matrices.resize(numMatrices);
		for (int i = 0; i < numMatrices; i++)
			stream >> matrices[i];
	}

	void Put(NiStream& stream) {
		stream << numMatrices;
		for (int i = 0; i < numMatrices; i++)
			stream << matrices[i];
	}
};

class bhkPoseArray : public NiObject {
private:
	uint numBones = 0;
	std::vector<StringRef> bones;

	uint numPoses = 0;
	std::vector<BonePose> poses;

public:
	static constexpr const char* BlockName = "bhkPoseArray";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	bhkPoseArray* Clone() { return new bhkPoseArray(*this); }
};

class bhkRagdollTemplate : public NiExtraData {
private:
	uint numBones = 0;
	BlockRefArray<NiObject> boneRefs;

public:
	static constexpr const char* BlockName = "bhkRagdollTemplate";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	bhkRagdollTemplate* Clone() { return new bhkRagdollTemplate(*this); }

	BlockRefArray<NiObject>& GetBones();
};

class bhkRagdollTemplateData : public NiObject {
private:
	StringRef name;
	float mass = 9.0f;
	float restitution = 0.8f;
	float friction = 0.3f;
	float radius = 1.0f;
	HavokMaterial material = 7;
	uint numConstraints = 0;
	std::vector<ConstraintData> constraints;

public:
	static constexpr const char* BlockName = "bhkRagdollTemplateData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	bhkRagdollTemplateData* Clone() { return new bhkRagdollTemplateData(*this); }
};
