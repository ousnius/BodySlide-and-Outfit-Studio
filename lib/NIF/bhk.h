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
	float unkFloat[6];
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
	NiCollisionObject(NiHeader* hdr);
	NiCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiCollisionObject* Clone() { return new NiCollisionObject(*this); }
};

class bhkNiCollisionObject : public NiCollisionObject {
private:
	ushort flags = 1;
	BlockRef<NiObject> bodyRef;

public:
	bhkNiCollisionObject(NiHeader* hdr);
	bhkNiCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkNiCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkNiCollisionObject* Clone() { return new bhkNiCollisionObject(*this); }
};

class bhkCollisionObject : public bhkNiCollisionObject {
public:
	bhkCollisionObject(NiHeader* hdr);
	bhkCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkCollisionObject* Clone() { return new bhkCollisionObject(*this); }
};

class bhkNPCollisionObject : public bhkCollisionObject {
private:
	uint bodyID = 0;

public:
	bhkNPCollisionObject(NiHeader* hdr);
	bhkNPCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkNPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkNPCollisionObject* Clone() { return new bhkNPCollisionObject(*this); }
};

class bhkPCollisionObject : public bhkNiCollisionObject {
public:
	bhkPCollisionObject(NiHeader* hdr);
	bhkPCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkPCollisionObject* Clone() { return new bhkPCollisionObject(*this); }
};

class bhkSPCollisionObject : public bhkPCollisionObject {
public:
	bhkSPCollisionObject(NiHeader* hdr);
	bhkSPCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkSPCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSPCollisionObject* Clone() { return new bhkSPCollisionObject(*this); }
};

class bhkBlendCollisionObject : public bhkCollisionObject {
private:
	float heirGain;
	float velGain;

public:
	bhkBlendCollisionObject(NiHeader* hdr);
	bhkBlendCollisionObject(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkBlendCollisionObject";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkBlendCollisionObject* Clone() { return new bhkBlendCollisionObject(*this); }
};

class bhkPhysicsSystem : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	bhkPhysicsSystem(NiHeader* hdr, const uint size = 0);
	bhkPhysicsSystem(std::fstream& file, NiHeader* hdr);

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	bhkPlaneShape(NiHeader* hdr);
	bhkPlaneShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkPlaneShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkPlaneShape* Clone() { return new bhkPlaneShape(*this); }
};

class bhkSphereRepShape : public bhkShape {
private:
	HavokMaterial material;
	float radius;

public:
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
};

class bhkConvexShape : public bhkSphereRepShape {
};

class bhkConvexVerticesShape : public bhkConvexShape {
private:
	uint mat;
	float radius;
	hkWorldObjCInfoProperty vertsProp;
	hkWorldObjCInfoProperty normalsProp;

	uint numVerts;
	std::vector<Vector4> verts;

	uint numNormals;
	std::vector<Vector4> normals;

public:
	bhkConvexVerticesShape(NiHeader* hdr);
	bhkConvexVerticesShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkConvexVerticesShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkConvexVerticesShape* Clone() { return new bhkConvexVerticesShape(*this); }
};

class bhkBoxShape : public bhkConvexShape {
private:
	byte padding[8];
	Vector3 dimensions;
	float radius2;

public:
	bhkBoxShape(NiHeader* hdr);
	bhkBoxShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkBoxShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkBoxShape* Clone() { return new bhkBoxShape(*this); }
};

class bhkSphereShape : public bhkConvexShape {
public:
	bhkSphereShape(NiHeader* hdr);
	bhkSphereShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkSphereShape";
	virtual const char* GetBlockName() { return BlockName; }

	bhkSphereShape* Clone() { return new bhkSphereShape(*this); }
};

class bhkTransformShape : public bhkShape {
private:
	BlockRef<bhkShape> shapeRef;
	HavokMaterial material;
	float radius;
	byte padding[8];
	Matrix4 xform;

public:
	bhkTransformShape(NiHeader* hdr);
	bhkTransformShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkTransformShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkTransformShape* Clone() { return new bhkTransformShape(*this); }
};

class bhkConvexTransformShape : public bhkTransformShape {
public:
	bhkConvexTransformShape(NiHeader* hdr);
	bhkConvexTransformShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkConvexTransformShape";
	virtual const char* GetBlockName() { return BlockName; }

	bhkConvexTransformShape* Clone() { return new bhkConvexTransformShape(*this); }
};

class bhkCapsuleShape : public bhkConvexShape {
private:
	byte padding[8];
	Vector3 point1;
	float radius1;
	Vector3 point2;
	float radius2;

public:
	bhkCapsuleShape(NiHeader* hdr);
	bhkCapsuleShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkCapsuleShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	bhkMoppBvTreeShape(NiHeader* hdr);
	bhkMoppBvTreeShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkMoppBvTreeShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	bhkNiTriStripsShape(NiHeader* hdr);
	bhkNiTriStripsShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkNiTriStripsShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	bhkListShape(NiHeader* hdr);
	bhkListShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkListShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkWorldObject* Clone() { return new bhkWorldObject(*this); }
};

class bhkPhantom : public bhkWorldObject {
};

class bhkShapePhantom : public bhkPhantom {
};

class bhkSimpleShapePhantom : public bhkShapePhantom {
private:
	byte padding[8];
	Matrix4 transform;

public:
	bhkSimpleShapePhantom(NiHeader* hdr);
	bhkSimpleShapePhantom(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkSimpleShapePhantom";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkSimpleShapePhantom* Clone() { return new bhkSimpleShapePhantom(*this); }
};

class bhkEntity : public bhkWorldObject {
};

class bhkConstraint : public bhkSerializable {
private:
	BlockRefArray<bhkEntity> entityRefs;
	uint priority;

public:
	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
};

class bhkHingeConstraint : public bhkConstraint {
private:
	HingeDesc hinge;

public:
	bhkHingeConstraint(NiHeader* hdr);
	bhkHingeConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkHingeConstraint* Clone() { return new bhkHingeConstraint(*this); }
};

class bhkLimitedHingeConstraint : public bhkConstraint {
private:
	LimitedHingeDesc limitedHinge;

public:
	bhkLimitedHingeConstraint(NiHeader* hdr);
	bhkLimitedHingeConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkLimitedHingeConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	void Put(std::fstream& file);
	void Get(std::fstream& file, NiObject* parent);
	void GetChildRefs(std::set<int*>& refs);
	int CalcDescSize();
};

class bhkBreakableConstraint : public bhkConstraint {
private:
	SubConstraintDesc subConstraint;
	float threshold;
	bool removeIfBroken;

public:
	bhkBreakableConstraint(NiHeader* hdr);
	bhkBreakableConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkBreakableConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkBreakableConstraint* Clone() { return new bhkBreakableConstraint(*this); }
};

class bhkRagdollConstraint : public bhkConstraint {
private:
	RagdollDesc ragdoll;

public:
	bhkRagdollConstraint(NiHeader* hdr);
	bhkRagdollConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkRagdollConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkRagdollConstraint* Clone() { return new bhkRagdollConstraint(*this); }
};

class bhkStiffSpringConstraint : public bhkConstraint {
private:
	StiffSpringDesc stiffSpring;

public:
	bhkStiffSpringConstraint(NiHeader* hdr);
	bhkStiffSpringConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkStiffSpringConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	bhkStiffSpringConstraint* Clone() { return new bhkStiffSpringConstraint(*this); }
};

class bhkBallAndSocketConstraint : public bhkConstraint {
private:
	BallAndSocketDesc ballAndSocket;

public:
	bhkBallAndSocketConstraint(NiHeader* hdr);
	bhkBallAndSocketConstraint(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkBallAndSocketConstraint";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	bhkBallSocketConstraintChain(NiHeader* hdr);
	bhkBallSocketConstraintChain(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkBallSocketConstraintChain";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	bhkRigidBody(NiHeader* hdr);
	bhkRigidBody(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkRigidBody";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkRigidBody* Clone() { return new bhkRigidBody(*this); }
};

class bhkRigidBodyT : public bhkRigidBody {
public:
	bhkRigidBodyT(NiHeader* hdr);
	bhkRigidBodyT(std::fstream& file, NiHeader* hdr);

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

	uint numMat32;
	std::vector<uint> mat32;
	uint numMat16;
	std::vector<uint> mat16;
	uint numMat8;
	std::vector<uint> mat8;

	uint numMaterials;
	std::vector<bhkCMSDMaterial> materials;

	uint numNamedMat;

	uint numTransforms;
	std::vector<bhkCMSDTransform> transforms;

	uint numBigVerts;
	std::vector<Vector4> bigVerts;

	uint numBigTris;
	std::vector<bhkCMSDBigTris> bigTris;

	uint numChunks;
	std::vector<bhkCMSDChunk> chunks;

	uint numConvexPieceA;

public:
	bhkCompressedMeshShapeData(NiHeader* hdr);
	bhkCompressedMeshShapeData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkCompressedMeshShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	bhkCompressedMeshShape(NiHeader* hdr);
	bhkCompressedMeshShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "bhkCompressedMeshShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	bhkCompressedMeshShape* Clone() { return new bhkCompressedMeshShape(*this); }
};
