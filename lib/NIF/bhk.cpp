/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "bhk.h"

void NiCollisionObject::Get(NiStream& stream) {
	NiObject::Get(stream);

	targetRef.Get(stream);
}

void NiCollisionObject::Put(NiStream& stream) {
	NiObject::Put(stream);

	targetRef.Put(stream);
}

void NiCollisionObject::GetPtrs(std::set<Ref*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef);
}


BoundingVolume::BoundingVolume() {
	bvUnion = new UnionBV();
}

BoundingVolume::~BoundingVolume() {
	delete bvUnion;
}

void BoundingVolume::Get(NiStream& stream) {
	stream >> collisionType;

	switch (collisionType) {
	case SPHERE_BV:
		stream >> bvSphere;
		break;
	case BOX_BV:
		stream >> bvBox;
		break;
	case CAPSULE_BV:
		stream >> bvCapsule;
		break;
	case UNION_BV:
		bvUnion->Get(stream);
		break;
	case HALFSPACE_BV:
		stream >> bvHalfSpace;
		break;
	default:
		break;
	}
}

void BoundingVolume::Put(NiStream& stream) {
	stream << collisionType;

	switch (collisionType) {
	case SPHERE_BV:
		stream << bvSphere;
		break;
	case BOX_BV:
		stream << bvBox;
		break;
	case CAPSULE_BV:
		stream << bvCapsule;
		break;
	case UNION_BV:
		bvUnion->Put(stream);
		break;
	case HALFSPACE_BV:
		stream << bvHalfSpace;
		break;
	default:
		break;
	}
}


void NiCollisionData::Get(NiStream& stream) {
	NiCollisionObject::Get(stream);

	stream >> propagationMode;
	stream >> collisionMode;
	stream >> useABV;

	if (useABV)
		boundingVolume.Get(stream);
}

void NiCollisionData::Put(NiStream& stream) {
	NiCollisionObject::Put(stream);

	stream << propagationMode;
	stream << collisionMode;
	stream << useABV;

	if (useABV)
		boundingVolume.Put(stream);
}


void bhkNiCollisionObject::Get(NiStream& stream) {
	NiCollisionObject::Get(stream);

	stream >> flags;
	bodyRef.Get(stream);
}

void bhkNiCollisionObject::Put(NiStream& stream) {
	NiCollisionObject::Put(stream);

	stream << flags;
	bodyRef.Put(stream);
}

void bhkNiCollisionObject::GetChildRefs(std::set<Ref*>& refs) {
	NiCollisionObject::GetChildRefs(refs);

	refs.insert(&bodyRef);
}


void bhkNPCollisionObject::Get(NiStream& stream) {
	bhkCollisionObject::Get(stream);

	stream >> bodyID;
}

void bhkNPCollisionObject::Put(NiStream& stream) {
	bhkCollisionObject::Put(stream);

	stream << bodyID;
}


void bhkBlendCollisionObject::Get(NiStream& stream) {
	bhkCollisionObject::Get(stream);

	stream >> heirGain;
	stream >> velGain;
}

void bhkBlendCollisionObject::Put(NiStream& stream) {
	bhkCollisionObject::Put(stream);

	stream << heirGain;
	stream << velGain;
}


bhkPhysicsSystem::bhkPhysicsSystem(const uint size) {
	numBytes = size;
	data.resize(size);
}

void bhkPhysicsSystem::Get(NiStream& stream) {
	BSExtraData::Get(stream);

	stream >> numBytes;
	data.resize(numBytes);
	if (data.empty())
		return;

	stream.read(&data[0], numBytes);
}

void bhkPhysicsSystem::Put(NiStream& stream) {
	BSExtraData::Put(stream);

	stream << numBytes;
	if (data.empty())
		return;

	stream.write(&data[0], numBytes);
}


bhkRagdollSystem::bhkRagdollSystem(const uint size) {
	numBytes = size;
	data.resize(size);
}

void bhkRagdollSystem::Get(NiStream& stream) {
	BSExtraData::Get(stream);

	stream >> numBytes;
	data.resize(numBytes);
	if (data.empty())
		return;

	stream.read(&data[0], numBytes);
}

void bhkRagdollSystem::Put(NiStream& stream) {
	BSExtraData::Put(stream);

	stream << numBytes;
	if (data.empty())
		return;

	stream.write(&data[0], numBytes);
}


void bhkBlendController::Get(NiStream& stream) {
	NiTimeController::Get(stream);

	stream >> keys;
}

void bhkBlendController::Put(NiStream& stream) {
	NiTimeController::Put(stream);

	stream << keys;
}


void bhkPlaneShape::Get(NiStream& stream) {
	bhkHeightFieldShape::Get(stream);

	stream >> material;
	stream >> unkVec;
	stream >> direction;
	stream >> constant;
	stream >> halfExtents;
	stream >> center;
}

void bhkPlaneShape::Put(NiStream& stream) {
	bhkHeightFieldShape::Put(stream);

	stream << material;
	stream << unkVec;
	stream << direction;
	stream << constant;
	stream << halfExtents;
	stream << center;
}


void bhkSphereRepShape::Get(NiStream& stream) {
	bhkShape::Get(stream);

	stream >> material;
	stream >> radius;
}

void bhkSphereRepShape::Put(NiStream& stream) {
	bhkShape::Put(stream);

	stream << material;
	stream << radius;
}


void bhkMultiSphereShape::Get(NiStream& stream) {
	bhkSphereRepShape::Get(stream);

	stream >> unkFloat1;
	stream >> unkFloat2;

	stream >> numSpheres;
	spheres.resize(numSpheres);
	for (int i = 0; i < numSpheres; i++)
		stream >> spheres[i];
}

void bhkMultiSphereShape::Put(NiStream& stream) {
	bhkSphereRepShape::Put(stream);

	stream << unkFloat1;
	stream << unkFloat2;

	stream << numSpheres;
	for (int i = 0; i < numSpheres; i++)
		stream << spheres[i];
}


void bhkConvexListShape::Get(NiStream& stream) {
	bhkShape::Get(stream);

	shapeRefs.Get(stream);
	stream >> material;
	stream >> radius;
	stream >> unkInt1;
	stream >> unkFloat1;
	stream >> childShapeProp;
	stream >> unkByte1;
	stream >> unkFloat2;
}

void bhkConvexListShape::Put(NiStream& stream) {
	bhkShape::Put(stream);

	shapeRefs.Put(stream);
	stream << material;
	stream << radius;
	stream << unkInt1;
	stream << unkFloat1;
	stream << childShapeProp;
	stream << unkByte1;
	stream << unkFloat2;
}

void bhkConvexListShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShape::GetChildRefs(refs);

	shapeRefs.GetIndexPtrs(refs);
}

BlockRefArray<bhkConvexShape>& bhkConvexListShape::GetShapes() {
	return shapeRefs;
}


void bhkConvexVerticesShape::Get(NiStream& stream) {
	bhkConvexShape::Get(stream);

	stream >> vertsProp;
	stream >> normalsProp;

	stream >> numVerts;
	verts.resize(numVerts);
	for (int i = 0; i < numVerts; i++)
		stream >> verts[i];

	stream >> numNormals;
	normals.resize(numNormals);
	for (int i = 0; i < numNormals; i++)
		stream >> normals[i];
}

void bhkConvexVerticesShape::Put(NiStream& stream) {
	bhkConvexShape::Put(stream);

	stream << vertsProp;
	stream << normalsProp;

	stream << numVerts;
	for (int i = 0; i < numVerts; i++)
		stream << verts[i];

	stream << numNormals;
	for (int i = 0; i < numNormals; i++)
		stream << normals[i];
}


void bhkBoxShape::Get(NiStream& stream) {
	bhkConvexShape::Get(stream);

	stream >> padding;
	stream >> dimensions;
	stream >> radius2;
}

void bhkBoxShape::Put(NiStream& stream) {
	bhkConvexShape::Put(stream);

	stream << padding;
	stream << dimensions;
	stream << radius2;
}


void bhkTransformShape::Get(NiStream& stream) {
	bhkShape::Get(stream);

	shapeRef.Get(stream);
	stream >> material;
	stream >> radius;
	stream >> padding;
	stream >> xform;
}

void bhkTransformShape::Put(NiStream& stream) {
	bhkShape::Put(stream);

	shapeRef.Put(stream);
	stream << material;
	stream << radius;
	stream << padding;
	stream << xform;
}

void bhkTransformShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&shapeRef);
}


void bhkCapsuleShape::Get(NiStream& stream) {
	bhkConvexShape::Get(stream);

	stream >> padding;
	stream >> point1;
	stream >> radius1;
	stream >> point2;
	stream >> radius2;
}

void bhkCapsuleShape::Put(NiStream& stream) {
	bhkConvexShape::Put(stream);

	stream << padding;
	stream << point1;
	stream << radius1;
	stream << point2;
	stream << radius2;
}


void bhkMoppBvTreeShape::Get(NiStream& stream) {
	bhkBvTreeShape::Get(stream);

	shapeRef.Get(stream);
	stream >> userData;
	stream >> shapeCollection;
	stream >> code;
	stream >> scale;
	stream >> dataSize;
	stream >> offset;

	if (stream.GetVersion().User() >= 12)
		stream >> buildType;

	data.resize(dataSize);
	for (int i = 0; i < dataSize; i++)
		stream >> data[i];
}

void bhkMoppBvTreeShape::Put(NiStream& stream) {
	bhkBvTreeShape::Put(stream);

	shapeRef.Put(stream);
	stream << userData;
	stream << shapeCollection;
	stream << code;
	stream << scale;
	stream << dataSize;
	stream << offset;

	if (stream.GetVersion().User() >= 12)
		stream << buildType;

	for (int i = 0; i < dataSize; i++)
		stream << data[i];
}

void bhkMoppBvTreeShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkBvTreeShape::GetChildRefs(refs);

	refs.insert(&shapeRef);
}


void bhkNiTriStripsShape::Get(NiStream& stream) {
	bhkShape::Get(stream);

	stream >> material;
	stream >> radius;
	stream >> unused1;
	stream >> unused2;
	stream >> unused3;
	stream >> unused4;
	stream >> unused5;
	stream >> growBy;
	stream >> scale;

	partRefs.Get(stream);

	stream >> numFilters;
	filters.resize(numFilters);
	for (int i = 0; i < numFilters; i++)
		stream >> filters[i];
}

void bhkNiTriStripsShape::Put(NiStream& stream) {
	bhkShape::Put(stream);

	stream << material;
	stream << radius;
	stream << unused1;
	stream << unused2;
	stream << unused3;
	stream << unused4;
	stream << unused5;
	stream << growBy;
	stream << scale;

	partRefs.Put(stream);

	stream << numFilters;
	for (int i = 0; i < numFilters; i++)
		stream << filters[i];
}

void bhkNiTriStripsShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShape::GetChildRefs(refs);

	partRefs.GetIndexPtrs(refs);
}

BlockRefArray<NiTriStripsData>& bhkNiTriStripsShape::GetParts() {
	return partRefs;
}


void bhkListShape::Get(NiStream& stream) {
	bhkShapeCollection::Get(stream);

	subShapeRefs.Get(stream);

	stream >> material;
	stream >> childShapeProp;
	stream >> childFilterProp;

	stream >> numUnkInts;
	unkInts.resize(numUnkInts);

	for (int i = 0; i < numUnkInts; i++)
		stream >> unkInts[i];
}

void bhkListShape::Put(NiStream& stream) {
	bhkShapeCollection::Put(stream);

	subShapeRefs.Put(stream);

	stream << material;
	stream << childShapeProp;
	stream << childFilterProp;

	stream << numUnkInts;
	for (int i = 0; i < numUnkInts; i++)
		stream << unkInts[i];
}

void bhkListShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShapeCollection::GetChildRefs(refs);

	subShapeRefs.GetIndexPtrs(refs);
}

BlockRefArray<bhkShape>& bhkListShape::GetSubShapes() {
	return subShapeRefs;
}


void hkPackedNiTriStripsData::Get(NiStream& stream) {
	bhkShapeCollection::Get(stream);

	stream >> keyCount;

	if (stream.GetVersion().Stream() > 11) {
		triData.resize(keyCount);
		for (int i = 0; i < keyCount; i++)
			stream >> triData[i];
	}
	else {
		triNormData.resize(keyCount);
		for (int i = 0; i < keyCount; i++)
			stream >> triNormData[i];
	}

	stream >> numVerts;

	if (stream.GetVersion().Stream() > 11)
		stream >> unkByte;

	compressedVertData.resize(numVerts);
	for (int i = 0; i < numVerts; i++)
		stream >> compressedVertData[i];

	if (stream.GetVersion().Stream() > 11) {
		stream >> partCount;
		data.resize(partCount);
		for (int i = 0; i < partCount; i++)
			stream >> data[i];
	}
}

void hkPackedNiTriStripsData::Put(NiStream& stream) {
	bhkShapeCollection::Put(stream);

	stream << keyCount;

	if (stream.GetVersion().Stream() > 11) {
		for (int i = 0; i < keyCount; i++)
			stream << triData[i];
	}
	else {
		for (int i = 0; i < keyCount; i++)
			stream << triNormData[i];
	}

	stream << numVerts;

	if (stream.GetVersion().Stream() > 11)
		stream << unkByte;

	for (int i = 0; i < numVerts; i++)
		stream << compressedVertData[i];

	if (stream.GetVersion().Stream() > 11) {
		stream << partCount;
		for (int i = 0; i < partCount; i++)
			stream << data[i];
	}
}


void bhkPackedNiTriStripsShape::Get(NiStream& stream) {
	bhkShapeCollection::Get(stream);

	if (stream.GetVersion().Stream() <= 11) {
		stream >> partCount;
		data.resize(partCount);
		for (int i = 0; i < partCount; i++)
			stream >> data[i];
	}

	stream >> userData;
	stream >> unused1;
	stream >> radius;
	stream >> unused2;
	stream >> scaling;
	stream >> radius2;
	stream >> scaling2;
	dataRef.Get(stream);
}

void bhkPackedNiTriStripsShape::Put(NiStream& stream) {
	bhkShapeCollection::Put(stream);

	if (stream.GetVersion().Stream() <= 11) {
		stream << partCount;
		for (int i = 0; i < partCount; i++)
			stream << data[i];
	}

	stream << userData;
	stream << unused1;
	stream << radius;
	stream << unused2;
	stream << scaling;
	stream << radius2;
	stream << scaling2;
	dataRef.Put(stream);
}

void bhkPackedNiTriStripsShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShapeCollection::GetChildRefs(refs);

	refs.insert(&dataRef);
}


void bhkLiquidAction::Get(NiStream& stream) {
	bhkSerializable::Get(stream);

	stream >> userData;
	stream >> unkInt1;
	stream >> unkInt2;
	stream >> initialStickForce;
	stream >> stickStrength;
	stream >> neighborDistance;
	stream >> neighborStrength;
}

void bhkLiquidAction::Put(NiStream& stream) {
	bhkSerializable::Put(stream);

	stream << userData;
	stream << unkInt1;
	stream << unkInt2;
	stream << initialStickForce;
	stream << stickStrength;
	stream << neighborDistance;
	stream << neighborStrength;
}


void bhkOrientHingedBodyAction::Get(NiStream& stream) {
	bhkSerializable::Get(stream);

	bodyRef.Get(stream);
	stream >> unkInt1;
	stream >> unkInt2;
	stream >> padding;
	stream >> hingeAxisLS;
	stream >> forwardLS;
	stream >> strength;
	stream >> damping;
	stream >> padding2;
}

void bhkOrientHingedBodyAction::Put(NiStream& stream) {
	bhkSerializable::Put(stream);

	bodyRef.Put(stream);
	stream << unkInt1;
	stream << unkInt2;
	stream << padding;
	stream << hingeAxisLS;
	stream << forwardLS;
	stream << strength;
	stream << damping;
	stream << padding2;
}

void bhkOrientHingedBodyAction::GetPtrs(std::set<Ref*>& ptrs) {
	bhkSerializable::GetPtrs(ptrs);

	ptrs.insert(&bodyRef);
}


void bhkWorldObject::Get(NiStream& stream) {
	bhkSerializable::Get(stream);

	shapeRef.Get(stream);
	stream >> collisionFilter;
	stream >> unkInt1;
	stream >> broadPhaseType;
	stream.read((char*)unkBytes, 3);
	stream >> prop;
}

void bhkWorldObject::Put(NiStream& stream) {
	bhkSerializable::Put(stream);

	shapeRef.Put(stream);
	stream << collisionFilter;
	stream << unkInt1;
	stream << broadPhaseType;
	stream.write((char*)unkBytes, 3);
	stream << prop;
}

void bhkWorldObject::GetChildRefs(std::set<Ref*>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(&shapeRef);
}


void bhkSimpleShapePhantom::Get(NiStream& stream) {
	bhkShapePhantom::Get(stream);

	stream >> padding;
	stream >> transform;
}

void bhkSimpleShapePhantom::Put(NiStream& stream) {
	bhkShapePhantom::Put(stream);

	stream << padding;
	stream << transform;
}


void bhkAabbPhantom::Get(NiStream& stream) {
	bhkShapePhantom::Get(stream);

	stream >> padding;
	stream >> aabbMin;
	stream >> aabbMax;
}

void bhkAabbPhantom::Put(NiStream& stream) {
	bhkShapePhantom::Put(stream);

	stream << padding;
	stream << aabbMin;
	stream << aabbMax;
}


void bhkConstraint::Get(NiStream& stream) {
	bhkSerializable::Get(stream);

	entityRefs.Get(stream);

	stream >> priority;
}

void bhkConstraint::Put(NiStream& stream) {
	bhkSerializable::Put(stream);

	entityRefs.Put(stream);

	stream << priority;
}

void bhkConstraint::GetPtrs(std::set<Ref*>& ptrs) {
	bhkSerializable::GetPtrs(ptrs);

	entityRefs.GetIndexPtrs(ptrs);
}

BlockRefArray<bhkEntity>& bhkConstraint::GetEntities() {
	return entityRefs;
}


void bhkHingeConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> hinge;
}

void bhkHingeConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << hinge;
}


void bhkLimitedHingeConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> limitedHinge.hinge;
	stream >> limitedHinge.minAngle;
	stream >> limitedHinge.maxAngle;
	stream >> limitedHinge.maxFriction;
	limitedHinge.motorDesc.Get(stream);
}

void bhkLimitedHingeConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << limitedHinge.hinge;
	stream << limitedHinge.minAngle;
	stream << limitedHinge.maxAngle;
	stream << limitedHinge.maxFriction;
	limitedHinge.motorDesc.Put(stream);
}


void ConstraintData::Get(NiStream& stream) {
	stream >> type;

	entityRefs.Get(stream);
	stream >> priority;

	switch (type) {
	case BallAndSocket:
		stream.read((char*)&desc1, 32);
		break;
	case Hinge:
		stream.read((char*)&desc2, 128);
		break;
	case LimitedHinge:
		stream >> desc3.hinge;
		stream >> desc3.minAngle;
		stream >> desc3.maxAngle;
		stream >> desc3.maxFriction;
		desc3.motorDesc.Get(stream);
		break;
	case Prismatic:
		stream >> desc4.slidingA;
		stream >> desc4.rotationA;
		stream >> desc4.planeA;
		stream >> desc4.pivotA;
		stream >> desc4.slidingB;
		stream >> desc4.rotationB;
		stream >> desc4.planeB;
		stream >> desc4.pivotB;
		stream >> desc4.minDistance;
		stream >> desc4.maxDistance;
		stream >> desc4.friction;
		desc4.motorDesc.Get(stream);
		break;
	case Ragdoll:
		stream >> desc5.twistA;
		stream >> desc5.planeA;
		stream >> desc5.motorA;
		stream >> desc5.pivotA;
		stream >> desc5.twistB;
		stream >> desc5.planeB;
		stream >> desc5.motorB;
		stream >> desc5.pivotB;
		stream >> desc5.coneMaxAngle;
		stream >> desc5.planeMinAngle;
		stream >> desc5.planeMaxAngle;
		stream >> desc5.twistMinAngle;
		stream >> desc5.twistMaxAngle;
		stream >> desc5.maxFriction;
		desc5.motorDesc.Get(stream);
		break;
	case StiffSpring:
		stream.read((char*)&desc6, 36);
		break;
	}

	stream >> strength;
}

void ConstraintData::Put(NiStream& stream) {
	stream << type;

	entityRefs.Put(stream, 2);
	stream << priority;

	switch (type) {
	case BallAndSocket:
		stream.write((char*)&desc1, 32);
		break;
	case Hinge:
		stream.write((char*)&desc2, 128);
		break;
	case LimitedHinge:
		stream << desc3.hinge;
		stream << desc3.minAngle;
		stream << desc3.maxAngle;
		stream << desc3.maxFriction;
		desc3.motorDesc.Put(stream);
		break;
	case Prismatic:
		stream << desc4.slidingA;
		stream << desc4.rotationA;
		stream << desc4.planeA;
		stream << desc4.pivotA;
		stream << desc4.slidingB;
		stream << desc4.rotationB;
		stream << desc4.planeB;
		stream << desc4.pivotB;
		stream << desc4.minDistance;
		stream << desc4.maxDistance;
		stream << desc4.friction;
		desc4.motorDesc.Put(stream);
		break;
	case Ragdoll:
		stream << desc5.twistA;
		stream << desc5.planeA;
		stream << desc5.motorA;
		stream << desc5.pivotA;
		stream << desc5.twistB;
		stream << desc5.planeB;
		stream << desc5.motorB;
		stream << desc5.pivotB;
		stream << desc5.coneMaxAngle;
		stream << desc5.planeMinAngle;
		stream << desc5.planeMaxAngle;
		stream << desc5.twistMinAngle;
		stream << desc5.twistMaxAngle;
		stream << desc5.maxFriction;
		desc5.motorDesc.Put(stream);
		break;
	case StiffSpring:
		stream.write((char*)&desc6, 36);
		break;
	}

	stream << strength;
}

void ConstraintData::GetPtrs(std::set<Ref*>& ptrs) {
	entityRefs.GetIndexPtrs(ptrs);
}

BlockRefArray<bhkEntity>& ConstraintData::GetEntities() {
	return entityRefs;
}


void bhkBreakableConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	subConstraint.Get(stream);
	stream >> removeWhenBroken;
}

void bhkBreakableConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	subConstraint.Put(stream);
	stream << removeWhenBroken;
}

void bhkBreakableConstraint::GetPtrs(std::set<Ref*>& ptrs) {
	bhkConstraint::GetPtrs(ptrs);

	subConstraint.GetPtrs(ptrs);
}


void bhkRagdollConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> ragdoll.twistA;
	stream >> ragdoll.planeA;
	stream >> ragdoll.motorA;
	stream >> ragdoll.pivotA;
	stream >> ragdoll.twistB;
	stream >> ragdoll.planeB;
	stream >> ragdoll.motorB;
	stream >> ragdoll.pivotB;
	stream >> ragdoll.coneMaxAngle;
	stream >> ragdoll.planeMinAngle;
	stream >> ragdoll.planeMaxAngle;
	stream >> ragdoll.twistMinAngle;
	stream >> ragdoll.twistMaxAngle;
	stream >> ragdoll.maxFriction;
	ragdoll.motorDesc.Get(stream);
}

void bhkRagdollConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << ragdoll.twistA;
	stream << ragdoll.planeA;
	stream << ragdoll.motorA;
	stream << ragdoll.pivotA;
	stream << ragdoll.twistB;
	stream << ragdoll.planeB;
	stream << ragdoll.motorB;
	stream << ragdoll.pivotB;
	stream << ragdoll.coneMaxAngle;
	stream << ragdoll.planeMinAngle;
	stream << ragdoll.planeMaxAngle;
	stream << ragdoll.twistMinAngle;
	stream << ragdoll.twistMaxAngle;
	stream << ragdoll.maxFriction;
	ragdoll.motorDesc.Put(stream);
}


void bhkStiffSpringConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> stiffSpring.pivotA;
	stream >> stiffSpring.pivotB;
	stream >> stiffSpring.length;
}

void bhkStiffSpringConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << stiffSpring.pivotA;
	stream << stiffSpring.pivotB;
	stream << stiffSpring.length;
}


void bhkPrismaticConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> prismatic.slidingA;
	stream >> prismatic.rotationA;
	stream >> prismatic.planeA;
	stream >> prismatic.pivotA;
	stream >> prismatic.slidingB;
	stream >> prismatic.rotationB;
	stream >> prismatic.planeB;
	stream >> prismatic.pivotB;
	stream >> prismatic.minDistance;
	stream >> prismatic.maxDistance;
	stream >> prismatic.friction;
	prismatic.motorDesc.Get(stream);
}

void bhkPrismaticConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << prismatic.slidingA;
	stream << prismatic.rotationA;
	stream << prismatic.planeA;
	stream << prismatic.pivotA;
	stream << prismatic.slidingB;
	stream << prismatic.rotationB;
	stream << prismatic.planeB;
	stream << prismatic.pivotB;
	stream << prismatic.minDistance;
	stream << prismatic.maxDistance;
	stream << prismatic.friction;
	prismatic.motorDesc.Put(stream);
}


void bhkMalleableConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	subConstraint.Get(stream);
}

void bhkMalleableConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	subConstraint.Put(stream);
}


void bhkBallAndSocketConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> ballAndSocket.translationA;
	stream >> ballAndSocket.translationB;
}

void bhkBallAndSocketConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << ballAndSocket.translationA;
	stream << ballAndSocket.translationB;
}


void bhkBallSocketConstraintChain::Get(NiStream& stream) {
	bhkSerializable::Get(stream);

	stream >> numPivots;
	pivots.resize(numPivots);
	for (int i = 0; i < numPivots; i++)
		stream >> pivots[i];

	stream >> tau;
	stream >> damping;
	stream >> cfm;
	stream >> maxErrorDistance;

	entityARefs.Get(stream);

	stream >> numEntities;
	entityARef.Get(stream);
	entityBRef.Get(stream);
	stream >> priority;
}

void bhkBallSocketConstraintChain::Put(NiStream& stream) {
	bhkSerializable::Put(stream);

	stream << numPivots;
	for (int i = 0; i < numPivots; i++)
		stream.write((char*)&pivots[i], 16);

	stream << tau;
	stream << damping;
	stream << cfm;
	stream << maxErrorDistance;

	entityARefs.Put(stream);

	stream << numEntities;
	entityARef.Put(stream);
	entityBRef.Put(stream);
	stream << priority;
}

void bhkBallSocketConstraintChain::GetPtrs(std::set<Ref*>& ptrs) {
	bhkSerializable::GetPtrs(ptrs);

	entityARefs.GetIndexPtrs(ptrs);
	ptrs.insert(&entityARef);
	ptrs.insert(&entityBRef);
}

BlockRefArray<bhkEntity>& bhkBallSocketConstraintChain::GetEntitiesA() {
	return entityARefs;
}

int bhkBallSocketConstraintChain::GetEntityARef() {
	return entityARef.GetIndex();
}

void bhkBallSocketConstraintChain::SetEntityARef(int entityRef) {
	entityARef.SetIndex(entityRef);
}

int bhkBallSocketConstraintChain::GetEntityBRef() {
	return entityBRef.GetIndex();
}

void bhkBallSocketConstraintChain::SetEntityBRef(int entityRef) {
	entityBRef.SetIndex(entityRef);
}


void bhkRigidBody::Get(NiStream& stream) {
	bhkEntity::Get(stream);

	stream >> collisionResponse;
	stream >> unusedByte1;
	stream >> processContactCallbackDelay;
	stream >> unkInt1;
	stream >> collisionFilterCopy;
	stream.read((char*)unkShorts2, 12);
	stream >> translation;
	stream >> rotation;
	stream >> linearVelocity;
	stream >> angularVelocity;
	stream.read((char*)inertiaMatrix, 48);
	stream >> center;
	stream >> mass;
	stream >> linearDamping;
	stream >> angularDamping;

	if (stream.GetVersion().User() >= 12) {
		stream >> timeFactor;
		stream >> gravityFactor;
	}

	stream >> friction;

	if (stream.GetVersion().User() >= 12)
		stream >> rollingFrictionMult;

	stream >> restitution;
	stream >> maxLinearVelocity;
	stream >> maxAngularVelocity;
	stream >> penetrationDepth;
	stream >> motionSystem;
	stream >> deactivatorType;
	stream >> solverDeactivation;
	stream >> qualityType;
	stream >> autoRemoveLevel;
	stream >> responseModifierFlag;
	stream >> numShapeKeysInContactPointProps;
	stream >> forceCollideOntoPpu;
	stream >> unkInt2;
	stream >> unkInt3;

	if (stream.GetVersion().User() >= 12)
		stream >> unkInt4;

	constraintRefs.Get(stream);

	if (stream.GetVersion().User() <= 11)
		stream >> unkInt5;

	if (stream.GetVersion().User() >= 12)
		stream >> unkShort3;
}

void bhkRigidBody::Put(NiStream& stream) {
	bhkEntity::Put(stream);

	stream << collisionResponse;
	stream << unusedByte1;
	stream << processContactCallbackDelay;
	stream << unkInt1;
	stream << collisionFilterCopy;
	stream.write((char*)unkShorts2, 12);
	stream << translation;
	stream << rotation;
	stream << linearVelocity;
	stream << angularVelocity;
	stream.write((char*)inertiaMatrix, 48);
	stream << center;
	stream << mass;
	stream << linearDamping;
	stream << angularDamping;

	if (stream.GetVersion().User() >= 12) {
		stream << timeFactor;
		stream << gravityFactor;
	}

	stream << friction;

	if (stream.GetVersion().User() >= 12)
		stream << rollingFrictionMult;

	stream << restitution;
	stream << maxLinearVelocity;
	stream << maxAngularVelocity;
	stream << penetrationDepth;
	stream << motionSystem;
	stream << deactivatorType;
	stream << solverDeactivation;
	stream << qualityType;
	stream << autoRemoveLevel;
	stream << responseModifierFlag;
	stream << numShapeKeysInContactPointProps;
	stream << forceCollideOntoPpu;
	stream << unkInt2;
	stream << unkInt3;

	if (stream.GetVersion().User() >= 12)
		stream << unkInt4;

	constraintRefs.Put(stream);

	if (stream.GetVersion().User() <= 11)
		stream << unkInt5;

	if (stream.GetVersion().User() >= 12)
		stream << unkShort3;
}

void bhkRigidBody::GetChildRefs(std::set<Ref*>& refs) {
	bhkEntity::GetChildRefs(refs);

	constraintRefs.GetIndexPtrs(refs);
}

BlockRefArray<bhkSerializable>& bhkRigidBody::GetConstraints() {
	return constraintRefs;
}


void bhkCompressedMeshShapeData::Get(NiStream& stream) {
	bhkRefObject::Get(stream);

	stream >> bitsPerIndex;
	stream >> bitsPerWIndex;
	stream >> maskWIndex;
	stream >> maskIndex;
	stream >> error;
	stream >> aabbBoundMin;
	stream >> aabbBoundMax;
	stream >> weldingType;
	stream >> materialType;

	stream >> numMat32;
	mat32.resize(numMat32);
	for (int i = 0; i < numMat32; i++)
		stream >> mat32[i];

	stream >> numMat16;
	mat16.resize(numMat16);
	for (int i = 0; i < numMat16; i++)
		stream >> mat16[i];

	stream >> numMat8;
	mat8.resize(numMat8);
	for (int i = 0; i < numMat8; i++)
		stream >> mat8[i];

	stream >> numMaterials;
	materials.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		stream >> materials[i];

	stream >> numNamedMat;

	stream >> numTransforms;
	transforms.resize(numTransforms);
	for (int i = 0; i < numTransforms; i++)
		stream >> transforms[i];

	stream >> numBigVerts;
	bigVerts.resize(numBigVerts);
	for (int i = 0; i < numBigVerts; i++)
		stream >> bigVerts[i];

	stream >> numBigTris;
	bigTris.resize(numBigTris);
	for (int i = 0; i < numBigTris; i++)
		stream.read((char*)&bigTris[i], 12);

	stream >> numChunks;
	chunks.resize(numChunks);
	for (int i = 0; i < numChunks; i++) {
		stream >> chunks[i].translation;
		stream >> chunks[i].matIndex;
		stream >> chunks[i].reference;
		stream >> chunks[i].transformIndex;

		stream >> chunks[i].numVerts;
		chunks[i].verts.resize(chunks[i].numVerts);
		for (int j = 0; j < chunks[i].numVerts; j++)
			stream >> chunks[i].verts[j];

		stream >> chunks[i].numIndices;
		chunks[i].indices.resize(chunks[i].numIndices);
		for (int j = 0; j < chunks[i].numIndices; j++)
			stream >> chunks[i].indices[j];

		stream >> chunks[i].numStrips;
		chunks[i].strips.resize(chunks[i].numStrips);
		for (int j = 0; j < chunks[i].numStrips; j++)
			stream >> chunks[i].strips[j];

		stream >> chunks[i].numWeldingInfo;
		chunks[i].weldingInfo.resize(chunks[i].numWeldingInfo);
		for (int j = 0; j < chunks[i].numWeldingInfo; j++)
			stream >> chunks[i].weldingInfo[j];
	}

	stream >> numConvexPieceA;
}

void bhkCompressedMeshShapeData::Put(NiStream& stream) {
	bhkRefObject::Put(stream);

	stream << bitsPerIndex;
	stream << bitsPerWIndex;
	stream << maskWIndex;
	stream << maskIndex;
	stream << error;
	stream << aabbBoundMin;
	stream << aabbBoundMax;
	stream << weldingType;
	stream << materialType;

	stream << numMat32;
	for (int i = 0; i < numMat32; i++)
		stream << mat32[i];

	stream << numMat16;
	for (int i = 0; i < numMat16; i++)
		stream << mat16[i];

	stream << numMat8;
	for (int i = 0; i < numMat8; i++)
		stream << mat8[i];

	stream << numMaterials;
	for (int i = 0; i < numMaterials; i++)
		stream << materials[i];

	stream << numNamedMat;

	stream << numTransforms;
	for (int i = 0; i < numTransforms; i++)
		stream << transforms[i];

	stream << numBigVerts;
	for (int i = 0; i < numBigVerts; i++)
		stream << bigVerts[i];

	stream << numBigTris;
	for (int i = 0; i < numBigTris; i++)
		stream.write((char*)&bigTris[i], 12);

	stream << numChunks;
	for (int i = 0; i < numChunks; i++) {
		stream << chunks[i].translation;
		stream << chunks[i].matIndex;
		stream << chunks[i].reference;
		stream << chunks[i].transformIndex;

		stream << chunks[i].numVerts;
		for (int j = 0; j < chunks[i].numVerts; j++)
			stream << chunks[i].verts[j];

		stream << chunks[i].numIndices;
		for (int j = 0; j < chunks[i].numIndices; j++)
			stream << chunks[i].indices[j];

		stream << chunks[i].numStrips;
		for (int j = 0; j < chunks[i].numStrips; j++)
			stream << chunks[i].strips[j];

		stream << chunks[i].numWeldingInfo;
		for (int j = 0; j < chunks[i].numWeldingInfo; j++)
			stream << chunks[i].weldingInfo[j];
	}

	stream << numConvexPieceA;
}


void bhkCompressedMeshShape::Get(NiStream& stream) {
	bhkShape::Get(stream);

	targetRef.Get(stream);
	stream >> userData;
	stream >> radius;
	stream >> unkFloat;
	stream >> scaling;
	stream >> radius2;
	stream >> scaling2;
	dataRef.Get(stream);
}

void bhkCompressedMeshShape::Put(NiStream& stream) {
	bhkShape::Put(stream);

	targetRef.Put(stream);
	stream << userData;
	stream << radius;
	stream << unkFloat;
	stream << scaling;
	stream << radius2;
	stream << scaling2;
	dataRef.Put(stream);
}

void bhkCompressedMeshShape::GetChildRefs(std::set<Ref*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&dataRef);
}

void bhkCompressedMeshShape::GetPtrs(std::set<Ref*>& ptrs) {
	bhkShape::GetPtrs(ptrs);

	ptrs.insert(&targetRef);
}


void bhkPoseArray::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> numBones;
	bones.resize(numBones);
	for (int i = 0; i < numBones; i++)
		bones[i].Get(stream);

	stream >> numPoses;
	poses.resize(numPoses);
	for (int i = 0; i < numPoses; i++)
		poses[i].Get(stream);
}

void bhkPoseArray::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << numBones;
	for (int i = 0; i < numBones; i++)
		bones[i].Put(stream);

	stream << numPoses;
	for (int i = 0; i < numPoses; i++)
		poses[i].Put(stream);
}

void bhkPoseArray::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	for (auto &b : bones)
		refs.insert(&b);
}


void bhkRagdollTemplate::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numBones;
	boneRefs.Get(stream);
}

void bhkRagdollTemplate::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numBones;
	boneRefs.Put(stream);
}

void bhkRagdollTemplate::GetChildRefs(std::set<Ref*>& refs) {
	NiExtraData::GetChildRefs(refs);

	boneRefs.GetIndexPtrs(refs);
}

BlockRefArray<NiObject>& bhkRagdollTemplate::GetBones() {
	return boneRefs;
}


void bhkRagdollTemplateData::Get(NiStream& stream) {
	NiObject::Get(stream);

	name.Get(stream);
	stream >> mass;
	stream >> restitution;
	stream >> friction;
	stream >> radius;
	stream >> material;

	stream >> numConstraints;
	constraints.resize(numConstraints);
	for (int i = 0; i < numConstraints; i++)
		constraints[i].Get(stream);
}

void bhkRagdollTemplateData::Put(NiStream& stream) {
	NiObject::Put(stream);

	name.Put(stream);
	stream << mass;
	stream << restitution;
	stream << friction;
	stream << radius;
	stream << material;

	stream << numConstraints;
	for (int i = 0; i < numConstraints; i++)
		constraints[i].Put(stream);
}

void bhkRagdollTemplateData::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name);
}
