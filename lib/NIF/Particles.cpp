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


NiPSysEmitterCtlrData::NiPSysEmitterCtlrData(NiStream& stream) : NiPSysEmitterCtlrData() {
	Get(stream);
}

void NiPSysEmitterCtlrData::Get(NiStream& stream) {
	NiObject::Get(stream);

	floatKeys.Get(stream);

	stream >> numVisibilityKeys;
	visibilityKeys.resize(numVisibilityKeys);
	for (int i = 0; i < numVisibilityKeys; i++) {
		stream >> visibilityKeys[i].time;
		stream >> visibilityKeys[i].value;
	}
}

void NiPSysEmitterCtlrData::Put(NiStream& stream) {
	NiObject::Put(stream);

	floatKeys.Put(stream);

	stream << numVisibilityKeys;
	for (int i = 0; i < numVisibilityKeys; i++) {
		stream << visibilityKeys[i].time;
		stream << visibilityKeys[i].value;
	}
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


NiColorData::NiColorData(NiStream& stream) : NiColorData() {
	Get(stream);
}

void NiColorData::Get(NiStream& stream) {
	NiObject::Get(stream);

	data.Get(stream);
}

void NiColorData::Put(NiStream& stream) {
	NiObject::Put(stream);

	data.Put(stream);
}


NiPSysColorModifier::NiPSysColorModifier(NiStream& stream) : NiPSysColorModifier() {
	Get(stream);
}

void NiPSysColorModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	dataRef.Get(stream);
}

void NiPSysColorModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	dataRef.Put(stream);
}

void NiPSysColorModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}


NiPSysGrowFadeModifier::NiPSysGrowFadeModifier(NiStream& stream) : NiPSysGrowFadeModifier() {
	Get(stream);
}

void NiPSysGrowFadeModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> growTime;
	stream >> growGeneration;
	stream >> fadeTime;
	stream >> fadeGeneration;

	if (stream.GetVersion().User2() >= 34)
		stream >> baseScale;
}

void NiPSysGrowFadeModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << growTime;
	stream << growGeneration;
	stream << fadeTime;
	stream << fadeGeneration;

	if (stream.GetVersion().User2() >= 34)
		stream << baseScale;
}


NiPSysMeshUpdateModifier::NiPSysMeshUpdateModifier(NiStream& stream) : NiPSysMeshUpdateModifier() {
	Get(stream);
}

void NiPSysMeshUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	meshRefs.Get(stream);
}

void NiPSysMeshUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	meshRefs.Put(stream);
}

void NiPSysMeshUpdateModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	meshRefs.GetIndexPtrs(refs);
}


void NiPSysFieldModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	fieldObjectRef.Get(stream);
	stream >> magnitude;
	stream >> attenuation;
	stream >> useMaxDistance;
	stream >> maxDistance;
}

void NiPSysFieldModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	fieldObjectRef.Put(stream);
	stream << magnitude;
	stream << attenuation;
	stream << useMaxDistance;
	stream << maxDistance;
}

void NiPSysFieldModifier::GetChildRefs(std::set<int*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&fieldObjectRef.index);
}


NiPSysVortexFieldModifier::NiPSysVortexFieldModifier(NiStream& stream) : NiPSysVortexFieldModifier() {
	Get(stream);
}

void NiPSysVortexFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> direction;
}

void NiPSysVortexFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << direction;
}


NiPSysGravityFieldModifier::NiPSysGravityFieldModifier(NiStream& stream) : NiPSysGravityFieldModifier() {
	Get(stream);
}

void NiPSysGravityFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> direction;
}

void NiPSysGravityFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << direction;
}


NiPSysDragFieldModifier::NiPSysDragFieldModifier(NiStream& stream) : NiPSysDragFieldModifier() {
	Get(stream);
}

void NiPSysDragFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> useDirection;
	stream >> direction;
}

void NiPSysDragFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << useDirection;
	stream << direction;
}


NiPSysTurbulenceFieldModifier::NiPSysTurbulenceFieldModifier(NiStream& stream) : NiPSysTurbulenceFieldModifier() {
	Get(stream);
}

void NiPSysTurbulenceFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> frequency;
}

void NiPSysTurbulenceFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << frequency;
}


NiPSysAirFieldModifier::NiPSysAirFieldModifier(NiStream& stream) : NiPSysAirFieldModifier() {
	Get(stream);
}

void NiPSysAirFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> direction;
	stream >> unkFloat1;
	stream >> unkFloat2;
	stream >> unkBool1;
	stream >> unkBool2;
	stream >> unkBool3;
	stream >> unkFloat3;
}

void NiPSysAirFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << direction;
	stream << unkFloat1;
	stream << unkFloat2;
	stream << unkBool1;
	stream << unkBool2;
	stream << unkBool3;
	stream << unkFloat3;
}


NiPSysRadialFieldModifier::NiPSysRadialFieldModifier(NiStream& stream) : NiPSysRadialFieldModifier() {
	Get(stream);
}

void NiPSysRadialFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> radialType;
}

void NiPSysRadialFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << radialType;
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


BSParentVelocityModifier::BSParentVelocityModifier(NiStream& stream) : BSParentVelocityModifier() {
	Get(stream);
}

void BSParentVelocityModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> damping;
}

void BSParentVelocityModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << damping;
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


BSPSysArrayEmitter::BSPSysArrayEmitter(NiStream& stream) : BSPSysArrayEmitter() {
	Get(stream);
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
