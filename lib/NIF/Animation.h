/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "ExtraData.h"
#include "Keys.h"

class NiKeyframeData : public NiObject {
private:
	uint numRotationKeys = 0;
	KeyType rotationType = NO_INTERP;
	std::vector<Key<Quaternion>> quaternionKeys;
	KeyGroup<float> xRotations;
	KeyGroup<float> yRotations;
	KeyGroup<float> zRotations;
	KeyGroup<Vector3> translations;
	KeyGroup<float> scales;

public:
	NiKeyframeData() {}
	NiKeyframeData(NiStream& stream);

	static constexpr const char* BlockName = "NiKeyframeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiKeyframeData* Clone() { return new NiKeyframeData(*this); }
};

class NiTransformData : public NiKeyframeData {
public:
	NiTransformData();
	NiTransformData(NiStream& stream);

	static constexpr const char* BlockName = "NiTransformData";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformData* Clone() { return new NiTransformData(*this); }
};

class NiPosData : public NiObject {
private:
	KeyGroup<Vector3> data;

public:
	NiPosData() {}
	NiPosData(NiStream& stream);

	static constexpr const char* BlockName = "NiPosData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiPosData* Clone() { return new NiPosData(*this); }
};

class NiBoolData : public NiObject {
private:
	KeyGroup<byte> data;

public:
	NiBoolData() {}
	NiBoolData(NiStream& stream);

	static constexpr const char* BlockName = "NiBoolData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBoolData* Clone() { return new NiBoolData(*this); }
};

class NiFloatData : public NiObject {
private:
	KeyGroup<float> data;

public:
	NiFloatData() {}
	NiFloatData(NiStream& stream);

	static constexpr const char* BlockName = "NiFloatData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiFloatData* Clone() { return new NiFloatData(*this); }
};

class NiBSplineData : public NiObject {
private:
	uint numFloatControlPoints = 0;
	std::vector<float> floatControlPoints;

	uint numShortControlPoints = 0;
	std::vector<short> shortControlPoints;

public:
	NiBSplineData() {}
	NiBSplineData(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBSplineData* Clone() { return new NiBSplineData(*this); }
};

class NiBSplineBasisData : public NiObject {
private:
	uint numControlPoints = 0;

public:
	NiBSplineBasisData() {}
	NiBSplineBasisData(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineBasisData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBSplineBasisData* Clone() { return new NiBSplineBasisData(*this); }
};

class NiInterpolator : public NiObject {
};

class NiBSplineInterpolator : public NiInterpolator {
private:
	float startTime = 0.0f;
	float stopTime = 0.0f;
	BlockRef<NiBSplineData> splineDataRef;
	BlockRef<NiBSplineBasisData> basisDataRef;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
};

class NiBSplineFloatInterpolator : public NiBSplineInterpolator {
};

class NiBSplineCompFloatInterpolator : public NiBSplineFloatInterpolator {
private:
	float base = 0.0f;
	uint offset = 0;
	float bias = 0.0f;
	float multiplier = 0.0f;

public:
	NiBSplineCompFloatInterpolator() {}
	NiBSplineCompFloatInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineCompFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBSplineCompFloatInterpolator* Clone() { return new NiBSplineCompFloatInterpolator(*this); }
};

class NiBSplinePoint3Interpolator : public NiBSplineInterpolator {
private:
	float unkFloat1 = 0.0f;
	float unkFloat2 = 0.0f;
	float unkFloat3 = 0.0f;
	float unkFloat4 = 0.0f;
	float unkFloat5 = 0.0f;
	float unkFloat6 = 0.0f;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class NiBSplineCompPoint3Interpolator : public NiBSplinePoint3Interpolator {
public:
	NiBSplineCompPoint3Interpolator() {}
	NiBSplineCompPoint3Interpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineCompPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBSplineCompPoint3Interpolator* Clone() { return new NiBSplineCompPoint3Interpolator(*this); }
};

class NiBSplineTransformInterpolator : public NiBSplineInterpolator {
private:
	Vector3 translation;
	Quaternion rotation;
	float scale = 1.0f;

	uint translationOffset = 0;
	uint rotationOffset = 0;
	uint scaleOffset = 0;

public:
	NiBSplineTransformInterpolator() {}
	NiBSplineTransformInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBSplineTransformInterpolator* Clone() { return new NiBSplineTransformInterpolator(*this); }
};

class NiBSplineCompTransformInterpolator : public NiBSplineTransformInterpolator {
private:
	float translationBias = 0.0f;
	float translationMultiplier = 0.0f;
	float rotationBias = 0.0f;
	float rotationMultiplier = 0.0f;
	float scaleBias = 0.0f;
	float scaleMultiplier = 0.0f;

public:
	NiBSplineCompTransformInterpolator() {}
	NiBSplineCompTransformInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBSplineCompTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBSplineCompTransformInterpolator* Clone() { return new NiBSplineCompTransformInterpolator(*this); }
};

class NiBlendInterpolator : public NiInterpolator {
private:
	ushort flags;
	uint unkInt;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class NiBlendBoolInterpolator : public NiBlendInterpolator {
private:
	bool value = false;

public:
	NiBlendBoolInterpolator() {}
	NiBlendBoolInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBlendBoolInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBlendBoolInterpolator* Clone() { return new NiBlendBoolInterpolator(*this); }
};

class NiBlendFloatInterpolator : public NiBlendInterpolator {
private:
	float value = 0.0f;

public:
	NiBlendFloatInterpolator() {}
	NiBlendFloatInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBlendFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBlendFloatInterpolator* Clone() { return new NiBlendFloatInterpolator(*this); }
};

class NiBlendPoint3Interpolator : public NiBlendInterpolator {
private:
	Vector3 point;

public:
	NiBlendPoint3Interpolator() {}
	NiBlendPoint3Interpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBlendPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBlendPoint3Interpolator* Clone() { return new NiBlendPoint3Interpolator(*this); }
};

class NiBlendTransformInterpolator : public NiBlendInterpolator {
public:
	NiBlendTransformInterpolator() {}
	NiBlendTransformInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBlendTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBlendTransformInterpolator* Clone() { return new NiBlendTransformInterpolator(*this); }
};

class NiKeyBasedInterpolator : public NiInterpolator {
};

class NiBoolInterpolator : public NiKeyBasedInterpolator {
private:
	byte boolValue = 0;

public:
	BlockRef<NiBoolData> dataRef;

	NiBoolInterpolator() {}
	NiBoolInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBoolInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiBoolInterpolator* Clone() { return new NiBoolInterpolator(*this); }
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {
public:
	NiBoolTimelineInterpolator();
	NiBoolTimelineInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiBoolTimelineInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBoolTimelineInterpolator* Clone() { return new NiBoolTimelineInterpolator(*this); }
};

class NiFloatInterpolator : public NiKeyBasedInterpolator {
private:
	float floatValue = 0.0f;

public:
	BlockRef<NiFloatData> dataRef;

	NiFloatInterpolator() {}
	NiFloatInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiFloatInterpolator* Clone() { return new NiFloatInterpolator(*this); }
};

class NiTransformInterpolator : public NiKeyBasedInterpolator {
private:
	Vector3 translation;
	Quaternion rotation;
	float scale = 0.0f;

public:
	BlockRef<NiTransformData> dataRef;

	NiTransformInterpolator() {}
	NiTransformInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiTransformInterpolator* Clone() { return new NiTransformInterpolator(*this); }
};

class BSRotAccumTransfInterpolator : public NiTransformInterpolator {
public:
	BSRotAccumTransfInterpolator() {}
	BSRotAccumTransfInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "BSRotAccumTransfInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	BSRotAccumTransfInterpolator* Clone() { return new BSRotAccumTransfInterpolator(*this); }
};

class NiPoint3Interpolator : public NiKeyBasedInterpolator {
private:
	Vector3 point3Value;

public:
	BlockRef<NiPosData> dataRef;

	NiPoint3Interpolator() {}
	NiPoint3Interpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiPoint3Interpolator* Clone() { return new NiPoint3Interpolator(*this); }
};

class NiPathInterpolator : public NiKeyBasedInterpolator {
private:
	ushort flags = 0;
	uint bankDir = 0;
	float maxBankAngle = 0.0f;
	float smoothing = 0.0f;
	ushort followAxis = 0;

	BlockRef<NiPosData> pathDataRef;
	BlockRef<NiFloatData> percentDataRef;

public:
	NiPathInterpolator() {}
	NiPathInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiPathInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiPathInterpolator* Clone() { return new NiPathInterpolator(*this); }
};

enum LookAtFlags : ushort {
	LOOK_X_AXIS = 0x0000,
	LOOK_FLIP = 0x0001,
	LOOK_Y_AXIS = 0x0002,
	LOOK_Z_AXIS = 0x0004
};

class NiNode;

class NiLookAtInterpolator : public NiInterpolator {
private:
	LookAtFlags flags = LOOK_X_AXIS;
	BlockRef<NiNode> lookAtRef;

	StringRef lookAtName;

	QuatTransform transform;
	BlockRef<NiPoint3Interpolator> translateInterpRef;
	BlockRef<NiFloatInterpolator> rollInterpRef;
	BlockRef<NiFloatInterpolator> scaleInterpRef;

public:
	NiLookAtInterpolator() {}
	NiLookAtInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "NiLookAtInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);
	void GetPtrs(std::set<int*>& ptrs);
	NiLookAtInterpolator* Clone() { return new NiLookAtInterpolator(*this); }
};

struct BSTreadTransformData {
	Vector3 translation;
	Quaternion rotation;
	float scale = 1.0f;
};

struct BSTreadTransform {
	StringRef name;
	BSTreadTransformData transform1;
	BSTreadTransformData transform2;

	void Get(NiStream& stream) {
		name.Get(stream);
		stream >> transform1;
		stream >> transform2;
	}

	void Put(NiStream& stream) {
		name.Put(stream);
		stream << transform1;
		stream << transform2;
	}

	void GetStringRefs(std::set<StringRef*>& refs) {
		refs.insert(&name);
	}
};

class BSTreadTransfInterpolator : public NiInterpolator {
private:
	uint numTreadTransforms = 0;
	std::vector<BSTreadTransform> treadTransforms;
	BlockRef<NiFloatData> dataRef;

public:
	BSTreadTransfInterpolator() {}
	BSTreadTransfInterpolator(NiStream& stream);

	static constexpr const char* BlockName = "BSTreadTransfInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);
	BSTreadTransfInterpolator* Clone() { return new BSTreadTransfInterpolator(*this); }
};

class NiObjectNET;

class NiTimeController : public NiObject {
private:
	ushort flags = 0x000C;
	float frequency = 1.0f;
	float phase = 0.0f;
	float startTime = 0.0f;
	float stopTime = 0.0f;

public:
	BlockRef<NiTimeController> nextControllerRef;
	BlockRef<NiObjectNET> targetRef;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	void GetPtrs(std::set<int*>& ptrs);
};

class NiLookAtController : public NiTimeController {
private:
	ushort unkShort1 = 0;
	BlockRef<NiNode> lookAtNodePtr;

public:
	NiLookAtController() {}
	NiLookAtController(NiStream& stream);

	static constexpr const char* BlockName = "NiLookAtController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	NiLookAtController* Clone() { return new NiLookAtController(*this); }
};

class NiPathController : public NiTimeController {
private:
	ushort unkShort1 = 0;
	uint unkInt1 = 1;
	float unkFloat1 = 0.0f;
	float unkFloat2 = 0.0f;
	ushort unkShort2 = 0;
	BlockRef<NiPosData> posDataRef;
	BlockRef<NiFloatData> floatDataRef;

public:
	NiPathController() {}
	NiPathController(NiStream& stream);

	static constexpr const char* BlockName = "NiPathController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiPathController* Clone() { return new NiPathController(*this); }
};

class NiPSysResetOnLoopCtlr : public NiTimeController {
public:
	NiPSysResetOnLoopCtlr() {}
	NiPSysResetOnLoopCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysResetOnLoopCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysResetOnLoopCtlr* Clone() { return new NiPSysResetOnLoopCtlr(*this); }
};

class NiUVData : public NiObject {
private:
	KeyGroup<float> uTrans;
	KeyGroup<float> vTrans;
	KeyGroup<float> uScale;
	KeyGroup<float> vScale;

public:
	NiUVData() {}
	NiUVData(NiStream& stream);

	static constexpr const char* BlockName = "NiUVData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiUVData* Clone() { return new NiUVData(*this); }
};

class NiUVController : public NiTimeController {
private:
	ushort unkShort1 = 0;
	BlockRef<NiUVData> dataRef;

public:
	NiUVController() {}
	NiUVController(NiStream& stream);

	static constexpr const char* BlockName = "NiUVController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiUVController* Clone() { return new NiUVController(*this); }
};

class BSRefractionFirePeriodController : public NiTimeController {
private:
	BlockRef<NiInterpolator> interpRef;

public:
	BSRefractionFirePeriodController() {}
	BSRefractionFirePeriodController(NiStream& stream);

	static constexpr const char* BlockName = "BSRefractionFirePeriodController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	BSRefractionFirePeriodController* Clone() { return new BSRefractionFirePeriodController(*this); }
};

class BSFrustumFOVController : public NiTimeController {
public:
	BlockRef<NiInterpolator> interpolatorRef;

	BSFrustumFOVController() {}
	BSFrustumFOVController(NiStream& stream);

	static constexpr const char* BlockName = "BSFrustumFOVController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	BSFrustumFOVController* Clone() { return new BSFrustumFOVController(*this); }
};

class BSLagBoneController : public NiTimeController {
private:
	float linearVelocity = 0.0f;
	float linearRotation = 0.0f;
	float maxDistance = 0.0f;

public:
	BSLagBoneController() {}
	BSLagBoneController(NiStream& stream);

	static constexpr const char* BlockName = "BSLagBoneController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLagBoneController* Clone() { return new BSLagBoneController(*this); }
};

class BSShaderProperty;

class BSProceduralLightningController : public NiTimeController {
private:
	ushort subdivisions = 0;
	ushort numBranches = 0;
	ushort numBranchesPerVariation = 0;

	float length = 0.0f;
	float lengthVariation = 0.0f;
	float width = 0.0f;
	float childWidthMult = 0.0f;
	float arcOffset = 0.0f;
	bool fadeMainBolt = 0.0f;
	bool fadeChildBolts = 0.0f;
	bool animateArcOffset = 0.0f;

	BlockRef<BSShaderProperty> shaderPropertyRef;

public:
	BlockRef<NiInterpolator> generationInterpRef;
	BlockRef<NiInterpolator> mutationInterpRef;
	BlockRef<NiInterpolator> subdivisionInterpRef;
	BlockRef<NiInterpolator> numBranchesInterpRef;
	BlockRef<NiInterpolator> numBranchesVarInterpRef;
	BlockRef<NiInterpolator> lengthInterpRef;
	BlockRef<NiInterpolator> lengthVarInterpRef;
	BlockRef<NiInterpolator> widthInterpRef;
	BlockRef<NiInterpolator> arcOffsetInterpRef;

	BSProceduralLightningController() {}
	BSProceduralLightningController(NiStream& stream);

	static constexpr const char* BlockName = "BSProceduralLightningController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	BSProceduralLightningController* Clone() { return new BSProceduralLightningController(*this); }
};

class NiBoneLODController : public NiTimeController {
private:
	uint lod = 0;
	uint numLODs = 0;
	uint boneArraysSize = 0;
	std::vector<BlockRefArray<NiNode>> boneArrays;

public:
	NiBoneLODController() {}
	NiBoneLODController(NiStream& stream);

	static constexpr const char* BlockName = "NiBoneLODController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	NiBoneLODController* Clone() { return new NiBoneLODController(*this); }
};

class NiBSBoneLODController : public NiBoneLODController {
public:
	NiBSBoneLODController() {}
	NiBSBoneLODController(NiStream& stream);

	static constexpr const char* BlockName = "NiBSBoneLODController";
	virtual const char* GetBlockName() { return BlockName; }

	NiBSBoneLODController* Clone() { return new NiBSBoneLODController(*this); }
};

struct Morph {
	StringRef frameName;
	std::vector<Vector3> vectors;

	void Get(NiStream& stream, uint numVerts) {
		frameName.Get(stream);
		vectors.resize(numVerts);
		for (int i = 0; i < numVerts; i++)
			stream >> vectors[i];
	}

	void Put(NiStream& stream, uint numVerts) {
		frameName.Put(stream);
		for (int i = 0; i < numVerts; i++)
			stream << vectors[i];
	}

	void GetStringRefs(std::set<StringRef*>& refs) {
		refs.insert(&frameName);
	}
};

class NiMorphData : public NiObject {
private:
	uint numMorphs = 0;
	uint numVertices = 0;
	byte relativeTargets = 1;
	std::vector<Morph> morphs;

public:
	NiMorphData() {}
	NiMorphData(NiStream& stream);

	static constexpr const char* BlockName = "NiMorphData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);

	NiMorphData* Clone() { return new NiMorphData(*this); }
};

class NiInterpController : public NiTimeController {
};

struct MorphWeight {
	BlockRef<NiInterpolator> interpRef;
	float weight = 0.0f;

	void Get(NiStream& stream) {
		interpRef.Get(stream);
		stream >> weight;
	}

	void Put(NiStream& stream) {
		interpRef.Put(stream);
		stream << weight;
	}

	void GetChildRefs(std::set<int*>& refs) {
		refs.insert(&interpRef.index);
	}
};

class NiGeomMorpherController : public NiInterpController {
private:
	ushort extraFlags = 0;
	BlockRef<NiMorphData> dataRef;
	bool alwaysUpdate = false;

	uint numTargets = 0;
	std::vector<MorphWeight> interpWeights;

public:
	NiGeomMorpherController() {}
	NiGeomMorpherController(NiStream& stream);

	static constexpr const char* BlockName = "NiGeomMorpherController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);

	NiGeomMorpherController* Clone() { return new NiGeomMorpherController(*this); }
};

class NiSingleInterpController : public NiInterpController {
public:
	BlockRef<NiInterpController> interpolatorRef;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
};

class NiRollController : public NiSingleInterpController {
private:
	BlockRef<NiFloatData> dataRef;

public:
	NiRollController() {}
	NiRollController(NiStream& stream);

	static constexpr const char* BlockName = "NiRollController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);

	NiRollController* Clone() { return new NiRollController(*this); }
};

enum TargetColor : ushort {
	TC_AMBIENT,
	TC_DIFFUSE,
	TC_SPECULAR,
	TC_SELF_ILLUM
};

class NiPoint3InterpController : public NiSingleInterpController {
private:
	TargetColor targetColor = TC_AMBIENT;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class NiMaterialColorController : public NiPoint3InterpController {
public:
	NiMaterialColorController() {}
	NiMaterialColorController(NiStream& stream);

	static constexpr const char* BlockName = "NiMaterialColorController";
	virtual const char* GetBlockName() { return BlockName; }

	NiMaterialColorController* Clone() { return new NiMaterialColorController(*this); }
};

class NiLightColorController : public NiPoint3InterpController {
public:
	NiLightColorController() {}
	NiLightColorController(NiStream& stream);

	static constexpr const char* BlockName = "NiLightColorController";
	virtual const char* GetBlockName() { return BlockName; }

	NiLightColorController* Clone() { return new NiLightColorController(*this); }
};

class NiExtraDataController : public NiSingleInterpController {
};

class NiFloatExtraDataController : public NiExtraDataController {
private:
	StringRef extraData;

public:
	NiFloatExtraDataController() {}
	NiFloatExtraDataController(NiStream& stream);

	static constexpr const char* BlockName = "NiFloatExtraDataController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	NiFloatExtraDataController* Clone() { return new NiFloatExtraDataController(*this); }
};

class NiBoolInterpController : public NiSingleInterpController {
};

class NiVisController : public NiBoolInterpController {
public:
	NiVisController() {}
	NiVisController(NiStream& stream);

	static constexpr const char* BlockName = "NiVisController";
	virtual const char* GetBlockName() { return BlockName; }

	NiVisController* Clone() { return new NiVisController(*this); }
};

enum TexType : uint {
	BASE_MAP,
	DARK_MAP,
	DETAIL_MAP,
	GLOSS_MAP,
	GLOW_MAP,
	BUMP_MAP,
	NORMAL_MAP,
	UNKNOWN2_MAP,
	DECAL_0_MAP,
	DECAL_1_MAP,
	DECAL_2_MAP,
	DECAL_3_MAP
};

class NiFloatInterpController : public NiSingleInterpController {
};

class NiSourceTexture;

class NiFlipController : public NiFloatInterpController {
private:
	TexType textureSlot = BASE_MAP;
	BlockRefArray<NiSourceTexture> sourceRefs;

public:
	NiFlipController() {}
	NiFlipController(NiStream& stream);

	static constexpr const char* BlockName = "NiFlipController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);

	NiFlipController* Clone() { return new NiFlipController(*this); }
};

enum TexTransform : uint {
	TT_TRANSLATE_U,
	TT_TRANSLATE_V,
	TT_ROTATE,
	TT_SCALE_U,
	TT_SCALE_V
};

class NiTextureTransformController : public NiFloatInterpController {
private:
	byte unkByte1 = 0;
	TexType textureSlot = BASE_MAP;
	TexTransform operation = TT_TRANSLATE_U;

public:
	NiTextureTransformController() {}
	NiTextureTransformController(NiStream& stream);

	static constexpr const char* BlockName = "NiTextureTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiTextureTransformController* Clone() { return new NiTextureTransformController(*this); }
};

class NiLightDimmerController : public NiFloatInterpController {
public:
	NiLightDimmerController() {}
	NiLightDimmerController(NiStream& stream);

	static constexpr const char* BlockName = "NiLightDimmerController";
	virtual const char* GetBlockName() { return BlockName; }

	NiLightDimmerController* Clone() { return new NiLightDimmerController(*this); }
};

class NiLightRadiusController : public NiFloatInterpController {
public:
	NiLightRadiusController() {}
	NiLightRadiusController(NiStream& stream);

	static constexpr const char* BlockName = "NiLightRadiusController";
	virtual const char* GetBlockName() { return BlockName; }

	NiLightRadiusController* Clone() { return new NiLightRadiusController(*this); }
};

class NiAlphaController : public NiFloatInterpController {
public:
	NiAlphaController() {}
	NiAlphaController(NiStream& stream);

	static constexpr const char* BlockName = "NiAlphaController";
	virtual const char* GetBlockName() { return BlockName; }

	NiAlphaController* Clone() { return new NiAlphaController(*this); }
};

class NiPSysUpdateCtlr : public NiTimeController {
public:
	NiPSysUpdateCtlr() {}
	NiPSysUpdateCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysUpdateCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysUpdateCtlr* Clone() { return new NiPSysUpdateCtlr(*this); }
};

class BSNiAlphaPropertyTestRefController : public NiAlphaController {
public:
	BSNiAlphaPropertyTestRefController();
	BSNiAlphaPropertyTestRefController(NiStream& stream);

	static constexpr const char* BlockName = "BSNiAlphaPropertyTestRefController";
	virtual const char* GetBlockName() { return BlockName; }

	BSNiAlphaPropertyTestRefController* Clone() { return new BSNiAlphaPropertyTestRefController(*this); }
};

class NiKeyframeController : public NiSingleInterpController {
public:
	NiKeyframeController() {}
	NiKeyframeController(NiStream& stream);

	static constexpr const char* BlockName = "NiKeyframeController";
	virtual const char* GetBlockName() { return BlockName; }

	NiKeyframeController* Clone() { return new NiKeyframeController(*this); }
};

class NiTransformController : public NiKeyframeController {
public:
	NiTransformController();
	NiTransformController(NiStream& stream);

	static constexpr const char* BlockName = "NiTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformController* Clone() { return new NiTransformController(*this); }
};

class BSMaterialEmittanceMultController : public NiFloatInterpController {
public:
	BSMaterialEmittanceMultController() {}
	BSMaterialEmittanceMultController(NiStream& stream);

	static constexpr const char* BlockName = "BSMaterialEmittanceMultController";
	virtual const char* GetBlockName() { return BlockName; }

	BSMaterialEmittanceMultController* Clone() { return new BSMaterialEmittanceMultController(*this); }
};

class BSRefractionStrengthController : public NiFloatInterpController {
public:
	BSRefractionStrengthController() {}
	BSRefractionStrengthController(NiStream& stream);

	static constexpr const char* BlockName = "BSRefractionStrengthController";
	virtual const char* GetBlockName() { return BlockName; }

	BSRefractionStrengthController* Clone() { return new BSRefractionStrengthController(*this); }
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor = 0;

public:
	BSLightingShaderPropertyColorController() {}
	BSLightingShaderPropertyColorController(NiStream& stream);

	static constexpr const char* BlockName = "BSLightingShaderPropertyColorController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLightingShaderPropertyColorController* Clone() { return new BSLightingShaderPropertyColorController(*this); }
};

class BSLightingShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable = 0;

public:
	BSLightingShaderPropertyFloatController() {}
	BSLightingShaderPropertyFloatController(NiStream& stream);

	static constexpr const char* BlockName = "BSLightingShaderPropertyFloatController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLightingShaderPropertyFloatController* Clone() { return new BSLightingShaderPropertyFloatController(*this); }
};

class BSLightingShaderPropertyUShortController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable = 0;

public:
	BSLightingShaderPropertyUShortController() {}
	BSLightingShaderPropertyUShortController(NiStream& stream);

	static constexpr const char* BlockName = "BSLightingShaderPropertyUShortController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLightingShaderPropertyUShortController* Clone() { return new BSLightingShaderPropertyUShortController(*this); }
};

class BSEffectShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor = 0;

public:
	BSEffectShaderPropertyColorController() {}
	BSEffectShaderPropertyColorController(NiStream& stream);

	static constexpr const char* BlockName = "BSEffectShaderPropertyColorController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSEffectShaderPropertyColorController* Clone() { return new BSEffectShaderPropertyColorController(*this); }
};

class BSEffectShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable = 0;

public:
	BSEffectShaderPropertyFloatController() {}
	BSEffectShaderPropertyFloatController(NiStream& stream);

	static constexpr const char* BlockName = "BSEffectShaderPropertyFloatController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSEffectShaderPropertyFloatController* Clone() { return new BSEffectShaderPropertyFloatController(*this); }
};

class NiAVObject;

class NiMultiTargetTransformController : public NiInterpController {
private:
	BlockRefShortArray<NiAVObject> targetRefs;

public:
	NiMultiTargetTransformController() {}
	NiMultiTargetTransformController(NiStream& stream);

	static constexpr const char* BlockName = "NiMultiTargetTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	NiMultiTargetTransformController* Clone() { return new NiMultiTargetTransformController(*this); }
};

class NiPSysModifierCtlr : public NiSingleInterpController {
private:
	StringRef modifierName;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
};

class NiPSysModifierBoolCtlr : public NiPSysModifierCtlr {
};

class NiPSysModifierActiveCtlr : public NiPSysModifierBoolCtlr {
public:
	NiPSysModifierActiveCtlr() {}
	NiPSysModifierActiveCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysModifierActiveCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysModifierActiveCtlr* Clone() { return new NiPSysModifierActiveCtlr(*this); }
};

class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr {
};

class NiPSysEmitterLifeSpanCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterLifeSpanCtlr() {}
	NiPSysEmitterLifeSpanCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterLifeSpanCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterLifeSpanCtlr* Clone() { return new NiPSysEmitterLifeSpanCtlr(*this); }
};

class NiPSysEmitterSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterSpeedCtlr() {}
	NiPSysEmitterSpeedCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterSpeedCtlr* Clone() { return new NiPSysEmitterSpeedCtlr(*this); }
};

class NiPSysEmitterInitialRadiusCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterInitialRadiusCtlr() {}
	NiPSysEmitterInitialRadiusCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterInitialRadiusCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterInitialRadiusCtlr* Clone() { return new NiPSysEmitterInitialRadiusCtlr(*this); }
};

class NiPSysEmitterDeclinationCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterDeclinationCtlr() {}
	NiPSysEmitterDeclinationCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterDeclinationCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterDeclinationCtlr* Clone() { return new NiPSysEmitterDeclinationCtlr(*this); }
};

class NiPSysGravityStrengthCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysGravityStrengthCtlr() {}
	NiPSysGravityStrengthCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysGravityStrengthCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysGravityStrengthCtlr* Clone() { return new NiPSysGravityStrengthCtlr(*this); }
};

class NiPSysEmitterDeclinationVarCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterDeclinationVarCtlr() {}
	NiPSysEmitterDeclinationVarCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterDeclinationVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterDeclinationVarCtlr* Clone() { return new NiPSysEmitterDeclinationVarCtlr(*this); }
};

class NiPSysFieldMagnitudeCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysFieldMagnitudeCtlr() {}
	NiPSysFieldMagnitudeCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysFieldMagnitudeCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldMagnitudeCtlr* Clone() { return new NiPSysFieldMagnitudeCtlr(*this); }
};

class NiPSysFieldAttenuationCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysFieldAttenuationCtlr() {}
	NiPSysFieldAttenuationCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysFieldAttenuationCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldAttenuationCtlr* Clone() { return new NiPSysFieldAttenuationCtlr(*this); }
};

class NiPSysFieldMaxDistanceCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysFieldMaxDistanceCtlr() {}
	NiPSysFieldMaxDistanceCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysFieldMaxDistanceCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldMaxDistanceCtlr* Clone() { return new NiPSysFieldMaxDistanceCtlr(*this); }
};

class NiPSysAirFieldAirFrictionCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysAirFieldAirFrictionCtlr() {}
	NiPSysAirFieldAirFrictionCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysAirFieldAirFrictionCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldAirFrictionCtlr* Clone() { return new NiPSysAirFieldAirFrictionCtlr(*this); }
};

class NiPSysAirFieldInheritVelocityCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysAirFieldInheritVelocityCtlr() {}
	NiPSysAirFieldInheritVelocityCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysAirFieldInheritVelocityCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldInheritVelocityCtlr* Clone() { return new NiPSysAirFieldInheritVelocityCtlr(*this); }
};

class NiPSysAirFieldSpreadCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysAirFieldSpreadCtlr() {}
	NiPSysAirFieldSpreadCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysAirFieldSpreadCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldSpreadCtlr* Clone() { return new NiPSysAirFieldSpreadCtlr(*this); }
};

class NiPSysInitialRotSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotSpeedCtlr() {}
	NiPSysInitialRotSpeedCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysInitialRotSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotSpeedCtlr* Clone() { return new NiPSysInitialRotSpeedCtlr(*this); }
};

class NiPSysInitialRotSpeedVarCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotSpeedVarCtlr() {}
	NiPSysInitialRotSpeedVarCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysInitialRotSpeedVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotSpeedVarCtlr* Clone() { return new NiPSysInitialRotSpeedVarCtlr(*this); }
};

class NiPSysInitialRotAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotAngleCtlr() {}
	NiPSysInitialRotAngleCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysInitialRotAngleCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotAngleCtlr* Clone() { return new NiPSysInitialRotAngleCtlr(*this); }
};

class NiPSysInitialRotAngleVarCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotAngleVarCtlr() {}
	NiPSysInitialRotAngleVarCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysInitialRotAngleVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotAngleVarCtlr* Clone() { return new NiPSysInitialRotAngleVarCtlr(*this); }
};

class NiPSysEmitterPlanarAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterPlanarAngleCtlr() {}
	NiPSysEmitterPlanarAngleCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterPlanarAngleCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterPlanarAngleCtlr* Clone() { return new NiPSysEmitterPlanarAngleCtlr(*this); }
};

class NiPSysEmitterPlanarAngleVarCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterPlanarAngleVarCtlr() {}
	NiPSysEmitterPlanarAngleVarCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterPlanarAngleVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterPlanarAngleVarCtlr* Clone() { return new NiPSysEmitterPlanarAngleVarCtlr(*this); }
};

class NiPSysEmitterCtlr : public NiPSysModifierCtlr {
private:
	BlockRef<NiInterpolator> visInterpolatorRef;

public:
	NiPSysEmitterCtlr() {}
	NiPSysEmitterCtlr(NiStream& stream);

	static constexpr const char* BlockName = "NiPSysEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiPSysEmitterCtlr* Clone() { return new NiPSysEmitterCtlr(*this); }
};

class BSMasterParticleSystem;

class BSPSysMultiTargetEmitterCtlr : public NiPSysEmitterCtlr {
private:
	ushort maxEmitters = 0;
	BlockRef<BSMasterParticleSystem> masterParticleSystemRef;

public:
	BSPSysMultiTargetEmitterCtlr();
	BSPSysMultiTargetEmitterCtlr(NiStream& stream);

	static constexpr const char* BlockName = "BSPSysMultiTargetEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
	BSPSysMultiTargetEmitterCtlr* Clone() { return new BSPSysMultiTargetEmitterCtlr(*this); }
};

struct ControllerLink {
	BlockRef<NiInterpolator> interpolatorRef;
	BlockRef<NiTimeController> controllerRef;
	byte priority;

	StringRef nodeName;
	StringRef propType;
	StringRef ctrlType;
	StringRef ctrlID;
	StringRef interpID;
};

class NiSequence : public NiObject {
private:
	StringRef name;
	uint numControlledBlocks = 0;
	uint arrayGrowBy = 0;
	std::vector<ControllerLink> controlledBlocks;

public:
	NiSequence() {}
	NiSequence(NiStream& stream);

	static constexpr const char* BlockName = "NiSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);
	NiSequence* Clone() { return new NiSequence(*this); }
};

enum CycleType : uint {
	CYCLE_LOOP,
	CYCLE_REVERSE,
	CYCLE_CLAMP
};

class NiControllerManager;

class NiControllerSequence : public NiSequence {
private:
	float weight = 1.0f;
	BlockRef<NiTextKeyExtraData> textKeyRef;
	CycleType cycleType = CYCLE_LOOP;
	float frequency = 0.0f;
	float startTime = 0.0f;
	float stopTime = 0.0f;
	BlockRef<NiControllerManager> managerRef;

	StringRef accumRootName;

	ushort flags = 0;

public:
	NiControllerSequence();
	NiControllerSequence(NiStream& stream);

	static constexpr const char* BlockName = "NiControllerSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);
	void GetPtrs(std::set<int*>& ptrs);
	NiControllerSequence* Clone() { return new NiControllerSequence(*this); }
};

class NiDefaultAVObjectPalette;

class NiControllerManager : public NiTimeController {
private:
	bool cumulative = false;
	BlockRefArray<NiControllerSequence> controllerSequenceRefs;
	BlockRef<NiDefaultAVObjectPalette> objectPaletteRef;

public:
	NiControllerManager() {}
	NiControllerManager(NiStream& stream);

	static constexpr const char* BlockName = "NiControllerManager";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiControllerManager* Clone() { return new NiControllerManager(*this); }
};
