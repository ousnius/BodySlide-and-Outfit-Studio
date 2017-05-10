/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "bhk.h"

NiCollisionObject::NiCollisionObject(NiHeader* hdr) {
	NiObject::Init(hdr);
}

NiCollisionObject::NiCollisionObject(std::fstream& file, NiHeader* hdr) : NiCollisionObject(hdr) {
	Get(file);
}

void NiCollisionObject::Get(std::fstream& file) {
	NiObject::Get(file);

	targetRef.Get(file);
}

void NiCollisionObject::Put(std::fstream& file) {
	NiObject::Put(file);

	targetRef.Put(file);
}

int NiCollisionObject::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


bhkNiCollisionObject::bhkNiCollisionObject(NiHeader* hdr) : NiCollisionObject(hdr) {
}

bhkNiCollisionObject::bhkNiCollisionObject(std::fstream& file, NiHeader* hdr) : bhkNiCollisionObject(hdr) {
	Get(file);
}

void bhkNiCollisionObject::Get(std::fstream& file) {
	NiCollisionObject::Get(file);

	file.read((char*)&flags, 2);
	bodyRef.Get(file);
}

void bhkNiCollisionObject::Put(std::fstream& file) {
	NiCollisionObject::Put(file);

	file.write((char*)&flags, 2);
	bodyRef.Put(file);
}

void bhkNiCollisionObject::GetChildRefs(std::set<int*>& refs) {
	NiCollisionObject::GetChildRefs(refs);

	refs.insert(&bodyRef.index);
}

int bhkNiCollisionObject::CalcBlockSize() {
	NiCollisionObject::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


bhkCollisionObject::bhkCollisionObject(NiHeader* hdr) : bhkNiCollisionObject(hdr) {
}

bhkCollisionObject::bhkCollisionObject(std::fstream& file, NiHeader* hdr) : bhkCollisionObject(hdr) {
	Get(file);
}


bhkNPCollisionObject::bhkNPCollisionObject(NiHeader* hdr) : bhkCollisionObject(hdr) {
}

bhkNPCollisionObject::bhkNPCollisionObject(std::fstream& file, NiHeader* hdr) : bhkNPCollisionObject(hdr) {
	Get(file);
}

void bhkNPCollisionObject::Get(std::fstream& file) {
	bhkCollisionObject::Get(file);

	file.read((char*)&bodyID, 4);
}

void bhkNPCollisionObject::Put(std::fstream& file) {
	bhkCollisionObject::Put(file);

	file.write((char*)&bodyID, 4);
}

int bhkNPCollisionObject::CalcBlockSize() {
	bhkCollisionObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


bhkPCollisionObject::bhkPCollisionObject(NiHeader* hdr) : bhkNiCollisionObject(hdr) {
}

bhkPCollisionObject::bhkPCollisionObject(std::fstream& file, NiHeader* hdr) : bhkPCollisionObject(hdr) {
	Get(file);
}


bhkSPCollisionObject::bhkSPCollisionObject(NiHeader* hdr) : bhkPCollisionObject(hdr) {
}

bhkSPCollisionObject::bhkSPCollisionObject(std::fstream& file, NiHeader* hdr) : bhkSPCollisionObject(hdr) {
	Get(file);
}


bhkBlendCollisionObject::bhkBlendCollisionObject(NiHeader* hdr) : bhkCollisionObject(hdr) {
}

bhkBlendCollisionObject::bhkBlendCollisionObject(std::fstream& file, NiHeader* hdr) : bhkBlendCollisionObject(hdr) {
	Get(file);
}

void bhkBlendCollisionObject::Get(std::fstream& file) {
	bhkCollisionObject::Get(file);

	file.read((char*)&heirGain, 4);
	file.read((char*)&velGain, 4);
}

void bhkBlendCollisionObject::Put(std::fstream& file) {
	bhkCollisionObject::Put(file);

	file.write((char*)&heirGain, 4);
	file.write((char*)&velGain, 4);
}

int bhkBlendCollisionObject::CalcBlockSize() {
	bhkCollisionObject::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


bhkPhysicsSystem::bhkPhysicsSystem(NiHeader* hdr, const uint size) {
	BSExtraData::Init(hdr);

	numBytes = size;
	data.resize(size);
}

bhkPhysicsSystem::bhkPhysicsSystem(std::fstream& file, NiHeader* hdr) : bhkPhysicsSystem(hdr) {
	Get(file);
}

void bhkPhysicsSystem::Get(std::fstream& file) {
	BSExtraData::Get(file);

	file.read((char*)&numBytes, 4);
	data.resize(numBytes);
	if (data.empty())
		return;

	file.read(&data[0], numBytes);
}

void bhkPhysicsSystem::Put(std::fstream& file) {
	BSExtraData::Put(file);

	file.write((char*)&numBytes, 4);
	if (data.empty())
		return;

	file.write(&data[0], numBytes);
}

int bhkPhysicsSystem::CalcBlockSize() {
	BSExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


bhkPlaneShape::bhkPlaneShape(NiHeader* hdr) {
	bhkHeightFieldShape::Init(hdr);
}

bhkPlaneShape::bhkPlaneShape(std::fstream& file, NiHeader* hdr) : bhkPlaneShape(hdr) {
	Get(file);
}

void bhkPlaneShape::Get(std::fstream& file) {
	bhkHeightFieldShape::Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&unkVec, 12);
	file.read((char*)&direction, 12);
	file.read((char*)&constant, 4);
	file.read((char*)&halfExtents, 16);
	file.read((char*)&center, 16);
}

void bhkPlaneShape::Put(std::fstream& file) {
	bhkHeightFieldShape::Put(file);

	file.write((char*)&material, 4);
	file.write((char*)&unkVec, 12);
	file.write((char*)&direction, 12);
	file.write((char*)&constant, 4);
	file.write((char*)&halfExtents, 16);
	file.write((char*)&center, 16);
}

int bhkPlaneShape::CalcBlockSize() {
	bhkHeightFieldShape::CalcBlockSize();

	blockSize += 64;

	return blockSize;
}


void bhkSphereRepShape::Get(std::fstream& file) {
	bhkShape::Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&radius, 4);
}

void bhkSphereRepShape::Put(std::fstream& file) {
	bhkShape::Put(file);

	file.write((char*)&material, 4);
	file.write((char*)&radius, 4);
}

int bhkSphereRepShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


bhkConvexVerticesShape::bhkConvexVerticesShape(NiHeader* hdr) {
	bhkConvexShape::Init(hdr);

	numVerts = 0;
	numNormals = 0;
}

bhkConvexVerticesShape::bhkConvexVerticesShape(std::fstream& file, NiHeader* hdr) : bhkConvexVerticesShape(hdr) {
	Get(file);
}

void bhkConvexVerticesShape::Get(std::fstream& file) {
	bhkConvexShape::Get(file);

	file.read((char*)&vertsProp, 12);
	file.read((char*)&normalsProp, 12);

	file.read((char*)&numVerts, 4);
	verts.resize(numVerts);
	for (int i = 0; i < numVerts; i++)
		file.read((char*)&verts[i], 16);

	file.read((char*)&numNormals, 4);
	normals.resize(numNormals);
	for (int i = 0; i < numNormals; i++)
		file.read((char*)&normals[i], 16);
}

void bhkConvexVerticesShape::Put(std::fstream& file) {
	bhkConvexShape::Put(file);

	file.write((char*)&vertsProp, 12);
	file.write((char*)&normalsProp, 12);

	file.write((char*)&numVerts, 4);
	for (int i = 0; i < numVerts; i++)
		file.write((char*)&verts[i], 16);

	file.write((char*)&numNormals, 4);
	for (int i = 0; i < numNormals; i++)
		file.write((char*)&normals[i], 16);
}

int bhkConvexVerticesShape::CalcBlockSize() {
	bhkConvexShape::CalcBlockSize();

	blockSize += 32;
	blockSize += numVerts * 16;
	blockSize += numNormals * 16;

	return blockSize;
}


bhkBoxShape::bhkBoxShape(NiHeader* hdr) {
	bhkConvexShape::Init(hdr);
}

bhkBoxShape::bhkBoxShape(std::fstream& file, NiHeader* hdr) : bhkBoxShape(hdr) {
	Get(file);
}

void bhkBoxShape::Get(std::fstream& file) {
	bhkConvexShape::Get(file);

	file.read((char*)padding, 8);
	file.read((char*)&dimensions, 12);
	file.read((char*)&radius2, 4);
}

void bhkBoxShape::Put(std::fstream& file) {
	bhkConvexShape::Put(file);

	file.write((char*)padding, 8);
	file.write((char*)&dimensions, 12);
	file.write((char*)&radius2, 4);
}

int bhkBoxShape::CalcBlockSize() {
	bhkConvexShape::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


bhkSphereShape::bhkSphereShape(NiHeader* hdr) {
	bhkConvexShape::Init(hdr);
}

bhkSphereShape::bhkSphereShape(std::fstream& file, NiHeader* hdr) : bhkSphereShape(hdr) {
	Get(file);
}


bhkTransformShape::bhkTransformShape(NiHeader* hdr) {
	bhkShape::Init(hdr);
}

bhkTransformShape::bhkTransformShape(std::fstream& file, NiHeader* hdr) : bhkTransformShape(hdr) {
	Get(file);
}

void bhkTransformShape::Get(std::fstream& file) {
	bhkShape::Get(file);

	shapeRef.Get(file);
	file.read((char*)&material, 4);
	file.read((char*)&radius, 4);
	file.read((char*)padding, 8);
	file.read((char*)&xform, 64);
}

void bhkTransformShape::Put(std::fstream& file) {
	bhkShape::Put(file);

	shapeRef.Put(file);
	file.write((char*)&material, 4);
	file.write((char*)&radius, 4);
	file.write((char*)padding, 8);
	file.write((char*)&xform, 64);
}

void bhkTransformShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkTransformShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 84;

	return blockSize;
}


bhkConvexTransformShape::bhkConvexTransformShape(NiHeader* hdr) : bhkTransformShape(hdr) {
}

bhkConvexTransformShape::bhkConvexTransformShape(std::fstream& file, NiHeader* hdr) : bhkConvexTransformShape(hdr) {
	Get(file);
}


bhkCapsuleShape::bhkCapsuleShape(NiHeader* hdr) {
	bhkConvexShape::Init(hdr);
}

bhkCapsuleShape::bhkCapsuleShape(std::fstream& file, NiHeader* hdr) : bhkCapsuleShape(hdr) {
	Get(file);
}

void bhkCapsuleShape::Get(std::fstream& file) {
	bhkConvexShape::Get(file);

	file.read((char*)padding, 8);
	file.read((char*)&point1, 12);
	file.read((char*)&radius1, 4);
	file.read((char*)&point2, 12);
	file.read((char*)&radius2, 4);
}

void bhkCapsuleShape::Put(std::fstream& file) {
	bhkConvexShape::Put(file);

	file.write((char*)padding, 8);
	file.write((char*)&point1, 12);
	file.write((char*)&radius1, 4);
	file.write((char*)&point2, 12);
	file.write((char*)&radius2, 4);
}

int bhkCapsuleShape::CalcBlockSize() {
	bhkConvexShape::CalcBlockSize();

	blockSize += 40;

	return blockSize;
}


bhkMoppBvTreeShape::bhkMoppBvTreeShape(NiHeader* hdr) {
	bhkBvTreeShape::Init(hdr);
}

bhkMoppBvTreeShape::bhkMoppBvTreeShape(std::fstream& file, NiHeader* hdr) : bhkMoppBvTreeShape(hdr) {
	Get(file);
}

void bhkMoppBvTreeShape::Get(std::fstream& file) {
	bhkBvTreeShape::Get(file);

	shapeRef.Get(file);
	file.read((char*)&userData, 4);
	file.read((char*)&shapeCollection, 4);
	file.read((char*)&code, 4);
	file.read((char*)&scale, 4);
	file.read((char*)&dataSize, 4);
	file.read((char*)&offset, 16);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&buildType, 1);

	data.resize(dataSize);
	for (int i = 0; i < dataSize; i++)
		file.read((char*)&data[i], 1);
}

void bhkMoppBvTreeShape::Put(std::fstream& file) {
	bhkBvTreeShape::Put(file);

	shapeRef.Put(file);
	file.write((char*)&userData, 4);
	file.write((char*)&shapeCollection, 4);
	file.write((char*)&code, 4);
	file.write((char*)&scale, 4);
	file.write((char*)&dataSize, 4);
	file.write((char*)&offset, 16);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&buildType, 1);

	for (int i = 0; i < dataSize; i++)
		file.write((char*)&data[i], 1);
}

void bhkMoppBvTreeShape::GetChildRefs(std::set<int*>& refs) {
	bhkBvTreeShape::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkMoppBvTreeShape::CalcBlockSize() {
	bhkBvTreeShape::CalcBlockSize();

	blockSize += 40;

	if (header->GetUserVersion() >= 12)
		blockSize += 1;

	blockSize += dataSize;

	return blockSize;
}


bhkNiTriStripsShape::bhkNiTriStripsShape(NiHeader* hdr) {
	bhkShape::Init(hdr);
}

bhkNiTriStripsShape::bhkNiTriStripsShape(std::fstream& file, NiHeader* hdr) : bhkNiTriStripsShape(hdr) {
	Get(file);
}

void bhkNiTriStripsShape::Get(std::fstream& file) {
	bhkShape::Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&radius, 4);
	file.read((char*)&unused1, 4);
	file.read((char*)&unused2, 4);
	file.read((char*)&unused3, 4);
	file.read((char*)&unused4, 4);
	file.read((char*)&unused5, 4);
	file.read((char*)&growBy, 4);
	file.read((char*)&scale, 16);

	partRefs.Get(file);

	file.read((char*)&numFilters, 4);
	filters.resize(numFilters);
	for (int i = 0; i < numFilters; i++)
		file.read((char*)&filters[i], 4);
}

void bhkNiTriStripsShape::Put(std::fstream& file) {
	bhkShape::Put(file);

	file.write((char*)&material, 4);
	file.write((char*)&radius, 4);
	file.write((char*)&unused1, 4);
	file.write((char*)&unused2, 4);
	file.write((char*)&unused3, 4);
	file.write((char*)&unused4, 4);
	file.write((char*)&unused5, 4);
	file.write((char*)&growBy, 4);
	file.write((char*)&scale, 16);

	partRefs.Put(file);

	file.write((char*)&numFilters, 4);
	for (int i = 0; i < numFilters; i++)
		file.write((char*)&filters[i], 4);
}

void bhkNiTriStripsShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	partRefs.GetIndexPtrs(refs);
}

int bhkNiTriStripsShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 56;
	blockSize += partRefs.GetSize() * 4;
	blockSize += numFilters * 4;

	return blockSize;
}


bhkListShape::bhkListShape(NiHeader* hdr) {
	bhkShapeCollection::Init(hdr);
}

bhkListShape::bhkListShape(std::fstream& file, NiHeader* hdr) : bhkListShape(hdr) {
	Get(file);
}

void bhkListShape::Get(std::fstream& file) {
	bhkShapeCollection::Get(file);

	subShapeRefs.Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&childShapeProp, 12);
	file.read((char*)&childFilterProp, 12);

	file.read((char*)&numUnkInts, 4);
	unkInts.resize(numUnkInts);

	for (int i = 0; i < numUnkInts; i++)
		file.read((char*)&unkInts[i], 4);
}

void bhkListShape::Put(std::fstream& file) {
	bhkShapeCollection::Put(file);

	subShapeRefs.Put(file);

	file.write((char*)&material, 4);
	file.write((char*)&childShapeProp, 12);
	file.write((char*)&childFilterProp, 12);

	file.write((char*)&numUnkInts, 4);
	for (int i = 0; i < numUnkInts; i++)
		file.write((char*)&unkInts[i], 4);
}

void bhkListShape::GetChildRefs(std::set<int*>& refs) {
	bhkShapeCollection::GetChildRefs(refs);

	subShapeRefs.GetIndexPtrs(refs);
}

int bhkListShape::CalcBlockSize() {
	bhkShapeCollection::CalcBlockSize();

	blockSize += 36;
	blockSize += 4 * subShapeRefs.GetSize();
	blockSize += 4 * numUnkInts;

	return blockSize;
}


void bhkWorldObject::Init(NiHeader* hdr) {
	bhkSerializable::Init(hdr);
}

void bhkWorldObject::Get(std::fstream& file) {
	bhkSerializable::Get(file);

	shapeRef.Get(file);
	file.read((char*)&collisionFilter, 4);
	file.read((char*)&unkInt1, 4);
	file.read((char*)&broadPhaseType, 1);
	file.read((char*)unkBytes, 3);
	file.read((char*)&prop, 12);
}

void bhkWorldObject::Put(std::fstream& file) {
	bhkSerializable::Put(file);

	shapeRef.Put(file);
	file.write((char*)&collisionFilter, 4);
	file.write((char*)&unkInt1, 4);
	file.write((char*)&broadPhaseType, 1);
	file.write((char*)unkBytes, 3);
	file.write((char*)&prop, 12);
}

void bhkWorldObject::GetChildRefs(std::set<int*>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(&shapeRef.index);
}

int bhkWorldObject::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


bhkSimpleShapePhantom::bhkSimpleShapePhantom(NiHeader* hdr) {
	bhkShapePhantom::Init(hdr);
}

bhkSimpleShapePhantom::bhkSimpleShapePhantom(std::fstream& file, NiHeader* hdr) : bhkSimpleShapePhantom(hdr) {
	Get(file);
}

void bhkSimpleShapePhantom::Get(std::fstream& file) {
	bhkShapePhantom::Get(file);

	file.read((char*)padding, 8);
	file.read((char*)&transform, 64);
}

void bhkSimpleShapePhantom::Put(std::fstream& file) {
	bhkShapePhantom::Put(file);

	file.write((char*)padding, 8);
	file.write((char*)&transform, 64);
}

int bhkSimpleShapePhantom::CalcBlockSize() {
	bhkShapePhantom::CalcBlockSize();

	blockSize += 72;

	return blockSize;
}


void bhkConstraint::Init(NiHeader* hdr) {
	bhkSerializable::Init(hdr);
}

void bhkConstraint::Get(std::fstream& file) {
	bhkSerializable::Get(file);

	entityRefs.Get(file);

	file.read((char*)&priority, 4);
}

void bhkConstraint::Put(std::fstream& file) {
	bhkSerializable::Put(file);

	entityRefs.Put(file);

	file.write((char*)&priority, 4);
}

void bhkConstraint::GetChildRefs(std::set<int*>& refs) {
	bhkSerializable::GetChildRefs(refs);

	entityRefs.GetIndexPtrs(refs);
}

int bhkConstraint::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 8;
	blockSize += entityRefs.GetSize() * 4;

	return blockSize;
}


bhkHingeConstraint::bhkHingeConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkHingeConstraint::bhkHingeConstraint(std::fstream& file, NiHeader* hdr) : bhkHingeConstraint(hdr) {
	Get(file);
}

void bhkHingeConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&hinge, 128);
}

void bhkHingeConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&hinge, 128);
}

int bhkHingeConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 128;

	return blockSize;
}


bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(std::fstream& file, NiHeader* hdr) : bhkLimitedHingeConstraint(hdr) {
	Get(file);
}

void bhkLimitedHingeConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&limitedHinge.hinge, 128);
	file.read((char*)&limitedHinge.minAngle, 4);
	file.read((char*)&limitedHinge.maxAngle, 4);
	file.read((char*)&limitedHinge.maxFriction, 4);
	file.read((char*)&limitedHinge.enableMotor, 1);

	if (limitedHinge.enableMotor)
		file.read((char*)&limitedHinge.motor, 25);
}

void bhkLimitedHingeConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&limitedHinge.hinge, 128);
	file.write((char*)&limitedHinge.minAngle, 4);
	file.write((char*)&limitedHinge.maxAngle, 4);
	file.write((char*)&limitedHinge.maxFriction, 4);
	file.write((char*)&limitedHinge.enableMotor, 1);

	if (limitedHinge.enableMotor)
		file.write((char*)&limitedHinge.motor, 25);
}

int bhkLimitedHingeConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 141;
	if (limitedHinge.enableMotor)
		blockSize += 25;

	return blockSize;
}


void SubConstraintDesc::Get(std::fstream& file, NiObject* parent) {
	file.read((char*)&type, 4);

	entityRefs.Get(file);
	file.read((char*)&priority, 4);

	switch (type) {
	case BallAndSocket:
		file.read((char*)&desc1, 32);
		break;
	case Hinge:
		file.read((char*)&desc2, 128);
		break;
	case LimitedHinge:
		file.read((char*)&desc3, 166);
		break;
	case Prismatic:
		file.read((char*)&desc4, 141);
		break;
	case Ragdoll:
		file.read((char*)&desc5, 178);
		break;
	case StiffSpring:
		file.read((char*)&desc6, 36);
		break;
	}
}

void SubConstraintDesc::Put(std::fstream& file) {
	file.write((char*)&type, 4);
	entityRefs.Put(file);
	file.write((char*)&priority, 4);

	switch (type) {
	case BallAndSocket:
		file.write((char*)&desc1, 32);
		break;
	case Hinge:
		file.write((char*)&desc2, 128);
		break;
	case LimitedHinge:
		file.write((char*)&desc3, 166);
		break;
	case Prismatic:
		file.write((char*)&desc4, 141);
		break;
	case Ragdoll:
		file.write((char*)&desc5, 178);
		break;
	case StiffSpring:
		file.write((char*)&desc6, 36);
		break;
	}
}

void SubConstraintDesc::GetChildRefs(std::set<int*>& refs) {
	entityRefs.GetIndexPtrs(refs);
}

int SubConstraintDesc::CalcDescSize() {
	int descSize = 12;
	descSize += entityRefs.GetSize() * 4;

	switch (type) {
	case BallAndSocket:
		descSize += 32;
		break;
	case Hinge:
		descSize += 128;
		break;
	case LimitedHinge:
		descSize += 166;
		break;
	case Prismatic:
		descSize += 141;
		break;
	case Ragdoll:
		descSize += 178;
		break;
	case StiffSpring:
		descSize += 36;
		break;
	}

	return descSize;
}


bhkBreakableConstraint::bhkBreakableConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkBreakableConstraint::bhkBreakableConstraint(std::fstream& file, NiHeader* hdr) : bhkBreakableConstraint(hdr) {
	Get(file);
}

void bhkBreakableConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	subConstraint.Get(file, this);
	file.read((char*)&threshold, 4);
	file.read((char*)&removeIfBroken, 1);
}

void bhkBreakableConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	subConstraint.Put(file);
	file.write((char*)&threshold, 4);
	file.write((char*)&removeIfBroken, 1);
}

void bhkBreakableConstraint::GetChildRefs(std::set<int*>& refs) {
	bhkConstraint::GetChildRefs(refs);

	subConstraint.GetChildRefs(refs);
}

int bhkBreakableConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 5;
	blockSize += subConstraint.CalcDescSize();

	return blockSize;
}


bhkRagdollConstraint::bhkRagdollConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkRagdollConstraint::bhkRagdollConstraint(std::fstream& file, NiHeader* hdr) : bhkRagdollConstraint(hdr) {
	Get(file);
}

void bhkRagdollConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&ragdoll.twistA, 16);
	file.read((char*)&ragdoll.planeA, 16);
	file.read((char*)&ragdoll.motorA, 16);
	file.read((char*)&ragdoll.pivotA, 16);
	file.read((char*)&ragdoll.twistB, 16);
	file.read((char*)&ragdoll.planeB, 16);
	file.read((char*)&ragdoll.motorB, 16);
	file.read((char*)&ragdoll.pivotB, 16);
	file.read((char*)&ragdoll.coneMaxAngle, 4);
	file.read((char*)&ragdoll.planeMinAngle, 4);
	file.read((char*)&ragdoll.planeMaxAngle, 4);
	file.read((char*)&ragdoll.twistMinAngle, 4);
	file.read((char*)&ragdoll.twistMaxAngle, 4);
	file.read((char*)&ragdoll.maxFriction, 4);
	file.read((char*)&ragdoll.enableMotor, 1);

	if (ragdoll.enableMotor)
		file.read((char*)&ragdoll.motor, 25);
}

void bhkRagdollConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&ragdoll.twistA, 16);
	file.write((char*)&ragdoll.planeA, 16);
	file.write((char*)&ragdoll.motorA, 16);
	file.write((char*)&ragdoll.pivotA, 16);
	file.write((char*)&ragdoll.twistB, 16);
	file.write((char*)&ragdoll.planeB, 16);
	file.write((char*)&ragdoll.motorB, 16);
	file.write((char*)&ragdoll.pivotB, 16);
	file.write((char*)&ragdoll.coneMaxAngle, 4);
	file.write((char*)&ragdoll.planeMinAngle, 4);
	file.write((char*)&ragdoll.planeMaxAngle, 4);
	file.write((char*)&ragdoll.twistMinAngle, 4);
	file.write((char*)&ragdoll.twistMaxAngle, 4);
	file.write((char*)&ragdoll.maxFriction, 4);
	file.write((char*)&ragdoll.enableMotor, 1);

	if (ragdoll.enableMotor)
		file.write((char*)&ragdoll.motor, 25);
}

int bhkRagdollConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 153;
	if (ragdoll.enableMotor)
		blockSize += 25;

	return blockSize;
}


bhkStiffSpringConstraint::bhkStiffSpringConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkStiffSpringConstraint::bhkStiffSpringConstraint(std::fstream& file, NiHeader* hdr) : bhkStiffSpringConstraint(hdr) {
	Get(file);
}

void bhkStiffSpringConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&stiffSpring.pivotA, 16);
	file.read((char*)&stiffSpring.pivotB, 16);
	file.read((char*)&stiffSpring.length, 4);
}

void bhkStiffSpringConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&stiffSpring.pivotA, 16);
	file.write((char*)&stiffSpring.pivotB, 16);
	file.write((char*)&stiffSpring.length, 4);
}

int bhkStiffSpringConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 36;

	return blockSize;
}


bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(NiHeader* hdr) {
	bhkConstraint::Init(hdr);
}

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(std::fstream& file, NiHeader* hdr) : bhkBallAndSocketConstraint(hdr) {
	Get(file);
}

void bhkBallAndSocketConstraint::Get(std::fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&ballAndSocket.translationA, 16);
	file.read((char*)&ballAndSocket.translationB, 16);
}

void bhkBallAndSocketConstraint::Put(std::fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&ballAndSocket.translationA, 16);
	file.write((char*)&ballAndSocket.translationB, 16);
}

int bhkBallAndSocketConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(NiHeader* hdr) {
	bhkSerializable::Init(hdr);
}

bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(std::fstream& file, NiHeader* hdr) : bhkBallSocketConstraintChain(hdr) {
	Get(file);
}

void bhkBallSocketConstraintChain::Get(std::fstream& file) {
	bhkSerializable::Get(file);

	file.read((char*)&numPivots, 4);
	pivots.resize(numPivots);
	for (int i = 0; i < numPivots; i++)
		file.read((char*)&pivots[i], 16);

	file.read((char*)&tau, 4);
	file.read((char*)&damping, 4);
	file.read((char*)&cfm, 4);
	file.read((char*)&maxErrorDistance, 4);

	entityARefs.Get(file);

	file.read((char*)&numEntities, 4);
	entityARef.Get(file);
	entityBRef.Get(file);
	file.read((char*)&priority, 4);
}

void bhkBallSocketConstraintChain::Put(std::fstream& file) {
	bhkSerializable::Put(file);

	file.write((char*)&numPivots, 4);
	for (int i = 0; i < numPivots; i++)
		file.write((char*)&pivots[i], 16);

	file.write((char*)&tau, 4);
	file.write((char*)&damping, 4);
	file.write((char*)&cfm, 4);
	file.write((char*)&maxErrorDistance, 4);

	entityARefs.Put(file);

	file.write((char*)&numEntities, 4);
	entityARef.Put(file);
	entityBRef.Put(file);
	file.write((char*)&priority, 4);
}

void bhkBallSocketConstraintChain::GetChildRefs(std::set<int*>& refs) {
	bhkSerializable::GetChildRefs(refs);

	entityARefs.GetIndexPtrs(refs);
	refs.insert(&entityARef.index);
	refs.insert(&entityBRef.index);
}

int bhkBallSocketConstraintChain::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 40;
	blockSize += numPivots * 16;
	blockSize += entityARefs.GetSize() * 4;

	return blockSize;
}


bhkRigidBody::bhkRigidBody(NiHeader* hdr) {
	bhkEntity::Init(hdr);
}

bhkRigidBody::bhkRigidBody(std::fstream& file, NiHeader* hdr) : bhkRigidBody(hdr) {
	Get(file);
}

void bhkRigidBody::Get(std::fstream& file) {
	bhkEntity::Get(file);

	file.read((char*)&responseType, 1);
	file.read((char*)&unkByte, 1);
	file.read((char*)&processContactCallbackDelay, 2);
	file.read((char*)unkShorts, 4);
	file.read((char*)&collisionFilterCopy, 4);
	file.read((char*)unkShorts2, 12);
	file.read((char*)&translation, 16);
	file.read((char*)&rotation, 16);
	file.read((char*)&linearVelocity, 16);
	file.read((char*)&angularVelocity, 16);
	file.read((char*)inertiaMatrix, 48);
	file.read((char*)&center, 16);
	file.read((char*)&mass, 4);
	file.read((char*)&linearDamping, 4);
	file.read((char*)&angularDamping, 4);

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&timeFactor, 4);
		file.read((char*)&gravityFactor, 4);
	}

	file.read((char*)&friction, 4);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&rollingFrictionMult, 4);

	file.read((char*)&restitution, 4);
	file.read((char*)&maxLinearVelocity, 4);
	file.read((char*)&maxAngularVelocity, 4);
	file.read((char*)&penetrationDepth, 4);
	file.read((char*)&motionSystem, 1);
	file.read((char*)&deactivatorType, 1);
	file.read((char*)&solverDeactivation, 1);
	file.read((char*)&qualityType, 1);
	file.read((char*)&autoRemoveLevel, 1);
	file.read((char*)&responseModifierFlag, 1);
	file.read((char*)&numShapeKeysInContactPointProps, 1);
	file.read((char*)&forceCollideOntoPpu, 1);
	file.read((char*)&unkInt2, 4);
	file.read((char*)&unkInt3, 4);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&unkInt4, 4);

	constraintRefs.Get(file);

	if (header->GetUserVersion() <= 11)
		file.read((char*)&unkInt5, 4);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&unkShort3, 2);
}

void bhkRigidBody::Put(std::fstream& file) {
	bhkEntity::Put(file);

	file.write((char*)&responseType, 1);
	file.write((char*)&unkByte, 1);
	file.write((char*)&processContactCallbackDelay, 2);
	file.write((char*)unkShorts, 4);
	file.write((char*)&collisionFilterCopy, 4);
	file.write((char*)unkShorts2, 12);
	file.write((char*)&translation, 16);
	file.write((char*)&rotation, 16);
	file.write((char*)&linearVelocity, 16);
	file.write((char*)&angularVelocity, 16);
	file.write((char*)inertiaMatrix, 48);
	file.write((char*)&center, 16);
	file.write((char*)&mass, 4);
	file.write((char*)&linearDamping, 4);
	file.write((char*)&angularDamping, 4);

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&timeFactor, 4);
		file.write((char*)&gravityFactor, 4);
	}

	file.write((char*)&friction, 4);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&rollingFrictionMult, 4);

	file.write((char*)&restitution, 4);
	file.write((char*)&maxLinearVelocity, 4);
	file.write((char*)&maxAngularVelocity, 4);
	file.write((char*)&penetrationDepth, 4);
	file.write((char*)&motionSystem, 1);
	file.write((char*)&deactivatorType, 1);
	file.write((char*)&solverDeactivation, 1);
	file.write((char*)&qualityType, 1);
	file.write((char*)&autoRemoveLevel, 1);
	file.write((char*)&responseModifierFlag, 1);
	file.write((char*)&numShapeKeysInContactPointProps, 1);
	file.write((char*)&forceCollideOntoPpu, 1);
	file.write((char*)&unkInt2, 4);
	file.write((char*)&unkInt3, 4);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&unkInt4, 4);

	constraintRefs.Put(file);

	if (header->GetUserVersion() <= 11)
		file.write((char*)&unkInt5, 4);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&unkShort3, 2);
}

void bhkRigidBody::GetChildRefs(std::set<int*>& refs) {
	bhkEntity::GetChildRefs(refs);

	constraintRefs.GetIndexPtrs(refs);
}

int bhkRigidBody::CalcBlockSize() {
	bhkEntity::CalcBlockSize();

	blockSize += 204;
	blockSize += 4 * constraintRefs.GetSize();

	if (header->GetUserVersion() <= 11)
		blockSize += 4;

	if (header->GetUserVersion() >= 12)
		blockSize += 18;

	return blockSize;
}


bhkRigidBodyT::bhkRigidBodyT(NiHeader* hdr) : bhkRigidBody(hdr) {
}

bhkRigidBodyT::bhkRigidBodyT(std::fstream& file, NiHeader* hdr) : bhkRigidBodyT(hdr) {
	Get(file);
}


bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(NiHeader* hdr) {
	bhkRefObject::Init(hdr);

	numMat32 = 0;
	numMat16 = 0;
	numMat8 = 0;
	numMaterials = 0;
	numNamedMat = 0;
	numTransforms = 0;
	numBigVerts = 0;
	numBigTris = 0;
	numChunks = 0;
	numConvexPieceA = 0;
}

bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(std::fstream& file, NiHeader* hdr) : bhkCompressedMeshShapeData(hdr) {
	Get(file);
}

void bhkCompressedMeshShapeData::Get(std::fstream& file) {
	bhkRefObject::Get(file);

	file.read((char*)&bitsPerIndex, 4);
	file.read((char*)&bitsPerWIndex, 4);
	file.read((char*)&maskWIndex, 4);
	file.read((char*)&maskIndex, 4);
	file.read((char*)&error, 4);
	file.read((char*)&aabbBoundMin, 16);
	file.read((char*)&aabbBoundMax, 16);
	file.read((char*)&weldingType, 1);
	file.read((char*)&materialType, 1);

	file.read((char*)&numMat32, 4);
	mat32.resize(numMat32);
	for (int i = 0; i < numMat32; i++)
		file.read((char*)&mat32[i], 4);

	file.read((char*)&numMat16, 4);
	mat16.resize(numMat16);
	for (int i = 0; i < numMat16; i++)
		file.read((char*)&mat16[i], 4);

	file.read((char*)&numMat8, 4);
	mat8.resize(numMat8);
	for (int i = 0; i < numMat8; i++)
		file.read((char*)&mat8[i], 4);

	file.read((char*)&numMaterials, 4);
	materials.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		file.read((char*)&materials[i], 8);

	file.read((char*)&numNamedMat, 4);

	file.read((char*)&numTransforms, 4);
	transforms.resize(numTransforms);
	for (int i = 0; i < numTransforms; i++)
		file.read((char*)&transforms[i], 32);

	file.read((char*)&numBigVerts, 4);
	bigVerts.resize(numBigVerts);
	for (int i = 0; i < numBigVerts; i++)
		file.read((char*)&bigVerts[i], 16);

	file.read((char*)&numBigTris, 4);
	bigTris.resize(numBigTris);
	for (int i = 0; i < numBigTris; i++)
		file.read((char*)&bigTris[i], 12);

	file.read((char*)&numChunks, 4);
	chunks.resize(numChunks);
	for (int i = 0; i < numChunks; i++) {
		file.read((char*)&chunks[i].translation, 16);
		file.read((char*)&chunks[i].matIndex, 4);
		file.read((char*)&chunks[i].reference, 2);
		file.read((char*)&chunks[i].transformIndex, 2);

		file.read((char*)&chunks[i].numVerts, 4);
		chunks[i].verts.resize(chunks[i].numVerts);
		for (int j = 0; j < chunks[i].numVerts; j++)
			file.read((char*)&chunks[i].verts[j], 2);

		file.read((char*)&chunks[i].numIndices, 4);
		chunks[i].indices.resize(chunks[i].numIndices);
		for (int j = 0; j < chunks[i].numIndices; j++)
			file.read((char*)&chunks[i].indices[j], 2);

		file.read((char*)&chunks[i].numStrips, 4);
		chunks[i].strips.resize(chunks[i].numStrips);
		for (int j = 0; j < chunks[i].numStrips; j++)
			file.read((char*)&chunks[i].strips[j], 2);

		file.read((char*)&chunks[i].numWeldingInfo, 4);
		chunks[i].weldingInfo.resize(chunks[i].numWeldingInfo);
		for (int j = 0; j < chunks[i].numWeldingInfo; j++)
			file.read((char*)&chunks[i].weldingInfo[j], 2);
	}

	file.read((char*)&numConvexPieceA, 4);
}

void bhkCompressedMeshShapeData::Put(std::fstream& file) {
	bhkRefObject::Put(file);

	file.write((char*)&bitsPerIndex, 4);
	file.write((char*)&bitsPerWIndex, 4);
	file.write((char*)&maskWIndex, 4);
	file.write((char*)&maskIndex, 4);
	file.write((char*)&error, 4);
	file.write((char*)&aabbBoundMin, 16);
	file.write((char*)&aabbBoundMax, 16);
	file.write((char*)&weldingType, 1);
	file.write((char*)&materialType, 1);

	file.write((char*)&numMat32, 4);
	for (int i = 0; i < numMat32; i++)
		file.write((char*)&mat32[i], 4);

	file.write((char*)&numMat16, 4);
	for (int i = 0; i < numMat16; i++)
		file.write((char*)&mat16[i], 4);

	file.write((char*)&numMat8, 4);
	for (int i = 0; i < numMat8; i++)
		file.write((char*)&mat8[i], 4);

	file.write((char*)&numMaterials, 4);
	for (int i = 0; i < numMaterials; i++)
		file.write((char*)&materials[i], 8);

	file.write((char*)&numNamedMat, 4);

	file.write((char*)&numTransforms, 4);
	for (int i = 0; i < numTransforms; i++)
		file.write((char*)&transforms[i], 32);

	file.write((char*)&numBigVerts, 4);
	for (int i = 0; i < numBigVerts; i++)
		file.write((char*)&bigVerts[i], 16);

	file.write((char*)&numBigTris, 4);
	for (int i = 0; i < numBigTris; i++)
		file.write((char*)&bigTris[i], 12);

	file.write((char*)&numChunks, 4);
	for (int i = 0; i < numChunks; i++) {
		file.write((char*)&chunks[i].translation, 16);
		file.write((char*)&chunks[i].matIndex, 4);
		file.write((char*)&chunks[i].reference, 2);
		file.write((char*)&chunks[i].transformIndex, 2);

		file.write((char*)&chunks[i].numVerts, 4);
		for (int j = 0; j < chunks[i].numVerts; j++)
			file.write((char*)&chunks[i].verts[j], 2);

		file.write((char*)&chunks[i].numIndices, 4);
		for (int j = 0; j < chunks[i].numIndices; j++)
			file.write((char*)&chunks[i].indices[j], 2);

		file.write((char*)&chunks[i].numStrips, 4);
		for (int j = 0; j < chunks[i].numStrips; j++)
			file.write((char*)&chunks[i].strips[j], 2);

		file.write((char*)&chunks[i].numWeldingInfo, 4);
		for (int j = 0; j < chunks[i].numWeldingInfo; j++)
			file.write((char*)&chunks[i].weldingInfo[j], 2);
	}

	file.write((char*)&numConvexPieceA, 4);
}

int bhkCompressedMeshShapeData::CalcBlockSize() {
	bhkRefObject::CalcBlockSize();

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


bhkCompressedMeshShape::bhkCompressedMeshShape(NiHeader* hdr) {
	bhkShape::Init(hdr);
}

bhkCompressedMeshShape::bhkCompressedMeshShape(std::fstream& file, NiHeader* hdr) : bhkCompressedMeshShape(hdr) {
	Get(file);
}

void bhkCompressedMeshShape::Get(std::fstream& file) {
	bhkShape::Get(file);

	targetRef.Get(file);
	file.read((char*)&userData, 4);
	file.read((char*)&radius, 4);
	file.read((char*)&unkFloat, 4);
	file.read((char*)&scaling, 16);
	file.read((char*)&radius2, 4);
	file.read((char*)&scaling2, 16);
	dataRef.Get(file);
}

void bhkCompressedMeshShape::Put(std::fstream& file) {
	bhkShape::Put(file);

	targetRef.Put(file);
	file.write((char*)&userData, 4);
	file.write((char*)&radius, 4);
	file.write((char*)&unkFloat, 4);
	file.write((char*)&scaling, 16);
	file.write((char*)&radius2, 4);
	file.write((char*)&scaling2, 16);
	dataRef.Put(file);
}

void bhkCompressedMeshShape::GetChildRefs(std::set<int*>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int bhkCompressedMeshShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 56;

	return blockSize;
}
