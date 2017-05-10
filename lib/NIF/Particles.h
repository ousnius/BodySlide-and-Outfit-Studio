/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Geometry.h"

class NiParticlesData : public NiGeometryData {
private:
	bool hasRadii;
	ushort numActive;
	bool hasSizes;
	bool hasRotations;
	bool hasRotationAngles;
	bool hasRotationAxes;
	bool hasTextureIndices;

	uint numSubtexOffsets;
	std::vector<Vector4> subtexOffsets;

	float aspectRatio;
	ushort aspectFlags;
	float speedToAspectAspect2;
	float speedToAspectSpeed1;
	float speedToAspectSpeed2;

public:
	NiParticlesData(NiHeader* hdr);
	NiParticlesData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiParticlesData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiParticlesData* Clone() { return new NiParticlesData(*this); }
};

class NiRotatingParticlesData : public NiParticlesData {
public:
	NiRotatingParticlesData(NiHeader* hdr);
	NiRotatingParticlesData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiRotatingParticlesData";
	virtual const char* GetBlockName() { return BlockName; }

	NiRotatingParticlesData* Clone() { return new NiRotatingParticlesData(*this); }
};

class NiPSysData : public NiRotatingParticlesData {
private:
	bool hasRotationSpeeds;

public:
	NiPSysData(NiHeader* hdr);
	NiPSysData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysData* Clone() { return new NiPSysData(*this); }
};

class NiMeshPSysData : public NiPSysData {
private:
	uint defaultPoolSize;
	bool fillPoolsOnLoad;

	uint numGenerations;
	std::vector<uint> generationPoolSize;

	int nodeRef;

public:
	NiMeshPSysData(NiHeader* hdr);
	NiMeshPSysData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiMeshPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiMeshPSysData* Clone() { return new NiMeshPSysData(*this); }
};

class BSStripPSysData : public NiPSysData {
private:
	ushort maxPointCount;
	uint startCapSize;
	uint endCapSize;
	bool doZPrepass;

public:
	BSStripPSysData(NiHeader* hdr);
	BSStripPSysData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSStripPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSStripPSysData* Clone() { return new BSStripPSysData(*this); }
};

class NiParticleSystem;

class NiPSysModifier : public NiObject {
private:
	StringRef name;
	uint order;
	BlockRef<NiParticleSystem> targetRef;
	bool isActive;

public:
	~NiPSysModifier() {
		name.Clear(header);
	};

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
};

class BSPSysStripUpdateModifier : public NiPSysModifier {
private:
	float updateDeltaTime;

public:
	BSPSysStripUpdateModifier(NiHeader* hdr);
	BSPSysStripUpdateModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysStripUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysStripUpdateModifier* Clone() { return new BSPSysStripUpdateModifier(*this); }
};

class NiPSysSpawnModifier : public NiPSysModifier {
private:
	ushort numSpawnGenerations;
	float percentSpawned;
	ushort minSpawned;
	ushort maxSpawned;
	float spawnSpeedVariation;
	float spawnDirVariation;
	float lifeSpan;
	float lifeSpanVariation;

public:
	NiPSysSpawnModifier(NiHeader* hdr);
	NiPSysSpawnModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysSpawnModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysSpawnModifier* Clone() { return new NiPSysSpawnModifier(*this); }
};

class NiPSysAgeDeathModifier : public NiPSysModifier {
private:
	bool spawnOnDeath;
	BlockRef<NiPSysSpawnModifier> spawnModifierRef;

public:
	NiPSysAgeDeathModifier(NiHeader* hdr);
	NiPSysAgeDeathModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysAgeDeathModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysAgeDeathModifier* Clone() { return new NiPSysAgeDeathModifier(*this); }
};

class BSPSysLODModifier : public NiPSysModifier {
private:
	float lodBeginDistance;
	float lodEndDistance;
	float unknownFadeFactor1;
	float unknownFadeFactor2;

public:
	BSPSysLODModifier(NiHeader* hdr);
	BSPSysLODModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysLODModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysLODModifier* Clone() { return new BSPSysLODModifier(*this); }
};

class BSPSysSimpleColorModifier : public NiPSysModifier {
private:
	float fadeInPercent;
	float fadeOutPercent;
	float color1EndPercent;
	float color2StartPercent;
	float color2EndPercent;
	float color3StartPercent;
	Color4 color1;
	Color4 color2;
	Color4 color3;

public:
	BSPSysSimpleColorModifier(NiHeader* hdr);
	BSPSysSimpleColorModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysSimpleColorModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysSimpleColorModifier* Clone() { return new BSPSysSimpleColorModifier(*this); }
};

class NiPSysRotationModifier : public NiPSysModifier {
private:
	float initialSpeed;
	float initialSpeedVariation;
	float initialAngle;
	float initialAngleVariation;
	bool randomSpeedSign;
	bool randomInitialAxis;
	Vector3 initialAxis;

public:
	NiPSysRotationModifier(NiHeader* hdr);
	NiPSysRotationModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysRotationModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysRotationModifier* Clone() { return new NiPSysRotationModifier(*this); }
};

class BSPSysScaleModifier : public NiPSysModifier {
private:
	uint numFloats;
	std::vector<float> floats;

public:
	BSPSysScaleModifier(NiHeader* hdr);
	BSPSysScaleModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysScaleModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysScaleModifier* Clone() { return new BSPSysScaleModifier(*this); }
};

class NiPSysGravityModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> gravityObjRef;
	Vector3 gravityAxis;
	float decay;
	float strength;
	uint forceType;
	float turbulence;
	float turbulenceScale;
	bool worldAligned;

public:
	NiPSysGravityModifier(NiHeader* hdr);
	NiPSysGravityModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysGravityModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysGravityModifier* Clone() { return new NiPSysGravityModifier(*this); }
};

class NiPSysPositionModifier : public NiPSysModifier {
public:
	NiPSysPositionModifier(NiHeader* hdr);
	NiPSysPositionModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysPositionModifier";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysPositionModifier* Clone() { return new NiPSysPositionModifier(*this); }
};

class NiPSysBoundUpdateModifier : public NiPSysModifier {
private:
	ushort updateSkip;

public:
	NiPSysBoundUpdateModifier(NiHeader* hdr);
	NiPSysBoundUpdateModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysBoundUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysBoundUpdateModifier* Clone() { return new NiPSysBoundUpdateModifier(*this); }
};

class NiPSysDragModifier : public NiPSysModifier {
private:
	BlockRef<NiObject> parentRef;
	Vector3 dragAxis;
	float percentage;
	float range;
	float rangeFalloff;

public:
	NiPSysDragModifier(NiHeader* hdr);
	NiPSysDragModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysDragModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysDragModifier* Clone() { return new NiPSysDragModifier(*this); }
};

class BSPSysInheritVelocityModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> targetNodeRef;
	float changeToInherit;
	float velocityMult;
	float velocityVar;

public:
	BSPSysInheritVelocityModifier(NiHeader* hdr);
	BSPSysInheritVelocityModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysInheritVelocityModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysInheritVelocityModifier* Clone() { return new BSPSysInheritVelocityModifier(*this); }
};

class BSPSysSubTexModifier : public NiPSysModifier {
private:
	float startFrame;
	float startFrameVariation;
	float endFrame;
	float loopStartFrame;
	float loopStartFrameVariation;
	float frameCount;
	float frameCountVariation;

public:
	BSPSysSubTexModifier(NiHeader* hdr);
	BSPSysSubTexModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysSubTexModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysSubTexModifier* Clone() { return new BSPSysSubTexModifier(*this); }
};

class NiPSysBombModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> bombNodeRef;
	Vector3 bombAxis;
	float decay;
	float deltaV;
	uint decayType;
	uint symmetryType;

public:
	NiPSysBombModifier(NiHeader* hdr);
	NiPSysBombModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysBombModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysBombModifier* Clone() { return new NiPSysBombModifier(*this); }
};

class BSWindModifier : public NiPSysModifier {
private:
	float strength;

public:
	BSWindModifier(NiHeader* hdr);
	BSWindModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSWindModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSWindModifier* Clone() { return new BSWindModifier(*this); }
};

class BSPSysRecycleBoundModifier : public NiPSysModifier {
private:
	Vector3 boundOffset;
	Vector3 boundExtent;
	BlockRef<NiNode> targetNodeRef;

public:
	BSPSysRecycleBoundModifier(NiHeader* hdr);
	BSPSysRecycleBoundModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysRecycleBoundModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSPSysRecycleBoundModifier* Clone() { return new BSPSysRecycleBoundModifier(*this); }
};

class BSPSysHavokUpdateModifier : public NiPSysModifier {
private:
	BlockRefArray<NiNode> nodeRefs;
	BlockRef<NiPSysModifier> modifierRef;

public:
	BSPSysHavokUpdateModifier(NiHeader* hdr);
	BSPSysHavokUpdateModifier(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysHavokUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSPSysHavokUpdateModifier* Clone() { return new BSPSysHavokUpdateModifier(*this); }
};

class BSMasterParticleSystem : public NiNode {
private:
	ushort maxEmitterObjs;
	BlockRefArray<NiAVObject> particleSysRefs;

public:
	BSMasterParticleSystem(NiHeader* hdr);
	BSMasterParticleSystem(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMasterParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSMasterParticleSystem* Clone() { return new BSMasterParticleSystem(*this); }
};

class NiParticleSystem : public NiAVObject {
private:
	BlockRef<NiGeometryData> dataRef;
	BlockRef<NiObject> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;
	BlockRef<NiProperty> alphaPropertyRef;

	uint numMaterials = 0;
	std::vector<StringRef> materialNameRefs;
	std::vector<uint> materials;

	uint activeMaterial;
	byte defaultMatNeedsUpdate;

	BoundingSphere bounds;
	byte vertFlags1 = 81;
	byte vertFlags2 = 0;
	byte vertFlags3 = 0;
	byte vertFlags4 = 4;
	byte vertFlags5 = 0;
	byte vertFlags6 = 32;
	byte vertFlags7 = 64;
	byte vertFlags8 = 8;

	ushort farBegin = 0;
	ushort farEnd = 0;
	ushort nearBegin = 0;
	ushort nearEnd = 0;

	BlockRef<NiPSysData> psysDataRef;

	bool isWorldSpace;
	BlockRefArray<NiPSysModifier> modifierRefs;

public:
	~NiParticleSystem() {
		for (auto &m : materialNameRefs) {
			m.Clear(header);
		}
	};
	NiParticleSystem(NiHeader* hdr);
	NiParticleSystem(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiParticleSystem* Clone() { return new NiParticleSystem(*this); }
};

class NiMeshParticleSystem : public NiParticleSystem {
public:
	NiMeshParticleSystem(NiHeader* hdr);
	NiMeshParticleSystem(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiMeshParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	NiMeshParticleSystem* Clone() { return new NiMeshParticleSystem(*this); }
};

class BSStripParticleSystem : public NiParticleSystem {
public:
	BSStripParticleSystem(NiHeader* hdr);
	BSStripParticleSystem(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSStripParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	BSStripParticleSystem* Clone() { return new BSStripParticleSystem(*this); }
};

class NiPSysColliderManager;

class NiPSysCollider : public NiObject {
private:
	float bounce;
	bool spawnOnCollide;
	bool dieOnCollide;
	BlockRef<NiPSysSpawnModifier> spawnModifierRef;
	BlockRef<NiPSysColliderManager> managerRef;
	BlockRef<NiPSysCollider> nextColliderRef;
	BlockRef<NiNode> colliderNodeRef;

public:
	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
};

class NiPSysSphericalCollider : public NiPSysCollider {
private:
	float radius;

public:
	NiPSysSphericalCollider(NiHeader* hdr);
	NiPSysSphericalCollider(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysSphericalCollider";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysSphericalCollider* Clone() { return new NiPSysSphericalCollider(*this); }
};

class NiPSysPlanarCollider : public NiPSysCollider {
private:
	float width;
	float height;
	Vector3 xAxis;
	Vector3 yAxis;

public:
	NiPSysPlanarCollider(NiHeader* hdr);
	NiPSysPlanarCollider(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysPlanarCollider";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysPlanarCollider* Clone() { return new NiPSysPlanarCollider(*this); }
};

class NiPSysColliderManager : public NiPSysModifier {
private:
	BlockRef<NiPSysCollider> colliderRef;

public:
	NiPSysColliderManager(NiHeader* hdr);
	NiPSysColliderManager(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysColliderManager";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysColliderManager* Clone() { return new NiPSysColliderManager(*this); }
};

class NiPSysEmitter : public NiPSysModifier {
private:
	float speed;
	float speedVariation;
	float declination;
	float declinationVariation;
	float planarAngle;
	float planarAngleVariation;
	Color4 color;
	float radius;
	float radiusVariation;
	float lifeSpan;
	float lifeSpanVariation;

public:
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
};

class NiPSysVolumeEmitter : public NiPSysEmitter {
private:
	BlockRef<NiNode> emitterNodeRef;

public:
	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
};

class NiPSysSphereEmitter : public NiPSysVolumeEmitter {
private:
	float radius;

public:
	NiPSysSphereEmitter(NiHeader* hdr);
	NiPSysSphereEmitter(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysSphereEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysSphereEmitter* Clone() { return new NiPSysSphereEmitter(*this); }
};

class NiPSysCylinderEmitter : public NiPSysVolumeEmitter {
private:
	float radius;
	float height;

public:
	NiPSysCylinderEmitter(NiHeader* hdr);
	NiPSysCylinderEmitter(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysCylinderEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysCylinderEmitter* Clone() { return new NiPSysCylinderEmitter(*this); }
};

class NiPSysBoxEmitter : public NiPSysVolumeEmitter {
private:
	float width;
	float height;
	float depth;

public:
	NiPSysBoxEmitter(NiHeader* hdr);
	NiPSysBoxEmitter(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysBoxEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPSysBoxEmitter* Clone() { return new NiPSysBoxEmitter(*this); }
};

class NiPSysMeshEmitter : public NiPSysEmitter {
private:
	BlockRefArray<NiAVObject> meshRefs;
	uint velocityType;
	uint emissionType;
	Vector3 emissionAxis;

public:
	NiPSysMeshEmitter(NiHeader* hdr);
	NiPSysMeshEmitter(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysMeshEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysMeshEmitter* Clone() { return new NiPSysMeshEmitter(*this); }
};
