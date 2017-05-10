/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "ExtraData.h"

void NiExtraData::Get(std::fstream& file) {
	NiObject::Get(file);

	name.Get(file, header);
}

void NiExtraData::Put(std::fstream& file) {
	NiObject::Put(file);

	name.Put(file);
}

void NiExtraData::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	name.notifyStringDelete(stringID);
}

std::string NiExtraData::GetName() {
	return name.GetString(header);
}

void NiExtraData::SetName(const std::string& extraDataName) {
	name.SetString(header, extraDataName);
}

int NiExtraData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiBinaryExtraData::NiBinaryExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	size = 0;
}

NiBinaryExtraData::NiBinaryExtraData(std::fstream& file, NiHeader* hdr) : NiBinaryExtraData(hdr) {
	Get(file);
}

void NiBinaryExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&size, 4);
	data.resize(size);
	for (int i = 0; i < size; i++)
		file.read((char*)&data[i], 1);
}

void NiBinaryExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&size, 4);
	for (int i = 0; i < size; i++)
		file.write((char*)&data[i], 1);
}

int NiBinaryExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += size;

	return blockSize;
}


NiFloatExtraData::NiFloatExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	floatData = 0.0f;
}

NiFloatExtraData::NiFloatExtraData(std::fstream& file, NiHeader* hdr) : NiFloatExtraData(hdr) {
	Get(file);
}

void NiFloatExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&floatData, 4);
}

void NiFloatExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&floatData, 4);
}

int NiFloatExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiStringsExtraData::NiStringsExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	numStrings = 0;
}

NiStringsExtraData::NiStringsExtraData(std::fstream& file, NiHeader* hdr) : NiStringsExtraData(hdr){
	Get(file);
}

void NiStringsExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numStrings, 4);
	stringsData.resize(numStrings);
	for (int i = 0; i < numStrings; i++)
		stringsData[i].Get(file, 4);
}

void NiStringsExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numStrings, 4);
	for (int i = 0; i < numStrings; i++)
		stringsData[i].Put(file, 4, false);
}

int NiStringsExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numStrings * 4;

	for (int i = 0; i < numStrings; i++)
		blockSize += stringsData[i].GetLength();

	return blockSize;
}


NiStringExtraData::NiStringExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);
}

NiStringExtraData::NiStringExtraData(std::fstream& file, NiHeader* hdr) : NiStringExtraData(hdr) {
	Get(file);
}

void NiStringExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	stringData.Get(file, header);
}

void NiStringExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	stringData.Put(file);
}

void NiStringExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	stringData.notifyStringDelete(stringID);
}

std::string NiStringExtraData::GetStringData() {
	return stringData.GetString(header);
}

void NiStringExtraData::SetStringData(const std::string& str) {
	stringData.SetString(header, str);
}

int NiStringExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiBooleanExtraData::NiBooleanExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	booleanData = false;
}

NiBooleanExtraData::NiBooleanExtraData(std::fstream& file, NiHeader* hdr) : NiBooleanExtraData(hdr) {
	Get(file);
}

void NiBooleanExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&booleanData, 1);
}

void NiBooleanExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&booleanData, 1);
}

bool NiBooleanExtraData::GetBooleanData() {
	return booleanData;
}

void NiBooleanExtraData::SetBooleanData(const bool boolData) {
	this->booleanData = boolData;
}

int NiBooleanExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 1;

	return blockSize;
}


NiIntegerExtraData::NiIntegerExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	integerData = 0;
}

NiIntegerExtraData::NiIntegerExtraData(std::fstream& file, NiHeader* hdr) : NiIntegerExtraData(hdr) {
	Get(file);
}

void NiIntegerExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&integerData, 4);
}

void NiIntegerExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&integerData, 4);
}

uint NiIntegerExtraData::GetIntegerData() {
	return integerData;
}

void NiIntegerExtraData::SetIntegerData(const uint intData) {
	this->integerData = intData;
}

int NiIntegerExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSXFlags::BSXFlags(NiHeader* hdr) : NiIntegerExtraData(hdr) {
}

BSXFlags::BSXFlags(std::fstream& file, NiHeader* hdr) : BSXFlags(hdr) {
	Get(file);
}


BSInvMarker::BSInvMarker(NiHeader* hdr) {
	NiExtraData::Init(hdr);
}

BSInvMarker::BSInvMarker(std::fstream& file, NiHeader* hdr) : BSInvMarker(hdr) {
	Get(file);
}

void BSInvMarker::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&rotationX, 2);
	file.read((char*)&rotationY, 2);
	file.read((char*)&rotationZ, 2);
	file.read((char*)&zoom, 4);
}

void BSInvMarker::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&rotationX, 2);
	file.write((char*)&rotationY, 2);
	file.write((char*)&rotationZ, 2);
	file.write((char*)&zoom, 4);
}

int BSInvMarker::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 10;

	return blockSize;
}


BSFurnitureMarker::BSFurnitureMarker(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	numPositions = 0;
}

BSFurnitureMarker::BSFurnitureMarker(std::fstream& file, NiHeader* hdr) : BSFurnitureMarker(hdr) {
	Get(file);
}

void BSFurnitureMarker::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numPositions, 4);
	positions.resize(numPositions);

	for (int i = 0; i < numPositions; i++) {
		file.read((char*)&positions[i].offset, 12);

		if (header->GetUserVersion() <= 11) {
			file.read((char*)&positions[i].orientation, 2);
			file.read((char*)&positions[i].posRef1, 1);
			file.read((char*)&positions[i].posRef2, 1);
		}

		if (header->GetUserVersion() >= 12) {
			file.read((char*)&positions[i].heading, 4);
			file.read((char*)&positions[i].animationType, 2);
			file.read((char*)&positions[i].entryPoints, 2);
		}
	}
}

void BSFurnitureMarker::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numPositions, 4);

	for (int i = 0; i < numPositions; i++) {
		file.write((char*)&positions[i].offset, 12);

		if (header->GetUserVersion() <= 11) {
			file.write((char*)&positions[i].orientation, 2);
			file.write((char*)&positions[i].posRef1, 1);
			file.write((char*)&positions[i].posRef2, 1);
		}

		if (header->GetUserVersion() >= 12) {
			file.write((char*)&positions[i].heading, 4);
			file.write((char*)&positions[i].animationType, 2);
			file.write((char*)&positions[i].entryPoints, 2);
		}
	}
}

int BSFurnitureMarker::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += 12 * numPositions;

	if (header->GetUserVersion() <= 11)
		blockSize += 4 * numPositions;

	if (header->GetUserVersion() >= 12)
		blockSize += 8 * numPositions;

	return blockSize;
}


BSFurnitureMarkerNode::BSFurnitureMarkerNode(NiHeader* hdr) : BSFurnitureMarker(hdr) {
}

BSFurnitureMarkerNode::BSFurnitureMarkerNode(std::fstream& file, NiHeader* hdr) : BSFurnitureMarkerNode(hdr) {
	Get(file);
}


BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(NiHeader* hdr) : NiFloatExtraData(hdr) {
	numVectorBlocks = 0;
}

BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(std::fstream& file, NiHeader* hdr) : BSDecalPlacementVectorExtraData(hdr) {
	Get(file);
}

void BSDecalPlacementVectorExtraData::Get(std::fstream& file) {
	NiFloatExtraData::Get(file);

	file.read((char*)&numVectorBlocks, 2);
	decalVectorBlocks.resize(numVectorBlocks);

	for (int i = 0; i < numVectorBlocks; i++) {
		file.read((char*)&decalVectorBlocks[i].numVectors, 2);

		decalVectorBlocks[i].points.resize(decalVectorBlocks[i].numVectors);
		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			file.read((char*)&decalVectorBlocks[i].points[j], 12);

		decalVectorBlocks[i].normals.resize(decalVectorBlocks[i].numVectors);
		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			file.read((char*)&decalVectorBlocks[i].normals[j], 12);
	}
}

void BSDecalPlacementVectorExtraData::Put(std::fstream& file) {
	NiFloatExtraData::Put(file);

	file.write((char*)&numVectorBlocks, 2);

	for (int i = 0; i < numVectorBlocks; i++) {
		file.write((char*)&decalVectorBlocks[i].numVectors, 2);

		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			file.write((char*)&decalVectorBlocks[i].points[j], 12);

		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			file.write((char*)&decalVectorBlocks[i].normals[j], 12);
	}
}

int BSDecalPlacementVectorExtraData::CalcBlockSize() {
	NiFloatExtraData::CalcBlockSize();

	blockSize += 2;
	for (int i = 0; i < numVectorBlocks; i++)
		blockSize += 2 + decalVectorBlocks[i].numVectors * 24;

	return blockSize;
}


BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);
}

BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(std::fstream& file, NiHeader* hdr) : BSBehaviorGraphExtraData(hdr) {
	Get(file);
}

void BSBehaviorGraphExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	behaviorGraphFile.Get(file, header);
	file.read((char*)&controlsBaseSkel, 1);
}

void BSBehaviorGraphExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	behaviorGraphFile.Put(file);
	file.write((char*)&controlsBaseSkel, 1);
}

void BSBehaviorGraphExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	behaviorGraphFile.notifyStringDelete(stringID);
}

int BSBehaviorGraphExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSBound::BSBound(NiHeader* hdr) {
	NiExtraData::Init(hdr);
}

BSBound::BSBound(std::fstream& file, NiHeader* hdr) : BSBound(hdr) {
	Get(file);
}

void BSBound::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&halfExtents, 12);
}

void BSBound::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&halfExtents, 12);
}

int BSBound::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


BSBoneLODExtraData::BSBoneLODExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	numBoneLODs = 0;
}

BSBoneLODExtraData::BSBoneLODExtraData(std::fstream& file, NiHeader* hdr) : BSBoneLODExtraData(hdr) {
	Get(file);
}

void BSBoneLODExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numBoneLODs, 4);
	boneLODs.resize(numBoneLODs);
	for (int i = 0; i < numBoneLODs; i++) {
		file.read((char*)&boneLODs[i].distance, 4);
		boneLODs[i].boneName.Get(file, header);
	}
}

void BSBoneLODExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numBoneLODs, 4);
	for (int i = 0; i < numBoneLODs; i++) {
		file.write((char*)&boneLODs[i].distance, 4);
		boneLODs[i].boneName.Put(file);
	}
}

void BSBoneLODExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	for (int i = 0; i < numBoneLODs; i++)
		boneLODs[i].boneName.notifyStringDelete(stringID);
}

int BSBoneLODExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBoneLODs * 8;

	return blockSize;
}


NiTextKeyExtraData::NiTextKeyExtraData(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	numTextKeys = 0;
}

NiTextKeyExtraData::NiTextKeyExtraData(std::fstream& file, NiHeader* hdr) : NiTextKeyExtraData(hdr) {
	Get(file);
}

void NiTextKeyExtraData::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numTextKeys, 4);
	textKeys.resize(numTextKeys);
	for (int i = 0; i < numTextKeys; i++) {
		file.read((char*)&textKeys[i].time, 4);
		textKeys[i].value.Get(file, header);
	}
}

void NiTextKeyExtraData::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numTextKeys, 4);
	for (int i = 0; i < numTextKeys; i++) {
		file.write((char*)&textKeys[i].time, 4);
		textKeys[i].value.Put(file);
	}
}

void NiTextKeyExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	for (int i = 0; i < numTextKeys; i++)
		textKeys[i].value.notifyStringDelete(stringID);
}

int NiTextKeyExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numTextKeys * 8;

	return blockSize;
}


BSConnectPoint::BSConnectPoint() {
	scale = 1.0f;
}

BSConnectPoint::BSConnectPoint(std::fstream& file) {
	Get(file);
}

void BSConnectPoint::Get(std::fstream& file) {
	root.Get(file, 4);
	variableName.Get(file, 4);

	file.read((char*)&rotation, 16);
	file.read((char*)&translation, 12);
	file.read((char*)&scale, 4);
}

void BSConnectPoint::Put(std::fstream& file) {
	root.Put(file, 4, false);
	variableName.Put(file, 4, false);

	file.write((char*)&rotation, 16);
	file.write((char*)&translation, 12);
	file.write((char*)&scale, 4);
}

int BSConnectPoint::CalcBlockSize() {
	int blockSize = 40;
	blockSize += root.GetLength();
	blockSize += variableName.GetLength();
	return blockSize;
}


BSConnectPointParents::BSConnectPointParents(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	numConnectPoints = 0;
}

BSConnectPointParents::BSConnectPointParents(std::fstream& file, NiHeader* hdr) : BSConnectPointParents(hdr) {
	Get(file);
}

void BSConnectPointParents::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numConnectPoints, 4);

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints.push_back(BSConnectPoint(file));
}

void BSConnectPointParents::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numConnectPoints, 4);

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints[i].Put(file);
}

int BSConnectPointParents::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	for (int i = 0; i < numConnectPoints; i++)
		blockSize += connectPoints[i].CalcBlockSize();

	return blockSize;
}


BSConnectPointChildren::BSConnectPointChildren(NiHeader* hdr) {
	NiExtraData::Init(hdr);

	unkByte = 1;
	numTargets = 0;
}

BSConnectPointChildren::BSConnectPointChildren(std::fstream& file, NiHeader* hdr) : BSConnectPointChildren(hdr) {
	Get(file);
}

void BSConnectPointChildren::Get(std::fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&unkByte, 1);
	file.read((char*)&numTargets, 4);

	targets.resize(numTargets);
	for (int i = 0; i < numTargets; i++)
		targets[i].Get(file, 4);
}

void BSConnectPointChildren::Put(std::fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&unkByte, 1);
	file.write((char*)&numTargets, 4);

	for (int i = 0; i < numTargets; i++)
		targets[i].Put(file, 4, false);
}

int BSConnectPointChildren::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 5;
	blockSize += numTargets * 4;

	for (int i = 0; i < numTargets; i++)
		blockSize += targets[i].GetLength();

	return blockSize;
}


BSClothExtraData::BSClothExtraData(NiHeader* hdr, const uint size) {
	BSExtraData::Init(hdr);

	numBytes = size;
	data.resize(size);
}

BSClothExtraData::BSClothExtraData(std::fstream& file, NiHeader* hdr) : BSClothExtraData(hdr) {
	Get(file);
}

void BSClothExtraData::Get(std::fstream& file) {
	BSExtraData::Get(file);

	file.read((char*)&numBytes, 4);
	data.resize(numBytes);
	if (data.empty())
		return;

	file.read(&data[0], numBytes);
}

void BSClothExtraData::Put(std::fstream& file) {
	BSExtraData::Put(file);

	file.write((char*)&numBytes, 4);
	if (data.empty())
		return;

	file.write(&data[0], numBytes);
}

int BSClothExtraData::CalcBlockSize() {
	BSExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


bool BSClothExtraData::ToHKX(const std::string& fileName) {
	std::ofstream file(fileName, std::ios_base::binary);
	if (!file)
		return false;

	file.write(data.data(), numBytes);
	return true;
}

bool BSClothExtraData::FromHKX(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if (!file)
		return false;

	numBytes = file.tellg();
	file.seekg(0, std::ios::beg);

	data.resize(numBytes);
	file.read(data.data(), numBytes);
	return true;
}
