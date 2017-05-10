/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Animation.h"

NiKeyframeData::NiKeyframeData(NiHeader* hdr) {
	NiObject::Init(hdr);

	numRotationKeys = 0;
}

NiKeyframeData::NiKeyframeData(std::fstream& file, NiHeader* hdr) : NiKeyframeData(hdr) {
	Get(file);
}

void NiKeyframeData::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numRotationKeys, 4);

	if (numRotationKeys > 0) {
		file.read((char*)&rotationType, 4);

		if (rotationType != 4) {
			quaternionKeys.resize(numRotationKeys);

			for (int i = 0; i < numRotationKeys; i++) {
				file.read((char*)&quaternionKeys[i].time, 4);
				file.read((char*)&quaternionKeys[i].value, 16);

				if (rotationType == 3)
					file.read((char*)&quaternionKeys[i].tbc, 12);
			}
		}
		else {
			xRotations.Get(file);
			yRotations.Get(file);
			zRotations.Get(file);
		}
	}

	translations.Get(file);
	scales.Get(file);
}

void NiKeyframeData::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numRotationKeys, 4);

	if (numRotationKeys > 0) {
		file.write((char*)&rotationType, 4);

		if (rotationType != 4) {
			for (int i = 0; i < numRotationKeys; i++) {
				file.write((char*)&quaternionKeys[i].time, 4);
				file.write((char*)&quaternionKeys[i].value, 16);

				if (rotationType == 3)
					file.write((char*)&quaternionKeys[i].tbc, 12);
			}
		}
		else {
			xRotations.Put(file);
			yRotations.Put(file);
			zRotations.Put(file);
		}
	}

	translations.Put(file);
	scales.Put(file);
}

int NiKeyframeData::CalcBlockSize() {
	NiObject::CalcBlockSize();

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


NiTransformData::NiTransformData(NiHeader* hdr) : NiKeyframeData(hdr) {
}

NiTransformData::NiTransformData(std::fstream& file, NiHeader* hdr) : NiTransformData(hdr) {
	Get(file);
}


NiPosData::NiPosData(NiHeader* hdr) {
	NiObject::Init(hdr);
}

NiPosData::NiPosData(std::fstream& file, NiHeader* hdr) : NiPosData(hdr) {
	Get(file);
}

void NiPosData::Get(std::fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiPosData::Put(std::fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiPosData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiBoolData::NiBoolData(NiHeader* hdr) {
	NiObject::Init(hdr);
}

NiBoolData::NiBoolData(std::fstream& file, NiHeader* hdr) : NiBoolData(hdr) {
	Get(file);
}

void NiBoolData::Get(std::fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiBoolData::Put(std::fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiBoolData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiFloatData::NiFloatData(NiHeader* hdr) {
	NiObject::Init(hdr);
}

NiFloatData::NiFloatData(std::fstream& file, NiHeader* hdr) : NiFloatData(hdr) {
	Get(file);
}

void NiFloatData::Get(std::fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiFloatData::Put(std::fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiFloatData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


void NiTimeController::Init(NiHeader* hdr) {
	NiObject::Init(hdr);

	flags = 0x000C;
	frequency = 1.0f;
	phase = 0.0f;
	startTime = 0.0f;
	stopTime = 0.0f;
}

void NiTimeController::Get(std::fstream& file) {
	NiObject::Get(file);

	nextControllerRef.Get(file);
	file.read((char*)&flags, 2);
	file.read((char*)&frequency, 4);
	file.read((char*)&phase, 4);
	file.read((char*)&startTime, 4);
	file.read((char*)&stopTime, 4);
	targetRef.Get(file);
}

void NiTimeController::Put(std::fstream& file) {
	NiObject::Put(file);

	nextControllerRef.Put(file);
	file.write((char*)&flags, 2);
	file.write((char*)&frequency, 4);
	file.write((char*)&phase, 4);
	file.write((char*)&startTime, 4);
	file.write((char*)&stopTime, 4);
	targetRef.Put(file);
}

void NiTimeController::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&nextControllerRef.index);
}

int NiTimeController::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 26;

	return blockSize;
}


BSFrustumFOVController::BSFrustumFOVController(NiHeader* hdr) {
	NiTimeController::Init(hdr);
}

BSFrustumFOVController::BSFrustumFOVController(std::fstream& file, NiHeader* hdr) : BSFrustumFOVController(hdr) {
	Get(file);
}

void BSFrustumFOVController::Get(std::fstream& file) {
	NiTimeController::Get(file);

	interpolatorRef.Get(file);
}

void BSFrustumFOVController::Put(std::fstream& file) {
	NiTimeController::Put(file);

	interpolatorRef.Put(file);
}

void BSFrustumFOVController::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(&interpolatorRef.index);
}

int BSFrustumFOVController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSLagBoneController::BSLagBoneController(NiHeader* hdr) {
	NiTimeController::Init(hdr);
}

BSLagBoneController::BSLagBoneController(std::fstream& file, NiHeader* hdr) : BSLagBoneController(hdr) {
	Get(file);
}

void BSLagBoneController::Get(std::fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&linearVelocity, 4);
	file.read((char*)&linearRotation, 4);
	file.read((char*)&maxDistance, 4);
}

void BSLagBoneController::Put(std::fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&linearVelocity, 4);
	file.write((char*)&linearRotation, 4);
	file.write((char*)&maxDistance, 4);
}

int BSLagBoneController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


BSProceduralLightningController::BSProceduralLightningController(NiHeader* hdr) {
	NiTimeController::Init(hdr);
}

BSProceduralLightningController::BSProceduralLightningController(std::fstream& file, NiHeader* hdr) : BSProceduralLightningController(hdr) {
	Get(file);
}

void BSProceduralLightningController::Get(std::fstream& file) {
	NiTimeController::Get(file);

	generationInterpRef.Get(file);
	mutationInterpRef.Get(file);
	subdivisionInterpRef.Get(file);
	numBranchesInterpRef.Get(file);
	numBranchesVarInterpRef.Get(file);
	lengthInterpRef.Get(file);
	lengthVarInterpRef.Get(file);
	widthInterpRef.Get(file);
	arcOffsetInterpRef.Get(file);

	file.read((char*)&subdivisions, 2);
	file.read((char*)&numBranches, 2);
	file.read((char*)&numBranchesPerVariation, 2);

	file.read((char*)&length, 4);
	file.read((char*)&lengthVariation, 4);
	file.read((char*)&width, 4);
	file.read((char*)&childWidthMult, 4);
	file.read((char*)&arcOffset, 4);

	file.read((char*)&fadeMainBolt, 1);
	file.read((char*)&fadeChildBolts, 1);
	file.read((char*)&animateArcOffset, 1);

	shaderPropertyRef.Get(file);
}

void BSProceduralLightningController::Put(std::fstream& file) {
	NiTimeController::Put(file);

	generationInterpRef.Put(file);
	mutationInterpRef.Put(file);
	subdivisionInterpRef.Put(file);
	numBranchesInterpRef.Put(file);
	numBranchesVarInterpRef.Put(file);
	lengthInterpRef.Put(file);
	lengthVarInterpRef.Put(file);
	widthInterpRef.Put(file);
	arcOffsetInterpRef.Put(file);

	file.write((char*)&subdivisions, 2);
	file.write((char*)&numBranches, 2);
	file.write((char*)&numBranchesPerVariation, 2);

	file.write((char*)&length, 4);
	file.write((char*)&lengthVariation, 4);
	file.write((char*)&width, 4);
	file.write((char*)&childWidthMult, 4);
	file.write((char*)&arcOffset, 4);

	file.write((char*)&fadeMainBolt, 1);
	file.write((char*)&fadeChildBolts, 1);
	file.write((char*)&animateArcOffset, 1);

	shaderPropertyRef.Put(file);
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

int BSProceduralLightningController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 69;

	return blockSize;
}


NiBoneLODController::NiBoneLODController(NiHeader* hdr) {
	NiTimeController::Init(hdr);

	numLODs = 0;
	boneArraysSize = 0;
}

NiBoneLODController::NiBoneLODController(std::fstream& file, NiHeader* hdr) : NiBoneLODController(hdr) {
	Get(file);
}

void NiBoneLODController::Get(std::fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&lod, 4);
	file.read((char*)&numLODs, 4);

	file.read((char*)&boneArraysSize, 4);
	boneArrays.resize(boneArraysSize);
	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].Get(file);
}

void NiBoneLODController::Put(std::fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&lod, 4);
	file.write((char*)&numLODs, 4);

	file.write((char*)&boneArraysSize, 4);
	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].Put(file);
}

void NiBoneLODController::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	for (int i = 0; i < boneArraysSize; i++)
		boneArrays[i].GetIndexPtrs(refs);
}

int NiBoneLODController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 12;
	blockSize += boneArraysSize * 4;
	for (int i = 0; i < boneArraysSize; i++)
		blockSize += boneArrays[i].GetSize() * 4;

	return blockSize;
}


void NiSingleInterpController::Init(NiHeader* hdr) {
	NiInterpController::Init(hdr);
}

void NiSingleInterpController::Get(std::fstream& file) {
	NiInterpController::Get(file);

	interpolatorRef.Get(file);
}

void NiSingleInterpController::Put(std::fstream& file) {
	NiInterpController::Put(file);

	interpolatorRef.Put(file);
}

void NiSingleInterpController::GetChildRefs(std::set<int*>& refs) {
	NiInterpController::GetChildRefs(refs);

	refs.insert(&interpolatorRef.index);
}

int NiSingleInterpController::CalcBlockSize() {
	NiInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiFloatExtraDataController::NiFloatExtraDataController(NiHeader* hdr) {
	NiExtraDataController::Init(hdr);
}

NiFloatExtraDataController::NiFloatExtraDataController(std::fstream& file, NiHeader* hdr) : NiFloatExtraDataController(hdr) {
	Get(file);
}

void NiFloatExtraDataController::Get(std::fstream& file) {
	NiExtraDataController::Get(file);

	extraData.Get(file, header);
}

void NiFloatExtraDataController::Put(std::fstream& file) {
	NiExtraDataController::Put(file);

	extraData.Put(file);
}

void NiFloatExtraDataController::notifyStringDelete(int stringID) {
	NiExtraDataController::notifyStringDelete(stringID);

	extraData.notifyStringDelete(stringID);
}

int NiFloatExtraDataController::CalcBlockSize() {
	NiExtraDataController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiVisController::NiVisController(NiHeader* hdr) {
	NiBoolInterpController::Init(hdr);
}

NiVisController::NiVisController(std::fstream& file, NiHeader* hdr) : NiVisController(hdr) {
	Get(file);
}


NiAlphaController::NiAlphaController(NiHeader* hdr) {
	NiFloatInterpController::Init(hdr);
}

NiAlphaController::NiAlphaController(std::fstream& file, NiHeader* hdr) : NiAlphaController(hdr) {
	Get(file);
}


NiPSysUpdateCtlr::NiPSysUpdateCtlr(NiHeader* hdr) {
	NiTimeController::Init(hdr);
}

NiPSysUpdateCtlr::NiPSysUpdateCtlr(std::fstream& file, NiHeader* hdr) : NiPSysUpdateCtlr(hdr) {
	Get(file);
}


BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController(NiHeader* hdr) : NiAlphaController(hdr) {
}

BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController(std::fstream& file, NiHeader* hdr) : BSNiAlphaPropertyTestRefController(hdr) {
	Get(file);
}


NiKeyframeController::NiKeyframeController(NiHeader* hdr) {
	NiSingleInterpController::Init(hdr);
}

NiKeyframeController::NiKeyframeController(std::fstream& file, NiHeader* hdr) : NiKeyframeController(hdr) {
	Get(file);
}


NiTransformController::NiTransformController(NiHeader* hdr) : NiKeyframeController(hdr) {
}

NiTransformController::NiTransformController(std::fstream& file, NiHeader* hdr) : NiTransformController(hdr) {
	Get(file);
}


BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(NiHeader* hdr) {
	NiFloatInterpController::Init(hdr);

	typeOfControlledColor = 0;
}

BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(std::fstream& file, NiHeader* hdr) : BSLightingShaderPropertyColorController(hdr) {
	Get(file);
}

void BSLightingShaderPropertyColorController::Get(std::fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledColor, 4);
}

void BSLightingShaderPropertyColorController::Put(std::fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledColor, 4);
}

int BSLightingShaderPropertyColorController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(NiHeader* hdr) {
	NiFloatInterpController::Init(hdr);

	typeOfControlledVariable = 0;
}

BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(std::fstream& file, NiHeader* hdr) : BSLightingShaderPropertyFloatController(hdr) {
	Get(file);
}

void BSLightingShaderPropertyFloatController::Get(std::fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledVariable, 4);
}

void BSLightingShaderPropertyFloatController::Put(std::fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledVariable, 4);
}

int BSLightingShaderPropertyFloatController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(NiHeader* hdr) {
	NiFloatInterpController::Init(hdr);

	typeOfControlledColor = 0;
}

BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(std::fstream& file, NiHeader* hdr) : BSEffectShaderPropertyColorController(hdr) {
	Get(file);
}

void BSEffectShaderPropertyColorController::Get(std::fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledColor, 4);
}

void BSEffectShaderPropertyColorController::Put(std::fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledColor, 4);
}

int BSEffectShaderPropertyColorController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(NiHeader* hdr) {
	NiFloatInterpController::Init(hdr);

	typeOfControlledVariable = 0;
}

BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(std::fstream& file, NiHeader* hdr) : BSEffectShaderPropertyFloatController(hdr) {
	Get(file);
}

void BSEffectShaderPropertyFloatController::Get(std::fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledVariable, 4);
}

void BSEffectShaderPropertyFloatController::Put(std::fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledVariable, 4);
}

int BSEffectShaderPropertyFloatController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiMultiTargetTransformController::NiMultiTargetTransformController(NiHeader* hdr) {
	NiInterpController::Init(hdr);
}

NiMultiTargetTransformController::NiMultiTargetTransformController(std::fstream& file, NiHeader* hdr) : NiMultiTargetTransformController(hdr) {
	Get(file);
}

void NiMultiTargetTransformController::Get(std::fstream& file) {
	NiInterpController::Get(file);

	targetRefs.Get(file);
}

void NiMultiTargetTransformController::Put(std::fstream& file) {
	NiInterpController::Put(file);

	targetRefs.Put(file);
}

int NiMultiTargetTransformController::CalcBlockSize() {
	NiInterpController::CalcBlockSize();

	blockSize += 2;
	blockSize += targetRefs.GetSize() * 4;

	return blockSize;
}


void NiPSysModifierCtlr::Get(std::fstream& file) {
	NiSingleInterpController::Get(file);

	modifierName.Get(file, header);
}

void NiPSysModifierCtlr::Put(std::fstream& file) {
	NiSingleInterpController::Put(file);

	modifierName.Put(file);
}

void NiPSysModifierCtlr::notifyStringDelete(int stringID) {
	NiSingleInterpController::notifyStringDelete(stringID);

	modifierName.notifyStringDelete(stringID);
}

int NiPSysModifierCtlr::CalcBlockSize() {
	NiSingleInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(std::fstream& file, NiHeader* hdr) : NiPSysModifierActiveCtlr(hdr) {
	Get(file);
}


NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterLifeSpanCtlr(hdr) {
	Get(file);
}


NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterSpeedCtlr(hdr) {
	Get(file);
}


NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterInitialRadiusCtlr(hdr) {
	Get(file);
}


NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterPlanarAngleCtlr(hdr) {
	Get(file);
}


NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterDeclinationCtlr(hdr) {
	Get(file);
}


NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr(std::fstream& file, NiHeader* hdr) : NiPSysGravityStrengthCtlr(hdr) {
	Get(file);
}


NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr(std::fstream& file, NiHeader* hdr) : NiPSysInitialRotSpeedCtlr(hdr) {
	Get(file);
}


NiPSysEmitterCtlr::NiPSysEmitterCtlr(NiHeader* hdr) {
	NiPSysModifierCtlr::Init(hdr);
}

NiPSysEmitterCtlr::NiPSysEmitterCtlr(std::fstream& file, NiHeader* hdr) : NiPSysEmitterCtlr(hdr) {
	Get(file);
}

void NiPSysEmitterCtlr::Get(std::fstream& file) {
	NiPSysModifierCtlr::Get(file);

	visInterpolatorRef.Get(file);
}

void NiPSysEmitterCtlr::Put(std::fstream& file) {
	NiPSysModifierCtlr::Put(file);

	visInterpolatorRef.Put(file);
}

void NiPSysEmitterCtlr::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifierCtlr::GetChildRefs(refs);

	refs.insert(&visInterpolatorRef.index);
}

int NiPSysEmitterCtlr::CalcBlockSize() {
	NiPSysModifierCtlr::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr(NiHeader* hdr) : NiPSysEmitterCtlr(hdr) {
}

BSPSysMultiTargetEmitterCtlr::BSPSysMultiTargetEmitterCtlr(std::fstream& file, NiHeader* hdr) : BSPSysMultiTargetEmitterCtlr(hdr) {
	Get(file);
}

void BSPSysMultiTargetEmitterCtlr::Get(std::fstream& file) {
	NiPSysEmitterCtlr::Get(file);

	file.read((char*)&maxEmitters, 2);
	masterParticleSystemRef.Get(file);
}

void BSPSysMultiTargetEmitterCtlr::Put(std::fstream& file) {
	NiPSysEmitterCtlr::Put(file);

	file.write((char*)&maxEmitters, 2);
	masterParticleSystemRef.Put(file);
}

void BSPSysMultiTargetEmitterCtlr::GetChildRefs(std::set<int*>& refs) {
	NiPSysEmitterCtlr::GetChildRefs(refs);

	refs.insert(&masterParticleSystemRef.index);
}

int BSPSysMultiTargetEmitterCtlr::CalcBlockSize() {
	NiPSysEmitterCtlr::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


void NiBlendInterpolator::Get(std::fstream& file) {
	NiInterpolator::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&unkInt, 4);
}

void NiBlendInterpolator::Put(std::fstream& file) {
	NiInterpolator::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&unkInt, 4);
}

int NiBlendInterpolator::CalcBlockSize() {
	NiInterpolator::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


NiBlendBoolInterpolator::NiBlendBoolInterpolator(NiHeader* hdr) {
	NiBlendInterpolator::Init(hdr);

	value = false;
}

NiBlendBoolInterpolator::NiBlendBoolInterpolator(std::fstream& file, NiHeader* hdr) : NiBlendBoolInterpolator(hdr) {
	Get(file);
}

void NiBlendBoolInterpolator::Get(std::fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&value, 1);
}

void NiBlendBoolInterpolator::Put(std::fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&value, 1);
}

int NiBlendBoolInterpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 1;

	return blockSize;
}


NiBlendFloatInterpolator::NiBlendFloatInterpolator(NiHeader* hdr) {
	NiBlendInterpolator::Init(hdr);

	value = 0.0f;
}

NiBlendFloatInterpolator::NiBlendFloatInterpolator(std::fstream& file, NiHeader* hdr) : NiBlendFloatInterpolator(hdr) {
	Get(file);
}

void NiBlendFloatInterpolator::Get(std::fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&value, 4);
}

void NiBlendFloatInterpolator::Put(std::fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&value, 4);
}

int NiBlendFloatInterpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(NiHeader* hdr) {
	NiBlendInterpolator::Init(hdr);
}

NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(std::fstream& file, NiHeader* hdr) : NiBlendPoint3Interpolator(hdr) {
	Get(file);
}

void NiBlendPoint3Interpolator::Get(std::fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&point, 12);
}

void NiBlendPoint3Interpolator::Put(std::fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&point, 12);
}

int NiBlendPoint3Interpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


NiBoolInterpolator::NiBoolInterpolator(NiHeader* hdr) {
	NiKeyBasedInterpolator::Init(hdr);
}

NiBoolInterpolator::NiBoolInterpolator(std::fstream& file, NiHeader* hdr) : NiBoolInterpolator(hdr) {
	Get(file);
}

void NiBoolInterpolator::Get(std::fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&boolValue, 1);
	dataRef.Get(file);
}

void NiBoolInterpolator::Put(std::fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&boolValue, 1);
	dataRef.Put(file);
}

void NiBoolInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiBoolInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(NiHeader* hdr) : NiBoolInterpolator(hdr) {
}

NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(std::fstream& file, NiHeader* hdr) : NiBoolTimelineInterpolator(hdr) {
	Get(file);
}


NiFloatInterpolator::NiFloatInterpolator(NiHeader* hdr) {
	NiKeyBasedInterpolator::Init(hdr);
}

NiFloatInterpolator::NiFloatInterpolator(std::fstream& file, NiHeader* hdr) : NiFloatInterpolator(hdr) {
	Get(file);
}

void NiFloatInterpolator::Get(std::fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&floatValue, 4);
	dataRef.Get(file);
}

void NiFloatInterpolator::Put(std::fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&floatValue, 4);
	dataRef.Put(file);
}

void NiFloatInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiFloatInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


NiTransformInterpolator::NiTransformInterpolator(NiHeader* hdr) {
	NiKeyBasedInterpolator::Init(hdr);
}

NiTransformInterpolator::NiTransformInterpolator(std::fstream& file, NiHeader* hdr) : NiTransformInterpolator(hdr) {
	Get(file);
}

void NiTransformInterpolator::Get(std::fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&translation, 12);
	file.read((char*)&rotation, 16);
	file.read((char*)&scale, 4);
	dataRef.Get(file);
}

void NiTransformInterpolator::Put(std::fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&translation, 12);
	file.write((char*)&rotation, 16);
	file.write((char*)&scale, 4);
	dataRef.Put(file);
}

void NiTransformInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiTransformInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 36;

	return blockSize;
}


NiPoint3Interpolator::NiPoint3Interpolator(NiHeader* hdr) {
	NiKeyBasedInterpolator::Init(hdr);
}

NiPoint3Interpolator::NiPoint3Interpolator(std::fstream& file, NiHeader* hdr) : NiPoint3Interpolator(hdr) {
	Get(file);
}

void NiPoint3Interpolator::Get(std::fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&point3Value, 12);
	dataRef.Get(file);
}

void NiPoint3Interpolator::Put(std::fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&point3Value, 12);
	dataRef.Put(file);
}

void NiPoint3Interpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int NiPoint3Interpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


NiPathInterpolator::NiPathInterpolator(NiHeader* hdr) {
	NiKeyBasedInterpolator::Init(hdr);
}

NiPathInterpolator::NiPathInterpolator(std::fstream& file, NiHeader* hdr) : NiPathInterpolator(hdr) {
	Get(file);
}

void NiPathInterpolator::Get(std::fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&bankDir, 4);
	file.read((char*)&maxBankAngle, 4);
	file.read((char*)&smoothing, 4);
	file.read((char*)&followAxis, 2);
	pathDataRef.Get(file);
	percentDataRef.Get(file);
}

void NiPathInterpolator::Put(std::fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&bankDir, 4);
	file.write((char*)&maxBankAngle, 4);
	file.write((char*)&smoothing, 4);
	file.write((char*)&followAxis, 2);
	pathDataRef.Put(file);
	percentDataRef.Put(file);
}

void NiPathInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(&pathDataRef.index);
	refs.insert(&percentDataRef.index);
}

int NiPathInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


NiLookAtInterpolator::NiLookAtInterpolator(NiHeader* hdr) {
	NiInterpolator::Init(hdr);
}

NiLookAtInterpolator::NiLookAtInterpolator(std::fstream& file, NiHeader* hdr) : NiLookAtInterpolator(hdr) {
	Get(file);
}

void NiLookAtInterpolator::Get(std::fstream& file) {
	NiInterpolator::Get(file);

	file.read((char*)&flags, 2);
	lookAtRef.Get(file);
	lookAtName.Get(file, header);
	file.read((char*)&transform, 32);
	translateInterpRef.Get(file);
	rollInterpRef.Get(file);
	scaleInterpRef.Get(file);
}

void NiLookAtInterpolator::Put(std::fstream& file) {
	NiInterpolator::Put(file);

	file.write((char*)&flags, 2);
	lookAtRef.Put(file);
	lookAtName.Put(file);
	file.write((char*)&transform, 32);
	translateInterpRef.Put(file);
	rollInterpRef.Put(file);
	scaleInterpRef.Put(file);
}

void NiLookAtInterpolator::notifyStringDelete(int stringID) {
	NiInterpolator::notifyStringDelete(stringID);

	lookAtName.notifyStringDelete(stringID);
}

void NiLookAtInterpolator::GetChildRefs(std::set<int*>& refs) {
	NiInterpolator::GetChildRefs(refs);

	refs.insert(&lookAtRef.index);
	refs.insert(&translateInterpRef.index);
	refs.insert(&rollInterpRef.index);
	refs.insert(&scaleInterpRef.index);
}

int NiLookAtInterpolator::CalcBlockSize() {
	NiInterpolator::CalcBlockSize();

	blockSize += 54;

	return blockSize;
}


NiSequence::NiSequence(NiHeader* hdr) {
	NiObject::Init(hdr);

	numControlledBlocks = 0;
}

NiSequence::NiSequence(std::fstream& file, NiHeader* hdr) : NiSequence(hdr) {
	Get(file);
}

void NiSequence::Get(std::fstream& file) {
	NiObject::Get(file);

	name.Get(file, header);

	file.read((char*)&numControlledBlocks, 4);
	file.read((char*)&unkInt1, 4);

	controlledBlocks.resize(numControlledBlocks);
	for (int i = 0; i < numControlledBlocks; i++) {
		controlledBlocks[i].interpolatorRef.Get(file);
		controlledBlocks[i].controllerRef.Get(file);
		file.read((char*)&controlledBlocks[i].priority, 1);

		controlledBlocks[i].nodeName.Get(file, header);
		controlledBlocks[i].propType.Get(file, header);
		controlledBlocks[i].ctrlType.Get(file, header);
		controlledBlocks[i].ctrlID.Get(file, header);
		controlledBlocks[i].interpID.Get(file, header);
	}
}

void NiSequence::Put(std::fstream& file) {
	NiObject::Put(file);

	name.Put(file);
	file.write((char*)&numControlledBlocks, 4);
	file.write((char*)&unkInt1, 4);

	for (int i = 0; i < numControlledBlocks; i++) {
		controlledBlocks[i].interpolatorRef.Put(file);
		controlledBlocks[i].controllerRef.Put(file);
		file.write((char*)&controlledBlocks[i].priority, 1);
		controlledBlocks[i].nodeName.Put(file);
		controlledBlocks[i].propType.Put(file);
		controlledBlocks[i].ctrlType.Put(file);
		controlledBlocks[i].ctrlID.Put(file);
		controlledBlocks[i].interpID.Put(file);
	}
}

void NiSequence::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	name.notifyStringDelete(stringID);

	for (int i = 0; i < numControlledBlocks; i++) {
		controlledBlocks[i].nodeName.notifyStringDelete(stringID);
		controlledBlocks[i].propType.notifyStringDelete(stringID);
		controlledBlocks[i].ctrlType.notifyStringDelete(stringID);
		controlledBlocks[i].ctrlID.notifyStringDelete(stringID);
		controlledBlocks[i].interpID.notifyStringDelete(stringID);
	}
}

void NiSequence::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	for (int i = 0; i < numControlledBlocks; i++) {
		refs.insert(&controlledBlocks[i].interpolatorRef.index);
		refs.insert(&controlledBlocks[i].controllerRef.index);
	}
}

int NiSequence::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 12;
	blockSize += numControlledBlocks * 29;

	return blockSize;
}


NiControllerSequence::NiControllerSequence(NiHeader* hdr) : NiSequence(hdr) {
}

NiControllerSequence::NiControllerSequence(std::fstream& file, NiHeader* hdr) : NiControllerSequence(hdr) {
	Get(file);
}

void NiControllerSequence::Get(std::fstream& file) {
	NiSequence::Get(file);

	file.read((char*)&weight, 4);
	textKeyRef.Get(file);
	file.read((char*)&textKeyRef, 4);
	file.read((char*)&cycleType, 4);
	file.read((char*)&frequency, 4);
	file.read((char*)&startTime, 4);
	file.read((char*)&stopTime, 4);
	managerRef.Get(file);
	accumRootName.Get(file, header);
	file.read((char*)&flags, 2);
}

void NiControllerSequence::Put(std::fstream& file) {
	NiSequence::Put(file);

	file.write((char*)&weight, 4);
	textKeyRef.Put(file);
	file.write((char*)&cycleType, 4);
	file.write((char*)&frequency, 4);
	file.write((char*)&startTime, 4);
	file.write((char*)&stopTime, 4);
	managerRef.Put(file);
	accumRootName.Put(file);
	file.write((char*)&flags, 2);
}

void NiControllerSequence::notifyStringDelete(int stringID) {
	NiSequence::notifyStringDelete(stringID);

	accumRootName.notifyStringDelete(stringID);
}

void NiControllerSequence::GetChildRefs(std::set<int*>& refs) {
	NiSequence::GetChildRefs(refs);

	refs.insert(&textKeyRef.index);
	refs.insert(&managerRef.index);
}

int NiControllerSequence::CalcBlockSize() {
	NiSequence::CalcBlockSize();

	blockSize += 34;

	return blockSize;
}


NiControllerManager::NiControllerManager(NiHeader* hdr) {
	NiTimeController::Init(hdr);
}

NiControllerManager::NiControllerManager(std::fstream& file, NiHeader* hdr) : NiControllerManager(hdr) {
	Get(file);
}

void NiControllerManager::Get(std::fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&cumulative, 1);

	controllerSequenceRefs.Get(file);
	objectPaletteRef.Get(file);
}

void NiControllerManager::Put(std::fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&cumulative, 1);

	controllerSequenceRefs.Put(file);
	objectPaletteRef.Put(file);
}

void NiControllerManager::GetChildRefs(std::set<int*>& refs) {
	NiTimeController::GetChildRefs(refs);

	controllerSequenceRefs.GetIndexPtrs(refs);
	refs.insert(&objectPaletteRef.index);
}

int NiControllerManager::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 9;
	blockSize += controllerSequenceRefs.GetSize() * 4;

	return blockSize;
}
