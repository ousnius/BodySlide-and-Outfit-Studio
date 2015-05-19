#include "NifFile.h"

NiObject::~NiObject() { 
}

void NiObject::Init() {
	blockSize = 0;
}

void NiObject::notifyBlockDelete(int blockID) {
	return;
}

void NiObject::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	return;
}

void NiObject::Get(fstream& file) {
}

void NiObject::Put(fstream& file) {
}

int NiObject::CalcBlockSize() {
	return blockSize;
}

void NiObject::SetBlockSize(int sz) {
	blockSize = sz;
}

bool NiObject::VerCheck(int v1, int v2, int v3, int v4) {
	if (header->version4 >= v1 && header->version3 >= v2 && header->version2 >= v3 && header->version1 >= v4)
		return true;

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
	byte smSize;
	ushort medSize;
	uint bigSize;
	if (szSize == 1) {
		smSize = str.length();
		if (!smSize && outputNull)
			smSize = 1;
		file.write((char*)&smSize, 1);
	}
	else if (szSize == 2) {
		medSize = str.length();
		if (!medSize && outputNull)
			medSize = 1;
		file.write((char*)&medSize, 2);
	}
	else if (szSize == 4) {
		bigSize = str.length();
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
	byte smSize;
	ushort medSize;
	uint bigSize;

	if (szSize == 1) {
		file.read((char*)&smSize, 1);
		file.read(buf, smSize);
		buf[smSize] = 0;
	}
	else if (szSize == 2) {
		file.read((char*)&medSize, 2);
		if (medSize < 1025)
			file.read(buf, medSize);
		buf[medSize] = 0;
	}
	else if (szSize == 4) {
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

	skyrimShaderType = 0;
	name = "";
	nameRef = -1;
	numExtraData = 0;
	controllerRef = -1;
}

void NiObjectNET::Get(fstream& file) {
	NiObject::Get(file);

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

	blockSize += 12;
	blockSize += numExtraData * 4;
	return blockSize;
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
	blockSize += numProperties * 4;
	if (header->userVersion >= 11 && header->userVersion2 > 26)
		blockSize += 2;

	return blockSize;
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

	CalcBlockSize();
}

NiNode::NiNode(NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NINODE;
	numChildren = 0;
	numEffects = 0;

	CalcBlockSize();
}

NiNode::NiNode(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NINODE;
	numChildren = 0;
	numEffects = 0;

	Get(file);
	CalcBlockSize();
}

int NiNode::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

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
	return NiAVObject::CalcBlockSize();
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


NiTriShape::NiTriShape(NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISHAPE;

	CalcBlockSize();
}

NiTriShape::NiTriShape(fstream& file, NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = NITRISHAPE;

	Get(file);
	CalcBlockSize();
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


void NifBlockTriStrips::Get(fstream& file) {
	file.read((char*)&nameID, 4);
	if (nameID != -1)
		shapeName = header->strings[nameID].str;
	else
		shapeName = "";

	int intData;
	file.read((char*)&numExtra, 4);
	for (int i = 0; i < numExtra; i++) {
		file.read((char*)&intData, 4);
		extradata.push_back(intData);
	}
	file.read((char*)&controllerRef, 4);
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
	if (header->userVersion <= 11) {
		file.read((char*)&numProperties, 4);
		for (int i = 0; i < numProperties; i++) {
			file.read((char*)&intData, 4);
			properties.push_back(intData);
		}
	}
	else
		numProperties = 0;


	file.read((char*)&collisionRef, 4);
	file.read((char*)&dataRef, 4);
	file.read((char*)&skinRef, 4);
	file.read((char*)&numMat, 4);
	for (int i = 0; i < numMat; i++)
		materialNames.push_back(NiString(file, 2));

	for (int i = 0; i < numMat; i++){
		file.read((char*)&intData, 4);
		materialExtra.push_back(intData);
	}
	file.read((char*)&activeMat, 4);

	if (header->userVersion == 1)
		file.read((char*)&unkByte, 1);

	file.read((char*)&dirty, 1);

	if (header->userVersion == 12) {
		file.read((char*)&propertiesRef1, 4);
		file.read((char*)&propertiesRef2, 4);
	}
}

void NifBlockTriStrips::Put(fstream& file) {
	file.write((char*)&nameID, 4);
	file.write((char*)&numExtra, 4);
	for (int i = 0; i < numExtra; i++)
		file.write((char*)&extradata[i], 4);
	file.write((char*)&controllerRef, 4);
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
			file.write((char*)&properties[i], 4);
	}
	file.write((char*)&collisionRef, 4);
	file.write((char*)&dataRef, 4);
	file.write((char*)&skinRef, 4);
	file.write((char*)&numMat, 4);
	for (int i = 0; i < numMat; i++)
		materialNames[i].Put(file, 2);
	for (int i = 0; i < numMat; i++)
		file.write((char*)&materialExtra[i], 4);
	file.write((char*)&activeMat, 4);
	if (header->userVersion == 1)
		file.write((char*)&unkByte, 1);
	file.write((char*)&dirty, 1);
	if (header->userVersion == 12) {
		file.write((char*)&propertiesRef1, 4);
		file.write((char*)&propertiesRef2, 4);
	}
}

void NifBlockTriStrips::notifyBlockDelete(int blockID) {
	for (int i = 0; i < numExtra; i++) {
		if (extradata[i] == blockID) {
			extradata.erase(extradata.begin() + i);
			i--;
			numExtra--;
			blockSize -= 4;
		}
		else if (extradata[i] > blockID)
			extradata[i]--;
	}

	if (controllerRef == blockID)
		controllerRef = -1;
	else if (controllerRef > blockID)
		controllerRef--;

	for (int i = 0; i < numProperties; i++) {
		if (properties[i] == blockID) {
			properties.erase(properties.begin() + i);
			i--;
			numProperties--;
			blockSize -= 4;
		}
		else if (properties[i] > blockID)
			properties[i]--;
	}

	if (collisionRef == blockID)
		collisionRef = -1;
	else if (collisionRef > blockID)
		collisionRef--;

	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinRef == blockID)
		skinRef = -1;
	else if (skinRef > blockID)
		skinRef--;

	if (propertiesRef1 >= blockID)
		propertiesRef1--;
	if (propertiesRef2 >= blockID)
		propertiesRef2--;
}

NifBlockTriStrips::NifBlockTriStrips(NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISTRIPS;
	nameID = -1;
	shapeName = "";
	propertiesRef1 = -1;
	propertiesRef2 = -1;
	controllerRef = -1;
	numExtra = 0;
	scale = 1.0f;
	numProperties = 0;
	flags = 14;
	unkShort1 = 8;
	collisionRef = -1;
	dataRef = -1;
	skinRef = -1;
	numMat = 0;
	activeMat = 0;
	unkByte = 255;
	dirty = 0;
	blockSize = 97;
}

NifBlockTriStrips::NifBlockTriStrips(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISTRIPS;
	propertiesRef1 = -1;
	propertiesRef2 = -1;
	numExtra = 0;
	numProperties = 0;
	Get(file);
}


void NifBlockTriStripsData::Get(fstream& file) {
	file.read((char*)&unkInt, 4);
	file.read((char*)&numverts, 2);
	file.read((char*)&keepflags, 1);
	file.read((char*)&compressflags, 1);
	file.read((char*)&hasVertices, 1);

	Vector3 v;
	for (int i = 0; i < numverts; i++) {
		file.read((char*)&v.x, 4);
		file.read((char*)&v.y, 4);
		file.read((char*)&v.z, 4);
		vertices.push_back(v);
	}
	file.read((char*)&numUVs, 2);
	if (header->userVersion == 12) {
		file.read((char*)&skyrimMaterial, 4);
		myver = 12;
	}
	else
		myver = 11;

	file.read((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&v.x, 4);
			file.read((char*)&v.y, 4);
			file.read((char*)&v.z, 4);
			normals.push_back(v);
		}
		if (numUVs & 0xF000) {
			for (int i = 0; i < numverts; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				tangents.push_back(v);
			}
			for (int i = 0; i < numverts; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				binormals.push_back(v);
			}
		}
	}
	file.read((char*)&Center.x, 4);
	file.read((char*)&Center.y, 4);
	file.read((char*)&Center.z, 4);
	file.read((char*)&Radius, 4);
	file.read((char*)&hasVertColors, 1);

	if (hasVertColors == 1) {
		Color4 c;
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&c, 16);
			vertcolors.push_back(c);
		}
	}
	if (numUVs >= 1) {
		Vector2 uv;
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&uv.u, 4);
			file.read((char*)&uv.v, 4);
			uvs.push_back(uv);
		}
	}
	file.read((char*)&consistencyflags, 2);
	file.read((char*)&additionaldata, 4);
	file.read((char*)&numTriangles, 2);

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

void NifBlockTriStripsData::Put(fstream& file) {
	file.write((char*)&unkInt, 4);
	file.write((char*)&numverts, 2);
	file.write((char*)&keepflags, 1);
	file.write((char*)&compressflags, 1);
	file.write((char*)&hasVertices, 1);
	for (int i = 0; i < numverts; i++) {
		file.write((char*)&vertices[i].x, 4);
		file.write((char*)&vertices[i].y, 4);
		file.write((char*)&vertices[i].z, 4);
	}
	file.write((char*)&numUVs, 2);
	if (header->userVersion == 12)
		file.write((char*)&skyrimMaterial, 4);

	file.write((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numverts; i++) {
			file.write((char*)&normals[i].x, 4);
			file.write((char*)&normals[i].y, 4);
			file.write((char*)&normals[i].z, 4);
		}
		if (numUVs & 0xF000) {
			for (int i = 0; i < numverts; i++) {
				file.write((char*)&tangents[i].x, 4);
				file.write((char*)&tangents[i].y, 4);
				file.write((char*)&tangents[i].z, 4);
			}
			for (int i = 0; i < numverts; i++) {
				file.write((char*)&binormals[i].x, 4);
				file.write((char*)&binormals[i].y, 4);
				file.write((char*)&binormals[i].z, 4);
			}
		}
	}
	file.write((char*)&Center.x, 4);
	file.write((char*)&Center.y, 4);
	file.write((char*)&Center.z, 4);
	file.write((char*)&Radius, 4);
	file.write((char*)&hasVertColors, 1);

	if (hasVertColors == 1) {
		for (int i = 0; i < numverts; i++)
			file.write((char*)&vertcolors[i], 16);
	}

	if (numUVs >= 1) {
		for (int i = 0; i < numverts; i++) {
			file.write((char*)&uvs[i].u, 4);
			file.write((char*)&uvs[i].v, 4);
		}
	}
	file.write((char*)&consistencyflags, 2);
	file.write((char*)&additionaldata, 4);
	file.write((char*)&numTriangles, 2);
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

void NifBlockTriStripsData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertices.size(), 0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = binormals.size() > 0;
	bool hasCol = vertcolors.size() > 0;
	bool hasUV = uvs.size() > 0;

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {  // Found one to remove
			indexCollapse[i] = -1;  // Flag delete
			remCount++;
			j++;
		}
		else {
			indexCollapse[i] = remCount;
		}
	}

	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numverts--;
			blockSize -= sizeof(Vector3);
			if (hasNorm) {
				normals.erase(normals.begin() + i);
				blockSize -= sizeof(Vector3);
			}
			if (hasTan){
				tangents.erase(tangents.begin() + i);
				blockSize -= sizeof(Vector3);
			}
			if (hasBin){
				binormals.erase(binormals.begin() + i);
				blockSize -= sizeof(Vector3);
			}
			if (hasCol) {
				vertcolors.erase(vertcolors.begin() + i);
				blockSize -= sizeof(Color4);
			}
			if (hasUV) {
				uvs.erase(uvs.begin() + i);
				blockSize -= sizeof(Vector2);
			}
		}
	}

	// This is not a healthy way to delete strip data. Probably need to restrip the shape.
	for (int i = 0; i < numStrips; i++) {
		for (int j = 0; j < stripLengths[i]; j++) {
			if (indexCollapse[points[i][j]] == -1) {
				points[i].erase(points[i].begin() + j);
				stripLengths[i]--;
				blockSize -= 2;
			}
		}
	}
}

NifBlockTriStripsData::NifBlockTriStripsData(NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISTRIPSDATA;
	virtScale = 1.0f;
	scaleFromCenter = true;
	myver = 12;
}

NifBlockTriStripsData::NifBlockTriStripsData(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISTRIPSDATA;
	virtScale = 1.0f;
	scaleFromCenter = true;
	myver = 12;
	Get(file);
}

int NifBlockTriStripsData::CalcBlockSize() {
	if (myver == 12)
		blockSize = 44;
	else
		blockSize = 40;

	blockSize += numverts * 12;				//verts
	blockSize += normals.size() * 12;		//normals
	blockSize += tangents.size() * 12;		//tangents
	blockSize += binormals.size() * 12;		//binormals
	blockSize += vertcolors.size() * 16;	//vertcolors
	blockSize += uvs.size() * 8;			//uvs
	blockSize += stripLengths.size() * 2;	//striplengths

	for (auto pl : points)
		blockSize += pl.size() * 2;

	return blockSize;
}

void NifBlockTriStripsData::StripsToTris(vector<Triangle>* outTris) {
	Triangle t;
	for (int strip = 0; strip < numStrips; strip++) {
		for (int vi = 0; vi < stripLengths[strip] - 2; vi++) {
			if (vi & 1) {
				t.p1 = points[strip][vi];
				t.p2 = points[strip][vi + 2];
				t.p3 = points[strip][vi + 1];
			}
			else {
				t.p1 = points[strip][vi];
				t.p2 = points[strip][vi + 1];
				t.p3 = points[strip][vi + 2];
			}

			if (t.p1 == t.p2 || t.p2 == t.p3 || t.p3 == t.p1)
				continue;

			outTris->push_back(t);
		}
	}
}


void NifBlockTriShapeData::Get(fstream& file) {
	if (header->userVersion >= 12)
		myver = 12;
	else
		myver = 11;

	file.read((char*)&unkInt, 4);
	file.read((char*)&numverts, 2);
	file.read((char*)&keepflags, 1);
	file.read((char*)&compressflags, 1);
	file.read((char*)&hasVertices, 1);

	Vector3 v;
	for (int i = 0; i < numverts; i++) {
		file.read((char*)&v.x, 4);
		file.read((char*)&v.y, 4);
		file.read((char*)&v.z, 4);
		vertices.push_back(v);
	}
	file.read((char*)&numUVs, 2);
	if (header->userVersion == 12)
		file.read((char*)&skyrimMaterial, 4);

	file.read((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&v.x, 4);
			file.read((char*)&v.y, 4);
			file.read((char*)&v.z, 4);
			normals.push_back(v);
		}
		if (numUVs & 0xF000) {
			for (int i = 0; i < numverts; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				tangents.push_back(v);
			}
			for (int i = 0; i < numverts; i++) {
				file.read((char*)&v.x, 4);
				file.read((char*)&v.y, 4);
				file.read((char*)&v.z, 4);
				binormals.push_back(v);
			}
		}
	}
	file.read((char*)&Center.x, 4);
	file.read((char*)&Center.y, 4);
	file.read((char*)&Center.z, 4);
	file.read((char*)&Radius, 4);
	file.read((char*)&hasVertColors, 1);

	if (hasVertColors == 1) {
		Color4 c;
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&c, 16);
			vertcolors.push_back(c);
		}
	}
	if (numUVs >= 1) {
		Vector2 uv;
		for (int i = 0; i < numverts; i++) {
			file.read((char*)&uv.u, 4);
			file.read((char*)&uv.v, 4);
			uvs.push_back(uv);
		}
	}
	file.read((char*)&consistencyflags, 2);
	file.read((char*)&additionaldata, 4);
	file.read((char*)&numTriangles, 2);
	file.read((char*)&numTriPoints, 4);
	file.read((char*)&hasTriangles, 1);
	if (hasTriangles == 1) {
		Triangle Triangle;
		for (int i = 0; i < numTriangles; i++) {
			file.read((char*)&Triangle.p1, 2);
			file.read((char*)&Triangle.p2, 2);
			file.read((char*)&Triangle.p3, 2);
			tris.push_back(Triangle);
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

void NifBlockTriShapeData::Put(fstream& file) {
	file.write((char*)&unkInt, 4);
	file.write((char*)&numverts, 2);
	file.write((char*)&keepflags, 1);
	file.write((char*)&compressflags, 1);
	file.write((char*)&hasVertices, 1);
	for (int i = 0; i < numverts; i++) {
		file.write((char*)&vertices[i].x, 4);
		file.write((char*)&vertices[i].y, 4);
		file.write((char*)&vertices[i].z, 4);
	}
	file.write((char*)&numUVs, 2);
	if (header->userVersion == 12)
		file.write((char*)&skyrimMaterial, 4);

	file.write((char*)&hasNormals, 1);
	if (hasNormals == 1) {
		for (int i = 0; i < numverts; i++) {
			file.write((char*)&normals[i].x, 4);
			file.write((char*)&normals[i].y, 4);
			file.write((char*)&normals[i].z, 4);
		}
		if (numUVs & 0xF000) {
			for (int i = 0; i < numverts; i++) {
				file.write((char*)&tangents[i].x, 4);
				file.write((char*)&tangents[i].y, 4);
				file.write((char*)&tangents[i].z, 4);
			}
			for (int i = 0; i < numverts; i++) {
				file.write((char*)&binormals[i].x, 4);
				file.write((char*)&binormals[i].y, 4);
				file.write((char*)&binormals[i].z, 4);
			}
		}
	}
	file.write((char*)&Center.x, 4);
	file.write((char*)&Center.y, 4);
	file.write((char*)&Center.z, 4);
	file.write((char*)&Radius, 4);
	file.write((char*)&hasVertColors, 1);

	if (hasVertColors == 1) {
		for (int i = 0; i < numverts; i++)
			file.write((char*)&vertcolors[i], 16);
	}
	if (numUVs >= 1) {
		for (int i = 0; i < numverts; i++) {
			file.write((char*)&uvs[i].u, 4);
			file.write((char*)&uvs[i].v, 4);
		}
	}
	file.write((char*)&consistencyflags, 2);
	file.write((char*)&additionaldata, 4);
	file.write((char*)&numTriangles, 2);
	file.write((char*)&numTriPoints, 4);
	file.write((char*)&hasTriangles, 1);
	if (hasTriangles == 1) {
		for (int i = 0; i < numTriangles; i++) {
			file.write((char*)&tris[i].p1, 2);
			file.write((char*)&tris[i].p2, 2);
			file.write((char*)&tris[i].p3, 2);
		}
	}
	file.write((char*)&numMatchGroups, 2);
	for (int i = 0; i < numMatchGroups; i++) {
		file.write((char*)&matchGroups[i].count, 2);
		for (int j = 0; j < matchGroups[i].count; j++)
			file.write((char*)&matchGroups[i].matches[j], 2);
	}
}

void NifBlockTriShapeData::Create(vector<Vector3>* verts, vector<Triangle>* inTris, vector<Vector2>* texcoords) {
	unkInt = 0;
	Vector3 a(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector3 b(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	Radius = 0.0f;
	numverts = verts->size();
	keepflags = 0;
	compressflags = 0;
	hasVertices = true;
	for (auto v : (*verts)) {
		vertices.push_back(v);
		a.x = min(a.x, v.x);
		a.y = min(a.y, v.y);
		a.z = min(a.z, v.z);
		b.x = max(b.x, v.x);
		b.y = max(b.y, v.y);
		b.z = max(b.z, v.z);
	}
	if (texcoords->size() > 0)
		numUVs = 4097;
	else
		numUVs = 0;

	skyrimMaterial = 0;
	hasNormals = false;
	Center = (a + b) / 2.0f;
	for (auto v : (*verts))
		Radius = max(Radius, Center.DistanceTo(v));

	hasVertColors = false;

	for (auto uv : (*texcoords))
		uvs.push_back(uv);

	consistencyflags = 16384;
	additionaldata = -1;
	numTriangles = inTris->size();
	numTriPoints = numTriangles * 3;
	hasTriangles = true;
	for (auto t : (*inTris))
		tris.push_back(t);

	numMatchGroups = 0;

	blockSize = 48;
	blockSize += 12 * numverts;  // vert data size
	if (texcoords->size() > 0)
		blockSize += 8 * numverts;	 // uv data size

	blockSize += 6 * numTriangles; // Triangle data size
}

void NifBlockTriShapeData::RecalcNormals() {
	if (!hasNormals)
		return;

	for (int i = 0; i < numverts; i++) {
		normals[i].x = 0;
		normals[i].y = 0;
		normals[i].z = 0;
	}

	Vector3 norm;
	for (int i = 0; i < numTriangles; i++) {
		// Calc normal
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
	for (int i = 0; i < numverts; i++)
		normals[i].Normalize();

	Vertex* matchverts = new Vertex[numverts];

	for (int i = 0; i < numverts; i++) {
		matchverts[i].x = vertices[i].x;
		matchverts[i].nx = normals[i].x;
		matchverts[i].y = vertices[i].y;
		matchverts[i].ny = normals[i].y;
		matchverts[i].z = vertices[i].z;
		matchverts[i].nz = normals[i].z;
	}

	kd_matcher matcher(matchverts, numverts);
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

	for (int i = 0; i < numverts; i++) {
		normals[i].x = matchverts[i].nx;
		normals[i].y = matchverts[i].ny;
		normals[i].z = matchverts[i].nz;
	}

	delete[] matchverts;
}

void NifBlockTriShapeData::CalcTangentSpace() {
	if (!hasNormals)
		return;

	numUVs |= 4096;

	blockSize -= tangents.size() * 12;
	blockSize -= binormals.size() * 12;
	tangents.resize(numverts);
	binormals.resize(numverts);
	blockSize += tangents.size() * 12;
	blockSize += binormals.size() * 12;

	vector<Vector3> tan1;
	vector<Vector3> tan2;
	tan1.resize(numverts);
	tan2.resize(numverts);

	for (int i = 0; i < numTriangles; i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvs[i1];
		Vector2 w2 = uvs[i2];
		Vector2 w3 = uvs[i3];

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

	for (int i = 0; i < numverts; i++) {
		Vector3 n = normals[i];
		Vector3 t = tan1[i];

		// Gram-Schmidt orthogonalize
		// Vector3.OrthoNormalize(ref n, ref t);
		binormals[i].x = t.x;
		binormals[i].y = t.y;
		binormals[i].z = t.z;
		binormals[i].Normalize();

		tangents[i] = binormals[i].cross(normals[i]);
		tangents[i].Normalize();
	}
	//theMesh.tangents = tangents; 
}

void NifBlockTriShapeData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<int> indexCollapse(vertices.size(), 0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = binormals.size() > 0;
	bool hasCol = vertcolors.size() > 0;
	bool hasUV = uvs.size() > 0;

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

	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numverts--;
			blockSize -= 12;
			if (hasNorm) {
				normals.erase(normals.begin() + i);
				blockSize -= 12;
			}
			if (hasTan){
				tangents.erase(tangents.begin() + i);
				blockSize -= 12;
			}
			if (hasBin){
				binormals.erase(binormals.begin() + i);
				blockSize -= 12;
			}
			if (hasCol) {
				vertcolors.erase(vertcolors.begin() + i);
				blockSize -= 16;
			}
			if (hasUV) {
				uvs.erase(uvs.begin() + i);
				blockSize -= 8;
			}
		}

	}
	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapse[tris[i].p1] == -1 || indexCollapse[tris[i].p2] == -1 || indexCollapse[tris[i].p3] == -1) {
			tris.erase(tris.begin() + i);
			numTriangles--;
			numTriPoints -= 3;
			blockSize -= 6;
		}
		else {
			tris[i].p1 = tris[i].p1 - indexCollapse[tris[i].p1];
			tris[i].p2 = tris[i].p2 - indexCollapse[tris[i].p2];
			tris[i].p3 = tris[i].p3 - indexCollapse[tris[i].p3];
		}
	}
}

int NifBlockTriShapeData::CalcBlockSize() {
	if (myver == 12)
		blockSize = 48;
	else
		blockSize = 44;

	blockSize += numverts * 12;	//verts
	blockSize += normals.size() * 12;	//normals
	blockSize += tangents.size() * 12;	//tangents
	blockSize += binormals.size() * 12;	//binormals
	blockSize += vertcolors.size() * 16;  //vertcolors
	blockSize += uvs.size() * 8;	//uvs 
	blockSize += tris.size() * 6;	//tris
	for (auto mg : matchGroups) {
		blockSize += 4;
		blockSize += mg.count * 2;
	}

	return blockSize;
}

NifBlockTriShapeData::NifBlockTriShapeData(NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISHAPEDATA;
	scaleFromCenter = true;
	virtScale = 1.0f;
	numverts = 0;
	numUVs = 0;
	numTriangles = 0;
	numTriPoints = 0;
	numMatchGroups = 0;
	myver = 12;
}

NifBlockTriShapeData::NifBlockTriShapeData(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NITRISHAPEDATA;
	scaleFromCenter = true;
	virtScale = 1.0f;
	numverts = 0;
	numUVs = 0;
	numTriangles = 0;
	numTriPoints = 0;
	numMatchGroups = 0;
	Get(file);
}
 
void NifBlockTriStripsData::RecalcNormals() {
	if (!hasNormals)
		return;

	for (int i = 0; i < numverts; i++) {
		normals[i].x = 0;
		normals[i].y = 0;
		normals[i].z = 0;
	}

	vector<Triangle> tris;
	StripsToTris(&tris);

	Vector3 norm;
	for (int i = 0; i < tris.size(); i++) {
		// Calc normal
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
	for (int i = 0; i < numverts; i++)
		normals[i].Normalize();

	Vertex* matchverts = new Vertex[numverts];

	for (int i = 0; i < numverts; i++) {
		matchverts[i].x = vertices[i].x;
		matchverts[i].nx = normals[i].x;
		matchverts[i].y = vertices[i].y;
		matchverts[i].ny = normals[i].y;
		matchverts[i].z = vertices[i].z;
		matchverts[i].nz = normals[i].z;
	}

	kd_matcher matcher(matchverts, numverts);
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

	for (int i = 0; i < numverts; i++) {
		normals[i].x = matchverts[i].nx;
		normals[i].y = matchverts[i].ny;
		normals[i].z = matchverts[i].nz;
	}

	delete[] matchverts;
}

void NifBlockTriStripsData::CalcTangentSpace() {
	if (!hasNormals)
		return;

	numUVs |= 4096;

	blockSize -= tangents.size() * 12;
	blockSize -= binormals.size() * 12;
	tangents.resize(numverts);
	binormals.resize(numverts);
	blockSize += tangents.size() * 12;
	blockSize += binormals.size() * 12;

	vector<Vector3> tan1;
	vector<Vector3> tan2;
	tan1.resize(numverts);
	tan2.resize(numverts);

	vector<Triangle> tris;
	StripsToTris(&tris);

	for (int i = 0; i < tris.size(); i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvs[i1];
		Vector2 w2 = uvs[i2];
		Vector2 w3 = uvs[i3];

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

	for (int i = 0; i < numverts; i++) {
		Vector3 n = normals[i];
		Vector3 t = tan1[i];

		// Gram-Schmidt orthogonalize
		// Vector3.OrthoNormalize(ref n, ref t);
		binormals[i].x = t.x;
		binormals[i].y = t.y;
		binormals[i].z = t.z;
		binormals[i].Normalize();

		tangents[i] = binormals[i].cross(normals[i]);
		tangents[i].Normalize();
	}
	//theMesh.tangents = tangents; 
}


void NifBlockNiSkinData::Get(fstream& file) {
	file.read((char*)&rootTransform, 52);
	file.read((char*)&numBones, 4);
	file.read((char*)&hasVertWeights, 1);

	SkinWeight sWt;
	BoneData boneData;
	for (int i = 0; i < numBones; i++) {
		boneData.skin_weights.clear();

		file.read((char*)&boneData.boneTransform, 52);
		file.read((char*)&boneData.boundSphereOffset, 12);
		file.read((char*)&boneData.boundSphereRadius, 4);
		file.read((char*)&boneData.numVerts, 2);

		for (int j = 0; j < boneData.numVerts; j++) {
			file.read((char*)&sWt.index, 2);
			file.read((char*)&sWt.weight, 4);
			boneData.skin_weights.push_back(sWt);
		}

		Bones.push_back(boneData);
	}
}

void NifBlockNiSkinData::Put(fstream& file) {
	file.write((char*)&rootTransform, 52);
	file.write((char*)&numBones, 4);
	file.write((char*)&hasVertWeights, 1);

	for (int i = 0; i < numBones; i++) {
		file.write((char*)&Bones[i].boneTransform, 52);
		file.write((char*)&Bones[i].boundSphereOffset, 12);
		file.write((char*)&Bones[i].boundSphereRadius, 4);
		file.write((char*)&Bones[i].numVerts, 2);

		for (int j = 0; j < Bones[i].numVerts; j++) {
			file.write((char*)&Bones[i].skin_weights[j].index, 2);
			file.write((char*)&Bones[i].skin_weights[j].weight, 4);
		}
	}
}

void NifBlockNiSkinData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	ushort highestRemoved = vertIndices[vertIndices.size() - 1];
	vector<int> indexCollapse(highestRemoved + 1, 0);

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
	for (auto b = Bones.begin(); b != Bones.end(); ++b) {
		for (int i = b->numVerts - 1; i >= 0; i--) {
			ival = b->skin_weights[i].index;
			if (b->skin_weights[i].index > highestRemoved) {
				b->skin_weights[i].index -= remCount;
			}
			else
				if (indexCollapse[ival] == -1) {
					b->skin_weights.erase(b->skin_weights.begin() + i);
					b->numVerts--;
					blockSize -= 6;
				}
				else {
					b->skin_weights[i].index -= indexCollapse[ival];
				}
		}
	}
}

NifBlockNiSkinData::NifBlockNiSkinData(NiHeader& hdr) {
	header = &hdr;
	blockType = NISKINDATA;
	rootTransform.scale = 1.0f;
	numBones = 0;
	hasVertWeights = 1;
	blockSize = 57;
}

NifBlockNiSkinData::NifBlockNiSkinData(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NISKINDATA;
	Get(file);
}
	
int NifBlockNiSkinData::CalcBlockSize() {
	numBones = Bones.size();
	blockSize = 57;
	for (auto bd : Bones)
		blockSize += bd.CalcSize();

	return blockSize;
}


int NifBlockNiSkinPartition::CalcBlockSize() {
	blockSize = 4;
	for (auto p : partitionBlocks) {
		if (myver >= 12)							// plain data size
			blockSize += 16;
		else
			blockSize += 14;
		blockSize += 2 * p.numBones;				// bonelist size
		blockSize += 2 * p.numVertices;				// vertmap size
		blockSize += 4 * p.numVertices;				// boneindex size
		blockSize += 16 * p.numVertices;			// vertexWeights size
		blockSize += 2 * p.numStrips;				// striplengths size
		for (int i = 0; i < p.numStrips; i++)
			blockSize += 2 * p.stripLengths[i];		// striplist size

		if (p.numStrips == 0)
			blockSize += 6 * p.numTriangles;		// Triangle list size
	}
	return blockSize;
}

void NifBlockNiSkinPartition::Get(fstream& file) {
	PartitionBlock partBlock;
	ushort uShort;
	VertexWeight wt;
	BoneIndices bi;
	Triangle Triangle;

	file.read((char*)&numPartitions, 4);
	for (int p = 0; p < numPartitions; p++) {
		partBlock.bones.clear();
		partBlock.vertMap.clear();
		partBlock.vertWeights.clear();
		partBlock.stripLengths.clear();
		partBlock.strips.clear();
		partBlock.tris.clear();
		partBlock.boneindex.clear();

		file.read((char*)&partBlock.numVertices, 2);
		file.read((char*)&partBlock.numTriangles, 2);
		file.read((char*)&partBlock.numBones, 2);
		file.read((char*)&partBlock.numStrips, 2);
		file.read((char*)&partBlock.numWeightsPerVert, 2);
		for (int i = 0; i < partBlock.numBones; i++) {
			file.read((char*)&uShort, 2);
			partBlock.bones.push_back(uShort);
		}
		file.read((char*)&partBlock.hasVertMap, 1);
		for (int i = 0; (i < partBlock.numVertices) && partBlock.hasVertMap; i++) {
			file.read((char*)&uShort, 2);
			partBlock.vertMap.push_back(uShort);
		}
		file.read((char*)&partBlock.hasVertWeights, 1);
		for (int i = 0; (i < partBlock.numVertices) && partBlock.hasVertWeights; i++) {
			file.read((char*)&wt.w1, 4);
			file.read((char*)&wt.w2, 4);
			file.read((char*)&wt.w3, 4);
			file.read((char*)&wt.w4, 4);
			partBlock.vertWeights.push_back(wt);
		}
		for (int i = 0; i < partBlock.numStrips; i++) {
			file.read((char*)&uShort, 2);
			partBlock.stripLengths.push_back(uShort);
		}
		file.read((char*)&partBlock.hasFaces, 1);
		for (int i = 0; (i < partBlock.numStrips) && partBlock.hasFaces; i++) {
			partBlock.strips.push_back(vector<ushort>());
			for (int j = 0; j < partBlock.stripLengths[i]; j++) {
				file.read((char*)&uShort, 2);
				partBlock.strips[i].push_back(uShort);
			}
		}
		if (partBlock.numStrips == 0) {
			for (int i = 0; (i < partBlock.numTriangles) && partBlock.hasFaces; i++) {
				file.read((char*)&Triangle.p1, 2);
				file.read((char*)&Triangle.p2, 2);
				file.read((char*)&Triangle.p3, 2);
				partBlock.tris.push_back(Triangle);
			}
		}
		file.read((char*)&partBlock.hasBoneIndices, 1);
		for (int i = 0; i < partBlock.numVertices && partBlock.hasBoneIndices; i++) {
			file.read((char*)&bi.i1, 1);
			file.read((char*)&bi.i2, 1);
			file.read((char*)&bi.i3, 1);
			file.read((char*)&bi.i4, 1);
			partBlock.boneindex.push_back(bi);
		}
		if (header->userVersion >= 12) {
			file.read((char*)&partBlock.unknown, 2);
			myver = 12;
		}
		else
			myver = 11;
		partitionBlocks.push_back(partBlock);
	}
}

void NifBlockNiSkinPartition::Put(fstream& file) {
	file.write((char*)&numPartitions, 4);
	for (int p = 0; p < numPartitions; p++) {
		file.write((char*)&partitionBlocks[p].numVertices, 2);
		file.write((char*)&partitionBlocks[p].numTriangles, 2);
		file.write((char*)&partitionBlocks[p].numBones, 2);
		file.write((char*)&partitionBlocks[p].numStrips, 2);
		file.write((char*)&partitionBlocks[p].numWeightsPerVert, 2);
		for (int i = 0; i < partitionBlocks[p].numBones; i++)
			file.write((char*)&partitionBlocks[p].bones[i], 2);

		file.write((char*)&partitionBlocks[p].hasVertMap, 1);
		for (int i = 0; (i < partitionBlocks[p].numVertices) && partitionBlocks[p].hasVertMap; i++)
			file.write((char*)&partitionBlocks[p].vertMap[i], 2);

		file.write((char*)&partitionBlocks[p].hasVertWeights, 1);
		for (int i = 0; (i < partitionBlocks[p].numVertices) && partitionBlocks[p].hasVertWeights; i++)
			file.write((char*)&partitionBlocks[p].vertWeights[i], 16);

		for (int i = 0; i < partitionBlocks[p].numStrips; i++)
			file.write((char*)&partitionBlocks[p].stripLengths[i], 2);

		file.write((char*)&partitionBlocks[p].hasFaces, 1);
		for (int i = 0; (i < partitionBlocks[p].numStrips) && partitionBlocks[p].hasFaces; i++)
			for (int j = 0; j < partitionBlocks[p].stripLengths[i]; j++)
				file.write((char*)&partitionBlocks[p].strips[i][j], 2);

		if (partitionBlocks[p].numStrips == 0) {
			for (int i = 0; (i < partitionBlocks[p].numTriangles) && partitionBlocks[p].hasFaces; i++)
				file.write((char*)&partitionBlocks[p].tris[i].p1, 6);
		}

		file.write((char*)&partitionBlocks[p].hasBoneIndices, 1);
		for (int i = 0; i < partitionBlocks[p].numVertices && partitionBlocks[p].hasBoneIndices; i++)
			file.write((char*)&partitionBlocks[p].boneindex[i], 4);

		if (header->userVersion >= 12)
			file.write((char*)&partitionBlocks[p].unknown, 2);
	}
}

void NifBlockNiSkinPartition::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	ushort highestRemoved = vertIndices.back();
	vector<int> indexCollapse(highestRemoved + 1, 0);

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
	size_t maxVertMapSz = 0;
	for (auto p = partitionBlocks.begin(); p != partitionBlocks.end(); ++p)
		if (p->vertMap.size() > maxVertMapSz)
			maxVertMapSz = p->vertMap.size();

	vector<int> mapCollapse(maxVertMapSz, 0);
	int mapRemCount = 0;
	int mapHighestRemoved = maxVertMapSz;

	for (auto p = partitionBlocks.begin(); p != partitionBlocks.end(); ++p) {
		mapRemCount = 0;
		mapHighestRemoved = maxVertMapSz;
		for (int i = 0; i < p->vertMap.size(); i++) {
			if (p->vertMap[i] < indexCollapse.size() && indexCollapse[p->vertMap[i]] == -1) {
				mapRemCount++;
				mapCollapse[i] = -1;
				mapHighestRemoved = i;
			}
			else {
				mapCollapse[i] = mapRemCount;
			}
		}
		for (int i = p->vertMap.size() - 1; i >= 0; i--) {
			ival = p->vertMap[i];
			if (ival > highestRemoved) {
				p->vertMap[i] -= remCount;
			}
			else
				if (indexCollapse[ival] == -1) {
					p->vertMap.erase(p->vertMap.begin() + i);
					p->numVertices--;
					blockSize -= 2;
					if (p->hasVertWeights) {
						p->vertWeights.erase(p->vertWeights.begin() + i);
						blockSize -= 16;
					} if (p->hasBoneIndices)
						p->boneindex.erase(p->boneindex.begin() + i);
					blockSize -= 4;
				}
				else {
					p->vertMap[i] -= indexCollapse[ival];
				}
		}

		for (int i = p->numTriangles - 1; i >= 0; i--) {
			if (p->tris[i].p1 > mapHighestRemoved) {
				p->tris[i].p1 -= mapRemCount;
			}
			else
				if (mapCollapse[p->tris[i].p1] == -1) {
					p->tris.erase(p->tris.begin() + i);
					p->numTriangles--;
					blockSize -= 6;
					continue;
				}
				else {
					p->tris[i].p1 -= mapCollapse[p->tris[i].p1];
				}
				if (p->tris[i].p2 > mapHighestRemoved) {
					p->tris[i].p2 -= mapRemCount;
				}
				else
					if (mapCollapse[p->tris[i].p2] == -1) {
						p->tris.erase(p->tris.begin() + i);
						p->numTriangles--;
						blockSize -= 6;
						continue;
					}
					else {
						p->tris[i].p2 -= mapCollapse[p->tris[i].p2];
					}
					if (p->tris[i].p3 > mapHighestRemoved) {
						p->tris[i].p3 -= mapRemCount;
					}
					else
						if (mapCollapse[p->tris[i].p3] == -1) {
							p->tris.erase(p->tris.begin() + i);
							p->numTriangles--;
							blockSize -= 6;
							continue;
						}
						else {
							p->tris[i].p3 -= mapCollapse[p->tris[i].p3];
						}
		}
	}
}

int NifBlockNiSkinPartition::RemoveEmptyPartitions(vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = partitionBlocks.size() - 1; i >= 0; i--) {
		if (partitionBlocks[i].numVertices == 0) {
			if (myver >= 12)
				blockSize -= (16 + 2 * partitionBlocks[i].numBones);
			else
				blockSize -= (14 + 2 * partitionBlocks[i].numBones);
			outDeletedIndices.insert(outDeletedIndices.begin(), i);
			partitionBlocks.erase(partitionBlocks.begin() + i);
			numPartitions--;
		}
	}
	return outDeletedIndices.size();
}

NifBlockNiSkinPartition::NifBlockNiSkinPartition(NiHeader& hdr) {
	header = &hdr;
	blockType = NISKINPARTITION;
	numPartitions = 0;
	blockSize = 4;
	myver = 12;
	needsBuild = true;
}

NifBlockNiSkinPartition::NifBlockNiSkinPartition(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NISKINPARTITION;
	myver = 12;
	Get(file);
	needsBuild = false;
}


void NifBlockNiSkinInstance::Get(fstream& file) {
	file.read((char*)&dataRef, 4);
	file.read((char*)&skinRef, 4);
	file.read((char*)&skeletonRoot, 4);
	file.read((char*)&numBones, 4);

	uint uInt;
	for (int i = 0; i < numBones; i++) {
		file.read((char*)&uInt, 4);
		bonePtrs.push_back(uInt);
	}
}

void NifBlockNiSkinInstance::Put(fstream& file) {
	file.write((char*)&dataRef, 4);
	file.write((char*)&skinRef, 4);
	file.write((char*)&skeletonRoot, 4);
	file.write((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++)
		file.write((char*)&bonePtrs[i], 4);
}

NifBlockNiSkinInstance::NifBlockNiSkinInstance(NiHeader& hdr) {
	header = &hdr;
	blockType = NISKININSTANCE;
	dataRef = -1;
	skinRef = -1;
	skeletonRoot = -1;
	numBones = 0;
	blockSize = 16;
}

NifBlockNiSkinInstance::NifBlockNiSkinInstance(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NISKININSTANCE;
	Get(file);
}

int NifBlockNiSkinInstance::CalcBlockSize() {
	blockSize = 16;
	blockSize += numBones * 4;
	return blockSize;
}

void NifBlockNiSkinInstance::notifyBlockDelete(int blockID) {
	int boneIndex = -1;
	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinRef == blockID)
		skinRef = -1;
	else if (skinRef > blockID)
		skinRef--;

	if (skeletonRoot == blockID)
		skeletonRoot = -1;
	else if (skeletonRoot > blockID)
		skeletonRoot--;

	for (int i = 0; i < numBones; i++) {
		if (bonePtrs[i] == blockID) {
			bonePtrs.erase(bonePtrs.begin() + i);
			i--;
			boneIndex = i;
			numBones--;
			blockSize -= 4;
		}
		else if (bonePtrs[i] > blockID)
			bonePtrs[i]--;
	}
	if (boneIndex >= 0 && dataRef != -1) { // Bone was removed, clear out the skinning data for it.
		if (header->blocks == nullptr)
			return;

		NifBlockNiSkinData* skinData = dynamic_cast<NifBlockNiSkinData*>((*header->blocks)[dataRef]);
		if (!skinData)
			return;

		skinData->Bones.erase(skinData->Bones.begin() + boneIndex);
		skinData->CalcBlockSize();
	}
}


void NifBlockBSDismemberment::Get(fstream& file) {
	file.read((char*)&dataRef, 4);
	file.read((char*)&skinRef, 4);
	file.read((char*)&skeletonRoot, 4);
	file.read((char*)&numBones, 4);

	uint uInt;
	for (int i = 0; i < numBones; i++) {
		file.read((char*)&uInt, 4);
		bonePtrs.push_back(uInt);
	}

	Partition part;
	file.read((char*)&numPartitions, 4);
	for (int i = 0; i < numPartitions; i++) {
		file.read((char*)&part.flags, 2);
		file.read((char*)&part.partID, 2);
		partitions.push_back(part);
	}
}

void NifBlockBSDismemberment::Put(fstream& file) {
	file.write((char*)&dataRef, 4);
	file.write((char*)&skinRef, 4);
	file.write((char*)&skeletonRoot, 4);
	file.write((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++)
		file.write((char*)&bonePtrs[i], 4);

	file.write((char*)&numPartitions, 4);
	for (int i = 0; i < numPartitions; i++) {
		file.write((char*)&partitions[i].flags, 2);
		file.write((char*)&partitions[i].partID, 2);
	}
}

NifBlockBSDismemberment::NifBlockBSDismemberment(NiHeader& hdr) {
	header = &hdr;
	blockType = BSDISMEMBERSKININSTANCE;
	dataRef = -1;
	skinRef = -1;
	skeletonRoot = -1;
	numBones = 0;
	numPartitions = 0;
	blockSize = 20;
}

NifBlockBSDismemberment::NifBlockBSDismemberment(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = BSDISMEMBERSKININSTANCE;
	Get(file);
}

int NifBlockBSDismemberment::CalcBlockSize() {
	blockSize = 20;
	blockSize += numBones * 4;
	blockSize += numPartitions * 4;
	return blockSize;
}

void NifBlockBSDismemberment::notifyBlockDelete(int blockID) {
	int boneIndex = -1;
	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinRef == blockID)
		skinRef = -1;
	else if (skinRef > blockID)
		skinRef--;

	if (skeletonRoot == blockID)
		skeletonRoot = -1;
	else if (skeletonRoot > blockID)
		skeletonRoot--;

	for (int i = 0; i < numBones; i++) {
		if (bonePtrs[i] == blockID) {
			bonePtrs.erase(bonePtrs.begin() + i);
			boneIndex = i;
			i--;
			numBones--;
			blockSize -= 4;
		}
		else if (bonePtrs[i] > blockID)
			bonePtrs[i]--;
	}
	if (boneIndex >= 0 && dataRef != -1) { // Bone was removed, clear out the skinning data for it.
		if (!header->blocks)
			return;

		NifBlockNiSkinData* skinData = dynamic_cast<NifBlockNiSkinData*>((*header->blocks)[dataRef]);
		if (!skinData)
			return;

		skinData->Bones.erase(skinData->Bones.begin() + boneIndex);
		skinData->CalcBlockSize();
	}
}


NiUnknown::NiUnknown() {
	blockType = NIUNKNOWN;
	data = nullptr;
}

NiUnknown::NiUnknown(fstream& file, uint size) {
	blockType = NIUNKNOWN;
	blockSize = size;
	data = new char[size];
	Get(file);
}

NiUnknown::NiUnknown(uint size) {
	blockType = NIUNKNOWN;
	blockSize = size;
	data = new char[size];
}

NiUnknown::~NiUnknown() {
	if (data)
		delete[] data;
}

void NiUnknown::Clone(NiUnknown* other) {
	if (blockSize == other->blockSize)
		memcpy(data, other->data, blockSize);
}

void NiUnknown::Get(fstream& file) {
	if (!data)
		return;

	file.read(data, blockSize);
}

void NiUnknown::Put(fstream& file) {
	if (!data)
		return;

	file.write(data, blockSize);
}


void NifBlockBSLightShadeProp::Get(fstream& file) {

	// read skyrim shader type before getting NiObjectNET parent

	file.read((char*)&shaderType, 4);
	file.read((char*)&nameID, 4);
	if (nameID != -1)
		shaderName = header->strings[nameID].str;
	else
		shaderName = "";

	int intVal;
	file.read((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++) {
		file.read((char*)&intVal, 4);
		extraData.push_back(intVal);
	}

	file.read((char*)&controllerRef, 4);
	file.read((char*)&shaderFlags1, 4);
	file.read((char*)&shaderFlags2, 4);
	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	file.read((char*)&texsetRef, 4);

	file.read((char*)&emissiveClr.x, 4);
	file.read((char*)&emissiveClr.y, 4);
	file.read((char*)&emissiveClr.z, 4);
	file.read((char*)&emissivleMult, 4);
	file.read((char*)&texClampMode, 4);
	file.read((char*)&alpha, 4);
	file.read((char*)&unk, 4);
	file.read((char*)&glossiness, 4);
	file.read((char*)&specClr.x, 4);
	file.read((char*)&specClr.y, 4);
	file.read((char*)&specClr.z, 4);
	file.read((char*)&specStr, 4);
	file.read((char*)&lightFX1, 4);
	file.read((char*)&lightFX2, 4);

	if (shaderType == 1) {
		file.read((char*)&envMapScale, 4);
	}
	else if (shaderType == 5) {
		file.read((char*)&skinTintClr.x, 4);
		file.read((char*)&skinTintClr.y, 4);
		file.read((char*)&skinTintClr.z, 4);
	}
	else if (shaderType == 6) {
		file.read((char*)&hairTintClr.x, 4);
		file.read((char*)&hairTintClr.y, 4);
		file.read((char*)&hairTintClr.z, 4);
	}
	else if (shaderType == 7) {
		file.read((char*)&maxPasses, 4);
		file.read((char*)&scale, 4);
	}
	else if (shaderType == 11) {
		file.read((char*)&parallaxThickness, 4);
		file.read((char*)&parallaxRefrScale, 4);
		file.read((char*)&parallaxTexScale.u, 4);
		file.read((char*)&parallaxTexScale.v, 4);
		file.read((char*)&parallaxEnvMapStr, 4);
	}
	else if (shaderType == 14) {
		file.read((char*)&sparkleParams.r, 4);
		file.read((char*)&sparkleParams.g, 4);
		file.read((char*)&sparkleParams.b, 4);
		file.read((char*)&sparkleParams.a, 4);
	}
	else if (shaderType == 16) {
		file.read((char*)&eyeCubeScale, 4);
		file.read((char*)&eyeLeftReflectCenter.x, 4);
		file.read((char*)&eyeLeftReflectCenter.y, 4);
		file.read((char*)&eyeLeftReflectCenter.z, 4);
		file.read((char*)&eyeRightReflectCenter.x, 4);
		file.read((char*)&eyeRightReflectCenter.y, 4);
		file.read((char*)&eyeRightReflectCenter.z, 4);
	}
}

void NifBlockBSLightShadeProp::Put(fstream& file) {

	// write skyrim shader type before putting NiObjectNET parent

	file.write((char*)&shaderType, 4);
	file.write((char*)&nameID, 4);

	file.write((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++)
		file.write((char*)&extraData[i], 4);

	file.write((char*)&controllerRef, 4);
	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);

	file.write((char*)&texsetRef, 4);
	file.write((char*)&emissiveClr.x, 4);
	file.write((char*)&emissiveClr.y, 4);
	file.write((char*)&emissiveClr.z, 4);
	file.write((char*)&emissivleMult, 4);
	file.write((char*)&texClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&unk, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specClr.x, 4);
	file.write((char*)&specClr.y, 4);
	file.write((char*)&specClr.z, 4);
	file.write((char*)&specStr, 4);
	file.write((char*)&lightFX1, 4);
	file.write((char*)&lightFX2, 4);

	if (shaderType == 1) {
		file.write((char*)&envMapScale, 4);
	}
	else if (shaderType == 5) {
		file.write((char*)&skinTintClr.x, 4);
		file.write((char*)&skinTintClr.y, 4);
		file.write((char*)&skinTintClr.z, 4);
	}
	else if (shaderType == 6) {
		file.write((char*)&hairTintClr.x, 4);
		file.write((char*)&hairTintClr.y, 4);
		file.write((char*)&hairTintClr.z, 4);
	}
	else if (shaderType == 7) {
		file.write((char*)&maxPasses, 4);
		file.write((char*)&scale, 4);
	}
	else if (shaderType == 11) {
		file.write((char*)&parallaxThickness, 4);
		file.write((char*)&parallaxRefrScale, 4);
		file.write((char*)&parallaxTexScale.u, 4);
		file.write((char*)&parallaxTexScale.v, 4);
		file.write((char*)&parallaxEnvMapStr, 4);
	}
	else if (shaderType == 14) {
		file.write((char*)&sparkleParams.r, 4);
		file.write((char*)&sparkleParams.g, 4);
		file.write((char*)&sparkleParams.b, 4);
		file.write((char*)&sparkleParams.a, 4);
	}
	else if (shaderType == 16) {
		file.write((char*)&eyeCubeScale, 4);
		file.write((char*)&eyeLeftReflectCenter.x, 4);
		file.write((char*)&eyeLeftReflectCenter.y, 4);
		file.write((char*)&eyeLeftReflectCenter.z, 4);
		file.write((char*)&eyeRightReflectCenter.x, 4);
		file.write((char*)&eyeRightReflectCenter.y, 4);
		file.write((char*)&eyeRightReflectCenter.z, 4);
	}
}

void NifBlockBSLightShadeProp::Clone(NifBlockBSLightShadeProp* Other) {
	(*this) = (*Other);
}

NifBlockBSLightShadeProp::NifBlockBSLightShadeProp(NiHeader& hdr) {
	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;
	shaderType = 0;
	nameID = -1;
	shaderName = "";
	numExtraData = 0;
	controllerRef = -1;
	shaderFlags1 = 0x82400303;
	shaderFlags2 = 0x8001;

	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;

	texsetRef = -1;
	emissiveClr.x = 0.0f;
	emissiveClr.y = 0.0f;
	emissiveClr.z = 0.0f;;
	emissivleMult = 1.0f;

	texClampMode = 3;
	alpha = 1.0f;
	unk = 0.0f;

	glossiness = 20.0f;
	specClr.x = 1.0f;
	specClr.y = 1.0f;
	specClr.z = 1.0f;
	specStr = 1.0f;

	lightFX1 = 0.3f;
	lightFX2 = 2.0f;
	blockSize = 100;
}

NifBlockBSLightShadeProp::NifBlockBSLightShadeProp(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;
	Get(file);
}
	
void NifBlockBSLightShadeProp::notifyBlockDelete(int blockID) {
	if (texsetRef == blockID)
		texsetRef = -1;
	else if (texsetRef > blockID)
		texsetRef--;
}

bool NifBlockBSLightShadeProp::IsSkinShader() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool NifBlockBSLightShadeProp::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}


NifBlockBSShaderTextureSet::NifBlockBSShaderTextureSet(NiHeader& hdr) {
	header = &hdr;
	blockType = BSSHADERTEXTURESET;
	numTex = 9;
	for (int i = 0; i < 9; i++)
		textures.push_back(NiString(false));

	blockSize = 40;
}

NifBlockBSShaderTextureSet::NifBlockBSShaderTextureSet(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = BSSHADERTEXTURESET;
	Get(file);
}
	
void NifBlockBSShaderTextureSet::Get(fstream& file) {
	file.read((char*)&numTex, 4);
	for (int i = 0; i < numTex; i++)
		textures.push_back(NiString(file, 4, false));
}
	
void NifBlockBSShaderTextureSet::Put(fstream& file) {
	file.write((char*)&numTex, 4);
	for (int i = 0; i < numTex; i++)
		textures[i].Put(file, 4);
}

int NifBlockBSShaderTextureSet::CalcBlockSize() {
	blockSize = 4;
	for (auto tex : textures) {
		blockSize += 4;
		blockSize += tex.str.length();
	}

	return blockSize;
}


NifBlockAlphaProperty::NifBlockAlphaProperty(NiHeader& hdr) {
	header = &hdr;
	blockType = NIALPHAPROPERTY;
	blockSize = 15;
	alphaName = "";
	nameID = -1;
	numExtraData = 0;
	controllerRef = -1;
	flags = 4844;
	threshold = 128;
}

NifBlockAlphaProperty::NifBlockAlphaProperty(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NIALPHAPROPERTY;
	Get(file);
}

void NifBlockAlphaProperty::Get(fstream& file) {
	file.read((char*)&nameID, 4);
	if (nameID != -1)
		alphaName = header->strings[nameID].str;
	else
		alphaName = "";

	int intVal;
	file.read((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++) {
		file.read((char*)&intVal, 4);
		extraData.push_back(intVal);
	}
	file.read((char*)&controllerRef, 4);
	file.read((char*)&flags, 2);
	file.read((char*)&threshold, 1);
}

void NifBlockAlphaProperty::Put(fstream& file) {
	file.write((char*)&nameID, 4);
	file.write((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++)
		file.write((char*)&extraData[i], 4);
	file.write((char*)&controllerRef, 4);
	file.write((char*)&flags, 2);
	file.write((char*)&threshold, 1);
}

int NifBlockAlphaProperty::CalcBlockSize() {
	blockSize = 15;
	for (int i = 0; i < numExtraData; i++)
		blockSize += 4;

	return blockSize;
}

void NifBlockStringExtraData::Get(fstream& file) {
	file.read((char*)&nameID, 4);
	if (nameID != -1)
		name = header->strings[nameID].str;
	else
		name = "";

	//file.read((char*)&nextExtraData, 4);
	//file.read((char*)&bytesRemaining, 4);

	file.read((char*)&stringDataId, 4);
	if (stringDataId != -1)
		stringData = header->strings[stringDataId].str;
	else
		stringData = "";
}

void NifBlockStringExtraData::Put(fstream& file) {
	file.write((char*)&nameID, 4);

	//file.write((char*)&nextExtraData, 4);
	//file.write((char*)&bytesRemaining, 4);

	file.write((char*)&stringDataId, 4);
}

NifBlockStringExtraData::NifBlockStringExtraData(NiHeader& hdr) {
	header = &hdr;
	blockType = NISTRINGEXTRADATA;
	blockSize = 8;
	nameID = -1;
	stringDataId = -1;
	name = "";
	//nextExtraData = -1;
	//bytesRemaining = 4;
	stringData = "";
}

NifBlockStringExtraData::NifBlockStringExtraData(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = NISTRINGEXTRADATA;
	Get(file);
}


void NifBlockBSShadePPLgtProp::Get(fstream& file) {
	file.read((char*)&nameID, 4);
	if (nameID != -1)
		shaderName = header->strings[nameID].str;
	else
		shaderName = "";

	int intVal;
	file.read((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++) {
		file.read((char*)&intVal, 4);
		extraData.push_back(intVal);
	}

	file.read((char*)&controllerRef, 4);
	file.read((char*)&flags, 2);
	file.read((char*)&shaderType, 4);
	file.read((char*)&shaderFlags, 4);
	file.read((char*)&unkInt2, 4);
	file.read((char*)&envMapScale, 4);
	file.read((char*)&unkInt3, 4);
	file.read((char*)&texsetRef, 4);
	file.read((char*)&unkFloat2, 4);
	file.read((char*)&refractionPeriod, 4);
	file.read((char*)&unkFloat4, 4);
	file.read((char*)&unkFloat5, 4);
}

void NifBlockBSShadePPLgtProp::Put(fstream& file) {
	file.write((char*)&nameID, 4);
	file.write((char*)&numExtraData, 4);
	for (int i = 0; i < numExtraData; i++)
		file.write((char*)&extraData[i], 4);

	file.write((char*)&controllerRef, 4);
	file.write((char*)&flags, 2);
	file.write((char*)&shaderType, 4);
	file.write((char*)&shaderFlags, 4);
	file.write((char*)&unkInt2, 4);
	file.write((char*)&envMapScale, 4);
	file.write((char*)&unkInt3, 4);
	file.write((char*)&texsetRef, 4);
	file.write((char*)&unkFloat2, 4);
	file.write((char*)&refractionPeriod, 4);
	file.write((char*)&unkFloat4, 4);
	file.write((char*)&unkFloat5, 4);
}

void NifBlockBSShadePPLgtProp::Clone(NifBlockBSShadePPLgtProp* Other) {
	(*this) = (*Other);
}

NifBlockBSShadePPLgtProp::NifBlockBSShadePPLgtProp(NiHeader& hdr) {
	header = &hdr;
	blockType = BSSHADERPPLIGHTINGPROPERTY;
	shaderType = 0;
	nameID = -1;
	shaderName = "";
	numExtraData = 0;
	controllerRef = -1;
	flags = 0x0001;
	shaderType = 0x00000001;
	shaderFlags = 0x82000103;

	unkInt2 = 1;
	envMapScale = 1.0f;
	unkInt3 = 3;

	texsetRef = -1;
	unkFloat2 = 0.0f;
	refractionPeriod = 0;
	unkFloat4 = 4.0f;
	unkFloat5 = 1.0f;

	blockSize = 58;
}

NifBlockBSShadePPLgtProp::NifBlockBSShadePPLgtProp(fstream& file, NiHeader& hdr) {
	header = &hdr;
	blockType = BSSHADERPPLIGHTINGPROPERTY;
	Get(file);
}

void NifBlockBSShadePPLgtProp::notifyBlockDelete(int blockID) {
	if (texsetRef == blockID)
		texsetRef = -1;
	else if (texsetRef > blockID)
		texsetRef--;
}

bool NifBlockBSShadePPLgtProp::IsSkinShader() {
	return shaderType == 0x0000000e;
}
