/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "ExtraData.h"

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
		}
	}

	int CalcMotorSize() {
		switch (motorType) {
		case MOTOR_POSITION:
			return 25;
		case MOTOR_VELOCITY:
			return 18;
		case MOTOR_SPRING:
			return 17;
		default:
			return 0;
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
	HavokMaterial material;
	HavokFilter layer;
};

struct bhkCMSDBigTris {
	ushort triangle1;
	ushort triangle2;
	ushort triangle3;
	uint material;
	ushort weldingInfo;
};

struct bhkCMSDTransform {
	Vector4 translation;
	Quaternion rotation;
};

struct bhkCMSDChunk {
	Vector4 translation;
	uint matIndex;
	ushort reference;
	ushort transformIndex;

	uint numVerts;
	std::vector<ushort> verts;
	uint numIndices;
	std::vector<ushort> indices;
	uint numStrips;
	std::vector<ushort> strips;
	uint numWeldingInfo;
	std::vector<ushort> weldingInfo;
};

class NiAVObject;

class NiCollisionObject : public NiObject {
private:
	BlockRef<NiAVObject> targetRef;

public:
	NiCollisionObject() {}
	NiCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "NiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	int CalcBlockSize(NiVersion& version);
	NiCollisionObject* Clone() { return new NiCollisionObject(*this); }
};

class bhkNiCollisionObject : public NiCollisionObject {
private:
	ushort flags = 1;
	BlockRef<NiObject> bodyRef;

public:
	bhkNiCollisionObject();
	bhkNiCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkNiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkNiCollisionObject* Clone() { return new bhkNiCollisionObject(*this); }
};

class bhkCollisionObject : public bhkNiCollisionObject {
public:
	bhkCollisionObject();
	bhkCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkCollisionObject* Clone() { return new bhkCollisionObject(*this); }
};

class bhkNPCollisionObject : public bhkCollisionObject {
private:
	uint bodyID = 0;

public:
	bhkNPCollisionObject();
	bhkNPCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkNPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkNPCollisionObject* Clone() { return new bhkNPCollisionObject(*this); }
};

class bhkPCollisionObject : public bhkNiCollisionObject {
public:
	bhkPCollisionObject();
	bhkPCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkPCollisionObject* Clone() { return new bhkPCollisionObject(*this); }
};

class bhkSPCollisionObject : public bhkPCollisionObject {
public:
	bhkSPCollisionObject();
	bhkSPCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkSPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSPCollisionObject* Clone() { return new bhkSPCollisionObject(*this); }
};

class bhkBlendCollisionObject : public bhkCollisionObject {
private:
	float heirGain = 0.0f;
	float velGain = 0.0f;

public:
	bhkBlendCollisionObject();
	bhkBlendCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "bhkBlendCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkBlendCollisionObject* Clone() { return new bhkBlendCollisionObject(*this); }
};

class bhkPhysicsSystem : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	bhkPhysicsSystem(const uint size = 0);
	bhkPhysicsSystem(NiStream& stream);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkPhysicsSystem* Clone() { return new bhkPhysicsSystem(*this); }
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
	bhkPlaneShape() {}
	bhkPlaneShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkPlaneShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkPlaneShape* Clone() { return new bhkPlaneShape(*this); }
};

class bhkSphereRepShape : public bhkShape {
private:
	HavokMaterial material;
	float radius;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
};

class bhkConvexShape : public bhkSphereRepShape {
};

class bhkConvexVerticesShape : public bhkConvexShape {
private:
	uint mat = 0;
	float radius = 0.0f;
	hkWorldObjCInfoProperty vertsProp;
	hkWorldObjCInfoProperty normalsProp;

	uint numVerts = 0;
	std::vector<Vector4> verts;

	uint numNormals = 0;
	std::vector<Vector4> normals;

public:
	bhkConvexVerticesShape() {}
	bhkConvexVerticesShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkConvexVerticesShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkConvexVerticesShape* Clone() { return new bhkConvexVerticesShape(*this); }
};

class bhkBoxShape : public bhkConvexShape {
private:
	uint64_t padding = 0;
	Vector3 dimensions;
	float radius2 = 0.0f;

public:
	bhkBoxShape() {}
	bhkBoxShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkBoxShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkBoxShape* Clone() { return new bhkBoxShape(*this); }
};

class bhkSphereShape : public bhkConvexShape {
public:
	bhkSphereShape() {}
	bhkSphereShape(NiStream& stream);

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
	bhkTransformShape() {}
	bhkTransformShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkTransformShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkTransformShape* Clone() { return new bhkTransformShape(*this); }
};

class bhkConvexTransformShape : public bhkTransformShape {
public:
	bhkConvexTransformShape();
	bhkConvexTransformShape(NiStream& stream);

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
	bhkCapsuleShape() {}
	bhkCapsuleShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkCapsuleShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
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
	bhkMoppBvTreeShape() {}
	bhkMoppBvTreeShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkMoppBvTreeShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
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
	bhkNiTriStripsShape() {}
	bhkNiTriStripsShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkNiTriStripsShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkNiTriStripsShape* Clone() { return new bhkNiTriStripsShape(*this); }
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
	bhkListShape() {}
	bhkListShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkListShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkListShape* Clone() { return new bhkListShape(*this); }
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
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
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
	bhkSimpleShapePhantom() {}
	bhkSimpleShapePhantom(NiStream& stream);

	static constexpr const char* BlockName = "bhkSimpleShapePhantom";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkSimpleShapePhantom* Clone() { return new bhkSimpleShapePhantom(*this); }
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
	void GetPtrs(std::set<int*>& ptrs);
	int CalcBlockSize(NiVersion& version);
};

class bhkHingeConstraint : public bhkConstraint {
private:
	HingeDesc hinge;

public:
	bhkHingeConstraint() {}
	bhkHingeConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkHingeConstraint* Clone() { return new bhkHingeConstraint(*this); }
};

class bhkLimitedHingeConstraint : public bhkConstraint {
private:
	LimitedHingeDesc limitedHinge;

public:
	bhkLimitedHingeConstraint() {}
	bhkLimitedHingeConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkLimitedHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkLimitedHingeConstraint* Clone() { return new bhkLimitedHingeConstraint(*this); }
};

class SubConstraintDesc {
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
	void GetPtrs(std::set<int*>& ptrs);
	int CalcDescSize();
};

class bhkBreakableConstraint : public bhkConstraint {
private:
	SubConstraintDesc subConstraint;
	bool removeWhenBroken = false;

public:
	bhkBreakableConstraint() {}
	bhkBreakableConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkBreakableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	int CalcBlockSize(NiVersion& version);
	bhkBreakableConstraint* Clone() { return new bhkBreakableConstraint(*this); }
};

class bhkRagdollConstraint : public bhkConstraint {
private:
	RagdollDesc ragdoll;

public:
	bhkRagdollConstraint() {}
	bhkRagdollConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkRagdollConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkRagdollConstraint* Clone() { return new bhkRagdollConstraint(*this); }
};

class bhkStiffSpringConstraint : public bhkConstraint {
private:
	StiffSpringDesc stiffSpring;

public:
	bhkStiffSpringConstraint() {}
	bhkStiffSpringConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkStiffSpringConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkStiffSpringConstraint* Clone() { return new bhkStiffSpringConstraint(*this); }
};

class bhkPrismaticConstraint : public bhkConstraint {
private:
	PrismaticDesc prismatic;

public:
	bhkPrismaticConstraint() {}
	bhkPrismaticConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkPrismaticConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkPrismaticConstraint* Clone() { return new bhkPrismaticConstraint(*this); }
};

class bhkMalleableConstraint : public bhkConstraint {
private:
	SubConstraintDesc subConstraint;

public:
	bhkMalleableConstraint() {}
	bhkMalleableConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkMalleableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkMalleableConstraint* Clone() { return new bhkMalleableConstraint(*this); }
};

class bhkBallAndSocketConstraint : public bhkConstraint {
private:
	BallAndSocketDesc ballAndSocket;

public:
	bhkBallAndSocketConstraint() {}
	bhkBallAndSocketConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkBallAndSocketConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
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
	bhkBallSocketConstraintChain() {}
	bhkBallSocketConstraintChain(NiStream& stream);

	static constexpr const char* BlockName = "bhkBallSocketConstraintChain";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	int CalcBlockSize(NiVersion& version);
	bhkBallSocketConstraintChain* Clone() { return new bhkBallSocketConstraintChain(*this); }
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
	Quaternion rotation;
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
	bhkRigidBody() {}
	bhkRigidBody(NiStream& stream);

	static constexpr const char* BlockName = "bhkRigidBody";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkRigidBody* Clone() { return new bhkRigidBody(*this); }
};

class bhkRigidBodyT : public bhkRigidBody {
public:
	bhkRigidBodyT();
	bhkRigidBodyT(NiStream& stream);

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
	bhkCompressedMeshShapeData() {}
	bhkCompressedMeshShapeData(NiStream& stream);

	static constexpr const char* BlockName = "bhkCompressedMeshShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
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
	Vector4 scaling2 = Vector4(1.0f, 1.0f, 1.0f, 1.0f);;
	BlockRef<bhkCompressedMeshShapeData> dataRef;

public:
	bhkCompressedMeshShape() {}
	bhkCompressedMeshShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkCompressedMeshShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	void GetPtrs(std::set<int*>& ptrs);
	int CalcBlockSize(NiVersion& version);
	bhkCompressedMeshShape* Clone() { return new bhkCompressedMeshShape(*this); }
};
