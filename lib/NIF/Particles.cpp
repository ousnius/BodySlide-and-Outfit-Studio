/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Particles.h"

NiParticlesData::NiParticlesData(NiHeader* hdr) {
	NiGeometryData::Init(hdr);
	NiGeometryData::isPSys = true;

	hasRadii = false;
	numActive = 0;
	hasSizes = false;
	hasRotations = false;
	hasRotationAngles = false;
	hasRotationAxes = false;
	hasTextureIndices = false;
	numSubtexOffsets = 0;
}

NiParticlesData::NiParticlesData(std::fstream& file, NiHeader* hdr) : NiParticlesData(hdr) {
	Get(file);
}

void NiParticlesData::Get(std::fstream& file) {
	NiGeometryData::Get(file);

	file.read((char*)&hasRadii, 1);
	file.read((char*)&numActive, 2);
	file.read((char*)&hasSizes, 1);
	file.read((char*)&hasRotations, 1);
	file.read((char*)&hasRotationAngles, 1);
	file.read((char*)&hasRotationAxes, 1);
	file.read((char*)&hasTextureIndices, 1);

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&numSubtexOffsets, 4);
	}
	else {
		byte numOffsets = 0;
		file.read((char*)&numOffsets, 1);
		numSubtexOffsets = numOffsets;
	}

	subtexOffsets.resize(numSubtexOffsets);
	for (int i = 0; i < numSubtexOffsets; i++)
		file.read((char*)&subtexOffsets[i], 16);

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&aspectRatio, 4);
		file.read((char*)&aspectFlags, 2);
		file.read((char*)&speedToAspectAspect2, 4);
		file.read((char*)&speedToAspectSpeed1, 4);
		file.read((char*)&speedToAspectSpeed2, 4);
	}
}

void NiParticlesData::Put(std::fstream& file) {
	NiGeometryData::Put(file);

	file.write((char*)&hasRadii, 1);
	file.write((char*)&numActive, 2);
	file.write((char*)&hasSizes, 1);
	file.write((char*)&hasRotations, 1);
	file.write((char*)&hasRotationAngles, 1);
	file.write((char*)&hasRotationAxes, 1);
	file.write((char*)&hasTextureIndices, 1);

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&numSubtexOffsets, 4);
	}
	else {
		byte numOffsets = numSubtexOffsets > 255 ? 255 : numSubtexOffsets;
		file.write((char*)&numOffsets, 1);
	}

	for (int i = 0; i < numSubtexOffsets; i++)
		file.write((char*)&subtexOffsets[i], 16);

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&aspectRatio, 4);
		file.write((char*)&aspectFlags, 2);
		file.write((char*)&speedToAspectAspect2, 4);
		file.write((char*)&speedToAspectSpeed1, 4);
		file.write((char*)&speedToAspectSpeed2, 4);
	}
}

int NiParticlesData::CalcBlockSize() {
	NiGeometryData::CalcBlockSize();

	blockSize += 8;

	if (header->GetUserVersion() >= 12)
		blockSize += 22;
	else
		blockSize += 1;

	blockSize += numSubtexOffsets * 16;

	return blockSize;
}


NiRotatingParticlesData::NiRotatingParticlesData(NiHeader* hdr) : NiParticlesData(hdr) {
}

NiRotatingParticlesData::NiRotatingParticlesData(std::fstream& file, NiHeader* hdr) : NiRotatingParticlesData(hdr) {
	Get(file);
}


NiPSysData::NiPSysData(NiHeader* hdr) : NiRotatingParticlesData(hdr) {
	hasRotationSpeeds = false;
}

NiPSysData::NiPSysData(std::fstream& file, NiHeader* hdr) : NiPSysData(hdr) {
	Get(file);
}

void NiPSysData::Get(std::fstream& file) {
	NiRotatingParticlesData::Get(file);

	file.read((char*)&hasRotationSpeeds, 1);
}

void NiPSysData::Put(std::fstream& file) {
	NiRotatingParticlesData::Put(file);

	file.write((char*)&hasRotationSpeeds, 1);
}

int NiPSysData::CalcBlockSize() {
	NiRotatingParticlesData::CalcBlockSize();

	blockSize += 1;

	return blockSize;
}


NiMeshPSysData::NiMeshPSysData(NiHeader* hdr) : NiPSysData(hdr) {
	nodeRef = 0xFFFFFFFF;
}

NiMeshPSysData::NiMeshPSysData(std::fstream& file, NiHeader* hdr) : NiMeshPSysData(hdr) {
	Get(file);
}

void NiMeshPSysData::Get(std::fstream& file) {
	NiPSysData::Get(file);

	file.read((char*)&defaultPoolSize, 4);
	file.read((char*)&fillPoolsOnLoad, 1);

	file.read((char*)&numGenerations, 4);
	generationPoolSize.resize(numGenerations);
	for (int i = 0; i < numGenerations; i++)
		file.read((char*)&generationPoolSize[i], 4);

	file.read((char*)&nodeRef, 4);
}

void NiMeshPSysData::Put(std::fstream& file) {
	NiPSysData::Put(file);

	file.write((char*)&defaultPoolSize, 4);
	file.write((char*)&fillPoolsOnLoad, 1);

	file.write((char*)&numGenerations, 4);
	for (int i = 0; i < numGenerations; i++)
		file.write((char*)&generationPoolSize[i], 4);

	file.write((char*)&nodeRef, 4);
}

void NiMeshPSysData::GetChildRefs(std::set<int*>& refs) {
	NiPSysData::GetChildRefs(refs);

	refs.insert(&nodeRef);
}

int NiMeshPSysData::CalcBlockSize() {
	NiPSysData::CalcBlockSize();

	blockSize += 9;
	blockSize += numGenerations * 4;

	return blockSize;
}


BSStripPSysData::BSStripPSysData(NiHeader* hdr) : NiPSysData(hdr) {
}

BSStripPSysData::BSStripPSysData(std::fstream& file, NiHeader* hdr) : BSStripPSysData(hdr) {
	Get(file);
}

void BSStripPSysData::Get(std::fstream& file) {
	NiPSysData::Get(file);

	file.read((char*)&maxPointCount, 2);
	file.read((char*)&startCapSize, 4);
	file.read((char*)&endCapSize, 4);
	file.read((char*)&doZPrepass, 1);
}

void BSStripPSysData::Put(std::fstream& file) {
	NiPSysData::Put(file);

	file.write((char*)&maxPointCount, 2);
	file.write((char*)&startCapSize, 4);
	file.write((char*)&endCapSize, 4);
	file.write((char*)&doZPrepass, 1);
}

int BSStripPSysData::CalcBlockSize() {
	NiPSysData::CalcBlockSize();

	blockSize += 11;

	return blockSize;
}


void NiPSysModifier::Init(NiHeader* hdr) {
	NiObject::Init(hdr);
}

void NiPSysModifier::Get(std::fstream& file) {
	NiObject::Get(file);

	name.Get(file, header);

	file.read((char*)&order, 4);
	targetRef.Get(file);
	file.read((char*)&isActive, 1);
}

void NiPSysModifier::Put(std::fstream& file) {
	NiObject::Put(file);

	name.Put(file);
	file.write((char*)&order, 4);
	targetRef.Put(file);
	file.write((char*)&isActive, 1);
}

void NiPSysModifier::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	name.notifyStringDelete(stringID);
}

int NiPSysModifier::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 13;

	return blockSize;
}


BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(std::fstream& file, NiHeader* hdr) : BSPSysStripUpdateModifier(hdr) {
	Get(file);
}

void BSPSysStripUpdateModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&updateDeltaTime, 4);
}

void BSPSysStripUpdateModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&updateDeltaTime, 4);
}

int BSPSysStripUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysSpawnModifier::NiPSysSpawnModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);

	numSpawnGenerations = 0;
}

NiPSysSpawnModifier::NiPSysSpawnModifier(std::fstream& file, NiHeader* hdr) : NiPSysSpawnModifier(hdr) {
	Get(file);
}

void NiPSysSpawnModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&numSpawnGenerations, 2);
	file.read((char*)&percentSpawned, 4);
	file.read((char*)&minSpawned, 2);
	file.read((char*)&maxSpawned, 2);
	file.read((char*)&spawnSpeedVariation, 4);
	file.read((char*)&spawnDirVariation, 4);
	file.read((char*)&lifeSpan, 4);
	file.read((char*)&lifeSpanVariation, 4);
}

void NiPSysSpawnModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&numSpawnGenerations, 2);
	file.write((char*)&percentSpawned, 4);
	file.write((char*)&minSpawned, 2);
	file.write((char*)&maxSpawned, 2);
	file.write((char*)&spawnSpeedVariation, 4);
	file.write((char*)&spawnDirVariation, 4);
	file.write((char*)&lifeSpan, 4);
	file.write((char*)&lifeSpanVariation, 4);
}

int NiPSysSpawnModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 26;

	return blockSize;
}


NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(std::fstream& file, NiHeader* hdr) : NiPSysAgeDeathModifier(hdr) {
	Get(file);
}

void NiPSysAgeDeathModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&spawnOnDeath, 1);
	spawnModifierRef.Get(file);
}

void NiPSysAgeDeathModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&spawnOnDeath, 1);
	spawnModifierRef.Put(file);
}

void NiPSysAgeDeathModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&spawnModifierRef.index);
}

int NiPSysAgeDeathModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSPSysLODModifier::BSPSysLODModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysLODModifier::BSPSysLODModifier(std::fstream& file, NiHeader* hdr) : BSPSysLODModifier(hdr) {
	Get(file);
}

void BSPSysLODModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&lodBeginDistance, 4);
	file.read((char*)&lodEndDistance, 4);
	file.read((char*)&unknownFadeFactor1, 4);
	file.read((char*)&unknownFadeFactor2, 4);
}

void BSPSysLODModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&lodBeginDistance, 4);
	file.write((char*)&lodEndDistance, 4);
	file.write((char*)&unknownFadeFactor1, 4);
	file.write((char*)&unknownFadeFactor2, 4);
}

int BSPSysLODModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(std::fstream& file, NiHeader* hdr) : BSPSysSimpleColorModifier(hdr) {
	Get(file);
}

void BSPSysSimpleColorModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&fadeInPercent, 4);
	file.read((char*)&fadeOutPercent, 4);
	file.read((char*)&color1EndPercent, 4);
	file.read((char*)&color2StartPercent, 4);
	file.read((char*)&color2EndPercent, 4);
	file.read((char*)&color3StartPercent, 4);
	file.read((char*)&color1, 16);
	file.read((char*)&color2, 16);
	file.read((char*)&color3, 16);
}

void BSPSysSimpleColorModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&fadeInPercent, 4);
	file.write((char*)&fadeOutPercent, 4);
	file.write((char*)&color1EndPercent, 4);
	file.write((char*)&color2StartPercent, 4);
	file.write((char*)&color2EndPercent, 4);
	file.write((char*)&color3StartPercent, 4);
	file.write((char*)&color1, 16);
	file.write((char*)&color2, 16);
	file.write((char*)&color3, 16);
}

int BSPSysSimpleColorModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 72;

	return blockSize;
}


NiPSysRotationModifier::NiPSysRotationModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysRotationModifier::NiPSysRotationModifier(std::fstream& file, NiHeader* hdr) : NiPSysRotationModifier(hdr) {
	Get(file);
}

void NiPSysRotationModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&initialSpeed, 4);
	file.read((char*)&initialSpeedVariation, 4);
	file.read((char*)&initialAngle, 4);
	file.read((char*)&initialAngleVariation, 4);
	file.read((char*)&randomSpeedSign, 1);
	file.read((char*)&randomInitialAxis, 1);
	file.read((char*)&initialAxis, 12);
}

void NiPSysRotationModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&initialSpeed, 4);
	file.write((char*)&initialSpeedVariation, 4);
	file.write((char*)&initialAngle, 4);
	file.write((char*)&initialAngleVariation, 4);
	file.write((char*)&randomSpeedSign, 1);
	file.write((char*)&randomInitialAxis, 1);
	file.write((char*)&initialAxis, 12);
}

int NiPSysRotationModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 30;

	return blockSize;
}


BSPSysScaleModifier::BSPSysScaleModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);

	numFloats = 0;
}

BSPSysScaleModifier::BSPSysScaleModifier(std::fstream& file, NiHeader* hdr) : BSPSysScaleModifier(hdr) {
	Get(file);
}

void BSPSysScaleModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&numFloats, 4);
	floats.resize(numFloats);
	for (int i = 0; i < numFloats; i++)
		file.read((char*)&floats[i], 4);
}

void BSPSysScaleModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&numFloats, 4);
	for (int i = 0; i < numFloats; i++)
		file.write((char*)&floats[i], 4);
}

int BSPSysScaleModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;
	blockSize += numFloats * 4;

	return blockSize;
}


NiPSysGravityModifier::NiPSysGravityModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysGravityModifier::NiPSysGravityModifier(std::fstream& file, NiHeader* hdr) : NiPSysGravityModifier(hdr) {
	Get(file);
}

void NiPSysGravityModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	gravityObjRef.Get(file);
	file.read((char*)&gravityAxis, 12);
	file.read((char*)&decay, 4);
	file.read((char*)&strength, 4);
	file.read((char*)&forceType, 4);
	file.read((char*)&turbulence, 4);
	file.read((char*)&turbulenceScale, 4);
	file.read((char*)&worldAligned, 1);
}

void NiPSysGravityModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	gravityObjRef.Put(file);
	file.write((char*)&gravityAxis, 12);
	file.write((char*)&decay, 4);
	file.write((char*)&strength, 4);
	file.write((char*)&forceType, 4);
	file.write((char*)&turbulence, 4);
	file.write((char*)&turbulenceScale, 4);
	file.write((char*)&worldAligned, 1);
}

void NiPSysGravityModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&gravityObjRef.index);
}

int NiPSysGravityModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 37;

	return blockSize;
}


NiPSysPositionModifier::NiPSysPositionModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysPositionModifier::NiPSysPositionModifier(std::fstream& file, NiHeader* hdr) : NiPSysPositionModifier(hdr) {
	Get(file);
}


NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(std::fstream& file, NiHeader* hdr) : NiPSysBoundUpdateModifier(hdr) {
	Get(file);
}

void NiPSysBoundUpdateModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&updateSkip, 2);
}

void NiPSysBoundUpdateModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&updateSkip, 2);
}

int NiPSysBoundUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 2;

	return blockSize;
}


NiPSysDragModifier::NiPSysDragModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysDragModifier::NiPSysDragModifier(std::fstream& file, NiHeader* hdr) : NiPSysDragModifier(hdr) {
	Get(file);
}

void NiPSysDragModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	parentRef.Get(file);
	file.read((char*)&dragAxis, 12);
	file.read((char*)&percentage, 4);
	file.read((char*)&range, 4);
	file.read((char*)&rangeFalloff, 4);
}

void NiPSysDragModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	parentRef.Put(file);
	file.write((char*)&dragAxis, 12);
	file.write((char*)&percentage, 4);
	file.write((char*)&range, 4);
	file.write((char*)&rangeFalloff, 4);
}

void NiPSysDragModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&parentRef.index);
}

int NiPSysDragModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(std::fstream& file, NiHeader* hdr) : BSPSysInheritVelocityModifier(hdr) {
	Get(file);
}

void BSPSysInheritVelocityModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	targetNodeRef.Get(file);
	file.read((char*)&changeToInherit, 4);
	file.read((char*)&velocityMult, 4);
	file.read((char*)&velocityVar, 4);
}

void BSPSysInheritVelocityModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	targetNodeRef.Put(file);
	file.write((char*)&changeToInherit, 4);
	file.write((char*)&velocityMult, 4);
	file.write((char*)&velocityVar, 4);
}

int BSPSysInheritVelocityModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


BSPSysSubTexModifier::BSPSysSubTexModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysSubTexModifier::BSPSysSubTexModifier(std::fstream& file, NiHeader* hdr) : BSPSysSubTexModifier(hdr) {
	Get(file);
}

void BSPSysSubTexModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&startFrame, 4);
	file.read((char*)&startFrameVariation, 4);
	file.read((char*)&endFrame, 4);
	file.read((char*)&loopStartFrame, 4);
	file.read((char*)&loopStartFrameVariation, 4);
	file.read((char*)&frameCount, 4);
	file.read((char*)&frameCountVariation, 4);
}

void BSPSysSubTexModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&startFrame, 4);
	file.write((char*)&startFrameVariation, 4);
	file.write((char*)&endFrame, 4);
	file.write((char*)&loopStartFrame, 4);
	file.write((char*)&loopStartFrameVariation, 4);
	file.write((char*)&frameCount, 4);
	file.write((char*)&frameCountVariation, 4);
}

int BSPSysSubTexModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


NiPSysBombModifier::NiPSysBombModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysBombModifier::NiPSysBombModifier(std::fstream& file, NiHeader* hdr) : NiPSysBombModifier(hdr) {
	Get(file);
}

void NiPSysBombModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	bombNodeRef.Get(file);
	file.read((char*)&bombAxis, 12);
	file.read((char*)&decay, 4);
	file.read((char*)&deltaV, 4);
	file.read((char*)&decayType, 4);
	file.read((char*)&symmetryType, 4);
}

void NiPSysBombModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	bombNodeRef.Put(file);
	file.write((char*)&bombAxis, 12);
	file.write((char*)&decay, 4);
	file.write((char*)&deltaV, 4);
	file.write((char*)&decayType, 4);
	file.write((char*)&symmetryType, 4);
}

void NiPSysBombModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&bombNodeRef.index);
}

int NiPSysBombModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


BSWindModifier::BSWindModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSWindModifier::BSWindModifier(std::fstream& file, NiHeader* hdr) : BSWindModifier(hdr) {
	Get(file);
}

void BSWindModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&strength, 4);
}

void BSWindModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&strength, 4);
}

int BSWindModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSPSysRecycleBoundModifier::BSPSysRecycleBoundModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysRecycleBoundModifier::BSPSysRecycleBoundModifier(std::fstream& file, NiHeader* hdr) : BSPSysRecycleBoundModifier(hdr) {
	Get(file);
}

void BSPSysRecycleBoundModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&boundOffset, 12);
	file.read((char*)&boundExtent, 12);
	targetNodeRef.Get(file);
}

void BSPSysRecycleBoundModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&boundOffset, 12);
	file.write((char*)&boundExtent, 12);
	targetNodeRef.Put(file);
}

int BSPSysRecycleBoundModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


BSPSysHavokUpdateModifier::BSPSysHavokUpdateModifier(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

BSPSysHavokUpdateModifier::BSPSysHavokUpdateModifier(std::fstream& file, NiHeader* hdr) : BSPSysHavokUpdateModifier(hdr) {
	Get(file);
}

void BSPSysHavokUpdateModifier::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	nodeRefs.Get(file);
	modifierRef.Get(file);
}

void BSPSysHavokUpdateModifier::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	nodeRefs.Put(file);
	modifierRef.Put(file);
}

void BSPSysHavokUpdateModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	nodeRefs.GetIndexPtrs(refs);
	refs.insert(&modifierRef.index);
}

int BSPSysHavokUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 8;
	blockSize += nodeRefs.GetSize() * 4;

	return blockSize;
}


BSMasterParticleSystem::BSMasterParticleSystem(NiHeader* hdr) : NiNode(hdr) {
}

BSMasterParticleSystem::BSMasterParticleSystem(std::fstream& file, NiHeader* hdr) : BSMasterParticleSystem(hdr) {
	Get(file);
}

void BSMasterParticleSystem::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&maxEmitterObjs, 2);
	particleSysRefs.Get(file);
}

void BSMasterParticleSystem::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&maxEmitterObjs, 2);
	particleSysRefs.Put(file);
}

void BSMasterParticleSystem::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	particleSysRefs.GetIndexPtrs(refs);
}

int BSMasterParticleSystem::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 6;
	blockSize += particleSysRefs.GetSize() * 4;

	return blockSize;
}


NiParticleSystem::NiParticleSystem(NiHeader* hdr) {
	NiAVObject::Init(hdr);
}

NiParticleSystem::NiParticleSystem(std::fstream& file, NiHeader* hdr) : NiParticleSystem(hdr) {
	Get(file);
}

void NiParticleSystem::Get(std::fstream& file) {
	NiAVObject::Get(file);

	if (header->GetUserVersion2() >= 100) {
		file.read((char*)&bounds, 16);
		skinInstanceRef.Get(file);
		shaderPropertyRef.Get(file);
		alphaPropertyRef.Get(file);
		file.read((char*)&vertFlags1, 1);
		file.read((char*)&vertFlags2, 1);
		file.read((char*)&vertFlags3, 1);
		file.read((char*)&vertFlags4, 1);
		file.read((char*)&vertFlags5, 1);
		file.read((char*)&vertFlags6, 1);
		file.read((char*)&vertFlags7, 1);
		file.read((char*)&vertFlags8, 1);
	}
	else {
		dataRef.Get(file);
		psysDataRef.index = dataRef.index;
		skinInstanceRef.Get(file);

		file.read((char*)&numMaterials, 4);
		materialNameRefs.resize(numMaterials);
		for (int i = 0; i < numMaterials; i++)
			materialNameRefs[i].Get(file, header);

		materials.resize(numMaterials);
		for (int i = 0; i < numMaterials; i++)
			file.read((char*)&materials[i], 4);

		file.read((char*)&activeMaterial, 4);
		file.read((char*)&defaultMatNeedsUpdate, 1);

		if (header->GetUserVersion() >= 12) {
			shaderPropertyRef.Get(file);
			alphaPropertyRef.Get(file);
		}
	}

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&farBegin, 2);
		file.read((char*)&farEnd, 2);
		file.read((char*)&nearBegin, 2);
		file.read((char*)&nearEnd, 2);

		if (header->GetUserVersion2() >= 100)
			psysDataRef.Get(file);
	}

	file.read((char*)&isWorldSpace, 1);
	modifierRefs.Get(file);
}

void NiParticleSystem::Put(std::fstream& file) {
	NiAVObject::Put(file);

	if (header->GetUserVersion2() >= 100) {
		file.write((char*)&bounds, 16);
		skinInstanceRef.Put(file);
		shaderPropertyRef.Put(file);
		alphaPropertyRef.Put(file);
		file.write((char*)&vertFlags1, 1);
		file.write((char*)&vertFlags2, 1);
		file.write((char*)&vertFlags3, 1);
		file.write((char*)&vertFlags4, 1);
		file.write((char*)&vertFlags5, 1);
		file.write((char*)&vertFlags6, 1);
		file.write((char*)&vertFlags7, 1);
		file.write((char*)&vertFlags8, 1);
	}
	else {
		dataRef.Put(file);
		skinInstanceRef.Put(file);

		file.write((char*)&numMaterials, 4);
		for (int i = 0; i < numMaterials; i++)
			materialNameRefs[i].Put(file);

		for (int i = 0; i < numMaterials; i++)
			file.write((char*)&materials[i], 4);

		file.write((char*)&activeMaterial, 4);
		file.write((char*)&defaultMatNeedsUpdate, 1);

		if (header->GetUserVersion() >= 12) {
			shaderPropertyRef.Put(file);
			alphaPropertyRef.Put(file);
		}
	}

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&farBegin, 2);
		file.write((char*)&farEnd, 2);
		file.write((char*)&nearBegin, 2);
		file.write((char*)&nearEnd, 2);

		if (header->GetUserVersion2() >= 100)
			psysDataRef.Put(file);
	}

	file.write((char*)&isWorldSpace, 1);
	modifierRefs.Put(file);
}

void NiParticleSystem::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
	refs.insert(&skinInstanceRef.index);
	refs.insert(&shaderPropertyRef.index);
	refs.insert(&alphaPropertyRef.index);
	refs.insert(&psysDataRef.index);
	modifierRefs.GetIndexPtrs(refs);
}

int NiParticleSystem::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 5;
	blockSize += modifierRefs.GetSize() * 4;

	if (header->GetUserVersion2() >= 100) {
		blockSize += 36;
	}
	else {
		blockSize += 17;
		blockSize += numMaterials * 8;

		if (header->GetUserVersion() >= 12)
			blockSize += 8;
	}

	if (header->GetUserVersion() >= 12) {
		blockSize += 8;

		if (header->GetUserVersion2() >= 100)
			blockSize += 4;
	}

	return blockSize;
}


NiMeshParticleSystem::NiMeshParticleSystem(NiHeader* hdr) : NiParticleSystem(hdr) {
}

NiMeshParticleSystem::NiMeshParticleSystem(std::fstream& file, NiHeader* hdr) : NiMeshParticleSystem(hdr) {
	Get(file);
}


BSStripParticleSystem::BSStripParticleSystem(NiHeader* hdr) : NiParticleSystem(hdr) {
}

BSStripParticleSystem::BSStripParticleSystem(std::fstream& file, NiHeader* hdr) : BSStripParticleSystem(hdr) {
	Get(file);
}


void NiPSysCollider::Init(NiHeader* hdr) {
	NiObject::Init(hdr);
}

void NiPSysCollider::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&bounce, 4);
	file.read((char*)&spawnOnCollide, 1);
	file.read((char*)&dieOnCollide, 1);
	spawnModifierRef.Get(file);
	managerRef.Get(file);
	nextColliderRef.Get(file);
	colliderNodeRef.Get(file);
}

void NiPSysCollider::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&bounce, 4);
	file.write((char*)&spawnOnCollide, 1);
	file.write((char*)&dieOnCollide, 1);
	spawnModifierRef.Put(file);
	managerRef.Put(file);
	nextColliderRef.Put(file);
	colliderNodeRef.Put(file);
}

void NiPSysCollider::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&spawnModifierRef.index);
	refs.insert(&managerRef.index);
	refs.insert(&nextColliderRef.index);
	refs.insert(&colliderNodeRef.index);
}

int NiPSysCollider::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 22;

	return blockSize;
}


NiPSysSphericalCollider::NiPSysSphericalCollider(NiHeader* hdr) {
	NiPSysCollider::Init(hdr);
}

NiPSysSphericalCollider::NiPSysSphericalCollider(std::fstream& file, NiHeader* hdr) : NiPSysSphericalCollider(hdr) {
	Get(file);
}

void NiPSysSphericalCollider::Get(std::fstream& file) {
	NiPSysCollider::Get(file);

	file.read((char*)&radius, 4);
}

void NiPSysSphericalCollider::Put(std::fstream& file) {
	NiPSysCollider::Put(file);

	file.write((char*)&radius, 4);
}

int NiPSysSphericalCollider::CalcBlockSize() {
	NiPSysCollider::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysPlanarCollider::NiPSysPlanarCollider(NiHeader* hdr) {
	NiPSysCollider::Init(hdr);
}

NiPSysPlanarCollider::NiPSysPlanarCollider(std::fstream& file, NiHeader* hdr) : NiPSysPlanarCollider(hdr) {
	Get(file);
}

void NiPSysPlanarCollider::Get(std::fstream& file) {
	NiPSysCollider::Get(file);

	file.read((char*)&width, 4);
	file.read((char*)&height, 4);
	file.read((char*)&xAxis, 12);
	file.read((char*)&yAxis, 12);
}

void NiPSysPlanarCollider::Put(std::fstream& file) {
	NiPSysCollider::Put(file);

	file.write((char*)&width, 4);
	file.write((char*)&height, 4);
	file.write((char*)&xAxis, 12);
	file.write((char*)&yAxis, 12);
}

int NiPSysPlanarCollider::CalcBlockSize() {
	NiPSysCollider::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


NiPSysColliderManager::NiPSysColliderManager(NiHeader* hdr) {
	NiPSysModifier::Init(hdr);
}

NiPSysColliderManager::NiPSysColliderManager(std::fstream& file, NiHeader* hdr) : NiPSysColliderManager(hdr) {
	Get(file);
}

void NiPSysColliderManager::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	colliderRef.Get(file);
}

void NiPSysColliderManager::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	colliderRef.Put(file);
}

void NiPSysColliderManager::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&colliderRef.index);
}

int NiPSysColliderManager::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


void NiPSysEmitter::Get(std::fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&speed, 4);
	file.read((char*)&speedVariation, 4);
	file.read((char*)&declination, 4);
	file.read((char*)&declinationVariation, 4);
	file.read((char*)&planarAngle, 4);
	file.read((char*)&planarAngleVariation, 4);
	file.read((char*)&color, 16);
	file.read((char*)&radius, 4);
	file.read((char*)&radiusVariation, 4);
	file.read((char*)&lifeSpan, 4);
	file.read((char*)&lifeSpanVariation, 4);
}

void NiPSysEmitter::Put(std::fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&speed, 4);
	file.write((char*)&speedVariation, 4);
	file.write((char*)&declination, 4);
	file.write((char*)&declinationVariation, 4);
	file.write((char*)&planarAngle, 4);
	file.write((char*)&planarAngleVariation, 4);
	file.write((char*)&color, 16);
	file.write((char*)&radius, 4);
	file.write((char*)&radiusVariation, 4);
	file.write((char*)&lifeSpan, 4);
	file.write((char*)&lifeSpanVariation, 4);
}

int NiPSysEmitter::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 56;

	return blockSize;
}


void NiPSysVolumeEmitter::Init(NiHeader* hdr) {
	NiPSysEmitter::Init(hdr);
}

void NiPSysVolumeEmitter::Get(std::fstream& file) {
	NiPSysEmitter::Get(file);

	emitterNodeRef.Get(file);
}

void NiPSysVolumeEmitter::Put(std::fstream& file) {
	NiPSysEmitter::Put(file);

	emitterNodeRef.Put(file);
}

void NiPSysVolumeEmitter::GetChildRefs(std::set<int*>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	refs.insert(&emitterNodeRef.index);
}

int NiPSysVolumeEmitter::CalcBlockSize() {
	NiPSysEmitter::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysSphereEmitter::NiPSysSphereEmitter(NiHeader* hdr) {
	NiPSysVolumeEmitter::Init(hdr);
}

NiPSysSphereEmitter::NiPSysSphereEmitter(std::fstream& file, NiHeader* hdr) : NiPSysSphereEmitter(hdr) {
	Get(file);
}

void NiPSysSphereEmitter::Get(std::fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&radius, 4);
}

void NiPSysSphereEmitter::Put(std::fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&radius, 4);
}

int NiPSysSphereEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysCylinderEmitter::NiPSysCylinderEmitter(NiHeader* hdr) {
	NiPSysVolumeEmitter::Init(hdr);
}

NiPSysCylinderEmitter::NiPSysCylinderEmitter(std::fstream& file, NiHeader* hdr) : NiPSysCylinderEmitter(hdr) {
	Get(file);
}

void NiPSysCylinderEmitter::Get(std::fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&radius, 4);
	file.read((char*)&height, 4);
}

void NiPSysCylinderEmitter::Put(std::fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&radius, 4);
	file.write((char*)&height, 4);
}

int NiPSysCylinderEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


NiPSysBoxEmitter::NiPSysBoxEmitter(NiHeader* hdr) {
	NiPSysVolumeEmitter::Init(hdr);
}

NiPSysBoxEmitter::NiPSysBoxEmitter(std::fstream& file, NiHeader* hdr) : NiPSysBoxEmitter(hdr) {
	Get(file);
}

void NiPSysBoxEmitter::Get(std::fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&width, 4);
	file.read((char*)&height, 4);
	file.read((char*)&depth, 4);
}

void NiPSysBoxEmitter::Put(std::fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&width, 4);
	file.write((char*)&height, 4);
	file.write((char*)&depth, 4);
}

int NiPSysBoxEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


NiPSysMeshEmitter::NiPSysMeshEmitter(NiHeader* hdr) {
	NiPSysEmitter::Init(hdr);
}

NiPSysMeshEmitter::NiPSysMeshEmitter(std::fstream& file, NiHeader* hdr) : NiPSysMeshEmitter(hdr) {
	Get(file);
}

void NiPSysMeshEmitter::Get(std::fstream& file) {
	NiPSysEmitter::Get(file);

	meshRefs.Get(file);

	file.read((char*)&velocityType, 4);
	file.read((char*)&emissionType, 4);
	file.read((char*)&emissionAxis, 12);
}

void NiPSysMeshEmitter::Put(std::fstream& file) {
	NiPSysEmitter::Put(file);

	meshRefs.Put(file);

	file.write((char*)&velocityType, 4);
	file.write((char*)&emissionType, 4);
	file.write((char*)&emissionAxis, 12);
}

void NiPSysMeshEmitter::GetChildRefs(std::set<int*>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	meshRefs.GetIndexPtrs(refs);
}

int NiPSysMeshEmitter::CalcBlockSize() {
	NiPSysEmitter::CalcBlockSize();

	blockSize += 24;
	blockSize += meshRefs.GetSize() * 4;

	return blockSize;
}
