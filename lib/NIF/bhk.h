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
	byte layer;
	byte flagsAndParts;
	ushort group;
};

struct hkWorldObjCInfoProperty {
	uint data;
	uint size;
	uint capacityAndFlags;
};

struct MotorDesc {
	float unkFloat1;
	float unkFloat2;
	float unkFloat3;
	float unkFloat4;
	float unkFloat5;
	float unkFloat6;
	byte unkByte;
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
	float minAngle;
	float maxAngle;
	float maxFriction;
	bool enableMotor;
	MotorDesc motor;
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
	float coneMaxAngle;
	float planeMinAngle;
	float planeMaxAngle;
	float twistMinAngle;
	float twistMaxAngle;
	float maxFriction;
	bool enableMotor;
	MotorDesc motor;
};

struct StiffSpringDesc {
	Vector4 pivotA;
	Vector4 pivotB;
	float length;
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
	float minDistance;
	float maxDistance;
	float friction;
	byte unkByte;
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
	NiCollisionObject();
	NiCollisionObject(NiStream& stream);

	static constexpr const char* BlockName = "NiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
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
	float heirGain;
	float velGain;

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
	HavokMaterial material;
	Vector3 unkVec;
	Vector3 direction;
	float constant;
	Vector4 halfExtents;
	Vector4 center;

public:
	bhkPlaneShape();
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
	uint mat;
	float radius;
	hkWorldObjCInfoProperty vertsProp;
	hkWorldObjCInfoProperty normalsProp;

	uint numVerts = 0;
	std::vector<Vector4> verts;

	uint numNormals = 0;
	std::vector<Vector4> normals;

public:
	bhkConvexVerticesShape();
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
	uint64_t padding;
	Vector3 dimensions;
	float radius2;

public:
	bhkBoxShape();
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
	bhkSphereShape();
	bhkSphereShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkSphereShape";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSphereShape* Clone() { return new bhkSphereShape(*this); }
};

class bhkTransformShape : public bhkShape {
private:
	BlockRef<bhkShape> shapeRef;
	HavokMaterial material;
	float radius;
	uint64_t padding;
	Matrix4 xform;

public:
	bhkTransformShape();
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
	uint64_t padding;
	Vector3 point1;
	float radius1;
	Vector3 point2;
	float radius2;

public:
	bhkCapsuleShape();
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
	uint userData;
	uint shapeCollection;
	uint code;
	float scale;
	uint dataSize = 0;
	Vector4 offset;
	byte buildType;							// User Version >= 12
	std::vector<byte> data;

public:
	bhkMoppBvTreeShape();
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
	HavokMaterial material;
	float radius;
	uint unused1;
	uint unused2;
	uint unused3;
	uint unused4;
	uint unused5;
	uint growBy;
	Vector4 scale;

	BlockRefArray<NiTriStripsData> partRefs;

	uint numFilters = 0;
	std::vector<uint> filters;

public:
	bhkNiTriStripsShape();
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
	HavokMaterial material;
	hkWorldObjCInfoProperty childShapeProp;
	hkWorldObjCInfoProperty childFilterProp;
	uint numUnkInts = 0;
	std::vector<uint> unkInts;

public:
	bhkListShape();
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
	void Init();
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
	uint64_t padding;
	Matrix4 transform;

public:
	bhkSimpleShapePhantom();
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
	void Init();
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
};

class bhkHingeConstraint : public bhkConstraint {
private:
	HingeDesc hinge;

public:
	bhkHingeConstraint();
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
	bhkLimitedHingeConstraint();
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
	uint priority;

	BallAndSocketDesc desc1;
	HingeDesc desc2;
	LimitedHingeDesc desc3;
	PrismaticDesc desc4;
	RagdollDesc desc5;
	StiffSpringDesc desc6;

public:
	void Get(NiStream& stream, NiObject* parent);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcDescSize();
};

class bhkBreakableConstraint : public bhkConstraint {
private:
	SubConstraintDesc subConstraint;
	float threshold;
	bool removeIfBroken;

public:
	bhkBreakableConstraint();
	bhkBreakableConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkBreakableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkBreakableConstraint* Clone() { return new bhkBreakableConstraint(*this); }
};

class bhkRagdollConstraint : public bhkConstraint {
private:
	RagdollDesc ragdoll;

public:
	bhkRagdollConstraint();
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
	bhkStiffSpringConstraint();
	bhkStiffSpringConstraint(NiStream& stream);

	static constexpr const char* BlockName = "bhkStiffSpringConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	bhkStiffSpringConstraint* Clone() { return new bhkStiffSpringConstraint(*this); }
};

class bhkBallAndSocketConstraint : public bhkConstraint {
private:
	BallAndSocketDesc ballAndSocket;

public:
	bhkBallAndSocketConstraint();
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

	float tau;
	float damping;
	float cfm;
	float maxErrorDistance;

	BlockRefArray<bhkEntity> entityARefs;

	uint numEntities = 0;
	BlockRef<bhkEntity> entityARef;
	BlockRef<bhkEntity> entityBRef;
	uint priority;

public:
	bhkBallSocketConstraintChain();
	bhkBallSocketConstraintChain(NiStream& stream);

	static constexpr const char* BlockName = "bhkBallSocketConstraintChain";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkBallSocketConstraintChain* Clone() { return new bhkBallSocketConstraintChain(*this); }
};

class bhkRigidBody : public bhkEntity {
private:
	byte responseType;
	byte unkByte;
	ushort processContactCallbackDelay;
	ushort unkShorts[2];
	HavokFilter collisionFilterCopy;
	ushort unkShorts2[6];
	Vector4 translation;
	Quaternion rotation;
	Vector4 linearVelocity;
	Vector4 angularVelocity;
	float inertiaMatrix[12];
	Vector4 center;
	float mass;
	float linearDamping;
	float angularDamping;
	float timeFactor;						// User Version >= 12
	float gravityFactor;					// User Version >= 12
	float friction;
	float rollingFrictionMult;				// User Version >= 12
	float restitution;
	float maxLinearVelocity;
	float maxAngularVelocity;
	float penetrationDepth;
	byte motionSystem;
	byte deactivatorType;
	byte solverDeactivation;
	byte qualityType;
	byte autoRemoveLevel;
	byte responseModifierFlag;
	byte numShapeKeysInContactPointProps;
	bool forceCollideOntoPpu;
	uint unkInt2;
	uint unkInt3;
	uint unkInt4;							// User Version >= 12
	BlockRefArray<bhkSerializable> constraintRefs;
	uint unkInt5;							// User Version <= 11
	ushort unkShort3;						// User Version >= 12

public:
	bhkRigidBody();
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
	uint bitsPerIndex;
	uint bitsPerWIndex;
	uint maskWIndex;
	uint maskIndex;
	float error;
	Vector4 aabbBoundMin;
	Vector4 aabbBoundMax;
	byte weldingType;
	byte materialType;

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
	bhkCompressedMeshShapeData();
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
	uint userData;
	float radius;
	float unkFloat;
	Vector4 scaling;
	float radius2;
	Vector4 scaling2;
	BlockRef<bhkCompressedMeshShapeData> dataRef;

public:
	bhkCompressedMeshShape();
	bhkCompressedMeshShape(NiStream& stream);

	static constexpr const char* BlockName = "bhkCompressedMeshShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	bhkCompressedMeshShape* Clone() { return new bhkCompressedMeshShape(*this); }
};
