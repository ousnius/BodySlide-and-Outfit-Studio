/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "NifFile.h"
#pragma warning (disable : 4100)

NiObject::~NiObject() {
}

void NiObject::Init() {
}

void NiObject::notifyBlockDelete(int blockID) {
}

void NiObject::notifyVerticesDelete(const vector<ushort>& vertIndices) {
}

void NiObject::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
}

void NiObject::Get(fstream& file) {
}

void NiObject::Put(fstream& file) {
}

int NiObject::CalcBlockSize() {
	blockSize = 0;	// Calculate from the ground up
	return blockSize;
}


NiHeader::NiHeader() {
	header = this;
	blockSize = 0;
	numBlocks = 0;
	numStrings = 0;
	blocks = nullptr;
	blockType = NIHEADER;
}

void NiHeader::Clear() {
	numBlockTypes = 0;
	numStrings = 0;
	numBlocks = 0;
	blocks = nullptr;
	blockTypes.clear();
	blockIndex.clear();
	blockSizes.clear();
	strings.clear();
}

void NiHeader::SetVersion(const byte& v1, const byte& v2, const byte& v3, const byte& v4, const uint& userVer, const uint& userVer2) {
	string verString = "Gamebryo File Format, Version " + to_string(v1) + '.' + to_string(v2) + '.' + to_string(v3) + '.' + to_string(v4);
	strncpy(verStr, verString.c_str(), 0x26);

	version4 = v1;
	version3 = v2;
	version2 = v3;
	version1 = v4;
	userVersion = userVer;
	userVersion2 = userVer2;
}

bool NiHeader::VerCheck(int v1, int v2, int v3, int v4, bool equal) {
	if (equal) {
		if (version4 == v1 && version3 == v2 && version2 == v3 && version1 == v4)
			return true;
	}
	else {
		if (version4 >= v1 && version3 >= v2 && version2 >= v3 && version1 >= v4)
			return true;
	}

	return false;
}

bool NiHeader::HasBlockType(const string& typeStr) {
	for (int i = 0; i < blockTypes.size(); i++)
		if (blockTypes[i].str == typeStr)
			return true;

	return false;
}

ushort NiHeader::AddOrFindBlockTypeId(const string& blockTypeName) {
	NiString niStr;
	ushort typeId = (ushort)blockTypes.size();
	for (ushort i = 0; i < blockTypes.size(); i++) {
		if (blockTypes[i].str == blockTypeName) {
			typeId = i;
			break;
		}
	}

	// Shader block type not found, add it
	if (typeId == blockTypes.size()) {
		niStr.str = blockTypeName;
		blockTypes.push_back(niStr);
		numBlockTypes++;
	}
	return typeId;
}

int NiHeader::FindStringId(const string& str) {
	for (int i = 0; i < strings.size(); i++)
		if (strings[i].str.compare(str) == 0)
			return i;

	return -1;
}

int NiHeader::AddOrFindStringId(const string& str) {
	if (str.empty())
		return 0xFFFFFFFF;

	for (int i = 0; i < strings.size(); i++)
		if (strings[i].str.compare(str) == 0)
			return i;

	int r = strings.size();
	NiString niStr;
	niStr.str = str;
	strings.push_back(niStr);
	numStrings++;
	if (str.length() > maxStringLen)
		maxStringLen = str.length();

	return r;
}

void NiHeader::Get(fstream& file) {
	file.read(verStr, 38);
	if (_strnicmp(verStr, "Gamebryo", 8) != 0) {
		verStr[0] = 0;
		return;
	}

	//file >> bitangentY;
	unk1 = 10;
	file >> version1 >> version2 >> version3 >> version4;
	file >> endian;
	file.read((char*)&userVersion, 4);
	file.read((char*)&numBlocks, 4);
	file.read((char*)&userVersion2, 4);
	creator.Get(file, 1);
	exportInfo1.Get(file, 1);
	exportInfo2.Get(file, 1);
	
	if (userVersion2 >= 130)
		exportInfo3.Get(file, 1);

	file.read((char*)&numBlockTypes, 2);
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes.push_back(NiString(file, 4));

	ushort uShort;
	for (int i = 0; i < numBlocks; i++) {
		file.read((char*)&uShort, 2);
		blockIndex.push_back(uShort);
	}
	uint uInt;
	for (int i = 0; i < numBlocks; i++) {
		file.read((char*)&uInt, 4);
		blockSizes.push_back(uInt);
	}

	file.read((char*)&numStrings, 4);
	file.read((char*)&maxStringLen, 4);
	for (int i = 0; i < numStrings; i++)
		strings.push_back(NiString(file, 4));

	file.read((char*)&unkInt2, 4);
}

void NiHeader::Put(fstream& file) {
	file.write(verStr, 0x26);
	file << unk1;
	file << version1 << version2 << version3 << version4;
	file << endian;
	file.write((char*)&userVersion, 4);
	file.write((char*)&numBlocks, 4);
	file.write((char*)&userVersion2, 4);
	creator.Put(file, 1);
	exportInfo1.Put(file, 1);
	exportInfo2.Put(file, 1);
	if (userVersion2 >= 130)
		exportInfo3.Put(file, 1);

	file.write((char*)&numBlockTypes, 2);
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes[i].Put(file, 4, false);

	for (int i = 0; i < numBlocks; i++)
		file.write((char*)&blockIndex[i], 2);

	for (int i = 0; i < numBlocks; i++)
		file.write((char*)&blockSizes[i], 4);

	file.write((char*)&numStrings, 4);
	file.write((char*)&maxStringLen, 4);
	for (int i = 0; i < numStrings; i++)
		strings[i].Put(file, 4, false);

	file.write((char*)&unkInt2, 4);
}


NiString::NiString() {
}

NiString::NiString(fstream& file, int szSize) {
	Get(file, szSize);
}

void NiString::Put(fstream& file, int szSize, bool wantNullOutput) {
	if (szSize == 1) {
		byte smSize = str.length();
		if (wantNullOutput)
			smSize += 1;

		file.write((char*)&smSize, 1);
	}
	else if (szSize == 2) {
		ushort medSize = str.length();
		if (wantNullOutput)
			medSize += 1;

		file.write((char*)&medSize, 2);
	}
	else if (szSize == 4) {
		uint bigSize = str.length();
		if (wantNullOutput)
			bigSize += 1;

		file.write((char*)&bigSize, 4);
	}

	file.write(str.c_str(), str.length());
	if (wantNullOutput)
		file.put(0);
}

void NiString::Get(fstream& file, int szSize) {
	char buf[1025];

	if (szSize == 1) {
		byte smSize;
		file.read((char*)&smSize, 1);
		file.read(buf, smSize);
		buf[smSize] = 0;
	}
	else if (szSize == 2) {
		ushort medSize;
		file.read((char*)&medSize, 2);
		if (medSize < 1025)
			file.read(buf, medSize);
		buf[medSize] = 0;
	}
	else if (szSize == 4) {
		uint bigSize;
		file.read((char*)&bigSize, 4);
		if (bigSize < 1025)
			file.read(buf, bigSize);
		buf[bigSize] = 0;
	}
	else
		return;

	str = buf;
}


void NiObjectNET::Init() {
	NiObject::Init();

	bBSLightingShaderProperty = false;
	skyrimShaderType = 0;
	name.clear();
	nameRef = 0xFFFFFFFF;
	numExtraData = 0;
	controllerRef = -1;
}

void NiObjectNET::Get(fstream& file) {
	NiObject::Get(file);

	if (bBSLightingShaderProperty && header->userVersion >= 12)
		file.read((char*)&skyrimShaderType, 4);

	file.read((char*)&nameRef, 4);
	if (nameRef != -1)
		name = header->strings[nameRef].str;
	else
		name.clear();

	int intData;
	file.read((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++) {
		file.read((char*)&intData, 4);
		extraDataRef.push_back(intData);
	}

	file.read((char*)&controllerRef, 4);
}

void NiObjectNET::Put(fstream& file) {
	NiObject::Put(file);

	if (bBSLightingShaderProperty && header->userVersion >= 12)
		file.write((char*)&skyrimShaderType, 4);

	file.write((char*)&nameRef, 4);

	file.write((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++)
		file.write((char*)&extraDataRef[i], 4);

	file.write((char*)&controllerRef, 4);
}

void NiObjectNET::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	for (int i = 0; i < numExtraData; i++) {
		if (extraDataRef[i] == blockID) {
			extraDataRef.erase(extraDataRef.begin() + i);
			i--;
			numExtraData--;
		}
		else if (extraDataRef[i] > blockID)
			extraDataRef[i]--;
	}

	if (controllerRef == blockID)
		controllerRef = -1;
	else if (controllerRef > blockID)
		controllerRef--;
}

void NiObjectNET::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	for (int i = 0; i < numExtraData; i++) {
		if (extraDataRef[i] == blockIndexLo)
			extraDataRef[i] = blockIndexHi;
		else if (extraDataRef[i] == blockIndexHi)
			extraDataRef[i] = blockIndexLo;
	}

	if (controllerRef == blockIndexLo)
		controllerRef = blockIndexHi;
	else if (controllerRef == blockIndexHi)
		controllerRef = blockIndexLo;
}

string NiObjectNET::GetName() {
	return name;
}

void NiObjectNET::SetName(const string& propertyName, bool renameExisting) {
	if (renameExisting) {
		header->strings[nameRef].str = propertyName;
		if (header->maxStringLen < propertyName.length())
			header->maxStringLen = propertyName.length();
	}
	else
		nameRef = header->AddOrFindStringId(propertyName);

	name = propertyName;
}

void NiObjectNET::ClearName() {
	nameRef = 0xFFFFFFFF;
	name.clear();
}

int NiObjectNET::CalcBlockSize() {
	NiObject::CalcBlockSize();

	if (bBSLightingShaderProperty && header->userVersion >= 12)
		blockSize += 4;

	blockSize += 12;
	blockSize += numExtraData * 4;

	return blockSize;
}


void NiProperty::Init() {
	NiObjectNET::Init();
}

void NiProperty::Get(fstream& file) {
	NiObjectNET::Get(file);
}

void NiProperty::Put(fstream& file) {
	NiObjectNET::Put(file);
}

void NiProperty::notifyBlockDelete(int blockID) {
	NiObjectNET::notifyBlockDelete(blockID);
}

void NiProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObjectNET::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiProperty::CalcBlockSize() {
	return NiObjectNET::CalcBlockSize();
}


void NiAVObject::Init() {
	NiObjectNET::Init();

	flags = 14;
	unkShort1 = 8;
	rotation[0].x = 1.0f;
	rotation[1].y = 1.0f;
	rotation[2].z = 1.0f;
	scale = 1.0f;
	numProperties = 0;
	collisionRef = -1;
}

void NiAVObject::Get(fstream& file) {
	NiObjectNET::Get(file);

	file.read((char*)&flags, 2);

	if (header->userVersion >= 11 && header->userVersion2 > 26)
		file.read((char*)&unkShort1, 2);

	file.read((char*)&translation.x, 4);
	file.read((char*)&translation.y, 4);
	file.read((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.read((char*)&rotation[i].x, 4);
		file.read((char*)&rotation[i].y, 4);
		file.read((char*)&rotation[i].z, 4);
	}

	file.read((char*)&scale, 4);

	int intData;
	if (header->userVersion <= 11) {
		file.read((char*)&numProperties, 4);
		for (int i = 0; i < numProperties; i++) {
			file.read((char*)&intData, 4);
			propertiesRef.push_back(intData);
		}
	}

	file.read((char*)&collisionRef, 4);
}

void NiAVObject::Put(fstream& file) {
	NiObjectNET::Put(file);

	file.write((char*)&flags, 2);

	if (header->userVersion >= 11 && header->userVersion2 > 26)
		file.write((char*)&unkShort1, 2);

	file.write((char*)&translation.x, 4);
	file.write((char*)&translation.y, 4);
	file.write((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.write((char*)&rotation[i].x, 4);
		file.write((char*)&rotation[i].y, 4);
		file.write((char*)&rotation[i].z, 4);
	}

	file.write((char*)&scale, 4);

	if (header->userVersion <= 11) {
		file.write((char*)&numProperties, 4);
		for (int i = 0; i < numProperties; i++)
			file.write((char*)&propertiesRef[i], 4);
	}

	file.write((char*)&collisionRef, 4);
}

void NiAVObject::notifyBlockDelete(int blockID) {
	NiObjectNET::notifyBlockDelete(blockID);

	if (collisionRef == blockID)
		collisionRef = -1;
	else if (collisionRef > blockID)
		collisionRef--;

	for (int i = 0; i < numProperties; i++) {
		if (propertiesRef[i] == blockID) {
			propertiesRef.erase(propertiesRef.begin() + i);
			i--;
			numProperties--;
		}
		else if (propertiesRef[i] > blockID)
			propertiesRef[i]--;
	}
}
void NiAVObject::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObjectNET::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (collisionRef == blockIndexLo)
		collisionRef = blockIndexHi;
	else if (collisionRef == blockIndexHi)
		collisionRef = blockIndexLo;

	for (int i = 0; i < numProperties; i++) {
		if (propertiesRef[i] == blockIndexHi)
			propertiesRef[i] = blockIndexLo;
		else if (propertiesRef[i] == blockIndexLo)
			propertiesRef[i] = blockIndexHi;
	}
}

int NiAVObject::CalcBlockSize() {
	NiObjectNET::CalcBlockSize();

	blockSize += 58;
	if (header->userVersion <= 11) {
		blockSize += 4;
		blockSize += numProperties * 4;
	}
	if (header->userVersion >= 11 && header->userVersion2 > 26)
		blockSize += 2;

	return blockSize;
}


NiNode::NiNode(NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NINODE;
	numChildren = 0;
	numEffects = 0;
}

NiNode::NiNode(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NINODE;
	numChildren = 0;
	numEffects = 0;

	Get(file);
}

void NiNode::Get(fstream& file) {
	NiAVObject::Get(file);

	int intData;
	file.read((char*)&numChildren, 4);
	for (int i = 0; i < numChildren; i++) {
		file.read((char*)&intData, 4);
		children.push_back(intData);
	}

	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		file.read((char*)&numEffects, 4);
		for (int i = 0; i < numEffects; i++) {
			file.read((char*)&intData, 4);
			effects.push_back(intData);
		}
	}
}

void NiNode::Put(fstream& file) {
	NiAVObject::Put(file);

	if (children.size() != numChildren) {	// error somewhere else with block management :(
		return;
	}

	file.write((char*)&numChildren, 4);
	for (int i = 0; i < numChildren; i++)
		file.write((char*)&children[i], 4);

	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		file.write((char*)&numEffects, 4);
		for (int i = 0; i < numEffects; i++)
			file.write((char*)&effects[i], 4);
	}
}

void NiNode::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	for (int i = 0; i < numChildren; i++) {
		if (children[i] == blockID) {
			children.erase(children.begin() + i);
			i--;
			numChildren--;
		}
		else if (children[i] > blockID)
			children[i]--;
	}
	for (int i = 0; i < numEffects; i++) {
		if (effects[i] == blockID) {
			effects.erase(effects.begin() + i);
			i--;
			numEffects--;
		}
		else if (effects[i] > blockID)
			effects[i]--;
	}
}

void NiNode::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numChildren; i++) {
		if (children[i] == blockIndexLo)
			children[i] = blockIndexHi;
		else if (children[i] == blockIndexHi)
			children[i] = blockIndexLo;
	}
	for (int i = 0; i < numEffects; i++) {
		if (effects[i] == blockIndexLo)
			effects[i] = blockIndexHi;
		else if (effects[i] == blockIndexHi)
			effects[i] = blockIndexLo;
	}
}

int NiNode::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 4;
	blockSize += numChildren * 4;
	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		blockSize += 4;
		blockSize += numEffects * 4;
	}
	return blockSize;
}

BSTriShape::BSTriShape(NiHeader& hdr) {
	NiAVObject::Init();

	blockType = BSTRISHAPE;
	header = &hdr;

	memset(unkProps, 0, sizeof(float)* 4);
	skinInstanceRef = -1;
	shaderPropertyRef = -1;
	alphaPropertyRef = -1;

	// flags for vert data look to be stored in here.  byte 0 or byte 6 specifically look promising .  
	//  using byte 6 currently, bit 3 indicating sub index data,  bit 2 indicating the presence of color data.  bit 1 indicating presence of normal data	
	vertFlags[0] = 0x5;
	vertFlags[1] = 0x2;
	vertFlags[2] = 0x43;
	vertFlags[3] = 0x50;
	vertFlags[4] = 0x0;
	vertFlags[5] = 0xB0;
	vertFlags[6] = 0x1;
	vertFlags[7] = 0x0;
	numTriangles = 0;
	numVertices = 0;
	dataSize = 0;
	vertRecSize = 20;	// size of vertex structure calculated with (datasize - (numtris*6)) / numverts;
}

BSTriShape::BSTriShape(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	blockType = BSTRISHAPE;
	header = &hdr;

	Get(file);
}

void BSTriShape::Get(fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&unkShort1, 2);

	file.read((char*)&translation.x, 4);
	file.read((char*)&translation.y, 4);
	file.read((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.read((char*)&rotation[i].x, 4);
		file.read((char*)&rotation[i].y, 4);
		file.read((char*)&rotation[i].z, 4);
	}

	file.read((char*)&scale, 4);
	file.read((char*)&collisionRef, 4);

	for (int i = 0; i < 4; i++)
		file.read((char*)&unkProps[i], 4);

	file.read((char*)&skinInstanceRef, 4);
	file.read((char*)&shaderPropertyRef, 4);

	file.read((char*)&alphaPropertyRef, 4);
	for (int i = 0; i < 8; i++)
		file.read((char*)&vertFlags[i], 1);

	file.read((char*)&numTriangles, 4);
	file.read((char*)&numVertices, 2);

	file.read((char*)&dataSize, 4);
	vertRecSize = (dataSize - (numTriangles * 6)) / numVertices;

	half_float::half halfData;
	vertData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		file.read((char*)&halfData, 2);
		vertData[i].vert.x = halfData;
		file.read((char*)&halfData, 2);
		vertData[i].vert.y = halfData;
		file.read((char*)&halfData, 2);
		vertData[i].vert.z = halfData;

		file.read((char*)&halfData, 2);
		vertData[i].bitangentX = halfData;

		file.read((char*)&halfData, 2);
		vertData[i].uv.u = halfData;
		file.read((char*)&halfData, 2);
		vertData[i].uv.v = halfData;

		if (vertFlags[6] & 0x1) {
			for (int j = 0; j < 3; j++)
				file.read((char*)&vertData[i].normal[j], 1);
		
			file.read((char*)&vertData[i].bitangentY, 1);

			for (int j = 0; j < 3; j++)
				file.read((char*)&vertData[i].tangent[j], 1);

			file.read((char*)&vertData[i].bitangentZ, 1);
		}

		if (vertFlags[6] & 0x2)
			file.read((char*)&vertData[i].colorData, 4);

		if (vertFlags[6] & 0x4) {
			for (int j = 0; j < 4; j++) {
				file.read((char*)&halfData, 2);
				vertData[i].weights[j] = halfData;//h2float(shortData);
			}

			for (int j = 0; j < 4; j++)
				file.read((char*)&vertData[i].weightBones[j], 1);
		}
	}

	triangles.resize(numTriangles);
	for (int i = 0; i < numTriangles; i++) {
		file.read((char*)&triangles[i].p1, 2);
		file.read((char*)&triangles[i].p2, 2);
		file.read((char*)&triangles[i].p3, 2);
	}
}

void BSTriShape::Put(fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&unkShort1, 2);

	file.write((char*)&translation.x, 4);
	file.write((char*)&translation.y, 4);
	file.write((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.write((char*)&rotation[i].x, 4);
		file.write((char*)&rotation[i].y, 4);
		file.write((char*)&rotation[i].z, 4);
	}

	file.write((char*)&scale, 4);
	file.write((char*)&collisionRef, 4);

	for (int i = 0; i < 4; i++)
		file.write((char*)&unkProps[i], 4);

	file.write((char*)&skinInstanceRef, 4);
	file.write((char*)&shaderPropertyRef, 4);

	file.write((char*)&alphaPropertyRef, 4);
	for (int i = 0; i < 8; i++)
		file.write((char*)&vertFlags[i], 1);

	file.write((char*)&numTriangles, 4);
	file.write((char*)&numVertices, 2);

	file.write((char*)&dataSize, 4);
	
	half_float::half halfData;
	for (int i = 0; i < numVertices; i++) {
		halfData = vertData[i].vert.x;
		file.write((char*)&halfData, 2);

		halfData = vertData[i].vert.y;
		file.write((char*)&halfData, 2);

		halfData = vertData[i].vert.z;
		file.write((char*)&halfData, 2);

		halfData = vertData[i].bitangentX;
		file.write((char*)&halfData, 2);
		
		halfData = vertData[i].uv.u;
		file.write((char*)&halfData, 2);

		halfData = vertData[i].uv.v;
		file.write((char*)&halfData, 2);

		if (vertFlags[6] & 0x1) {
			for (int j = 0; j < 3; j++)
				file.write((char*)&vertData[i].normal[j], 1);
		
			file.write((char*)&vertData[i].bitangentY, 1);

			for (int j = 0; j < 3; j++)
				file.write((char*)&vertData[i].tangent[j], 1);

			file.write((char*)&vertData[i].bitangentZ, 1);
		}

		if (vertFlags[6] & 0x2)
			file.write((char*)&vertData[i].colorData, 4);

		if (vertFlags[6] & 0x4) {
			for (int j = 0; j < 4; j++) {
				halfData = vertData[i].weights[j];
				file.write((char*)&halfData, 2);
			}

			for (int j = 0; j < 4; j++)
				file.write((char*)&vertData[i].weightBones[j], 1);
		}
	}

	for (int i = 0; i < numTriangles; i++) {
		file.write((char*)&triangles[i].p1, 2);
		file.write((char*)&triangles[i].p2, 2);
		file.write((char*)&triangles[i].p3, 2);
	}
}

void BSTriShape::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (skinInstanceRef == blockID)
		skinInstanceRef = -1;
	else if (skinInstanceRef > blockID)
		skinInstanceRef--;

	if (shaderPropertyRef == blockID)
		shaderPropertyRef = -1;
	else if (shaderPropertyRef > blockID)
		shaderPropertyRef--;

	if (alphaPropertyRef == blockID)
		alphaPropertyRef = -1;
	else if (alphaPropertyRef > blockID)
		alphaPropertyRef--;
}

void BSTriShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (skinInstanceRef == blockIndexLo)
		skinInstanceRef = blockIndexHi;
	else if (skinInstanceRef == blockIndexHi)
		skinInstanceRef = blockIndexLo;

	if (shaderPropertyRef == blockIndexLo)
		shaderPropertyRef = blockIndexHi;
	else if (shaderPropertyRef == blockIndexHi)
		shaderPropertyRef = blockIndexLo;

	if (alphaPropertyRef == blockIndexLo)
		alphaPropertyRef = blockIndexHi;
	else if (alphaPropertyRef == blockIndexHi)
		alphaPropertyRef = blockIndexLo;
}

void BSTriShape::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertData.size(), 0);
	vector<int> indexCollapseTris(vertData.size(), 0);
	
	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			indexCollapseTris[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapseTris[i] = remCount;
	}

	for (int i = vertData.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertData.erase(vertData.begin() + i);
			numVertices--;
		}
	}

	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapseTris[triangles[i].p1] == -1 || indexCollapseTris[triangles[i].p2] == -1 || indexCollapseTris[triangles[i].p3] == -1) {
			triangles.erase(triangles.begin() + i);
			numTriangles--;
		}
		else {
			triangles[i].p1 = triangles[i].p1 - indexCollapseTris[triangles[i].p1];
			triangles[i].p2 = triangles[i].p2 - indexCollapseTris[triangles[i].p2];
			triangles[i].p3 = triangles[i].p3 - indexCollapseTris[triangles[i].p3];
		}
	}
}

int BSTriShape::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 46 + 6 * numTriangles;

	int vdataSize = 12;
	if (vertFlags[6] & 0x1)		//normals
		vdataSize += 8;

	if (vertFlags[6] & 0x2)		//colors
		vdataSize += 4;

	if (vertFlags[6] & 0x4)		//skinning
		vdataSize += 12;

	blockSize += vdataSize * numVertices;

	return blockSize;
}

const vector<Vector3>* BSTriShape::GetRawVerts(bool xform) {
	rawVertices.clear();
	rawVertices.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawVertices[i] = vertData[i].vert;

	return &rawVertices;
}

const vector<Vector3>* BSTriShape::GetNormalData(bool xform) {
	rawNormals.clear();
	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float q1 = (((float)vertData[i].normal[0])/255.0f) * 2.0f - 1.0f;
		float q2 = (((float)vertData[i].normal[1])/255.0f) * 2.0f - 1.0f;
		float q3 = (((float)vertData[i].normal[2])/255.0f) * 2.0f - 1.0f;

		float x = q1;
		float y = q2;
		float z = q3;

		if (xform) {
			rawNormals[i].x = -x;
			rawNormals[i].z = y;
			rawNormals[i].y = z;
		}
		else {
			rawNormals[i].x = x;
			rawNormals[i].z = z;
			rawNormals[i].y = y;
		}
	}

	return &rawNormals;
}

const vector<Vector3>* BSTriShape::GetTangentData(bool xform) {
	rawTangents.clear();
	rawTangents.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float q6 = (((float)vertData[i].tangent[0]) / 255.0f) * 2.0f - 1.0f;
		float q7 = (((float)vertData[i].tangent[1]) / 255.0f) * 2.0f - 1.0f;
		float q8 = (((float)vertData[i].tangent[2]) / 255.0f) * 2.0f - 1.0f;
		float x = q6;
		float y = q7;
		float z = q8;

		if (xform) {
			rawTangents[i].x = -x;
			rawTangents[i].z = y;
			rawTangents[i].y = z;
		}
		else {
			rawTangents[i].x = x;
			rawTangents[i].z = z;
			rawTangents[i].y = y;
		}
	}

	return &rawTangents;
}

const vector<Vector3>* BSTriShape::GetBitangentData(bool xform) {
	rawBitangents.clear();
	rawBitangents.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float x = (vertData[i].bitangentX);
		float y = (((float)vertData[i].bitangentY) / 255.0f) * 2.0f - 1.0f;
		float z = (((float)vertData[i].bitangentZ) / 255.0f) * 2.0f - 1.0f;


		if (xform) {
			rawBitangents[i].x = -x;
			rawBitangents[i].z = y;
			rawBitangents[i].y = z;
		}
		else {
			rawBitangents[i].x = x;
			rawBitangents[i].z = z;
			rawBitangents[i].y = y;
		}
	}
	return &rawBitangents;
}

const vector<Vector2>* BSTriShape::GetUVData() {
	rawUvs.clear();
	rawUvs.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawUvs[i] = vertData[i].uv;

	return &rawUvs;
}

void BSTriShape::SetNormals(const vector<Vector3>& inNorms) {
	rawNormals.clear();
	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawNormals[i] = inNorms[i];
		vertData[i].normal[0] = (unsigned char)round((((inNorms[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[1] = (unsigned char)round((((inNorms[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[2] = (unsigned char)round((((inNorms[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::RecalcNormals(const bool& smooth, const float& smoothThresh) {
	GetRawVerts();

	Vertex* verts = new Vertex[numVertices];
	Triangle* tris = new Triangle[numTriangles];

	for (int i = 0; i < numVertices; i++) {
		verts[i].x = rawVertices[i].x * -0.1f;
		verts[i].z = rawVertices[i].y * 0.1f;
		verts[i].y = rawVertices[i].z * 0.1f;
		verts[i].indexRef = i;
	}

	// Load tris. Also sum face normals here.
	Vector3 norm;
	for (int j = 0; j < numTriangles; j++) {
		tris[j].p1 = triangles[j].p1;
		tris[j].p2 = triangles[j].p2;
		tris[j].p3 = triangles[j].p3;
		tris[j].trinormal(verts, &norm);
		verts[tris[j].p1].nx += norm.x;
		verts[tris[j].p1].ny += norm.y;
		verts[tris[j].p1].nz += norm.z;
		verts[tris[j].p2].nx += norm.x;
		verts[tris[j].p2].ny += norm.y;
		verts[tris[j].p2].nz += norm.z;
		verts[tris[j].p3].nx += norm.x;
		verts[tris[j].p3].ny += norm.y;
		verts[tris[j].p3].nz += norm.z;
	}

	// Normalize all vertex normals to smooth them out.
	for (int i = 0; i < numVertices; i++) {
		Vector3* pn = (Vector3*)&verts[i].nx;
		pn->Normalize();
	}

	vector<int>* vertTris = new vector<int>[numVertices];
	for (int t = 0; t < numTriangles; t++) {
		vertTris[tris[t].p1].push_back(t);
		vertTris[tris[t].p2].push_back(t);
		vertTris[tris[t].p3].push_back(t);
	}

	unordered_map<int, vector<int>> weldVerts;
	kd_matcher matcher(verts, numVertices);
	for (int i = 0; i < matcher.matches.size(); i++) {
		Vertex* a = matcher.matches[i].first;
		Vertex* b = matcher.matches[i].second;
		weldVerts[a->indexRef].push_back(b->indexRef);
		weldVerts[b->indexRef].push_back(a->indexRef);

		if (smooth) {
			float dot = (a->nx * b->nx + a->ny * b->ny + a->nz * b->nz);
			if (dot < 1.57079633f) {
				a->nx = ((a->nx + b->nx) / 2.0f);
				a->ny = ((a->ny + b->ny) / 2.0f);
				a->nz = ((a->nz + b->nz) / 2.0f);
				b->nx = a->nx;
				b->ny = a->ny;
				b->nz = a->nz;
			}
		}
	}

	// Smooth normals
	if (smooth) {
		Vector3 tn;
		for (int v = 0; v < numVertices; v++) {
			norm.x = norm.y = norm.z = 0.0f;
			if (weldVerts.find(v) != weldVerts.end())
				continue;

			for (auto &t : vertTris[v]) {
				tris[t].trinormal(verts, &tn);
				norm += tn;
			}

			for (auto &wv : weldVerts[v]) {
				bool first = true;
				if (vertTris[wv].size() < 2)
					continue;

				for (auto &t : vertTris[wv]) {
					tris[t].trinormal(verts, &tn);
					if (!first) {
						float angle = fabs(norm.angle(tn));
						if (angle > smoothThresh * DEG2RAD)
							continue;
					}
					else
						first = false;

					norm += tn;
				}
			}

			norm = norm / (float)vertTris[v].size();
			norm.Normalize();
			verts[v].nx = norm.x;
			verts[v].ny = norm.y;
			verts[v].nz = norm.z;
		}
	}

	rawNormals.clear();
	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawNormals[i].x = -verts[i].nx;
		rawNormals[i].y = verts[i].nz;
		rawNormals[i].z = verts[i].ny;
		vertData[i].normal[0] = (unsigned char)round((((rawNormals[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[1] = (unsigned char)round((((rawNormals[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[2] = (unsigned char)round((((rawNormals[i].z + 1.0f) / 2.0f) * 255.0f));
	}

	delete[] vertTris;
	delete[] verts;
	delete[] tris;
}

void BSTriShape::CalcTangentSpace() {
	if (rawNormals.empty())
		GetNormalData(false);

	vector<Vector3> tan1;
	vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	for (int i = 0; i < triangles.size(); i++) {
		int i1 = triangles[i].p1;
		int i2 = triangles[i].p2;
		int i3 = triangles[i].p3;

		Vector3 v1 = vertData[i1].vert;
		Vector3 v2 = vertData[i2].vert;
		Vector3 v3 = vertData[i3].vert;

		Vector2 w1 = vertData[i1].uv;
		Vector2 w2 = vertData[i2].uv;
		Vector2 w3 = vertData[i3].uv;
		
		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r =  (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += tdir;
		tan1[i2] += tdir;
		tan1[i3] += tdir;

		tan2[i1] += sdir;
		tan2[i2] += sdir;
		tan2[i3] += sdir;
	}

	rawBitangents.resize(numVertices);
	rawTangents.resize(numVertices);

	for (int i = 0; i < numVertices; i++) {
		rawTangents[i] = tan1[i];
		rawBitangents[i] = tan2[i];

		if (rawTangents[i].IsZero() || rawBitangents[i].IsZero()) {
			rawTangents[i].x = rawNormals[i].y;
			rawTangents[i].y = rawNormals[i].z;
			rawTangents[i].z = rawNormals[i].x;
			rawBitangents[i] = rawNormals[i].cross(rawTangents[i]);
		}
		else {
			rawTangents[i].Normalize();
			rawTangents[i] = (rawTangents[i] - rawNormals[i] * rawNormals[i].dot(rawTangents[i]));
			rawTangents[i].Normalize();

			rawBitangents[i].Normalize();

			rawBitangents[i] = (rawBitangents[i] - rawNormals[i] * rawNormals[i].dot(rawBitangents[i]));
			rawBitangents[i] = (rawBitangents[i] - rawTangents[i] * rawTangents[i].dot(rawBitangents[i]));

			rawBitangents[i].Normalize();
		}

		vertData[i].tangent[0] = (unsigned char)round((((rawTangents[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[1] = (unsigned char)round((((rawTangents[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[2] = (unsigned char)round((((rawTangents[i].z + 1.0f) / 2.0f) * 255.0f));

		vertData[i].bitangentX = rawBitangents[i].x;
		vertData[i].bitangentY = (unsigned char)round((((rawBitangents[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].bitangentZ = (unsigned char)round((((rawBitangents[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	unkShort1 = 0; //from AVObject -- zero for these blocks.
	numVertices = verts->size();
	numTriangles = tris->size();

	vertData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		vertData[i].vert = (*verts)[i];

		if (uvs && uvs->size() == numVertices)
			vertData[i].uv = (*uvs)[i];

		vertData[i].bitangentX = 0.0f;
		vertData[i].bitangentY = 0;
		vertData[i].bitangentZ = 0;
		vertData[i].normal[0] = vertData[i].normal[1] = vertData[i].normal[2] = 0;
		memset(vertData[i].colorData, 255, 4);
		memset(vertData[i].weights, 0, sizeof(float)* 4);
		memset(vertData[i].weightBones, 0, 4);
	}

	if (normals && normals->size() == numVertices) {
		SetNormals((*normals));
		CalcTangentSpace();
	}

	vertRecSize = 12;
	if (vertFlags[6] & 0x1)		//normals
		vertRecSize += 8;

	if (vertFlags[6] & 0x2)		//colors
		vertRecSize += 4;

	if (vertFlags[6] & 0x4)		//skinning
		vertRecSize += 12;

	dataSize = 6 * numTriangles + numVertices * vertRecSize;

	triangles.resize(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		triangles[i] = (*tris)[i];
}


BSSubIndexTriShape::BSSubIndexTriShape(NiHeader& hdr) : BSTriShape(hdr) {
	blockType = BSSUBINDEXTRISHAPE;

	numTriangles2 = 0;
	numSegments = 0;
	numSubIndexParts = 0;
	numSequences = 0;
	numSubIndexRecords = 0;
}

BSSubIndexTriShape::BSSubIndexTriShape(fstream& file, NiHeader& hdr) : BSTriShape(file, hdr) {
	blockType = BSSUBINDEXTRISHAPE;
	Get(file);
}

void BSSubIndexTriShape::Get(fstream& file) {
	file.read((char*)&numTriangles2, 4);
	file.read((char*)&numSegments, 4);
	file.read((char*)&numSubIndexParts, 4);

	segments.resize(numSubIndexParts * 4);
	for (int i = 0; i < numSubIndexParts * 4; i++)
		file.read((char*)&segments[i], 4);

	if (numSubIndexParts > numSegments) {
		file.read((char*)&numSequences, 4);
		file.read((char*)&numSubIndexRecords, 4);

		sequences.resize(numSegments);
		for (int i = 0; i < numSegments; i++)
			file.read((char*)&sequences[i], 4);

		subIndexRecords.resize(numSubIndexParts);
		for (int i = 0; i < numSubIndexParts; i++) {
			file.read((char*)&subIndexRecords[i].unk1, 4);
			file.read((char*)&subIndexRecords[i].unk2, 4);
			file.read((char*)&subIndexRecords[i].numExtra, 4);

			subIndexRecords[i].extraData.resize(subIndexRecords[i].numExtra);
			for (int j = 0; j < subIndexRecords[i].numExtra; j++)
				file.read((char*)&subIndexRecords[i].extraData[j], 4);
		}

		ssfFile.Get(file, 2);
	}
}

void BSSubIndexTriShape::Put(fstream& file) {
	BSTriShape::Put(file);

	file.write((char*)&numTriangles2, 4);
	file.write((char*)&numSegments, 4);
	file.write((char*)&numSubIndexParts, 4);

	for (int i = 0; i < numSubIndexParts * 4; i++)
		file.write((char*)&segments[i], 4);

	if (numSubIndexParts > numSegments) {
		file.write((char*)&numSequences, 4);
		file.write((char*)&numSubIndexRecords, 4);

		for (int i = 0; i < numSegments; i++)
			file.write((char*)&sequences[i], 4);

		for (int i = 0; i < numSubIndexParts; i++) {
			file.write((char*)&subIndexRecords[i].unk1, 4);
			file.write((char*)&subIndexRecords[i].unk2, 4);
			file.write((char*)&subIndexRecords[i].numExtra, 4);

			for (int j = 0; j < subIndexRecords[i].numExtra; j++)
				file.write((char*)&subIndexRecords[i].extraData[j], 4);
		}

		ssfFile.Put(file, 2, false);
	}
}

void BSSubIndexTriShape::notifyBlockDelete(int blockID) {
	BSTriShape::notifyBlockDelete(blockID);
}

void BSSubIndexTriShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	BSTriShape::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void BSSubIndexTriShape::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	SetDefaultSegments();
}

int BSSubIndexTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	blockSize += 12;										// tris and first record counts
	blockSize += numSubIndexParts * 4 * 4;					// sub index record array A

	if (numSubIndexParts > numSegments) {
		blockSize += 8;										// second index accounts
		blockSize += numSegments * 4;						// sequence array
		for (int i = 0; i < numSubIndexParts; i++) {		// sub index records B
			blockSize += 12;								// unknown data per record
			blockSize += 4 * subIndexRecords[i].numExtra;	// extra data per record
		}

		blockSize += 2;
		blockSize += ssfFile.str.length();
	}

	return blockSize;
}

void BSSubIndexTriShape::SetDefaultSegments() {
	vertFlags[0] = 8;
	vertFlags[6] = 5;

	numTriangles2 = numTriangles;
	numSegments = 4;
	numSubIndexParts = 4;
	numSequences = 0;
	numSubIndexRecords = 0;

	sequences.clear();
	subIndexRecords.clear();
	ssfFile.str.clear();

	segments.resize(4 * 32);
	int n = 0;
	for (int i = 0; i < 3; i++) {
		segments[n++] = 0x0;
		segments[n++] = 0x0;
		segments[n++] = 0xFFFFFFFF;
		segments[n++] = 0x0;
	}

	segments[n++] = 0x0;
	segments[n++] = numTriangles;
	segments[n++] = 0xFFFFFFFF;
	segments[n++] = 0x0;
}

void BSSubIndexTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	SetDefaultSegments();
}


BSMeshLODTriShape::BSMeshLODTriShape(NiHeader& hdr) : BSTriShape(hdr) {
	blockType = BSMESHLODTRISHAPE;

	lodSize0 = 0;
	lodSize1 = 0;
	lodSize2 = 0;
}

BSMeshLODTriShape::BSMeshLODTriShape(fstream& file, NiHeader& hdr) : BSTriShape(file, hdr) {
	blockType = BSMESHLODTRISHAPE;
	Get(file);
}

void BSMeshLODTriShape::Get(fstream& file) {
	file.read((char*)&lodSize0, 4);
	file.read((char*)&lodSize1, 4);
	file.read((char*)&lodSize2, 4);
}

void BSMeshLODTriShape::Put(fstream& file) {
	BSTriShape::Put(file);

	file.write((char*)&lodSize0, 4);
	file.write((char*)&lodSize1, 4);
	file.write((char*)&lodSize2, 4);
}

void BSMeshLODTriShape::notifyBlockDelete(int blockID) {
	BSTriShape::notifyBlockDelete(blockID);
}

void BSMeshLODTriShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	BSTriShape::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void BSMeshLODTriShape::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);
}

int BSMeshLODTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}

void BSMeshLODTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);
}


void NiGeometry::Init() {
	NiAVObject::Init();

	propertiesRef1 = -1;
	propertiesRef2 = -1;
	dataRef = -1;
	skinInstanceRef = -1;
	numMaterials = 0;
	activeMaterial = 0;
	dirty = 0;
}

void NiGeometry::Get(fstream& file) {
	NiAVObject::Get(file);

	file.read((char*)&dataRef, 4);
	file.read((char*)&skinInstanceRef, 4);
	file.read((char*)&numMaterials, 4);
	for (int i = 0; i < numMaterials; i++)
		materialNames.push_back(NiString(file, 2));

	int intData;
	for (int i = 0; i < numMaterials; i++){
		file.read((char*)&intData, 4);
		materialExtra.push_back(intData);
	}
	file.read((char*)&activeMaterial, 4);
	file.read((char*)&dirty, 1);

	if (header->userVersion == 12) {
		file.read((char*)&propertiesRef1, 4);
		file.read((char*)&propertiesRef2, 4);
	}
}

void NiGeometry::Put(fstream& file) {
	NiAVObject::Put(file);

	file.write((char*)&dataRef, 4);
	file.write((char*)&skinInstanceRef, 4);
	file.write((char*)&numMaterials, 4);
	for (int i = 0; i < numMaterials; i++)
		materialNames[i].Put(file, 2);

	for (int i = 0; i < numMaterials; i++)
		file.write((char*)&materialExtra[i], 4);

	file.write((char*)&activeMaterial, 4);
	file.write((char*)&dirty, 1);

	if (header->userVersion > 11) {
		file.write((char*)&propertiesRef1, 4);
		file.write((char*)&propertiesRef2, 4);
	}
}

void NiGeometry::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinInstanceRef == blockID)
		skinInstanceRef = -1;
	else if (skinInstanceRef > blockID)
		skinInstanceRef--;

	if (propertiesRef1 >= blockID)
		propertiesRef1--;
	if (propertiesRef2 >= blockID)
		propertiesRef2--;
}

void NiGeometry::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;

	if (skinInstanceRef == blockIndexLo)
		skinInstanceRef = blockIndexHi;
	else if (skinInstanceRef == blockIndexHi)
		skinInstanceRef = blockIndexLo;

	if (propertiesRef1 == blockIndexLo)
		propertiesRef1 = blockIndexHi;
	else if (propertiesRef1 == blockIndexHi)
		propertiesRef1 = blockIndexLo;

	if (propertiesRef2 == blockIndexLo)
		propertiesRef2 = blockIndexHi;
	else if (propertiesRef2 == blockIndexHi)
		propertiesRef2 = blockIndexLo;
}

int NiGeometry::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 17;
	blockSize += numMaterials * 2;
	blockSize += numMaterials * 4;
	if (header->userVersion > 11)
		blockSize += 8;

	return blockSize;
}


void NiGeometryData::Init() {
	NiObject::Init();

	unkInt = 0;
	numVertices = 0;
	keepFlags = 0;
	compressFlags = 0;
	hasVertices = true;
	numUVSets = 0;
	//extraVectorsFlags = 0;
	unkInt2 = 0;
	hasNormals = false;
	radius = 0.0f;
	hasVertexColors = false;
	consistencyFlags = 0;
	additionalData = 0xFFFFFFFF;
}

void NiGeometryData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&unkInt, 4);
	file.read((char*)&numVertices, 2);
	file.read((char*)&keepFlags, 1);
	file.read((char*)&compressFlags, 1);
	file.read((char*)&hasVertices, 1);

	Vector3 v;
	for (int i = 0; i < numVertices; i++) {
		file.read((char*)&v.x, 4);
		file.read((char*)&v.y, 4);
		file.read((char*)&v.z, 4);
		vertices.push_back(v);
	}

	file.read((char*)&numUVSets, 2);
	//file.read((char*)&extraVectorsFlags, 1);
	if (header->userVersion == 12)
		file.read((char*)&unkInt2, 4);

	file.read((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numVertices; i++) {
			file.read((char*)&v.x, 4);
			file.read((char*)&v.y, 4);
			file.read((char*)&v.z, 4);
			normals.push_back(v);
		}
		if (numUVSets & 0xF000) {
			for (int i = 0; i < numVertices; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				tangents.push_back(v);
			}
			for (int i = 0; i < numVertices; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				bitangents.push_back(v);
			}
		}
	}

	file.read((char*)&center.x, 4);
	file.read((char*)&center.y, 4);
	file.read((char*)&center.z, 4);
	file.read((char*)&radius, 4);

	file.read((char*)&hasVertexColors, 1);
	if (hasVertexColors == 1) {
		Color4 c;
		for (int i = 0; i < numVertices; i++) {
			file.read((char*)&c, 16);
			vertexColors.push_back(c);
		}
	}
	if (numUVSets >= 1) {
		Vector2 uv;
		for (int i = 0; i < numVertices; i++) {
			file.read((char*)&uv.u, 4);
			file.read((char*)&uv.v, 4);
			uvSets.push_back(uv);
		}
	}

	file.read((char*)&consistencyFlags, 2);
	file.read((char*)&additionalData, 4);
}

void NiGeometryData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&unkInt, 4);
	file.write((char*)&numVertices, 2);
	file.write((char*)&keepFlags, 1);
	file.write((char*)&compressFlags, 1);

	file.write((char*)&hasVertices, 1);
	for (int i = 0; i < numVertices; i++) {
		file.write((char*)&vertices[i].x, 4);
		file.write((char*)&vertices[i].y, 4);
		file.write((char*)&vertices[i].z, 4);
	}

	file.write((char*)&numUVSets, 2);
	//file.write((char*)&extraVectorsFlags, 1);
	if (header->userVersion == 12)
		file.write((char*)&unkInt2, 4);

	file.write((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numVertices; i++) {
			file.write((char*)&normals[i].x, 4);
			file.write((char*)&normals[i].y, 4);
			file.write((char*)&normals[i].z, 4);
		}
		if (numUVSets & 0xF000) {
			for (int i = 0; i < numVertices; i++) {
				file.write((char*)&tangents[i].x, 4);
				file.write((char*)&tangents[i].y, 4);
				file.write((char*)&tangents[i].z, 4);
			}
			for (int i = 0; i < numVertices; i++) {
				file.write((char*)&bitangents[i].x, 4);
				file.write((char*)&bitangents[i].y, 4);
				file.write((char*)&bitangents[i].z, 4);
			}
		}
	}
	file.write((char*)&center.x, 4);
	file.write((char*)&center.y, 4);
	file.write((char*)&center.z, 4);
	file.write((char*)&radius, 4);

	file.write((char*)&hasVertexColors, 1);
	if (hasVertexColors == 1) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&vertexColors[i], 16);
	}
	if (numUVSets >= 1) {
		for (int i = 0; i < numVertices; i++) {
			file.write((char*)&uvSets[i].u, 4);
			file.write((char*)&uvSets[i].v, 4);
		}
	}
	file.write((char*)&consistencyFlags, 2);
	file.write((char*)&additionalData, 4);
}

void NiGeometryData::Create(vector<Vector3>* verts, vector<Triangle>* inTris, vector<Vector2>* texcoords) {
	Vector3 a(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3 b(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	unkInt = 0;
	radius = 0.0f;
	numVertices = verts->size();
	keepFlags = 0;
	compressFlags = 0;
	hasVertices = true;
	for (auto &v : *verts) {
		vertices.push_back(v);
		a.x = min(a.x, v.x);
		a.y = min(a.y, v.y);
		a.z = min(a.z, v.z);
		b.x = max(b.x, v.x);
		b.y = max(b.y, v.y);
		b.z = max(b.z, v.z);
	}

	if (texcoords->size() > 0)
		numUVSets = 4097;
	else
		numUVSets = 0;

	unkInt2 = 0;
	hasNormals = false;
	center = (a + b) / 2.0f;
	for (auto &v : *verts)
		radius = max(radius, center.DistanceTo(v));

	hasVertexColors = false;

	for (auto &uv : *texcoords)
		uvSets.push_back(uv);

	consistencyFlags = 16384;
	additionalData = 0xFFFFFFFF;
}


void NiGeometryData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertices.size(), 0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = bitangents.size() > 0;
	bool hasCol = vertexColors.size() > 0;
	bool hasUV = uvSets.size() > 0;

	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			j++;
		}
	}

	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numVertices--;
			if (hasNorm)
				normals.erase(normals.begin() + i);
			if (hasTan)
				tangents.erase(tangents.begin() + i);
			if (hasBin)
				bitangents.erase(bitangents.begin() + i);
			if (hasCol)
				vertexColors.erase(vertexColors.begin() + i);
			if (hasUV)
				uvSets.erase(uvSets.begin() + i);
		}
	}
}

void NiGeometryData::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);
}

void NiGeometryData::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void NiGeometryData::RecalcNormals() {
	for (int i = 0; i < numVertices; i++) {
		normals[i].x = 0;
		normals[i].y = 0;
		normals[i].z = 0;
	}
}

void NiGeometryData::CalcTangentSpace() {
	numUVSets |= 4096;

	tangents.resize(numVertices);
	bitangents.resize(numVertices);
}

int NiGeometryData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 35;
	if (header->userVersion == 12)
		blockSize += 4;

	blockSize += numVertices * 12;			// Verts
	blockSize += normals.size() * 12;		// Normals
	blockSize += tangents.size() * 12;		// Tangents
	blockSize += bitangents.size() * 12;	// Bitangents
	blockSize += vertexColors.size() * 16;	// Vertex Colors
	blockSize += uvSets.size() * 8;			// UVs 

	return blockSize;
}


void NiTriBasedGeom::Init() {
	NiGeometry::Init();
}

void NiTriBasedGeom::Get(fstream& file) {
	NiGeometry::Get(file);
}

void NiTriBasedGeom::Put(fstream& file) {
	NiGeometry::Put(file);
}

void NiTriBasedGeom::notifyBlockDelete(int blockID) {
	NiGeometry::notifyBlockDelete(blockID);
}

void NiTriBasedGeom::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiGeometry::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiTriBasedGeom::CalcBlockSize() {
	return NiGeometry::CalcBlockSize();
}


void NiTriBasedGeomData::Init() {
	NiGeometryData::Init();

	numTriangles = 0;
}

void NiTriBasedGeomData::Get(fstream& file) {
	NiGeometryData::Get(file);

	file.read((char*)&numTriangles, 2);
}

void NiTriBasedGeomData::Put(fstream& file) {
	NiGeometryData::Put(file);

	file.write((char*)&numTriangles, 2);
}

void NiTriBasedGeomData::Create(vector<Vector3>* verts, vector<Triangle>* inTris, vector<Vector2>* texcoords) {
	NiGeometryData::Create(verts, inTris, texcoords);

	numTriangles = inTris->size();
}

void NiTriBasedGeomData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	NiGeometryData::notifyVerticesDelete(vertIndices);
}

void NiTriBasedGeomData::notifyBlockDelete(int blockID) {
	NiGeometryData::notifyBlockDelete(blockID);
}

void NiTriBasedGeomData::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiGeometryData::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void NiTriBasedGeomData::RecalcNormals() {
	NiGeometryData::RecalcNormals();
}

void NiTriBasedGeomData::CalcTangentSpace() {
	NiGeometryData::CalcTangentSpace();
}

int NiTriBasedGeomData::CalcBlockSize() {
	NiGeometryData::CalcBlockSize();

	blockSize += 2;
	return blockSize;
}


NiTriShape::NiTriShape(NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISHAPE;
}

NiTriShape::NiTriShape(fstream& file, NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISHAPE;

	Get(file);
}

void NiTriShape::Get(fstream& file) {
	NiTriBasedGeom::Get(file);
}

void NiTriShape::Put(fstream& file) {
	NiTriBasedGeom::Put(file);
}

void NiTriShape::notifyBlockDelete(int blockID) {
	NiTriBasedGeom::notifyBlockDelete(blockID);
}

void NiTriShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTriBasedGeom::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiTriShape::CalcBlockSize() {
	return NiTriBasedGeom::CalcBlockSize();
}


NiTriShapeData::NiTriShapeData(NiHeader& hdr) {
	NiTriBasedGeomData::Init();

	header = &hdr;
	blockType = NITRISHAPEDATA;
	numTrianglePoints = 0;
	hasTriangles = false;
	numMatchGroups = 0;
}

NiTriShapeData::NiTriShapeData(fstream& file, NiHeader& hdr) {
	NiTriBasedGeomData::Init();

	header = &hdr;
	blockType = NITRISHAPEDATA;
	numTrianglePoints = 0;
	hasTriangles = false;
	numMatchGroups = 0;

	Get(file);
}

void NiTriShapeData::Get(fstream& file) {
	NiTriBasedGeomData::Get(file);

	file.read((char*)&numTrianglePoints, 4);
	file.read((char*)&hasTriangles, 1);
	if (hasTriangles == 1) {
		Triangle triangle;
		for (int i = 0; i < numTriangles; i++) {
			file.read((char*)&triangle.p1, 2);
			file.read((char*)&triangle.p2, 2);
			file.read((char*)&triangle.p3, 2);
			triangles.push_back(triangle);
		}
	}

	ushort uShort;
	MatchGroup mg;
	file.read((char*)&numMatchGroups, 2);
	for (int i = 0; i < numMatchGroups; i++) {
		file.read((char*)&mg.count, 2);
		mg.matches.clear();
		for (int j = 0; j < mg.count; j++) {
			file.read((char*)&uShort, 2);
			mg.matches.push_back(uShort);
		}
		matchGroups.push_back(mg);
	}

	// Not supported yet, so clear it again after reading
	matchGroups.clear();
	numMatchGroups = 0;
}

void NiTriShapeData::Put(fstream& file) {
	NiTriBasedGeomData::Put(file);

	file.write((char*)&numTrianglePoints, 4);
	file.write((char*)&hasTriangles, 1);
	if (hasTriangles == 1) {
		for (int i = 0; i < numTriangles; i++) {
			file.write((char*)&triangles[i].p1, 2);
			file.write((char*)&triangles[i].p2, 2);
			file.write((char*)&triangles[i].p3, 2);
		}
	}
	file.write((char*)&numMatchGroups, 2);
	for (int i = 0; i < numMatchGroups; i++) {
		file.write((char*)&matchGroups[i].count, 2);
		for (int j = 0; j < matchGroups[i].count; j++)
			file.write((char*)&matchGroups[i].matches[j], 2);
	}
}

void NiTriShapeData::Create(vector<Vector3>* verts, vector<Triangle>* inTris, vector<Vector2>* texcoords) {
	NiTriBasedGeomData::Create(verts, inTris, texcoords);

	numTrianglePoints = numTriangles * 3;
	hasTriangles = true;
	for (auto &t : *inTris)
		triangles.push_back(t);

	numMatchGroups = 0;
}

void NiTriShapeData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertices.size(), 0);
	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);

	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapse[triangles[i].p1] == -1 || indexCollapse[triangles[i].p2] == -1 || indexCollapse[triangles[i].p3] == -1) {
			triangles.erase(triangles.begin() + i);
			numTriangles--;
			numTrianglePoints -= 3;
		}
		else {
			triangles[i].p1 = triangles[i].p1 - indexCollapse[triangles[i].p1];
			triangles[i].p2 = triangles[i].p2 - indexCollapse[triangles[i].p2];
			triangles[i].p3 = triangles[i].p3 - indexCollapse[triangles[i].p3];
		}
	}
}

void NiTriShapeData::notifyBlockDelete(int blockID) {
	NiTriBasedGeomData::notifyBlockDelete(blockID);
}

void NiTriShapeData::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTriBasedGeomData::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void NiTriShapeData::RecalcNormals() {
	if (!hasNormals)
		return;

	NiTriBasedGeomData::RecalcNormals();

	// Calc normal
	Vector3 norm;
	for (int i = 0; i < numTriangles; i++) {
		triangles[i].trinormal(vertices, &norm);
		normals[triangles[i].p1].x += norm.x;
		normals[triangles[i].p1].y += norm.y;
		normals[triangles[i].p1].z += norm.z;
		normals[triangles[i].p2].x += norm.x;
		normals[triangles[i].p2].y += norm.y;
		normals[triangles[i].p2].z += norm.z;
		normals[triangles[i].p3].x += norm.x;
		normals[triangles[i].p3].y += norm.y;
		normals[triangles[i].p3].z += norm.z;
	}

	// Normalize all vertex normals to smooth them out
	for (int i = 0; i < numVertices; i++)
		normals[i].Normalize();

	Vertex* matchVerts = new Vertex[numVertices];

	for (int i = 0; i < numVertices; i++) {
		matchVerts[i].x = vertices[i].x;
		matchVerts[i].nx = normals[i].x;
		matchVerts[i].y = vertices[i].y;
		matchVerts[i].ny = normals[i].y;
		matchVerts[i].z = vertices[i].z;
		matchVerts[i].nz = normals[i].z;
	}

	kd_matcher matcher(matchVerts, numVertices);
	for (int i = 0; i < matcher.matches.size(); i++) {
		Vertex* a = matcher.matches[i].first;
		Vertex* b = matcher.matches[i].second;
		float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
		if (dot < 1.57079633f) {
			a->nx = ((a->nx + b->nx) / 2.0f);
			a->ny = ((a->ny + b->ny) / 2.0f);
			a->nz = ((a->nz + b->nz) / 2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}

	for (int i = 0; i < numVertices; i++) {
		normals[i].x = matchVerts[i].nx;
		normals[i].y = matchVerts[i].ny;
		normals[i].z = matchVerts[i].nz;
	}

	delete[] matchVerts;
}

void NiTriShapeData::CalcTangentSpace() {
	if (!hasNormals)
		return;

	NiTriBasedGeomData::CalcTangentSpace();

	vector<Vector3> tan1;
	vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	for (int i = 0; i < numTriangles; i++) {
		int i1 = triangles[i].p1;
		int i2 = triangles[i].p2;
		int i3 = triangles[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[i1];
		Vector2 w2 = uvSets[i2];
		Vector2 w3 = uvSets[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		r = (r >= 0 ? +1 : -1);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		Vector3 n = normals[i];
		Vector3 t = tan1[i];

		bitangents[i].x = t.x;
		bitangents[i].y = t.y;
		bitangents[i].z = t.z;
		bitangents[i].Normalize();

		tangents[i] = bitangents[i].cross(normals[i]);
		tangents[i].Normalize();
	}
}

int NiTriShapeData::CalcBlockSize() {
	NiTriBasedGeomData::CalcBlockSize();

	blockSize += 7;
	blockSize += triangles.size() * 6;	// Triangles
	for (auto &mg : matchGroups) {
		blockSize += 4;
		blockSize += mg.count * 2;
	}

	return blockSize;
}


NiTriStrips::NiTriStrips(NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISTRIPS;
}

NiTriStrips::NiTriStrips(fstream& file, NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISTRIPS;

	Get(file);
}

void NiTriStrips::Get(fstream& file) {
	NiTriBasedGeom::Get(file);
}

void NiTriStrips::Put(fstream& file) {
	NiTriBasedGeom::Put(file);
}

void NiTriStrips::notifyBlockDelete(int blockID) {
	NiTriBasedGeom::notifyBlockDelete(blockID);
}

void NiTriStrips::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTriBasedGeom::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiTriStrips::CalcBlockSize() {
	return NiTriBasedGeom::CalcBlockSize();
}


NiTriStripsData::NiTriStripsData(NiHeader& hdr) {
	NiTriBasedGeomData::Init();

	header = &hdr;
	blockType = NITRISTRIPSDATA;
	numStrips = 0;
	hasPoints = false;
}

NiTriStripsData::NiTriStripsData(fstream& file, NiHeader& hdr) {
	NiTriBasedGeomData::Init();

	header = &hdr;
	blockType = NITRISTRIPSDATA;
	numStrips = 0;
	hasPoints = false;

	Get(file);
}

void NiTriStripsData::Get(fstream& file) {
	NiTriBasedGeomData::Get(file);

	ushort uShort;
	file.read((char*)&numStrips, 2);
	for (int i = 0; i < numStrips; i++) {
		file.read((char*)&uShort, 2);
		stripLengths.push_back(uShort);
	}

	file.read((char*)&hasPoints, 1);
	if (hasPoints) {
		for (int i = 0; i < numStrips; i++) {
			points.push_back(vector<ushort>());
			for (int j = 0; j < stripLengths[i]; j++) {
				file.read((char*)&uShort, 2);
				points[i].push_back(uShort);
			}
		}
	}
}

void NiTriStripsData::Put(fstream& file) {
	NiTriBasedGeomData::Put(file);

	file.write((char*)&numStrips, 2);
	for (int i = 0; i < numStrips; i++)
		file.write((char*)&stripLengths[i], 2);

	file.write((char*)&hasPoints, 1);

	if (hasPoints) {
		for (int i = 0; i < numStrips; i++)
			for (int j = 0; j < stripLengths[i]; j++)
				file.write((char*)&points[i][j], 2);
	}
}

void NiTriStripsData::notifyBlockDelete(int blockID) {
	NiTriBasedGeomData::notifyBlockDelete(blockID);
}

void NiTriStripsData::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTriBasedGeomData::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void NiTriStripsData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertices.size(), 0);
	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);

	// This is not a healthy way to delete strip data. Probably need to restrip the shape.
	for (int i = 0; i < numStrips; i++) {
		for (int j = 0; j < stripLengths[i]; j++) {
			if (indexCollapse[points[i][j]] == -1) {
				points[i].erase(points[i].begin() + j);
				stripLengths[i]--;
			}
		}
	}
}

void NiTriStripsData::StripsToTris(vector<Triangle>* outTris) {
	Triangle triangle;
	for (int strip = 0; strip < numStrips; strip++) {
		for (int vi = 0; vi < stripLengths[strip] - 2; vi++) {
			if (vi & 1) {
				triangle.p1 = points[strip][vi];
				triangle.p2 = points[strip][vi + 2];
				triangle.p3 = points[strip][vi + 1];
			}
			else {
				triangle.p1 = points[strip][vi];
				triangle.p2 = points[strip][vi + 1];
				triangle.p3 = points[strip][vi + 2];
			}

			if (triangle.p1 == triangle.p2 || triangle.p2 == triangle.p3 || triangle.p3 == triangle.p1)
				continue;

			outTris->push_back(triangle);
		}
	}
}
 
void NiTriStripsData::RecalcNormals() {
	if (!hasNormals)
		return;

	NiTriBasedGeomData::RecalcNormals();

	vector<Triangle> tris;
	StripsToTris(&tris);

	// Calc normal
	Vector3 norm;
	for (int i = 0; i < tris.size(); i++) {
		tris[i].trinormal(vertices, &norm);
		normals[tris[i].p1].x += norm.x;
		normals[tris[i].p1].y += norm.y;
		normals[tris[i].p1].z += norm.z;
		normals[tris[i].p2].x += norm.x;
		normals[tris[i].p2].y += norm.y;
		normals[tris[i].p2].z += norm.z;
		normals[tris[i].p3].x += norm.x;
		normals[tris[i].p3].y += norm.y;
		normals[tris[i].p3].z += norm.z;
	}

	// Normalize all vertex normals to smooth them out
	for (int i = 0; i < numVertices; i++)
		normals[i].Normalize();

	Vertex* matchVerts = new Vertex[numVertices];

	for (int i = 0; i < numVertices; i++) {
		matchVerts[i].x = vertices[i].x;
		matchVerts[i].nx = normals[i].x;
		matchVerts[i].y = vertices[i].y;
		matchVerts[i].ny = normals[i].y;
		matchVerts[i].z = vertices[i].z;
		matchVerts[i].nz = normals[i].z;
	}

	kd_matcher matcher(matchVerts, numVertices);
	for (int i = 0; i < matcher.matches.size(); i++) {
		Vertex* a = matcher.matches[i].first;
		Vertex* b = matcher.matches[i].second;
		float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
		if (dot < 1.57079633f) {
			a->nx = ((a->nx + b->nx) / 2.0f);
			a->ny = ((a->ny + b->ny) / 2.0f);
			a->nz = ((a->nz + b->nz) / 2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}

	for (int i = 0; i < numVertices; i++) {
		normals[i].x = matchVerts[i].nx;
		normals[i].y = matchVerts[i].ny;
		normals[i].z = matchVerts[i].nz;
	}

	delete[] matchVerts;
}

void NiTriStripsData::CalcTangentSpace() {
	if (!hasNormals)
		return;

	NiTriBasedGeomData::CalcTangentSpace();

	vector<Vector3> tan1;
	vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	vector<Triangle> tris;
	StripsToTris(&tris);

	for (int i = 0; i < tris.size(); i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[i1];
		Vector2 w2 = uvSets[i2];
		Vector2 w3 = uvSets[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		Vector3 n = normals[i];
		Vector3 t = tan1[i];

		bitangents[i].x = t.x;
		bitangents[i].y = t.y;
		bitangents[i].z = t.z;
		bitangents[i].Normalize();

		tangents[i] = bitangents[i].cross(normals[i]);
		tangents[i].Normalize();
	}
}

int NiTriStripsData::CalcBlockSize() {
	NiTriBasedGeomData::CalcBlockSize();

	blockSize += 3;
	blockSize += numStrips * 2;				// Strip Lengths

	for (auto &pl : points)
		blockSize += pl.size() * 2;

	return blockSize;
}


NiSkinInstance::NiSkinInstance(NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = NISKININSTANCE;
}

NiSkinInstance::NiSkinInstance(fstream& file, NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = NISKININSTANCE;

	Get(file);
}

void NiSkinInstance::Init() {
	NiObject::Init();

	dataRef = -1;
	skinPartitionRef = -1;
	skeletonRootRef = -1;
	numBones = 0;
}

void NiSkinInstance::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&dataRef, 4);
	file.read((char*)&skinPartitionRef, 4);
	file.read((char*)&skeletonRootRef, 4);
	file.read((char*)&numBones, 4);

	uint uInt;
	for (int i = 0; i < numBones; i++) {
		file.read((char*)&uInt, 4);
		bones.push_back(uInt);
	}
}

void NiSkinInstance::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&dataRef, 4);
	file.write((char*)&skinPartitionRef, 4);
	file.write((char*)&skeletonRootRef, 4);
	file.write((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++)
		file.write((char*)&bones[i], 4);
}

void NiSkinInstance::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	int boneIndex = -1;
	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinPartitionRef == blockID)
		skinPartitionRef = -1;
	else if (skinPartitionRef > blockID)
		skinPartitionRef--;

	if (skeletonRootRef == blockID)
		skeletonRootRef = -1;
	else if (skeletonRootRef > blockID)
		skeletonRootRef--;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockID) {
			bones.erase(bones.begin() + i);
			boneIndex = i;
			i--;
			numBones--;
		}
		else if (bones[i] > blockID)
			bones[i]--;
	}
	if (boneIndex >= 0 && dataRef != -1) { // Bone was removed, clear out the skinning data for it.
		if (header->blocks == nullptr)
			return;

		NiSkinData* skinData = dynamic_cast<NiSkinData*>((*header->blocks)[dataRef]);
		if (!skinData)
			return;

		skinData->bones.erase(skinData->bones.begin() + boneIndex);
		skinData->numBones--;
	}
}

void NiSkinInstance::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);
	
	if (dataRef== blockIndexLo)
		dataRef= blockIndexHi;
	else if (dataRef== blockIndexHi)
		dataRef= blockIndexLo;	

	if (skinPartitionRef== blockIndexLo)
		skinPartitionRef= blockIndexHi;
	else if (skinPartitionRef== blockIndexHi)
		skinPartitionRef= blockIndexLo;
	
	if (skeletonRootRef== blockIndexLo)
		skeletonRootRef= blockIndexHi;
	else if (skeletonRootRef== blockIndexHi)
		skeletonRootRef= blockIndexLo;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockIndexLo)
			bones[i] = blockIndexHi;
		else if (bones[i] == blockIndexHi)
			bones[i] = blockIndexLo;
	}
}

int NiSkinInstance::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 16;
	blockSize += numBones * 4;
	return blockSize;
}


BSDismemberSkinInstance::BSDismemberSkinInstance(NiHeader& hdr) {
	NiSkinInstance::Init();

	header = &hdr;
	blockType = BSDISMEMBERSKININSTANCE;
	numPartitions = 0;
}

BSDismemberSkinInstance::BSDismemberSkinInstance(fstream& file, NiHeader& hdr) {
	NiSkinInstance::Init();

	header = &hdr;
	blockType = BSDISMEMBERSKININSTANCE;

	Get(file);
}

void BSDismemberSkinInstance::Get(fstream& file) {
	NiSkinInstance::Get(file);

	Partition part;
	file.read((char*)&numPartitions, 4);
	for (int i = 0; i < numPartitions; i++) {
		file.read((char*)&part.flags, 2);
		file.read((char*)&part.partID, 2);
		partitions.push_back(part);
	}
}

void BSDismemberSkinInstance::Put(fstream& file) {
	NiSkinInstance::Put(file);

	file.write((char*)&numPartitions, 4);
	for (int i = 0; i < numPartitions; i++) {
		file.write((char*)&partitions[i].flags, 2);
		file.write((char*)&partitions[i].partID, 2);
	}
}

void BSDismemberSkinInstance::notifyBlockDelete(int blockID) {
	NiSkinInstance::notifyBlockDelete(blockID);
}

void BSDismemberSkinInstance::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiSkinInstance::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSDismemberSkinInstance::CalcBlockSize() {
	NiSkinInstance::CalcBlockSize();

	blockSize += 4;
	blockSize += numPartitions * 4;
	return blockSize;
}



BSSkinInstance::BSSkinInstance(NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = BSSKININSTANCE;
}

BSSkinInstance::BSSkinInstance(fstream& file, NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = BSSKININSTANCE;

	Get(file);
}

void BSSkinInstance::Init() {
	NiObject::Init();

	targetRef = 0xFFFFFFFF;
	boneDataRef = 0xFFFFFFFF;
	numBones = 0;
	numVertices = 0;
}

void BSSkinInstance::Get(fstream& file) {
	NiObject::Get(file);
	uint intData;

	file.read((char*)&targetRef, 4);
	file.read((char*)&boneDataRef, 4);
	file.read((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++) {
		file.read((char*)&intData, 4);
		bones.push_back(intData);
	}

	file.read((char*)&numVertices, 4);
}

void BSSkinInstance::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&targetRef, 4);
	file.write((char*)&boneDataRef, 4);
	file.write((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++) {
		file.write((char*)&bones[i], 4);
	}
	file.write((char*)&numVertices, 4);
}

void BSSkinInstance::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	int boneIndex = -1;
	if (boneDataRef == blockID)
		boneDataRef = 0xFFFFFFFF;
	else if (boneDataRef > blockID)
		boneDataRef--;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockID) {
			bones.erase(bones.begin() + i);
			boneIndex = i;
			i--;
			numBones--;
		}
		else if (bones[i] > blockID)
			bones[i]--;
	}
}

void BSSkinInstance::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (boneDataRef == blockIndexLo)
		boneDataRef = blockIndexHi;
	else if (boneDataRef == blockIndexHi)
		boneDataRef = blockIndexLo;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockIndexLo)
			bones[i] = blockIndexHi;
		else if (bones[i] == blockIndexHi)
			bones[i] = blockIndexLo;
	}
}

BSSkinBoneData::BSSkinBoneData(NiHeader& hdr) {	
	NiObject::Init();
	nBones = 0;
	header = &hdr;
	blockType = BSBONEDATA;
}

BSSkinBoneData::BSSkinBoneData(fstream& file, NiHeader& hdr) {	
	NiObject::Init();

	nBones = 0;
	header = &hdr;
	blockType = BSBONEDATA;
	Get(file);
}

void BSSkinBoneData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nBones, 4);
	
	BoneData boneData;
	for (int i = 0; i < nBones; i++) {

		file.read((char*)&boneData.boundSphereOffset, 12);
		file.read((char*)&boneData.boundSphereRadius, 4);
		file.read((char*)&boneData.boneTransform, 52);

		boneXforms.push_back(boneData);
	}
}

void BSSkinBoneData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nBones, 4);

	for (int i = 0; i < nBones; i++) {

		file.write((char*)&boneXforms[i].boundSphereOffset, 12);
		file.write((char*)&boneXforms[i].boundSphereRadius, 4);
		file.write((char*)&boneXforms[i].boneTransform, 52);
	}
}

int BSSkinBoneData::CalcBlockSize() {
	NiObject::CalcBlockSize();
	blockSize += 68 * nBones;
	blockSize += 4;
	return blockSize;
}

int BSSkinInstance::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 16;
	blockSize += numBones * 4;
	return blockSize;
}

NiSkinData::NiSkinData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINDATA;
	skinTransform.scale = 1.0f;
	numBones = 0;
	hasVertWeights = 1;
}

NiSkinData::NiSkinData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINDATA;
	skinTransform.scale = 1.0f;
	numBones = 0;
	hasVertWeights = 1;

	Get(file);
}

void NiSkinData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&skinTransform, 52);
	file.read((char*)&numBones, 4);
	file.read((char*)&hasVertWeights, 1);

	SkinWeight sWt;
	BoneData boneData;
	for (int i = 0; i < numBones; i++) {
		boneData.vertexWeights.clear();

		file.read((char*)&boneData.boneTransform, 52);
		file.read((char*)&boneData.boundSphereOffset, 12);
		file.read((char*)&boneData.boundSphereRadius, 4);
		file.read((char*)&boneData.numVertices, 2);

		for (int j = 0; j < boneData.numVertices; j++) {
			file.read((char*)&sWt.index, 2);
			file.read((char*)&sWt.weight, 4);
			boneData.vertexWeights.push_back(sWt);
		}

		bones.push_back(boneData);
	}
	numBones = bones.size();
}

void NiSkinData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&skinTransform, 52);
	file.write((char*)&numBones, 4);
	file.write((char*)&hasVertWeights, 1);

	for (int i = 0; i < numBones; i++) {
		file.write((char*)&bones[i].boneTransform, 52);
		file.write((char*)&bones[i].boundSphereOffset, 12);
		file.write((char*)&bones[i].boundSphereRadius, 4);
		file.write((char*)&bones[i].numVertices, 2);

		for (int j = 0; j < bones[i].numVertices; j++) {
			file.write((char*)&bones[i].vertexWeights[j].index, 2);
			file.write((char*)&bones[i].vertexWeights[j].weight, 4);
		}
	}
}

void NiSkinData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	ushort highestRemoved = vertIndices[vertIndices.size() - 1];
	vector<int> indexCollapse(highestRemoved + 1, 0);

	NiObject::notifyVerticesDelete(vertIndices);

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	ushort ival;
	for (auto &b : bones) {
		for (int i = b.numVertices - 1; i >= 0; i--) {
			ival = b.vertexWeights[i].index;
			if (b.vertexWeights[i].index > highestRemoved) {
				b.vertexWeights[i].index -= remCount;
			}
			else if (indexCollapse[ival] == -1) {
				b.vertexWeights.erase(b.vertexWeights.begin() + i);
				b.numVertices--;
			}
			else
				b.vertexWeights[i].index -= indexCollapse[ival];
		}
	}
}
	
int NiSkinData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 57;
	for (auto & b : bones)
		blockSize += b.CalcSize();

	return blockSize;
}


NiSkinPartition::NiSkinPartition(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINPARTITION;
	numPartitions = 0;
	needsBuild = true;
}

NiSkinPartition::NiSkinPartition(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINPARTITION;
	numPartitions = 0;
	needsBuild = false;

	Get(file);
}

void NiSkinPartition::Get(fstream& file) {
	NiObject::Get(file);

	PartitionBlock partition;
	ushort uShort;
	VertexWeight weights;
	BoneIndices indices;
	Triangle triangle;

	file.read((char*)&numPartitions, 4);
	for (int p = 0; p < numPartitions; p++) {
		partition.bones.clear();
		partition.vertexMap.clear();
		partition.vertexWeights.clear();
		partition.stripLengths.clear();
		partition.strips.clear();
		partition.triangles.clear();
		partition.boneIndices.clear();

		file.read((char*)&partition.numVertices, 2);
		file.read((char*)&partition.numTriangles, 2);
		file.read((char*)&partition.numBones, 2);
		file.read((char*)&partition.numStrips, 2);
		file.read((char*)&partition.numWeightsPerVertex, 2);

		for (int i = 0; i < partition.numBones; i++) {
			file.read((char*)&uShort, 2);
			partition.bones.push_back(uShort);
		}

		file.read((char*)&partition.hasVertexMap, 1);
		for (int i = 0; (i < partition.numVertices) && partition.hasVertexMap; i++) {
			file.read((char*)&uShort, 2);
			partition.vertexMap.push_back(uShort);
		}

		file.read((char*)&partition.hasVertexWeights, 1);
		for (int i = 0; (i < partition.numVertices) && partition.hasVertexWeights; i++) {
			file.read((char*)&weights.w1, 4);
			file.read((char*)&weights.w2, 4);
			file.read((char*)&weights.w3, 4);
			file.read((char*)&weights.w4, 4);
			partition.vertexWeights.push_back(weights);
		}

		for (int i = 0; i < partition.numStrips; i++) {
			file.read((char*)&uShort, 2);
			partition.stripLengths.push_back(uShort);
		}

		file.read((char*)&partition.hasFaces, 1);
		for (int i = 0; (i < partition.numStrips) && partition.hasFaces; i++) {
			partition.strips.push_back(vector<ushort>());
			for (int j = 0; j < partition.stripLengths[i]; j++) {
				file.read((char*)&uShort, 2);
				partition.strips[i].push_back(uShort);
			}
		}

		if (partition.numStrips == 0) {
			for (int i = 0; (i < partition.numTriangles) && partition.hasFaces; i++) {
				file.read((char*)&triangle.p1, 2);
				file.read((char*)&triangle.p2, 2);
				file.read((char*)&triangle.p3, 2);
				partition.triangles.push_back(triangle);
			}
		}

		file.read((char*)&partition.hasBoneIndices, 1);
		for (int i = 0; i < partition.numVertices && partition.hasBoneIndices; i++) {
			file.read((char*)&indices.i1, 1);
			file.read((char*)&indices.i2, 1);
			file.read((char*)&indices.i3, 1);
			file.read((char*)&indices.i4, 1);
			partition.boneIndices.push_back(indices);
		}

		if (header->userVersion >= 12)
			file.read((char*)&partition.unkShort, 2);

		partitions.push_back(partition);
	}
}

void NiSkinPartition::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numPartitions, 4);
	for (int p = 0; p < numPartitions; p++) {
		file.write((char*)&partitions[p].numVertices, 2);
		file.write((char*)&partitions[p].numTriangles, 2);
		file.write((char*)&partitions[p].numBones, 2);
		file.write((char*)&partitions[p].numStrips, 2);
		file.write((char*)&partitions[p].numWeightsPerVertex, 2);
		for (int i = 0; i < partitions[p].numBones; i++)
			file.write((char*)&partitions[p].bones[i], 2);

		file.write((char*)&partitions[p].hasVertexMap, 1);
		for (int i = 0; (i < partitions[p].numVertices) && partitions[p].hasVertexMap; i++)
			file.write((char*)&partitions[p].vertexMap[i], 2);

		file.write((char*)&partitions[p].hasVertexWeights, 1);
		for (int i = 0; (i < partitions[p].numVertices) && partitions[p].hasVertexWeights; i++)
			file.write((char*)&partitions[p].vertexWeights[i], 16);

		for (int i = 0; i < partitions[p].numStrips; i++)
			file.write((char*)&partitions[p].stripLengths[i], 2);

		file.write((char*)&partitions[p].hasFaces, 1);
		for (int i = 0; (i < partitions[p].numStrips) && partitions[p].hasFaces; i++)
			for (int j = 0; j < partitions[p].stripLengths[i]; j++)
				file.write((char*)&partitions[p].strips[i][j], 2);

		if (partitions[p].numStrips == 0) {
			for (int i = 0; (i < partitions[p].numTriangles) && partitions[p].hasFaces; i++)
				file.write((char*)&partitions[p].triangles[i].p1, 6);
		}

		file.write((char*)&partitions[p].hasBoneIndices, 1);
		for (int i = 0; i < partitions[p].numVertices && partitions[p].hasBoneIndices; i++)
			file.write((char*)&partitions[p].boneIndices[i], 4);

		if (header->userVersion >= 12)
			file.write((char*)&partitions[p].unkShort, 2);
	}
}

void NiSkinPartition::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	ushort highestRemoved = vertIndices.back();
	vector<int> indexCollapse(highestRemoved + 1, 0);

	NiObject::notifyVerticesDelete(vertIndices);

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	size_t maxVertexMapSz = 0;
	for (auto &p : partitions)
		if (p.vertexMap.size() > maxVertexMapSz)
			maxVertexMapSz = p.vertexMap.size();

	vector<int> mapCollapse(maxVertexMapSz, 0);
	int mapRemCount = 0;
	int mapHighestRemoved = maxVertexMapSz;

	ushort index;
	for (auto &p : partitions) {
		mapRemCount = 0;
		mapHighestRemoved = maxVertexMapSz;
		for (int i = 0; i < p.vertexMap.size(); i++) {
			if (p.vertexMap[i] < indexCollapse.size() && indexCollapse[p.vertexMap[i]] == -1) {
				mapRemCount++;
				mapCollapse[i] = -1;
				mapHighestRemoved = i;
			}
			else
				mapCollapse[i] = mapRemCount;
		}

		for (int i = p.vertexMap.size() - 1; i >= 0; i--) {
			index = p.vertexMap[i];
			if (index > highestRemoved) {
				p.vertexMap[i] -= remCount;
			}
			else if (indexCollapse[index] == -1) {
				p.vertexMap.erase(p.vertexMap.begin() + i);
				p.numVertices--;
				if (p.hasVertexWeights)
					p.vertexWeights.erase(p.vertexWeights.begin() + i);
				if (p.hasBoneIndices)
					p.boneIndices.erase(p.boneIndices.begin() + i);
			}
			else
				p.vertexMap[i] -= indexCollapse[index];
		}

		for (int i = p.numTriangles - 1; i >= 0; i--) {
			if (p.triangles[i].p1 > mapHighestRemoved) {
				p.triangles[i].p1 -= mapRemCount;
			}
			else if (mapCollapse[p.triangles[i].p1] == -1) {
				p.triangles.erase(p.triangles.begin() + i);
				p.numTriangles--;
				continue;
			}
			else
				p.triangles[i].p1 -= mapCollapse[p.triangles[i].p1];

			if (p.triangles[i].p2 > mapHighestRemoved) {
				p.triangles[i].p2 -= mapRemCount;
			}
			else if (mapCollapse[p.triangles[i].p2] == -1) {
				p.triangles.erase(p.triangles.begin() + i);
				p.numTriangles--;
				continue;
			}
			else
				p.triangles[i].p2 -= mapCollapse[p.triangles[i].p2];

			if (p.triangles[i].p3 > mapHighestRemoved) {
				p.triangles[i].p3 -= mapRemCount;
			}
			else if (mapCollapse[p.triangles[i].p3] == -1) {
				p.triangles.erase(p.triangles.begin() + i);
				p.numTriangles--;
				continue;
			}
			else
				p.triangles[i].p3 -= mapCollapse[p.triangles[i].p3];
		}
	}
}

int NiSkinPartition::RemoveEmptyPartitions(vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = partitions.size() - 1; i >= 0; i--) {
		if (partitions[i].numVertices == 0) {
			outDeletedIndices.insert(outDeletedIndices.begin(), i);
			partitions.erase(partitions.begin() + i);
			numPartitions--;
		}
	}
	return outDeletedIndices.size();
}

int NiSkinPartition::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;
	for (auto &p : partitions) {
		if (header->userVersion >= 12)				// Plain data size
			blockSize += 16;
		else
			blockSize += 14;

		blockSize += 2 * p.numBones;				// Bone list size
		blockSize += 2 * p.numVertices;				// Vertex map size
		blockSize += 4 * p.numVertices;				// Bone index size
		blockSize += 16 * p.numVertices;			// Vertex weights size
		blockSize += 2 * p.numStrips;				// Strip lengths size
		for (int i = 0; i < p.numStrips; i++)
			blockSize += 2 * p.stripLengths[i];		// Strip list size

		if (p.numStrips == 0)
			blockSize += 6 * p.numTriangles;		// Triangle list size
	}

	return blockSize;
}


void NiInterpolator::Init() {
	NiObject::Init();
}

void NiInterpolator::Get(fstream& file) {
	NiObject::Get(file);
}

void NiInterpolator::Put(fstream& file) {
	NiObject::Put(file);
}

void NiInterpolator::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);
}
void NiInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiInterpolator::CalcBlockSize() {
	return NiObject::CalcBlockSize();
}


void NiKeyBasedInterpolator::Init() {
	NiInterpolator::Init();
}

void NiKeyBasedInterpolator::Get(fstream& file) {
	NiInterpolator::Get(file);
}

void NiKeyBasedInterpolator::Put(fstream& file) {
	NiInterpolator::Put(file);
}

void NiKeyBasedInterpolator::notifyBlockDelete(int blockID) {
	NiInterpolator::notifyBlockDelete(blockID);
}
void NiKeyBasedInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiKeyBasedInterpolator::CalcBlockSize() {
	return NiInterpolator::CalcBlockSize();
}


NiFloatInterpolator::NiFloatInterpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIFLOATINTERPOLATOR;
	floatValue = 0.0f;
	dataRef = -1;
}

NiFloatInterpolator::NiFloatInterpolator(fstream& file, NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIFLOATINTERPOLATOR;

	Get(file);
}

void NiFloatInterpolator::Get(fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&floatValue, 4);
	file.read((char*)&dataRef, 4);
}

void NiFloatInterpolator::Put(fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&floatValue, 4);
	file.write((char*)&dataRef, 4);
}

void NiFloatInterpolator::notifyBlockDelete(int blockID) {
	NiKeyBasedInterpolator::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;
}
void NiFloatInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiKeyBasedInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

int NiFloatInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


NiTransformInterpolator::NiTransformInterpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NITRANSFORMINTERPOLATOR;
	scale = 0.0f;
	dataRef = -1;
}

NiTransformInterpolator::NiTransformInterpolator(fstream& file, NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NITRANSFORMINTERPOLATOR;

	Get(file);
}

void NiTransformInterpolator::Get(fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&translation, 12);
	file.read((char*)&rotation, 16);
	file.read((char*)&scale, 4);
	file.read((char*)&dataRef, 4);
}

void NiTransformInterpolator::Put(fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&translation, 12);
	file.write((char*)&rotation, 16);
	file.write((char*)&scale, 4);
	file.write((char*)&dataRef, 4);
}

void NiTransformInterpolator::notifyBlockDelete(int blockID) {
	NiKeyBasedInterpolator::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;
}

void NiTransformInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiKeyBasedInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

int NiTransformInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 36;

	return blockSize;
}


NiPoint3Interpolator::NiPoint3Interpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIPOINT3INTERPOLATOR;
	dataRef = -1;
}

NiPoint3Interpolator::NiPoint3Interpolator(fstream& file, NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIPOINT3INTERPOLATOR;

	Get(file);
}

void NiPoint3Interpolator::Get(fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&point3Value, 12);
	file.read((char*)&dataRef, 4);
}

void NiPoint3Interpolator::Put(fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&point3Value, 12);
	file.write((char*)&dataRef, 4);
}

void NiPoint3Interpolator::notifyBlockDelete(int blockID) {
	NiKeyBasedInterpolator::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;
}

void NiPoint3Interpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiKeyBasedInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

int NiPoint3Interpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


void NiTimeController::Init() {
	NiObject::Init();

	nextControllerRef = -1;
	flags = 0x000C;
	frequency = 1.0f;
	phase = 0.0f;
	startTime = 0.0f;
	stopTime = 0.0f;
	targetRef = -1;
}

void NiTimeController::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nextControllerRef, 4);
	file.read((char*)&flags, 2);
	file.read((char*)&frequency, 4);
	file.read((char*)&phase, 4);
	file.read((char*)&startTime, 4);
	file.read((char*)&stopTime, 4);
	file.read((char*)&targetRef, 4);
}

void NiTimeController::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nextControllerRef, 4);
	file.write((char*)&flags, 2);
	file.write((char*)&frequency, 4);
	file.write((char*)&phase, 4);
	file.write((char*)&startTime, 4);
	file.write((char*)&stopTime, 4);
	file.write((char*)&targetRef, 4);
}

void NiTimeController::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	if (nextControllerRef == blockID)
		nextControllerRef = -1;
	else if (nextControllerRef > blockID)
		nextControllerRef--;

	if (targetRef == blockID)
		targetRef = -1;
	else if (targetRef > blockID)
		targetRef--;
}

void NiTimeController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (nextControllerRef == blockIndexLo)
		nextControllerRef = blockIndexHi;
	else if (nextControllerRef == blockIndexHi)
		nextControllerRef = blockIndexLo;

	if (targetRef == blockIndexLo)
		targetRef = blockIndexHi;
	else if (targetRef == blockIndexHi)
		targetRef = blockIndexLo;
}

int NiTimeController::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 26;

	return blockSize;
}


void NiInterpController::Init() {
	NiTimeController::Init();
}

void NiInterpController::Get(fstream& file) {
	NiTimeController::Get(file);
}

void NiInterpController::Put(fstream& file) {
	NiTimeController::Put(file);
}

void NiInterpController::notifyBlockDelete(int blockID) {
	NiTimeController::notifyBlockDelete(blockID);
}

void NiInterpController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTimeController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiInterpController::CalcBlockSize() {
	return NiTimeController::CalcBlockSize();
}


void NiSingleInterpController::Init() {
	NiInterpController::Init();

	interpolatorRef = -1;
}

void NiSingleInterpController::Get(fstream& file) {
	NiInterpController::Get(file);

	file.read((char*)&interpolatorRef, 4);
}

void NiSingleInterpController::Put(fstream& file) {
	NiInterpController::Put(file);

	file.write((char*)&interpolatorRef, 4);
}

void NiSingleInterpController::notifyBlockDelete(int blockID) {
	NiInterpController::notifyBlockDelete(blockID);

	if (interpolatorRef == blockID)
		interpolatorRef = -1;
	else if (interpolatorRef > blockID)
		interpolatorRef--;
}

void NiSingleInterpController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (interpolatorRef == blockIndexLo)
		interpolatorRef = blockIndexHi;
	else if (interpolatorRef == blockIndexHi)
		interpolatorRef = blockIndexLo;
}

int NiSingleInterpController::CalcBlockSize() {
	NiInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


void NiFloatInterpController::Init() {
	NiSingleInterpController::Init();
}

void NiFloatInterpController::Get(fstream& file) {
	NiSingleInterpController::Get(file);
}

void NiFloatInterpController::Put(fstream& file) {
	NiSingleInterpController::Put(file);
}

void NiFloatInterpController::notifyBlockDelete(int blockID) {
	NiSingleInterpController::notifyBlockDelete(blockID);
}

void NiFloatInterpController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiSingleInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiFloatInterpController::CalcBlockSize() {
	return NiSingleInterpController::CalcBlockSize();
}


BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER;
	typeOfControlledColor = 0;
}

BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(fstream& file, NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER;
	typeOfControlledColor = 0;

	Get(file);
}

void BSLightingShaderPropertyColorController::Get(fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledColor, 4);
}

void BSLightingShaderPropertyColorController::Put(fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledColor, 4);
}

void BSLightingShaderPropertyColorController::notifyBlockDelete(int blockID) {
	NiFloatInterpController::notifyBlockDelete(blockID);
}

void BSLightingShaderPropertyColorController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiFloatInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSLightingShaderPropertyColorController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER;
	typeOfControlledVariable = 0;
}

BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(fstream& file, NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER;

	Get(file);
}

void BSLightingShaderPropertyFloatController::Get(fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledVariable, 4);
}

void BSLightingShaderPropertyFloatController::Put(fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledVariable, 4);
}

void BSLightingShaderPropertyFloatController::notifyBlockDelete(int blockID) {
	NiFloatInterpController::notifyBlockDelete(blockID);
}

void BSLightingShaderPropertyFloatController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiFloatInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSLightingShaderPropertyFloatController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTYCOLORCONTROLLER;
	typeOfControlledColor = 0;
}

BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(fstream& file, NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTYCOLORCONTROLLER;
	typeOfControlledColor = 0;

	Get(file);
}

void BSEffectShaderPropertyColorController::Get(fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledColor, 4);
}

void BSEffectShaderPropertyColorController::Put(fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledColor, 4);
}

void BSEffectShaderPropertyColorController::notifyBlockDelete(int blockID) {
	NiFloatInterpController::notifyBlockDelete(blockID);
}

void BSEffectShaderPropertyColorController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiFloatInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSEffectShaderPropertyColorController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTYFLOATCONTROLLER;
	typeOfControlledVariable = 0;
}

BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(fstream& file, NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTYFLOATCONTROLLER;

	Get(file);
}

void BSEffectShaderPropertyFloatController::Get(fstream& file) {
	NiFloatInterpController::Get(file);

	file.read((char*)&typeOfControlledVariable, 4);
}

void BSEffectShaderPropertyFloatController::Put(fstream& file) {
	NiFloatInterpController::Put(file);

	file.write((char*)&typeOfControlledVariable, 4);
}

void BSEffectShaderPropertyFloatController::notifyBlockDelete(int blockID) {
	NiFloatInterpController::notifyBlockDelete(blockID);
}

void BSEffectShaderPropertyFloatController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiFloatInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSEffectShaderPropertyFloatController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


void NiShader::Get(fstream& file) {
}

void NiShader::Put(fstream& file) {
}

void NiShader::notifyBlockDelete(int blockID) {
}

void NiShader::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
}


bool NiShader::IsSkin() {
	return false;
}

bool NiShader::IsDoubleSided() {
	return false;
}

uint NiShader::GetType() {
	return 0xFFFFFFFF;
}

void NiShader::SetType(uint type) {
}

Vector3 NiShader::GetSpecularColor() {
	return Vector3();
}

void NiShader::SetSpecularColor(Vector3 color) {
}

float NiShader::GetSpecularStrength() {
	return 0.0f;
}

void NiShader::SetSpecularStrength(float strength) {
}

float NiShader::GetGlossiness() {
	return 0.0f;
}

void NiShader::SetGlossiness(float gloss) {
}

int NiShader::GetTextureSetRef() {
	return -1;
}

void NiShader::SetTextureSetRef(int texSetRef) {
}

Color4 NiShader::GetEmissiveColor() {
	return Color4();
}

void NiShader::SetEmissiveColor(Color4 color) {
}

float NiShader::GetEmissiveMultiple() {
	return 0.0f;
}

void NiShader::SetEmissiveMultiple(float emissive) {
}

string NiShader::GetWetMaterialName() {
	return "";
}

void NiShader::SetWetMaterialName(const string& matName) {
}

int NiShader::CalcBlockSize() {
	return blockSize;
}



BSLightingShaderProperty::BSLightingShaderProperty(NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;

	if (hdr.userVersion == 12 && hdr.userVersion2 >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;
	textureSetRef = -1;

	emissiveMultiple = 1.0f;
	wetMaterialNameRef = 0xFFFFFFFF;
	textureClampMode = 3;
	alpha = 1.0f;
	refractionStrength = 0.0f;

	if (hdr.userVersion == 12 && hdr.userVersion2 >= 120)
		glossiness = 1.0f;
	else
		glossiness = 20.0f;

	specularColor = Vector3(1.0f, 1.0f, 1.0f);
	specularStrength = 1.0f;
	lightingEffect1 = 0.3f;
	lightingEffect2 = 2.0f;

	environmentMapScale = 1.0f;
	maxPasses = 1.0f;
	scale = 1.0f;
	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	eyeCubemapScale = 1.0f;

	unk1 = 0x7F7FFFFF;
	unk2 = 0x3D4CCCCD;

	unk[0] = 1.0f;
	unk[1] = 5.0f;
	unk[2] = 0.6f;
	unk[3] = 1.4f;
	unk[4] = 0.2f;
	unk[5] = 1.0f;
	unk[6] = 1.6f;
	unk[7] = 0.0f;

	memset(pad, 0, 16);
}

BSLightingShaderProperty::BSLightingShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;

	if (hdr.userVersion == 12 && hdr.userVersion2 >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	wetMaterialNameRef = 0xFFFFFFFF;
	environmentMapScale = 1.0f;
	maxPasses = 1.0f;
	scale = 1.0f;

	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	eyeCubemapScale = 1.0f;

	Get(file);
}

void BSLightingShaderProperty::Get(fstream& file) {
	NiProperty::Get(file);

	if (header->userVersion == 12) {
		file.read((char*)&shaderFlags1, 4);
		file.read((char*)&shaderFlags2, 4);
	}

	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	file.read((char*)&textureSetRef, 4);

	file.read((char*)&emissiveColor.x, 4);
	file.read((char*)&emissiveColor.y, 4);
	file.read((char*)&emissiveColor.z, 4);
	file.read((char*)&emissiveMultiple, 4);
	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		file.read((char*)&wetMaterialNameRef, 4);
		if (wetMaterialNameRef != -1)
			wetMaterialName = header->strings[wetMaterialNameRef].str;
		else
			wetMaterialName.clear();
	}
	file.read((char*)&textureClampMode, 4);
	file.read((char*)&alpha, 4);
	file.read((char*)&refractionStrength, 4);
	file.read((char*)&glossiness, 4);
	file.read((char*)&specularColor.x, 4);
	file.read((char*)&specularColor.y, 4);
	file.read((char*)&specularColor.z, 4);
	file.read((char*)&specularStrength, 4);
	file.read((char*)&lightingEffect1, 4);

	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		file.read((char*)&lightingEffect2, 4);
	}
	else if (header->userVersion == 12 && header->userVersion2 >= 130) {
		file.read((char*)&unk1, 4);
		file.read((char*)&unk2, 4);
	}

	if (skyrimShaderType == 1) {
		file.read((char*)&environmentMapScale, 4);
	}
	else if (skyrimShaderType == 5 && header->userVersion2 < 130) {
		file.read((char*)&skinTintColor.x, 4);
		file.read((char*)&skinTintColor.y, 4);
		file.read((char*)&skinTintColor.z, 4);
	}
	else if (skyrimShaderType == 6) {
		file.read((char*)&hairTintColor.x, 4);
		file.read((char*)&hairTintColor.y, 4);
		file.read((char*)&hairTintColor.z, 4);
	}
	else if (skyrimShaderType == 7) {
		file.read((char*)&maxPasses, 4);
		file.read((char*)&scale, 4);
	}
	else if (skyrimShaderType == 11) {
		file.read((char*)&parallaxInnerLayerThickness, 4);
		file.read((char*)&parallaxRefractionScale, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.read((char*)&parallaxEnvmapStrength, 4);
	}
	else if (skyrimShaderType == 14) {
		file.read((char*)&sparkleParameters.r, 4);
		file.read((char*)&sparkleParameters.g, 4);
		file.read((char*)&sparkleParameters.b, 4);
		file.read((char*)&sparkleParameters.a, 4);
	}
	else if (skyrimShaderType == 16) {
		file.read((char*)&eyeCubemapScale, 4);
		file.read((char*)&eyeLeftReflectionCenter.x, 4);
		file.read((char*)&eyeLeftReflectionCenter.y, 4);
		file.read((char*)&eyeLeftReflectionCenter.z, 4);
		file.read((char*)&eyeRightReflectionCenter.x, 4);
		file.read((char*)&eyeRightReflectionCenter.y, 4);
		file.read((char*)&eyeRightReflectionCenter.z, 4);
	}

	if (header->userVersion2 >= 130) {
		for (int i = 0; i < 8; i++) {
			file.read((char*)&unk[i], 4);
		}
	}
	if (skyrimShaderType == 1 && header->userVersion2 >= 130) {
		for (int i = 0; i < 2; i++) {
			file.read((char*)&pad[i], 1);
		}
	}
	if (skyrimShaderType == 5 && header->userVersion2 >= 130) {
		for (int i = 0; i < 16; i++) {
			file.read((char*)&pad[i], 1);
		}
	}
}

void BSLightingShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	if (header->userVersion == 12) {
		file.write((char*)&shaderFlags1, 4);
		file.write((char*)&shaderFlags2, 4);
	}

	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	file.write((char*)&textureSetRef, 4);

	file.write((char*)&emissiveColor.x, 4);
	file.write((char*)&emissiveColor.y, 4);
	file.write((char*)&emissiveColor.z, 4);
	file.write((char*)&emissiveMultiple, 4);
	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		file.write((char*)&wetMaterialNameRef, 4);
	}
	file.write((char*)&textureClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&refractionStrength, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specularColor.x, 4);
	file.write((char*)&specularColor.y, 4);
	file.write((char*)&specularColor.z, 4);
	file.write((char*)&specularStrength, 4);
	file.write((char*)&lightingEffect1, 4);

	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		file.write((char*)&lightingEffect2, 4);
	}
	else if (header->userVersion == 12 && header->userVersion2 >= 130) {
		file.write((char*)&unk1, 4);
		file.write((char*)&unk2, 4);
	}

	if (skyrimShaderType == 1) {
		file.write((char*)&environmentMapScale, 4);
	}
	else if (skyrimShaderType == 5 && header->userVersion2 < 130) {
		file.write((char*)&skinTintColor.x, 4);
		file.write((char*)&skinTintColor.y, 4);
		file.write((char*)&skinTintColor.z, 4);
	}
	else if (skyrimShaderType == 6) {
		file.write((char*)&hairTintColor.x, 4);
		file.write((char*)&hairTintColor.y, 4);
		file.write((char*)&hairTintColor.z, 4);
	}
	else if (skyrimShaderType == 7) {
		file.write((char*)&maxPasses, 4);
		file.write((char*)&scale, 4);
	}
	else if (skyrimShaderType == 11) {
		file.write((char*)&parallaxInnerLayerThickness, 4);
		file.write((char*)&parallaxRefractionScale, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.write((char*)&parallaxEnvmapStrength, 4);
	}
	else if (skyrimShaderType == 14) {
		file.write((char*)&sparkleParameters.r, 4);
		file.write((char*)&sparkleParameters.g, 4);
		file.write((char*)&sparkleParameters.b, 4);
		file.write((char*)&sparkleParameters.a, 4);
	}
	else if (skyrimShaderType == 16) {
		file.write((char*)&eyeCubemapScale, 4);
		file.write((char*)&eyeLeftReflectionCenter.x, 4);
		file.write((char*)&eyeLeftReflectionCenter.y, 4);
		file.write((char*)&eyeLeftReflectionCenter.z, 4);
		file.write((char*)&eyeRightReflectionCenter.x, 4);
		file.write((char*)&eyeRightReflectionCenter.y, 4);
		file.write((char*)&eyeRightReflectionCenter.z, 4);
	}


	if (header->userVersion2 >= 130) {
		for (int i = 0; i < 8; i++) {
			file.write((char*)&unk[i], 4);
		}
	}
	if (skyrimShaderType == 1 && header->userVersion2 >= 130) {
		for (int i = 0; i < 2; i++) {
			file.write((char*)&pad[i], 1);
		}
	}
	if (skyrimShaderType == 5 && header->userVersion2 >= 130) {
		for (int i = 0; i < 16; i++) {
			file.write((char*)&pad[i], 1);
		}
	}
}

void BSLightingShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);

	if (textureSetRef == blockID)
		textureSetRef = -1;
	else if (textureSetRef > blockID)
		textureSetRef--;
}


void BSLightingShaderProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (textureSetRef == blockIndexLo)
		textureSetRef = blockIndexHi;
	else if (textureSetRef == blockIndexHi)
		textureSetRef = blockIndexLo;
}

bool BSLightingShaderProperty::IsSkin() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSLightingShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

uint BSLightingShaderProperty::GetType() {
	return skyrimShaderType;
}

void BSLightingShaderProperty::SetType(uint type) {
	skyrimShaderType = type;
}

Vector3 BSLightingShaderProperty::GetSpecularColor() {
	return specularColor;
}

void BSLightingShaderProperty::SetSpecularColor(Vector3 color) {
	specularColor = color;
}

float BSLightingShaderProperty::GetSpecularStrength() {
	return specularStrength;
}

void BSLightingShaderProperty::SetSpecularStrength(float strength) {
	specularStrength = strength;
}

float BSLightingShaderProperty::GetGlossiness() {
	return glossiness;
}

void BSLightingShaderProperty::SetGlossiness(float gloss) {
	glossiness = gloss;
}

int BSLightingShaderProperty::GetTextureSetRef() {
	return textureSetRef;
}

void BSLightingShaderProperty::SetTextureSetRef(int texSetRef) {
	textureSetRef = texSetRef;
}

Color4 BSLightingShaderProperty::GetEmissiveColor() {
	Color4 color;
	color.r = emissiveColor.x;
	color.g = emissiveColor.y;
	color.b = emissiveColor.z;
	return color;
}

void BSLightingShaderProperty::SetEmissiveColor(Color4 color) {
	emissiveColor.x = color.r;
	emissiveColor.y = color.g;
	emissiveColor.z = color.b;
}

float BSLightingShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSLightingShaderProperty::SetEmissiveMultiple(float emissive) {
	emissiveMultiple = emissive;
}

string BSLightingShaderProperty::GetWetMaterialName() {
	return wetMaterialName;
}

void BSLightingShaderProperty::SetWetMaterialName(const string& matName) {
	wetMaterialNameRef = header->AddOrFindStringId(matName);
	wetMaterialName = matName;
}

int BSLightingShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	if (header->userVersion == 12)
		blockSize += 8;

	blockSize += 72;

	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		blockSize += 12;
	}

	if (header->userVersion <= 12 && header->userVersion2 < 130) {
		blockSize += 4;
	}

	if (skyrimShaderType == 1) {
		blockSize += 4;
	}
	else if (skyrimShaderType == 5) {
		if (header->userVersion2 < 130) {
			blockSize += 12;
		}
	}
	else if (skyrimShaderType == 6) {
		blockSize += 12;
	}
	else if (skyrimShaderType == 7){
		blockSize += 8;
	}
	else if (skyrimShaderType == 11){
		blockSize += 20;
	}
	else if (skyrimShaderType == 14){
		blockSize += 16;
	}
	else if (skyrimShaderType == 16){
		blockSize += 28;
	}

	if (header->userVersion2 >= 130) {
		blockSize += 32;
	}

	if (skyrimShaderType == 1 && header->userVersion2 >= 130) {
		blockSize += 2;
	}
	if (skyrimShaderType == 5 && header->userVersion2 >= 130) {
		blockSize += 16;
	}

	return blockSize;
}


void BSShaderProperty::Init() {
	NiProperty::Init();

	smooth = 1;
	shaderType = 1;
	shaderFlags = 0x82000000;
	shaderFlags2 = 1;
	environmentMapScale = 1.0f;
}

void BSShaderProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&smooth, 2);
	file.read((char*)&shaderType, 4);
	file.read((char*)&shaderFlags, 4);
	file.read((char*)&shaderFlags2, 4);

	if (header->userVersion == 11)
		file.read((char*)&environmentMapScale, 4);
}

void BSShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&smooth, 2);
	file.write((char*)&shaderType, 4);
	file.write((char*)&shaderFlags, 4);
	file.write((char*)&shaderFlags2, 4);

	if (header->userVersion == 11)
		file.write((char*)&environmentMapScale, 4);
}

void BSShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}

void BSShaderProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

uint BSShaderProperty::GetType() {
	return shaderType;
}

void BSShaderProperty::SetType(uint type) {
	shaderType = type;
}

int BSShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 14;
	if (header->userVersion == 11)
		blockSize += 4;

	return blockSize;
}


BSEffectShaderProperty::BSEffectShaderProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTY;
	shaderFlags1 = 0;
	shaderFlags2 = 0;
	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;
	textureClampMode = 0;

	falloffStartAngle = 1.0f;
	falloffStopAngle = 1.0f;
	falloffStartOpacity = 0.0f;
	falloffStopOpacity = 0.0f;
	emissiveColor.r = 0.0f;
	emissiveColor.g = 0.0f;
	emissiveColor.b = 0.0f;
	emissiveColor.a = 0.0f;
	emissiveMultiple = 0.0f;
	softFalloffDepth = 0.0f;

	envMapScale = 1.0f;
}

BSEffectShaderProperty::BSEffectShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSEFFECTSHADERPROPERTY;

	Get(file);
}

void BSEffectShaderProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&shaderFlags1, 4);
	file.read((char*)&shaderFlags2, 4);
	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	sourceTexture = NiString(file, 4);
	file.read((char*)&textureClampMode, 4);

	file.read((char*)&falloffStartAngle, 4);
	file.read((char*)&falloffStopAngle, 4);
	file.read((char*)&falloffStartOpacity, 4);
	file.read((char*)&falloffStopOpacity, 4);
	file.read((char*)&emissiveColor, 16);
	file.read((char*)&emissiveMultiple, 4);
	file.read((char*)&softFalloffDepth, 4);
	greyscaleTexture = NiString(file, 4);

	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		envMapTexture = NiString(file, 4);
		normalTexture = NiString(file, 4);
		envMaskTexture = NiString(file, 4);
		file.read((char*)&envMapScale, 4);
	}
}

void BSEffectShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	sourceTexture.Put(file, 4, false);
	file.write((char*)&textureClampMode, 4);

	file.write((char*)&falloffStartAngle, 4);
	file.write((char*)&falloffStopAngle, 4);
	file.write((char*)&falloffStartOpacity, 4);
	file.write((char*)&falloffStopOpacity, 4);
	file.write((char*)&emissiveColor, 16);
	file.write((char*)&emissiveMultiple, 4);
	file.write((char*)&softFalloffDepth, 4);
	greyscaleTexture.Put(file, 4, false);

	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		envMapTexture.Put(file, 4, false);
		normalTexture.Put(file, 4, false);
		envMaskTexture.Put(file, 4, false);
		file.write((char*)&envMapScale, 4);
	}
}

void BSEffectShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}

void BSEffectShaderProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

bool BSEffectShaderProperty::IsSkin() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSEffectShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

Color4 BSEffectShaderProperty::GetEmissiveColor() {
	return emissiveColor;
}

void BSEffectShaderProperty::SetEmissiveColor(Color4 color) {
	emissiveColor = color;
}

float BSEffectShaderProperty::GetEmissiveMultiple() {
	return emissiveMultiple;
}

void BSEffectShaderProperty::SetEmissiveMultiple(float emissive) {
	emissiveMultiple = emissive;
}

int BSEffectShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 76;
	blockSize += sourceTexture.str.length();
	blockSize += greyscaleTexture.str.length();

	if (header->userVersion == 12 && header->userVersion2 >= 130) {
		blockSize += 16;
		blockSize += envMapTexture.str.length();
		blockSize += normalTexture.str.length();
		blockSize += envMaskTexture.str.length();
	}

	return blockSize;
}


void BSShaderLightingProperty::Init() {
	BSShaderProperty::Init();

	textureClampMode = 3;
}

void BSShaderLightingProperty::Get(fstream& file) {
	BSShaderProperty::Get(file);

	if (header->userVersion <= 11)
		file.read((char*)&textureClampMode, 4);
}

void BSShaderLightingProperty::Put(fstream& file) {
	BSShaderProperty::Put(file);

	if (header->userVersion <= 11)
		file.write((char*)&textureClampMode, 4);
}

void BSShaderLightingProperty::notifyBlockDelete(int blockID) {
	BSShaderProperty::notifyBlockDelete(blockID);
}


void BSShaderLightingProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	BSShaderProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int BSShaderLightingProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	if (header->userVersion <= 11)
		blockSize += 4;

	return blockSize;
}


BSShaderPPLightingProperty::BSShaderPPLightingProperty(NiHeader& hdr) {
	BSShaderLightingProperty::Init();

	header = &hdr;
	blockType = BSSHADERPPLIGHTINGPROPERTY;
	textureSetRef = -1;
	refractionStrength = 0.0;
	refractionFirePeriod = 0;
	unkFloat4 = 4.0f;
	unkFloat5 = 1.0f;
	emissiveColor.r = 0.0f;
	emissiveColor.g = 0.0f;
	emissiveColor.b = 0.0f;
	emissiveColor.a = 0.0f;
}

BSShaderPPLightingProperty::BSShaderPPLightingProperty(fstream& file, NiHeader& hdr) {
	BSShaderLightingProperty::Init();

	header = &hdr;
	blockType = BSSHADERPPLIGHTINGPROPERTY;
	refractionStrength = 0.0;
	refractionFirePeriod = 0;
	unkFloat4 = 4.0f;
	unkFloat5 = 1.0f;
	emissiveColor.r = 0.0f;
	emissiveColor.g = 0.0f;
	emissiveColor.b = 0.0f;
	emissiveColor.a = 0.0f;

	Get(file);
}

void BSShaderPPLightingProperty::Get(fstream& file) {
	BSShaderLightingProperty::Get(file);

	file.read((char*)&textureSetRef, 4);
	
	if (header->userVersion == 11) {
		file.read((char*)&refractionStrength, 4);
		file.read((char*)&refractionFirePeriod, 4);
		file.read((char*)&unkFloat4, 4);
		file.read((char*)&unkFloat5, 4);
	}

	if (header->userVersion >= 12) {
		file.read((char*)&emissiveColor.r, 4);
		file.read((char*)&emissiveColor.g, 4);
		file.read((char*)&emissiveColor.b, 4);
		file.read((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::Put(fstream& file) {
	BSShaderLightingProperty::Put(file);

	file.write((char*)&textureSetRef, 4);

	if (header->userVersion == 11) {
		file.write((char*)&refractionStrength, 4);
		file.write((char*)&refractionFirePeriod, 4);
		file.write((char*)&unkFloat4, 4);
		file.write((char*)&unkFloat5, 4);
	}

	if (header->userVersion >= 12) {
		file.write((char*)&emissiveColor.r, 4);
		file.write((char*)&emissiveColor.g, 4);
		file.write((char*)&emissiveColor.b, 4);
		file.write((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::notifyBlockDelete(int blockID) {
	BSShaderLightingProperty::notifyBlockDelete(blockID);

	if (textureSetRef == blockID)
		textureSetRef = -1;
	else if (textureSetRef > blockID)
		textureSetRef--;
}


void BSShaderPPLightingProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	BSShaderLightingProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (textureSetRef == blockIndexLo)
		textureSetRef = blockIndexHi;
	else if (textureSetRef == blockIndexHi)
		textureSetRef = blockIndexLo;
}

bool BSShaderPPLightingProperty::IsSkin() {
	return shaderType == 0x0000000e;
}

int BSShaderPPLightingProperty::GetTextureSetRef() {
	return textureSetRef;
}

void BSShaderPPLightingProperty::SetTextureSetRef(int texSetRef) {
	textureSetRef = texSetRef;
}

int BSShaderPPLightingProperty::CalcBlockSize() {
	BSShaderLightingProperty::CalcBlockSize();

	blockSize += 4;

	if (header->userVersion == 11)
		blockSize += 16;

	if (header->userVersion >= 12)
		blockSize += 16;

	return blockSize;
}


BSShaderTextureSet::BSShaderTextureSet(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSSHADERTEXTURESET;

	if (header->userVersion == 12 && header->userVersion2 >= 130)
		numTextures = 10;
	else if (header->userVersion == 12)
		numTextures = 9;
	else
		numTextures = 6;

	for (int i = 0; i < numTextures; i++)
		textures.push_back(NiString());
}

BSShaderTextureSet::BSShaderTextureSet(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSSHADERTEXTURESET;

	Get(file);
}
	
void BSShaderTextureSet::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numTextures, 4);
	for (int i = 0; i < numTextures; i++)
		textures.push_back(NiString(file, 4));
}
	
void BSShaderTextureSet::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numTextures, 4);
	for (int i = 0; i < numTextures; i++)
		textures[i].Put(file, 4, false);
}

int BSShaderTextureSet::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;
	for (auto &tex : textures) {
		blockSize += 4;
		blockSize += tex.str.length();
	}

	return blockSize;
}


NiAlphaProperty::NiAlphaProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NIALPHAPROPERTY;
	flags = 4844;
	threshold = 128;
}

NiAlphaProperty::NiAlphaProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NIALPHAPROPERTY;

	Get(file);
}

void NiAlphaProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&threshold, 1);
}

void NiAlphaProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&threshold, 1);
}

void NiAlphaProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}


void NiAlphaProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiAlphaProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 3;

	return blockSize;
}


NiMaterialProperty::NiMaterialProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NIMATERIALPROPERTY;
	glossiness = 1.0f;
	alpha = 1.0f;
	emitMulti = 1.0f;
}

NiMaterialProperty::NiMaterialProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NIMATERIALPROPERTY;
	emitMulti = 1.0f;

	Get(file);
}

void NiMaterialProperty::Get(fstream& file) {
	NiProperty::Get(file);

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->userVersion >= 11 && header->userVersion2 > 21)) {
		file.read((char*)&colorAmbient, 12);
		file.read((char*)&colorDiffuse, 12);
	}

	file.read((char*)&colorSpecular, 12);
	file.read((char*)&colorEmissive, 12);
	file.read((char*)&glossiness, 4);
	file.read((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->userVersion >= 11 && header->userVersion2 > 21)
		file.read((char*)&emitMulti, 4);
}

void NiMaterialProperty::Put(fstream& file) {
	NiProperty::Put(file);

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->userVersion >= 11 && header->userVersion2 > 21)) {
		file.write((char*)&colorAmbient, 12);
		file.write((char*)&colorDiffuse, 12);
	}

	file.write((char*)&colorSpecular, 12);
	file.write((char*)&colorEmissive, 12);
	file.write((char*)&glossiness, 4);
	file.write((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->userVersion >= 11 && header->userVersion2 > 21)
		file.write((char*)&emitMulti, 4);
}

void NiMaterialProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}


void NiMaterialProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

Vector3 NiMaterialProperty::GetSpecularColor() {
	return colorSpecular;
}

void NiMaterialProperty::SetSpecularColor(Vector3 color) {
	colorSpecular = color;
}

float NiMaterialProperty::GetGlossiness() {
	return glossiness;
}

void NiMaterialProperty::SetGlossiness(float gloss) {
	glossiness = gloss;
}

Color4 NiMaterialProperty::GetEmissiveColor() {
	Color4 color;
	color.r = colorEmissive.x;
	color.g = colorEmissive.y;
	color.b = colorEmissive.z;
	return color;
}

void NiMaterialProperty::SetEmissiveColor(Color4 color) {
	colorEmissive.x = color.r;
	colorEmissive.y = color.g;
	colorEmissive.z = color.b;
}

float NiMaterialProperty::GetEmissiveMultiple() {
	return emitMulti;
}

void NiMaterialProperty::SetEmissiveMultiple(float emissive) {
	emitMulti = emissive;
}

int NiMaterialProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->userVersion >= 11 && header->userVersion2 > 21))
		blockSize += 24;
	else
		blockSize += 4;

	blockSize += 32;

	return blockSize;
}


NiStencilProperty::NiStencilProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NISTENCILPROPERTY;
	flags = 19840;
	stencilRef = 0;
	stencilMask = 0xffffffff;
}

NiStencilProperty::NiStencilProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = NISTENCILPROPERTY;

	Get(file);
}

void NiStencilProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&stencilRef, 4);
	file.read((char*)&stencilMask, 4);
}

void NiStencilProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&stencilRef, 4);
	file.write((char*)&stencilMask, 4);
}

void NiStencilProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}

void NiStencilProperty::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiProperty::notifyBlockSwap(blockIndexLo, blockIndexHi);
}

int NiStencilProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 10;

	return blockSize;
}


void NiExtraData::Init() {
	NiObject::Init();

	nameRef = 0xFFFFFFFF;
	name.clear();
}

void NiExtraData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nameRef, 4);
	if (nameRef != -1)
		name = header->strings[nameRef].str;
	else
		name.clear();
}

void NiExtraData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nameRef, 4);
}

string NiExtraData::GetName() {
	return name;
}

void NiExtraData::SetName(const string& extraDataName) {
	nameRef = header->AddOrFindStringId(extraDataName);
	name = extraDataName;
}

int NiExtraData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiStringExtraData::NiStringExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NISTRINGEXTRADATA;
	stringDataRef = 0xFFFFFFFF;
	stringData.clear();
}

NiStringExtraData::NiStringExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NISTRINGEXTRADATA;

	Get(file);
}

void NiStringExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&stringDataRef, 4);
	if (stringDataRef != -1)
		stringData = header->strings[stringDataRef].str;
	else
		stringData.clear();
}

void NiStringExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&stringDataRef, 4);
}

string NiStringExtraData::GetStringData() {
	return stringData;
}

void NiStringExtraData::SetStringData(const string& str) {
	stringDataRef = header->AddOrFindStringId(str);
	stringData = str;
}

int NiStringExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiIntegerExtraData::NiIntegerExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIINTEGEREXTRADATA;
	integerData = 0;
}

NiIntegerExtraData::NiIntegerExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIINTEGEREXTRADATA;

	Get(file);
}

void NiIntegerExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&integerData, 4);
}

void NiIntegerExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&integerData, 4);
}

uint NiIntegerExtraData::GetIntegerData() {
	return integerData;
}

void NiIntegerExtraData::SetIntegerData(const uint& integerData) {
	this->integerData = integerData;
}

int NiIntegerExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSXFlags::BSXFlags(NiHeader& hdr) : NiIntegerExtraData(hdr) {
	header = &hdr;
	blockType = BSXFLAGS;
}

BSXFlags::BSXFlags(fstream& file, NiHeader& hdr) : NiIntegerExtraData(file, hdr) {
	header = &hdr;
	blockType = BSXFLAGS;

	Get(file);
}

void BSXFlags::Get(fstream& file) {
}

void BSXFlags::Put(fstream& file) {
	NiIntegerExtraData::Put(file);
}

uint BSXFlags::GetIntegerData() {
	return NiIntegerExtraData::GetIntegerData();
}

void BSXFlags::SetIntegerData(const uint& integerData) {
	NiIntegerExtraData::SetIntegerData(integerData);
}

int BSXFlags::CalcBlockSize() {
	return NiIntegerExtraData::CalcBlockSize();
}


BSConnectPoint::BSConnectPoint() {
	scale = 1.0f;
}

BSConnectPoint::BSConnectPoint(fstream& file) {
	Get(file);
}

void BSConnectPoint::Get(fstream& file) {
	root.Get(file, 4);
	variableName.Get(file, 4);

	file.read((char*)&rotation, 16);
	file.read((char*)&translation, 12);
	file.read((char*)&scale, 4);
}

void BSConnectPoint::Put(fstream& file) {
	root.Put(file, 4);
	variableName.Put(file, 4);

	file.write((char*)&rotation, 16);
	file.write((char*)&translation, 12);
	file.write((char*)&scale, 4);
}

int BSConnectPoint::CalcBlockSize() {
	int blockSize = 40;
	blockSize += root.str.length();
	blockSize += variableName.str.length();
	return blockSize;
}


BSConnectPointParents::BSConnectPointParents(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSCONNECTPOINTPARENTS;
	numConnectPoints = 0;
}

BSConnectPointParents::BSConnectPointParents(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSCONNECTPOINTPARENTS;

	Get(file);
}

void BSConnectPointParents::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numConnectPoints, 4);

	for (int i = 0; i < numConnectPoints; i++)
		connectPoints.push_back(BSConnectPoint(file));
}

void BSConnectPointParents::Put(fstream& file) {
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


BSConnectPointChildren::BSConnectPointChildren(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSCONNECTPOINTCHILDREN;
	unkByte = 1;
	numTargets = 0;
}

BSConnectPointChildren::BSConnectPointChildren(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSCONNECTPOINTCHILDREN;

	Get(file);
}

void BSConnectPointChildren::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&unkByte, 1);
	file.read((char*)&numTargets, 4);

	for (int i = 0; i < numTargets; i++)
		targets.push_back(NiString(file, 4));
}

void BSConnectPointChildren::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&unkByte, 1);
	file.write((char*)&numTargets, 4);

	for (int i = 0; i < numTargets; i++)
		targets[i].Put(file, 4);
}

int BSConnectPointChildren::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 5;

	for (int i = 0; i < numTargets; i++)
		blockSize += targets[i].str.length();

	return blockSize;
}


void BSExtraData::Init() {
	NiObject::Init();
}

void BSExtraData::Get(fstream& file) {
	NiObject::Get(file);
}

void BSExtraData::Put(fstream& file) {
	NiObject::Put(file);
}

int BSExtraData::CalcBlockSize() {
	NiObject::CalcBlockSize();
	return blockSize;
}


BSClothExtraData::BSClothExtraData() {
	BSExtraData::Init();

	blockType = BSCLOTHEXTRADATA;
	numBytes = 0;
	data.clear();
}

BSClothExtraData::BSClothExtraData(NiHeader& hdr, uint size) {
	BSExtraData::Init();

	header = &hdr;
	blockType = BSCLOTHEXTRADATA;
	numBytes = size;
	data.resize(size);
}

BSClothExtraData::BSClothExtraData(fstream& file, NiHeader& hdr) {
	BSExtraData::Init();

	header = &hdr;
	blockType = BSCLOTHEXTRADATA;

	Get(file);
}

void BSClothExtraData::Get(fstream& file) {
	BSExtraData::Get(file);

	file.read((char*)&numBytes, 4);
	data.resize(numBytes);
	if (data.empty())
		return;

	file.read(&data[0], numBytes);
}

void BSClothExtraData::Put(fstream& file) {
	BSExtraData::Put(file);

	file.write((char*)&numBytes, 4);
	if (data.empty())
		return;

	file.write(&data[0], numBytes);
}

void BSClothExtraData::Clone(BSClothExtraData* other) {
	numBytes = other->numBytes;
	data.resize(numBytes);
	if (data.empty())
		return;

	for (int i = 0; i < numBytes; i++)
		data[i] = other->data[i];
}

int BSClothExtraData::CalcBlockSize() {
	BSExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


bool BSClothExtraData::ToHKX(const string& fileName) {
	ofstream file(fileName, ios_base::binary);
	if (!file)
		return false;

	file.write(data.data(), numBytes);
	return true;
}

bool BSClothExtraData::FromHKX(const string& fileName) {
	ifstream file(fileName, ios::binary | ios::ate);
	if (!file)
		return false;

	numBytes = file.tellg();
	file.seekg(0, ios::beg);

	data.resize(numBytes);
	file.read(data.data(), numBytes);
	return true;
}


NiCollisionObject::NiCollisionObject(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NICOLLISIONOBJECT;
	target = 0xFFFFFFFF;
}

NiCollisionObject::NiCollisionObject(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NICOLLISIONOBJECT;

	Get(file);
}

void NiCollisionObject::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&target, 4);
}

void NiCollisionObject::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&target, 4);
}

int NiCollisionObject::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


bhkNiCollisionObject::bhkNiCollisionObject(NiHeader& hdr) : NiCollisionObject(hdr) {
	blockType = BHKNICOLLISIONOBJECT;
	flags = 1;
	body = 0xFFFFFFFF;
}

bhkNiCollisionObject::bhkNiCollisionObject(fstream& file, NiHeader& hdr) : NiCollisionObject(file, hdr) {
	blockType = BHKNICOLLISIONOBJECT;

	Get(file);
}

void bhkNiCollisionObject::Get(fstream& file) {
	file.read((char*)&flags, 2);
	file.read((char*)&body, 4);
}

void bhkNiCollisionObject::Put(fstream& file) {
	NiCollisionObject::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&body, 4);
}

int bhkNiCollisionObject::CalcBlockSize() {
	NiCollisionObject::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


bhkCollisionObject::bhkCollisionObject(NiHeader& hdr) : bhkNiCollisionObject(hdr) {
	blockType = BHKCOLLISIONOBJECT;
}

bhkCollisionObject::bhkCollisionObject(fstream& file, NiHeader& hdr) : bhkNiCollisionObject(file, hdr) {
	blockType = BHKCOLLISIONOBJECT;

	Get(file);
}

void bhkCollisionObject::Get(fstream& file) {
}

void bhkCollisionObject::Put(fstream& file) {
	bhkNiCollisionObject::Put(file);
}

int bhkCollisionObject::CalcBlockSize() {
	return bhkNiCollisionObject::CalcBlockSize();
}


bhkNPCollisionObject::bhkNPCollisionObject(NiHeader& hdr) : bhkCollisionObject(hdr) {
	blockType = BHKNPCOLLISIONOBJECT;
	unkInt = 0;
}

bhkNPCollisionObject::bhkNPCollisionObject(fstream& file, NiHeader& hdr) : bhkCollisionObject(file, hdr) {
	blockType = BHKNPCOLLISIONOBJECT;

	Get(file);
}

void bhkNPCollisionObject::Get(fstream& file) {
	file.read((char*)&unkInt, 4);
}

void bhkNPCollisionObject::Put(fstream& file) {
	bhkCollisionObject::Put(file);

	file.write((char*)&unkInt, 4);
}

int bhkNPCollisionObject::CalcBlockSize() {
	bhkCollisionObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


bhkPhysicsSystem::bhkPhysicsSystem() {
	BSExtraData::Init();

	blockType = BHKPHYSICSSYSTEM;
	numBytes = 0;
	data.clear();
}

bhkPhysicsSystem::bhkPhysicsSystem(NiHeader& hdr, uint size) {
	BSExtraData::Init();

	header = &hdr;
	blockType = BHKPHYSICSSYSTEM;
	numBytes = size;
	data.resize(size);
}

bhkPhysicsSystem::bhkPhysicsSystem(fstream& file, NiHeader& hdr) {
	BSExtraData::Init();

	header = &hdr;
	blockType = BHKPHYSICSSYSTEM;

	Get(file);
}

void bhkPhysicsSystem::Get(fstream& file) {
	BSExtraData::Get(file);

	file.read((char*)&numBytes, 4);
	data.resize(numBytes);
	if (data.empty())
		return;

	file.read(&data[0], numBytes);
}

void bhkPhysicsSystem::Put(fstream& file) {
	BSExtraData::Put(file);

	file.write((char*)&numBytes, 4);
	if (data.empty())
		return;

	file.write(&data[0], numBytes);
}

void bhkPhysicsSystem::Clone(bhkPhysicsSystem* other) {
	numBytes = other->numBytes;
	data.resize(numBytes);
	if (data.empty())
		return;

	for (int i = 0; i < numBytes; i++)
		data[i] = other->data[i];
}

int bhkPhysicsSystem::CalcBlockSize() {
	BSExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


NiUnknown::NiUnknown() {
	NiObject::Init();

	blockType = NIUNKNOWN;
}

NiUnknown::NiUnknown(fstream& file, uint size) {
	NiObject::Init();

	blockType = NIUNKNOWN;
	data.resize(size);

	blockSize = size;
	Get(file);
}

NiUnknown::NiUnknown(uint size) {
	NiObject::Init();

	blockType = NIUNKNOWN;
	data.resize(size);

	blockSize = size;
}

void NiUnknown::Get(fstream& file) {
	if (data.empty())
		return;

	file.read(&data[0], blockSize);
}

void NiUnknown::Put(fstream& file) {
	if (data.empty())
		return;

	file.write(&data[0], blockSize);
}

void NiUnknown::Clone(NiUnknown* other) {
	if (blockSize == other->blockSize)
		for (int i = 0; i < blockSize; i++)
			data[i] = other->data[i];
}

int NiUnknown::CalcBlockSize() {
	return blockSize;
}
