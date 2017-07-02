/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "ExtraData.h"
#include <fstream>

void NiExtraData::Get(NiStream& stream) {
	NiObject::Get(stream);

	name.Get(stream);
}

void NiExtraData::Put(NiStream& stream) {
	NiObject::Put(stream);

	name.Put(stream);
}

void NiExtraData::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name);
}

std::string NiExtraData::GetName() {
	return name.GetString();
}

void NiExtraData::SetName(const std::string& extraDataName) {
	name.SetString(extraDataName);
}

int NiExtraData::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiBinaryExtraData::NiBinaryExtraData(NiStream& stream) : NiBinaryExtraData() {
	Get(stream);
}

void NiBinaryExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> size;
	data.resize(size);
	for (int i = 0; i < size; i++)
		stream >> data[i];
}

void NiBinaryExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << size;
	for (int i = 0; i < size; i++)
		stream << data[i];
}

int NiBinaryExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += size;

	return blockSize;
}


NiFloatExtraData::NiFloatExtraData(NiStream& stream) : NiFloatExtraData() {
	Get(stream);
}

void NiFloatExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> floatData;
}

void NiFloatExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << floatData;
}

int NiFloatExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiStringsExtraData::NiStringsExtraData(NiStream& stream) : NiStringsExtraData() {
	Get(stream);
}

void NiStringsExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numStrings;
	stringsData.resize(numStrings);
	for (int i = 0; i < numStrings; i++)
		stringsData[i].Get(stream, 4);
}

void NiStringsExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numStrings;
	for (int i = 0; i < numStrings; i++)
		stringsData[i].Put(stream, 4, false);
}

int NiStringsExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += numStrings * 4;

	for (int i = 0; i < numStrings; i++)
		blockSize += stringsData[i].GetLength();

	return blockSize;
}


NiStringExtraData::NiStringExtraData(NiStream& stream) : NiStringExtraData() {
	Get(stream);
}

void NiStringExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stringData.Get(stream);
}

void NiStringExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stringData.Put(stream);
}

void NiStringExtraData::GetStringRefs(std::set<StringRef*>& refs) {
	NiExtraData::GetStringRefs(refs);

	refs.insert(&stringData);
}

std::string NiStringExtraData::GetStringData() {
	return stringData.GetString();
}

void NiStringExtraData::SetStringData(const std::string& str) {
	stringData.SetString(str);
}

int NiStringExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


NiBooleanExtraData::NiBooleanExtraData(NiStream& stream) : NiBooleanExtraData() {
	Get(stream);
}

void NiBooleanExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> booleanData;
}

void NiBooleanExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << booleanData;
}

bool NiBooleanExtraData::GetBooleanData() {
	return booleanData;
}

void NiBooleanExtraData::SetBooleanData(const bool boolData) {
	this->booleanData = boolData;
}

int NiBooleanExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 1;

	return blockSize;
}


NiIntegerExtraData::NiIntegerExtraData(NiStream& stream) : NiIntegerExtraData() {
	Get(stream);
}

void NiIntegerExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> integerData;
}

void NiIntegerExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << integerData;
}

uint NiIntegerExtraData::GetIntegerData() {
	return integerData;
}

void NiIntegerExtraData::SetIntegerData(const uint intData) {
	this->integerData = intData;
}

int NiIntegerExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSXFlags::BSXFlags() : NiIntegerExtraData() {
}

BSXFlags::BSXFlags(NiStream& stream) : BSXFlags() {
	Get(stream);
}


BSInvMarker::BSInvMarker(NiStream& stream) : BSInvMarker() {
	Get(stream);
}

void BSInvMarker::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> rotationX;
	stream >> rotationY;
	stream >> rotationZ;
	stream >> zoom;
}

void BSInvMarker::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << rotationX;
	stream << rotationY;
	stream << rotationZ;
	stream << zoom;
}

int BSInvMarker::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 10;

	return blockSize;
}


BSFurnitureMarker::BSFurnitureMarker(NiStream& stream) : BSFurnitureMarker() {
	Get(stream);
}

void BSFurnitureMarker::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numPositions;
	positions.resize(numPositions);

	for (int i = 0; i < numPositions; i++) {
		stream >> positions[i].offset;

		if (stream.GetVersion().User() <= 11) {
			stream >> positions[i].orientation;
			stream >> positions[i].posRef1;
			stream >> positions[i].posRef2;
		}

		if (stream.GetVersion().User() >= 12) {
			stream >> positions[i].heading;
			stream >> positions[i].animationType;
			stream >> positions[i].entryPoints;
		}
	}
}

void BSFurnitureMarker::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numPositions;

	for (int i = 0; i < numPositions; i++) {
		stream << positions[i].offset;

		if (stream.GetVersion().User() <= 11) {
			stream << positions[i].orientation;
			stream << positions[i].posRef1;
			stream << positions[i].posRef2;
		}

		if (stream.GetVersion().User() >= 12) {
			stream << positions[i].heading;
			stream << positions[i].animationType;
			stream << positions[i].entryPoints;
		}
	}
}

int BSFurnitureMarker::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += 12 * numPositions;

	if (version.User() <= 11)
		blockSize += 4 * numPositions;

	if (version.User() >= 12)
		blockSize += 8 * numPositions;

	return blockSize;
}


BSFurnitureMarkerNode::BSFurnitureMarkerNode() : BSFurnitureMarker() {
}

BSFurnitureMarkerNode::BSFurnitureMarkerNode(NiStream& stream) : BSFurnitureMarkerNode() {
	Get(stream);
}


BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData() : NiFloatExtraData() {
}

BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(NiStream& stream) : BSDecalPlacementVectorExtraData() {
	Get(stream);
}

void BSDecalPlacementVectorExtraData::Get(NiStream& stream) {
	NiFloatExtraData::Get(stream);

	stream >> numVectorBlocks;
	decalVectorBlocks.resize(numVectorBlocks);

	for (int i = 0; i < numVectorBlocks; i++) {
		stream >> decalVectorBlocks[i].numVectors;

		decalVectorBlocks[i].points.resize(decalVectorBlocks[i].numVectors);
		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			stream >> decalVectorBlocks[i].points[j];

		decalVectorBlocks[i].normals.resize(decalVectorBlocks[i].numVectors);
		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			stream >> decalVectorBlocks[i].normals[j];
	}
}

void BSDecalPlacementVectorExtraData::Put(NiStream& stream) {
	NiFloatExtraData::Put(stream);

	stream << numVectorBlocks;

	for (int i = 0; i < numVectorBlocks; i++) {
		stream << decalVectorBlocks[i].numVectors;

		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			stream << decalVectorBlocks[i].points[j];

		for (int j = 0; j < decalVectorBlocks[i].numVectors; j++)
			stream << decalVectorBlocks[i].normals[j];
	}
}

int BSDecalPlacementVectorExtraData::CalcBlockSize(NiVersion& version) {
	NiFloatExtraData::CalcBlockSize(version);

	blockSize += 2;
	for (int i = 0; i < numVectorBlocks; i++)
		blockSize += 2 + decalVectorBlocks[i].numVectors * 24;

	return blockSize;
}


BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(NiStream& stream) : BSBehaviorGraphExtraData() {
	Get(stream);
}

void BSBehaviorGraphExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	behaviorGraphFile.Get(stream);
	stream >> controlsBaseSkel;
}

void BSBehaviorGraphExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	behaviorGraphFile.Put(stream);
	stream << controlsBaseSkel;
}

void BSBehaviorGraphExtraData::GetStringRefs(std::set<StringRef*>& refs) {
	NiExtraData::GetStringRefs(refs);

	refs.insert(&behaviorGraphFile);
}

int BSBehaviorGraphExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 5;

	return blockSize;
}


BSBound::BSBound(NiStream& stream) : BSBound() {
	Get(stream);
}

void BSBound::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> center;
	stream >> halfExtents;
}

void BSBound::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << center;
	stream << halfExtents;
}

int BSBound::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 24;

	return blockSize;
}


BSBoneLODExtraData::BSBoneLODExtraData(NiStream& stream) : BSBoneLODExtraData() {
	Get(stream);
}

void BSBoneLODExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numBoneLODs;
	boneLODs.resize(numBoneLODs);
	for (int i = 0; i < numBoneLODs; i++) {
		stream >> boneLODs[i].distance;
		boneLODs[i].boneName.Get(stream);
	}
}

void BSBoneLODExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numBoneLODs;
	for (int i = 0; i < numBoneLODs; i++) {
		stream << boneLODs[i].distance;
		boneLODs[i].boneName.Put(stream);
	}
}

void BSBoneLODExtraData::GetStringRefs(std::set<StringRef*>& refs) {
	NiExtraData::GetStringRefs(refs);

	for (int i = 0; i < numBoneLODs; i++)
		refs.insert(&boneLODs[i].boneName);
}

int BSBoneLODExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += numBoneLODs * 8;

	return blockSize;
}


NiTextKeyExtraData::NiTextKeyExtraData(NiStream& stream) : NiTextKeyExtraData() {
	Get(stream);
}

void NiTextKeyExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numTextKeys;
	textKeys.resize(numTextKeys);
	for (int i = 0; i < numTextKeys; i++) {
		stream >> textKeys[i].time;
		textKeys[i].value.Get(stream);
	}
}

void NiTextKeyExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numTextKeys;
	for (int i = 0; i < numTextKeys; i++) {
		stream << textKeys[i].time;
		textKeys[i].value.Put(stream);
	}
}

void NiTextKeyExtraData::GetStringRefs(std::set<StringRef*>& refs) {
	NiExtraData::GetStringRefs(refs);

	for (int i = 0; i < numTextKeys; i++)
		refs.insert(&textKeys[i].value);
}

int NiTextKeyExtraData::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;
	blockSize += numTextKeys * 8;

	return blockSize;
}


BSConnectPoint::BSConnectPoint() {
}

BSConnectPoint::BSConnectPoint(NiStream& stream) {
	Get(stream);
}

void BSConnectPoint::Get(NiStream& stream) {
	root.Get(stream, 4);
	variableName.Get(stream, 4);

	stream >> rotation;
	stream >> translation;
	stream >> scale;
}

void BSConnectPoint::Put(NiStream& stream) {
	root.Put(stream, 4, false);
	variableName.Put(stream, 4, false);

	stream << rotation;
	stream << translation;
	stream << scale;
}

int BSConnectPoint::CalcBlockSize(NiVersion& version) {
	int blockSize = 40;
	blockSize += root.GetLength();
	blockSize += variableName.GetLength();
	return blockSize;
}


BSConnectPointParents::BSConnectPointParents(NiStream& stream) : BSConnectPointParents() {
	Get(stream);
}

void BSConnectPointParents::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numConnectPoints;

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints.push_back(BSConnectPoint(stream));
}

void BSConnectPointParents::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numConnectPoints;

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints[i].Put(stream);
}

int BSConnectPointParents::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 4;

	for (int i = 0; i < numConnectPoints; i++)
		blockSize += connectPoints[i].CalcBlockSize(version);

	return blockSize;
}


BSConnectPointChildren::BSConnectPointChildren(NiStream& stream) : BSConnectPointChildren() {
	Get(stream);
}

void BSConnectPointChildren::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> unkByte;
	stream >> numTargets;

	targets.resize(numTargets);
	for (int i = 0; i < numTargets; i++)
		targets[i].Get(stream, 4);
}

void BSConnectPointChildren::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << unkByte;
	stream << numTargets;

	for (int i = 0; i < numTargets; i++)
		targets[i].Put(stream, 4, false);
}

int BSConnectPointChildren::CalcBlockSize(NiVersion& version) {
	NiExtraData::CalcBlockSize(version);

	blockSize += 5;
	blockSize += numTargets * 4;

	for (int i = 0; i < numTargets; i++)
		blockSize += targets[i].GetLength();

	return blockSize;
}


BSClothExtraData::BSClothExtraData(const uint size) {
	numBytes = size;
	data.resize(size);
}

BSClothExtraData::BSClothExtraData(NiStream& stream) : BSClothExtraData() {
	Get(stream);
}

void BSClothExtraData::Get(NiStream& stream) {
	BSExtraData::Get(stream);

	stream >> numBytes;
	data.resize(numBytes);
	if (data.empty())
		return;

	stream.read(&data[0], numBytes);
}

void BSClothExtraData::Put(NiStream& stream) {
	BSExtraData::Put(stream);

	stream << numBytes;
	if (data.empty())
		return;

	stream.write(&data[0], numBytes);
}

int BSClothExtraData::CalcBlockSize(NiVersion& version) {
	BSExtraData::CalcBlockSize(version);

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
