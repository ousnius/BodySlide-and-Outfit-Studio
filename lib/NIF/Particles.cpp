/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Particles.h"

NiParticlesData::NiParticlesData() {
	NiGeometryData::isPSys = true;
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


void NiParticleMeshesData::Get(NiStream& stream) {
	NiRotatingParticlesData::Get(stream);

	dataRef.Get(stream);
}

void NiParticleMeshesData::Put(NiStream& stream) {
	NiRotatingParticlesData::Put(stream);

	dataRef.Put(stream);
}

void NiParticleMeshesData::GetChildRefs(std::set<Ref*>& refs) {
	NiRotatingParticlesData::GetChildRefs(refs);

	refs.insert(&dataRef);
}


void NiPSysData::Get(NiStream& stream) {
	NiRotatingParticlesData::Get(stream);

	stream >> hasRotationSpeeds;
}

void NiPSysData::Put(NiStream& stream) {
	NiRotatingParticlesData::Put(stream);

	stream << hasRotationSpeeds;
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

void NiMeshPSysData::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysData::GetChildRefs(refs);

	refs.insert(&nodeRef);
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

void NiPSysModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef);
}


void BSPSysStripUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> updateDeltaTime;
}

void BSPSysStripUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << updateDeltaTime;
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

void NiPSysAgeDeathModifier::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&spawnModifierRef);
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

void NiPSysGravityModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&gravityObjRef);
}


void NiPSysBoundUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> updateSkip;
}

void NiPSysBoundUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << updateSkip;
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

void NiPSysDragModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&parentRef);
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

void BSPSysInheritVelocityModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&targetNodeRef);
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

void NiPSysBombModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&bombNodeRef);
}


void NiColorData::Get(NiStream& stream) {
	NiObject::Get(stream);

	data.Get(stream);
}

void NiColorData::Put(NiStream& stream) {
	NiObject::Put(stream);

	data.Put(stream);
}


void NiPSysColorModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	dataRef.Get(stream);
}

void NiPSysColorModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	dataRef.Put(stream);
}

void NiPSysColorModifier::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&dataRef);
}


void NiPSysGrowFadeModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> growTime;
	stream >> growGeneration;
	stream >> fadeTime;
	stream >> fadeGeneration;

	if (stream.GetVersion().Stream() >= 34)
		stream >> baseScale;
}

void NiPSysGrowFadeModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << growTime;
	stream << growGeneration;
	stream << fadeTime;
	stream << fadeGeneration;

	if (stream.GetVersion().Stream() >= 34)
		stream << baseScale;
}


void NiPSysMeshUpdateModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	meshRefs.Get(stream);
}

void NiPSysMeshUpdateModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	meshRefs.Put(stream);
}

void NiPSysMeshUpdateModifier::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	meshRefs.GetIndexPtrs(refs);
}

BlockRefArray<NiAVObject>& NiPSysMeshUpdateModifier::GetMeshes() {
	return meshRefs;
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

void NiPSysFieldModifier::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&fieldObjectRef);
}


void NiPSysVortexFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> direction;
}

void NiPSysVortexFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << direction;
}


void NiPSysGravityFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> direction;
}

void NiPSysGravityFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << direction;
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


void NiPSysTurbulenceFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> frequency;
}

void NiPSysTurbulenceFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << frequency;
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


void NiPSysRadialFieldModifier::Get(NiStream& stream) {
	NiPSysFieldModifier::Get(stream);

	stream >> radialType;
}

void NiPSysRadialFieldModifier::Put(NiStream& stream) {
	NiPSysFieldModifier::Put(stream);

	stream << radialType;
}


void BSWindModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> strength;
}

void BSWindModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << strength;
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

void BSPSysRecycleBoundModifier::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysModifier::GetPtrs(ptrs);

	ptrs.insert(&targetNodeRef);
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

void BSPSysHavokUpdateModifier::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	nodeRefs.GetIndexPtrs(refs);
	refs.insert(&modifierRef);
}


BlockRefArray<NiNode>& BSPSysHavokUpdateModifier::GetNodes() {
	return nodeRefs;
}

int BSPSysHavokUpdateModifier::GetModifierRef() {
	return modifierRef.GetIndex();
}

void BSPSysHavokUpdateModifier::SetModifierRef(int modRef) {
	modifierRef.SetIndex(modRef);
}


void BSParentVelocityModifier::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	stream >> damping;
}

void BSParentVelocityModifier::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	stream << damping;
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

void BSMasterParticleSystem::GetChildRefs(std::set<Ref*>& refs) {
	NiNode::GetChildRefs(refs);

	particleSysRefs.GetIndexPtrs(refs);
}

BlockRefArray<NiAVObject>& BSMasterParticleSystem::GetParticleSystems() {
	return particleSysRefs;
}


void NiParticleSystem::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	if (stream.GetVersion().Stream() >= 100) {
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
		psysDataRef.SetIndex(dataRef.GetIndex());
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

		if (stream.GetVersion().Stream() >= 100) {
			psysDataRef.Get(stream);
			dataRef.SetIndex(psysDataRef.GetIndex());
		}
	}

	stream >> isWorldSpace;
	modifierRefs.Get(stream);
}

void NiParticleSystem::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	if (stream.GetVersion().Stream() >= 100) {
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

		if (stream.GetVersion().Stream() >= 100)
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

void NiParticleSystem::GetChildRefs(std::set<Ref*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&dataRef);
	refs.insert(&skinInstanceRef);
	refs.insert(&shaderPropertyRef);
	refs.insert(&alphaPropertyRef);
	refs.insert(&psysDataRef);
	modifierRefs.GetIndexPtrs(refs);
}

int NiParticleSystem::GetDataRef() {
	return dataRef.GetIndex();
}

void NiParticleSystem::SetDataRef(int datRef) {
	dataRef.SetIndex(datRef);
}

int NiParticleSystem::GetSkinInstanceRef() {
	return skinInstanceRef.GetIndex();
}

void NiParticleSystem::SetSkinInstanceRef(int skinRef) {
	skinInstanceRef.SetIndex(skinRef);
}

int NiParticleSystem::GetShaderPropertyRef() {
	return shaderPropertyRef.GetIndex();
}

void NiParticleSystem::SetShaderPropertyRef(int shaderRef) {
	shaderPropertyRef.SetIndex(shaderRef);
}

int NiParticleSystem::GetAlphaPropertyRef() {
	return alphaPropertyRef.GetIndex();
}

void NiParticleSystem::SetAlphaPropertyRef(int alphaRef) {
	alphaPropertyRef.SetIndex(alphaRef);
}

int NiParticleSystem::GetPSysDataRef() {
	return psysDataRef.GetIndex();
}

void NiParticleSystem::SetPSysDataRef(int psysDatRef) {
	psysDataRef.SetIndex(psysDatRef);
}

BlockRefArray<NiPSysModifier>& NiParticleSystem::GetModifiers() {
	return modifierRefs;
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

void NiPSysCollider::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&spawnModifierRef);
	refs.insert(&nextColliderRef);
}

void NiPSysCollider::GetPtrs(std::set<Ref*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&managerRef);
	ptrs.insert(&colliderNodeRef);
}


void NiPSysSphericalCollider::Get(NiStream& stream) {
	NiPSysCollider::Get(stream);

	stream >> radius;
}

void NiPSysSphericalCollider::Put(NiStream& stream) {
	NiPSysCollider::Put(stream);

	stream << radius;
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


void NiPSysColliderManager::Get(NiStream& stream) {
	NiPSysModifier::Get(stream);

	colliderRef.Get(stream);
}

void NiPSysColliderManager::Put(NiStream& stream) {
	NiPSysModifier::Put(stream);

	colliderRef.Put(stream);
}

void NiPSysColliderManager::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(&colliderRef);
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

void NiPSysVolumeEmitter::GetPtrs(std::set<Ref*>& ptrs) {
	NiPSysEmitter::GetPtrs(ptrs);

	ptrs.insert(&emitterNodeRef);
}


void NiPSysSphereEmitter::Get(NiStream& stream) {
	NiPSysVolumeEmitter::Get(stream);

	stream >> radius;
}

void NiPSysSphereEmitter::Put(NiStream& stream) {
	NiPSysVolumeEmitter::Put(stream);

	stream << radius;
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

void NiPSysMeshEmitter::GetChildRefs(std::set<Ref*>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	meshRefs.GetIndexPtrs(refs);
}

BlockRefArray<NiAVObject>& NiPSysMeshEmitter::GetMeshes() {
	return meshRefs;
}
