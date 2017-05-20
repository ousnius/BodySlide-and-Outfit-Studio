/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Animation.h"

NiKeyframeData::NiKeyframeData() {
	NiObject::Init();
}

NiKeyframeData::NiKeyframeData(NiStream& stream) : NiKeyframeData() {
	Get(stream);
}

void NiKeyframeData::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> numRotationKeys;

	if (numRotationKeys > 0) {
		stream >> rotationType;

		if (rotationType != 4) {
			quaternionKeys.resize(numRotationKeys);

			for (int i = 0; i < numRotationKeys; i++) {
				stream >> quaternionKeys[i].time;
				stream >> quaternionKeys[i].value;

				if (rotationType == 3)
					stream >> quaternionKeys[i].tbc;
			}
		}
		else {
			xRotations.Get(stream);
			yRotations.Get(stream);
			zRotations.Get(stream);
		}
	}

	translations.Get(stream);
	scales.Get(stream);
}

void NiKeyframeData::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << numRotationKeys;

	if (numRotationKeys > 0) {
		stream << rotationType;

		if (rotationType != 4) {
			for (int i = 0; i < numRotationKeys; i++) {
				stream << quaternionKeys[i].time;
				stream << quaternionKeys[i].value;

				if (rotationType == 3)
					stream << quaternionKeys[i].tbc;
			}
		}
		else {
			xRotations.Put(stream);
			yRotations.Put(stream);
			zRotations.Put(stream);
		}
	}

	translations.Put(stream);
	scales.Put(stream);
}

int NiKeyframeData::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 4;

	if (numRotationKeys > 0) {
		blockSize += 4;

		if (rotationType != 4) {
			blockSize += 20 * numRotationKeys;

			if (rotationType == 3)
				blockSize += 12 * numRotationKeys;
		}
		else {
			blockSize += xRotations.CalcGroupSize();
			blockSize += yRotations.CalcGroupSize();
			blockSize += zRotations.CalcGroupSize();
		}
	}

	blockSize += translations.CalcGroupSize();
	blockSize += scales.CalcGroupSize();

	return blockSize;
}


NiTransformData::NiTransformData() : NiKeyframeData() {
}

NiTransformData::NiTransformData(NiStream& stream) : NiTransformData() {
	Get(stream);
}


NiPosData::NiPosData() {
	NiObject::Init();
}

NiPosData::NiPosData(NiStream& stream) : NiPosData() {
	Get(stream);
}

void NiPosData::Get(NiStream& stream) {
	NiObject::Get(stream);

	data.Get(stream);
}

void NiPosData::Put(NiStream& stream) {
	NiObject::Put(stream);

	data.Put(stream);
}

int NiPosData::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiBoolData::NiBoolData() {
	NiObject::Init();
}

NiBoolData::NiBoolData(NiStream& stream) : NiBoolData() {
	Get(stream);
}

void NiBoolData::Get(NiStream& stream) {
	NiObject::Get(stream);

	data.Get(stream);
}

void NiBoolData::Put(NiStream& stream) {
	NiObject::Put(stream);

	data.Put(stream);
}

int NiBoolData::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiFloatData::NiFloatData() {
	NiObject::Init();
}

NiFloatData::NiFloatData(NiStream& stream) : NiFloatData() {
	Get(stream);
}

void NiFloatData::Get(NiStream& stream) {
	NiObject::Get(stream);

	data.Get(stream);
}

void NiFloatData::Put(NiStream& stream) {
	NiObject::Put(stream);

	data.Put(stream);
}

int NiFloatData::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += data.CalcGroupSize();

	return blockSize;
}


void NiTimeController::Init() {
	NiObject::Init();
}

void NiTimeController::Get(NiStream& stream) {
	NiObject::Get(stream);

	nextControllerRef.Get(stream);
	stream >> flags;
	stream >> frequency;
	stream >> phase;
	stream >> startTime;
	stream >> stopTime;
	targetRef.Get(stream);
}

void NiTimeController::Put(NiStream& stream) {
	NiObject::Put(stream);

	nextControllerRef.Put(stream);
	stream << flags;
	stream << frequency;
	stream << phase;
	stream << startTime;
	stream << stopTime;
	targetRef.Put(stream);
}

void NiTimeController::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&nextControllerRef.index);
}

int NiTimeController::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 26;

	return blockSize;
}


BSFrustumFOVController::BSFrustumFOVController() {
	NiTimeController::Init();
}

BSFrustumFOVController::BSFrustumFOVController(NiStream& stream) : BSFrustumFOVController() {
	Get(stream);
}

void BSFrustumFOVController::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	interpolatorRef.Get(stream);
}

void BSFrustumFOVController::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	interpolatorRef.Put(stream);
}

void BSFrustumFOVController::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(&interpolatorRef.index);
}

int BSFrustumFOVController::CalcBlockSize(NiVersion& version) {
	NiTimeController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSLagBoneController::BSLagBoneController() {
	NiTimeController::Init();
}

BSLagBoneController::BSLagBoneController(NiStream& stream) : BSLagBoneController() {
	Get(stream);
}

void BSLagBoneController::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	stream >> linearVelocity;
	stream >> linearRotation;
	stream >> maxDistance;
}

void BSLagBoneController::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	stream << linearVelocity;
	stream << linearRotation;
	stream << maxDistance;
}

int BSLagBoneController::CalcBlockSize(NiVersion& version) {
	NiTimeController::CalcBlockSize(version);

	blockSize += 12;

	return blockSize;
}


BSProceduralLightningController::BSProceduralLightningController() {
	NiTimeController::Init();
}

BSProceduralLightningController::BSProceduralLightningController(NiStream& stream) : BSProceduralLightningController() {
	Get(stream);
}

void BSProceduralLightningController::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	generationInterpRef.Get(stream);
	mutationInterpRef.Get(stream);
	subdivisionInterpRef.Get(stream);
	numBranchesInterpRef.Get(stream);
	numBranchesVarInterpRef.Get(stream);
	lengthInterpRef.Get(stream);
	lengthVarInterpRef.Get(stream);
	widthInterpRef.Get(stream);
	arcOffsetInterpRef.Get(stream);

	stream >> subdivisions;
	stream >> numBranches;
	stream >> numBranchesPerVariation;

	stream >> length;
	stream >> lengthVariation;
	stream >> width;
	stream >> childWidthMult;
	stream >> arcOffset;

	stream >> fadeMainBolt;
	stream >> fadeChildBolts;
	stream >> animateArcOffset;

	shaderPropertyRef.Get(stream);
}

void BSProceduralLightningController::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	generationInterpRef.Put(stream);
	mutationInterpRef.Put(stream);
	subdivisionInterpRef.Put(stream);
	numBranchesInterpRef.Put(stream);
	numBranchesVarInterpRef.Put(stream);
	lengthInterpRef.Put(stream);
	lengthVarInterpRef.Put(stream);
	widthInterpRef.Put(stream);
	arcOffsetInterpRef.Put(stream);

	stream << subdivisions;
	stream << numBranches;
	stream << numBranchesPerVariation;

	stream << length;
	stream << lengthVariation;
	stream << width;
	stream << childWidthMult;
	stream << arcOffset;

	stream << fadeMainBolt;
	stream << fadeChildBolts;
	stream << animateArcOffset;

	shaderPropertyRef.Put(stream);
}

void BSProceduralLightningController::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(&generationInterpRef.index);
	refs.insert(&mutationInterpRef.index);
	refs.insert(&subdivisionInterpRef.index);
	refs.insert(&numBranchesInterpRef.index);
	refs.insert(&numBranchesVarInterpRef.index);
	refs.insert(&lengthInterpRef.index);
	refs.insert(&lengthVarInterpRef.index);
	refs.insert(&widthInterpRef.index);
	refs.insert(&arcOffsetInterpRef.index);
	refs.insert(&shaderPropertyRef.index);
}

int BSProceduralLightningController::CalcBlockSize(NiVersion& version) {
	NiTimeController::CalcBlockSize(version);

	blockSize += 69;

	return blockSize;
}


NiBoneLODController::NiBoneLODController() {
	NiTimeController::Init();
}

NiBoneLODController::NiBoneLODController(NiStream& stream) : NiBoneLODController() {
	Get(stream);
}

void NiBoneLODController::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	stream >> lod;
	stream >> numLODs;

	stream >> boneArraysSize;
	boneArrays.resize(boneArraysSize);
	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].Get(stream);
}

void NiBoneLODController::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	stream << lod;
	stream << numLODs;

	stream << boneArraysSize;
	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].Put(stream);
}

void NiBoneLODController::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].GetIndexPtrs(refs);
}

int NiBoneLODController::CalcBlockSize(NiVersion& version) {
	NiTimeController::CalcBlockSize(version);

	blockSize += 12;
	blockSize += boneArraysSize * 4;
	for (int i = 0; i < boneArraysSize; i++)
		blockSize += boneArrays[i].GetSize() * 4;

	return blockSize;
}


void NiSingleInterpController::Init() {
	NiInterpController::Init();
}

void NiSingleInterpController::Get(NiStream& stream) {
	NiInterpController::Get(stream);

	interpolatorRef.Get(stream);
}

void NiSingleInterpController::Put(NiStream& stream) {
	NiInterpController::Put(stream);

	interpolatorRef.Put(stream);
}

void NiSingleInterpController::GetChildRefs(std::set<int*>& refs) {
	NiInterpController::GetChildRefs(refs);

	refs.insert(&interpolatorRef.index);
}

int NiSingleInterpController::CalcBlockSize(NiVersion& version) {
	NiInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiFloatExtraDataController::NiFloatExtraDataController() {
	NiExtraDataController::Init();
}

NiFloatExtraDataController::NiFloatExtraDataController(NiStream& stream) : NiFloatExtraDataController() {
	Get(stream);
}

void NiFloatExtraDataController::Get(NiStream& stream) {
	NiExtraDataController::Get(stream);

	extraData.Get(stream);
}

void NiFloatExtraDataController::Put(NiStream& stream) {
	NiExtraDataController::Put(stream);

	extraData.Put(stream);
}

void NiFloatExtraDataController::GetStringRefs(std::set<StringRef*>& refs) {
	NiExtraDataController::GetStringRefs(refs);

	refs.insert(&extraData);
}

int NiFloatExtraDataController::CalcBlockSize(NiVersion& version) {
	NiExtraDataController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiVisController::NiVisController() {
	NiBoolInterpController::Init();
}

NiVisController::NiVisController(NiStream& stream) : NiVisController() {
	Get(stream);
}


NiAlphaController::NiAlphaController() {
	NiFloatInterpController::Init();
}

NiAlphaController::NiAlphaController(NiStream& stream) : NiAlphaController() {
	Get(stream);
}


NiPSysUpdateCtlr::NiPSysUpdateCtlr() {
	NiTimeController::Init();
}

NiPSysUpdateCtlr::NiPSysUpdateCtlr(NiStream& stream) : NiPSysUpdateCtlr() {
	Get(stream);
}


BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController() : NiAlphaController() {
}

BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController(NiStream& stream) : BSNiAlphaPropertyTestRefController() {
	Get(stream);
}


NiKeyframeController::NiKeyframeController() {
	NiSingleInterpController::Init();
}

NiKeyframeController::NiKeyframeController(NiStream& stream) : NiKeyframeController() {
	Get(stream);
}


NiTransformController::NiTransformController() : NiKeyframeController() {
}

NiTransformController::NiTransformController(NiStream& stream) : NiTransformController() {
	Get(stream);
}


BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController() {
	NiFloatInterpController::Init();
}

BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(NiStream& stream) : BSLightingShaderPropertyColorController() {
	Get(stream);
}

void BSLightingShaderPropertyColorController::Get(NiStream& stream) {
	NiFloatInterpController::Get(stream);

	stream >> typeOfControlledColor;
}

void BSLightingShaderPropertyColorController::Put(NiStream& stream) {
	NiFloatInterpController::Put(stream);

	stream << typeOfControlledColor;
}

int BSLightingShaderPropertyColorController::CalcBlockSize(NiVersion& version) {
	NiFloatInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController() {
	NiFloatInterpController::Init();
}

BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(NiStream& stream) : BSLightingShaderPropertyFloatController() {
	Get(stream);
}

void BSLightingShaderPropertyFloatController::Get(NiStream& stream) {
	NiFloatInterpController::Get(stream);

	stream >> typeOfControlledVariable;
}

void BSLightingShaderPropertyFloatController::Put(NiStream& stream) {
	NiFloatInterpController::Put(stream);

	stream << typeOfControlledVariable;
}

int BSLightingShaderPropertyFloatController::CalcBlockSize(NiVersion& version) {
	NiFloatInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController() {
	NiFloatInterpController::Init();
}

BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(NiStream& stream) : BSEffectShaderPropertyColorController() {
	Get(stream);
}

void BSEffectShaderPropertyColorController::Get(NiStream& stream) {
	NiFloatInterpController::Get(stream);

	stream >> typeOfControlledColor;
}

void BSEffectShaderPropertyColorController::Put(NiStream& stream) {
	NiFloatInterpController::Put(stream);

	stream << typeOfControlledColor;
}

int BSEffectShaderPropertyColorController::CalcBlockSize(NiVersion& version) {
	NiFloatInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController() {
	NiFloatInterpController::Init();
}

BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(NiStream& stream) : BSEffectShaderPropertyFloatController() {
	Get(stream);
}

void BSEffectShaderPropertyFloatController::Get(NiStream& stream) {
	NiFloatInterpController::Get(stream);

	stream >> typeOfControlledVariable;
}

void BSEffectShaderPropertyFloatController::Put(NiStream& stream) {
	NiFloatInterpController::Put(stream);

	stream << typeOfControlledVariable;
}

int BSEffectShaderPropertyFloatController::CalcBlockSize(NiVersion& version) {
	NiFloatInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiMultiTargetTransformController::NiMultiTargetTransformController() {
	NiInterpController::Init();
}

NiMultiTargetTransformController::NiMultiTargetTransformController(NiStream& stream) : NiMultiTargetTransformController() {
	Get(stream);
}

void NiMultiTargetTransformController::Get(NiStream& stream) {
	NiInterpController::Get(stream);

	targetRefs.Get(stream);
}

void NiMultiTargetTransformController::Put(NiStream& stream) {
	NiInterpController::Put(stream);

	targetRefs.Put(stream);
}

void NiMultiTargetTransformController::GetChildRefs(std::set<int*>& refs) {
	NiInterpController::GetChildRefs(refs);

	targetRefs.GetIndexPtrs(refs);
}

int NiMultiTargetTransformController::CalcBlockSize(NiVersion& version) {
	NiInterpController::CalcBlockSize(version);

	blockSize += 2;
	blockSize += targetRefs.GetSize() * 4;

	return blockSize;
}


void NiPSysModifierCtlr::Get(NiStream& stream) {
	NiSingleInterpController::Get(stream);

	modifierName.Get(stream);
}

void NiPSysModifierCtlr::Put(NiStream& stream) {
	NiSingleInterpController::Put(stream);

	modifierName.Put(stream);
}

void NiPSysModifierCtlr::GetStringRefs(std::set<StringRef*>& refs) {
	NiSingleInterpController::GetStringRefs(refs);

	refs.insert(&modifierName);
}

int NiPSysModifierCtlr::CalcBlockSize(NiVersion& version) {
	NiSingleInterpController::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(NiStream& stream) : NiPSysModifierActiveCtlr() {
	Get(stream);
}


NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr(NiStream& stream) : NiPSysEmitterLifeSpanCtlr() {
	Get(stream);
}


NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr(NiStream& stream) : NiPSysEmitterSpeedCtlr() {
	Get(stream);
}


NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr(NiStream& stream) : NiPSysEmitterInitialRadiusCtlr() {
	Get(stream);
}


NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr(NiStream& stream) : NiPSysEmitterPlanarAngleCtlr() {
	Get(stream);
}


NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr(NiStream& stream) : NiPSysEmitterDeclinationCtlr() {
	Get(stream);
}


NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr(NiStream& stream) : NiPSysGravityStrengthCtlr() {
	Get(stream);
}


NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr(NiStream& stream) : NiPSysInitialRotSpeedCtlr() {
	Get(stream);
}


NiPSysEmitterCtlr::NiPSysEmitterCtlr() {
	NiPSysModifierCtlr::Init();
}

NiPSysEmitterCtlr::NiPSysEmitterCtlr(NiStream& stream) : NiPSysEmitterCtlr() {
	Get(stream);
}

void NiPSysEmitterCtlr::Get(NiStream& stream) {
	NiPSysModifierCtlr::Get(stream);

	visInterpolatorRef.Get(stream);
}

void NiPSysEmitterCtlr::Put(NiStream& stream) {
	NiPSysModifierCtlr::Put(stream);

	visInterpolatorRef.Put(stream);
}

void NiPSysEmitterCtlr::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifierCtlr::GetChildRefs(refs);

	refs.insert(&visInterpolatorRef.index);
}

int NiPSysEmitterCtlr::CalcBlockSize(NiVersion& version) {
	NiPSysModifierCtlr::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr() : NiPSysEmitterCtlr() {
}

BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr(NiStream& stream) : BSPSysMultiTargetEmitterCtlr() {
	Get(stream);
}

void BSPSysMultiTargetEmitterCtlr::Get(NiStream& stream) {
	NiPSysEmitterCtlr::Get(stream);

	stream >> maxEmitters;
	masterParticleSystemRef.Get(stream);
}

void BSPSysMultiTargetEmitterCtlr::Put(NiStream& stream) {
	NiPSysEmitterCtlr::Put(stream);

	stream << maxEmitters;
	masterParticleSystemRef.Put(stream);
}

void BSPSysMultiTargetEmitterCtlr::GetChildRefs(std::set<int*>& refs) {
	NiPSysEmitterCtlr::GetChildRefs(refs);

	refs.insert(&masterParticleSystemRef.index);
}

int BSPSysMultiTargetEmitterCtlr::CalcBlockSize(NiVersion& version) {
	NiPSysEmitterCtlr::CalcBlockSize(version);

	blockSize += 6;

	return blockSize;
}


void NiBlendInterpolator::Get(NiStream& stream) {
	NiInterpolator::Get(stream);

	stream >> flags;
	stream >> unkInt;
}

void NiBlendInterpolator::Put(NiStream& stream) {
	NiInterpolator::Put(stream);

	stream << flags;
	stream << unkInt;
}

int NiBlendInterpolator::CalcBlockSize(NiVersion& version) {
	NiInterpolator::CalcBlockSize(version);

	blockSize += 6;

	return blockSize;
}


NiBlendBoolInterpolator::NiBlendBoolInterpolator() {
	NiBlendInterpolator::Init();
}

NiBlendBoolInterpolator::NiBlendBoolInterpolator(NiStream& stream) : NiBlendBoolInterpolator() {
	Get(stream);
}

void NiBlendBoolInterpolator::Get(NiStream& stream) {
	NiBlendInterpolator::Get(stream);

	stream >> value;
}

void NiBlendBoolInterpolator::Put(NiStream& stream) {
	NiBlendInterpolator::Put(stream);

	stream << value;
}

int NiBlendBoolInterpolator::CalcBlockSize(NiVersion& version) {
	NiBlendInterpolator::CalcBlockSize(version);

	blockSize += 1;

	return blockSize;
}


NiBlendFloatInterpolator::NiBlendFloatInterpolator() {
	NiBlendInterpolator::Init();
}

NiBlendFloatInterpolator::NiBlendFloatInterpolator(NiStream& stream) : NiBlendFloatInterpolator() {
	Get(stream);
}

void NiBlendFloatInterpolator::Get(NiStream& stream) {
	NiBlendInterpolator::Get(stream);

	stream >> value;
}

void NiBlendFloatInterpolator::Put(NiStream& stream) {
	NiBlendInterpolator::Put(stream);

	stream << value;
}

int NiBlendFloatInterpolator::CalcBlockSize(NiVersion& version) {
	NiBlendInterpolator::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiBlendPoint3Interpolator::NiBlendPoint3Interpolator() {
	NiBlendInterpolator::Init();
}

NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(NiStream& stream) : NiBlendPoint3Interpolator() {
	Get(stream);
}

void NiBlendPoint3Interpolator::Get(NiStream& stream) {
	NiBlendInterpolator::Get(stream);

	stream >> point;
}

void NiBlendPoint3Interpolator::Put(NiStream& stream) {
	NiBlendInterpolator::Put(stream);

	stream << point;
}

int NiBlendPoint3Interpolator::CalcBlockSize(NiVersion& version) {
	NiBlendInterpolator::CalcBlockSize(version);

	blockSize += 12;

	return blockSize;
}


NiBoolInterpolator::NiBoolInterpolator() {
	NiKeyBasedInterpolator::Init();
}

NiBoolInterpolator::NiBoolInterpolator(NiStream& stream) : NiBoolInterpolator() {
	Get(stream);
}

void NiBoolInterpolator::Get(NiStream& stream) {
	NiKeyBasedInterpolator::Get(stream);

	stream >> boolValue;
	dataRef.Get(stream);
}

void NiBoolInterpolator::Put(NiStream& stream) {
	NiKeyBasedInterpolator::Put(stream);

	stream << boolValue;
	dataRef.Put(stream);
}

void NiBoolInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiBoolInterpolator::CalcBlockSize(NiVersion& version) {
	NiKeyBasedInterpolator::CalcBlockSize(version);

	blockSize += 5;

	return blockSize;
}


NiBoolTimelineInterpolator::NiBoolTimelineInterpolator() : NiBoolInterpolator() {
}

NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(NiStream& stream) : NiBoolTimelineInterpolator() {
	Get(stream);
}


NiFloatInterpolator::NiFloatInterpolator() {
	NiKeyBasedInterpolator::Init();
}

NiFloatInterpolator::NiFloatInterpolator(NiStream& stream) : NiFloatInterpolator() {
	Get(stream);
}

void NiFloatInterpolator::Get(NiStream& stream) {
	NiKeyBasedInterpolator::Get(stream);

	stream >> floatValue;
	dataRef.Get(stream);
}

void NiFloatInterpolator::Put(NiStream& stream) {
	NiKeyBasedInterpolator::Put(stream);

	stream << floatValue;
	dataRef.Put(stream);
}

void NiFloatInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiFloatInterpolator::CalcBlockSize(NiVersion& version) {
	NiKeyBasedInterpolator::CalcBlockSize(version);

	blockSize += 8;

	return blockSize;
}


NiTransformInterpolator::NiTransformInterpolator() {
	NiKeyBasedInterpolator::Init();
}

NiTransformInterpolator::NiTransformInterpolator(NiStream& stream) : NiTransformInterpolator() {
	Get(stream);
}

void NiTransformInterpolator::Get(NiStream& stream) {
	NiKeyBasedInterpolator::Get(stream);

	stream >> translation;
	stream >> rotation;
	stream >> scale;
	dataRef.Get(stream);
}

void NiTransformInterpolator::Put(NiStream& stream) {
	NiKeyBasedInterpolator::Put(stream);

	stream << translation;
	stream << rotation;
	stream << scale;
	dataRef.Put(stream);
}

void NiTransformInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiTransformInterpolator::CalcBlockSize(NiVersion& version) {
	NiKeyBasedInterpolator::CalcBlockSize(version);

	blockSize += 36;

	return blockSize;
}


NiPoint3Interpolator::NiPoint3Interpolator() {
	NiKeyBasedInterpolator::Init();
}

NiPoint3Interpolator::NiPoint3Interpolator(NiStream& stream) : NiPoint3Interpolator() {
	Get(stream);
}

void NiPoint3Interpolator::Get(NiStream& stream) {
	NiKeyBasedInterpolator::Get(stream);

	stream >> point3Value;
	dataRef.Get(stream);
}

void NiPoint3Interpolator::Put(NiStream& stream) {
	NiKeyBasedInterpolator::Put(stream);

	stream << point3Value;
	dataRef.Put(stream);
}

void NiPoint3Interpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiPoint3Interpolator::CalcBlockSize(NiVersion& version) {
	NiKeyBasedInterpolator::CalcBlockSize(version);

	blockSize += 16;

	return blockSize;
}


NiPathInterpolator::NiPathInterpolator() {
	NiKeyBasedInterpolator::Init();
}

NiPathInterpolator::NiPathInterpolator(NiStream& stream) : NiPathInterpolator() {
	Get(stream);
}

void NiPathInterpolator::Get(NiStream& stream) {
	NiKeyBasedInterpolator::Get(stream);

	stream >> flags;
	stream >> bankDir;
	stream >> maxBankAngle;
	stream >> smoothing;
	stream >> followAxis;
	pathDataRef.Get(stream);
	percentDataRef.Get(stream);
}

void NiPathInterpolator::Put(NiStream& stream) {
	NiKeyBasedInterpolator::Put(stream);

	stream << flags;
	stream << bankDir;
	stream << maxBankAngle;
	stream << smoothing;
	stream << followAxis;
	pathDataRef.Put(stream);
	percentDataRef.Put(stream);
}

void NiPathInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&pathDataRef.index);
	refs.insert(&percentDataRef.index);
}

int NiPathInterpolator::CalcBlockSize(NiVersion& version) {
	NiKeyBasedInterpolator::CalcBlockSize(version);

	blockSize += 24;

	return blockSize;
}


NiLookAtInterpolator::NiLookAtInterpolator() {
	NiInterpolator::Init();
}

NiLookAtInterpolator::NiLookAtInterpolator(NiStream& stream) : NiLookAtInterpolator() {
	Get(stream);
}

void NiLookAtInterpolator::Get(NiStream& stream) {
	NiInterpolator::Get(stream);
	
	stream >> flags;
	lookAtRef.Get(stream);
	lookAtName.Get(stream);
	stream >> transform;
	translateInterpRef.Get(stream);
	rollInterpRef.Get(stream);
	scaleInterpRef.Get(stream);
}

void NiLookAtInterpolator::Put(NiStream& stream) {
	NiInterpolator::Put(stream);

	stream << flags;
	lookAtRef.Put(stream);
	lookAtName.Put(stream);
	stream << transform;
	translateInterpRef.Put(stream);
	rollInterpRef.Put(stream);
	scaleInterpRef.Put(stream);
}

void NiLookAtInterpolator::GetStringRefs(std::set<StringRef*>& refs) {
	NiInterpolator::GetStringRefs(refs);

	refs.insert(&lookAtName);
}

void NiLookAtInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiInterpolator::GetChildRefs(refs);

	refs.insert(&lookAtRef.index);
	refs.insert(&translateInterpRef.index);
	refs.insert(&rollInterpRef.index);
	refs.insert(&scaleInterpRef.index);
}

int NiLookAtInterpolator::CalcBlockSize(NiVersion& version) {
	NiInterpolator::CalcBlockSize(version);

	blockSize += 54;

	return blockSize;
}


NiSequence::NiSequence() {
	NiObject::Init();
}

NiSequence::NiSequence(NiStream& stream) : NiSequence() {
	Get(stream);
}

void NiSequence::Get(NiStream& stream) {
	NiObject::Get(stream);

	name.Get(stream);

	stream >> numControlledBlocks;
	stream >> unkInt1;

	controlledBlocks.resize(numControlledBlocks);
	for (int i = 0; i < numControlledBlocks; i++) {
		controlledBlocks[i].interpolatorRef.Get(stream);
		controlledBlocks[i].controllerRef.Get(stream);
		stream >> controlledBlocks[i].priority;

		controlledBlocks[i].nodeName.Get(stream);
		controlledBlocks[i].propType.Get(stream);
		controlledBlocks[i].ctrlType.Get(stream);
		controlledBlocks[i].ctrlID.Get(stream);
		controlledBlocks[i].interpID.Get(stream);
	}
}

void NiSequence::Put(NiStream& stream) {
	NiObject::Put(stream);

	name.Put(stream);

	stream << numControlledBlocks;
	stream << unkInt1;

	for (int i = 0; i < numControlledBlocks; i++) {
		controlledBlocks[i].interpolatorRef.Put(stream);
		controlledBlocks[i].controllerRef.Put(stream);
		stream << controlledBlocks[i].priority;
		controlledBlocks[i].nodeName.Put(stream);
		controlledBlocks[i].propType.Put(stream);
		controlledBlocks[i].ctrlType.Put(stream);
		controlledBlocks[i].ctrlID.Put(stream);
		controlledBlocks[i].interpID.Put(stream);
	}
}

void NiSequence::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name);

	for (int i = 0; i < numControlledBlocks; i++) {
		refs.insert(&controlledBlocks[i].nodeName);
		refs.insert(&controlledBlocks[i].propType);
		refs.insert(&controlledBlocks[i].ctrlType);
		refs.insert(&controlledBlocks[i].ctrlID);
		refs.insert(&controlledBlocks[i].interpID);
	}
}

void NiSequence::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	for (int i = 0; i < numControlledBlocks; i++) {
		refs.insert(&controlledBlocks[i].interpolatorRef.index);
		refs.insert(&controlledBlocks[i].controllerRef.index);
	}
}

int NiSequence::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 12;
	blockSize += numControlledBlocks * 29;

	return blockSize;
}


NiControllerSequence::NiControllerSequence() : NiSequence() {
}

NiControllerSequence::NiControllerSequence(NiStream& stream) : NiControllerSequence() {
	Get(stream);
}

void NiControllerSequence::Get(NiStream& stream) {
	NiSequence::Get(stream);

	stream >> weight;
	textKeyRef.Get(stream);
	stream >> cycleType;
	stream >> frequency;
	stream >> startTime;
	stream >> stopTime;
	managerRef.Get(stream);
	accumRootName.Get(stream);
	stream >> flags;
}

void NiControllerSequence::Put(NiStream& stream) {
	NiSequence::Put(stream);

	stream << weight;
	textKeyRef.Put(stream);
	stream << cycleType;
	stream << frequency;
	stream << startTime;
	stream << stopTime;
	managerRef.Put(stream);
	accumRootName.Put(stream);
	stream << flags;
}

void NiControllerSequence::GetStringRefs(std::set<StringRef*>& refs) {
	NiSequence::GetStringRefs(refs);

	refs.insert(&accumRootName);
}

void NiControllerSequence::GetChildRefs(std::set<int*>& refs) {
	NiSequence::GetChildRefs(refs);

	refs.insert(&textKeyRef.index);
	refs.insert(&managerRef.index);
}

int NiControllerSequence::CalcBlockSize(NiVersion& version) {
	NiSequence::CalcBlockSize(version);

	blockSize += 34;

	return blockSize;
}


NiControllerManager::NiControllerManager() {
	NiTimeController::Init();
}

NiControllerManager::NiControllerManager(NiStream& stream) : NiControllerManager() {
	Get(stream);
}

void NiControllerManager::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	stream >> cumulative;

	controllerSequenceRefs.Get(stream);
	objectPaletteRef.Get(stream);
}

void NiControllerManager::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	stream << cumulative;

	controllerSequenceRefs.Put(stream);
	objectPaletteRef.Put(stream);
}

void NiControllerManager::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	controllerSequenceRefs.GetIndexPtrs(refs);
	refs.insert(&objectPaletteRef.index);
}

int NiControllerManager::CalcBlockSize(NiVersion& version) {
	NiTimeController::CalcBlockSize(version);

	blockSize += 9;
	blockSize += controllerSequenceRefs.GetSize() * 4;

	return blockSize;
}
