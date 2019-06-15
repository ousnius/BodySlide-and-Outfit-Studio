/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Geometry.h"
#include "Nodes.h"

class NiParticles : public NiGeometry {
public:
	static constexpr const char* BlockName = "NiParticles";
	virtual const char* GetBlockName() { return BlockName; }

	NiParticles* Clone() { return new NiParticles(*this); }
};

class NiAutoNormalParticles : public NiParticles {
public:
	static constexpr const char* BlockName = "NiAutoNormalParticles";
	virtual const char* GetBlockName() { return BlockName; }

	NiAutoNormalParticles* Clone() { return new NiAutoNormalParticles(*this); }
};

class NiParticleMeshes : public NiParticles {
public:
	static constexpr const char* BlockName = "NiParticleMeshes";
	virtual const char* GetBlockName() { return BlockName; }

	NiParticleMeshes* Clone() { return new NiParticleMeshes(*this); }
};

class NiRotatingParticles : public NiParticles {
public:
	static constexpr const char* BlockName = "NiRotatingParticles";
	virtual const char* GetBlockName() { return BlockName; }

	NiRotatingParticles* Clone() { return new NiRotatingParticles(*this); }
};

class NiParticlesData : public NiGeometryData {
private:
	bool hasRadii = false;
	ushort numActive = 0;
	bool hasSizes = false;
	bool hasRotations = false;
	bool hasRotationAngles = false;
	bool hasRotationAxes = false;
	bool hasTextureIndices = false;

	uint numSubtexOffsets = 0;
	std::vector<Vector4> subtexOffsets;

	float aspectRatio;
	ushort aspectFlags;
	float speedToAspectAspect2;
	float speedToAspectSpeed1;
	float speedToAspectSpeed2;

public:
	NiParticlesData();

	static constexpr const char* BlockName = "NiParticlesData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiParticlesData* Clone() { return new NiParticlesData(*this); }
};

class NiAutoNormalParticlesData : public NiParticlesData {
public:
	static constexpr const char* BlockName = "NiAutoNormalParticlesData";
	virtual const char* GetBlockName() { return BlockName; }

	NiAutoNormalParticlesData* Clone() { return new NiAutoNormalParticlesData(*this); }
};

class NiRotatingParticlesData : public NiParticlesData {
public:
	static constexpr const char* BlockName = "NiRotatingParticlesData";
	virtual const char* GetBlockName() { return BlockName; }

	NiRotatingParticlesData* Clone() { return new NiRotatingParticlesData(*this); }
};

class NiParticleMeshesData : public NiRotatingParticlesData {
private:
	BlockRef<NiAVObject> dataRef;

public:
	static constexpr const char* BlockName = "NiParticleMeshesData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	NiParticleMeshesData* Clone() { return new NiParticleMeshesData(*this); }
};

class NiPSysData : public NiRotatingParticlesData {
private:
	bool hasRotationSpeeds = false;

public:
	static constexpr const char* BlockName = "NiPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysData* Clone() { return new NiPSysData(*this); }
};

class NiMeshPSysData : public NiPSysData {
private:
	uint defaultPoolSize = 0;
	bool fillPoolsOnLoad = false;

	uint numGenerations = 0;
	std::vector<uint> generationPoolSize;

	BlockRef<NiNode> nodeRef;

public:
	static constexpr const char* BlockName = "NiMeshPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiMeshPSysData* Clone() { return new NiMeshPSysData(*this); }
};

class BSStripPSysData : public NiPSysData {
private:
	ushort maxPointCount = 0;
	uint startCapSize = 0;
	uint endCapSize = 0;
	bool doZPrepass = false;

public:
	static constexpr const char* BlockName = "BSStripPSysData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSStripPSysData* Clone() { return new BSStripPSysData(*this); }
};

class NiPSysEmitterCtlrData : public NiObject {
private:
	KeyGroup<float> floatKeys;
	uint numVisibilityKeys = 0;
	std::vector<Key<byte>> visibilityKeys;

public:
	static constexpr const char* BlockName = "NiPSysEmitterCtlrData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysEmitterCtlrData* Clone() { return new NiPSysEmitterCtlrData(*this); }
};

class NiParticleSystem;

class NiPSysModifier : public NiObject {
private:
	StringRef name;
	uint order;
	BlockRef<NiParticleSystem> targetRef;
	bool isActive;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
};

class BSPSysStripUpdateModifier : public NiPSysModifier {
private:
	float updateDeltaTime = 0.0f;

public:
	static constexpr const char* BlockName = "BSPSysStripUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPSysStripUpdateModifier* Clone() { return new BSPSysStripUpdateModifier(*this); }
};

class NiPSysSpawnModifier : public NiPSysModifier {
private:
	ushort numSpawnGenerations = 0;
	float percentSpawned = 0.0f;
	ushort minSpawned = 0;
	ushort maxSpawned = 0;
	float spawnSpeedVariation = 0.0f;
	float spawnDirVariation = 0.0f;
	float lifeSpan = 0.0f;
	float lifeSpanVariation = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysSpawnModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysSpawnModifier* Clone() { return new NiPSysSpawnModifier(*this); }
};

class NiPSysAgeDeathModifier : public NiPSysModifier {
private:
	bool spawnOnDeath = false;
	BlockRef<NiPSysSpawnModifier> spawnModifierRef;

public:
	static constexpr const char* BlockName = "NiPSysAgeDeathModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPSysAgeDeathModifier* Clone() { return new NiPSysAgeDeathModifier(*this); }
};

class BSPSysLODModifier : public NiPSysModifier {
private:
	float lodBeginDistance = 0.0f;
	float lodEndDistance = 0.0f;
	float unknownFadeFactor1 = 0.0f;
	float unknownFadeFactor2 = 0.0f;

public:
	static constexpr const char* BlockName = "BSPSysLODModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPSysLODModifier* Clone() { return new BSPSysLODModifier(*this); }
};

class BSPSysSimpleColorModifier : public NiPSysModifier {
private:
	float fadeInPercent = 0.0f;
	float fadeOutPercent = 0.0f;
	float color1EndPercent = 0.0f;
	float color2StartPercent = 0.0f;
	float color2EndPercent = 0.0f;
	float color3StartPercent = 0.0f;
	Color4 color1;
	Color4 color2;
	Color4 color3;

public:
	static constexpr const char* BlockName = "BSPSysSimpleColorModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPSysSimpleColorModifier* Clone() { return new BSPSysSimpleColorModifier(*this); }
};

class NiPSysRotationModifier : public NiPSysModifier {
private:
	float initialSpeed = 0.0f;
	float initialSpeedVariation = 0.0f;
	float initialAngle = 0.0f;
	float initialAngleVariation = 0.0f;
	bool randomSpeedSign = false;
	bool randomInitialAxis = false;
	Vector3 initialAxis;

public:
	static constexpr const char* BlockName = "NiPSysRotationModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysRotationModifier* Clone() { return new NiPSysRotationModifier(*this); }
};

class BSPSysScaleModifier : public NiPSysModifier {
private:
	uint numFloats = 0;
	std::vector<float> floats;

public:
	static constexpr const char* BlockName = "BSPSysScaleModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPSysScaleModifier* Clone() { return new BSPSysScaleModifier(*this); }
};

enum ForceType : uint {
	FORCE_PLANAR,
	FORCE_SPHERICAL,
	FORCE_UNKNOWN
};

class NiPSysGravityModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> gravityObjRef;
	Vector3 gravityAxis;
	float decay = 0.0f;
	float strength = 0.0f;
	ForceType forceType = FORCE_UNKNOWN;
	float turbulence = 0.0f;
	float turbulenceScale = 1.0f;
	bool worldAligned = false;

public:
	static constexpr const char* BlockName = "NiPSysGravityModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiPSysGravityModifier* Clone() { return new NiPSysGravityModifier(*this); }
};

class NiPSysPositionModifier : public NiPSysModifier {
public:
	static constexpr const char* BlockName = "NiPSysPositionModifier";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysPositionModifier* Clone() { return new NiPSysPositionModifier(*this); }
};

class NiPSysBoundUpdateModifier : public NiPSysModifier {
private:
	ushort updateSkip = 0;

public:
	static constexpr const char* BlockName = "NiPSysBoundUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysBoundUpdateModifier* Clone() { return new NiPSysBoundUpdateModifier(*this); }
};

class NiPSysDragModifier : public NiPSysModifier {
private:
	BlockRef<NiObject> parentRef;
	Vector3 dragAxis;
	float percentage = 0.0f;
	float range = 0.0f;
	float rangeFalloff = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysDragModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiPSysDragModifier* Clone() { return new NiPSysDragModifier(*this); }
};

class BSPSysInheritVelocityModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> targetNodeRef;
	float changeToInherit = 0.0f;
	float velocityMult = 0.0f;
	float velocityVar = 0.0f;

public:
	static constexpr const char* BlockName = "BSPSysInheritVelocityModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	BSPSysInheritVelocityModifier* Clone() { return new BSPSysInheritVelocityModifier(*this); }
};

class BSPSysSubTexModifier : public NiPSysModifier {
private:
	float startFrame = 0.0f;
	float startFrameVariation = 0.0f;
	float endFrame = 0.0f;
	float loopStartFrame = 0.0f;
	float loopStartFrameVariation = 0.0f;
	float frameCount = 0.0f;
	float frameCountVariation = 0.0f;

public:
	static constexpr const char* BlockName = "BSPSysSubTexModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPSysSubTexModifier* Clone() { return new BSPSysSubTexModifier(*this); }
};

enum DecayType : uint {
	DECAY_NONE,
	DECAY_LINEAR,
	DECAY_EXPONENTIAL
};

enum SymmetryType : uint {
	SYMMETRY_SPHERICAL,
	SYMMETRY_CYLINDRICAL,
	SYMMETRY_PLANAR
};

class NiPSysBombModifier : public NiPSysModifier {
private:
	BlockRef<NiNode> bombNodeRef;
	Vector3 bombAxis;
	float decay = 0.0f;
	float deltaV = 0.0f;
	DecayType decayType = DECAY_NONE;
	SymmetryType symmetryType = SYMMETRY_SPHERICAL;

public:
	static constexpr const char* BlockName = "NiPSysBombModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiPSysBombModifier* Clone() { return new NiPSysBombModifier(*this); }
};

class NiColorData : public NiObject {
private:
	KeyGroup<Color4> data;

public:
	static constexpr const char* BlockName = "NiColorData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiColorData* Clone() { return new NiColorData(*this); }
};

class NiPSysColorModifier : public NiPSysModifier {
private:
	BlockRef<NiColorData> dataRef;

public:
	static constexpr const char* BlockName = "NiPSysColorModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPSysColorModifier* Clone() { return new NiPSysColorModifier(*this); }
};

class NiPSysGrowFadeModifier : public NiPSysModifier {
private:
	float growTime = 0.0f;
	ushort growGeneration = 0;
	float fadeTime = 0.0f;
	ushort fadeGeneration = 0;
	float baseScale = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysGrowFadeModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysGrowFadeModifier* Clone() { return new NiPSysGrowFadeModifier(*this); }
};

class NiPSysMeshUpdateModifier : public NiPSysModifier {
private:
	BlockRefArray<NiAVObject> meshRefs;

public:
	static constexpr const char* BlockName = "NiPSysMeshUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPSysMeshUpdateModifier* Clone() { return new NiPSysMeshUpdateModifier(*this); }

	BlockRefArray<NiAVObject>& GetMeshes();
};

class NiPSysFieldModifier : public NiPSysModifier {
private:
	BlockRef<NiAVObject> fieldObjectRef;
	float magnitude = 0.0f;
	float attenuation = 0.0f;
	bool useMaxDistance = false;
	float maxDistance = 0.0f;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
};

class NiPSysVortexFieldModifier : public NiPSysFieldModifier {
private:
	Vector3 direction;

public:
	static constexpr const char* BlockName = "NiPSysVortexFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysVortexFieldModifier* Clone() { return new NiPSysVortexFieldModifier(*this); }
};

class NiPSysGravityFieldModifier : public NiPSysFieldModifier {
private:
	Vector3 direction;

public:
	static constexpr const char* BlockName = "NiPSysGravityFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysGravityFieldModifier* Clone() { return new NiPSysGravityFieldModifier(*this); }
};

class NiPSysDragFieldModifier : public NiPSysFieldModifier {
private:
	bool useDirection = false;
	Vector3 direction;

public:
	static constexpr const char* BlockName = "NiPSysDragFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysDragFieldModifier* Clone() { return new NiPSysDragFieldModifier(*this); }
};

class NiPSysTurbulenceFieldModifier : public NiPSysFieldModifier {
private:
	float frequency = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysTurbulenceFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysTurbulenceFieldModifier* Clone() { return new NiPSysTurbulenceFieldModifier(*this); }
};

class NiPSysAirFieldModifier : public NiPSysFieldModifier {
private:
	Vector3 direction;
	float unkFloat1 = 0.0f;
	float unkFloat2 = 0.0f;
	bool unkBool1 = false;
	bool unkBool2 = false;
	bool unkBool3 = false;
	float unkFloat3 = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysAirFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysAirFieldModifier* Clone() { return new NiPSysAirFieldModifier(*this); }
};

class NiPSysRadialFieldModifier : public NiPSysFieldModifier {
private:
	uint radialType = 0;

public:
	static constexpr const char* BlockName = "NiPSysRadialFieldModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysRadialFieldModifier* Clone() { return new NiPSysRadialFieldModifier(*this); }
};

class BSWindModifier : public NiPSysModifier {
private:
	float strength = 0.0f;

public:
	static constexpr const char* BlockName = "BSWindModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSWindModifier* Clone() { return new BSWindModifier(*this); }
};

class BSPSysRecycleBoundModifier : public NiPSysModifier {
private:
	Vector3 boundOffset;
	Vector3 boundExtent;
	BlockRef<NiNode> targetNodeRef;

public:
	static constexpr const char* BlockName = "BSPSysRecycleBoundModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	BSPSysRecycleBoundModifier* Clone() { return new BSPSysRecycleBoundModifier(*this); }
};

class BSPSysHavokUpdateModifier : public NiPSysModifier {
private:
	BlockRefArray<NiNode> nodeRefs;
	BlockRef<NiPSysModifier> modifierRef;

public:
	static constexpr const char* BlockName = "BSPSysHavokUpdateModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	BSPSysHavokUpdateModifier* Clone() { return new BSPSysHavokUpdateModifier(*this); }

	BlockRefArray<NiNode>& GetNodes();

	int GetModifierRef();
	void SetModifierRef(int modRef);
};

class BSParentVelocityModifier : public NiPSysModifier {
private:
	float damping = 0.0f;

public:
	static constexpr const char* BlockName = "BSParentVelocityModifier";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSParentVelocityModifier* Clone() { return new BSParentVelocityModifier(*this); }
};

class BSMasterParticleSystem : public NiNode {
private:
	ushort maxEmitterObjs = 0;
	BlockRefArray<NiAVObject> particleSysRefs;

public:
	static constexpr const char* BlockName = "BSMasterParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<Ref*>& refs);
	BSMasterParticleSystem* Clone() { return new BSMasterParticleSystem(*this); }

	BlockRefArray<NiAVObject>& GetParticleSystems();
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
	static constexpr const char* BlockName = "NiParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	NiParticleSystem* Clone() { return new NiParticleSystem(*this); }

	int GetDataRef();
	void SetDataRef(int datRef);

	int GetSkinInstanceRef();
	void SetSkinInstanceRef(int skinRef);

	int GetShaderPropertyRef();
	void SetShaderPropertyRef(int shaderRef);

	int GetAlphaPropertyRef();
	void SetAlphaPropertyRef(int alphaRef);

	int GetPSysDataRef();
	void SetPSysDataRef(int psysDatRef);

	BlockRefArray<NiPSysModifier>& GetModifiers();
};

class NiMeshParticleSystem : public NiParticleSystem {
public:
	static constexpr const char* BlockName = "NiMeshParticleSystem";
	virtual const char* GetBlockName() { return BlockName; }

	NiMeshParticleSystem* Clone() { return new NiMeshParticleSystem(*this); }
};

class BSStripParticleSystem : public NiParticleSystem {
public:
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
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
};

class NiPSysSphericalCollider : public NiPSysCollider {
private:
	float radius = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysSphericalCollider";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysSphericalCollider* Clone() { return new NiPSysSphericalCollider(*this); }
};

class NiPSysPlanarCollider : public NiPSysCollider {
private:
	float width = 0.0f;
	float height = 0.0f;
	Vector3 xAxis;
	Vector3 yAxis;

public:
	static constexpr const char* BlockName = "NiPSysPlanarCollider";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysPlanarCollider* Clone() { return new NiPSysPlanarCollider(*this); }
};

class NiPSysColliderManager : public NiPSysModifier {
private:
	BlockRef<NiPSysCollider> colliderRef;

public:
	static constexpr const char* BlockName = "NiPSysColliderManager";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
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
	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class NiPSysVolumeEmitter : public NiPSysEmitter {
private:
	BlockRef<NiNode> emitterNodeRef;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
};

class NiPSysSphereEmitter : public NiPSysVolumeEmitter {
private:
	float radius = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysSphereEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysSphereEmitter* Clone() { return new NiPSysSphereEmitter(*this); }
};

class NiPSysCylinderEmitter : public NiPSysVolumeEmitter {
private:
	float radius = 0.0f;
	float height = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysCylinderEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysCylinderEmitter* Clone() { return new NiPSysCylinderEmitter(*this); }
};

class NiPSysBoxEmitter : public NiPSysVolumeEmitter {
private:
	float width = 0.0f;
	float height = 0.0f;
	float depth = 0.0f;

public:
	static constexpr const char* BlockName = "NiPSysBoxEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPSysBoxEmitter* Clone() { return new NiPSysBoxEmitter(*this); }
};

class BSPSysArrayEmitter : public NiPSysVolumeEmitter {
public:
	static constexpr const char* BlockName = "BSPSysArrayEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	BSPSysArrayEmitter* Clone() { return new BSPSysArrayEmitter(*this); }
};

enum VelocityType : uint {
	VELOCITY_USE_NORMALS,
	VELOCITY_USE_RANDOM,
	VELOCITY_USE_DIRECTION
};

enum EmitFrom : uint {
	EMIT_FROM_VERTICES,
	EMIT_FROM_FACE_CENTER,
	EMIT_FROM_EDGE_CENTER,
	EMIT_FROM_FACE_SURFACE,
	EMIT_FROM_EDGE_SURFACE
};

class NiPSysMeshEmitter : public NiPSysEmitter {
private:
	BlockRefArray<NiAVObject> meshRefs;
	VelocityType velocityType = VELOCITY_USE_NORMALS;
	EmitFrom emissionType = EMIT_FROM_VERTICES;
	Vector3 emissionAxis;

public:
	static constexpr const char* BlockName = "NiPSysMeshEmitter";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPSysMeshEmitter* Clone() { return new NiPSysMeshEmitter(*this); }

	BlockRefArray<NiAVObject>& GetMeshes();
};
