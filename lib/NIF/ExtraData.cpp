/*
BodySlide and Outfit Studio
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

std::vector<byte> NiBinaryExtraData::GetData() {
	return data;
}

void NiBinaryExtraData::SetData(const std::vector<byte>& dat) {
	size = dat.size();
	data = dat;
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

std::vector<NiString> NiStringsExtraData::GetStringsData() {
	return stringsData;
}

void NiStringsExtraData::SetStringsData(const std::vector<NiString>& strsData) {
	numStrings = strsData.size();
	stringsData = strsData;
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

std::vector<FurniturePosition> BSFurnitureMarker::GetPositions() {
	return positions;
}

void BSFurnitureMarker::SetPositions(const std::vector<FurniturePosition>& pos) {
	numPositions = pos.size();
	positions = pos;
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

std::vector<DecalVectorBlock> BSDecalPlacementVectorExtraData::GetDecalVectorBlocks() {
	return decalVectorBlocks;
}

void BSDecalPlacementVectorExtraData::SetDecalVectorBlocks(const std::vector<DecalVectorBlock>& vectorBlocks) {
	numVectorBlocks = vectorBlocks.size();
	decalVectorBlocks = vectorBlocks;
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

std::vector<BoneLOD> BSBoneLODExtraData::GetBoneLODs() {
	return boneLODs;
}

void BSBoneLODExtraData::SetBoneLODs(const std::vector<BoneLOD>& lods) {
	numBoneLODs = lods.size();
	boneLODs = lods;
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

std::vector<Key<StringRef>> NiTextKeyExtraData::GetTextKeys() {
	return textKeys;
}

void NiTextKeyExtraData::SetTextKeys(const std::vector<Key<StringRef>>& keys) {
	numTextKeys = keys.size();
	textKeys = keys;
}


void BSDistantObjectLargeRefExtraData::Get(NiStream& stream) {
	NiExtraData::Get(stream);
	stream >> largeRef;
}

void BSDistantObjectLargeRefExtraData::Put(NiStream& stream) {
	NiExtraData::Put(stream);
	stream << largeRef;
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


void BSConnectPointParents::Get(NiStream& stream) {
	NiExtraData::Get(stream);

	stream >> numConnectPoints;
	connectPoints.resize(numConnectPoints);

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints[i].Get(stream);
}

void BSConnectPointParents::Put(NiStream& stream) {
	NiExtraData::Put(stream);

	stream << numConnectPoints;

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints[i].Put(stream);
}

std::vector<BSConnectPoint> BSConnectPointParents::GetConnectPoints() {
	return connectPoints;
}

void BSConnectPointParents::SetConnectPoints(const std::vector<BSConnectPoint>& cps) {
	numConnectPoints = cps.size();
	connectPoints = cps;
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

std::vector<NiString> BSConnectPointChildren::GetTargets() {
	return targets;
}

void BSConnectPointChildren::SetTargets(const std::vector<NiString>& targ) {
	numTargets = targ.size();
	targets = targ;
}


BSClothExtraData::BSClothExtraData(const uint size) {
	numBytes = size;
	data.resize(size);
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

std::vector<char> BSClothExtraData::GetData() {
	return data;
}

void BSClothExtraData::SetData(const std::vector<char>& dat) {
	numBytes = dat.size();
	data = dat;
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
