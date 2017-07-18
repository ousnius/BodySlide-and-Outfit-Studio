/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "ExtraData.h"
#include "utils/half.hpp"

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

float NiFloatExtraData::GetFloatData() {
	return floatData;
}

void NiFloatExtraData::SetFloatData(const float fltData) {
	floatData = fltData;
}


NiFloatsExtraData::NiFloatsExtraData(NiStream& stream) : NiFloatsExtraData() {
	Get(stream);
}

void NiFloatsExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numFloats;
	floatsData.resize(numFloats);
	for (int i = 0; i < numFloats; i++)
		stream >> floatsData[i];
}

void NiFloatsExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numFloats;
	for (int i = 0; i < numFloats; i++)
		stream << floatsData[i];
}

std::vector<float> NiFloatsExtraData::GetFloatsData() {
	return floatsData;
}

void NiFloatsExtraData::SetFloatsData(const std::vector<float>& fltsData) {
	numFloats = fltsData.size();
	floatsData = fltsData;
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
	booleanData = boolData;
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
	integerData = intData;
}


NiIntegersExtraData::NiIntegersExtraData(NiStream& stream) : NiIntegersExtraData() {
	Get(stream);
}

void NiIntegersExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numIntegers;
	integersData.resize(numIntegers);
	for (int i = 0; i < numIntegers; i++)
		stream >> integersData[i];
}

void NiIntegersExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numIntegers;
	for (int i = 0; i < numIntegers; i++)
		stream << integersData[i];
}

std::vector<uint> NiIntegersExtraData::GetIntegersData() {
	return integersData;
}

void NiIntegersExtraData::SetIntegersData(const std::vector<uint>& intData) {
	numIntegers = intData.size();
	integersData = intData;
}


NiVectorExtraData::NiVectorExtraData(NiStream& stream) : NiVectorExtraData() {
	Get(stream);
}

void NiVectorExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> vectorData;
}

void NiVectorExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << vectorData;
}

Vector4 NiVectorExtraData::GetVectorData() {
	return vectorData;
}

void NiVectorExtraData::SetVectorData(const Vector4& vecData) {
	vectorData = vecData;
}


NiColorExtraData::NiColorExtraData(NiStream& stream) : NiColorExtraData() {
	Get(stream);
}

void NiColorExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> colorData;
}

void NiColorExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << colorData;
}

Color4 NiColorExtraData::GetColorData() {
	return colorData;
}

void NiColorExtraData::SetColorData(const Color4& colData) {
	colorData = colData;
}


BSXFlags::BSXFlags() : NiIntegerExtraData() {
}

BSXFlags::BSXFlags(NiStream& stream) : BSXFlags() {
	Get(stream);
}


BSWArray::BSWArray(NiStream& stream) : BSWArray() {
	Get(stream);
}

void BSWArray::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numData;
	data.resize(numData);
	for (int i = 0; i < numData; i++)
		stream >> data[i];
}

void BSWArray::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numData;
	for (int i = 0; i < numData; i++)
		stream << data[i];
}

std::vector<uint> BSWArray::GetData() {
	return data;
}

void BSWArray::SetData(const std::vector<uint>& dat) {
	numData = dat.size();
	data = dat;
}


BSPositionData::BSPositionData(NiStream& stream) : BSPositionData() {
	Get(stream);
}

void BSPositionData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numData;
	data.resize(numData);
	for (int i = 0; i < numData; i++)
		stream >> data[i];
}

void BSPositionData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numData;
	for (int i = 0; i < numData; i++)
		stream << data[i];
}

std::vector<half_float::half> BSPositionData::GetData() {
	return data;
}

void BSPositionData::SetData(const std::vector<half_float::half>& dat) {
	numData = dat.size();
	data = dat;
}


BSEyeCenterExtraData::BSEyeCenterExtraData(NiStream& stream) : BSEyeCenterExtraData() {
	Get(stream);
}

void BSEyeCenterExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numData;
	data.resize(numData);
	for (int i = 0; i < numData; i++)
		stream >> data[i];
}

void BSEyeCenterExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numData;
	for (int i = 0; i < numData; i++)
		stream << data[i];
}

std::vector<float> BSEyeCenterExtraData::GetData() {
	return data;
}

void BSEyeCenterExtraData::SetData(const std::vector<float>& dat) {
	numData = dat.size();
	data = dat;
}


void BSPackedGeomData::Get(NiStream& stream) {
	stream >> numVerts;

	stream >> lodLevels;
	lod.resize(lodLevels);
	for (int i = 0; i < lodLevels; i++)
		stream >> lod[i];

	stream >> numCombined;
	combined.resize(numCombined);
	for (int i = 0; i < numCombined; i++)
		stream >> combined[i];

	stream >> unkInt1;
	stream >> unkInt2;
}

void BSPackedGeomData::Put(NiStream& stream) {
	stream << numVerts;

	stream << lodLevels;
	for (int i = 0; i < lodLevels; i++)
		stream << lod[i];

	stream << numCombined;
	for (int i = 0; i < numCombined; i++)
		stream << combined[i];

	stream << unkInt1;
	stream << unkInt2;
}


BSPackedCombinedSharedGeomDataExtra::BSPackedCombinedSharedGeomDataExtra(NiStream& stream) : BSPackedCombinedSharedGeomDataExtra() {
	Get(stream);
}

void BSPackedCombinedSharedGeomDataExtra::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	vertDesc.Get(stream);
	stream >> numVertices;
	stream >> numTriangles;
	stream >> unkFlags1;
	stream >> unkFlags2;

	stream >> numData;
	objects.resize(numData);
	data.resize(numData);

	for (int i = 0; i < numData; i++)
		stream >> objects[i];

	for (int i = 0; i < numData; i++)
		data[i].Get(stream);
}

void BSPackedCombinedSharedGeomDataExtra::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	vertDesc.Put(stream);
	stream << numVertices;
	stream << numTriangles;
	stream << unkFlags1;
	stream << unkFlags2;
	stream << numData;

	for (int i = 0; i < numData; i++)
		stream << objects[i];

	for (int i = 0; i < numData; i++)
		data[i].Put(stream);
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


BSDistantObjectLargeRefExtraData::BSDistantObjectLargeRefExtraData(NiStream& stream) : BSDistantObjectLargeRefExtraData() {
	Get(stream);
}

void BSDistantObjectLargeRefExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);
	stream >> largeRef;
}

void BSDistantObjectLargeRefExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);
	stream << largeRef;
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
