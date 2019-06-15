/*
BodySlide and Outfit Studio
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
	static constexpr const char* BlockName = "NiKeyframeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiKeyframeData* Clone() { return new NiKeyframeData(*this); }
};

class NiTransformData : public NiKeyframeData {
public:
	static constexpr const char* BlockName = "NiTransformData";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformData* Clone() { return new NiTransformData(*this); }
};

class NiPosData : public NiObject {
private:
	KeyGroup<Vector3> data;

public:
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
	void GetChildRefs(std::set<Ref*>& refs);
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
	static constexpr const char* BlockName = "NiBlendPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBlendPoint3Interpolator* Clone() { return new NiBlendPoint3Interpolator(*this); }
};

class NiBlendTransformInterpolator : public NiBlendInterpolator {
public:
	static constexpr const char* BlockName = "NiBlendTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBlendTransformInterpolator* Clone() { return new NiBlendTransformInterpolator(*this); }
};

class NiKeyBasedInterpolator : public NiInterpolator {
};

class NiBoolInterpolator : public NiKeyBasedInterpolator {
private:
	byte boolValue = 0;
	BlockRef<NiBoolData> dataRef;

public:
	static constexpr const char* BlockName = "NiBoolInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiBoolInterpolator* Clone() { return new NiBoolInterpolator(*this); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(int datRef) { dataRef.SetIndex(datRef); }
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {
public:
	static constexpr const char* BlockName = "NiBoolTimelineInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBoolTimelineInterpolator* Clone() { return new NiBoolTimelineInterpolator(*this); }
};

class NiFloatInterpolator : public NiKeyBasedInterpolator {
private:
	float floatValue = 0.0f;
	BlockRef<NiFloatData> dataRef;

public:
	static constexpr const char* BlockName = "NiFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiFloatInterpolator* Clone() { return new NiFloatInterpolator(*this); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(int datRef) { dataRef.SetIndex(datRef); }
};

class NiTransformInterpolator : public NiKeyBasedInterpolator {
private:
	Vector3 translation;
	Quaternion rotation;
	float scale = 0.0f;
	BlockRef<NiTransformData> dataRef;

public:
	static constexpr const char* BlockName = "NiTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiTransformInterpolator* Clone() { return new NiTransformInterpolator(*this); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(int datRef) { dataRef.SetIndex(datRef); }
};

class BSRotAccumTransfInterpolator : public NiTransformInterpolator {
public:
	static constexpr const char* BlockName = "BSRotAccumTransfInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	BSRotAccumTransfInterpolator* Clone() { return new BSRotAccumTransfInterpolator(*this); }
};

class NiPoint3Interpolator : public NiKeyBasedInterpolator {
private:
	Vector3 point3Value;
	BlockRef<NiPosData> dataRef;

public:
	static constexpr const char* BlockName = "NiPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPoint3Interpolator* Clone() { return new NiPoint3Interpolator(*this); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(int datRef) { dataRef.SetIndex(datRef); }
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
	static constexpr const char* BlockName = "NiPathInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
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
	static constexpr const char* BlockName = "NiLookAtInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
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
	static constexpr const char* BlockName = "BSTreadTransfInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	BSTreadTransfInterpolator* Clone() { return new BSTreadTransfInterpolator(*this); }
};

class NiObjectNET;

class NiTimeController : public NiObject {
private:
	BlockRef<NiTimeController> nextControllerRef;
	ushort flags = 0x000C;
	float frequency = 1.0f;
	float phase = 0.0f;
	float startTime = 0.0f;
	float stopTime = 0.0f;
	BlockRef<NiObjectNET> targetRef;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);

	int GetNextControllerRef() { return nextControllerRef.GetIndex(); }
	void SetNextControllerRef(int ctlrRef) { nextControllerRef.SetIndex(ctlrRef); }

	int GetTargetRef() { return targetRef.GetIndex(); }
	void SetTargetRef(int targRef) { targetRef.SetIndex(targRef); }
};

class NiLookAtController : public NiTimeController {
private:
	ushort unkShort1 = 0;
	BlockRef<NiNode> lookAtNodePtr;

public:
	static constexpr const char* BlockName = "NiLookAtController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
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
	static constexpr const char* BlockName = "NiPathController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPathController* Clone() { return new NiPathController(*this); }
};

class NiPSysResetOnLoopCtlr : public NiTimeController {
public:
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
	static constexpr const char* BlockName = "NiUVController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiUVController* Clone() { return new NiUVController(*this); }
};

class BSFrustumFOVController : public NiTimeController {
private:
	BlockRef<NiInterpolator> interpolatorRef;

public:
	static constexpr const char* BlockName = "BSFrustumFOVController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	BSFrustumFOVController* Clone() { return new BSFrustumFOVController(*this); }

	int GetInterpolatorRef() { return interpolatorRef.GetIndex(); }
	void SetInterpolatorRef(int interpRef) { interpolatorRef.SetIndex(interpRef); }
};

class BSLagBoneController : public NiTimeController {
private:
	float linearVelocity = 0.0f;
	float linearRotation = 0.0f;
	float maxDistance = 0.0f;

public:
	static constexpr const char* BlockName = "BSLagBoneController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLagBoneController* Clone() { return new BSLagBoneController(*this); }
};

class BSShaderProperty;

class BSProceduralLightningController : public NiTimeController {
private:
	BlockRef<NiInterpolator> generationInterpRef;
	BlockRef<NiInterpolator> mutationInterpRef;
	BlockRef<NiInterpolator> subdivisionInterpRef;
	BlockRef<NiInterpolator> numBranchesInterpRef;
	BlockRef<NiInterpolator> numBranchesVarInterpRef;
	BlockRef<NiInterpolator> lengthInterpRef;
	BlockRef<NiInterpolator> lengthVarInterpRef;
	BlockRef<NiInterpolator> widthInterpRef;
	BlockRef<NiInterpolator> arcOffsetInterpRef;

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
	static constexpr const char* BlockName = "BSProceduralLightningController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	BSProceduralLightningController* Clone() { return new BSProceduralLightningController(*this); }

	int GetGenerationInterpRef() { return generationInterpRef.GetIndex(); }
	void SetGenerationInterpRef(int interpRef) { generationInterpRef.SetIndex(interpRef); }

	int GetMutationInterpRef() { return mutationInterpRef.GetIndex(); }
	void SetMutationInterpRef(int interpRef) { mutationInterpRef.SetIndex(interpRef); }

	int GetSubdivisionInterpRef() { return subdivisionInterpRef.GetIndex(); }
	void SetSubdivisionInterpRef(int interpRef) { subdivisionInterpRef.SetIndex(interpRef); }

	int GetNumBranchesInterpRef() { return numBranchesInterpRef.GetIndex(); }
	void SetNumBranchesInterpRef(int interpRef) { numBranchesInterpRef.SetIndex(interpRef); }

	int GetNumBranchesVarInterpRef() { return numBranchesVarInterpRef.GetIndex(); }
	void SetNumBranchesVarInterpRef(int interpRef) { numBranchesVarInterpRef.SetIndex(interpRef); }

	int GetLengthInterpRef() { return lengthInterpRef.GetIndex(); }
	void SetLengthInterpRef(int interpRef) { lengthInterpRef.SetIndex(interpRef); }

	int GetLengthVarInterpRef() { return lengthVarInterpRef.GetIndex(); }
	void SetLengthVarInterpRef(int interpRef) { lengthVarInterpRef.SetIndex(interpRef); }

	int GetWidthInterpRef() { return widthInterpRef.GetIndex(); }
	void SetWidthInterpRef(int interpRef) { widthInterpRef.SetIndex(interpRef); }

	int GetArcOffsetInterpRef() { return arcOffsetInterpRef.GetIndex(); }
	void SetArcOffsetInterpRef(int interpRef) { arcOffsetInterpRef.SetIndex(interpRef); }

	int GetShaderPropertyRef() { return shaderPropertyRef.GetIndex(); }
	void SetShaderPropertyRef(int shaderRef) { shaderPropertyRef.SetIndex(shaderRef); }
};

class NiBoneLODController : public NiTimeController {
private:
	uint lod = 0;
	uint numLODs = 0;
	uint boneArraysSize = 0;
	std::vector<BlockRefArray<NiNode>> boneArrays;

public:
	static constexpr const char* BlockName = "NiBoneLODController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiBoneLODController* Clone() { return new NiBoneLODController(*this); }
};

class NiBSBoneLODController : public NiBoneLODController {
public:
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

	void GetChildRefs(std::set<Ref*>& refs) {
		refs.insert(&interpRef);
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
	static constexpr const char* BlockName = "NiGeomMorpherController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	NiGeomMorpherController* Clone() { return new NiGeomMorpherController(*this); }
};

class NiSingleInterpController : public NiInterpController {
private:
	BlockRef<NiInterpController> interpolatorRef;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	int GetInterpolatorRef() { return interpolatorRef.GetIndex(); }
	void SetInterpolatorRef(int interpRef) { interpolatorRef.SetIndex(interpRef); }
};

class NiRollController : public NiSingleInterpController {
private:
	BlockRef<NiFloatData> dataRef;

public:
	static constexpr const char* BlockName = "NiRollController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

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
	static constexpr const char* BlockName = "NiMaterialColorController";
	virtual const char* GetBlockName() { return BlockName; }

	NiMaterialColorController* Clone() { return new NiMaterialColorController(*this); }
};

class NiLightColorController : public NiPoint3InterpController {
public:
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
	static constexpr const char* BlockName = "NiFloatExtraDataController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	NiFloatExtraDataController* Clone() { return new NiFloatExtraDataController(*this); }
};

class NiVisData : public NiObject {
private:
	uint numKeys = 0;
	std::vector<Key<byte>> keys;

public:
	static constexpr const char* BlockName = "NiVisData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiVisData* Clone() { return new NiVisData(*this); }
};

class NiBoolInterpController : public NiSingleInterpController {
};

class NiVisController : public NiBoolInterpController {
public:
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

class BSRefractionFirePeriodController : public NiSingleInterpController {
public:
	static constexpr const char* BlockName = "BSRefractionFirePeriodController";
	virtual const char* GetBlockName() { return BlockName; }

	BSRefractionFirePeriodController* Clone() { return new BSRefractionFirePeriodController(*this); }
};

class NiSourceTexture;

class NiFlipController : public NiFloatInterpController {
private:
	TexType textureSlot = BASE_MAP;
	BlockRefArray<NiSourceTexture> sourceRefs;

public:
	static constexpr const char* BlockName = "NiFlipController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	NiFlipController* Clone() { return new NiFlipController(*this); }

	BlockRefArray<NiSourceTexture>& GetSources();
};

enum TexTransformType : uint {
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
	TexTransformType operation = TT_TRANSLATE_U;

public:
	static constexpr const char* BlockName = "NiTextureTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiTextureTransformController* Clone() { return new NiTextureTransformController(*this); }
};

class NiLightDimmerController : public NiFloatInterpController {
public:
	static constexpr const char* BlockName = "NiLightDimmerController";
	virtual const char* GetBlockName() { return BlockName; }

	NiLightDimmerController* Clone() { return new NiLightDimmerController(*this); }
};

class NiLightRadiusController : public NiFloatInterpController {
public:
	static constexpr const char* BlockName = "NiLightRadiusController";
	virtual const char* GetBlockName() { return BlockName; }

	NiLightRadiusController* Clone() { return new NiLightRadiusController(*this); }
};

class NiAlphaController : public NiFloatInterpController {
public:
	static constexpr const char* BlockName = "NiAlphaController";
	virtual const char* GetBlockName() { return BlockName; }

	NiAlphaController* Clone() { return new NiAlphaController(*this); }
};

class NiPSysUpdateCtlr : public NiTimeController {
public:
	static constexpr const char* BlockName = "NiPSysUpdateCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysUpdateCtlr* Clone() { return new NiPSysUpdateCtlr(*this); }
};

class BSNiAlphaPropertyTestRefController : public NiAlphaController {
public:
	static constexpr const char* BlockName = "BSNiAlphaPropertyTestRefController";
	virtual const char* GetBlockName() { return BlockName; }

	BSNiAlphaPropertyTestRefController* Clone() { return new BSNiAlphaPropertyTestRefController(*this); }
};

class NiKeyframeController : public NiSingleInterpController {
public:
	static constexpr const char* BlockName = "NiKeyframeController";
	virtual const char* GetBlockName() { return BlockName; }

	NiKeyframeController* Clone() { return new NiKeyframeController(*this); }
};

class NiTransformController : public NiKeyframeController {
public:
	static constexpr const char* BlockName = "NiTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformController* Clone() { return new NiTransformController(*this); }
};

class BSMaterialEmittanceMultController : public NiFloatInterpController {
public:
	static constexpr const char* BlockName = "BSMaterialEmittanceMultController";
	virtual const char* GetBlockName() { return BlockName; }

	BSMaterialEmittanceMultController* Clone() { return new BSMaterialEmittanceMultController(*this); }
};

class BSRefractionStrengthController : public NiFloatInterpController {
public:
	static constexpr const char* BlockName = "BSRefractionStrengthController";
	virtual const char* GetBlockName() { return BlockName; }

	BSRefractionStrengthController* Clone() { return new BSRefractionStrengthController(*this); }
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor = 0;

public:
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
	static constexpr const char* BlockName = "NiMultiTargetTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiMultiTargetTransformController* Clone() { return new NiMultiTargetTransformController(*this); }

	BlockRefShortArray<NiAVObject>& GetTargets();
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
	static constexpr const char* BlockName = "NiPSysModifierActiveCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysModifierActiveCtlr* Clone() { return new NiPSysModifierActiveCtlr(*this); }
};

class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr {
};

class NiPSysEmitterLifeSpanCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterLifeSpanCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterLifeSpanCtlr* Clone() { return new NiPSysEmitterLifeSpanCtlr(*this); }
};

class NiPSysEmitterSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterSpeedCtlr* Clone() { return new NiPSysEmitterSpeedCtlr(*this); }
};

class NiPSysEmitterInitialRadiusCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterInitialRadiusCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterInitialRadiusCtlr* Clone() { return new NiPSysEmitterInitialRadiusCtlr(*this); }
};

class NiPSysEmitterDeclinationCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterDeclinationCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterDeclinationCtlr* Clone() { return new NiPSysEmitterDeclinationCtlr(*this); }
};

class NiPSysGravityStrengthCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysGravityStrengthCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysGravityStrengthCtlr* Clone() { return new NiPSysGravityStrengthCtlr(*this); }
};

class NiPSysEmitterDeclinationVarCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterDeclinationVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterDeclinationVarCtlr* Clone() { return new NiPSysEmitterDeclinationVarCtlr(*this); }
};

class NiPSysFieldMagnitudeCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysFieldMagnitudeCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldMagnitudeCtlr* Clone() { return new NiPSysFieldMagnitudeCtlr(*this); }
};

class NiPSysFieldAttenuationCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysFieldAttenuationCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldAttenuationCtlr* Clone() { return new NiPSysFieldAttenuationCtlr(*this); }
};

class NiPSysFieldMaxDistanceCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysFieldMaxDistanceCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysFieldMaxDistanceCtlr* Clone() { return new NiPSysFieldMaxDistanceCtlr(*this); }
};

class NiPSysAirFieldAirFrictionCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysAirFieldAirFrictionCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldAirFrictionCtlr* Clone() { return new NiPSysAirFieldAirFrictionCtlr(*this); }
};

class NiPSysAirFieldInheritVelocityCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysAirFieldInheritVelocityCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldInheritVelocityCtlr* Clone() { return new NiPSysAirFieldInheritVelocityCtlr(*this); }
};

class NiPSysAirFieldSpreadCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysAirFieldSpreadCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysAirFieldSpreadCtlr* Clone() { return new NiPSysAirFieldSpreadCtlr(*this); }
};

class NiPSysInitialRotSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysInitialRotSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotSpeedCtlr* Clone() { return new NiPSysInitialRotSpeedCtlr(*this); }
};

class NiPSysInitialRotSpeedVarCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysInitialRotSpeedVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotSpeedVarCtlr* Clone() { return new NiPSysInitialRotSpeedVarCtlr(*this); }
};

class NiPSysInitialRotAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysInitialRotAngleCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotAngleCtlr* Clone() { return new NiPSysInitialRotAngleCtlr(*this); }
};

class NiPSysInitialRotAngleVarCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysInitialRotAngleVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotAngleVarCtlr* Clone() { return new NiPSysInitialRotAngleVarCtlr(*this); }
};

class NiPSysEmitterPlanarAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterPlanarAngleCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterPlanarAngleCtlr* Clone() { return new NiPSysEmitterPlanarAngleCtlr(*this); }
};

class NiPSysEmitterPlanarAngleVarCtlr : public NiPSysModifierFloatCtlr {
public:
	static constexpr const char* BlockName = "NiPSysEmitterPlanarAngleVarCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterPlanarAngleVarCtlr* Clone() { return new NiPSysEmitterPlanarAngleVarCtlr(*this); }
};

class NiPSysEmitterCtlr : public NiPSysModifierCtlr {
private:
	BlockRef<NiInterpolator> visInterpolatorRef;

public:
	static constexpr const char* BlockName = "NiPSysEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiPSysEmitterCtlr* Clone() { return new NiPSysEmitterCtlr(*this); }
};

class BSMasterParticleSystem;

class BSPSysMultiTargetEmitterCtlr : public NiPSysEmitterCtlr {
private:
	ushort maxEmitters = 0;
	BlockRef<BSMasterParticleSystem> masterParticleSystemRef;

public:
	static constexpr const char* BlockName = "BSPSysMultiTargetEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<Ref*>& ptrs);
	BSPSysMultiTargetEmitterCtlr* Clone() { return new BSPSysMultiTargetEmitterCtlr(*this); }
};

class NiStringPalette : public NiObject {
private:
	NiString palette;
	uint length = 0;

public:
	static constexpr const char* BlockName = "NiStringPalette";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiStringPalette* Clone() { return new NiStringPalette(*this); }
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
	static constexpr const char* BlockName = "NiSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	NiSequence* Clone() { return new NiSequence(*this); }
};

enum CycleType : uint {
	CYCLE_LOOP,
	CYCLE_REVERSE,
	CYCLE_CLAMP
};

class BSAnimNote : public NiObject {
public:
	enum AnimNoteType : uint {
		ANT_INVALID,
		ANT_GRABIK,
		ANT_LOOKIK
	};

private:
	AnimNoteType type = ANT_INVALID;
	float time = 0.0f;
	uint arm = 0;
	float gain = 0.0f;
	uint state = 0;

public:
	static constexpr const char* BlockName = "BSAnimNote";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	BSAnimNote* Clone() { return new BSAnimNote(*this); }
};

class BSAnimNotes : public NiObject {
private:
	BlockRefShortArray<BSAnimNote> animNoteRefs;

public:
	static constexpr const char* BlockName = "BSAnimNotes";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	BSAnimNotes* Clone() { return new BSAnimNotes(*this); }

	BlockRefShortArray<BSAnimNote>& GetAnimNotes();
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

	BlockRef<BSAnimNotes> animNotesRef;
	BlockRefShortArray<BSAnimNotes> animNotesRefs;

public:
	static constexpr const char* BlockName = "NiControllerSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiControllerSequence* Clone() { return new NiControllerSequence(*this); }

	int GetAnimNotesRef();
	void SetAnimNotesRef(int notesRef);

	BlockRefShortArray<BSAnimNotes>& GetAnimNotes();
};

class NiDefaultAVObjectPalette;

class NiControllerManager : public NiTimeController {
private:
	bool cumulative = false;
	BlockRefArray<NiControllerSequence> controllerSequenceRefs;
	BlockRef<NiDefaultAVObjectPalette> objectPaletteRef;

public:
	static constexpr const char* BlockName = "NiControllerManager";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	NiControllerManager* Clone() { return new NiControllerManager(*this); }

	BlockRefArray<NiControllerSequence>& GetControllerSequences();

	int GetObjectPaletteRef();
	void SetObjectPaletteRef(int paletteRef);
};
