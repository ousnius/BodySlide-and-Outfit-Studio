/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Particles.h"

NiParticlesData::NiParticlesData() {
	NiGeometryData::isPSys = true;
}

NiParticlesData::NiParticlesData(NiStream& stream) : NiParticlesData() {
	Get(stream);
}

void NiParticlesData::Get(NiStream& stream) {
	NiGeometryData::Get(stream);

	stream >> hasRadii;
	stream >> numActive;
	stream >> hasSizes;
	stream >> hasRotations;
	stream >> hasRotationAngles;
	stream >> hasRotationAxes;
	stream >> hasTextureIndices;

	if (stream.GetVersion().User() >= 12) {
		stream >> numSubtexOffsets;
	}
	else {
		byte numOffsets = 0;
		stream >> numOffsets;
		numSubtexOffsets = numOffsets;
	}

	subtexOffsets.resize(numSubtexOffsets);
	for (int i = 0; i < numSubtexOffsets; i++)
		stream >> subtexOffsets[i];

	if (stream.GetVersion().User() >= 12) {
		stream >> aspectRatio;
		stream >> aspectFlags;
		stream >> speedToAspectAspect2;
		stream >> speedToAspectSpeed1;
		stream >> speedToAspectSpeed2;
	}
}

void NiParticlesData::Put(NiStream& stream) {
	NiGeometryData::Put(stream);

	stream << hasRadii;
	stream << numActive;
	stream << hasSizes;
	stream << hasRotations;
	stream << hasRotationAngles;
	stream << hasRotationAxes;
	stream << hasTextureIndices;

	if (stream.GetVersion().User() >= 12) {
		stream << numSubtexOffsets;
	}
	else {
		byte numOffsets = numSubtexOffsets > 255 ? 255 : numSubtexOffsets;
		stream << numOffsets;
	}

	for (int i = 0; i < numSubtexOffsets; i++)
		stream << subtexOffsets[i];

	if (stream.GetVersion().User() >= 12) {
		stream << aspectRatio;
		stream << aspectFlags;
		stream << speedToAspectAspect2;
		stream << speedToAspectSpeed1;
		stream << speedToAspectSpeed2;
	}
}

int NiParticlesData::CalcBlockSize(NiVersion& version) {
	NiGeometryData::CalcBlockSize(version);

	blockSize += 8;

	if (version.User() >= 12)
		blockSize += 22;
	else
		blockSize += 1;

	blockSize += numSubtexOffsets * 16;

	return blockSize;
}


NiRotatingParticlesData::NiRotatingParticlesData() : NiParticlesData() {
}

NiRotatingParticlesData::NiRotatingParticlesData(NiStream& stream) : NiRotatingParticlesData() {
	Get(stream);
}


NiPSysData::NiPSysData() : NiRotatingParticlesData() {
}

NiPSysData::NiPSysData(NiStream& stream) : NiPSysData() {
	Get(stream);
}

void NiPSysData::Get(NiStream& stream) {
	NiRotatingParticlesData::Get(stream);

	stream >> hasRotationSpeeds;
}

void NiPSysData::Put(NiStream& stream) {
	NiRotatingParticlesData::Put(stream);

	stream << hasRotationSpeeds;
}

int NiPSysData::CalcBlockSize(NiVersion& version) {
	NiRotatingParticlesData::CalcBlockSize(version);

	blockSize += 1;

	return blockSize;
}


NiMeshPSysData::NiMeshPSysData() : NiPSysData() {
}

NiMeshPSysData::NiMeshPSysData(NiStream& stream) : NiMeshPSysData() {
	Get(stream);
}

void NiMeshPSysData::Get(NiStream& stream) {
	NiPSysData::Get(stream);

	stream >> defaultPoolSize;
	stream >> fillPoolsOnLoad;

	stream >> numGenerations;
	generationPoolSize.resize(numGenerations);
	for (int i = 0; i < numGenerations; i++)
		stream >> generationPoolSize[i];

	nodeRef.Get(stream);
}

void NiMeshPSysData::Put(NiStream& stream) {
	NiPSysData::Put(stream);

	stream << defaultPoolSize;
	stream << fillPoolsOnLoad;

	stream << numGenerations;
	for (int i = 0; i < numGenerations; i++)
		stream << generationPoolSize[i];

	nodeRef.Put(stream);
}

void NiMeshPSysData::GetChildRefs(std::set<int*>& refs) {
	NiPSysData::GetChildRefs(refs);

	refs.insert(&nodeRef.index);
}

int NiMeshPSysData::CalcBlockSize(NiVersion& version) {
	NiPSysData::CalcBlockSize(version);

	blockSize += 9;
	blockSize += numGenerations * 4;

	return blockSize;
}


BSStripPSysData::BSStripPSysData() : NiPSysData() {
}

BSStripPSysData::BSStripPSysData(NiStream& stream) : BSStripPSysData() {
	Get(stream);
}

void BSStripPSysData::Get(NiStream& stream) {
	NiPSysData::Get(stream);

	stream >> maxPointCount;
	stream >> startCapSize;
	stream >> endCapSize;
	stream >> doZPrepass;
}

void BSStripPSysData::Put(NiStream& stream) {
	NiPSysData::Put(stream);

	stream << maxPointCount;
	stream << startCapSize;
	stream << endCapSize;
	stream << doZPrepass;
}

int BSStripPSysData::CalcBlockSize(NiVersion& version) {
	NiPSysData::CalcBlockSize(version);

	blockSize += 11;

	return blockSize;
}


void NiPSysModifier::Get(NiStream& stream) {
	NiObject::Get(stream);

	name.Get(stream);

	stream >> order;
	targetRef.Get(stream);
	stream >> isActive;
}

void NiPSysModifier::Put(NiStream& stream) {
	NiObject::Put(stream);

	name.Put(stream);

	stream << order;
	targetRef.Put(stream);
	stream << isActive;
}

void NiPSysModifier::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name);
}

void NiPSysModifier::GetPtrs(std::set<int*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef.index);
}

int NiPSysModifier::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 13;

	return blockSize;
}


BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(NiStream& stream) : BSPSysStripUpdateModifier() {
	Get(stream);
}

void BSPSysStripUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> updateDeltaTime;
}

void BSPSysStripUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << updateDeltaTime;
}

int BSPSysStripUpdateModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiPSysSpawnModifier::NiPSysSpawnModifier(NiStream& stream) : NiPSysSpawnModifier() {
	Get(stream);
}

void NiPSysSpawnModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> numSpawnGenerations;
	stream >> percentSpawned;
	stream >> minSpawned;
	stream >> maxSpawned;
	stream >> spawnSpeedVariation;
	stream >> spawnDirVariation;
	stream >> lifeSpan;
	stream >> lifeSpanVariation;
}

void NiPSysSpawnModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << numSpawnGenerations;
	stream << percentSpawned;
	stream << minSpawned;
	stream << maxSpawned;
	stream << spawnSpeedVariation;
	stream << spawnDirVariation;
	stream << lifeSpan;
	stream << lifeSpanVariation;
}

int NiPSysSpawnModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 26;

	return blockSize;
}


NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(NiStream& stream) : NiPSysAgeDeathModifier() {
	Get(stream);
}

void NiPSysAgeDeathModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> spawnOnDeath;
	spawnModifierRef.Get(stream);
}

void NiPSysAgeDeathModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << spawnOnDeath;
	spawnModifierRef.Put(stream);
}

void NiPSysAgeDeathModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&spawnModifierRef.index);
}

int NiPSysAgeDeathModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 5;

	return blockSize;
}


BSPSysLODModifier::BSPSysLODModifier(NiStream& stream) : BSPSysLODModifier() {
	Get(stream);
}

void BSPSysLODModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> lodBeginDistance;
	stream >> lodEndDistance;
	stream >> unknownFadeFactor1;
	stream >> unknownFadeFactor2;
}

void BSPSysLODModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << lodBeginDistance;
	stream << lodEndDistance;
	stream << unknownFadeFactor1;
	stream << unknownFadeFactor2;
}

int BSPSysLODModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 16;

	return blockSize;
}


BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(NiStream& stream) : BSPSysSimpleColorModifier() {
	Get(stream);
}

void BSPSysSimpleColorModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> fadeInPercent;
	stream >> fadeOutPercent;
	stream >> color1EndPercent;
	stream >> color2StartPercent;
	stream >> color2EndPercent;
	stream >> color3StartPercent;
	stream >> color1;
	stream >> color2;
	stream >> color3;
}

void BSPSysSimpleColorModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << fadeInPercent;
	stream << fadeOutPercent;
	stream << color1EndPercent;
	stream << color2StartPercent;
	stream << color2EndPercent;
	stream << color3StartPercent;
	stream << color1;
	stream << color2;
	stream << color3;
}

int BSPSysSimpleColorModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 72;

	return blockSize;
}


NiPSysRotationModifier::NiPSysRotationModifier(NiStream& stream) : NiPSysRotationModifier() {
	Get(stream);
}

void NiPSysRotationModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> initialSpeed;
	stream >> initialSpeedVariation;
	stream >> initialAngle;
	stream >> initialAngleVariation;
	stream >> randomSpeedSign;
	stream >> randomInitialAxis;
	stream >> initialAxis;
}

void NiPSysRotationModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << initialSpeed;
	stream << initialSpeedVariation;
	stream << initialAngle;
	stream << initialAngleVariation;
	stream << randomSpeedSign;
	stream << randomInitialAxis;
	stream << initialAxis;
}

int NiPSysRotationModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 30;

	return blockSize;
}


BSPSysScaleModifier::BSPSysScaleModifier(NiStream& stream) : BSPSysScaleModifier() {
	Get(stream);
}

void BSPSysScaleModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> numFloats;
	floats.resize(numFloats);
	for (int i = 0; i < numFloats; i++)
		stream >> floats[i];
}

void BSPSysScaleModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << numFloats;
	for (int i = 0; i < numFloats; i++)
		stream << floats[i];
}

int BSPSysScaleModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 4;
	blockSize += numFloats * 4;

	return blockSize;
}


NiPSysGravityModifier::NiPSysGravityModifier(NiStream& stream) : NiPSysGravityModifier() {
	Get(stream);
}

void NiPSysGravityModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	gravityObjRef.Get(stream);
	stream >> gravityAxis;
	stream >> decay;
	stream >> strength;
	stream >> forceType;
	stream >> turbulence;
	stream >> turbulenceScale;
	stream >> worldAligned;
}

void NiPSysGravityModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	gravityObjRef.Put(stream);
	stream << gravityAxis;
	stream << decay;
	stream << strength;
	stream << forceType;
	stream << turbulence;
	stream << turbulenceScale;
	stream << worldAligned;
}

void NiPSysGravityModifier::GetPtrs(std::set<int*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&gravityObjRef.index);
}

int NiPSysGravityModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 37;

	return blockSize;
}


NiPSysPositionModifier::NiPSysPositionModifier(NiStream& stream) : NiPSysPositionModifier() {
	Get(stream);
}


NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(NiStream& stream) : NiPSysBoundUpdateModifier() {
	Get(stream);
}

void NiPSysBoundUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> updateSkip;
}

void NiPSysBoundUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << updateSkip;
}

int NiPSysBoundUpdateModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 2;

	return blockSize;
}


NiPSysDragModifier::NiPSysDragModifier(NiStream& stream) : NiPSysDragModifier() {
	Get(stream);
}

void NiPSysDragModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	parentRef.Get(stream);
	stream >> dragAxis;
	stream >> percentage;
	stream >> range;
	stream >> rangeFalloff;
}

void NiPSysDragModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	parentRef.Put(stream);
	stream << dragAxis;
	stream << percentage;
	stream << range;
	stream << rangeFalloff;
}

void NiPSysDragModifier::GetPtrs(std::set<int*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&parentRef.index);
}

int NiPSysDragModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 28;

	return blockSize;
}


BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(NiStream& stream) : BSPSysInheritVelocityModifier() {
	Get(stream);
}

void BSPSysInheritVelocityModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	targetNodeRef.Get(stream);
	stream >> changeToInherit;
	stream >> velocityMult;
	stream >> velocityVar;
}

void BSPSysInheritVelocityModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	targetNodeRef.Put(stream);
	stream << changeToInherit;
	stream << velocityMult;
	stream << velocityVar;
}

void BSPSysInheritVelocityModifier::GetPtrs(std::set<int*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&targetNodeRef.index);
}

int BSPSysInheritVelocityModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 16;

	return blockSize;
}


BSPSysSubTexModifier::BSPSysSubTexModifier(NiStream& stream) : BSPSysSubTexModifier() {
	Get(stream);
}

void BSPSysSubTexModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> startFrame;
	stream >> startFrameVariation;
	stream >> endFrame;
	stream >> loopStartFrame;
	stream >> loopStartFrameVariation;
	stream >> frameCount;
	stream >> frameCountVariation;
}

void BSPSysSubTexModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << startFrame;
	stream << startFrameVariation;
	stream << endFrame;
	stream << loopStartFrame;
	stream << loopStartFrameVariation;
	stream << frameCount;
	stream << frameCountVariation;
}

int BSPSysSubTexModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 28;

	return blockSize;
}


NiPSysBombModifier::NiPSysBombModifier(NiStream& stream) : NiPSysBombModifier() {
	Get(stream);
}

void NiPSysBombModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	bombNodeRef.Get(stream);
	stream >> bombAxis;
	stream >> decay;
	stream >> deltaV;
	stream >> decayType;
	stream >> symmetryType;
}

void NiPSysBombModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	bombNodeRef.Put(stream);
	stream << bombAxis;
	stream << decay;
	stream << deltaV;
	stream << decayType;
	stream << symmetryType;
}

void NiPSysBombModifier::GetPtrs(std::set<int*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&bombNodeRef.index);
}

int NiPSysBombModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 32;

	return blockSize;
}


BSWindModifier::BSWindModifier(NiStream& stream) : BSWindModifier() {
	Get(stream);
}

void BSWindModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> strength;
}

void BSWindModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << strength;
}

int BSWindModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSPSysRecycleBoundModifier::BSPSysRecycleBoundModifier(NiStream& stream) : BSPSysRecycleBoundModifier() {
	Get(stream);
}

void BSPSysRecycleBoundModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> boundOffset;
	stream >> boundExtent;
	targetNodeRef.Get(stream);
}

void BSPSysRecycleBoundModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << boundOffset;
	stream << boundExtent;
	targetNodeRef.Put(stream);
}

void BSPSysRecycleBoundModifier::GetPtrs(std::set<int*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&targetNodeRef.index);
}

int BSPSysRecycleBoundModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 28;

	return blockSize;
}


BSPSysHavokUpdateModifier::BSPSysHavokUpdateModifier(NiStream& stream) : BSPSysHavokUpdateModifier() {
	Get(stream);
}

void BSPSysHavokUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	nodeRefs.Get(stream);
	modifierRef.Get(stream);
}

void BSPSysHavokUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	nodeRefs.Put(stream);
	modifierRef.Put(stream);
}

void BSPSysHavokUpdateModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	nodeRefs.GetIndexPtrs(refs);
	refs.insert(&modifierRef.index);
}

int BSPSysHavokUpdateModifier::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 4;
	blockSize += nodeRefs.CalcBlockSize();

	return blockSize;
}


BSMasterParticleSystem::BSMasterParticleSystem() : NiNode() {
}

BSMasterParticleSystem::BSMasterParticleSystem(NiStream& stream) : BSMasterParticleSystem() {
	Get(stream);
}

void BSMasterParticleSystem::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> maxEmitterObjs;
	particleSysRefs.Get(stream);
}

void BSMasterParticleSystem::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << maxEmitterObjs;
	particleSysRefs.Put(stream);
}

void BSMasterParticleSystem::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	particleSysRefs.GetIndexPtrs(refs);
}

int BSMasterParticleSystem::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 6;
	blockSize += particleSysRefs.GetSize() * 4;

	return blockSize;
}


NiParticleSystem::NiParticleSystem(NiStream& stream) : NiParticleSystem() {
	Get(stream);
}

void NiParticleSystem::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	if (stream.GetVersion().User2() >= 100) {
		stream >> bounds;
		skinInstanceRef.Get(stream);
		shaderPropertyRef.Get(stream);
		alphaPropertyRef.Get(stream);
		stream >> vertFlags1;
		stream >> vertFlags2;
		stream >> vertFlags3;
		stream >> vertFlags4;
		stream >> vertFlags5;
		stream >> vertFlags6;
		stream >> vertFlags7;
		stream >> vertFlags8;
	}
	else {
		dataRef.Get(stream);
		psysDataRef.index = dataRef.index;
		skinInstanceRef.Get(stream);

		stream >> numMaterials;
		materialNameRefs.resize(numMaterials);
		for (int i = 0; i < numMaterials; i++)
			materialNameRefs[i].Get(stream);

		materials.resize(numMaterials);
		for (int i = 0; i < numMaterials; i++)
			stream >> materials[i];

		stream >> activeMaterial;
		stream >> defaultMatNeedsUpdate;

		if (stream.GetVersion().User() >= 12) {
			shaderPropertyRef.Get(stream);
			alphaPropertyRef.Get(stream);
		}
	}

	if (stream.GetVersion().User() >= 12) {
		stream >> farBegin;
		stream >> farEnd;
		stream >> nearBegin;
		stream >> nearEnd;

		if (stream.GetVersion().User2() >= 100)
			psysDataRef.Get(stream);
	}

	stream >> isWorldSpace;
	modifierRefs.Get(stream);
}

void NiParticleSystem::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	if (stream.GetVersion().User2() >= 100) {
		stream << bounds;
		skinInstanceRef.Put(stream);
		shaderPropertyRef.Put(stream);
		alphaPropertyRef.Put(stream);
		stream << vertFlags1;
		stream << vertFlags2;
		stream << vertFlags3;
		stream << vertFlags4;
		stream << vertFlags5;
		stream << vertFlags6;
		stream << vertFlags7;
		stream << vertFlags8;
	}
	else {
		dataRef.Put(stream);
		skinInstanceRef.Put(stream);

		stream << numMaterials;
		for (int i = 0; i < numMaterials; i++)
			materialNameRefs[i].Put(stream);

		for (int i = 0; i < numMaterials; i++)
			stream << materials[i];

		stream << activeMaterial;
		stream << defaultMatNeedsUpdate;

		if (stream.GetVersion().User() >= 12) {
			shaderPropertyRef.Put(stream);
			alphaPropertyRef.Put(stream);
		}
	}

	if (stream.GetVersion().User() >= 12) {
		stream << farBegin;
		stream << farEnd;
		stream << nearBegin;
		stream << nearEnd;

		if (stream.GetVersion().User2() >= 100)
			psysDataRef.Put(stream);
	}

	stream << isWorldSpace;
	modifierRefs.Put(stream);
}

void NiParticleSystem::GetStringRefs(std::set<StringRef*>& refs) {
	NiAVObject::GetStringRefs(refs);

	for (auto &m : materialNameRefs)
		refs.insert(&m);
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

int NiParticleSystem::CalcBlockSize(NiVersion& version) {
	NiAVObject::CalcBlockSize(version);

	blockSize += 1;
	blockSize += modifierRefs.CalcBlockSize();

	if (version.User2() >= 100) {
		blockSize += 36;
	}
	else {
		blockSize += 17;
		blockSize += numMaterials * 8;

		if (version.User() >= 12)
			blockSize += 8;
	}

	if (version.User() >= 12) {
		blockSize += 8;

		if (version.User2() >= 100)
			blockSize += 4;
	}

	return blockSize;
}


NiMeshParticleSystem::NiMeshParticleSystem() : NiParticleSystem() {
}

NiMeshParticleSystem::NiMeshParticleSystem(NiStream& stream) : NiMeshParticleSystem() {
	Get(stream);
}


BSStripParticleSystem::BSStripParticleSystem() : NiParticleSystem() {
}

BSStripParticleSystem::BSStripParticleSystem(NiStream& stream) : BSStripParticleSystem() {
	Get(stream);
}


void NiPSysCollider::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> bounce;
	stream >> spawnOnCollide;
	stream >> dieOnCollide;
	spawnModifierRef.Get(stream);
	managerRef.Get(stream);
	nextColliderRef.Get(stream);
	colliderNodeRef.Get(stream);
}

void NiPSysCollider::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << bounce;
	stream << spawnOnCollide;
	stream << dieOnCollide;
	spawnModifierRef.Put(stream);
	managerRef.Put(stream);
	nextColliderRef.Put(stream);
	colliderNodeRef.Put(stream);
}

void NiPSysCollider::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&spawnModifierRef.index);
	refs.insert(&nextColliderRef.index);
}

void NiPSysCollider::GetPtrs(std::set<int*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&managerRef.index);
	ptrs.insert(&colliderNodeRef.index);
}

int NiPSysCollider::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 22;

	return blockSize;
}


NiPSysSphericalCollider::NiPSysSphericalCollider(NiStream& stream) : NiPSysSphericalCollider() {
	Get(stream);
}

void NiPSysSphericalCollider::Get(NiStream& stream) {
	NiPSysCollider::Get(stream);

	stream >> radius;
}

void NiPSysSphericalCollider::Put(NiStream& stream) {
	NiPSysCollider::Put(stream);

	stream << radius;
}

int NiPSysSphericalCollider::CalcBlockSize(NiVersion& version) {
	NiPSysCollider::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiPSysPlanarCollider::NiPSysPlanarCollider(NiStream& stream) : NiPSysPlanarCollider() {
	Get(stream);
}

void NiPSysPlanarCollider::Get(NiStream& stream) {
	NiPSysCollider::Get(stream);

	stream >> width;
	stream >> height;
	stream >> xAxis;
	stream >> yAxis;
}

void NiPSysPlanarCollider::Put(NiStream& stream) {
	NiPSysCollider::Put(stream);

	stream << width;
	stream << height;
	stream << xAxis;
	stream << yAxis;
}

int NiPSysPlanarCollider::CalcBlockSize(NiVersion& version) {
	NiPSysCollider::CalcBlockSize(version);

	blockSize += 32;

	return blockSize;
}


NiPSysColliderManager::NiPSysColliderManager(NiStream& stream) : NiPSysColliderManager() {
	Get(stream);
}

void NiPSysColliderManager::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	colliderRef.Get(stream);
}

void NiPSysColliderManager::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	colliderRef.Put(stream);
}

void NiPSysColliderManager::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&colliderRef.index);
}

int NiPSysColliderManager::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


void NiPSysEmitter::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> speed;
	stream >> speedVariation;
	stream >> declination;
	stream >> declinationVariation;
	stream >> planarAngle;
	stream >> planarAngleVariation;
	stream >> color;
	stream >> radius;
	stream >> radiusVariation;
	stream >> lifeSpan;
	stream >> lifeSpanVariation;
}

void NiPSysEmitter::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << speed;
	stream << speedVariation;
	stream << declination;
	stream << declinationVariation;
	stream << planarAngle;
	stream << planarAngleVariation;
	stream << color;
	stream << radius;
	stream << radiusVariation;
	stream << lifeSpan;
	stream << lifeSpanVariation;
}

int NiPSysEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysModifier::CalcBlockSize(version);

	blockSize += 56;

	return blockSize;
}


void NiPSysVolumeEmitter::Get(NiStream& stream) {
	NiPSysEmitter::Get(stream);

	emitterNodeRef.Get(stream);
}

void NiPSysVolumeEmitter::Put(NiStream& stream) {
	NiPSysEmitter::Put(stream);

	emitterNodeRef.Put(stream);
}

void NiPSysVolumeEmitter::GetPtrs(std::set<int*>& ptrs) {
	NiPSysEmitter::GetPtrs(ptrs);

	ptrs.insert(&emitterNodeRef.index);
}

int NiPSysVolumeEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysEmitter::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiPSysSphereEmitter::NiPSysSphereEmitter(NiStream& stream) : NiPSysSphereEmitter() {
	Get(stream);
}

void NiPSysSphereEmitter::Get(NiStream& stream) {
	NiPSysVolumeEmitter::Get(stream);

	stream >> radius;
}

void NiPSysSphereEmitter::Put(NiStream& stream) {
	NiPSysVolumeEmitter::Put(stream);

	stream << radius;
}

int NiPSysSphereEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysVolumeEmitter::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiPSysCylinderEmitter::NiPSysCylinderEmitter(NiStream& stream) : NiPSysCylinderEmitter() {
	Get(stream);
}

void NiPSysCylinderEmitter::Get(NiStream& stream) {
	NiPSysVolumeEmitter::Get(stream);

	stream >> radius;
	stream >> height;
}

void NiPSysCylinderEmitter::Put(NiStream& stream) {
	NiPSysVolumeEmitter::Put(stream);

	stream << radius;
	stream << height;
}

int NiPSysCylinderEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysVolumeEmitter::CalcBlockSize(version);

	blockSize += 8;

	return blockSize;
}


NiPSysBoxEmitter::NiPSysBoxEmitter(NiStream& stream) : NiPSysBoxEmitter() {
	Get(stream);
}

void NiPSysBoxEmitter::Get(NiStream& stream) {
	NiPSysVolumeEmitter::Get(stream);

	stream >> width;
	stream >> height;
	stream >> depth;
}

void NiPSysBoxEmitter::Put(NiStream& stream) {
	NiPSysVolumeEmitter::Put(stream);

	stream << width;
	stream << height;
	stream << depth;
}

int NiPSysBoxEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysVolumeEmitter::CalcBlockSize(version);

	blockSize += 12;

	return blockSize;
}


NiPSysMeshEmitter::NiPSysMeshEmitter(NiStream& stream) : NiPSysMeshEmitter() {
	Get(stream);
}

void NiPSysMeshEmitter::Get(NiStream& stream) {
	NiPSysEmitter::Get(stream);

	meshRefs.Get(stream);

	stream >> velocityType;
	stream >> emissionType;
	stream >> emissionAxis;
}

void NiPSysMeshEmitter::Put(NiStream& stream) {
	NiPSysEmitter::Put(stream);

	meshRefs.Put(stream);

	stream << velocityType;
	stream << emissionType;
	stream << emissionAxis;
}

void NiPSysMeshEmitter::GetChildRefs(std::set<int*>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	meshRefs.GetIndexPtrs(refs);
}

int NiPSysMeshEmitter::CalcBlockSize(NiVersion& version) {
	NiPSysEmitter::CalcBlockSize(version);

	blockSize += 20;
	blockSize += meshRefs.CalcBlockSize();

	return blockSize;
}
