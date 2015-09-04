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

void NiObject::Get(fstream& file) {
}

void NiObject::Put(fstream& file) {
}

int NiObject::CalcBlockSize() {
	blockSize = 0;	// Calculate from the ground up
	return blockSize;
}

bool NiObject::VerCheck(int v1, int v2, int v3, int v4, bool equal) {
	if (equal) {
		if (header->version4 == v1 && header->version3 == v2 && header->version2 == v3 && header->version1 == v4)
			return true;
	}
	else {
		if (header->version4 >= v1 && header->version3 >= v2 && header->version2 >= v3 && header->version1 >= v4)
			return true;
	}

	return false;
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

void NiHeader::Get(fstream& file) {
	file.read(verStr, 38);
	if (_strnicmp(verStr, "Gamebryo", 8) != 0) {
		verStr[0] = 0;
		return;
	}

	//file >> unk1;
	unk1 = 10;
	file >> version1 >> version2 >> version3 >> version4;
	file >> endian;
	file.read((char*)&userVersion, 4);
	file.read((char*)&numBlocks, 4);
	file.read((char*)&userVersion2, 4);
	creator.Get(file, 1);
	exportInfo1.Get(file, 1);
	exportInfo2.Get(file, 1);
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
	file.write((char*)&numBlockTypes, 2);
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes[i].Put(file, 4);

	for (int i = 0; i < numBlocks; i++)
		file.write((char*)&blockIndex[i], 2);

	for (int i = 0; i < numBlocks; i++)
		file.write((char*)&blockSizes[i], 4);

	file.write((char*)&numStrings, 4);
	file.write((char*)&maxStringLen, 4);
	for (int i = 0; i < numStrings; i++)
		strings[i].Put(file, 4);

	file.write((char*)&unkInt2, 4);
}


NiString::NiString(bool wantOutputNull) {
	outputNull = wantOutputNull;
}

NiString::NiString(fstream& file, int szSize, bool wantNullOutput) {
	outputNull = wantNullOutput;
	Get(file, szSize);
}

void NiString::Put(fstream& file, int szSize) {
	if (szSize == 1) {
		byte smSize = str.length();
		if (!smSize && outputNull)
			smSize = 1;
		file.write((char*)&smSize, 1);
	}
	else if (szSize == 2) {
		ushort medSize = str.length();
		if (!medSize && outputNull)
			medSize = 1;
		file.write((char*)&medSize, 2);
	}
	else if (szSize == 4) {
		uint bigSize = str.length();
		if (!bigSize && outputNull)
			bigSize = 1;
		file.write((char*)&bigSize, 4);
	}

	file.write(str.c_str(), str.length());
	if (str.length() == 0 && outputNull)
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
	name = "";
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
		name = "";

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

int NiProperty::CalcBlockSize() {
	return NiObjectNET::CalcBlockSize();
}


void NiAVObject::Init() {
	NiObjectNET::Init();

	flags = 14;
	unkShort1 = 8;
	rotation[0].x = 0.0f;
	rotation[1].y = 0.0f;
	rotation[2].z = 0.0f;
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

	file.read((char*)&numEffects, 4);
	for (int i = 0; i < numEffects; i++) {
		file.read((char*)&intData, 4);
		effects.push_back(intData);
	}
}

void NiNode::Put(fstream& file) {
	NiAVObject::Put(file);

	file.write((char*)&numChildren, 4);
	for (int i = 0; i < numChildren; i++)
		file.write((char*)&children[i], 4);

	file.write((char*)&numEffects, 4);
	for (int i = 0; i < numEffects; i++)
		file.write((char*)&effects[i], 4);
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

int NiNode::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 8;
	blockSize += numChildren * 4;
	blockSize += numEffects * 4;
	return blockSize;
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
	numPartitions = 0;

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

int BSDismemberSkinInstance::CalcBlockSize() {
	NiSkinInstance::CalcBlockSize();

	blockSize += 4;
	blockSize += numPartitions * 4;
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


BSLightingShaderProperty::BSLightingShaderProperty(NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;
	shaderFlags1 = 0x82400303;
	shaderFlags2 = 0x8001;
	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;
	textureSetRef = -1;

	emissiveMultiple = 1.0f;
	textureClampMode = 3;
	alpha = 1.0f;
	refractionStrength = 0.0f;
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
	sparkleParameters.r = 0.0f;
	sparkleParameters.g = 0.0f;
	sparkleParameters.b = 0.0f;
	sparkleParameters.a = 0.0f;
	eyeCubemapScale = 1.0f;
}

BSLightingShaderProperty::BSLightingShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;
	shaderFlags1 = 0x82400303;
	shaderFlags2 = 0x8001;

	environmentMapScale = 1.0f;
	maxPasses = 1.0f;
	scale = 1.0f;
	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	sparkleParameters.r = 0.0f;
	sparkleParameters.g = 0.0f;
	sparkleParameters.b = 0.0f;
	sparkleParameters.a = 0.0f;
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
	file.read((char*)&textureClampMode, 4);
	file.read((char*)&alpha, 4);
	file.read((char*)&refractionStrength, 4);
	file.read((char*)&glossiness, 4);
	file.read((char*)&specularColor.x, 4);
	file.read((char*)&specularColor.y, 4);
	file.read((char*)&specularColor.z, 4);
	file.read((char*)&specularStrength, 4);
	file.read((char*)&lightingEffect1, 4);
	file.read((char*)&lightingEffect2, 4);

	if (skyrimShaderType == 1) {
		file.read((char*)&environmentMapScale, 4);
	}
	else if (skyrimShaderType == 5) {
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
	file.write((char*)&textureClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&refractionStrength, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specularColor.x, 4);
	file.write((char*)&specularColor.y, 4);
	file.write((char*)&specularColor.z, 4);
	file.write((char*)&specularStrength, 4);
	file.write((char*)&lightingEffect1, 4);
	file.write((char*)&lightingEffect2, 4);

	if (skyrimShaderType == 1) {
		file.write((char*)&environmentMapScale, 4);
	}
	else if (skyrimShaderType == 5) {
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
}

void BSLightingShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);

	if (textureSetRef == blockID)
		textureSetRef = -1;
	else if (textureSetRef > blockID)
		textureSetRef--;
}

bool BSLightingShaderProperty::IsSkinShader() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSLightingShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

int BSLightingShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	if (header->userVersion == 12)
		blockSize += 8;

	blockSize += 76;

	if (skyrimShaderType == 1)
		blockSize += 4;
	else if (skyrimShaderType == 5)
		blockSize += 12;
	else if (skyrimShaderType == 6)
		blockSize += 12;
	else if (skyrimShaderType == 7)
		blockSize += 8;
	else if (skyrimShaderType == 11)
		blockSize += 20;
	else if (skyrimShaderType == 14)
		blockSize += 16;
	else if (skyrimShaderType == 16)
		blockSize += 28;

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
	sourceTexture = NiString(false);
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
	greyscaleTexture = NiString(false);
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
	sourceTexture = NiString(file, 4, false);
	file.read((char*)&textureClampMode, 4);

	file.read((char*)&falloffStartAngle, 4);
	file.read((char*)&falloffStopAngle, 4);
	file.read((char*)&falloffStartOpacity, 4);
	file.read((char*)&falloffStopOpacity, 4);
	file.read((char*)&emissiveColor, 16);
	file.read((char*)&emissiveMultiple, 4);
	file.read((char*)&softFalloffDepth, 4);
	greyscaleTexture = NiString(file, 4, false);
}

void BSEffectShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	sourceTexture.Put(file, 4);
	file.write((char*)&textureClampMode, 4);

	file.write((char*)&falloffStartAngle, 4);
	file.write((char*)&falloffStopAngle, 4);
	file.write((char*)&falloffStartOpacity, 4);
	file.write((char*)&falloffStopOpacity, 4);
	file.write((char*)&emissiveColor, 16);
	file.write((char*)&emissiveMultiple, 4);
	file.write((char*)&softFalloffDepth, 4);
	greyscaleTexture.Put(file, 4);
}

void BSEffectShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);
}

bool BSEffectShaderProperty::IsSkinShader() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSEffectShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

int BSEffectShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 76;
	blockSize += sourceTexture.str.length();
	blockSize += greyscaleTexture.str.length();

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
	if (textureSetRef == blockID)
		textureSetRef = -1;
	else if (textureSetRef > blockID)
		textureSetRef--;
}

bool BSShaderPPLightingProperty::IsSkinShader() {
	return shaderType == 0x0000000e;
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

	if (header->userVersion == 12)
		numTextures = 9;
	else
		numTextures = 6;

	for (int i = 0; i < numTextures; i++)
		textures.push_back(NiString(false));
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
		textures.push_back(NiString(file, 4, false));
}
	
void BSShaderTextureSet::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numTextures, 4);
	for (int i = 0; i < numTextures; i++)
		textures[i].Put(file, 4);
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

int NiStencilProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 10;

	return blockSize;
}


void NiExtraData::Init() {
	NiObject::Init();

	nameRef = 0xFFFFFFFF;
	name = "";
}

void NiExtraData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nameRef, 4);
	if (nameRef != -1)
		name = header->strings[nameRef].str;
	else
		name = "";
}

void NiExtraData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nameRef, 4);
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
	stringData = "";
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
		stringData = "";
}

void NiStringExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&stringDataRef, 4);
}

int NiStringExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

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
