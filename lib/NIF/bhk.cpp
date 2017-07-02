/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "bhk.h"

NiCollisionObject::NiCollisionObject(NiStream& stream) : NiCollisionObject() {
	Get(stream);
}

void NiCollisionObject::Get(NiStream& stream) {
	NiObject::Get(stream);

	targetRef.Get(stream);
}

void NiCollisionObject::Put(NiStream& stream) {
	NiObject::Put(stream);

	targetRef.Put(stream);
}

void NiCollisionObject::GetPtrs(std::set<int*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef.index);
}

int NiCollisionObject::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


bhkNiCollisionObject::bhkNiCollisionObject() : NiCollisionObject() {
}

bhkNiCollisionObject::bhkNiCollisionObject(NiStream& stream) : bhkNiCollisionObject() {
	Get(stream);
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

void bhkNiCollisionObject::GetChildRefs(std::set<int*>& refs) {
	NiCollisionObject::GetChildRefs(refs);

	refs.insert(&bodyRef.index);
}

int bhkNiCollisionObject::CalcBlockSize(NiVersion& version) {
	NiCollisionObject::CalcBlockSize(version);

	blockSize += 6;

	return blockSize;
}


bhkCollisionObject::bhkCollisionObject() : bhkNiCollisionObject() {
}

bhkCollisionObject::bhkCollisionObject(NiStream& stream) : bhkCollisionObject() {
	Get(stream);
}


bhkNPCollisionObject::bhkNPCollisionObject() : bhkCollisionObject() {
}

bhkNPCollisionObject::bhkNPCollisionObject(NiStream& stream) : bhkNPCollisionObject() {
	Get(stream);
}

void bhkNPCollisionObject::Get(NiStream& stream) {
	bhkCollisionObject::Get(stream);

	stream >> bodyID;
}

void bhkNPCollisionObject::Put(NiStream& stream) {
	bhkCollisionObject::Put(stream);

	stream << bodyID;
}

int bhkNPCollisionObject::CalcBlockSize(NiVersion& version) {
	bhkCollisionObject::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


bhkPCollisionObject::bhkPCollisionObject() : bhkNiCollisionObject() {
}

bhkPCollisionObject::bhkPCollisionObject(NiStream& stream) : bhkPCollisionObject() {
	Get(stream);
}


bhkSPCollisionObject::bhkSPCollisionObject() : bhkPCollisionObject() {
}

bhkSPCollisionObject::bhkSPCollisionObject(NiStream& stream) : bhkSPCollisionObject() {
	Get(stream);
}


bhkBlendCollisionObject::bhkBlendCollisionObject() : bhkCollisionObject() {
}

bhkBlendCollisionObject::bhkBlendCollisionObject(NiStream& stream) : bhkBlendCollisionObject() {
	Get(stream);
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

int bhkBlendCollisionObject::CalcBlockSize(NiVersion& version) {
	bhkCollisionObject::CalcBlockSize(version);

	blockSize += 8;

	return blockSize;
}


bhkPhysicsSystem::bhkPhysicsSystem(const uint size) {
	numBytes = size;
	data.resize(size);
}

bhkPhysicsSystem::bhkPhysicsSystem(NiStream& stream) : bhkPhysicsSystem() {
	Get(stream);
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

int bhkPhysicsSystem::CalcBlockSize(NiVersion& version) {
	BSExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


bhkPlaneShape::bhkPlaneShape(NiStream& stream) : bhkPlaneShape() {
	Get(stream);
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

int bhkPlaneShape::CalcBlockSize(NiVersion& version) {
	bhkHeightFieldShape::CalcBlockSize(version);

	blockSize += 64;

	return blockSize;
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

int bhkSphereRepShape::CalcBlockSize(NiVersion& version) {
	bhkShape::CalcBlockSize(version);

	blockSize += 8;

	return blockSize;
}


bhkConvexVerticesShape::bhkConvexVerticesShape(NiStream& stream) : bhkConvexVerticesShape() {
	Get(stream);
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

int bhkConvexVerticesShape::CalcBlockSize(NiVersion& version) {
	bhkConvexShape::CalcBlockSize(version);

	blockSize += 32;
	blockSize += numVerts * 16;
	blockSize += numNormals * 16;

	return blockSize;
}


bhkBoxShape::bhkBoxShape(NiStream& stream) : bhkBoxShape() {
	Get(stream);
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

int bhkBoxShape::CalcBlockSize(NiVersion& version) {
	bhkConvexShape::CalcBlockSize(version);

	blockSize += 24;

	return blockSize;
}


bhkSphereShape::bhkSphereShape(NiStream& stream) : bhkSphereShape() {
	Get(stream);
}


bhkTransformShape::bhkTransformShape(NiStream& stream) : bhkTransformShape() {
	Get(stream);
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

void bhkTransformShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkTransformShape::CalcBlockSize(NiVersion& version) {
	bhkShape::CalcBlockSize(version);

	blockSize += 84;

	return blockSize;
}


bhkConvexTransformShape::bhkConvexTransformShape() : bhkTransformShape() {
}

bhkConvexTransformShape::bhkConvexTransformShape(NiStream& stream) : bhkConvexTransformShape() {
	Get(stream);
}


bhkCapsuleShape::bhkCapsuleShape(NiStream& stream) : bhkCapsuleShape() {
	Get(stream);
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

int bhkCapsuleShape::CalcBlockSize(NiVersion& version) {
	bhkConvexShape::CalcBlockSize(version);

	blockSize += 40;

	return blockSize;
}


bhkMoppBvTreeShape::bhkMoppBvTreeShape(NiStream& stream) : bhkMoppBvTreeShape() {
	Get(stream);
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

void bhkMoppBvTreeShape::GetChildRefs(std::set<int*>& refs) {
	bhkBvTreeShape::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkMoppBvTreeShape::CalcBlockSize(NiVersion& version) {
	bhkBvTreeShape::CalcBlockSize(version);

	blockSize += 40;

	if (version.User() >= 12)
		blockSize += 1;

	blockSize += dataSize;

	return blockSize;
}


bhkNiTriStripsShape::bhkNiTriStripsShape(NiStream& stream) : bhkNiTriStripsShape() {
	Get(stream);
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

void bhkNiTriStripsShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	partRefs.GetIndexPtrs(refs);
}

int bhkNiTriStripsShape::CalcBlockSize(NiVersion& version) {
	bhkShape::CalcBlockSize(version);

	blockSize += 52;
	blockSize += partRefs.CalcBlockSize();
	blockSize += numFilters * 4;

	return blockSize;
}


bhkListShape::bhkListShape(NiStream& stream) : bhkListShape() {
	Get(stream);
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

void bhkListShape::GetChildRefs(std::set<int*>& refs) {
	bhkShapeCollection::GetChildRefs(refs);

	subShapeRefs.GetIndexPtrs(refs);
}

int bhkListShape::CalcBlockSize(NiVersion& version) {
	bhkShapeCollection::CalcBlockSize(version);

	blockSize += 32;
	blockSize += subShapeRefs.CalcBlockSize();
	blockSize += 4 * numUnkInts;

	return blockSize;
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

void bhkWorldObject::GetChildRefs(std::set<int*>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkWorldObject::CalcBlockSize(NiVersion& version) {
	bhkSerializable::CalcBlockSize(version);

	blockSize += 28;

	return blockSize;
}


bhkSimpleShapePhantom::bhkSimpleShapePhantom(NiStream& stream) : bhkSimpleShapePhantom() {
	Get(stream);
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

int bhkSimpleShapePhantom::CalcBlockSize(NiVersion& version) {
	bhkShapePhantom::CalcBlockSize(version);

	blockSize += 72;

	return blockSize;
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

void bhkConstraint::GetPtrs(std::set<int*>& ptrs) {
	bhkSerializable::GetPtrs(ptrs);

	entityRefs.GetIndexPtrs(ptrs);
}

int bhkConstraint::CalcBlockSize(NiVersion& version) {
	bhkSerializable::CalcBlockSize(version);

	blockSize += 4;
	blockSize += entityRefs.CalcBlockSize();

	return blockSize;
}


bhkHingeConstraint::bhkHingeConstraint(NiStream& stream) : bhkHingeConstraint() {
	Get(stream);
}

void bhkHingeConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	stream >> hinge;
}

void bhkHingeConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	stream << hinge;
}

int bhkHingeConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 128;

	return blockSize;
}


bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(NiStream& stream) : bhkLimitedHingeConstraint() {
	Get(stream);
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

int bhkLimitedHingeConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 141;
	blockSize += limitedHinge.motorDesc.CalcMotorSize();

	return blockSize;
}


void SubConstraintDesc::Get(NiStream& stream) {
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

void SubConstraintDesc::Put(NiStream& stream) {
	stream << type;

	entityRefs.Put(stream);
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

void SubConstraintDesc::GetPtrs(std::set<int*>& ptrs) {
	entityRefs.GetIndexPtrs(ptrs);
}

int SubConstraintDesc::CalcDescSize() {
	int descSize = 12;
	descSize += entityRefs.CalcBlockSize();

	switch (type) {
	case BallAndSocket:
		descSize += 32;
		break;
	case Hinge:
		descSize += 128;
		break;
	case LimitedHinge:
		descSize += 141;
		descSize += desc3.motorDesc.CalcMotorSize();
		break;
	case Prismatic:
		descSize += 116;
		descSize += desc4.motorDesc.CalcMotorSize();
		break;
	case Ragdoll:
		descSize += 153;
		descSize += desc5.motorDesc.CalcMotorSize();
		break;
	case StiffSpring:
		descSize += 36;
		break;
	}

	return descSize;
}


bhkBreakableConstraint::bhkBreakableConstraint(NiStream& stream) : bhkBreakableConstraint() {
	Get(stream);
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

void bhkBreakableConstraint::GetPtrs(std::set<int*>& ptrs) {
	bhkConstraint::GetPtrs(ptrs);

	subConstraint.GetPtrs(ptrs);
}

int bhkBreakableConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 1;
	blockSize += subConstraint.CalcDescSize();

	return blockSize;
}


bhkRagdollConstraint::bhkRagdollConstraint(NiStream& stream) : bhkRagdollConstraint() {
	Get(stream);
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

int bhkRagdollConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 153;
	blockSize += ragdoll.motorDesc.CalcMotorSize();

	return blockSize;
}


bhkStiffSpringConstraint::bhkStiffSpringConstraint(NiStream& stream) : bhkStiffSpringConstraint() {
	Get(stream);
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

int bhkStiffSpringConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 36;

	return blockSize;
}


bhkPrismaticConstraint::bhkPrismaticConstraint(NiStream& stream) : bhkPrismaticConstraint() {
	Get(stream);
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

int bhkPrismaticConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 116;
	blockSize += prismatic.motorDesc.CalcMotorSize();

	return blockSize;
}


bhkMalleableConstraint::bhkMalleableConstraint(NiStream& stream) : bhkMalleableConstraint() {
	Get(stream);
}

void bhkMalleableConstraint::Get(NiStream& stream) {
	bhkConstraint::Get(stream);

	subConstraint.Get(stream);
}

void bhkMalleableConstraint::Put(NiStream& stream) {
	bhkConstraint::Put(stream);

	subConstraint.Put(stream);
}

int bhkMalleableConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += subConstraint.CalcDescSize();
	return blockSize;
}


bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(NiStream& stream) : bhkBallAndSocketConstraint() {
	Get(stream);
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

int bhkBallAndSocketConstraint::CalcBlockSize(NiVersion& version) {
	bhkConstraint::CalcBlockSize(version);

	blockSize += 32;

	return blockSize;
}


bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(NiStream& stream) : bhkBallSocketConstraintChain() {
	Get(stream);
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

void bhkBallSocketConstraintChain::GetPtrs(std::set<int*>& ptrs) {
	bhkSerializable::GetPtrs(ptrs);

	entityARefs.GetIndexPtrs(ptrs);
	ptrs.insert(&entityARef.index);
	ptrs.insert(&entityBRef.index);
}

int bhkBallSocketConstraintChain::CalcBlockSize(NiVersion& version) {
	bhkSerializable::CalcBlockSize(version);

	blockSize += 36;
	blockSize += numPivots * 16;
	blockSize += entityARefs.CalcBlockSize();

	return blockSize;
}


bhkRigidBody::bhkRigidBody(NiStream& stream) : bhkRigidBody() {
	Get(stream);
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

void bhkRigidBody::GetChildRefs(std::set<int*>& refs) {
	bhkEntity::GetChildRefs(refs);

	constraintRefs.GetIndexPtrs(refs);
}

int bhkRigidBody::CalcBlockSize(NiVersion& version) {
	bhkEntity::CalcBlockSize(version);

	blockSize += 200;
	blockSize += constraintRefs.CalcBlockSize();

	if (version.User() <= 11)
		blockSize += 4;

	if (version.User() >= 12)
		blockSize += 18;

	return blockSize;
}


bhkRigidBodyT::bhkRigidBodyT() : bhkRigidBody() {
}

bhkRigidBodyT::bhkRigidBodyT(NiStream& stream) : bhkRigidBodyT() {
	Get(stream);
}


bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(NiStream& stream) : bhkCompressedMeshShapeData() {
	Get(stream);
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

int bhkCompressedMeshShapeData::CalcBlockSize(NiVersion& version) {
	bhkRefObject::CalcBlockSize(version);

	blockSize += 94;
	blockSize += numMat32 * 4;
	blockSize += numMat16 * 4;
	blockSize += numMat8 * 4;
	blockSize += numMaterials * 8;
	blockSize += numTransforms * 32;
	blockSize += numBigVerts * 16;
	blockSize += numBigTris * 12;

	blockSize += numChunks * 40;
	for (int i = 0; i < numChunks; i++) {
		blockSize += chunks[i].numVerts * 2;
		blockSize += chunks[i].numIndices * 2;
		blockSize += chunks[i].numStrips * 2;
		blockSize += chunks[i].numWeldingInfo * 2;
	}

	return blockSize;
}


bhkCompressedMeshShape::bhkCompressedMeshShape(NiStream& stream) : bhkCompressedMeshShape() {
	Get(stream);
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

void bhkCompressedMeshShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

void bhkCompressedMeshShape::GetPtrs(std::set<int*>& ptrs) {
	bhkShape::GetPtrs(ptrs);

	ptrs.insert(&targetRef.index);
}

int bhkCompressedMeshShape::CalcBlockSize(NiVersion& version) {
	bhkShape::CalcBlockSize(version);

	blockSize += 56;

	return blockSize;
}
