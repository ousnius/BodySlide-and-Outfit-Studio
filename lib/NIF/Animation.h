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
	uint numRotationKeys;
	uint rotationType;
	std::vector<Key<Quaternion>> quaternionKeys;
	KeyGroup<float> xRotations;
	KeyGroup<float> yRotations;
	KeyGroup<float> zRotations;
	KeyGroup<Vector3> translations;
	KeyGroup<float> scales;

public:
	NiKeyframeData(NiHeader* hdr);
	NiKeyframeData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiKeyframeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiKeyframeData* Clone() { return new NiKeyframeData(*this); }
};

class NiTransformData : public NiKeyframeData {
public:
	NiTransformData(NiHeader* hdr);
	NiTransformData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTransformData";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformData* Clone() { return new NiTransformData(*this); }
};

class NiPosData : public NiObject {
private:
	KeyGroup<Vector3> data;

public:
	NiPosData(NiHeader* hdr);
	NiPosData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPosData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiPosData* Clone() { return new NiPosData(*this); }
};

class NiBoolData : public NiObject {
private:
	KeyGroup<byte> data;

public:
	NiBoolData(NiHeader* hdr);
	NiBoolData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBoolData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBoolData* Clone() { return new NiBoolData(*this); }
};

class NiFloatData : public NiObject {
private:
	KeyGroup<float> data;

public:
	NiFloatData(NiHeader* hdr);
	NiFloatData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiFloatData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiFloatData* Clone() { return new NiFloatData(*this); }
};

class NiInterpolator : public NiObject {
};

class NiBlendInterpolator : public NiInterpolator {
private:
	ushort flags;
	uint unkInt;

public:
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
};

class NiBlendBoolInterpolator : public NiBlendInterpolator {
private:
	bool value;

public:
	NiBlendBoolInterpolator(NiHeader* hdr);
	NiBlendBoolInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBlendBoolInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBlendBoolInterpolator* Clone() { return new NiBlendBoolInterpolator(*this); }
};

class NiBlendFloatInterpolator : public NiBlendInterpolator {
private:
	float value;

public:
	NiBlendFloatInterpolator(NiHeader* hdr);
	NiBlendFloatInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBlendFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBlendFloatInterpolator* Clone() { return new NiBlendFloatInterpolator(*this); }
};

class NiBlendPoint3Interpolator : public NiBlendInterpolator {
private:
	Vector3 point;

public:
	NiBlendPoint3Interpolator(NiHeader* hdr);
	NiBlendPoint3Interpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBlendPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBlendPoint3Interpolator* Clone() { return new NiBlendPoint3Interpolator(*this); }
};

class NiKeyBasedInterpolator : public NiInterpolator {
};

class NiBoolInterpolator : public NiKeyBasedInterpolator {
private:
	byte boolValue = 0;

public:
	BlockRef<NiBoolData> dataRef;

	NiBoolInterpolator(NiHeader* hdr);
	NiBoolInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBoolInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiBoolInterpolator* Clone() { return new NiBoolInterpolator(*this); }
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {
public:
	NiBoolTimelineInterpolator(NiHeader* hdr);
	NiBoolTimelineInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBoolTimelineInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	NiBoolTimelineInterpolator* Clone() { return new NiBoolTimelineInterpolator(*this); }
};

class NiFloatInterpolator : public NiKeyBasedInterpolator {
private:
	float floatValue = 0.0f;

public:
	BlockRef<NiFloatData> dataRef;

	NiFloatInterpolator(NiHeader* hdr);
	NiFloatInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiFloatInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiFloatInterpolator* Clone() { return new NiFloatInterpolator(*this); }
};

class NiTransformInterpolator : public NiKeyBasedInterpolator {
private:
	Vector3 translation;
	Quaternion rotation;
	float scale = 0.0f;

public:
	BlockRef<NiTransformData> dataRef;

	NiTransformInterpolator(NiHeader* hdr);
	NiTransformInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTransformInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiTransformInterpolator* Clone() { return new NiTransformInterpolator(*this); }
};

class NiPoint3Interpolator : public NiKeyBasedInterpolator {
private:
	Vector3 point3Value;

public:
	BlockRef<NiPosData> dataRef;

	NiPoint3Interpolator(NiHeader* hdr);
	NiPoint3Interpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPoint3Interpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPoint3Interpolator* Clone() { return new NiPoint3Interpolator(*this); }
};

class NiPathInterpolator : public NiKeyBasedInterpolator {
private:
	ushort flags;
	uint bankDir;
	float maxBankAngle;
	float smoothing;
	ushort followAxis;

	BlockRef<NiPosData> pathDataRef;
	BlockRef<NiFloatData> percentDataRef;

public:
	NiPathInterpolator(NiHeader* hdr);
	NiPathInterpolator(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPathInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPathInterpolator* Clone() { return new NiPathInterpolator(*this); }
};

class NiNode;

class NiLookAtInterpolator : public NiInterpolator {
private:
	ushort flags;
	BlockRef<NiNode> lookAtRef;

	StringRef lookAtName;

	QuatTransform transform;
	BlockRef<NiPoint3Interpolator> translateInterpRef;
	BlockRef<NiFloatInterpolator> rollInterpRef;
	BlockRef<NiFloatInterpolator> scaleInterpRef;

public:
	NiLookAtInterpolator(NiHeader* hdr);
	NiLookAtInterpolator(std::fstream& file, NiHeader* hdr);
	~NiLookAtInterpolator() {
		lookAtName.Clear(header);
	};

	static constexpr const char* BlockName = "NiLookAtInterpolator";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiLookAtInterpolator* Clone() { return new NiLookAtInterpolator(*this); }
};

class NiObjectNET;

class NiTimeController : public NiObject {
private:
	uint flags;
	float frequency;
	float phase;
	float startTime;
	float stopTime;

public:
	BlockRef<NiTimeController> nextControllerRef;
	BlockRef<NiObjectNET> targetRef;

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
};

class BSFrustumFOVController : public NiTimeController {
public:
	BlockRef<NiInterpolator> interpolatorRef;

	BSFrustumFOVController(NiHeader* hdr);
	BSFrustumFOVController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSFrustumFOVController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSFrustumFOVController* Clone() { return new BSFrustumFOVController(*this); }
};

class BSLagBoneController : public NiTimeController {
private:
	float linearVelocity;
	float linearRotation;
	float maxDistance;

public:
	BSLagBoneController(NiHeader* hdr);
	BSLagBoneController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSLagBoneController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSLagBoneController* Clone() { return new BSLagBoneController(*this); }
};

class BSShaderProperty;

class BSProceduralLightningController : public NiTimeController {
private:
	ushort subdivisions;
	ushort numBranches;
	ushort numBranchesPerVariation;

	float length;
	float lengthVariation;
	float width;
	float childWidthMult;
	float arcOffset;
	bool fadeMainBolt;
	bool fadeChildBolts;
	bool animateArcOffset;

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

	BSProceduralLightningController(NiHeader* hdr);
	BSProceduralLightningController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSProceduralLightningController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSProceduralLightningController* Clone() { return new BSProceduralLightningController(*this); }
};

class NiBoneLODController : public NiTimeController {
private:
	uint lod;
	uint numLODs;
	uint boneArraysSize;
	std::vector<BlockRefArray<NiNode>> boneArrays;

public:
	NiBoneLODController(NiHeader* hdr);
	NiBoneLODController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBoneLODController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiBoneLODController* Clone() { return new NiBoneLODController(*this); }
};

class NiInterpController : public NiTimeController {
};

class NiSingleInterpController : public NiInterpController {
public:
	BlockRef<NiInterpController> interpolatorRef;

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
};

class NiExtraDataController : public NiSingleInterpController {
};

class NiFloatExtraDataController : public NiExtraDataController {
private:
	StringRef extraData;

public:
	NiFloatExtraDataController(NiHeader* hdr);
	NiFloatExtraDataController(std::fstream& file, NiHeader* hdr);
	~NiFloatExtraDataController() {
		extraData.Clear(header);
	};

	static constexpr const char* BlockName = "NiFloatExtraDataController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	NiFloatExtraDataController* Clone() { return new NiFloatExtraDataController(*this); }
};

class NiBoolInterpController : public NiSingleInterpController {
};

class NiVisController : public NiBoolInterpController {
public:
	NiVisController(NiHeader* hdr);
	NiVisController(std::fstream& file, NiHeader* hdr);

	virtual const char* GetBlockName() { return BlockName; }

	NiVisController* Clone() { return new NiVisController(*this); }
};

class NiFloatInterpController : public NiSingleInterpController {
};

class NiAlphaController : public NiFloatInterpController {
public:
	NiAlphaController(NiHeader* hdr);
	NiAlphaController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiAlphaController";
	virtual const char* GetBlockName() { return BlockName; }

	NiAlphaController* Clone() { return new NiAlphaController(*this); }
};

class NiPSysUpdateCtlr : public NiTimeController {
public:
	NiPSysUpdateCtlr(NiHeader* hdr);
	NiPSysUpdateCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysUpdateCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysUpdateCtlr* Clone() { return new NiPSysUpdateCtlr(*this); }
};

class BSNiAlphaPropertyTestRefController : public NiAlphaController {
public:
	BSNiAlphaPropertyTestRefController(NiHeader* hdr);
	BSNiAlphaPropertyTestRefController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSNiAlphaPropertyTestRefController";
	virtual const char* GetBlockName() { return BlockName; }

	BSNiAlphaPropertyTestRefController* Clone() { return new BSNiAlphaPropertyTestRefController(*this); }
};

class NiKeyframeController : public NiSingleInterpController {
public:
	NiKeyframeController(NiHeader* hdr);
	NiKeyframeController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiKeyframeController";
	virtual const char* GetBlockName() { return BlockName; }

	NiKeyframeController* Clone() { return new NiKeyframeController(*this); }
};

class NiTransformController : public NiKeyframeController {
public:
	NiTransformController(NiHeader* hdr);
	NiTransformController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	NiTransformController* Clone() { return new NiTransformController(*this); }
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor;

public:
	BSLightingShaderPropertyColorController(NiHeader* hdr);
	BSLightingShaderPropertyColorController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSLightingShaderPropertyColorController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSLightingShaderPropertyColorController* Clone() { return new BSLightingShaderPropertyColorController(*this); }
};

class BSLightingShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable;

public:
	BSLightingShaderPropertyFloatController(NiHeader* hdr);
	BSLightingShaderPropertyFloatController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSLightingShaderPropertyFloatController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSLightingShaderPropertyFloatController* Clone() { return new BSLightingShaderPropertyFloatController(*this); }
};

class BSEffectShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor;

public:
	BSEffectShaderPropertyColorController(NiHeader* hdr);
	BSEffectShaderPropertyColorController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSEffectShaderPropertyColorController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSEffectShaderPropertyColorController* Clone() { return new BSEffectShaderPropertyColorController(*this); }
};

class BSEffectShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable;

public:
	BSEffectShaderPropertyFloatController(NiHeader* hdr);
	BSEffectShaderPropertyFloatController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSEffectShaderPropertyFloatController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSEffectShaderPropertyFloatController* Clone() { return new BSEffectShaderPropertyFloatController(*this); }
};

class NiAVObject;

class NiMultiTargetTransformController : public NiInterpController {
private:
	BlockRefShortArray<NiAVObject> targetRefs;

public:
	NiMultiTargetTransformController(NiHeader* hdr);
	NiMultiTargetTransformController(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiMultiTargetTransformController";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiMultiTargetTransformController* Clone() { return new NiMultiTargetTransformController(*this); }
};

class NiPSysModifierCtlr : public NiSingleInterpController {
private:
	StringRef modifierName;

public:
	~NiPSysModifierCtlr() {
		modifierName.Clear(header);
	};

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
};

class NiPSysModifierBoolCtlr : public NiPSysModifierCtlr {
};

class NiPSysModifierActiveCtlr : public NiPSysModifierBoolCtlr {
public:
	NiPSysModifierActiveCtlr(NiHeader* hdr);
	NiPSysModifierActiveCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysModifierActiveCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysModifierActiveCtlr* Clone() { return new NiPSysModifierActiveCtlr(*this); }
};

class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr {
};

class NiPSysEmitterLifeSpanCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterLifeSpanCtlr(NiHeader* hdr);
	NiPSysEmitterLifeSpanCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterLifeSpanCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterLifeSpanCtlr* Clone() { return new NiPSysEmitterLifeSpanCtlr(*this); }
};

class NiPSysEmitterSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterSpeedCtlr(NiHeader* hdr);
	NiPSysEmitterSpeedCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterSpeedCtlr* Clone() { return new NiPSysEmitterSpeedCtlr(*this); }
};

class NiPSysEmitterInitialRadiusCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterInitialRadiusCtlr(NiHeader* hdr);
	NiPSysEmitterInitialRadiusCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterInitialRadiusCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterInitialRadiusCtlr* Clone() { return new NiPSysEmitterInitialRadiusCtlr(*this); }
};

class NiPSysEmitterPlanarAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterPlanarAngleCtlr(NiHeader* hdr);
	NiPSysEmitterPlanarAngleCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterPlanarAngleCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterPlanarAngleCtlr* Clone() { return new NiPSysEmitterPlanarAngleCtlr(*this); }
};

class NiPSysEmitterDeclinationCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterDeclinationCtlr(NiHeader* hdr);
	NiPSysEmitterDeclinationCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterDeclinationCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysEmitterDeclinationCtlr* Clone() { return new NiPSysEmitterDeclinationCtlr(*this); }
};

class NiPSysGravityStrengthCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysGravityStrengthCtlr(NiHeader* hdr);
	NiPSysGravityStrengthCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysGravityStrengthCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysGravityStrengthCtlr* Clone() { return new NiPSysGravityStrengthCtlr(*this); }
};

class NiPSysInitialRotSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotSpeedCtlr(NiHeader* hdr);
	NiPSysInitialRotSpeedCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysInitialRotSpeedCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	NiPSysInitialRotSpeedCtlr* Clone() { return new NiPSysInitialRotSpeedCtlr(*this); }
};

class NiPSysEmitterCtlr : public NiPSysModifierCtlr {
private:
	BlockRef<NiInterpolator> visInterpolatorRef;

public:
	NiPSysEmitterCtlr(NiHeader* hdr);
	NiPSysEmitterCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiPSysEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiPSysEmitterCtlr* Clone() { return new NiPSysEmitterCtlr(*this); }
};

class BSMasterParticleSystem;

class BSPSysMultiTargetEmitterCtlr : public NiPSysEmitterCtlr {
private:
	ushort maxEmitters;
	BlockRef<BSMasterParticleSystem> masterParticleSystemRef;

public:
	BSPSysMultiTargetEmitterCtlr(NiHeader* hdr);
	BSPSysMultiTargetEmitterCtlr(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSPSysMultiTargetEmitterCtlr";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	uint numControlledBlocks;
	uint unkInt1;
	std::vector<ControllerLink> controlledBlocks;

public:
	NiSequence(NiHeader* hdr);
	NiSequence(std::fstream& file, NiHeader* hdr);
	~NiSequence() {
		for (auto &cb : controlledBlocks) {
			cb.nodeName.Clear(header);
			cb.propType.Clear(header);
			cb.ctrlType.Clear(header);
			cb.ctrlID.Clear(header);
			cb.interpID.Clear(header);
		}
	};

	static constexpr const char* BlockName = "NiSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiSequence* Clone() { return new NiSequence(*this); }
};

class NiControllerManager;

class NiControllerSequence : public NiSequence {
private:
	float weight;
	BlockRef<NiTextKeyExtraData> textKeyRef;
	uint cycleType;
	float frequency;
	float startTime;
	float stopTime;
	BlockRef<NiControllerManager> managerRef;

	StringRef accumRootName;

	ushort flags;

public:
	NiControllerSequence(NiHeader* hdr);
	NiControllerSequence(std::fstream& file, NiHeader* hdr);
	~NiControllerSequence() {
		accumRootName.Clear(header);
	};

	static constexpr const char* BlockName = "NiControllerSequence";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiControllerSequence* Clone() { return new NiControllerSequence(*this); }
};

class NiDefaultAVObjectPalette;

class NiControllerManager : public NiTimeController {
private:
	bool cumulative;
	BlockRefArray<NiControllerSequence> controllerSequenceRefs;
	BlockRef<NiDefaultAVObjectPalette> objectPaletteRef;

public:
	NiControllerManager(NiHeader* hdr);
	NiControllerManager(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiControllerManager";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiControllerManager* Clone() { return new NiControllerManager(*this); }
};
