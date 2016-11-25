/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "NifBlock.h"
#include "half.hpp"

#pragma warning (disable : 4100)


template<typename T>
KeyGroup<T>::KeyGroup() {
	numKeys = 0;
}

template<typename T>
KeyGroup<T>::KeyGroup(fstream& file) {
	Get(file);
}

template<typename T>
void KeyGroup<T>::Get(fstream& file) {
	file.read((char*)&numKeys, 4);
	keys.resize(numKeys);

	if (numKeys > 0) {
		file.read((char*)&interpolation, 4);

		for (int i = 0; i < numKeys; i++) {
			Key<T> key;
			file.read((char*)&key.time, 4);
			file.read((char*)&key.value, sizeof(T));

			switch (interpolation) {
			case 2:
				file.read((char*)&key.forward, sizeof(T));
				file.read((char*)&key.backward, sizeof(T));
				break;
			case 3:
				file.read((char*)&key.tbc, 12);
				break;
			}

			keys[i] = move(key);
		}
	}
}

template<typename T>
void KeyGroup<T>::Put(fstream& file) {
	file.write((char*)&numKeys, 4);

	if (numKeys > 0) {
		file.write((char*)&interpolation, 4);

		for (int i = 0; i < numKeys; i++) {
			file.write((char*)&keys[i].time, 4);
			file.write((char*)&keys[i].value, sizeof(T));

			switch (interpolation) {
			case 2:
				file.write((char*)&keys[i].forward, sizeof(T));
				file.write((char*)&keys[i].backward, sizeof(T));
				break;
			case 3:
				file.write((char*)&keys[i].tbc, 12);
				break;
			}
		}
	}
}

template<typename T>
int KeyGroup<T>::CalcGroupSize() {
	int groupSize = 4;

	if (numKeys > 0) {
		groupSize += 4;
		groupSize += 4 * numKeys;
		groupSize += sizeof(T) * numKeys;

		switch (interpolation) {
		case 2:
			groupSize += sizeof(T) * numKeys * 2;
			break;
		case 3:
			groupSize += 12 * numKeys;
			break;
		}
	}

	return groupSize;
}

SubConstraintDesc::SubConstraintDesc() {
	numEntities = 0;
}

SubConstraintDesc::SubConstraintDesc(fstream& file) {
	Get(file);
}

void SubConstraintDesc::Get(fstream& file) {
	file.read((char*)&type, 4);

	file.read((char*)&numEntities, 4);
	entities.resize(numEntities);
	for (int i = 0; i < numEntities; i++)
		file.read((char*)&entities[i], 4);

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

void SubConstraintDesc::Put(fstream& file) {
	file.write((char*)&type, 4);

	file.write((char*)&numEntities, 4);
	for (int i = 0; i < numEntities; i++)
		file.write((char*)&entities[i], 4);

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

void SubConstraintDesc::notifyBlockDelete(int blockID) {
	for (int i = 0; i < numEntities; i++) {
		if (entities[i] == blockID) {
			entities.erase(entities.begin() + i);
			i--;
			numEntities--;
		}
		else if (entities[i] > blockID)
			entities[i]--;
	}
}

void SubConstraintDesc::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	for (int i = 0; i < numEntities; i++) {
		if (entities[i] == blockIndexLo)
			entities[i] = blockIndexHi;
		else if (entities[i] == blockIndexHi)
			entities[i] = blockIndexLo;
	}
}

void SubConstraintDesc::GetChildRefs(set<int>& refs) {
	refs.insert(entities.begin(), entities.end());
}

int SubConstraintDesc::CalcDescSize() {
	int descSize = 12;
	descSize += numEntities * 4;

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


NiString::NiString(fstream& file, const int& szSize) {
	Get(file, szSize);
}

void NiString::Put(fstream& file, const int& szSize, const bool& wantNullOutput) {
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

void NiString::Get(fstream& file, const int& szSize) {
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


NiObject::NiObject() {
	blockSize = 0;
}

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

void NiObject::notifyStringDelete(int stringID) {
}

void NiObject::Get(fstream& file) {
}

void NiObject::Put(fstream& file) {
}

void NiObject::GetChildRefs(set<int>& refs) {
}

int NiObject::CalcBlockSize() {
	blockSize = 0;	// Calculate from the ground up
	return blockSize;
}


NiHeader::NiHeader() {
	valid = false;

	header = this;
	blockType = NIHEADER;

	numBlocks = 0;
	numStrings = 0;
	blocks = nullptr;
}

void NiHeader::Clear() {
	numBlockTypes = 0;
	numStrings = 0;
	numBlocks = 0;
	blocks = nullptr;
	blockTypes.clear();
	blockTypeIndices.clear();
	blockSizes.clear();
	strings.clear();
	stringRefCount.clear();
}

string NiHeader::GetVersionInfo() {
	return string(verStr, 0x26) +
		"\nUser Version: " + to_string(userVersion) +
		"\nUser Version 2: " + to_string(userVersion2);
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

bool NiHeader::VerCheck(const int& v1, const int& v2, const int& v3, const int& v4, const bool& equal) {
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

string NiHeader::GetCreatorInfo() {
	return creator.GetString();
}

void NiHeader::SetCreatorInfo(const string& creatorInfo) {
	creator.SetString(creatorInfo);
}

string NiHeader::GetExportInfo() {
	string exportInfo = exportInfo1.GetString();

	if (exportInfo2.GetLength() > 0) {
		exportInfo.append("\n");
		exportInfo.append(exportInfo2.GetString());
	}

	if (exportInfo3.GetLength() > 0) {
		exportInfo.append("\n");
		exportInfo.append(exportInfo3.GetString());
	}

	return exportInfo;
}

void NiHeader::SetExportInfo(const string& exportInfo) {
	exportInfo1.Clear();
	exportInfo2.Clear();
	exportInfo3.Clear();

	vector<NiString*> exportStrings(3);
	exportStrings[0] = &exportInfo1;
	exportStrings[1] = &exportInfo2;
	exportStrings[2] = &exportInfo3;

	auto it = exportStrings.begin();
	for (size_t i = 0; i < exportInfo.length() && it < exportStrings.end(); i += 256, it++) {
		if (i + 256 <= exportInfo.length())
			(*it)->SetString(exportInfo.substr(i, 256));
		else
			(*it)->SetString(exportInfo.substr(i, exportInfo.length() - i));
	}
}

void NiHeader::DeleteBlock(int blockId) {
	if (blockId == 0xFFFFFFFF)
		return;

	ushort blockTypeId = blockTypeIndices[blockId];
	int blockTypeRefCount = 0;
	for (int i = 0; i < blockTypeIndices.size(); i++)
		if (blockTypeIndices[i] == blockTypeId)
			blockTypeRefCount++;

	if (blockTypeRefCount < 2) {
		blockTypes.erase(blockTypes.begin() + blockTypeId);
		numBlockTypes--;
		for (int i = 0; i < blockTypeIndices.size(); i++)
			if (blockTypeIndices[i] > blockTypeId)
				blockTypeIndices[i]--;
	}

	delete (*blocks)[blockId];
	(*blocks).erase((*blocks).begin() + blockId);
	numBlocks--;
	blockTypeIndices.erase(blockTypeIndices.begin() + blockId);
	blockSizes.erase(blockSizes.begin() + blockId);

	for (int i = 0; i < numBlocks; i++)
		(*blocks)[i]->notifyBlockDelete(blockId);
}

void NiHeader::DeleteBlockByType(const string& blockTypeStr) {
	ushort blockTypeId;
	for (blockTypeId = 0; blockTypeId < numBlockTypes; blockTypeId++)
		if (blockTypes[blockTypeId].GetString() == blockTypeStr)
			break;

	if (blockTypeId == numBlockTypes)
		return;

	vector<int> indices;
	for (int i = 0; i < numBlocks; i++)
		if (blockTypeIndices[i] == blockTypeId)
			indices.push_back(i);

	for (int j = indices.size() - 1; j >= 0; j--)
		DeleteBlock(indices[j]);
}

int NiHeader::AddBlock(NiObject* newBlock, const string& blockTypeStr) {
	ushort btID = AddOrFindBlockTypeId(blockTypeStr);
	blockTypeIndices.push_back(btID);
	blockSizes.push_back(newBlock->CalcBlockSize());
	(*blocks).push_back(newBlock);
	numBlocks = (*blocks).size();
	return numBlocks - 1;
}

int NiHeader::ReplaceBlock(int oldBlockId, NiObject* newBlock, const string& blockTypeStr) {
	if (oldBlockId == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	ushort blockTypeId = blockTypeIndices[oldBlockId];
	int blockTypeRefCount = 0;
	for (int i = 0; i < blockTypeIndices.size(); i++)
		if (blockTypeIndices[i] == blockTypeId)
			blockTypeRefCount++;

	if (blockTypeRefCount < 2) {
		blockTypes.erase(blockTypes.begin() + blockTypeId);
		numBlockTypes--;
		for (int i = 0; i < blockTypeIndices.size(); i++)
			if (blockTypeIndices[i] > blockTypeId)
				blockTypeIndices[i]--;
	}

	delete (*blocks)[oldBlockId];

	ushort btID = AddOrFindBlockTypeId(blockTypeStr);
	blockTypeIndices[oldBlockId] = btID;
	blockSizes[oldBlockId] = newBlock->CalcBlockSize();
	(*blocks)[oldBlockId] = newBlock;
	return oldBlockId;
}

void NiHeader::SwapBlocks(const int& blockIndexLo, const int& blockIndexHi) {
	if (blockIndexLo == 0xFFFFFFFF || blockIndexHi == 0xFFFFFFFF)
		return;

	// First swap data
	iter_swap(blockTypeIndices.begin() + blockIndexLo, blockTypeIndices.begin() + blockIndexHi);
	iter_swap(blockSizes.begin() + blockIndexLo, blockSizes.begin() + blockIndexHi);
	iter_swap((*blocks).begin() + blockIndexLo, (*blocks).begin() + blockIndexHi);

	// Next tell all the blocks that the swap happened
	for (int i = 0; i < numBlocks; i++)
		(*blocks)[i]->notifyBlockSwap(blockIndexLo, blockIndexHi);
}

bool NiHeader::IsBlockReferenced(const int& blockId) {
	if (blockId == 0xFFFFFFFF)
		return false;

	for (auto &block : (*blocks)) {
		set<int> refs;
		block->GetChildRefs(refs);

		if (refs.find(blockId) != refs.end())
			return true;
	}

	return false;
}

void NiHeader::DeleteUnreferencedBlocks(bool* hadDeletions) {
	for (int i = 1; i < numBlocks; i++) {
		if (!IsBlockReferenced(i)) {
			DeleteBlock(i);

			// Deleting a block can cause others to become unreferenced
			if (hadDeletions)
				(*hadDeletions) = true;

			return DeleteUnreferencedBlocks();
		}
	}
}

ushort NiHeader::AddOrFindBlockTypeId(const string& blockTypeName) {
	NiString niStr;
	ushort typeId = (ushort)blockTypes.size();
	for (ushort i = 0; i < blockTypes.size(); i++) {
		if (blockTypes[i].GetString() == blockTypeName) {
			typeId = i;
			break;
		}
	}

	// Shader block type not found, add it
	if (typeId == blockTypes.size()) {
		niStr.SetString(blockTypeName);
		blockTypes.push_back(niStr);
		numBlockTypes++;
	}
	return typeId;
}

string NiHeader::GetBlockTypeStringById(const int& id) {
	return blockTypes[blockTypeIndices[id]].GetString();
}

ushort NiHeader::GetBlockTypeIndex(const int& id) {
	return blockTypeIndices[id];
}

void NiHeader::CalcAllBlockSizes() {
	for (int i = 0; i < numBlocks; i++)
		blockSizes[i] = (*blocks)[i]->CalcBlockSize();
}

int NiHeader::FindStringId(const string& str) {
	for (int i = 0; i < strings.size(); i++)
		if (strings[i].GetString().compare(str) == 0)
			return i;

	return 0xFFFFFFFF;
}

int NiHeader::AddOrFindStringId(const string& str) {
	if (str.empty())
		return 0xFFFFFFFF;

	for (int i = 0; i < strings.size(); i++) {
		if (strings[i].GetString().compare(str) == 0) {
			stringRefCount[i]++;
			return i;
		}
	}

	int r = strings.size();

	NiString niStr;
	niStr.SetString(str);
	strings.push_back(niStr);
	numStrings++;

	stringRefCount.resize(numStrings);
	stringRefCount[numStrings - 1]++;

	if (maxStringLen < str.length())
		maxStringLen = str.length();

	return r;
}

string NiHeader::GetStringById(const int& id) {
	if (id >= 0 && id < numStrings)
		return strings[id].GetString();

	return string();
}

void NiHeader::SetStringById(const int& id, const string& str) {
	if (id >= 0 && id < numStrings) {
		strings[id].SetString(str);
		if (maxStringLen < str.length())
			maxStringLen = str.length();
	}
}

void NiHeader::AddStringRef(const int& id) {
	if (id >= 0 && id < numStrings)
		stringRefCount[id]++;
}

void NiHeader::RemoveStringRef(const int& id) {
	if (id >= 0 && id < numStrings && stringRefCount[id] > 0)
		stringRefCount[id]--;
}

int NiHeader::RemoveUnusedStrings() {
	int count = 0;

	for (int i = 0; i < stringRefCount.size(); i++) {
		if (stringRefCount[i] <= 0) {
			strings.erase(strings.begin() + i);
			stringRefCount.erase(stringRefCount.begin() + i);
			numStrings--;

			for (auto &block : (*blocks))
				block->notifyStringDelete(i);

			i--;
			count++;
		}
	}

	maxStringLen = 0;
	for (auto &s : strings)
		if (maxStringLen < s.GetLength())
			maxStringLen = s.GetLength();

	return count;
}

void NiHeader::Get(fstream& file) {
	file.read(verStr, 38);
	if (_strnicmp(verStr, "Gamebryo", 8) != 0)
		return;

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
		blockTypeIndices.push_back(uShort);
	}
	uint uInt;
	for (int i = 0; i < numBlocks; i++) {
		file.read((char*)&uInt, 4);
		blockSizes.push_back(uInt);
	}

	file.read((char*)&numStrings, 4);
	file.read((char*)&maxStringLen, 4);

	strings.resize(numStrings);
	stringRefCount.resize(numStrings);
	for (int i = 0; i < numStrings; i++)
		strings[i] = NiString(file, 4);

	file.read((char*)&unkInt2, 4);
	valid = true;
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
		file.write((char*)&blockTypeIndices[i], 2);

	for (int i = 0; i < numBlocks; i++)
		file.write((char*)&blockSizes[i], 4);

	file.write((char*)&numStrings, 4);
	file.write((char*)&maxStringLen, 4);
	for (int i = 0; i < numStrings; i++)
		strings[i].Put(file, 4, false);

	file.write((char*)&unkInt2, 4);
}


NiObjectNET::~NiObjectNET() {
	header->RemoveStringRef(nameRef);
}

void NiObjectNET::Init() {
	NiObject::Init();

	bBSLightingShaderProperty = false;
	skyrimShaderType = 0;
	name.clear();
	nameRef = 0xFFFFFFFF;
	numExtraData = 0;
	controllerRef = 0xFFFFFFFF;
}

void NiObjectNET::Get(fstream& file) {
	NiObject::Get(file);

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
		file.read((char*)&skyrimShaderType, 4);

	file.read((char*)&nameRef, 4);
	if (nameRef != 0xFFFFFFFF) {
		name = header->GetStringById(nameRef);
		header->AddStringRef(nameRef);
	}
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

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
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
		controllerRef = 0xFFFFFFFF;
	else if (controllerRef > blockID)
		controllerRef--;
}

void NiObjectNET::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

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

void NiObjectNET::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	if (nameRef != 0xFFFFFFFF && nameRef > stringID)
		nameRef--;
}

string NiObjectNET::GetName() {
	return name;
}

void NiObjectNET::SetName(const string& propertyName, const bool& renameExisting) {
	if (renameExisting)
		header->SetStringById(nameRef, propertyName);
	else
		nameRef = header->AddOrFindStringId(propertyName);

	name = propertyName;
}

void NiObjectNET::ClearName() {
	nameRef = 0xFFFFFFFF;
	name.clear();
}

int NiObjectNET::GetExtraDataRef(const int& id) {
	if (id >= 0 && id < numExtraData)
		return extraDataRef[id];

	return 0xFFFFFFFF;
}

void NiObjectNET::AddExtraDataRef(const int& id) {
	extraDataRef.push_back(id);
	numExtraData++;
}

void NiObjectNET::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(extraDataRef.begin(), extraDataRef.end());
	refs.insert(controllerRef);
}

int NiObjectNET::CalcBlockSize() {
	NiObject::CalcBlockSize();

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
		blockSize += 4;

	blockSize += 12;
	blockSize += numExtraData * 4;

	return blockSize;
}


void NiAVObject::Init() {
	NiObjectNET::Init();

	flags = 524302;
	rotation[0].x = 1.0f;
	rotation[1].y = 1.0f;
	rotation[2].z = 1.0f;
	scale = 1.0f;
	numProperties = 0;
	collisionRef = 0xFFFFFFFF;
}

void NiAVObject::Get(fstream& file) {
	NiObjectNET::Get(file);

	flags = 0;
	if (header->GetUserVersion2() < 34)
		file.read((char*)&flags, 2);
	else
		file.read((char*)&flags, 4);

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
	if (header->GetUserVersion() <= 11) {
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

	if (header->GetUserVersion2() < 34)
		file.write((char*)&flags, 2);
	else
		file.write((char*)&flags, 4);

	file.write((char*)&translation.x, 4);
	file.write((char*)&translation.y, 4);
	file.write((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.write((char*)&rotation[i].x, 4);
		file.write((char*)&rotation[i].y, 4);
		file.write((char*)&rotation[i].z, 4);
	}

	file.write((char*)&scale, 4);

	if (header->GetUserVersion() <= 11) {
		file.write((char*)&numProperties, 4);
		for (int i = 0; i < numProperties; i++)
			file.write((char*)&propertiesRef[i], 4);
	}

	file.write((char*)&collisionRef, 4);
}

void NiAVObject::notifyBlockDelete(int blockID) {
	NiObjectNET::notifyBlockDelete(blockID);

	if (collisionRef == blockID)
		collisionRef = 0xFFFFFFFF;
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
		if (propertiesRef[i] == blockIndexLo)
			propertiesRef[i] = blockIndexHi;
		else if (propertiesRef[i] == blockIndexHi)
			propertiesRef[i] = blockIndexLo;
	}
}

void NiAVObject::GetChildRefs(set<int>& refs) {
	NiObjectNET::GetChildRefs(refs);

	refs.insert(propertiesRef.begin(), propertiesRef.end());
	refs.insert(collisionRef);
}

int NiAVObject::CalcBlockSize() {
	NiObjectNET::CalcBlockSize();

	blockSize += 58;
	if (header->GetUserVersion() <= 11) {
		blockSize += 4;
		blockSize += numProperties * 4;
	}
	if (header->GetUserVersion() >= 11 && header->GetUserVersion2() > 26)
		blockSize += 2;

	return blockSize;
}


NiNode::NiNode(NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = NINODE;
}

NiNode::NiNode(fstream& file, NiHeader& hdr) {
	Init();

	header = &hdr;
	blockType = NINODE;

	Get(file);
}

void NiNode::Init() {
	NiAVObject::Init();

	numChildren = 0;
	numEffects = 0;
}

void NiNode::Get(fstream& file) {
	NiAVObject::Get(file);

	int intData;
	file.read((char*)&numChildren, 4);
	for (int i = 0; i < numChildren; i++) {
		file.read((char*)&intData, 4);
		children.push_back(intData);
	}

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
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

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
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

void NiNode::GetChildRefs(set<int>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(children.begin(), children.end());
	refs.insert(effects.begin(), effects.end());
}

int NiNode::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 4;
	blockSize += numChildren * 4;
	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
		blockSize += 4;
		blockSize += numEffects * 4;
	}
	return blockSize;
}

int NiNode::GetChildRef(const int& id) {
	if (id >= 0 && id < numChildren)
		return children[id];

	return 0xFFFFFFFF;
}

void NiNode::AddChildRef(const int& id) {
	children.push_back(id);
	numChildren++;
}

void NiNode::ClearChildren() {
	children.clear();
	numChildren = 0;
}

int NiNode::GetEffectRef(const int& id) {
	if (id >= 0 && id < numEffects)
		return effects[id];

	return 0xFFFFFFFF;
}

void NiNode::AddEffectRef(const int& id) {
	effects.push_back(id);
	numEffects++;
}


BSFadeNode::BSFadeNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSFADENODE;
}

BSFadeNode::BSFadeNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSFADENODE;

	Get(file);
}


BSValueNode::BSValueNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSVALUENODE;
}

BSValueNode::BSValueNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSVALUENODE;

	Get(file);
}

void BSValueNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&value, 4);
	file.read((char*)&unkByte, 1);
}

void BSValueNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&value, 4);
	file.write((char*)&unkByte, 1);
}

int BSValueNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSLeafAnimNode::BSLeafAnimNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSLEAFANIMNODE;
}

BSLeafAnimNode::BSLeafAnimNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSLEAFANIMNODE;

	Get(file);
}


BSTreeNode::BSTreeNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSTREENODE;

	numBones1 = 0;
	numBones2 = 0;
}

BSTreeNode::BSTreeNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSTREENODE;

	Get(file);
}

void BSTreeNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&numBones1, 4);
	bones1.resize(numBones1);
	for (int i = 0; i < numBones1; i++)
		file.read((char*)&bones1[i], 4);

	file.read((char*)&numBones2, 4);
	bones2.resize(numBones2);
	for (int i = 0; i < numBones2; i++)
		file.read((char*)&bones2[i], 4);
}

void BSTreeNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&numBones1, 4);
	for (int i = 0; i < numBones1; i++)
		file.write((char*)&bones1[i], 4);

	file.write((char*)&numBones2, 4);
	for (int i = 0; i < numBones2; i++)
		file.write((char*)&bones2[i], 4);
}

void BSTreeNode::notifyBlockDelete(int blockID) {
	NiNode::notifyBlockDelete(blockID);

	for (int i = 0; i < numBones1; i++) {
		if (bones1[i] == blockID) {
			bones1.erase(bones1.begin() + i);
			i--;
			numBones1--;
		}
		else if (bones1[i] > blockID)
			bones1[i]--;
	}

	for (int i = 0; i < numBones2; i++) {
		if (bones2[i] == blockID) {
			bones2.erase(bones2.begin() + i);
			i--;
			numBones2--;
		}
		else if (bones2[i] > blockID)
			bones2[i]--;
	}
}

void BSTreeNode::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiNode::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numBones1; i++) {
		if (bones1[i] == blockIndexLo)
			bones1[i] = blockIndexHi;
		else if (bones1[i] == blockIndexHi)
			bones1[i] = blockIndexLo;
	}

	for (int i = 0; i < numBones2; i++) {
		if (bones2[i] == blockIndexLo)
			bones2[i] = blockIndexHi;
		else if (bones2[i] == blockIndexHi)
			bones2[i] = blockIndexLo;
	}
}

void BSTreeNode::GetChildRefs(set<int>& refs) {
	NiNode::GetChildRefs(refs);

	refs.insert(bones1.begin(), bones1.end());
	refs.insert(bones2.begin(), bones2.end());
}

int BSTreeNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 8;
	blockSize += numBones1 * 4;
	blockSize += numBones2 * 4;

	return blockSize;
}


BSOrderedNode::BSOrderedNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSORDEREDNODE;
}

BSOrderedNode::BSOrderedNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSORDEREDNODE;

	Get(file);
}

void BSOrderedNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&alphaSortBound, 16);
	file.read((char*)&isStaticBound, 1);
}

void BSOrderedNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&alphaSortBound, 16);
	file.write((char*)&isStaticBound, 1);
}

int BSOrderedNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 17;

	return blockSize;
}


BSMultiBoundNode::BSMultiBoundNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDNODE;
}

BSMultiBoundNode::BSMultiBoundNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDNODE;

	Get(file);
}

void BSMultiBoundNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&multiBoundRef, 4);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&cullingMode, 4);
}

void BSMultiBoundNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&multiBoundRef, 4);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&cullingMode, 4);
}

void BSMultiBoundNode::notifyBlockDelete(int blockID) {
	NiNode::notifyBlockDelete(blockID);

	if (multiBoundRef == blockID)
		multiBoundRef = 0xFFFFFFFF;
	else if (multiBoundRef > blockID)
		multiBoundRef--;
}

void BSMultiBoundNode::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiNode::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (multiBoundRef == blockIndexLo)
		multiBoundRef = blockIndexHi;
	else if (multiBoundRef == blockIndexHi)
		multiBoundRef = blockIndexLo;
}

void BSMultiBoundNode::GetChildRefs(set<int>& refs) {
	NiNode::GetChildRefs(refs);

	refs.insert(multiBoundRef);
}

int BSMultiBoundNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 4;
	if (header->GetUserVersion() >= 12)
		blockSize += 4;

	return blockSize;
}


BSBlastNode::BSBlastNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSBLASTNODE;
}

BSBlastNode::BSBlastNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSBLASTNODE;

	Get(file);
}

void BSBlastNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&min, 1);
	file.read((char*)&max, 1);
	file.read((char*)&current, 1);
}

void BSBlastNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&min, 1);
	file.write((char*)&max, 1);
	file.write((char*)&current, 1);
}

int BSBlastNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 3;

	return blockSize;
}


BSDamageStage::BSDamageStage(NiHeader& hdr) : BSBlastNode(hdr) {
	blockType = BSDAMAGESTAGE;
}

BSDamageStage::BSDamageStage(fstream& file, NiHeader& hdr) : BSBlastNode(file, hdr) {
	blockType = BSDAMAGESTAGE;
}


BSMasterParticleSystem::BSMasterParticleSystem(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSMASTERPARTICLESYSTEM;

	numParticleSys = 0;
}

BSMasterParticleSystem::BSMasterParticleSystem(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = BSMASTERPARTICLESYSTEM;

	Get(file);
}

void BSMasterParticleSystem::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&maxEmitterObjs, 2);

	file.read((char*)&numParticleSys, 4);
	particleSysRefs.resize(numParticleSys);
	for (int i = 0; i < numParticleSys; i++)
		file.read((char*)&particleSysRefs[i], 4);
}

void BSMasterParticleSystem::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&maxEmitterObjs, 2);

	file.write((char*)&numParticleSys, 4);
	for (int i = 0; i < numParticleSys; i++)
		file.write((char*)&particleSysRefs[i], 4);
}

void BSMasterParticleSystem::notifyBlockDelete(int blockID) {
	NiNode::notifyBlockDelete(blockID);

	for (int i = 0; i < numParticleSys; i++) {
		if (particleSysRefs[i] == blockID) {
			particleSysRefs.erase(particleSysRefs.begin() + i);
			i--;
			numParticleSys--;
		}
		else if (particleSysRefs[i] > blockID)
			particleSysRefs[i]--;
	}
}

void BSMasterParticleSystem::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiNode::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numParticleSys; i++) {
		if (particleSysRefs[i] == blockIndexLo)
			particleSysRefs[i] = blockIndexHi;
		else if (particleSysRefs[i] == blockIndexHi)
			particleSysRefs[i] = blockIndexLo;
	}
}

void BSMasterParticleSystem::GetChildRefs(set<int>& refs) {
	NiNode::GetChildRefs(refs);

	refs.insert(particleSysRefs.begin(), particleSysRefs.end());
}

int BSMasterParticleSystem::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 6;
	blockSize += numParticleSys * 4;

	return blockSize;
}


NiBillboardNode::NiBillboardNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = NIBILLBOARDNODE;
}

NiBillboardNode::NiBillboardNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = NIBILLBOARDNODE;

	Get(file);
}

void NiBillboardNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&billboardMode, 2);
}

void NiBillboardNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&billboardMode, 2);
}

int NiBillboardNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 2;

	return blockSize;
}


NiSwitchNode::NiSwitchNode(NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = NISWITCHNODE;
}

NiSwitchNode::NiSwitchNode(fstream& file, NiHeader& hdr) {
	NiNode::Init();

	header = &hdr;
	blockType = NISWITCHNODE;

	Get(file);
}

void NiSwitchNode::Get(fstream& file) {
	NiNode::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&index, 4);
}

void NiSwitchNode::Put(fstream& file) {
	NiNode::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&index, 4);
}

int NiSwitchNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


int NiShape::GetSkinInstanceRef() { return 0xFFFFFFFF; }
void NiShape::SetSkinInstanceRef(int skinInstanceRef) { }

int NiShape::GetShaderPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetShaderPropertyRef(int shaderPropertyRef) { }

int NiShape::GetAlphaPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetAlphaPropertyRef(int alphaPropertyRef) { }

int NiShape::GetDataRef() { return 0xFFFFFFFF; }
void NiShape::SetDataRef(int dataRef) { }

void NiShape::SetVertices(const bool& enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetVertices(enable);
};

bool NiShape::HasVertices() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertices();

	return false;
};

void NiShape::SetUVs(const bool& enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetUVs(enable);
};

bool NiShape::HasUVs() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasUVs();

	return false;
};

void NiShape::SetNormals(const bool& enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetNormals(enable);
};

bool NiShape::HasNormals() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasNormals();

	return false;
};

void NiShape::SetTangents(const bool& enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetTangents(enable);
};

bool NiShape::HasTangents() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasTangents();

	return false;
};

void NiShape::SetVertexColors(const bool& enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetVertexColors(enable);
};

bool NiShape::HasVertexColors() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertexColors();

	return false;
};

void NiShape::SetSkinned(const bool& enable) { };
bool NiShape::IsSkinned() { return false; };

void NiShape::SetBounds(const BoundingSphere& bounds) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetBounds(bounds);
}

BoundingSphere NiShape::GetBounds() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->GetBounds();

	return BoundingSphere();
}

void NiShape::UpdateBounds() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->UpdateBounds();
}

int NiShape::GetBoneID(const string& boneName) {
	auto boneCont = header->GetBlock<NiBoneContainer>(GetSkinInstanceRef());
	if (boneCont) {
		for (int i = 0; i < boneCont->numBones; i++) {
			auto node = header->GetBlock<NiNode>(boneCont->bones[i]);
			if (node && node->GetName() == boneName)
				return i;
		}
	}

	return 0xFFFFFFFF;
}


BSTriShape::BSTriShape(NiHeader& hdr) {
	NiAVObject::Init();

	blockType = BSTRISHAPE;
	header = &hdr;

	skinInstanceRef = 0xFFFFFFFF;
	shaderPropertyRef = 0xFFFFFFFF;
	alphaPropertyRef = 0xFFFFFFFF;

	vertFlags3 = 0x43;
	vertFlags4 = 0x50;
	vertFlags5 = 0x0;
	vertFlags6 = 0xB0;
	vertFlags7 = 0x1;
	vertFlags8 = 0x0;

	numTriangles = 0;
	numVertices = 0;
	dataSize = 0;
	vertexSize = 0;
	someDataSize = 0;
}

BSTriShape::BSTriShape(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	blockType = BSTRISHAPE;
	header = &hdr;

	vertexSize = 0;
	someDataSize = 0;

	Get(file);
}

void BSTriShape::Get(fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Get(file);

	file.read((char*)&flags, 4);

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

	file.read((char*)&bounds, 16);

	file.read((char*)&skinInstanceRef, 4);
	file.read((char*)&shaderPropertyRef, 4);

	file.read((char*)&alphaPropertyRef, 4);
	file.read((char*)&vertFlags1, 1);
	file.read((char*)&vertFlags2, 1);
	file.read((char*)&vertFlags3, 1);
	file.read((char*)&vertFlags4, 1);
	file.read((char*)&vertFlags5, 1);
	file.read((char*)&vertFlags6, 1);
	file.read((char*)&vertFlags7, 1);
	file.read((char*)&vertFlags8, 1);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130) {
		ushort num = 0;
		file.read((char*)&num, 2);
		numTriangles = num;
	}
	else
		file.read((char*)&numTriangles, 4);

	file.read((char*)&numVertices, 2);
	file.read((char*)&dataSize, 4);

	vertData.resize(numVertices);

	if (dataSize > 0) {
		half_float::half halfData;
		for (int i = 0; i < numVertices; i++) {
			if (HasVertices()) {
				if (IsFullPrecision()) {
					// Full precision
					file.read((char*)&vertData[i].vert.x, 4);
					file.read((char*)&vertData[i].vert.y, 4);
					file.read((char*)&vertData[i].vert.z, 4);

					file.read((char*)&vertData[i].bitangentX, 4);
				}
				else {
					// Half precision
					file.read((char*)&halfData, 2);
					vertData[i].vert.x = halfData;
					file.read((char*)&halfData, 2);
					vertData[i].vert.y = halfData;
					file.read((char*)&halfData, 2);
					vertData[i].vert.z = halfData;

					file.read((char*)&halfData, 2);
					vertData[i].bitangentX = halfData;
				}
			}

			if (HasUVs()) {
				file.read((char*)&halfData, 2);
				vertData[i].uv.u = halfData;
				file.read((char*)&halfData, 2);
				vertData[i].uv.v = halfData;
			}

			if (HasNormals()) {
				for (int j = 0; j < 3; j++)
					file.read((char*)&vertData[i].normal[j], 1);

				file.read((char*)&vertData[i].bitangentY, 1);

				if (HasTangents()) {
					for (int j = 0; j < 3; j++)
						file.read((char*)&vertData[i].tangent[j], 1);

					file.read((char*)&vertData[i].bitangentZ, 1);
				}
			}


			if (HasVertexColors())
				file.read((char*)&vertData[i].colorData, 4);

			if (IsSkinned()) {
				for (int j = 0; j < 4; j++) {
					file.read((char*)&halfData, 2);
					vertData[i].weights[j] = halfData;
				}

				for (int j = 0; j < 4; j++)
					file.read((char*)&vertData[i].weightBones[j], 1);
			}

			if ((vertFlags7 & (1 << 4)) != 0)
				file.read((char*)&vertData[i].eyeData, 4);
		}
	}

	triangles.resize(numTriangles);

	if (dataSize > 0) {
		for (int i = 0; i < numTriangles; i++) {
			file.read((char*)&triangles[i].p1, 2);
			file.read((char*)&triangles[i].p2, 2);
			file.read((char*)&triangles[i].p3, 2);
		}
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		file.read((char*)&someDataSize, 4);

		if (someDataSize > 0) {
			someVerts.resize(numVertices);
			someTris.resize(numTriangles);

			for (int i = 0; i < numVertices; i++) {
				file.read((char*)&someVerts[i].x, 4);
				file.read((char*)&someVerts[i].y, 4);
				file.read((char*)&someVerts[i].z, 4);
			}

			for (int i = 0; i < numTriangles; i++) {
				file.read((char*)&someTris[i].p1, 2);
				file.read((char*)&someTris[i].p2, 2);
				file.read((char*)&someTris[i].p3, 2);
			}
		}
	}
}

void BSTriShape::Put(fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Put(file);

	file.write((char*)&flags, 4);

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

	file.write((char*)&bounds, 16);

	file.write((char*)&skinInstanceRef, 4);
	file.write((char*)&shaderPropertyRef, 4);

	file.write((char*)&alphaPropertyRef, 4);
	file.write((char*)&vertFlags1, 1);
	file.write((char*)&vertFlags2, 1);
	file.write((char*)&vertFlags3, 1);
	file.write((char*)&vertFlags4, 1);
	file.write((char*)&vertFlags5, 1);
	file.write((char*)&vertFlags6, 1);
	file.write((char*)&vertFlags7, 1);
	file.write((char*)&vertFlags8, 1);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130 && IsSkinned()) {
		// Triangle and vertex data is in partition instead
		ushort numUShort = 0;
		uint numUInt = 0;
		file.write((char*)&numUShort, 2);

		if (blockType == BSDYNAMICTRISHAPE)
			file.write((char*)&numVertices, 2);
		else
			file.write((char*)&numUShort, 2);

		file.write((char*)&numUInt, 4);
	}
	else {
		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130) {
			ushort numUShort = numTriangles;
			file.write((char*)&numUShort, 2);
		}
		else
			file.write((char*)&numTriangles, 4);

		file.write((char*)&numVertices, 2);
		file.write((char*)&dataSize, 4);

		if (dataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				if (HasVertices()) {
					if (IsFullPrecision()) {
						// Full precision
						file.write((char*)&vertData[i].vert.x, 4);
						file.write((char*)&vertData[i].vert.y, 4);
						file.write((char*)&vertData[i].vert.z, 4);

						file.write((char*)&vertData[i].bitangentX, 4);
					}
					else {
						// Half precision
						halfData = vertData[i].vert.x;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.y;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.z;
						file.write((char*)&halfData, 2);

						halfData = vertData[i].bitangentX;
						file.write((char*)&halfData, 2);
					}
				}

				if (HasUVs()) {
					halfData = vertData[i].uv.u;
					file.write((char*)&halfData, 2);

					halfData = vertData[i].uv.v;
					file.write((char*)&halfData, 2);
				}

				if (HasNormals()) {
					for (int j = 0; j < 3; j++)
						file.write((char*)&vertData[i].normal[j], 1);

					file.write((char*)&vertData[i].bitangentY, 1);

					if (HasTangents()) {
						for (int j = 0; j < 3; j++)
							file.write((char*)&vertData[i].tangent[j], 1);

						file.write((char*)&vertData[i].bitangentZ, 1);
					}
				}

				if (HasVertexColors())
					file.write((char*)&vertData[i].colorData, 4);

				if (IsSkinned()) {
					for (int j = 0; j < 4; j++) {
						halfData = vertData[i].weights[j];
						file.write((char*)&halfData, 2);
					}

					for (int j = 0; j < 4; j++)
						file.write((char*)&vertData[i].weightBones[j], 1);
				}

				if ((vertFlags7 & (1 << 4)) != 0)
					file.write((char*)&vertData[i].eyeData, 4);
			}
		}

		if (dataSize > 0) {
			for (int i = 0; i < numTriangles; i++) {
				file.write((char*)&triangles[i].p1, 2);
				file.write((char*)&triangles[i].p2, 2);
				file.write((char*)&triangles[i].p3, 2);
			}
		}
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		file.write((char*)&someDataSize, 4);

		if (someDataSize > 0) {
			for (int i = 0; i < numVertices; i++) {
				file.write((char*)&someVerts[i].x, 4);
				file.write((char*)&someVerts[i].y, 4);
				file.write((char*)&someVerts[i].z, 4);
			}

			for (int i = 0; i < numTriangles; i++) {
				file.write((char*)&someTris[i].p1, 2);
				file.write((char*)&someTris[i].p2, 2);
				file.write((char*)&someTris[i].p3, 2);
			}
		}
	}
}

void BSTriShape::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (skinInstanceRef == blockID)
		skinInstanceRef = 0xFFFFFFFF;
	else if (skinInstanceRef > blockID)
		skinInstanceRef--;

	if (shaderPropertyRef == blockID)
		shaderPropertyRef = 0xFFFFFFFF;
	else if (shaderPropertyRef > blockID)
		shaderPropertyRef--;

	if (alphaPropertyRef == blockID)
		alphaPropertyRef = 0xFFFFFFFF;
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

	deletedTris.clear();

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
			deletedTris.push_back(i);
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

void BSTriShape::GetChildRefs(set<int>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(skinInstanceRef);
	refs.insert(shaderPropertyRef);
	refs.insert(alphaPropertyRef);
}

int BSTriShape::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 42;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130)
		blockSize += 2;
	else
		blockSize += 4;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130 && IsSkinned())
		CalcDataSizes();
	else
		blockSize += CalcDataSizes();

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		blockSize += 4;

		if (someDataSize > 0) {
			blockSize += 12 * numVertices;
			blockSize += 6 * numTriangles;
		}
	}

	return blockSize;
}

const vector<Vector3>* BSTriShape::GetRawVerts() {
	rawVertices.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawVertices[i] = vertData[i].vert;

	return &rawVertices;
}

const vector<Vector3>* BSTriShape::GetNormalData(bool xform) {
	if (!HasNormals())
		return nullptr;

	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float q1 = (((float)vertData[i].normal[0]) / 255.0f) * 2.0f - 1.0f;
		float q2 = (((float)vertData[i].normal[1]) / 255.0f) * 2.0f - 1.0f;
		float q3 = (((float)vertData[i].normal[2]) / 255.0f) * 2.0f - 1.0f;

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
	if (!HasTangents())
		return nullptr;

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
	if (!HasTangents())
		return nullptr;

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
	if (!HasUVs())
		return nullptr;

	rawUvs.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawUvs[i] = vertData[i].uv;

	return &rawUvs;
}

void BSTriShape::SetVertices(const bool& enable) {
	if (enable) {
		vertFlags6 |= 1 << 4;
		vertData.resize(numVertices);
	}
	else {
		vertFlags6 &= ~(1 << 4);
		vertData.clear();
		numVertices = 0;

		SetUVs(false);
		SetNormals(false);
		SetTangents(false);
		SetVertexColors(false);
		SetSkinned(false);
	}
}

void BSTriShape::SetUVs(const bool& enable) {
	if (enable)
		vertFlags6 |= 1 << 5;
	else
		vertFlags6 &= ~(1 << 5);
}

void BSTriShape::SetNormals(const bool& enable) {
	if (enable)
		vertFlags6 |= 1 << 7;
	else
		vertFlags6 &= ~(1 << 7);
}

void BSTriShape::SetTangents(const bool& enable) {
	if (enable)
		vertFlags7 |= 1 << 0;
	else
		vertFlags7 &= ~(1 << 0);
}

void BSTriShape::SetVertexColors(const bool& enable) {
	if (enable)
		vertFlags7 |= 1 << 1;
	else
		vertFlags7 &= ~(1 << 1);
}

void BSTriShape::SetSkinned(const bool& enable) {
	if (enable)
		vertFlags7 |= 1 << 2;
	else
		vertFlags7 &= ~(1 << 2);
}

void BSTriShape::SetFullPrecision(const bool& enable) {
	if (!CanChangePrecision())
		return;

	if (enable)
		vertFlags7 |= 1 << 6;
	else
		vertFlags7 &= ~(1 << 6);
}

void BSTriShape::UpdateBounds() {
	const vector<Vector3>* vertices = GetRawVerts();
	if (vertices)
		bounds = BoundingSphere(*vertices);
	else
		bounds = BoundingSphere();
}

void BSTriShape::SetNormals(const vector<Vector3>& inNorms) {
	SetNormals(true);

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
	SetNormals(true);

	vector<Vector3> verts(numVertices);
	vector<Vector3> norms(numVertices);
	for (int i = 0; i < numVertices; i++) {
		verts[i].x = rawVertices[i].x * -0.1f;
		verts[i].z = rawVertices[i].y * 0.1f;
		verts[i].y = rawVertices[i].z * 0.1f;
	}

	// Face normals
	Vector3 tn;
	for (int t = 0; t < numTriangles; t++) {
		triangles[t].trinormal(verts, &tn);
		norms[triangles[t].p1] += tn;
		norms[triangles[t].p2] += tn;
		norms[triangles[t].p3] += tn;
	}

	for (auto &n : norms)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(verts.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			pair<Vector3*, int>& a = matcher.matches[i].first;
			pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = norms[a.second];
			Vector3& bn = norms[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : norms)
			n.Normalize();
	}

	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawNormals[i].x = -norms[i].x;
		rawNormals[i].y = norms[i].z;
		rawNormals[i].z = norms[i].y;
		vertData[i].normal[0] = (unsigned char)round((((rawNormals[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[1] = (unsigned char)round((((rawNormals[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[2] = (unsigned char)round((((rawNormals[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
		return;

	GetNormalData(false);
	SetTangents(true);

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

		float r = (s1 * t2 - s2 * t1);
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

void BSTriShape::UpdateFlags() {
	if (IsFullPrecision()) {
		if (HasNormals()) {
			vertFlags3 = 4;

			if (HasTangents())
				vertFlags3 = 101;
		}
		else
			vertFlags3 = 0;

		if (HasTangents()) {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 135;
			else if (IsSkinned())
				vertFlags4 = 112;
			else if (HasVertexColors())
				vertFlags4 = 7;
			else
				vertFlags4 = 0;
		}
		else {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 101;
			else if (IsSkinned())
				vertFlags4 = 80;
			else if (HasVertexColors())
				vertFlags4 = 5;
			else
				vertFlags4 = 0;
		}
	}
	else {
		if (HasNormals()) {
			vertFlags3 = 2;

			if (HasTangents())
				vertFlags3 = 67;
		}
		else
			vertFlags3 = 0;

		if (HasTangents()) {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 101;
			else if (IsSkinned())
				vertFlags4 = 80;
			else if (HasVertexColors())
				vertFlags4 = 5;
			else
				vertFlags4 = 0;
		}
		else {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 67;
			else if (IsSkinned())
				vertFlags4 = 48;
			else if (HasVertexColors())
				vertFlags4 = 3;
			else
				vertFlags4 = 0;
		}
	}
}

int BSTriShape::CalcDataSizes() {
	vertexSize = 0;
	dataSize = 0;

	if (HasVertices()) {
		if (IsFullPrecision()) {	// Position + Bitangent X
			vertexSize += 4;
			vertFlags2 = 4;
		}
		else {
			vertexSize += 2;
			vertFlags2 = 2;
		}
	}
	else
		vertFlags2 = 0;

	if (HasUVs())
		vertexSize += 1;		// UVs

	if (HasNormals()) {			// Normals + Bitangent Y
		vertexSize += 1;

		if (HasTangents())		// Tangents + Bitangent Z
			vertexSize += 1;
	}

	if (HasVertexColors())		// Vertex Colors
		vertexSize += 1;

	if (IsSkinned())			// Skinning
		vertexSize += 3;

	if ((vertFlags7 & (1 << 4)) != 0)	// Eye Data
		vertexSize += 1;

	vertFlags1 = vertexSize;

	if (blockType == BSDYNAMICTRISHAPE)
		vertFlags1 = (vertFlags1 & 0xF) | 0x40;

	vertexSize *= 4;
	dataSize = vertexSize * numVertices + 6 * numTriangles;

	UpdateFlags();

	return dataSize;
}

void BSTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	flags = 14;
	numVertices = verts->size();
	numTriangles = tris->size();

	vertData.resize(numVertices);

	if (uvs && uvs->size() != numVertices)
		SetUVs(false);

	for (int i = 0; i < numVertices; i++) {
		vertData[i].vert = (*verts)[i];

		if (uvs && uvs->size() == numVertices)
			vertData[i].uv = (*uvs)[i];

		vertData[i].bitangentX = 0.0f;
		vertData[i].bitangentY = 0;
		vertData[i].bitangentZ = 0;
		vertData[i].normal[0] = vertData[i].normal[1] = vertData[i].normal[2] = 0;
		memset(vertData[i].colorData, 255, 4);
		memset(vertData[i].weights, 0, sizeof(float) * 4);
		memset(vertData[i].weightBones, 0, 4);
	}

	triangles.resize(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		triangles[i] = (*tris)[i];

	bounds = BoundingSphere(*verts);

	if (normals && normals->size() == numVertices) {
		SetNormals(*normals);
		CalcTangentSpace();
	}
	else {
		SetNormals(false);
		SetTangents(false);
	}
}


BSSubIndexTriShape::BSSubIndexTriShape(NiHeader& hdr) : BSTriShape(hdr) {
	blockType = BSSUBINDEXTRISHAPE;
}

BSSubIndexTriShape::BSSubIndexTriShape(fstream& file, NiHeader& hdr) : BSTriShape(file, hdr) {
	blockType = BSSUBINDEXTRISHAPE;
	Get(file);
}

void BSSubIndexTriShape::Get(fstream& file) {
	if (dataSize <= 0)
		return;

	file.read((char*)&segmentation.numPrimitives, 4);
	file.read((char*)&segmentation.numSegments, 4);
	file.read((char*)&segmentation.numTotalSegments, 4);

	segmentation.segments.resize(segmentation.numSegments);
	for (auto &segment : segmentation.segments) {
		file.read((char*)&segment.startIndex, 4);
		file.read((char*)&segment.numPrimitives, 4);
		file.read((char*)&segment.parentArrayIndex, 4);
		file.read((char*)&segment.numSubSegments, 4);

		segment.subSegments.resize(segment.numSubSegments);
		for (auto &subSegment : segment.subSegments) {
			file.read((char*)&subSegment.startIndex, 4);
			file.read((char*)&subSegment.numPrimitives, 4);
			file.read((char*)&subSegment.arrayIndex, 4);
			file.read((char*)&subSegment.unkInt1, 4);
		}
	}

	if (segmentation.numSegments < segmentation.numTotalSegments) {
		file.read((char*)&segmentation.subSegmentData.numSegments, 4);
		file.read((char*)&segmentation.subSegmentData.numTotalSegments, 4);

		segmentation.subSegmentData.arrayIndices.resize(segmentation.numSegments);
		for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
			file.read((char*)&arrayIndex, 4);

		segmentation.subSegmentData.dataRecords.resize(segmentation.numTotalSegments);
		for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
			file.read((char*)&dataRecord.segmentUser, 4);
			file.read((char*)&dataRecord.unkInt2, 4);
			file.read((char*)&dataRecord.numData, 4);

			dataRecord.extraData.resize(dataRecord.numData);
			for (auto &data : dataRecord.extraData)
				file.read((char*)&data, 4);
		}

		segmentation.subSegmentData.ssfFile.Get(file, 2);
	}
}

void BSSubIndexTriShape::Put(fstream& file) {
	BSTriShape::Put(file);

	if (dataSize <= 0)
		return;

	file.write((char*)&segmentation.numPrimitives, 4);
	file.write((char*)&segmentation.numSegments, 4);
	file.write((char*)&segmentation.numTotalSegments, 4);

	for (auto &segment : segmentation.segments) {
		file.write((char*)&segment.startIndex, 4);
		file.write((char*)&segment.numPrimitives, 4);
		file.write((char*)&segment.parentArrayIndex, 4);
		file.write((char*)&segment.numSubSegments, 4);

		for (auto &subSegment : segment.subSegments) {
			file.write((char*)&subSegment.startIndex, 4);
			file.write((char*)&subSegment.numPrimitives, 4);
			file.write((char*)&subSegment.arrayIndex, 4);
			file.write((char*)&subSegment.unkInt1, 4);
		}
	}

	if (segmentation.numSegments < segmentation.numTotalSegments) {
		file.write((char*)&segmentation.subSegmentData.numSegments, 4);
		file.write((char*)&segmentation.subSegmentData.numTotalSegments, 4);

		for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
			file.write((char*)&arrayIndex, 4);

		for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
			file.write((char*)&dataRecord.segmentUser, 4);
			file.write((char*)&dataRecord.unkInt2, 4);
			file.write((char*)&dataRecord.numData, 4);

			for (auto &data : dataRecord.extraData)
				file.write((char*)&data, 4);
		}

		segmentation.subSegmentData.ssfFile.Put(file, 2, false);
	}
}

void BSSubIndexTriShape::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	//Remove triangles from segments and re-fit lists
	segmentation.numPrimitives -= deletedTris.size();
	for (auto &segment : segmentation.segments) {
		// Delete primitives
		for (auto &id : deletedTris)
			if (segment.numPrimitives > 0 && id >= segment.startIndex / 3 && id < segment.startIndex / 3 + segment.numPrimitives)
				segment.numPrimitives--;

		// Align sub segments
		for (auto &subSegment : segment.subSegments)
			for (auto &id : deletedTris)
				if (subSegment.numPrimitives > 0 && id >= subSegment.startIndex / 3 && id < subSegment.startIndex / 3 + subSegment.numPrimitives)
					subSegment.numPrimitives--;
	}

	// Align segments
	int i = 0;
	for (auto &segment : segmentation.segments) {
		// Align sub segments
		int j = 0;
		for (auto &subSegment : segment.subSegments) {
			if (j == 0)
				subSegment.startIndex = segment.startIndex;

			if (j + 1 >= segment.numSubSegments)
				continue;

			BSSITSSubSegment& nextSubSegment = segment.subSegments[j + 1];
			nextSubSegment.startIndex = subSegment.startIndex + subSegment.numPrimitives * 3;
			j++;
		}

		if (i + 1 >= segmentation.numSegments)
			continue;

		BSSITSSegment& nextSegment = segmentation.segments[i + 1];
		nextSegment.startIndex = segment.startIndex + segment.numPrimitives * 3;

		i++;
	}
}

int BSSubIndexTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	if (dataSize > 0) {
		blockSize += 12;

		blockSize += segmentation.numSegments * 16;
		for (auto &segment : segmentation.segments)
			blockSize += segment.numSubSegments * 16;

		if (segmentation.numSegments < segmentation.numTotalSegments) {
			blockSize += 10;
			blockSize += segmentation.subSegmentData.numSegments * 4;
			blockSize += segmentation.subSegmentData.numTotalSegments * 12;

			for (auto &dataRecord : segmentation.subSegmentData.dataRecords)
				blockSize += dataRecord.numData * 4;

			blockSize += segmentation.subSegmentData.ssfFile.GetLength();
		}
	}

	return blockSize;
}

void BSSubIndexTriShape::SetDefaultSegments() {
	segmentation.numPrimitives = numTriangles;
	segmentation.numSegments = 4;
	segmentation.numTotalSegments = 4;

	segmentation.subSegmentData.numSegments = 0;
	segmentation.subSegmentData.numTotalSegments = 0;

	segmentation.subSegmentData.arrayIndices.clear();
	segmentation.subSegmentData.dataRecords.clear();
	segmentation.subSegmentData.ssfFile.Clear();

	segmentation.segments.resize(4);
	for (int i = 0; i < 3; i++) {
		segmentation.segments[i].startIndex = 0;
		segmentation.segments[i].numPrimitives = 0;
		segmentation.segments[i].parentArrayIndex = 0xFFFFFFFF;
		segmentation.segments[i].numSubSegments = 0;
	}

	segmentation.segments[3].startIndex = 0;
	segmentation.segments[3].numPrimitives = numTriangles;
	segmentation.segments[3].parentArrayIndex = 0xFFFFFFFF;
	segmentation.segments[3].numSubSegments = 0;
}

void BSSubIndexTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	// Skinned most of the time
	SetSkinned(true);
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

int BSMeshLODTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


BSDynamicTriShape::BSDynamicTriShape(NiHeader& hdr) : BSTriShape(hdr) {
	blockType = BSDYNAMICTRISHAPE;

	dynamicDataSize = 0;
}

BSDynamicTriShape::BSDynamicTriShape(fstream& file, NiHeader& hdr) : BSTriShape(file, hdr) {
	blockType = BSDYNAMICTRISHAPE;

	Get(file);
}

void BSDynamicTriShape::Get(fstream& file) {
	file.read((char*)&dynamicDataSize, 4);

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		file.read((char*)&dynamicData[i].x, 4);
		file.read((char*)&dynamicData[i].y, 4);
		file.read((char*)&dynamicData[i].z, 4);
		file.read((char*)&dynamicData[i].w, 4);
	}
}

void BSDynamicTriShape::Put(fstream& file) {
	BSTriShape::Put(file);

	file.write((char*)&dynamicDataSize, 4);

	for (int i = 0; i < numVertices; i++) {
		file.write((char*)&dynamicData[i].x, 4);
		file.write((char*)&dynamicData[i].y, 4);
		file.write((char*)&dynamicData[i].z, 4);
		file.write((char*)&dynamicData[i].w, 4);
	}
}

void BSDynamicTriShape::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	vector<int> indexCollapse(dynamicData.size(), 0);
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {
			indexCollapse[i] = -1;
			j++;
		}
	}

	for (int i = dynamicData.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			dynamicData.erase(dynamicData.begin() + i);
			dynamicDataSize--;
		}
	}
}

int BSDynamicTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	dynamicDataSize = numVertices * 16;

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		dynamicData[i].x = vertData[i].vert.x;
		dynamicData[i].y = vertData[i].vert.y;
		dynamicData[i].z = vertData[i].vert.z;
	}

	blockSize += 4;
	blockSize += dynamicDataSize;

	return blockSize;
}

void BSDynamicTriShape::Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	dynamicDataSize = verts->size();

	dynamicData.resize(dynamicDataSize);
	for (int i = 0; i < dynamicDataSize; i++) {
		dynamicData[i].x = (*verts)[i].x;
		dynamicData[i].y = (*verts)[i].y;
		dynamicData[i].z = (*verts)[i].z;
		dynamicData[i].w = 0.0f;
	}
}


void NiGeometry::Init() {
	NiAVObject::Init();

	shaderPropertyRef = 0xFFFFFFFF;
	alphaPropertyRef = 0xFFFFFFFF;
	dataRef = 0xFFFFFFFF;
	skinInstanceRef = 0xFFFFFFFF;
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
	for (int i = 0; i < numMaterials; i++) {
		file.read((char*)&intData, 4);
		materialExtra.push_back(intData);
	}

	file.read((char*)&activeMaterial, 4);
	file.read((char*)&dirty, 1);

	if (header->GetUserVersion() > 11) {
		file.read((char*)&shaderPropertyRef, 4);
		file.read((char*)&alphaPropertyRef, 4);
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

	if (header->GetUserVersion() > 11) {
		file.write((char*)&shaderPropertyRef, 4);
		file.write((char*)&alphaPropertyRef, 4);
	}
}

void NiGeometry::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;

	if (skinInstanceRef == blockID)
		skinInstanceRef = 0xFFFFFFFF;
	else if (skinInstanceRef > blockID)
		skinInstanceRef--;

	if (shaderPropertyRef == blockID)
		shaderPropertyRef = 0xFFFFFFFF;
	else if (shaderPropertyRef > blockID)
		shaderPropertyRef--;

	if (alphaPropertyRef == blockID)
		alphaPropertyRef = 0xFFFFFFFF;
	else if (alphaPropertyRef > blockID)
		alphaPropertyRef--;
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

	if (shaderPropertyRef == blockIndexLo)
		shaderPropertyRef = blockIndexHi;
	else if (shaderPropertyRef == blockIndexHi)
		shaderPropertyRef = blockIndexLo;

	if (alphaPropertyRef == blockIndexLo)
		alphaPropertyRef = blockIndexHi;
	else if (alphaPropertyRef == blockIndexHi)
		alphaPropertyRef = blockIndexLo;
}

void NiGeometry::GetChildRefs(set<int>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(dataRef);
	refs.insert(skinInstanceRef);
	refs.insert(shaderPropertyRef);
	refs.insert(alphaPropertyRef);
}

int NiGeometry::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 17;
	blockSize += numMaterials * 2;
	blockSize += numMaterials * 4;
	if (header->GetUserVersion() > 11)
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
	materialCRC = 0;
	hasNormals = false;
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

	if (hasVertices && !isPSys) {
		vertices.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&vertices[i], 12);
	}

	file.read((char*)&numUVSets, 2);

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (header->GetUserVersion2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (header->GetUserVersion() == 12)
		file.read((char*)&materialCRC, 4);

	file.read((char*)&hasNormals, 1);
	if (hasNormals && !isPSys) {
		normals.resize(numVertices);

		for (int i = 0; i < numVertices; i++)
			file.read((char*)&normals[i], 12);

		if (nbtMethod) {
			tangents.resize(numVertices);
			bitangents.resize(numVertices);

			for (int i = 0; i < numVertices; i++)
				file.read((char*)&tangents[i], 12);

			for (int i = 0; i < numVertices; i++)
				file.read((char*)&bitangents[i], 12);
		}
	}

	file.read((char*)&bounds, 16);

	file.read((char*)&hasVertexColors, 1);
	if (hasVertexColors && !isPSys) {
		vertexColors.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&vertexColors[i], 16);
	}

	if (numTextureSets > 0 && !isPSys) {
		uvSets.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&uvSets[i], 8);
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

	if (hasVertices && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&vertices[i], 12);
	}

	file.write((char*)&numUVSets, 2);

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (header->GetUserVersion2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (header->GetUserVersion() == 12)
		file.write((char*)&materialCRC, 4);

	file.write((char*)&hasNormals, 1);
	if (hasNormals && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&normals[i], 12);

		if (nbtMethod) {
			for (int i = 0; i < numVertices; i++)
				file.write((char*)&tangents[i], 12);

			for (int i = 0; i < numVertices; i++)
				file.write((char*)&bitangents[i], 12);
		}
	}

	file.write((char*)&bounds, 16);

	file.write((char*)&hasVertexColors, 1);
	if (hasVertexColors && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&vertexColors[i], 16);
	}

	if (numTextureSets > 0 && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&uvSets[i], 8);
	}

	file.write((char*)&consistencyFlags, 2);
	file.write((char*)&additionalData, 4);
}

void NiGeometryData::SetVertices(const bool& enable) {
	hasVertices = enable;
	if (enable) {
		vertices.resize(numVertices);
	}
	else {
		vertices.clear();
		numVertices = 0;

		SetNormals(false);
		SetVertexColors(false);
		SetUVs(false);
		SetTangents(false);
	}
}

void NiGeometryData::SetNormals(const bool& enable) {
	hasNormals = enable;
	if (enable)
		normals.resize(numVertices);
	else
		normals.clear();
}

void NiGeometryData::SetVertexColors(const bool& enable) {
	hasVertexColors = enable;
	if (enable)
		vertexColors.resize(numVertices);
	else
		vertexColors.clear();
}

void NiGeometryData::SetUVs(const bool& enable) {
	if (enable) {
		numUVSets |= 1 << 0;
		uvSets.resize(numVertices);
	}
	else {
		numUVSets &= ~(1 << 0);
		uvSets.clear();
	}
}

void NiGeometryData::SetTangents(const bool& enable) {
	if (enable) {
		numUVSets |= 1 << 12;
		tangents.resize(numVertices);
		bitangents.resize(numVertices);
	}
	else {
		numUVSets &= ~(1 << 12);
		tangents.clear();
		bitangents.clear();
	}
}

void NiGeometryData::UpdateBounds() {
	bounds = BoundingSphere(vertices);
}

void NiGeometryData::Create(vector<Vector3>* verts, vector<Triangle>* inTris, vector<Vector2>* texcoords) {
	unkInt = 0;
	keepFlags = 0;
	compressFlags = 0;

	hasVertices = true;
	numVertices = verts->size();
	for (auto &v : (*verts))
		vertices.push_back(v);

	if (texcoords->size() > 0)
		numUVSets = 4097;
	else
		numUVSets = 0;

	materialCRC = 0;
	hasNormals = false;
	hasVertexColors = false;

	bounds = BoundingSphere(*verts);

	for (auto &uv : *texcoords)
		uvSets.push_back(uv);

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

void NiGeometryData::RecalcNormals(const bool& smooth, const float& smoothThresh) {
	SetNormals(true);
}

void NiGeometryData::CalcTangentSpace() {
	SetTangents(true);
}

int NiGeometryData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 35;
	if (header->GetUserVersion() == 12)
		blockSize += 4;

	if (hasVertices && !isPSys) {
		ushort nbtMethod = numUVSets & 0xF000;
		byte numTextureSets = numUVSets & 0x3F;
		if (header->GetUserVersion2() >= 83)
			numTextureSets = numUVSets & 0x1;

		blockSize += numVertices * 12;

		if (hasNormals) {
			blockSize += normals.size() * 12;

			if (nbtMethod) {
				blockSize += tangents.size() * 12;
				blockSize += bitangents.size() * 12;
			}
		}

		if (hasVertexColors)
			blockSize += vertexColors.size() * 16;

		if (numTextureSets > 0)
			blockSize += uvSets.size() * 8;
	}

	return blockSize;
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
	if (hasTriangles) {
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
	if (hasTriangles) {
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

void NiTriShapeData::RecalcNormals(const bool& smooth, const float& smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	// Zero out existing normals
	for (auto &n : normals)
		n.Zero();

	// Face normals
	Vector3 tn;
	for (int t = 0; t < numTriangles; t++) {
		triangles[t].trinormal(vertices, &tn);
		normals[triangles[t].p1] += tn;
		normals[triangles[t].p2] += tn;
		normals[triangles[t].p3] += tn;
	}

	for (auto &n : normals)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(vertices.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			pair<Vector3*, int>& a = matcher.matches[i].first;
			pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = normals[a.second];
			Vector3& bn = normals[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : normals)
			n.Normalize();
	}
}

void NiTriShapeData::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
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

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		bitangents[i] = tan1[i];
		tangents[i] = tan2[i];

		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = normals[i].y;
			tangents[i].y = normals[i].z;
			tangents[i].z = normals[i].x;
			bitangents[i] = normals[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - normals[i] * normals[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - normals[i] * normals[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
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

void NiTriStripsData::RecalcNormals(const bool& smooth, const float& smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	vector<Triangle> tris;
	StripsToTris(&tris);

	// Zero out existing normals
	for (auto &n : normals)
		n.Zero();

	// Face normals
	Vector3 tn;
	for (int t = 0; t < numTriangles; t++) {
		tris[t].trinormal(vertices, &tn);
		normals[tris[t].p1] += tn;
		normals[tris[t].p2] += tn;
		normals[tris[t].p3] += tn;
	}

	for (auto &n : normals)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(vertices.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			pair<Vector3*, int>& a = matcher.matches[i].first;
			pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = normals[a.second];
			Vector3& bn = normals[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : normals)
			n.Normalize();
	}
}

void NiTriStripsData::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
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

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		bitangents[i] = tan1[i];
		tangents[i] = tan2[i];

		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = normals[i].y;
			tangents[i].y = normals[i].z;
			tangents[i].z = normals[i].x;
			bitangents[i] = normals[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - normals[i] * normals[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - normals[i] * normals[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
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


BSLODTriShape::BSLODTriShape(NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = BSLODTRISHAPE;
}

BSLODTriShape::BSLODTriShape(fstream& file, NiHeader& hdr) {
	NiTriBasedGeom::Init();

	header = &hdr;
	blockType = BSLODTRISHAPE;

	Get(file);
}

void BSLODTriShape::Get(fstream& file) {
	NiTriBasedGeom::Get(file);

	file.read((char*)&level0, 4);
	file.read((char*)&level1, 4);
	file.read((char*)&level2, 4);
}

void BSLODTriShape::Put(fstream& file) {
	NiTriBasedGeom::Put(file);

	file.write((char*)&level0, 4);
	file.write((char*)&level1, 4);
	file.write((char*)&level2, 4);
}

int BSLODTriShape::CalcBlockSize() {
	NiTriBasedGeom::CalcBlockSize();

	blockSize += 12;

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

	dataRef = 0xFFFFFFFF;
	skinPartitionRef = 0xFFFFFFFF;
	skeletonRootRef = 0xFFFFFFFF;
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

	int boneIndex = 0xFFFFFFFF;
	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;

	if (skinPartitionRef == blockID)
		skinPartitionRef = 0xFFFFFFFF;
	else if (skinPartitionRef > blockID)
		skinPartitionRef--;

	if (skeletonRootRef == blockID)
		skeletonRootRef = 0xFFFFFFFF;
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
	if (boneIndex >= 0 && dataRef != 0xFFFFFFFF) { // Bone was removed, clear out the skinning data for it.
		auto skinData = header->GetBlock<NiSkinData>(dataRef);
		if (!skinData)
			return;

		skinData->bones.erase(skinData->bones.begin() + boneIndex);
		skinData->numBones--;
	}
}

void NiSkinInstance::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;

	if (skinPartitionRef == blockIndexLo)
		skinPartitionRef = blockIndexHi;
	else if (skinPartitionRef == blockIndexHi)
		skinPartitionRef = blockIndexLo;

	if (skeletonRootRef == blockIndexLo)
		skeletonRootRef = blockIndexHi;
	else if (skeletonRootRef == blockIndexHi)
		skeletonRootRef = blockIndexLo;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockIndexLo)
			bones[i] = blockIndexHi;
		else if (bones[i] == blockIndexHi)
			bones[i] = blockIndexLo;
	}
}

void NiSkinInstance::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(dataRef);
	refs.insert(skinPartitionRef);
	refs.insert(skeletonRootRef);
	refs.insert(bones.begin(), bones.end());
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

	file.read((char*)&numPartitions, 4);
	partitions.resize(numPartitions);

	for (int i = 0; i < numPartitions; i++) {
		PartitionInfo part;
		file.read((char*)&part.flags, 2);
		file.read((char*)&part.partID, 2);
		partitions[i] = part;
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

int BSDismemberSkinInstance::CalcBlockSize() {
	NiSkinInstance::CalcBlockSize();

	blockSize += 4;
	blockSize += numPartitions * 4;
	return blockSize;
}

void BSDismemberSkinInstance::AddPartition(const BSDismemberSkinInstance::PartitionInfo& part) {
	partitions.push_back(part);
	numPartitions++;
}

void BSDismemberSkinInstance::RemovePartition(const int& id) {
	if (id >= 0 && id < numPartitions) {
		partitions.erase(partitions.begin() + id);
		numPartitions--;
	}
}

void BSDismemberSkinInstance::ClearPartitions() {
	partitions.clear();
	numPartitions = 0;
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
	dataRef = 0xFFFFFFFF;
	numBones = 0;
	numUnk = 0;
}

void BSSkinInstance::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&targetRef, 4);
	file.read((char*)&dataRef, 4);

	file.read((char*)&numBones, 4);
	bones.resize(numBones);
	for (int i = 0; i < numBones; i++)
		file.read((char*)&bones[i], 4);

	file.read((char*)&numUnk, 4);
	unk.resize(numUnk);
	for (int i = 0; i < numUnk; i++)
		file.read((char*)&unk[i], 12);
}

void BSSkinInstance::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&targetRef, 4);
	file.write((char*)&dataRef, 4);
	file.write((char*)&numBones, 4);
	for (int i = 0; i < numBones; i++)
		file.write((char*)&bones[i], 4);

	file.write((char*)&numUnk, 4);
	for (int i = 0; i < numUnk; i++)
		file.write((char*)&unk[i], 12);
}

void BSSkinInstance::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	int boneIndex = 0xFFFFFFFF;
	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;

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

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;

	for (int i = 0; i < numBones; i++) {
		if (bones[i] == blockIndexLo)
			bones[i] = blockIndexHi;
		else if (bones[i] == blockIndexHi)
			bones[i] = blockIndexLo;
	}
}

void BSSkinInstance::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(dataRef);
	refs.insert(bones.begin(), bones.end());
}

int BSSkinInstance::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 16;
	blockSize += numBones * 4;
	blockSize += numUnk * 12;

	return blockSize;
}

BSSkinBoneData::BSSkinBoneData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSBONEDATA;

	nBones = 0;
}

BSSkinBoneData::BSSkinBoneData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSBONEDATA;

	Get(file);
}

void BSSkinBoneData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nBones, 4);
	boneXforms.resize(nBones);
	for (int i = 0; i < nBones; i++) {
		file.read((char*)&boneXforms[i].bounds, 16);
		file.read((char*)&boneXforms[i].boneTransform, 52);
	}
}

void BSSkinBoneData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nBones, 4);

	for (int i = 0; i < nBones; i++) {
		file.write((char*)&boneXforms[i].bounds, 16);
		file.write((char*)&boneXforms[i].boneTransform, 52);
	}
}

int BSSkinBoneData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;
	blockSize += 68 * nBones;

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
		file.read((char*)&boneData.bounds, 16);
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
		file.write((char*)&bones[i].bounds, 16);
		file.write((char*)&bones[i].numVertices, 2);

		for (int j = 0; j < bones[i].numVertices; j++) {
			file.write((char*)&bones[i].vertexWeights[j].index, 2);
			file.write((char*)&bones[i].vertexWeights[j].weight, 4);
		}
	}
}

void NiSkinData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
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
	for (auto &b : bones)
		blockSize += 70 + b.numVertices * 6;

	return blockSize;
}


NiSkinPartition::NiSkinPartition(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINPARTITION;

	numPartitions = 0;
	dataSize = 0;
	vertexSize = 0;
	vertFlags1 = 0;
	vertFlags2 = 0;
	vertFlags3 = 0;
	vertFlags4 = 0;
	vertFlags5 = 0;
	vertFlags6 = 0;
	vertFlags7 = 0;
	vertFlags8 = 0;
	numVertices = 0;
}

NiSkinPartition::NiSkinPartition(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISKINPARTITION;

	dataSize = 0;
	vertexSize = 0;
	vertFlags1 = 0;
	vertFlags2 = 0;
	vertFlags3 = 0;
	vertFlags4 = 0;
	vertFlags5 = 0;
	vertFlags6 = 0;
	vertFlags7 = 0;
	vertFlags8 = 0;
	numVertices = 0;

	Get(file);
}

void NiSkinPartition::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numPartitions, 4);
	partitions.resize(numPartitions);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		file.read((char*)&dataSize, 4);
		file.read((char*)&vertexSize, 4);
		file.read((char*)&vertFlags1, 1);
		file.read((char*)&vertFlags2, 1);
		file.read((char*)&vertFlags3, 1);
		file.read((char*)&vertFlags4, 1);
		file.read((char*)&vertFlags5, 1);
		file.read((char*)&vertFlags6, 1);
		file.read((char*)&vertFlags7, 1);
		file.read((char*)&vertFlags8, 1);

		if (dataSize > 0) {
			numVertices = dataSize / vertexSize;
			vertData.resize(numVertices);

			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				if (HasVertices()) {
					if (IsFullPrecision()) {
						// Full precision
						file.read((char*)&vertData[i].vert.x, 4);
						file.read((char*)&vertData[i].vert.y, 4);
						file.read((char*)&vertData[i].vert.z, 4);

						file.read((char*)&vertData[i].bitangentX, 4);
					}
					else {
						// Half precision
						file.read((char*)&halfData, 2);
						vertData[i].vert.x = halfData;
						file.read((char*)&halfData, 2);
						vertData[i].vert.y = halfData;
						file.read((char*)&halfData, 2);
						vertData[i].vert.z = halfData;

						file.read((char*)&halfData, 2);
						vertData[i].bitangentX = halfData;
					}
				}

				if (HasUVs()) {
					file.read((char*)&halfData, 2);
					vertData[i].uv.u = halfData;
					file.read((char*)&halfData, 2);
					vertData[i].uv.v = halfData;
				}

				if (HasNormals()) {
					for (int j = 0; j < 3; j++)
						file.read((char*)&vertData[i].normal[j], 1);

					file.read((char*)&vertData[i].bitangentY, 1);

					if (HasTangents()) {
						for (int j = 0; j < 3; j++)
							file.read((char*)&vertData[i].tangent[j], 1);

						file.read((char*)&vertData[i].bitangentZ, 1);
					}
				}

				if (HasVertexColors())
					file.read((char*)&vertData[i].colorData, 4);

				if (IsSkinned()) {
					for (int j = 0; j < 4; j++) {
						file.read((char*)&halfData, 2);
						vertData[i].weights[j] = halfData;
					}

					for (int j = 0; j < 4; j++)
						file.read((char*)&vertData[i].weightBones[j], 1);
				}

				if ((vertFlags7 & (1 << 4)) != 0)
					file.read((char*)&vertData[i].eyeData, 4);
			}
		}
	}

	for (int p = 0; p < numPartitions; p++) {
		PartitionBlock partition;
		file.read((char*)&partition.numVertices, 2);
		file.read((char*)&partition.numTriangles, 2);
		file.read((char*)&partition.numBones, 2);
		file.read((char*)&partition.numStrips, 2);
		file.read((char*)&partition.numWeightsPerVertex, 2);

		partition.bones.resize(partition.numBones);
		for (int i = 0; i < partition.numBones; i++)
			file.read((char*)&partition.bones[i], 2);

		file.read((char*)&partition.hasVertexMap, 1);
		if (partition.hasVertexMap) {
			partition.vertexMap.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.vertexMap[i], 2);
		}

		file.read((char*)&partition.hasVertexWeights, 1);
		if (partition.hasVertexWeights) {
			partition.vertexWeights.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.vertexWeights[i], 16);
		}

		partition.stripLengths.resize(partition.numStrips);
		for (int i = 0; i < partition.numStrips; i++)
			file.read((char*)&partition.stripLengths[i], 2);

		file.read((char*)&partition.hasFaces, 1);
		if (partition.hasFaces) {
			partition.strips.resize(partition.numStrips);
			for (int i = 0; i < partition.numStrips; i++) {
				partition.strips[i].resize(partition.stripLengths[i]);
				for (int j = 0; j < partition.stripLengths[i]; j++)
					file.read((char*)&partition.strips[i][j], 2);
			}
		}

		if (partition.numStrips == 0 && partition.hasFaces) {
			partition.triangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++)
				file.read((char*)&partition.triangles[i], 6);
		}

		file.read((char*)&partition.hasBoneIndices, 1);
		if (partition.hasBoneIndices) {
			partition.boneIndices.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.boneIndices[i], 4);
		}

		if (header->GetUserVersion() >= 12)
			file.read((char*)&partition.unkShort, 2);

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			file.read((char*)&partition.vertFlags1, 1);
			file.read((char*)&partition.vertFlags2, 1);
			file.read((char*)&partition.vertFlags3, 1);
			file.read((char*)&partition.vertFlags4, 1);
			file.read((char*)&partition.vertFlags5, 1);
			file.read((char*)&partition.vertFlags6, 1);
			file.read((char*)&partition.vertFlags7, 1);
			file.read((char*)&partition.vertFlags8, 1);

			partition.trueTriangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++) {
				file.read((char*)&partition.trueTriangles[i].p1, 2);
				file.read((char*)&partition.trueTriangles[i].p2, 2);
				file.read((char*)&partition.trueTriangles[i].p3, 2);
			}
		}

		partitions[p] = partition;
	}
}

void NiSkinPartition::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numPartitions, 4);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		file.write((char*)&dataSize, 4);
		file.write((char*)&vertexSize, 4);
		file.write((char*)&vertFlags1, 1);
		file.write((char*)&vertFlags2, 1);
		file.write((char*)&vertFlags3, 1);
		file.write((char*)&vertFlags4, 1);
		file.write((char*)&vertFlags5, 1);
		file.write((char*)&vertFlags6, 1);
		file.write((char*)&vertFlags7, 1);
		file.write((char*)&vertFlags8, 1);

		if (dataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				if (HasVertices()) {
					if (IsFullPrecision()) {
						// Full precision
						file.write((char*)&vertData[i].vert.x, 4);
						file.write((char*)&vertData[i].vert.y, 4);
						file.write((char*)&vertData[i].vert.z, 4);

						file.write((char*)&vertData[i].bitangentX, 4);
					}
					else {
						// Half precision
						halfData = vertData[i].vert.x;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.y;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.z;
						file.write((char*)&halfData, 2);

						halfData = vertData[i].bitangentX;
						file.write((char*)&halfData, 2);
					}
				}

				if (HasUVs()) {
					halfData = vertData[i].uv.u;
					file.write((char*)&halfData, 2);
					halfData = vertData[i].uv.v;
					file.write((char*)&halfData, 2);
				}

				if (HasNormals()) {
					for (int j = 0; j < 3; j++)
						file.write((char*)&vertData[i].normal[j], 1);

					file.write((char*)&vertData[i].bitangentY, 1);

					if (HasTangents()) {
						for (int j = 0; j < 3; j++)
							file.write((char*)&vertData[i].tangent[j], 1);

						file.write((char*)&vertData[i].bitangentZ, 1);
					}
				}

				if (HasVertexColors())
					file.write((char*)&vertData[i].colorData, 4);

				if (IsSkinned()) {
					for (int j = 0; j < 4; j++) {
						halfData = vertData[i].weights[j];
						file.write((char*)&halfData, 2);
					}

					for (int j = 0; j < 4; j++)
						file.write((char*)&vertData[i].weightBones[j], 1);
				}

				if ((vertFlags7 & (1 << 4)) != 0)
					file.write((char*)&vertData[i].eyeData, 4);
			}
		}
	}

	for (int p = 0; p < numPartitions; p++) {
		file.write((char*)&partitions[p].numVertices, 2);
		file.write((char*)&partitions[p].numTriangles, 2);
		file.write((char*)&partitions[p].numBones, 2);
		file.write((char*)&partitions[p].numStrips, 2);
		file.write((char*)&partitions[p].numWeightsPerVertex, 2);

		for (int i = 0; i < partitions[p].numBones; i++)
			file.write((char*)&partitions[p].bones[i], 2);

		file.write((char*)&partitions[p].hasVertexMap, 1);
		if (partitions[p].hasVertexMap)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].vertexMap[i], 2);

		file.write((char*)&partitions[p].hasVertexWeights, 1);
		if (partitions[p].hasVertexWeights)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].vertexWeights[i], 16);

		for (int i = 0; i < partitions[p].numStrips; i++)
			file.write((char*)&partitions[p].stripLengths[i], 2);

		file.write((char*)&partitions[p].hasFaces, 1);
		if (partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numStrips; i++)
				for (int j = 0; j < partitions[p].stripLengths[i]; j++)
					file.write((char*)&partitions[p].strips[i][j], 2);

		if (partitions[p].numStrips == 0 && partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numTriangles; i++)
				file.write((char*)&partitions[p].triangles[i].p1, 6);

		file.write((char*)&partitions[p].hasBoneIndices, 1);
		if (partitions[p].hasBoneIndices)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].boneIndices[i], 4);

		if (header->GetUserVersion() >= 12)
			file.write((char*)&partitions[p].unkShort, 2);

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			file.write((char*)&partitions[p].vertFlags1, 1);
			file.write((char*)&partitions[p].vertFlags2, 1);
			file.write((char*)&partitions[p].vertFlags3, 1);
			file.write((char*)&partitions[p].vertFlags4, 1);
			file.write((char*)&partitions[p].vertFlags5, 1);
			file.write((char*)&partitions[p].vertFlags6, 1);
			file.write((char*)&partitions[p].vertFlags7, 1);
			file.write((char*)&partitions[p].vertFlags8, 1);

			if (partitions[p].trueTriangles.size() != partitions[p].numTriangles)
				partitions[p].trueTriangles = partitions[p].triangles;

			for (int i = 0; i < partitions[p].numTriangles; i++) {
				file.write((char*)&partitions[p].trueTriangles[i].p1, 2);
				file.write((char*)&partitions[p].trueTriangles[i].p2, 2);
				file.write((char*)&partitions[p].trueTriangles[i].p3, 2);
			}
		}
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

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			// Move triangles that match to the end
			auto removeTriEnd = partition(p.trueTriangles.begin(), p.trueTriangles.end(), [&vertIndices](const Triangle& tri) {
				if (find(vertIndices.begin(), vertIndices.end(), tri.p1) != vertIndices.end())
					return true;
				if (find(vertIndices.begin(), vertIndices.end(), tri.p2) != vertIndices.end())
					return true;
				if (find(vertIndices.begin(), vertIndices.end(), tri.p3) != vertIndices.end())
					return true;

				return false;
			});

			p.trueTriangles.erase(p.trueTriangles.begin(), removeTriEnd);
		}
	}

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		for (int i = vertData.size() - 1; i >= 0; i--)
			if (find(vertIndices.begin(), vertIndices.end(), i) != vertIndices.end())
				vertData.erase(vertData.begin() + i);

		numVertices = vertData.size();
	}
}

int NiSkinPartition::RemoveEmptyPartitions(vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = partitions.size() - 1; i >= 0; i--) {
		if (partitions[i].numVertices == 0) {
			outDeletedIndices.push_back(i);
			partitions.erase(partitions.begin() + i);
			numPartitions--;
		}
	}
	return outDeletedIndices.size();
}

int NiSkinPartition::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		blockSize += 16;

		dataSize = vertexSize * numVertices;
		blockSize += dataSize;
	}

	for (auto &p : partitions) {
		if (header->GetUserVersion() >= 12)				// Plain data size
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

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			blockSize += 8;
			blockSize += p.numTriangles * 6;
		}
	}

	return blockSize;
}


NiParticleSystem::NiParticleSystem(NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NIPARTICLESYSTEM;

	dataRef = 0xFFFFFFFF;
	skinInstanceRef = 0xFFFFFFFF;
	shaderPropertyRef = 0xFFFFFFFF;
	alphaPropertyRef = 0xFFFFFFFF;
	numMaterials = 0;

	psysDataRef = 0xFFFFFFFF;
	numModifiers = 0;
}

NiParticleSystem::NiParticleSystem(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NIPARTICLESYSTEM;

	Get(file);
}

void NiParticleSystem::Get(fstream& file) {
	NiAVObject::Get(file);

	if (header->GetUserVersion2() >= 100) {
		file.read((char*)&bounds, 16);
		file.read((char*)&skinInstanceRef, 4);
		file.read((char*)&shaderPropertyRef, 4);
		file.read((char*)&alphaPropertyRef, 4);
		file.read((char*)&vertFlags1, 1);
		file.read((char*)&vertFlags2, 1);
		file.read((char*)&vertFlags3, 1);
		file.read((char*)&vertFlags4, 1);
		file.read((char*)&vertFlags5, 1);
		file.read((char*)&vertFlags6, 1);
		file.read((char*)&vertFlags7, 1);
		file.read((char*)&vertFlags8, 1);
	}
	else {
		file.read((char*)&dataRef, 4);
		psysDataRef = dataRef;
		file.read((char*)&skinInstanceRef, 4);
		file.read((char*)&numMaterials, 4);

		if (numMaterials > 0) {
			file.read((char*)&materialString, 4);
			materials.resize(numMaterials);
			for (int i = 0; i < numMaterials; i++)
				file.read((char*)&materials[i], 4);
		}

		file.read((char*)&activeMaterial, 4);
		file.read((char*)&defaultMatNeedsUpdate, 1);

		if (header->GetUserVersion() >= 12) {
			file.read((char*)&shaderPropertyRef, 4);
			file.read((char*)&alphaPropertyRef, 4);
		}
	}

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&farBegin, 2);
		file.read((char*)&farEnd, 2);
		file.read((char*)&nearBegin, 2);
		file.read((char*)&nearEnd, 2);

		if (header->GetUserVersion2() >= 100)
			file.read((char*)&psysDataRef, 4);
	}

	file.read((char*)&isWorldSpace, 1);

	file.read((char*)&numModifiers, 4);
	modifiers.resize(numModifiers);
	for (int i = 0; i < numModifiers; i++)
		file.read((char*)&modifiers[i], 4);
}

void NiParticleSystem::Put(fstream& file) {
	NiAVObject::Put(file);

	if (header->GetUserVersion2() >= 100) {
		file.write((char*)&bounds, 16);
		file.write((char*)&skinInstanceRef, 4);
		file.write((char*)&shaderPropertyRef, 4);
		file.write((char*)&alphaPropertyRef, 4);
		file.write((char*)&vertFlags1, 1);
		file.write((char*)&vertFlags2, 1);
		file.write((char*)&vertFlags3, 1);
		file.write((char*)&vertFlags4, 1);
		file.write((char*)&vertFlags5, 1);
		file.write((char*)&vertFlags6, 1);
		file.write((char*)&vertFlags7, 1);
		file.write((char*)&vertFlags8, 1);
	}
	else {
		file.write((char*)&dataRef, 4);
		file.write((char*)&skinInstanceRef, 4);
		file.write((char*)&numMaterials, 4);

		if (numMaterials > 0) {
			file.write((char*)&materialString, 4);
			for (int i = 0; i < numMaterials; i++)
				file.write((char*)&materials[i], 4);
		}

		file.write((char*)&activeMaterial, 4);
		file.write((char*)&defaultMatNeedsUpdate, 1);

		if (header->GetUserVersion() >= 12) {
			file.write((char*)&shaderPropertyRef, 4);
			file.write((char*)&alphaPropertyRef, 4);
		}
	}

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&farBegin, 2);
		file.write((char*)&farEnd, 2);
		file.write((char*)&nearBegin, 2);
		file.write((char*)&nearEnd, 2);

		if (header->GetUserVersion2() >= 100)
			file.write((char*)&psysDataRef, 4);
	}

	file.write((char*)&isWorldSpace, 1);

	file.write((char*)&numModifiers, 4);
	for (int i = 0; i < numModifiers; i++)
		file.write((char*)&modifiers[i], 4);
}

void NiParticleSystem::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;

	if (skinInstanceRef == blockID)
		skinInstanceRef = 0xFFFFFFFF;
	else if (skinInstanceRef > blockID)
		skinInstanceRef--;

	if (shaderPropertyRef == blockID)
		shaderPropertyRef = 0xFFFFFFFF;
	else if (shaderPropertyRef > blockID)
		shaderPropertyRef--;

	if (alphaPropertyRef == blockID)
		alphaPropertyRef = 0xFFFFFFFF;
	else if (alphaPropertyRef > blockID)
		alphaPropertyRef--;

	if (psysDataRef == blockID)
		psysDataRef = 0xFFFFFFFF;
	else if (psysDataRef > blockID)
		psysDataRef--;

	for (int i = 0; i < numModifiers; i++) {
		if (modifiers[i] == blockID) {
			modifiers.erase(modifiers.begin() + i);
			i--;
			numModifiers--;
		}
		else if (modifiers[i] > blockID)
			modifiers[i]--;
	}
}

void NiParticleSystem::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;

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

	if (psysDataRef == blockIndexLo)
		psysDataRef = blockIndexHi;
	else if (psysDataRef == blockIndexHi)
		psysDataRef = blockIndexLo;

	for (int i = 0; i < numModifiers; i++) {
		if (modifiers[i] == blockIndexLo)
			modifiers[i] = blockIndexHi;
		else if (modifiers[i] == blockIndexHi)
			modifiers[i] = blockIndexLo;
	}
}

void NiParticleSystem::GetChildRefs(set<int>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(dataRef);
	refs.insert(skinInstanceRef);
	refs.insert(shaderPropertyRef);
	refs.insert(alphaPropertyRef);
	refs.insert(psysDataRef);
	refs.insert(modifiers.begin(), modifiers.end());
}

int NiParticleSystem::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 5;
	blockSize += numModifiers * 4;

	if (header->GetUserVersion2() >= 100) {
		blockSize += 36;
	}
	else {
		blockSize += 17;

		if (numMaterials > 0) {
			blockSize += 4;
			blockSize += numMaterials * 4;
		}

		if (header->GetUserVersion() >= 12)
			blockSize += 8;
	}

	if (header->GetUserVersion() >= 12) {
		blockSize += 8;

		if (header->GetUserVersion2() >= 100)
			blockSize += 4;
	}

	return blockSize;
}


NiMeshParticleSystem::NiMeshParticleSystem(NiHeader& hdr) : NiParticleSystem(hdr) {
	blockType = NIMESHPARTICLESYSTEM;
}

NiMeshParticleSystem::NiMeshParticleSystem(fstream& file, NiHeader& hdr) : NiParticleSystem(file, hdr) {
	blockType = NIMESHPARTICLESYSTEM;
}


BSStripParticleSystem::BSStripParticleSystem(NiHeader& hdr) : NiParticleSystem(hdr) {
	blockType = BSSTRIPPARTICLESYSTEM;
}

BSStripParticleSystem::BSStripParticleSystem(fstream& file, NiHeader& hdr) : NiParticleSystem(file, hdr) {
	blockType = BSSTRIPPARTICLESYSTEM;
}


NiParticlesData::NiParticlesData(NiHeader& hdr) {
	NiGeometryData::Init();
	NiGeometryData::isPSys = true;

	header = &hdr;
	blockType = NIPARTICLESDATA;

	hasRadii = false;
	numActive = 0;
	hasSizes = false;
	hasRotations = false;
	hasRotationAngles = false;
	hasRotationAxes = false;
	hasTextureIndices = false;
	numSubtexOffsets = 0;
}

NiParticlesData::NiParticlesData(fstream& file, NiHeader& hdr) {
	NiGeometryData::Init();
	NiGeometryData::isPSys = true;

	header = &hdr;
	blockType = NIPARTICLESDATA;

	Get(file);
}

void NiParticlesData::Get(fstream& file) {
	NiGeometryData::Get(file);

	file.read((char*)&hasRadii, 1);
	file.read((char*)&numActive, 2);
	file.read((char*)&hasSizes, 1);
	file.read((char*)&hasRotations, 1);
	file.read((char*)&hasRotationAngles, 1);
	file.read((char*)&hasRotationAxes, 1);
	file.read((char*)&hasTextureIndices, 1);

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&numSubtexOffsets, 4);
	}
	else {
		byte numOffsets = 0;
		file.read((char*)&numOffsets, 1);
		numSubtexOffsets = numOffsets;
	}

	subtexOffsets.resize(numSubtexOffsets);
	for (int i = 0; i < numSubtexOffsets; i++)
		file.read((char*)&subtexOffsets[i], 16);

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&aspectRatio, 4);
		file.read((char*)&aspectFlags, 2);
		file.read((char*)&speedToAspectAspect2, 4);
		file.read((char*)&speedToAspectSpeed1, 4);
		file.read((char*)&speedToAspectSpeed2, 4);
	}
}

void NiParticlesData::Put(fstream& file) {
	NiGeometryData::Put(file);

	file.write((char*)&hasRadii, 1);
	file.write((char*)&numActive, 2);
	file.write((char*)&hasSizes, 1);
	file.write((char*)&hasRotations, 1);
	file.write((char*)&hasRotationAngles, 1);
	file.write((char*)&hasRotationAxes, 1);
	file.write((char*)&hasTextureIndices, 1);

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&numSubtexOffsets, 4);
	}
	else {
		byte numOffsets = numSubtexOffsets > 255 ? 255 : numSubtexOffsets;
		file.write((char*)&numOffsets, 1);
	}

	for (int i = 0; i < numSubtexOffsets; i++)
		file.write((char*)&subtexOffsets[i], 16);

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&aspectRatio, 4);
		file.write((char*)&aspectFlags, 2);
		file.write((char*)&speedToAspectAspect2, 4);
		file.write((char*)&speedToAspectSpeed1, 4);
		file.write((char*)&speedToAspectSpeed2, 4);
	}
}

int NiParticlesData::CalcBlockSize() {
	NiGeometryData::CalcBlockSize();

	blockSize += 8;

	if (header->GetUserVersion() >= 12)
		blockSize += 22;
	else
		blockSize += 1;

	blockSize += numSubtexOffsets * 16;

	return blockSize;
}


NiRotatingParticlesData::NiRotatingParticlesData(NiHeader& hdr) : NiParticlesData(hdr) {
	blockType = NIROTATINGPARTICLESDATA;
}

NiRotatingParticlesData::NiRotatingParticlesData(fstream& file, NiHeader& hdr) : NiParticlesData(file, hdr) {
	blockType = NIROTATINGPARTICLESDATA;
}


NiPSysData::NiPSysData(NiHeader& hdr) : NiRotatingParticlesData(hdr) {
	blockType = NIPSYSDATA;
	hasRotationSpeeds = false;
}

NiPSysData::NiPSysData(fstream& file, NiHeader& hdr) : NiRotatingParticlesData(file, hdr) {
	blockType = NIPSYSDATA;

	Get(file);
}

void NiPSysData::Get(fstream& file) {
	file.read((char*)&hasRotationSpeeds, 1);
}

void NiPSysData::Put(fstream& file) {
	NiRotatingParticlesData::Put(file);

	file.write((char*)&hasRotationSpeeds, 1);
}

int NiPSysData::CalcBlockSize() {
	NiRotatingParticlesData::CalcBlockSize();

	blockSize += 1;

	return blockSize;
}


NiMeshPSysData::NiMeshPSysData(NiHeader& hdr) : NiPSysData(hdr) {
	header = &hdr;
	blockType = NIMESHPSYSDATA;

	nodeRef = 0xFFFFFFFF;
}

NiMeshPSysData::NiMeshPSysData(fstream& file, NiHeader& hdr) : NiPSysData(file, hdr) {
	header = &hdr;
	blockType = NIMESHPSYSDATA;

	Get(file);
}

void NiMeshPSysData::Get(fstream& file) {
	file.read((char*)&defaultPoolSize, 4);
	file.read((char*)&fillPoolsOnLoad, 1);

	file.read((char*)&numGenerations, 4);
	generationPoolSize.resize(numGenerations);
	for (int i = 0; i < numGenerations; i++)
		file.read((char*)&generationPoolSize[i], 4);

	file.read((char*)&nodeRef, 4);
}

void NiMeshPSysData::Put(fstream& file) {
	NiPSysData::Put(file);

	file.write((char*)&defaultPoolSize, 4);
	file.write((char*)&fillPoolsOnLoad, 1);

	file.write((char*)&numGenerations, 4);
	for (int i = 0; i < numGenerations; i++)
		file.write((char*)&generationPoolSize[i], 4);

	file.write((char*)&nodeRef, 4);
}

void NiMeshPSysData::notifyBlockDelete(int blockID) {
	NiPSysData::notifyBlockDelete(blockID);

	if (nodeRef == blockID)
		nodeRef = 0xFFFFFFFF;
	else if (nodeRef > blockID)
		nodeRef--;
}

void NiMeshPSysData::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysData::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (nodeRef == blockIndexLo)
		nodeRef = blockIndexHi;
	else if (nodeRef == blockIndexHi)
		nodeRef = blockIndexLo;
}

void NiMeshPSysData::GetChildRefs(set<int>& refs) {
	NiPSysData::GetChildRefs(refs);

	refs.insert(nodeRef);
}

int NiMeshPSysData::CalcBlockSize() {
	NiPSysData::CalcBlockSize();

	blockSize += 9;
	blockSize += numGenerations * 4;

	return blockSize;
}


BSStripPSysData::BSStripPSysData(NiHeader& hdr) : NiPSysData(hdr) {
	blockType = BSSTRIPPSYSDATA;
}

BSStripPSysData::BSStripPSysData(fstream& file, NiHeader& hdr) : NiPSysData(file, hdr) {
	blockType = BSSTRIPPSYSDATA;

	Get(file);
}

void BSStripPSysData::Get(fstream& file) {
	file.read((char*)&maxPointCount, 2);
	file.read((char*)&startCapSize, 4);
	file.read((char*)&endCapSize, 4);
	file.read((char*)&doZPrepass, 1);
}

void BSStripPSysData::Put(fstream& file) {
	NiPSysData::Put(file);

	file.write((char*)&maxPointCount, 2);
	file.write((char*)&startCapSize, 4);
	file.write((char*)&endCapSize, 4);
	file.write((char*)&doZPrepass, 1);
}

int BSStripPSysData::CalcBlockSize() {
	NiPSysData::CalcBlockSize();

	blockSize += 11;

	return blockSize;
}


NiCamera::NiCamera(NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NICAMERA;

	sceneRef = 0xFFFFFFFF;
	screenPolygonsRef = 0xFFFFFFFF;
	screenTexturesRef = 0xFFFFFFFF;
}

NiCamera::NiCamera(fstream& file, NiHeader& hdr) {
	NiAVObject::Init();

	header = &hdr;
	blockType = NICAMERA;

	Get(file);
}

void NiCamera::Get(fstream& file) {
	NiAVObject::Get(file);

	file.read((char*)&obsoleteFlags, 2);
	file.read((char*)&frustumLeft, 4);
	file.read((char*)&frustumRight, 4);
	file.read((char*)&frustumTop, 4);
	file.read((char*)&frustomBottom, 4);
	file.read((char*)&frustumNear, 4);
	file.read((char*)&frustumFar, 4);
	file.read((char*)&useOrtho, 1);
	file.read((char*)&viewportLeft, 4);
	file.read((char*)&viewportRight, 4);
	file.read((char*)&viewportTop, 4);
	file.read((char*)&viewportBottom, 4);
	file.read((char*)&lodAdjust, 4);

	file.read((char*)&sceneRef, 4);
	file.read((char*)&screenPolygonsRef, 4);
	file.read((char*)&screenTexturesRef, 4);
}

void NiCamera::Put(fstream& file) {
	NiAVObject::Put(file);

	file.write((char*)&obsoleteFlags, 2);
	file.write((char*)&frustumLeft, 4);
	file.write((char*)&frustumRight, 4);
	file.write((char*)&frustumTop, 4);
	file.write((char*)&frustomBottom, 4);
	file.write((char*)&frustumNear, 4);
	file.write((char*)&frustumFar, 4);
	file.write((char*)&useOrtho, 1);
	file.write((char*)&viewportLeft, 4);
	file.write((char*)&viewportRight, 4);
	file.write((char*)&viewportTop, 4);
	file.write((char*)&viewportBottom, 4);
	file.write((char*)&lodAdjust, 4);

	file.write((char*)&sceneRef, 4);
	file.write((char*)&screenPolygonsRef, 4);
	file.write((char*)&screenTexturesRef, 4);
}

void NiCamera::notifyBlockDelete(int blockID) {
	NiAVObject::notifyBlockDelete(blockID);

	if (sceneRef == blockID)
		sceneRef = 0xFFFFFFFF;
	else if (sceneRef > blockID)
		sceneRef--;

	if (screenPolygonsRef == blockID)
		screenPolygonsRef = 0xFFFFFFFF;
	else if (screenPolygonsRef > blockID)
		screenPolygonsRef--;

	if (screenTexturesRef == blockID)
		screenTexturesRef = 0xFFFFFFFF;
	else if (screenTexturesRef > blockID)
		screenTexturesRef--;
}

void NiCamera::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (sceneRef == blockIndexLo)
		sceneRef = blockIndexHi;
	else if (sceneRef == blockIndexHi)
		sceneRef = blockIndexLo;

	if (screenPolygonsRef == blockIndexLo)
		screenPolygonsRef = blockIndexHi;
	else if (screenPolygonsRef == blockIndexHi)
		screenPolygonsRef = blockIndexLo;

	if (screenTexturesRef == blockIndexLo)
		screenTexturesRef = blockIndexHi;
	else if (screenTexturesRef == blockIndexHi)
		screenTexturesRef = blockIndexLo;
}

void NiCamera::GetChildRefs(set<int>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(sceneRef);
	refs.insert(screenPolygonsRef);
	refs.insert(screenTexturesRef);
}

int NiCamera::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 59;

	return blockSize;
}


NiPSysModifier::~NiPSysModifier() {
	header->RemoveStringRef(nameRef);
}

void NiPSysModifier::Init() {
	NiObject::Init();

	nameRef = 0xFFFFFFFF;
	targetRef = 0xFFFFFFFF;
}

void NiPSysModifier::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nameRef, 4);
	if (nameRef != 0xFFFFFFFF) {
		name = header->GetStringById(nameRef);
		header->AddStringRef(nameRef);
	}

	file.read((char*)&order, 4);
	file.read((char*)&targetRef, 4);
	file.read((char*)&isActive, 1);
}

void NiPSysModifier::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nameRef, 4);
	file.write((char*)&order, 4);
	file.write((char*)&targetRef, 4);
	file.write((char*)&isActive, 1);
}

void NiPSysModifier::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	if (targetRef == blockID)
		targetRef = 0xFFFFFFFF;
	else if (targetRef > blockID)
		targetRef--;
}

void NiPSysModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (targetRef == blockIndexLo)
		targetRef = blockIndexHi;
	else if (targetRef == blockIndexHi)
		targetRef = blockIndexLo;
}

void NiPSysModifier::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	if (nameRef != 0xFFFFFFFF && nameRef > stringID)
		nameRef--;
}

void NiPSysModifier::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(targetRef);
}

int NiPSysModifier::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 13;

	return blockSize;
}


BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSTRIPUPDATEMODIFIER;
}

BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSTRIPUPDATEMODIFIER;

	Get(file);
}

void BSPSysStripUpdateModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&updateDeltaTime, 4);
}

void BSPSysStripUpdateModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&updateDeltaTime, 4);
}

int BSPSysStripUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSAGEDEATHMODIFIER;
}

NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSAGEDEATHMODIFIER;

	Get(file);
}

void NiPSysAgeDeathModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&spawnOnDeath, 1);
	file.read((char*)&spawnModifierRef, 4);
}

void NiPSysAgeDeathModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&spawnOnDeath, 1);
	file.write((char*)&spawnModifierRef, 4);
}

void NiPSysAgeDeathModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (spawnModifierRef == blockID)
		spawnModifierRef = 0xFFFFFFFF;
	else if (spawnModifierRef > blockID)
		spawnModifierRef--;
}

void NiPSysAgeDeathModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (spawnModifierRef == blockIndexLo)
		spawnModifierRef = blockIndexHi;
	else if (spawnModifierRef == blockIndexHi)
		spawnModifierRef = blockIndexLo;
}

void NiPSysAgeDeathModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(spawnModifierRef);
}

int NiPSysAgeDeathModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSPSysLODModifier::BSPSysLODModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSLODMODIFIER;
}

BSPSysLODModifier::BSPSysLODModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSLODMODIFIER;

	Get(file);
}

void BSPSysLODModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&unkVector, 16);
}

void BSPSysLODModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&unkVector, 16);
}

int BSPSysLODModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


NiPSysSpawnModifier::NiPSysSpawnModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSSPAWNMODIFIER;

	numSpawnGenerations = 0;
}

NiPSysSpawnModifier::NiPSysSpawnModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSSPAWNMODIFIER;

	Get(file);
}

void NiPSysSpawnModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&numSpawnGenerations, 2);
	file.read((char*)&percentSpawned, 4);
	file.read((char*)&minSpawned, 2);
	file.read((char*)&maxSpawned, 2);
	file.read((char*)&spawnSpeedVariation, 4);
	file.read((char*)&spawnDirVariation, 4);
	file.read((char*)&lifeSpan, 4);
	file.read((char*)&lifeSpanVariation, 4);
}

void NiPSysSpawnModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&numSpawnGenerations, 2);
	file.write((char*)&percentSpawned, 4);
	file.write((char*)&minSpawned, 2);
	file.write((char*)&maxSpawned, 2);
	file.write((char*)&spawnSpeedVariation, 4);
	file.write((char*)&spawnDirVariation, 4);
	file.write((char*)&lifeSpan, 4);
	file.write((char*)&lifeSpanVariation, 4);
}

int NiPSysSpawnModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 26;

	return blockSize;
}


BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSIMPLECOLORMODIFIER;
}

BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSIMPLECOLORMODIFIER;

	Get(file);
}

void BSPSysSimpleColorModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&fadeInPercent, 4);
	file.read((char*)&fadeOutPercent, 4);
	file.read((char*)&color1EndPercent, 4);
	file.read((char*)&color2StartPercent, 4);
	file.read((char*)&color2EndPercent, 4);
	file.read((char*)&color3StartPercent, 4);
	file.read((char*)&color1, 16);
	file.read((char*)&color2, 16);
	file.read((char*)&color3, 16);
}

void BSPSysSimpleColorModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&fadeInPercent, 4);
	file.write((char*)&fadeOutPercent, 4);
	file.write((char*)&color1EndPercent, 4);
	file.write((char*)&color2StartPercent, 4);
	file.write((char*)&color2EndPercent, 4);
	file.write((char*)&color3StartPercent, 4);
	file.write((char*)&color1, 16);
	file.write((char*)&color2, 16);
	file.write((char*)&color3, 16);
}

int BSPSysSimpleColorModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 72;

	return blockSize;
}


NiPSysRotationModifier::NiPSysRotationModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSROTATIONMODIFIER;
}

NiPSysRotationModifier::NiPSysRotationModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSROTATIONMODIFIER;

	Get(file);
}

void NiPSysRotationModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&initialSpeed, 4);
	file.read((char*)&initialSpeedVariation, 4);
	file.read((char*)&initialAngle, 4);
	file.read((char*)&initialAngleVariation, 4);
	file.read((char*)&randomSpeedSign, 1);
	file.read((char*)&randomInitialAxis, 1);
	file.read((char*)&initialAxis, 12);
}

void NiPSysRotationModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&initialSpeed, 4);
	file.write((char*)&initialSpeedVariation, 4);
	file.write((char*)&initialAngle, 4);
	file.write((char*)&initialAngleVariation, 4);
	file.write((char*)&randomSpeedSign, 1);
	file.write((char*)&randomInitialAxis, 1);
	file.write((char*)&initialAxis, 12);
}

int NiPSysRotationModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 30;

	return blockSize;
}


BSPSysScaleModifier::BSPSysScaleModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSCALEMODIFIER;

	numFloats = 0;
}

BSPSysScaleModifier::BSPSysScaleModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSCALEMODIFIER;

	Get(file);
}

void BSPSysScaleModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&numFloats, 4);
	floats.resize(numFloats);
	for (int i = 0; i < numFloats; i++)
		file.read((char*)&floats[i], 4);
}

void BSPSysScaleModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&numFloats, 4);
	for (int i = 0; i < numFloats; i++)
		file.write((char*)&floats[i], 4);
}

int BSPSysScaleModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;
	blockSize += numFloats * 4;

	return blockSize;
}


NiPSysGravityModifier::NiPSysGravityModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSGRAVITYMODIFIER;

	gravityObjRef = 0xFFFFFFFF;
}

NiPSysGravityModifier::NiPSysGravityModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSGRAVITYMODIFIER;

	Get(file);
}

void NiPSysGravityModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&gravityObjRef, 4);
	file.read((char*)&gravityAxis, 12);
	file.read((char*)&decay, 4);
	file.read((char*)&strength, 4);
	file.read((char*)&forceType, 4);
	file.read((char*)&turbulence, 4);
	file.read((char*)&turbulenceScale, 4);
	file.read((char*)&worldAligned, 1);
}

void NiPSysGravityModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&gravityObjRef, 4);
	file.write((char*)&gravityAxis, 12);
	file.write((char*)&decay, 4);
	file.write((char*)&strength, 4);
	file.write((char*)&forceType, 4);
	file.write((char*)&turbulence, 4);
	file.write((char*)&turbulenceScale, 4);
	file.write((char*)&worldAligned, 1);
}

void NiPSysGravityModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (gravityObjRef == blockID)
		gravityObjRef = 0xFFFFFFFF;
	else if (gravityObjRef > blockID)
		gravityObjRef--;
}

void NiPSysGravityModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (gravityObjRef == blockIndexLo)
		gravityObjRef = blockIndexHi;
	else if (gravityObjRef == blockIndexHi)
		gravityObjRef = blockIndexLo;
}

void NiPSysGravityModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(gravityObjRef);
}

int NiPSysGravityModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 37;

	return blockSize;
}


NiPSysPositionModifier::NiPSysPositionModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSPOSITIONMODIFIER;
}

NiPSysPositionModifier::NiPSysPositionModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSPOSITIONMODIFIER;

	Get(file);
}


NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSBOUNDUPDATEMODIFIER;
}

NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSBOUNDUPDATEMODIFIER;

	Get(file);
}

void NiPSysBoundUpdateModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&updateSkip, 2);
}

void NiPSysBoundUpdateModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&updateSkip, 2);
}

int NiPSysBoundUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 2;

	return blockSize;
}


NiPSysDragModifier::NiPSysDragModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSDRAGMODIFIER;

	parentRef = 0xFFFFFFFF;
}

NiPSysDragModifier::NiPSysDragModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSDRAGMODIFIER;

	Get(file);
}

void NiPSysDragModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&parentRef, 4);
	file.read((char*)&dragAxis, 12);
	file.read((char*)&percentage, 4);
	file.read((char*)&range, 4);
	file.read((char*)&rangeFalloff, 4);
}

void NiPSysDragModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&parentRef, 4);
	file.write((char*)&dragAxis, 12);
	file.write((char*)&percentage, 4);
	file.write((char*)&range, 4);
	file.write((char*)&rangeFalloff, 4);
}

void NiPSysDragModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (parentRef == blockID)
		parentRef = 0xFFFFFFFF;
	else if (parentRef > blockID)
		parentRef--;
}

void NiPSysDragModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (parentRef == blockIndexLo)
		parentRef = blockIndexHi;
	else if (parentRef == blockIndexHi)
		parentRef = blockIndexLo;
}

void NiPSysDragModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(parentRef);
}

int NiPSysDragModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSINHERITVELOCITYMODIFIER;

	targetNodeRef = 0xFFFFFFFF;
}

BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSINHERITVELOCITYMODIFIER;

	Get(file);
}

void BSPSysInheritVelocityModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&targetNodeRef, 4);
	file.read((char*)&changeToInherit, 4);
	file.read((char*)&velocityMult, 4);
	file.read((char*)&velocityVar, 4);
}

void BSPSysInheritVelocityModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&targetNodeRef, 4);
	file.write((char*)&changeToInherit, 4);
	file.write((char*)&velocityMult, 4);
	file.write((char*)&velocityVar, 4);
}

void BSPSysInheritVelocityModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (targetNodeRef == blockID)
		targetNodeRef = 0xFFFFFFFF;
	else if (targetNodeRef > blockID)
		targetNodeRef--;
}

void BSPSysInheritVelocityModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (targetNodeRef == blockIndexLo)
		targetNodeRef = blockIndexHi;
	else if (targetNodeRef == blockIndexHi)
		targetNodeRef = blockIndexLo;
}

void BSPSysInheritVelocityModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(targetNodeRef);
}

int BSPSysInheritVelocityModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


BSPSysSubTexModifier::BSPSysSubTexModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSUBTEXMODIFIER;
}

BSPSysSubTexModifier::BSPSysSubTexModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSSUBTEXMODIFIER;

	Get(file);
}

void BSPSysSubTexModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&startFrame, 4);
	file.read((char*)&startFrameVariation, 4);
	file.read((char*)&endFrame, 4);
	file.read((char*)&loopStartFrame, 4);
	file.read((char*)&loopStartFrameVariation, 4);
	file.read((char*)&frameCount, 4);
	file.read((char*)&frameCountVariation, 4);
}

void BSPSysSubTexModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&startFrame, 4);
	file.write((char*)&startFrameVariation, 4);
	file.write((char*)&endFrame, 4);
	file.write((char*)&loopStartFrame, 4);
	file.write((char*)&loopStartFrameVariation, 4);
	file.write((char*)&frameCount, 4);
	file.write((char*)&frameCountVariation, 4);
}

int BSPSysSubTexModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


NiPSysBombModifier::NiPSysBombModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSBOMBMODIFIER;

	bombNodeRef = 0xFFFFFFFF;
}

NiPSysBombModifier::NiPSysBombModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSBOMBMODIFIER;

	Get(file);
}

void NiPSysBombModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&bombNodeRef, 4);
	file.read((char*)&bombAxis, 12);
	file.read((char*)&decay, 4);
	file.read((char*)&deltaV, 4);
	file.read((char*)&decayType, 4);
	file.read((char*)&symmetryType, 4);
}

void NiPSysBombModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&bombNodeRef, 4);
	file.write((char*)&bombAxis, 12);
	file.write((char*)&decay, 4);
	file.write((char*)&deltaV, 4);
	file.write((char*)&decayType, 4);
	file.write((char*)&symmetryType, 4);
}

void NiPSysBombModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (bombNodeRef == blockID)
		bombNodeRef = 0xFFFFFFFF;
	else if (bombNodeRef > blockID)
		bombNodeRef--;
}

void NiPSysBombModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (bombNodeRef == blockIndexLo)
		bombNodeRef = blockIndexHi;
	else if (bombNodeRef == blockIndexHi)
		bombNodeRef = blockIndexLo;
}

void NiPSysBombModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(bombNodeRef);
}

int NiPSysBombModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


BSWindModifier::BSWindModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSWINDMODIFIER;
}

BSWindModifier::BSWindModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSWINDMODIFIER;

	Get(file);
}

void BSWindModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&strength, 4);
}

void BSWindModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&strength, 4);
}

int BSWindModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSPSysRecycleBoundModifier::BSPSysRecycleBoundModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSRECYCLEBOUNDMODIFIER;

	targetNodeRef = 0xFFFFFFFF;
}

BSPSysRecycleBoundModifier::BSPSysRecycleBoundModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSRECYCLEBOUNDMODIFIER;

	Get(file);
}

void BSPSysRecycleBoundModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&boundOffset, 12);
	file.read((char*)&boundExtent, 12);
	file.read((char*)&targetNodeRef, 4);
}

void BSPSysRecycleBoundModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&boundOffset, 12);
	file.write((char*)&boundExtent, 12);
	file.write((char*)&targetNodeRef, 4);
}

void BSPSysRecycleBoundModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (targetNodeRef == blockID)
		targetNodeRef = 0xFFFFFFFF;
	else if (targetNodeRef > blockID)
		targetNodeRef--;
}

void BSPSysRecycleBoundModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (targetNodeRef == blockIndexLo)
		targetNodeRef = blockIndexHi;
	else if (targetNodeRef == blockIndexHi)
		targetNodeRef = blockIndexLo;
}

void BSPSysRecycleBoundModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(targetNodeRef);
}

int BSPSysRecycleBoundModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


BSPSysHavokUpdateModifier::BSPSysHavokUpdateModifier(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSHAVOKUPDATEMODIFIER;

	modifierRef = 0xFFFFFFFF;
}

BSPSysHavokUpdateModifier::BSPSysHavokUpdateModifier(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = BSPSYSHAVOKUPDATEMODIFIER;

	Get(file);
}

void BSPSysHavokUpdateModifier::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&numNodes, 4);
	nodeRefs.resize(numNodes);
	for (int i = 0; i < numNodes; i++)
		file.read((char*)&nodeRefs[i], 4);

	file.read((char*)&modifierRef, 4);
}

void BSPSysHavokUpdateModifier::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&numNodes, 4);
	for (int i = 0; i < numNodes; i++)
		file.write((char*)&nodeRefs[i], 4);

	file.write((char*)&modifierRef, 4);
}

void BSPSysHavokUpdateModifier::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (modifierRef == blockID)
		modifierRef = 0xFFFFFFFF;
	else if (modifierRef > blockID)
		modifierRef--;

	for (int i = 0; i < numNodes; i++) {
		if (nodeRefs[i] == blockID) {
			nodeRefs.erase(nodeRefs.begin() + i);
			i--;
			numNodes--;
		}
		else if (nodeRefs[i] > blockID)
			nodeRefs[i]--;
	}
}

void BSPSysHavokUpdateModifier::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (modifierRef == blockIndexLo)
		modifierRef = blockIndexHi;
	else if (modifierRef == blockIndexHi)
		modifierRef = blockIndexLo;

	for (int i = 0; i < numNodes; i++) {
		if (nodeRefs[i] == blockIndexLo)
			nodeRefs[i] = blockIndexHi;
		else if (nodeRefs[i] == blockIndexHi)
			nodeRefs[i] = blockIndexLo;
	}
}

void BSPSysHavokUpdateModifier::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(modifierRef);
	refs.insert(nodeRefs.begin(), nodeRefs.end());
}

int BSPSysHavokUpdateModifier::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 8;
	blockSize += numNodes * 4;

	return blockSize;
}


void NiPSysCollider::Init() {
	NiObject::Init();

	spawnModifierRef = 0xFFFFFFFF;
	managerRef = 0xFFFFFFFF;
	nextColliderRef = 0xFFFFFFFF;
	colliderNodeRef = 0xFFFFFFFF;
}

void NiPSysCollider::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&bounce, 4);
	file.read((char*)&spawnOnCollide, 1);
	file.read((char*)&dieOnCollide, 1);
	file.read((char*)&spawnModifierRef, 4);
	file.read((char*)&managerRef, 4);
	file.read((char*)&nextColliderRef, 4);
	file.read((char*)&colliderNodeRef, 4);
}

void NiPSysCollider::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&bounce, 4);
	file.write((char*)&spawnOnCollide, 1);
	file.write((char*)&dieOnCollide, 1);
	file.write((char*)&spawnModifierRef, 4);
	file.write((char*)&managerRef, 4);
	file.write((char*)&nextColliderRef, 4);
	file.write((char*)&colliderNodeRef, 4);
}

void NiPSysCollider::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	if (spawnModifierRef == blockID)
		spawnModifierRef = 0xFFFFFFFF;
	else if (spawnModifierRef > blockID)
		spawnModifierRef--;

	if (managerRef == blockID)
		managerRef = 0xFFFFFFFF;
	else if (managerRef > blockID)
		managerRef--;

	if (nextColliderRef == blockID)
		nextColliderRef = 0xFFFFFFFF;
	else if (nextColliderRef > blockID)
		nextColliderRef--;

	if (colliderNodeRef == blockID)
		colliderNodeRef = 0xFFFFFFFF;
	else if (colliderNodeRef > blockID)
		colliderNodeRef--;
}

void NiPSysCollider::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (spawnModifierRef == blockIndexLo)
		spawnModifierRef = blockIndexHi;
	else if (spawnModifierRef == blockIndexHi)
		spawnModifierRef = blockIndexLo;

	if (managerRef == blockIndexLo)
		managerRef = blockIndexHi;
	else if (managerRef == blockIndexHi)
		managerRef = blockIndexLo;

	if (nextColliderRef == blockIndexLo)
		nextColliderRef = blockIndexHi;
	else if (nextColliderRef == blockIndexHi)
		nextColliderRef = blockIndexLo;

	if (colliderNodeRef == blockIndexLo)
		colliderNodeRef = blockIndexHi;
	else if (colliderNodeRef == blockIndexHi)
		colliderNodeRef = blockIndexLo;
}

void NiPSysCollider::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(spawnModifierRef);
	refs.insert(managerRef);
	refs.insert(nextColliderRef);
	refs.insert(colliderNodeRef);
}

int NiPSysCollider::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 22;

	return blockSize;
}


NiPSysSphericalCollider::NiPSysSphericalCollider(NiHeader& hdr) {
	NiPSysCollider::Init();

	header = &hdr;
	blockType = NIPSYSSPHERICALCOLLIDER;
}

NiPSysSphericalCollider::NiPSysSphericalCollider(fstream& file, NiHeader& hdr) {
	NiPSysCollider::Init();

	header = &hdr;
	blockType = NIPSYSSPHERICALCOLLIDER;

	Get(file);
}

void NiPSysSphericalCollider::Get(fstream& file) {
	NiPSysCollider::Get(file);

	file.read((char*)&radius, 4);
}

void NiPSysSphericalCollider::Put(fstream& file) {
	NiPSysCollider::Put(file);

	file.write((char*)&radius, 4);
}

int NiPSysSphericalCollider::CalcBlockSize() {
	NiPSysCollider::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysPlanarCollider::NiPSysPlanarCollider(NiHeader& hdr) {
	NiPSysCollider::Init();

	header = &hdr;
	blockType = NIPSYSPLANARCOLLIDER;
}

NiPSysPlanarCollider::NiPSysPlanarCollider(fstream& file, NiHeader& hdr) {
	NiPSysCollider::Init();

	header = &hdr;
	blockType = NIPSYSPLANARCOLLIDER;

	Get(file);
}

void NiPSysPlanarCollider::Get(fstream& file) {
	NiPSysCollider::Get(file);

	file.read((char*)&width, 4);
	file.read((char*)&height, 4);
	file.read((char*)&xAxis, 12);
	file.read((char*)&yAxis, 12);
}

void NiPSysPlanarCollider::Put(fstream& file) {
	NiPSysCollider::Put(file);

	file.write((char*)&width, 4);
	file.write((char*)&height, 4);
	file.write((char*)&xAxis, 12);
	file.write((char*)&yAxis, 12);
}

int NiPSysPlanarCollider::CalcBlockSize() {
	NiPSysCollider::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


NiPSysColliderManager::NiPSysColliderManager(NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSCOLLIDERMANAGER;

	colliderRef = 0xFFFFFFFF;
}

NiPSysColliderManager::NiPSysColliderManager(fstream& file, NiHeader& hdr) {
	NiPSysModifier::Init();

	header = &hdr;
	blockType = NIPSYSCOLLIDERMANAGER;

	Get(file);
}

void NiPSysColliderManager::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&colliderRef, 4);
}

void NiPSysColliderManager::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&colliderRef, 4);
}

void NiPSysColliderManager::notifyBlockDelete(int blockID) {
	NiPSysModifier::notifyBlockDelete(blockID);

	if (colliderRef == blockID)
		colliderRef = 0xFFFFFFFF;
	else if (colliderRef > blockID)
		colliderRef--;
}

void NiPSysColliderManager::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifier::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (colliderRef == blockIndexLo)
		colliderRef = blockIndexHi;
	else if (colliderRef == blockIndexHi)
		colliderRef = blockIndexLo;
}

void NiPSysColliderManager::GetChildRefs(set<int>& refs) {
	NiPSysModifier::GetChildRefs(refs);

	refs.insert(colliderRef);
}

int NiPSysColliderManager::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


void NiPSysEmitter::Get(fstream& file) {
	NiPSysModifier::Get(file);

	file.read((char*)&speed, 4);
	file.read((char*)&speedVariation, 4);
	file.read((char*)&declination, 4);
	file.read((char*)&declinationVariation, 4);
	file.read((char*)&planarAngle, 4);
	file.read((char*)&planarAngleVariation, 4);
	file.read((char*)&color, 16);
	file.read((char*)&radius, 4);
	file.read((char*)&radiusVariation, 4);
	file.read((char*)&lifeSpan, 4);
	file.read((char*)&lifeSpanVariation, 4);
}

void NiPSysEmitter::Put(fstream& file) {
	NiPSysModifier::Put(file);

	file.write((char*)&speed, 4);
	file.write((char*)&speedVariation, 4);
	file.write((char*)&declination, 4);
	file.write((char*)&declinationVariation, 4);
	file.write((char*)&planarAngle, 4);
	file.write((char*)&planarAngleVariation, 4);
	file.write((char*)&color, 16);
	file.write((char*)&radius, 4);
	file.write((char*)&radiusVariation, 4);
	file.write((char*)&lifeSpan, 4);
	file.write((char*)&lifeSpanVariation, 4);
}

int NiPSysEmitter::CalcBlockSize() {
	NiPSysModifier::CalcBlockSize();

	blockSize += 56;

	return blockSize;
}


void NiPSysVolumeEmitter::Init() {
	NiPSysEmitter::Init();

	emitterNodeRef = 0xFFFFFFFF;
}

void NiPSysVolumeEmitter::Get(fstream& file) {
	NiPSysEmitter::Get(file);

	file.read((char*)&emitterNodeRef, 4);
}

void NiPSysVolumeEmitter::Put(fstream& file) {
	NiPSysEmitter::Put(file);

	file.write((char*)&emitterNodeRef, 4);
}

void NiPSysVolumeEmitter::notifyBlockDelete(int blockID) {
	NiPSysEmitter::notifyBlockDelete(blockID);

	if (emitterNodeRef == blockID)
		emitterNodeRef = 0xFFFFFFFF;
	else if (emitterNodeRef > blockID)
		emitterNodeRef--;
}

void NiPSysVolumeEmitter::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysEmitter::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (emitterNodeRef == blockIndexLo)
		emitterNodeRef = blockIndexHi;
	else if (emitterNodeRef == blockIndexHi)
		emitterNodeRef = blockIndexLo;
}

void NiPSysVolumeEmitter::GetChildRefs(set<int>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	refs.insert(emitterNodeRef);
}

int NiPSysVolumeEmitter::CalcBlockSize() {
	NiPSysEmitter::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysSphereEmitter::NiPSysSphereEmitter(NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSSPHEREEMITTER;
}

NiPSysSphereEmitter::NiPSysSphereEmitter(fstream& file, NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSSPHEREEMITTER;

	Get(file);
}

void NiPSysSphereEmitter::Get(fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&radius, 4);
}

void NiPSysSphereEmitter::Put(fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&radius, 4);
}

int NiPSysSphereEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysCylinderEmitter::NiPSysCylinderEmitter(NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSCYLINDEREMITTER;
}

NiPSysCylinderEmitter::NiPSysCylinderEmitter(fstream& file, NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSCYLINDEREMITTER;

	Get(file);
}

void NiPSysCylinderEmitter::Get(fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&radius, 4);
	file.read((char*)&height, 4);
}

void NiPSysCylinderEmitter::Put(fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&radius, 4);
	file.write((char*)&height, 4);
}

int NiPSysCylinderEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


NiPSysBoxEmitter::NiPSysBoxEmitter(NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSBOXEMITTER;
}

NiPSysBoxEmitter::NiPSysBoxEmitter(fstream& file, NiHeader& hdr) {
	NiPSysVolumeEmitter::Init();

	header = &hdr;
	blockType = NIPSYSBOXEMITTER;

	Get(file);
}

void NiPSysBoxEmitter::Get(fstream& file) {
	NiPSysVolumeEmitter::Get(file);

	file.read((char*)&width, 4);
	file.read((char*)&height, 4);
	file.read((char*)&depth, 4);
}

void NiPSysBoxEmitter::Put(fstream& file) {
	NiPSysVolumeEmitter::Put(file);

	file.write((char*)&width, 4);
	file.write((char*)&height, 4);
	file.write((char*)&depth, 4);
}

int NiPSysBoxEmitter::CalcBlockSize() {
	NiPSysVolumeEmitter::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


NiPSysMeshEmitter::NiPSysMeshEmitter(NiHeader& hdr) {
	NiPSysEmitter::Init();

	header = &hdr;
	blockType = NIPSYSMESHEMITTER;

	numMeshes = 0;
}

NiPSysMeshEmitter::NiPSysMeshEmitter(fstream& file, NiHeader& hdr) {
	NiPSysEmitter::Init();

	header = &hdr;
	blockType = NIPSYSMESHEMITTER;

	Get(file);
}

void NiPSysMeshEmitter::Get(fstream& file) {
	NiPSysEmitter::Get(file);

	file.read((char*)&numMeshes, 4);
	meshRefs.resize(numMeshes);
	for (int i = 0; i < numMeshes; i++)
		file.read((char*)&meshRefs[i], 4);

	file.read((char*)&velocityType, 4);
	file.read((char*)&emissionType, 4);
	file.read((char*)&emissionAxis, 12);
}

void NiPSysMeshEmitter::Put(fstream& file) {
	NiPSysEmitter::Put(file);

	file.write((char*)&numMeshes, 4);
	for (int i = 0; i < numMeshes; i++)
		file.write((char*)&meshRefs[i], 4);

	file.write((char*)&velocityType, 4);
	file.write((char*)&emissionType, 4);
	file.write((char*)&emissionAxis, 12);
}

void NiPSysMeshEmitter::notifyBlockDelete(int blockID) {
	NiPSysEmitter::notifyBlockDelete(blockID);

	for (int i = 0; i < numMeshes; i++) {
		if (meshRefs[i] == blockID) {
			meshRefs.erase(meshRefs.begin() + i);
			i--;
			numMeshes--;
		}
		else if (meshRefs[i] > blockID)
			meshRefs[i]--;
	}
}

void NiPSysMeshEmitter::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysEmitter::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numMeshes; i++) {
		if (meshRefs[i] == blockIndexLo)
			meshRefs[i] = blockIndexHi;
		else if (meshRefs[i] == blockIndexHi)
			meshRefs[i] = blockIndexLo;
	}
}

void NiPSysMeshEmitter::GetChildRefs(set<int>& refs) {
	NiPSysEmitter::GetChildRefs(refs);

	refs.insert(meshRefs.begin(), meshRefs.end());
}

int NiPSysMeshEmitter::CalcBlockSize() {
	NiPSysEmitter::CalcBlockSize();

	blockSize += 24;
	blockSize += numMeshes * 4;

	return blockSize;
}


void NiBlendInterpolator::Get(fstream& file) {
	NiInterpolator::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&unkInt, 4);
}

void NiBlendInterpolator::Put(fstream& file) {
	NiInterpolator::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&unkInt, 4);
}

int NiBlendInterpolator::CalcBlockSize() {
	NiInterpolator::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


NiBlendBoolInterpolator::NiBlendBoolInterpolator(NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDBOOLINTERPOLATOR;

	value = false;
}

NiBlendBoolInterpolator::NiBlendBoolInterpolator(fstream& file, NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDBOOLINTERPOLATOR;

	Get(file);
}

void NiBlendBoolInterpolator::Get(fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&value, 1);
}

void NiBlendBoolInterpolator::Put(fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&value, 1);
}

int NiBlendBoolInterpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 1;

	return blockSize;
}


NiBlendFloatInterpolator::NiBlendFloatInterpolator(NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDFLOATINTERPOLATOR;

	value = 0.0f;
}

NiBlendFloatInterpolator::NiBlendFloatInterpolator(fstream& file, NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDFLOATINTERPOLATOR;

	Get(file);
}

void NiBlendFloatInterpolator::Get(fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&value, 4);
}

void NiBlendFloatInterpolator::Put(fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&value, 4);
}

int NiBlendFloatInterpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDPOINT3INTERPOLATOR;
}

NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(fstream& file, NiHeader& hdr) {
	NiBlendInterpolator::Init();

	header = &hdr;
	blockType = NIBLENDPOINT3INTERPOLATOR;

	Get(file);
}

void NiBlendPoint3Interpolator::Get(fstream& file) {
	NiBlendInterpolator::Get(file);

	file.read((char*)&point, 12);
}

void NiBlendPoint3Interpolator::Put(fstream& file) {
	NiBlendInterpolator::Put(file);

	file.write((char*)&point, 12);
}

int NiBlendPoint3Interpolator::CalcBlockSize() {
	NiBlendInterpolator::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


NiBoolInterpolator::NiBoolInterpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIBOOLINTERPOLATOR;

	boolValue = 0;
	dataRef = 0xFFFFFFFF;
}

NiBoolInterpolator::NiBoolInterpolator(fstream& file, NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIBOOLINTERPOLATOR;

	Get(file);
}

void NiBoolInterpolator::Get(fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&boolValue, 1);
	file.read((char*)&dataRef, 4);
}

void NiBoolInterpolator::Put(fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&boolValue, 1);
	file.write((char*)&dataRef, 4);
}

void NiBoolInterpolator::notifyBlockDelete(int blockID) {
	NiKeyBasedInterpolator::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;
}
void NiBoolInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiKeyBasedInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

void NiBoolInterpolator::GetChildRefs(set<int>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(dataRef);
}

int NiBoolInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(NiHeader& hdr) : NiBoolInterpolator(hdr) {
	blockType = NIBOOLTIMELINEINTERPOLATOR;
}

NiBoolTimelineInterpolator::NiBoolTimelineInterpolator(fstream& file, NiHeader& hdr) : NiBoolInterpolator(file, hdr) {
	blockType = NIBOOLTIMELINEINTERPOLATOR;
}


NiFloatInterpolator::NiFloatInterpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIFLOATINTERPOLATOR;

	floatValue = 0.0f;
	dataRef = 0xFFFFFFFF;
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
		dataRef = 0xFFFFFFFF;
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

void NiFloatInterpolator::GetChildRefs(set<int>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(dataRef);
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
	dataRef = 0xFFFFFFFF;
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
		dataRef = 0xFFFFFFFF;
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

void NiTransformInterpolator::GetChildRefs(set<int>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(dataRef);
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

	dataRef = 0xFFFFFFFF;
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
		dataRef = 0xFFFFFFFF;
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

void NiPoint3Interpolator::GetChildRefs(set<int>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(dataRef);
}

int NiPoint3Interpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 16;

	return blockSize;
}


NiPathInterpolator::NiPathInterpolator(NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIPATHINTERPOLATOR;

	pathDataRef = 0xFFFFFFFF;
	percentDataRef = 0xFFFFFFFF;
}

NiPathInterpolator::NiPathInterpolator(fstream& file, NiHeader& hdr) {
	NiKeyBasedInterpolator::Init();

	header = &hdr;
	blockType = NIPATHINTERPOLATOR;

	Get(file);
}

void NiPathInterpolator::Get(fstream& file) {
	NiKeyBasedInterpolator::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&bankDir, 4);
	file.read((char*)&maxBankAngle, 4);
	file.read((char*)&smoothing, 4);
	file.read((char*)&followAxis, 2);
	file.read((char*)&pathDataRef, 4);
	file.read((char*)&percentDataRef, 4);
}

void NiPathInterpolator::Put(fstream& file) {
	NiKeyBasedInterpolator::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&bankDir, 4);
	file.write((char*)&maxBankAngle, 4);
	file.write((char*)&smoothing, 4);
	file.write((char*)&followAxis, 2);
	file.write((char*)&pathDataRef, 4);
	file.write((char*)&percentDataRef, 4);
}

void NiPathInterpolator::notifyBlockDelete(int blockID) {
	NiKeyBasedInterpolator::notifyBlockDelete(blockID);

	if (pathDataRef == blockID)
		pathDataRef = 0xFFFFFFFF;
	else if (pathDataRef > blockID)
		pathDataRef--;

	if (percentDataRef == blockID)
		percentDataRef = 0xFFFFFFFF;
	else if (percentDataRef > blockID)
		percentDataRef--;
}

void NiPathInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiKeyBasedInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (pathDataRef == blockIndexLo)
		pathDataRef = blockIndexHi;
	else if (pathDataRef == blockIndexHi)
		pathDataRef = blockIndexLo;

	if (percentDataRef == blockIndexLo)
		percentDataRef = blockIndexHi;
	else if (percentDataRef == blockIndexHi)
		percentDataRef = blockIndexLo;
}

void NiPathInterpolator::GetChildRefs(set<int>& refs) {
	NiKeyBasedInterpolator::GetChildRefs(refs);

	refs.insert(pathDataRef);
	refs.insert(percentDataRef);
}

int NiPathInterpolator::CalcBlockSize() {
	NiKeyBasedInterpolator::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


NiLookAtInterpolator::NiLookAtInterpolator(NiHeader& hdr) {
	NiInterpolator::Init();

	blockType = NILOOKATINTERPOLATOR;

	lookAtNameRef = 0xFFFFFFFF;
}

NiLookAtInterpolator::NiLookAtInterpolator(fstream& file, NiHeader& hdr) {
	NiInterpolator::Init();

	blockType = NILOOKATINTERPOLATOR;
	Get(file);
}

NiLookAtInterpolator::~NiLookAtInterpolator() {
	header->RemoveStringRef(lookAtNameRef);
}

void NiLookAtInterpolator::Get(fstream& file) {
	NiInterpolator::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&lookAtRef, 4);

	file.read((char*)&lookAtNameRef, 4);
	if (lookAtNameRef != 0xFFFFFFFF) {
		lookAtName = header->GetStringById(lookAtNameRef);
		header->AddStringRef(lookAtNameRef);
	}

	file.read((char*)&transform, 32);
	file.read((char*)&translateInterpRef, 4);
	file.read((char*)&rollInterpRef, 4);
	file.read((char*)&scaleInterpRef, 4);
}

void NiLookAtInterpolator::Put(fstream& file) {
	NiInterpolator::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&lookAtRef, 4);
	file.write((char*)&lookAtNameRef, 4);
	file.write((char*)&transform, 32);
	file.write((char*)&translateInterpRef, 4);
	file.write((char*)&rollInterpRef, 4);
	file.write((char*)&scaleInterpRef, 4);
}

void NiLookAtInterpolator::notifyBlockDelete(int blockID) {
	NiInterpolator::notifyBlockDelete(blockID);

	if (lookAtRef == blockID)
		lookAtRef = 0xFFFFFFFF;
	else if (lookAtRef > blockID)
		lookAtRef--;

	if (translateInterpRef == blockID)
		translateInterpRef = 0xFFFFFFFF;
	else if (translateInterpRef > blockID)
		translateInterpRef--;

	if (rollInterpRef == blockID)
		rollInterpRef = 0xFFFFFFFF;
	else if (rollInterpRef > blockID)
		rollInterpRef--;

	if (scaleInterpRef == blockID)
		scaleInterpRef = 0xFFFFFFFF;
	else if (scaleInterpRef > blockID)
		scaleInterpRef--;
}

void NiLookAtInterpolator::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiInterpolator::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (lookAtRef == blockIndexLo)
		lookAtRef = blockIndexHi;
	else if (lookAtRef == blockIndexHi)
		lookAtRef = blockIndexLo;

	if (translateInterpRef == blockIndexLo)
		translateInterpRef = blockIndexHi;
	else if (translateInterpRef == blockIndexHi)
		translateInterpRef = blockIndexLo;

	if (rollInterpRef == blockIndexLo)
		rollInterpRef = blockIndexHi;
	else if (rollInterpRef == blockIndexHi)
		rollInterpRef = blockIndexLo;

	if (scaleInterpRef == blockIndexLo)
		scaleInterpRef = blockIndexHi;
	else if (scaleInterpRef == blockIndexHi)
		scaleInterpRef = blockIndexLo;
}

void NiLookAtInterpolator::notifyStringDelete(int stringID) {
	NiInterpolator::notifyStringDelete(stringID);

	if (lookAtNameRef != 0xFFFFFFFF && lookAtNameRef > stringID)
		lookAtNameRef--;
}

void NiLookAtInterpolator::GetChildRefs(set<int>& refs) {
	NiInterpolator::GetChildRefs(refs);

	refs.insert(lookAtRef);
	refs.insert(translateInterpRef);
	refs.insert(rollInterpRef);
	refs.insert(scaleInterpRef);
}

int NiLookAtInterpolator::CalcBlockSize() {
	NiInterpolator::CalcBlockSize();

	blockSize += 54;

	return blockSize;
}


NiKeyframeData::NiKeyframeData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIKEYFRAMEDATA;

	numRotationKeys = 0;
}

NiKeyframeData::NiKeyframeData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIKEYFRAMEDATA;

	Get(file);
}

void NiKeyframeData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numRotationKeys, 4);

	if (numRotationKeys > 0) {
		file.read((char*)&rotationType, 4);

		if (rotationType != 4) {
			quaternionKeys.resize(numRotationKeys);

			for (int i = 0; i < numRotationKeys; i++) {
				file.read((char*)&quaternionKeys[i].time, 4);
				file.read((char*)&quaternionKeys[i].value, 16);

				if (rotationType == 3)
					file.read((char*)&quaternionKeys[i].tbc, 12);
			}
		}
		else {
			xRotations.Get(file);
			yRotations.Get(file);
			zRotations.Get(file);
		}
	}

	translations.Get(file);
	scales.Get(file);
}

void NiKeyframeData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numRotationKeys, 4);

	if (numRotationKeys > 0) {
		file.write((char*)&rotationType, 4);

		if (rotationType != 4) {
			for (int i = 0; i < numRotationKeys; i++) {
				file.write((char*)&quaternionKeys[i].time, 4);
				file.write((char*)&quaternionKeys[i].value, 16);

				if (rotationType == 3)
					file.write((char*)&quaternionKeys[i].tbc, 12);
			}
		}
		else {
			xRotations.Put(file);
			yRotations.Put(file);
			zRotations.Put(file);
		}
	}

	translations.Put(file);
	scales.Put(file);
}

int NiKeyframeData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	if (numRotationKeys > 0) {
		blockSize += 4;

		if (rotationType != 4) {
			blockSize += 20 * numRotationKeys;

			if (rotationType == 3)
				blockSize += 12 * numRotationKeys;
		}
		else {
			blockSize += xRotations.CalcGroupSize();
			blockSize += yRotations.CalcGroupSize();
			blockSize += zRotations.CalcGroupSize();
		}
	}

	blockSize += translations.CalcGroupSize();
	blockSize += scales.CalcGroupSize();

	return blockSize;
}


NiTransformData::NiTransformData(NiHeader& hdr) : NiKeyframeData(hdr) {
	blockType = NITRANSFORMDATA;
}

NiTransformData::NiTransformData(fstream& file, NiHeader& hdr) : NiKeyframeData(file, hdr) {
	blockType = NITRANSFORMDATA;
}


NiPosData::NiPosData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIPOSDATA;
}

NiPosData::NiPosData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIPOSDATA;

	Get(file);
}

void NiPosData::Get(fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiPosData::Put(fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiPosData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiBoolData::NiBoolData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIBOOLDATA;
}

NiBoolData::NiBoolData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIBOOLDATA;

	Get(file);
}

void NiBoolData::Get(fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiBoolData::Put(fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiBoolData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


NiFloatData::NiFloatData(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIFLOATDATA;
}

NiFloatData::NiFloatData(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NIFLOATDATA;

	Get(file);
}

void NiFloatData::Get(fstream& file) {
	NiObject::Get(file);

	data.Get(file);
}

void NiFloatData::Put(fstream& file) {
	NiObject::Put(file);

	data.Put(file);
}

int NiFloatData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += data.CalcGroupSize();

	return blockSize;
}


void NiTimeController::Init() {
	NiObject::Init();

	nextControllerRef = 0xFFFFFFFF;
	flags = 0x000C;
	frequency = 1.0f;
	phase = 0.0f;
	startTime = 0.0f;
	stopTime = 0.0f;
	targetRef = 0xFFFFFFFF;
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
		nextControllerRef = 0xFFFFFFFF;
	else if (nextControllerRef > blockID)
		nextControllerRef--;

	if (targetRef == blockID)
		targetRef = 0xFFFFFFFF;
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

void NiTimeController::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(nextControllerRef);
	refs.insert(targetRef);
}

int NiTimeController::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 26;

	return blockSize;
}


BSFrustumFOVController::BSFrustumFOVController(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSFRUSTUMFOVCONTROLLER;

	interpolatorRef = 0xFFFFFFFF;
}

BSFrustumFOVController::BSFrustumFOVController(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSFRUSTUMFOVCONTROLLER;

	Get(file);
}

void BSFrustumFOVController::Get(fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&interpolatorRef, 4);
}

void BSFrustumFOVController::Put(fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&interpolatorRef, 4);
}

void BSFrustumFOVController::notifyBlockDelete(int blockID) {
	NiTimeController::notifyBlockDelete(blockID);

	if (interpolatorRef == blockID)
		interpolatorRef = 0xFFFFFFFF;
	else if (interpolatorRef > blockID)
		interpolatorRef--;
}

void BSFrustumFOVController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTimeController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (interpolatorRef == blockIndexLo)
		interpolatorRef = blockIndexHi;
	else if (interpolatorRef == blockIndexHi)
		interpolatorRef = blockIndexLo;
}

void BSFrustumFOVController::GetChildRefs(set<int>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(interpolatorRef);
}

int BSFrustumFOVController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSLagBoneController::BSLagBoneController(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSLAGBONECONTROLLER;
}

BSLagBoneController::BSLagBoneController(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSLAGBONECONTROLLER;

	Get(file);
}

void BSLagBoneController::Get(fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&linearVelocity, 4);
	file.read((char*)&linearRotation, 4);
	file.read((char*)&maxDistance, 4);
}

void BSLagBoneController::Put(fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&linearVelocity, 4);
	file.write((char*)&linearRotation, 4);
	file.write((char*)&maxDistance, 4);
}

int BSLagBoneController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


BSProceduralLightningController::BSProceduralLightningController(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSPROCEDURALLIGHTNINGCONTROLLER;
}

BSProceduralLightningController::BSProceduralLightningController(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = BSPROCEDURALLIGHTNINGCONTROLLER;

	Get(file);
}

void BSProceduralLightningController::Get(fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&generationInterpRef, 4);
	file.read((char*)&mutationInterpRef, 4);
	file.read((char*)&subdivisionInterpRef, 4);
	file.read((char*)&numBranchesInterpRef, 4);
	file.read((char*)&numBranchesVarInterpRef, 4);
	file.read((char*)&lengthInterpRef, 4);
	file.read((char*)&lengthVarInterpRef, 4);
	file.read((char*)&widthInterpRef, 4);
	file.read((char*)&arcOffsetInterpRef, 4);

	file.read((char*)&subdivisions, 2);
	file.read((char*)&numBranches, 2);
	file.read((char*)&numBranchesPerVariation, 2);

	file.read((char*)&length, 4);
	file.read((char*)&lengthVariation, 4);
	file.read((char*)&width, 4);
	file.read((char*)&childWidthMult, 4);
	file.read((char*)&arcOffset, 4);

	file.read((char*)&fadeMainBolt, 1);
	file.read((char*)&fadeChildBolts, 1);
	file.read((char*)&animateArcOffset, 1);

	file.read((char*)&shaderPropertyRef, 4);
}

void BSProceduralLightningController::Put(fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&generationInterpRef, 4);
	file.write((char*)&mutationInterpRef, 4);
	file.write((char*)&subdivisionInterpRef, 4);
	file.write((char*)&numBranchesInterpRef, 4);
	file.write((char*)&numBranchesVarInterpRef, 4);
	file.write((char*)&lengthInterpRef, 4);
	file.write((char*)&lengthVarInterpRef, 4);
	file.write((char*)&widthInterpRef, 4);
	file.write((char*)&arcOffsetInterpRef, 4);

	file.write((char*)&subdivisions, 2);
	file.write((char*)&numBranches, 2);
	file.write((char*)&numBranchesPerVariation, 2);

	file.write((char*)&length, 4);
	file.write((char*)&lengthVariation, 4);
	file.write((char*)&width, 4);
	file.write((char*)&childWidthMult, 4);
	file.write((char*)&arcOffset, 4);

	file.write((char*)&fadeMainBolt, 1);
	file.write((char*)&fadeChildBolts, 1);
	file.write((char*)&animateArcOffset, 1);

	file.write((char*)&shaderPropertyRef, 4);
}

void BSProceduralLightningController::notifyBlockDelete(int blockID) {
	NiTimeController::notifyBlockDelete(blockID);

	if (generationInterpRef == blockID)
		generationInterpRef = 0xFFFFFFFF;
	else if (generationInterpRef > blockID)
		generationInterpRef--;

	if (mutationInterpRef == blockID)
		mutationInterpRef = 0xFFFFFFFF;
	else if (mutationInterpRef > blockID)
		mutationInterpRef--;

	if (subdivisionInterpRef == blockID)
		subdivisionInterpRef = 0xFFFFFFFF;
	else if (subdivisionInterpRef > blockID)
		subdivisionInterpRef--;

	if (numBranchesInterpRef == blockID)
		numBranchesInterpRef = 0xFFFFFFFF;
	else if (numBranchesInterpRef > blockID)
		numBranchesInterpRef--;

	if (numBranchesVarInterpRef == blockID)
		numBranchesVarInterpRef = 0xFFFFFFFF;
	else if (numBranchesVarInterpRef > blockID)
		numBranchesVarInterpRef--;

	if (lengthInterpRef == blockID)
		lengthInterpRef = 0xFFFFFFFF;
	else if (lengthInterpRef > blockID)
		lengthInterpRef--;

	if (lengthVarInterpRef == blockID)
		lengthVarInterpRef = 0xFFFFFFFF;
	else if (lengthVarInterpRef > blockID)
		lengthVarInterpRef--;

	if (widthInterpRef == blockID)
		widthInterpRef = 0xFFFFFFFF;
	else if (widthInterpRef > blockID)
		widthInterpRef--;

	if (arcOffsetInterpRef == blockID)
		arcOffsetInterpRef = 0xFFFFFFFF;
	else if (arcOffsetInterpRef > blockID)
		arcOffsetInterpRef--;

	if (shaderPropertyRef == blockID)
		shaderPropertyRef = 0xFFFFFFFF;
	else if (shaderPropertyRef > blockID)
		shaderPropertyRef--;
}

void BSProceduralLightningController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTimeController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (generationInterpRef == blockIndexLo)
		generationInterpRef = blockIndexHi;
	else if (generationInterpRef == blockIndexHi)
		generationInterpRef = blockIndexLo;

	if (mutationInterpRef == blockIndexLo)
		mutationInterpRef = blockIndexHi;
	else if (mutationInterpRef == blockIndexHi)
		mutationInterpRef = blockIndexLo;

	if (subdivisionInterpRef == blockIndexLo)
		subdivisionInterpRef = blockIndexHi;
	else if (subdivisionInterpRef == blockIndexHi)
		subdivisionInterpRef = blockIndexLo;

	if (numBranchesInterpRef == blockIndexLo)
		numBranchesInterpRef = blockIndexHi;
	else if (numBranchesInterpRef == blockIndexHi)
		numBranchesInterpRef = blockIndexLo;

	if (numBranchesVarInterpRef == blockIndexLo)
		numBranchesVarInterpRef = blockIndexHi;
	else if (numBranchesVarInterpRef == blockIndexHi)
		numBranchesVarInterpRef = blockIndexLo;

	if (lengthInterpRef == blockIndexLo)
		lengthInterpRef = blockIndexHi;
	else if (lengthInterpRef == blockIndexHi)
		lengthInterpRef = blockIndexLo;

	if (lengthVarInterpRef == blockIndexLo)
		lengthVarInterpRef = blockIndexHi;
	else if (lengthVarInterpRef == blockIndexHi)
		lengthVarInterpRef = blockIndexLo;

	if (widthInterpRef == blockIndexLo)
		widthInterpRef = blockIndexHi;
	else if (widthInterpRef == blockIndexHi)
		widthInterpRef = blockIndexLo;

	if (arcOffsetInterpRef == blockIndexLo)
		arcOffsetInterpRef = blockIndexHi;
	else if (arcOffsetInterpRef == blockIndexHi)
		arcOffsetInterpRef = blockIndexLo;

	if (shaderPropertyRef == blockIndexLo)
		shaderPropertyRef = blockIndexHi;
	else if (shaderPropertyRef == blockIndexHi)
		shaderPropertyRef = blockIndexLo;
}

void BSProceduralLightningController::GetChildRefs(set<int>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(generationInterpRef);
	refs.insert(mutationInterpRef);
	refs.insert(subdivisionInterpRef);
	refs.insert(numBranchesInterpRef);
	refs.insert(numBranchesVarInterpRef);
	refs.insert(lengthInterpRef);
	refs.insert(lengthVarInterpRef);
	refs.insert(widthInterpRef);
	refs.insert(arcOffsetInterpRef);
	refs.insert(shaderPropertyRef);
}

int BSProceduralLightningController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 69;

	return blockSize;
}


NiBoneLODController::NiBoneLODController(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NIBONELODCONTROLLER;

	numLODs = 0;
	boneArraysSize = 0;
}

NiBoneLODController::NiBoneLODController(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NIBONELODCONTROLLER;

	Get(file);
}

void NiBoneLODController::Get(fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&lod, 4);
	file.read((char*)&numLODs, 4);

	file.read((char*)&boneArraysSize, 4);
	boneArrays.resize(boneArraysSize);
	for (int i = 0; i < boneArraysSize; i++) {
		file.read((char*)&boneArrays[i].nodeSetSize, 4);
		boneArrays[i].nodeRefs.resize(boneArrays[i].nodeSetSize);
		for (int j = 0; j < boneArrays[i].nodeSetSize; j++)
			file.read((char*)&boneArrays[i].nodeRefs[j], 4);
	}
}

void NiBoneLODController::Put(fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&lod, 4);
	file.write((char*)&numLODs, 4);

	file.write((char*)&boneArraysSize, 4);
	for (int i = 0; i < boneArraysSize; i++) {
		file.write((char*)&boneArrays[i].nodeSetSize, 4);
		for (int j = 0; j < boneArrays[i].nodeSetSize; j++)
			file.write((char*)&boneArrays[i].nodeRefs[j], 4);
	}
}

void NiBoneLODController::notifyBlockDelete(int blockID) {
	NiTimeController::notifyBlockDelete(blockID);

	for (int i = 0; i < boneArraysSize; i++) {
		for (int j = 0; j < boneArrays[i].nodeSetSize; j++) {
			if (boneArrays[i].nodeRefs[j] == blockID) {
				boneArrays[i].nodeRefs.erase(boneArrays[i].nodeRefs.begin() + j);
				j--;
				boneArrays[i].nodeSetSize--;
			}
			else if (boneArrays[i].nodeRefs[j] > blockID)
				boneArrays[i].nodeRefs[j]--;
		}

		if (boneArrays[i].nodeSetSize == 0) {
			boneArrays.erase(boneArrays.begin() + i);
			i--;
			boneArraysSize--;
		}
	}
}

void NiBoneLODController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTimeController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < boneArraysSize; i++) {
		for (int j = 0; j < boneArrays[i].nodeSetSize; j++) {
			if (boneArrays[i].nodeRefs[j] == blockIndexLo)
				boneArrays[i].nodeRefs[j] = blockIndexHi;
			else if (boneArrays[i].nodeRefs[j] == blockIndexHi)
				boneArrays[i].nodeRefs[j] = blockIndexLo;
		}
	}
}

void NiBoneLODController::GetChildRefs(set<int>& refs) {
	NiTimeController::GetChildRefs(refs);

	for (int i = 0; i < boneArraysSize; i++)
		refs.insert(boneArrays[i].nodeRefs.begin(), boneArrays[i].nodeRefs.end());
}

int NiBoneLODController::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 12;
	blockSize += boneArraysSize * 4;
	for (int i = 0; i < boneArraysSize; i++)
		blockSize += boneArrays[i].nodeSetSize * 4;

	return blockSize;
}


void NiSingleInterpController::Init() {
	NiInterpController::Init();

	interpolatorRef = 0xFFFFFFFF;
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
		interpolatorRef = 0xFFFFFFFF;
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

void NiSingleInterpController::GetChildRefs(set<int>& refs) {
	NiInterpController::GetChildRefs(refs);

	refs.insert(interpolatorRef);
}

int NiSingleInterpController::CalcBlockSize() {
	NiInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiFloatExtraDataController::NiFloatExtraDataController(NiHeader& hdr) {
	NiExtraDataController::Init();

	header = &hdr;
	blockType = NIFLOATEXTRADATACONTROLLER;

	extraDataRef = 0xFFFFFFFF;
}

NiFloatExtraDataController::NiFloatExtraDataController(fstream &file, NiHeader& hdr) {
	NiExtraDataController::Init();

	header = &hdr;
	blockType = NIFLOATEXTRADATACONTROLLER;

	Get(file);
}

NiFloatExtraDataController::~NiFloatExtraDataController() {
	header->RemoveStringRef(extraDataRef);
}

void NiFloatExtraDataController::Get(fstream& file) {
	NiExtraDataController::Get(file);

	file.read((char*)&extraDataRef, 4);
	if (extraDataRef != 0xFFFFFFFF) {
		extraData = header->GetStringById(extraDataRef);
		header->AddStringRef(extraDataRef);
	}
}

void NiFloatExtraDataController::Put(fstream& file) {
	NiExtraDataController::Put(file);

	file.write((char*)&extraDataRef, 4);
}

void NiFloatExtraDataController::notifyStringDelete(int stringID) {
	NiExtraDataController::notifyStringDelete(stringID);

	if (extraDataRef != 0xFFFFFFFF && extraDataRef > stringID)
		extraDataRef--;
}

int NiFloatExtraDataController::CalcBlockSize() {
	NiExtraDataController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiVisController::NiVisController(NiHeader& hdr) {
	NiBoolInterpController::Init();

	header = &hdr;
	blockType = NIVISCONTROLLER;
}

NiVisController::NiVisController(fstream &file, NiHeader& hdr) {
	NiBoolInterpController::Init();

	header = &hdr;
	blockType = NIVISCONTROLLER;

	Get(file);
}


NiAlphaController::NiAlphaController(NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = NIALPHACONTROLLER;
}

NiAlphaController::NiAlphaController(fstream &file, NiHeader& hdr) {
	NiFloatInterpController::Init();

	header = &hdr;
	blockType = NIALPHACONTROLLER;

	Get(file);
}


NiPSysUpdateCtlr::NiPSysUpdateCtlr(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NIPSYSUPDATECTLR;
}

NiPSysUpdateCtlr::NiPSysUpdateCtlr(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NIPSYSUPDATECTLR;

	Get(file);
}


BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController(NiHeader& hdr) : NiAlphaController(hdr) {
	blockType = BSNIALPHAPROPERTYTESTREFCONTROLLER;
}

BSNiAlphaPropertyTestRefController::BSNiAlphaPropertyTestRefController(fstream &file, NiHeader& hdr) : NiAlphaController(file, hdr) {
	blockType = BSNIALPHAPROPERTYTESTREFCONTROLLER;
}


NiKeyframeController::NiKeyframeController(NiHeader& hdr) {
	NiSingleInterpController::Init();

	header = &hdr;
	blockType = NIKEYFRAMECONTROLLER;
}

NiKeyframeController::NiKeyframeController(fstream &file, NiHeader& hdr) {
	NiSingleInterpController::Init();

	header = &hdr;
	blockType = NIKEYFRAMECONTROLLER;

	Get(file);
}


NiTransformController::NiTransformController(NiHeader& hdr) : NiKeyframeController(hdr) {
	blockType = NITRANSFORMCONTROLLER;
}

NiTransformController::NiTransformController(fstream &file, NiHeader& hdr) : NiKeyframeController(file, hdr) {
	blockType = NITRANSFORMCONTROLLER;
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

int BSEffectShaderPropertyFloatController::CalcBlockSize() {
	NiFloatInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiMultiTargetTransformController::NiMultiTargetTransformController(NiHeader& hdr) {
	NiInterpController::Init();

	header = &hdr;
	blockType = NIMULTITARGETTRANSFORMCONTROLLER;

	numTargets = 0;
}

NiMultiTargetTransformController::NiMultiTargetTransformController(fstream& file, NiHeader& hdr) {
	NiInterpController::Init();

	header = &hdr;
	blockType = NIMULTITARGETTRANSFORMCONTROLLER;

	Get(file);
}

void NiMultiTargetTransformController::Get(fstream& file) {
	NiInterpController::Get(file);

	file.read((char*)&numTargets, 2);
	targets.resize(numTargets);
	for (int i = 0; i < numTargets; i++)
		file.read((char*)&targets[i], 4);
}

void NiMultiTargetTransformController::Put(fstream& file) {
	NiInterpController::Put(file);

	file.write((char*)&numTargets, 2);
	for (int i = 0; i < numTargets; i++)
		file.write((char*)&targets[i], 4);
}

void NiMultiTargetTransformController::notifyBlockDelete(int blockID) {
	NiInterpController::notifyBlockDelete(blockID);

	for (int i = 0; i < numTargets; i++) {
		if (targets[i] == blockID) {
			targets.erase(targets.begin() + i);
			i--;
			numTargets--;
		}
		else if (targets[i] > blockID)
			targets[i]--;
	}
}

void NiMultiTargetTransformController::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiInterpController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numTargets; i++) {
		if (targets[i] == blockIndexLo)
			targets[i] = blockIndexHi;
		else if (targets[i] == blockIndexHi)
			targets[i] = blockIndexLo;
	}
}

void NiMultiTargetTransformController::GetChildRefs(set<int>& refs) {
	NiInterpController::GetChildRefs(refs);

	refs.insert(targets.begin(), targets.end());
}

int NiMultiTargetTransformController::CalcBlockSize() {
	NiInterpController::CalcBlockSize();

	blockSize += 2;
	blockSize += numTargets * 4;

	return blockSize;
}


NiPSysModifierCtlr::~NiPSysModifierCtlr() {
	header->RemoveStringRef(modifierNameRef);
}

void NiPSysModifierCtlr::Init() {
	NiSingleInterpController::Init();

	modifierNameRef = 0xFFFFFFFF;
	modifierName.clear();
}

void NiPSysModifierCtlr::Get(fstream& file) {
	NiSingleInterpController::Get(file);

	file.read((char*)&modifierNameRef, 4);
	if (modifierNameRef != 0xFFFFFFFF) {
		modifierName = header->GetStringById(modifierNameRef);
		header->AddStringRef(modifierNameRef);
	}
	else
		modifierName.clear();
}

void NiPSysModifierCtlr::Put(fstream& file) {
	NiSingleInterpController::Put(file);

	file.write((char*)&modifierNameRef, 4);
}

void NiPSysModifierCtlr::notifyStringDelete(int stringID) {
	NiSingleInterpController::notifyStringDelete(stringID);

	if (modifierNameRef != 0xFFFFFFFF && modifierNameRef > stringID)
		modifierNameRef--;
}

int NiPSysModifierCtlr::CalcBlockSize() {
	NiSingleInterpController::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSMODIFIERACTIVECTLR;
}

NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSMODIFIERACTIVECTLR;

	Get(file);
}


NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERLIFESPANCTLR;
}

NiPSysEmitterLifeSpanCtlr::NiPSysEmitterLifeSpanCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERLIFESPANCTLR;

	Get(file);
}


NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERSPEEDCTLR;
}

NiPSysEmitterSpeedCtlr::NiPSysEmitterSpeedCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERSPEEDCTLR;

	Get(file);
}


NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERINITIALRADIUSCTLR;
}

NiPSysEmitterInitialRadiusCtlr::NiPSysEmitterInitialRadiusCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERINITIALRADIUSCTLR;

	Get(file);
}


NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERPLANARANGLECTLR;
}

NiPSysEmitterPlanarAngleCtlr::NiPSysEmitterPlanarAngleCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERPLANARANGLECTLR;

	Get(file);
}


NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERDECLINATIONCTLR;
}

NiPSysEmitterDeclinationCtlr::NiPSysEmitterDeclinationCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERDECLINATIONCTLR;

	Get(file);
}


NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSGRAVITYSTRENGTHCTLR;
}

NiPSysGravityStrengthCtlr::NiPSysGravityStrengthCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSGRAVITYSTRENGTHCTLR;

	Get(file);
}


NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSINITIALROTSPEEDCTLR;
}

NiPSysInitialRotSpeedCtlr::NiPSysInitialRotSpeedCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSINITIALROTSPEEDCTLR;

	Get(file);
}


NiPSysEmitterCtlr::NiPSysEmitterCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERCTLR;

	visInterpolatorRef = 0xFFFFFFFF;
}

NiPSysEmitterCtlr::NiPSysEmitterCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSEMITTERCTLR;

	Get(file);
}

void NiPSysEmitterCtlr::Get(fstream& file) {
	NiPSysModifierCtlr::Get(file);

	file.read((char*)&visInterpolatorRef, 4);
}

void NiPSysEmitterCtlr::Put(fstream& file) {
	NiPSysModifierCtlr::Put(file);

	file.write((char*)&visInterpolatorRef, 4);
}

void NiPSysEmitterCtlr::notifyBlockDelete(int blockID) {
	NiPSysModifierCtlr::notifyBlockDelete(blockID);

	if (visInterpolatorRef == blockID)
		visInterpolatorRef = 0xFFFFFFFF;
	else if (visInterpolatorRef > blockID)
		visInterpolatorRef--;
}

void NiPSysEmitterCtlr::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifierCtlr::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (visInterpolatorRef == blockIndexLo)
		visInterpolatorRef = blockIndexHi;
	else if (visInterpolatorRef == blockIndexHi)
		visInterpolatorRef = blockIndexLo;
}

void NiPSysEmitterCtlr::GetChildRefs(set<int>& refs) {
	NiPSysModifierCtlr::GetChildRefs(refs);

	refs.insert(visInterpolatorRef);
}

int NiPSysEmitterCtlr::CalcBlockSize() {
	NiPSysModifierCtlr::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiPSysMultiTargetEmitterCtlr::NiPSysMultiTargetEmitterCtlr(NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSMULTITARGETEMITTERCTLR;

	masterParticleSystemRef = 0xFFFFFFFF;
}

NiPSysMultiTargetEmitterCtlr::NiPSysMultiTargetEmitterCtlr(fstream& file, NiHeader& hdr) {
	NiPSysModifierCtlr::Init();

	header = &hdr;
	blockType = NIPSYSMULTITARGETEMITTERCTLR;

	Get(file);
}

void NiPSysMultiTargetEmitterCtlr::Get(fstream& file) {
	NiPSysModifierCtlr::Get(file);

	file.read((char*)&maxEmitters, 2);
	file.read((char*)&masterParticleSystemRef, 4);
}

void NiPSysMultiTargetEmitterCtlr::Put(fstream& file) {
	NiPSysModifierCtlr::Put(file);

	file.write((char*)&maxEmitters, 2);
	file.write((char*)&masterParticleSystemRef, 4);
}

void NiPSysMultiTargetEmitterCtlr::notifyBlockDelete(int blockID) {
	NiPSysModifierCtlr::notifyBlockDelete(blockID);

	if (masterParticleSystemRef == blockID)
		masterParticleSystemRef = 0xFFFFFFFF;
	else if (masterParticleSystemRef > blockID)
		masterParticleSystemRef--;
}

void NiPSysMultiTargetEmitterCtlr::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiPSysModifierCtlr::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (masterParticleSystemRef == blockIndexLo)
		masterParticleSystemRef = blockIndexHi;
	else if (masterParticleSystemRef == blockIndexHi)
		masterParticleSystemRef = blockIndexLo;
}

void NiPSysMultiTargetEmitterCtlr::GetChildRefs(set<int>& refs) {
	NiPSysModifierCtlr::GetChildRefs(refs);

	refs.insert(masterParticleSystemRef);
}

int NiPSysMultiTargetEmitterCtlr::CalcBlockSize() {
	NiPSysModifierCtlr::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}


NiControllerManager::NiControllerManager(NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NICONTROLLERMANAGER;

	numControllerSequences = 0;
	objectPaletteRef = 0xFFFFFFFF;
}

NiControllerManager::NiControllerManager(fstream& file, NiHeader& hdr) {
	NiTimeController::Init();

	header = &hdr;
	blockType = NICONTROLLERMANAGER;

	Get(file);
}

void NiControllerManager::Get(fstream& file) {
	NiTimeController::Get(file);

	file.read((char*)&cumulative, 1);

	file.read((char*)&numControllerSequences, 4);
	controllerSequenceRefs.resize(numControllerSequences);
	for (int i = 0; i < numControllerSequences; i++)
		file.read((char*)&controllerSequenceRefs[i], 4);

	file.read((char*)&objectPaletteRef, 4);
}

void NiControllerManager::Put(fstream& file) {
	NiTimeController::Put(file);

	file.write((char*)&cumulative, 1);

	file.write((char*)&numControllerSequences, 4);
	for (int i = 0; i < numControllerSequences; i++)
		file.write((char*)&controllerSequenceRefs[i], 4);

	file.write((char*)&objectPaletteRef, 4);
}

void NiControllerManager::notifyBlockDelete(int blockID) {
	NiTimeController::notifyBlockDelete(blockID);

	for (int i = 0; i < numControllerSequences; i++) {
		if (controllerSequenceRefs[i] == blockID) {
			controllerSequenceRefs.erase(controllerSequenceRefs.begin() + i);
			i--;
			numControllerSequences--;
		}
		else if (controllerSequenceRefs[i] > blockID)
			controllerSequenceRefs[i]--;
	}

	if (objectPaletteRef == blockID)
		objectPaletteRef = 0xFFFFFFFF;
	else if (objectPaletteRef > blockID)
		objectPaletteRef--;
}

void NiControllerManager::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiTimeController::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numControllerSequences; i++) {
		if (controllerSequenceRefs[i] == blockIndexLo)
			controllerSequenceRefs[i] = blockIndexHi;
		else if (controllerSequenceRefs[i] == blockIndexHi)
			controllerSequenceRefs[i] = blockIndexLo;
	}

	if (objectPaletteRef == blockIndexLo)
		objectPaletteRef = blockIndexHi;
	else if (objectPaletteRef == blockIndexHi)
		objectPaletteRef = blockIndexLo;
}

void NiControllerManager::GetChildRefs(set<int>& refs) {
	NiTimeController::GetChildRefs(refs);

	refs.insert(controllerSequenceRefs.begin(), controllerSequenceRefs.end());
	refs.insert(objectPaletteRef);
}

int NiControllerManager::CalcBlockSize() {
	NiTimeController::CalcBlockSize();

	blockSize += 9;
	blockSize += numControllerSequences * 4;

	return blockSize;
}


NiSequence::NiSequence(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISEQUENCE;

	nameRef = 0xFFFFFFFF;
	numControlledBlocks = 0;
}

NiSequence::NiSequence(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NISEQUENCE;

	Get(file);
}

NiSequence::~NiSequence() {
	header->RemoveStringRef(nameRef);
	for (int i = 0; i < numControlledBlocks; i++) {
		header->RemoveStringRef(controlledBlocks[i].nodeNameRef);
		header->RemoveStringRef(controlledBlocks[i].propTypeRef);
		header->RemoveStringRef(controlledBlocks[i].ctrlTypeRef);
		header->RemoveStringRef(controlledBlocks[i].ctrlIDRef);
		header->RemoveStringRef(controlledBlocks[i].interpIDRef);
	}
}

void NiSequence::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nameRef, 4);
	if (nameRef != 0xFFFFFFFF) {
		name = header->GetStringById(nameRef);
		header->AddStringRef(nameRef);
	}

	file.read((char*)&numControlledBlocks, 4);
	file.read((char*)&unkInt1, 4);

	controlledBlocks.resize(numControlledBlocks);
	for (int i = 0; i < numControlledBlocks; i++) {
		file.read((char*)&controlledBlocks[i].interpolatorRef, 4);
		file.read((char*)&controlledBlocks[i].controllerRef, 4);
		file.read((char*)&controlledBlocks[i].priority, 1);

		file.read((char*)&controlledBlocks[i].nodeNameRef, 4);
		if (controlledBlocks[i].nodeNameRef != 0xFFFFFFFF) {
			controlledBlocks[i].nodeName = header->GetStringById(controlledBlocks[i].nodeNameRef);
			header->AddStringRef(controlledBlocks[i].nodeNameRef);
		}

		file.read((char*)&controlledBlocks[i].propTypeRef, 4);
		if (controlledBlocks[i].propTypeRef != 0xFFFFFFFF) {
			controlledBlocks[i].propType = header->GetStringById(controlledBlocks[i].propTypeRef);
			header->AddStringRef(controlledBlocks[i].propTypeRef);
		}

		file.read((char*)&controlledBlocks[i].ctrlTypeRef, 4);
		if (controlledBlocks[i].ctrlTypeRef != 0xFFFFFFFF) {
			controlledBlocks[i].ctrlType = header->GetStringById(controlledBlocks[i].ctrlTypeRef);
			header->AddStringRef(controlledBlocks[i].ctrlTypeRef);
		}

		file.read((char*)&controlledBlocks[i].ctrlIDRef, 4);
		if (controlledBlocks[i].ctrlIDRef != 0xFFFFFFFF) {
			controlledBlocks[i].ctrlID = header->GetStringById(controlledBlocks[i].ctrlIDRef);
			header->AddStringRef(controlledBlocks[i].ctrlIDRef);
		}

		file.read((char*)&controlledBlocks[i].interpIDRef, 4);
		if (controlledBlocks[i].interpIDRef != 0xFFFFFFFF) {
			controlledBlocks[i].interpID = header->GetStringById(controlledBlocks[i].interpIDRef);
			header->AddStringRef(controlledBlocks[i].interpIDRef);
		}
	}
}

void NiSequence::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nameRef, 4);
	file.write((char*)&numControlledBlocks, 4);
	file.write((char*)&unkInt1, 4);

	for (int i = 0; i < numControlledBlocks; i++) {
		file.write((char*)&controlledBlocks[i].interpolatorRef, 4);
		file.write((char*)&controlledBlocks[i].controllerRef, 4);
		file.write((char*)&controlledBlocks[i].priority, 1);
		file.write((char*)&controlledBlocks[i].nodeNameRef, 4);
		file.write((char*)&controlledBlocks[i].propTypeRef, 4);
		file.write((char*)&controlledBlocks[i].ctrlTypeRef, 4);
		file.write((char*)&controlledBlocks[i].ctrlIDRef, 4);
		file.write((char*)&controlledBlocks[i].interpIDRef, 4);
	}
}

void NiSequence::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	for (int i = 0; i < numControlledBlocks; i++) {
		if (controlledBlocks[i].interpolatorRef == blockID)
			controlledBlocks[i].interpolatorRef = 0xFFFFFFFF;
		else if (controlledBlocks[i].interpolatorRef > blockID)
			controlledBlocks[i].interpolatorRef--;

		if (controlledBlocks[i].controllerRef == blockID)
			controlledBlocks[i].controllerRef = 0xFFFFFFFF;
		else if (controlledBlocks[i].controllerRef > blockID)
			controlledBlocks[i].controllerRef--;
	}
}

void NiSequence::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numControlledBlocks; i++) {
		if (controlledBlocks[i].interpolatorRef == blockIndexLo)
			controlledBlocks[i].interpolatorRef = blockIndexHi;
		else if (controlledBlocks[i].interpolatorRef == blockIndexHi)
			controlledBlocks[i].interpolatorRef = blockIndexLo;

		if (controlledBlocks[i].controllerRef == blockIndexLo)
			controlledBlocks[i].controllerRef = blockIndexHi;
		else if (controlledBlocks[i].controllerRef == blockIndexHi)
			controlledBlocks[i].controllerRef = blockIndexLo;
	}
}

void NiSequence::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	if (nameRef != 0xFFFFFFFF && nameRef > stringID)
		nameRef--;

	for (int i = 0; i < numControlledBlocks; i++) {
		if (controlledBlocks[i].nodeNameRef != 0xFFFFFFFF && controlledBlocks[i].nodeNameRef > stringID)
			controlledBlocks[i].nodeNameRef--;

		if (controlledBlocks[i].propTypeRef != 0xFFFFFFFF && controlledBlocks[i].propTypeRef > stringID)
			controlledBlocks[i].propTypeRef--;

		if (controlledBlocks[i].ctrlTypeRef != 0xFFFFFFFF && controlledBlocks[i].ctrlTypeRef > stringID)
			controlledBlocks[i].ctrlTypeRef--;

		if (controlledBlocks[i].ctrlIDRef != 0xFFFFFFFF && controlledBlocks[i].ctrlIDRef > stringID)
			controlledBlocks[i].ctrlIDRef--;

		if (controlledBlocks[i].interpIDRef != 0xFFFFFFFF && controlledBlocks[i].interpIDRef > stringID)
			controlledBlocks[i].interpIDRef--;
	}
}

void NiSequence::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	for (int i = 0; i < numControlledBlocks; i++) {
		refs.insert(controlledBlocks[i].interpolatorRef);
		refs.insert(controlledBlocks[i].controllerRef);
	}
}

int NiSequence::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 12;
	blockSize += numControlledBlocks * 29;

	return blockSize;
}


NiControllerSequence::NiControllerSequence(NiHeader& hdr) : NiSequence(hdr) {
	blockType = NICONTROLLERSEQUENCE;

	textKeyRef = 0xFFFFFFFF;
	managerRef = 0xFFFFFFFF;
	accumRootNameRef = 0xFFFFFFFF;
}

NiControllerSequence::NiControllerSequence(fstream& file, NiHeader& hdr) : NiSequence(file, hdr) {
	blockType = NICONTROLLERSEQUENCE;
	Get(file);
}

NiControllerSequence::~NiControllerSequence() {
	header->RemoveStringRef(accumRootNameRef);
}

void NiControllerSequence::Get(fstream& file) {
	file.read((char*)&weight, 4);
	file.read((char*)&textKeyRef, 4);
	file.read((char*)&cycleType, 4);
	file.read((char*)&frequency, 4);
	file.read((char*)&startTime, 4);
	file.read((char*)&stopTime, 4);
	file.read((char*)&managerRef, 4);

	file.read((char*)&accumRootNameRef, 4);
	if (accumRootNameRef != 0xFFFFFFFF) {
		accumRootName = header->GetStringById(accumRootNameRef);
		header->AddStringRef(accumRootNameRef);
	}

	file.read((char*)&flags, 2);
}

void NiControllerSequence::Put(fstream& file) {
	NiSequence::Put(file);

	file.write((char*)&weight, 4);
	file.write((char*)&textKeyRef, 4);
	file.write((char*)&cycleType, 4);
	file.write((char*)&frequency, 4);
	file.write((char*)&startTime, 4);
	file.write((char*)&stopTime, 4);
	file.write((char*)&managerRef, 4);
	file.write((char*)&accumRootNameRef, 4);
	file.write((char*)&flags, 2);
}

void NiControllerSequence::notifyBlockDelete(int blockID) {
	NiSequence::notifyBlockDelete(blockID);

	if (textKeyRef == blockID)
		textKeyRef = 0xFFFFFFFF;
	else if (textKeyRef > blockID)
		textKeyRef--;

	if (managerRef == blockID)
		managerRef = 0xFFFFFFFF;
	else if (managerRef > blockID)
		managerRef--;
}

void NiControllerSequence::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiSequence::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (textKeyRef == blockIndexLo)
		textKeyRef = blockIndexHi;
	else if (textKeyRef == blockIndexHi)
		textKeyRef = blockIndexLo;

	if (managerRef == blockIndexLo)
		managerRef = blockIndexHi;
	else if (managerRef == blockIndexHi)
		managerRef = blockIndexLo;
}

void NiControllerSequence::notifyStringDelete(int stringID) {
	NiSequence::notifyStringDelete(stringID);

	if (accumRootNameRef != 0xFFFFFFFF && accumRootNameRef > stringID)
		accumRootNameRef--;
}

void NiControllerSequence::GetChildRefs(set<int>& refs) {
	NiSequence::GetChildRefs(refs);

	refs.insert(textKeyRef);
	refs.insert(managerRef);
}

int NiControllerSequence::CalcBlockSize() {
	NiSequence::CalcBlockSize();

	blockSize += 34;

	return blockSize;
}


NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(NiHeader& hdr) {
	NiAVObjectPalette::Init();

	header = &hdr;
	blockType = NIDEFAULTAVOBJECTPALETTE;

	sceneRef = 0xFFFFFFFF;
	numObjects = 0;
}

NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(fstream& file, NiHeader& hdr) {
	NiAVObjectPalette::Init();

	header = &hdr;
	blockType = NIDEFAULTAVOBJECTPALETTE;

	Get(file);
}

void NiDefaultAVObjectPalette::Get(fstream& file) {
	NiAVObjectPalette::Get(file);

	file.read((char*)&sceneRef, 4);

	file.read((char*)&numObjects, 4);
	objects.resize(numObjects);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Get(file, 4);
		file.read((char*)&objects[i].objectRef, 4);
	}
}

void NiDefaultAVObjectPalette::Put(fstream& file) {
	NiAVObjectPalette::Put(file);

	file.write((char*)&sceneRef, 4);

	file.write((char*)&numObjects, 4);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Put(file, 4, false);
		file.write((char*)&objects[i].objectRef, 4);
	}
}

void NiDefaultAVObjectPalette::notifyBlockDelete(int blockID) {
	NiAVObjectPalette::notifyBlockDelete(blockID);

	if (sceneRef == blockID)
		sceneRef = 0xFFFFFFFF;
	else if (sceneRef > blockID)
		sceneRef--;

	for (int i = 0; i < numObjects; i++) {
		if (objects[i].objectRef == blockID)
			objects[i].objectRef = 0xFFFFFFFF;
		else if (objects[i].objectRef > blockID)
			objects[i].objectRef--;
	}
}

void NiDefaultAVObjectPalette::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiAVObjectPalette::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (sceneRef == blockIndexLo)
		sceneRef = blockIndexHi;
	else if (sceneRef == blockIndexHi)
		sceneRef = blockIndexLo;

	for (int i = 0; i < numObjects; i++) {
		if (objects[i].objectRef == blockIndexLo)
			objects[i].objectRef = blockIndexHi;
		else if (objects[i].objectRef == blockIndexHi)
			objects[i].objectRef = blockIndexLo;
	}
}

void NiDefaultAVObjectPalette::GetChildRefs(set<int>& refs) {
	NiAVObjectPalette::GetChildRefs(refs);

	refs.insert(sceneRef);

	for (int i = 0; i < numObjects; i++)
		refs.insert(objects[i].objectRef);
}

int NiDefaultAVObjectPalette::CalcBlockSize() {
	NiAVObjectPalette::CalcBlockSize();

	blockSize += 8;
	blockSize += numObjects * 8;

	for (int i = 0; i < numObjects; i++)
		blockSize += objects[i].name.GetLength();

	return blockSize;
}


bool NiShader::IsSkinTint() {
	return false;
}

bool NiShader::IsSkinned() {
	return false;
}

void NiShader::SetSkinned(const bool& enable) {
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
	return 0xFFFFFFFF;
}

void NiShader::SetTextureSetRef(const int& texSetRef) {
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


BSLightingShaderProperty::~BSLightingShaderProperty() {
	header->RemoveStringRef(wetMaterialNameRef);
}

BSLightingShaderProperty::BSLightingShaderProperty(NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;

	if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120) {
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
	textureSetRef = 0xFFFFFFFF;

	emissiveMultiple = 1.0f;
	wetMaterialNameRef = 0xFFFFFFFF;
	textureClampMode = 3;
	alpha = 1.0f;
	refractionStrength = 0.0f;

	if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120)
		glossiness = 1.0f;
	else
		glossiness = 20.0f;

	specularColor = Vector3(1.0f, 1.0f, 1.0f);
	specularStrength = 1.0f;
	lightingEffect1 = 0.3f;
	lightingEffect2 = 2.0f;

	subsurfaceRolloff = 0.0f;
	unkFloat1 = numeric_limits<float>::max();
	backlightPower = 0.0f;
	grayscaleToPaletteScale = 1.0f;
	fresnelPower = 5.0f;
	wetnessSpecScale = 0.6f;
	wetnessSpecPower = 1.4f;
	wetnessMinVar = 0.2f;
	wetnessEnvmapScale = 1.0f;
	wetnessFresnelPower = 1.6f;
	wetnessMetalness = 0.0f;

	environmentMapScale = 1.0f;
	unkEnvmap = 0;
	unkSkinTint = 0;
	maxPasses = 1.0f;
	scale = 1.0f;
	parallaxInnerLayerThickness = 0.0f;
	parallaxRefractionScale = 1.0f;
	parallaxInnerLayerTextureScale.u = 1.0f;
	parallaxInnerLayerTextureScale.v = 1.0f;
	parallaxEnvmapStrength = 1.0f;
	eyeCubemapScale = 1.0f;
}

BSLightingShaderProperty::BSLightingShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();
	NiObjectNET::bBSLightingShaderProperty = true;

	header = &hdr;
	blockType = BSLIGHTINGSHADERPROPERTY;

	if (hdr.GetUserVersion() == 12 && hdr.GetUserVersion2() >= 120) {
		shaderFlags1 = 0x80400203;
		shaderFlags2 = 0x00000081;
	}
	else {
		shaderFlags1 = 0x82400303;
		shaderFlags2 = 0x00008001;
	}

	wetMaterialNameRef = 0xFFFFFFFF;
	lightingEffect1 = 0.3f;
	lightingEffect2 = 2.0f;

	subsurfaceRolloff = 0.0f;
	unkFloat1 = numeric_limits<float>::max();
	backlightPower = 0.0f;
	grayscaleToPaletteScale = 1.0f;
	fresnelPower = 5.0f;
	wetnessSpecScale = 0.6f;
	wetnessSpecPower = 1.4f;
	wetnessMinVar = 0.2f;
	wetnessEnvmapScale = 1.0f;
	wetnessFresnelPower = 1.6f;
	wetnessMetalness = 0.0f;

	environmentMapScale = 1.0f;
	unkEnvmap = 0;
	unkSkinTint = 0;
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

	if (header->GetUserVersion() == 12) {
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

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		file.read((char*)&wetMaterialNameRef, 4);
		if (wetMaterialNameRef != -1) {
			wetMaterialName = header->GetStringById(wetMaterialNameRef);
			header->AddStringRef(wetMaterialNameRef);
		}
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

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
		file.read((char*)&lightingEffect1, 4);
		file.read((char*)&lightingEffect2, 4);
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		file.read((char*)&subsurfaceRolloff, 4);
		file.read((char*)&unkFloat1, 4);
		file.read((char*)&backlightPower, 4);
		file.read((char*)&grayscaleToPaletteScale, 4);
		file.read((char*)&fresnelPower, 4);
		file.read((char*)&wetnessSpecScale, 4);
		file.read((char*)&wetnessSpecPower, 4);
		file.read((char*)&wetnessMinVar, 4);
		file.read((char*)&wetnessEnvmapScale, 4);
		file.read((char*)&wetnessFresnelPower, 4);
		file.read((char*)&wetnessMetalness, 4);
	}

	switch (skyrimShaderType) {
	case 1:
		file.read((char*)&environmentMapScale, 4);
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.read((char*)&unkEnvmap, 2);
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.read((char*)&unkSkinTint, 4);

		file.read((char*)&skinTintColor.x, 4);
		file.read((char*)&skinTintColor.y, 4);
		file.read((char*)&skinTintColor.z, 4);
		break;
	case 6:
		file.read((char*)&hairTintColor.x, 4);
		file.read((char*)&hairTintColor.y, 4);
		file.read((char*)&hairTintColor.z, 4);
		break;
	case 7:
		file.read((char*)&maxPasses, 4);
		file.read((char*)&scale, 4);
		break;
	case 11:
		file.read((char*)&parallaxInnerLayerThickness, 4);
		file.read((char*)&parallaxRefractionScale, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.read((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.read((char*)&parallaxEnvmapStrength, 4);
		break;
	case 14:
		file.read((char*)&sparkleParameters.r, 4);
		file.read((char*)&sparkleParameters.g, 4);
		file.read((char*)&sparkleParameters.b, 4);
		file.read((char*)&sparkleParameters.a, 4);
		break;
	case 16:
		file.read((char*)&eyeCubemapScale, 4);
		file.read((char*)&eyeLeftReflectionCenter.x, 4);
		file.read((char*)&eyeLeftReflectionCenter.y, 4);
		file.read((char*)&eyeLeftReflectionCenter.z, 4);
		file.read((char*)&eyeRightReflectionCenter.x, 4);
		file.read((char*)&eyeRightReflectionCenter.y, 4);
		file.read((char*)&eyeRightReflectionCenter.z, 4);
		break;
	}
}

void BSLightingShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	if (header->GetUserVersion() == 12) {
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

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		file.write((char*)&wetMaterialNameRef, 4);

	file.write((char*)&textureClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&refractionStrength, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specularColor.x, 4);
	file.write((char*)&specularColor.y, 4);
	file.write((char*)&specularColor.z, 4);
	file.write((char*)&specularStrength, 4);

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
		file.write((char*)&lightingEffect1, 4);
		file.write((char*)&lightingEffect2, 4);
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		file.write((char*)&subsurfaceRolloff, 4);
		file.write((char*)&unkFloat1, 4);
		file.write((char*)&backlightPower, 4);
		file.write((char*)&grayscaleToPaletteScale, 4);
		file.write((char*)&fresnelPower, 4);
		file.write((char*)&wetnessSpecScale, 4);
		file.write((char*)&wetnessSpecPower, 4);
		file.write((char*)&wetnessMinVar, 4);
		file.write((char*)&wetnessEnvmapScale, 4);
		file.write((char*)&wetnessFresnelPower, 4);
		file.write((char*)&wetnessMetalness, 4);
	}

	switch (skyrimShaderType) {
	case 1:
		file.write((char*)&environmentMapScale, 4);
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.write((char*)&unkEnvmap, 2);
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			file.write((char*)&unkSkinTint, 4);

		file.write((char*)&skinTintColor.x, 4);
		file.write((char*)&skinTintColor.y, 4);
		file.write((char*)&skinTintColor.z, 4);
		break;
	case 6:
		file.write((char*)&hairTintColor.x, 4);
		file.write((char*)&hairTintColor.y, 4);
		file.write((char*)&hairTintColor.z, 4);
		break;
	case 7:
		file.write((char*)&maxPasses, 4);
		file.write((char*)&scale, 4);
		break;
	case 11:
		file.write((char*)&parallaxInnerLayerThickness, 4);
		file.write((char*)&parallaxRefractionScale, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.u, 4);
		file.write((char*)&parallaxInnerLayerTextureScale.v, 4);
		file.write((char*)&parallaxEnvmapStrength, 4);
		break;
	case 14:
		file.write((char*)&sparkleParameters.r, 4);
		file.write((char*)&sparkleParameters.g, 4);
		file.write((char*)&sparkleParameters.b, 4);
		file.write((char*)&sparkleParameters.a, 4);
		break;
	case 16:
		file.write((char*)&eyeCubemapScale, 4);
		file.write((char*)&eyeLeftReflectionCenter.x, 4);
		file.write((char*)&eyeLeftReflectionCenter.y, 4);
		file.write((char*)&eyeLeftReflectionCenter.z, 4);
		file.write((char*)&eyeRightReflectionCenter.x, 4);
		file.write((char*)&eyeRightReflectionCenter.y, 4);
		file.write((char*)&eyeRightReflectionCenter.z, 4);
		break;
	}
}

void BSLightingShaderProperty::notifyBlockDelete(int blockID) {
	NiProperty::notifyBlockDelete(blockID);

	if (textureSetRef == blockID)
		textureSetRef = 0xFFFFFFFF;
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

void BSLightingShaderProperty::notifyStringDelete(int stringID) {
	NiProperty::notifyStringDelete(stringID);

	if (wetMaterialNameRef != 0xFFFFFFFF && wetMaterialNameRef > stringID)
		wetMaterialNameRef--;
}

void BSLightingShaderProperty::GetChildRefs(set<int>& refs) {
	NiProperty::GetChildRefs(refs);

	refs.insert(textureSetRef);
}

bool BSLightingShaderProperty::IsSkinTint() {
	return skyrimShaderType == 5;
}

bool BSLightingShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSLightingShaderProperty::SetSkinned(const bool& enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
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

void BSLightingShaderProperty::SetTextureSetRef(const int& texSetRef) {
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

	if (header->GetUserVersion() == 12)
		blockSize += 8;

	blockSize += 36;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		blockSize += 4;

	blockSize += 32;

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130)
		blockSize += 8;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		blockSize += 44;

	switch (skyrimShaderType) {
	case 1:
		blockSize += 4;
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			blockSize += 2;
		break;
	case 5:
		if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
			blockSize += 4;

		blockSize += 12;
		break;
	case 6:
		blockSize += 12;
		break;
	case 7:
		blockSize += 8;
		break;
	case 11:
		blockSize += 20;
		break;
	case 14:
		blockSize += 16;
		break;
	case 16:
		blockSize += 28;
		break;
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

	if (header->GetUserVersion() == 11)
		file.read((char*)&environmentMapScale, 4);
}

void BSShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&smooth, 2);
	file.write((char*)&shaderType, 4);
	file.write((char*)&shaderFlags, 4);
	file.write((char*)&shaderFlags2, 4);

	if (header->GetUserVersion() == 11)
		file.write((char*)&environmentMapScale, 4);
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
	if (header->GetUserVersion() == 11)
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

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
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

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		envMapTexture.Put(file, 4, false);
		normalTexture.Put(file, 4, false);
		envMaskTexture.Put(file, 4, false);
		file.write((char*)&envMapScale, 4);
	}
}

bool BSEffectShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSEffectShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSEffectShaderProperty::SetSkinned(const bool& enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
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
	blockSize += sourceTexture.GetLength();
	blockSize += greyscaleTexture.GetLength();

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130) {
		blockSize += 16;
		blockSize += envMapTexture.GetLength();
		blockSize += normalTexture.GetLength();
		blockSize += envMaskTexture.GetLength();
	}

	return blockSize;
}


BSWaterShaderProperty::BSWaterShaderProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSWATERSHADERPROPERTY;

	shaderFlags1 = 0;
	shaderFlags2 = 0;
	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;
	waterFlags = 0;
}

BSWaterShaderProperty::BSWaterShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSWATERSHADERPROPERTY;

	Get(file);
}

void BSWaterShaderProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&shaderFlags1, 4);
	file.read((char*)&shaderFlags2, 4);
	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	file.read((char*)&waterFlags, 4);
}

void BSWaterShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	file.write((char*)&waterFlags, 4);
}

bool BSWaterShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSWaterShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSWaterShaderProperty::SetSkinned(const bool& enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSWaterShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

int BSWaterShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


BSSkyShaderProperty::BSSkyShaderProperty(NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSSKYSHADERPROPERTY;

	shaderFlags1 = 0;
	shaderFlags2 = 0;
	uvOffset.u = 0.0f;
	uvOffset.v = 0.0f;
	uvScale.u = 1.0f;
	uvScale.v = 1.0f;
	skyFlags = 0;
}

BSSkyShaderProperty::BSSkyShaderProperty(fstream& file, NiHeader& hdr) {
	NiProperty::Init();

	header = &hdr;
	blockType = BSSKYSHADERPROPERTY;

	Get(file);
}

void BSSkyShaderProperty::Get(fstream& file) {
	NiProperty::Get(file);

	file.read((char*)&shaderFlags1, 4);
	file.read((char*)&shaderFlags2, 4);
	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	baseTexture.Get(file, 4);
	file.read((char*)&skyFlags, 4);
}

void BSSkyShaderProperty::Put(fstream& file) {
	NiProperty::Put(file);

	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	baseTexture.Put(file, 4, false);
	file.write((char*)&skyFlags, 4);
}

bool BSSkyShaderProperty::IsSkinTint() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool BSSkyShaderProperty::IsSkinned() {
	return (shaderFlags1 & (1 << 1)) != 0;
}

void BSSkyShaderProperty::SetSkinned(const bool& enable) {
	if (enable)
		shaderFlags1 |= 1 << 1;
	else
		shaderFlags1 &= ~(1 << 1);
}

bool BSSkyShaderProperty::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

int BSSkyShaderProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 32;
	blockSize += baseTexture.GetLength();

	return blockSize;
}


void BSShaderLightingProperty::Init() {
	BSShaderProperty::Init();

	textureClampMode = 3;
}

void BSShaderLightingProperty::Get(fstream& file) {
	BSShaderProperty::Get(file);

	if (header->GetUserVersion() <= 11)
		file.read((char*)&textureClampMode, 4);
}

void BSShaderLightingProperty::Put(fstream& file) {
	BSShaderProperty::Put(file);

	if (header->GetUserVersion() <= 11)
		file.write((char*)&textureClampMode, 4);
}

int BSShaderLightingProperty::CalcBlockSize() {
	BSShaderProperty::CalcBlockSize();

	if (header->GetUserVersion() <= 11)
		blockSize += 4;

	return blockSize;
}


BSShaderPPLightingProperty::BSShaderPPLightingProperty(NiHeader& hdr) {
	BSShaderLightingProperty::Init();

	header = &hdr;
	blockType = BSSHADERPPLIGHTINGPROPERTY;
	textureSetRef = 0xFFFFFFFF;
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

	if (header->GetUserVersion() == 11) {
		file.read((char*)&refractionStrength, 4);
		file.read((char*)&refractionFirePeriod, 4);
		file.read((char*)&unkFloat4, 4);
		file.read((char*)&unkFloat5, 4);
	}

	if (header->GetUserVersion() >= 12) {
		file.read((char*)&emissiveColor.r, 4);
		file.read((char*)&emissiveColor.g, 4);
		file.read((char*)&emissiveColor.b, 4);
		file.read((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::Put(fstream& file) {
	BSShaderLightingProperty::Put(file);

	file.write((char*)&textureSetRef, 4);

	if (header->GetUserVersion() == 11) {
		file.write((char*)&refractionStrength, 4);
		file.write((char*)&refractionFirePeriod, 4);
		file.write((char*)&unkFloat4, 4);
		file.write((char*)&unkFloat5, 4);
	}

	if (header->GetUserVersion() >= 12) {
		file.write((char*)&emissiveColor.r, 4);
		file.write((char*)&emissiveColor.g, 4);
		file.write((char*)&emissiveColor.b, 4);
		file.write((char*)&emissiveColor.a, 4);
	}
}

void BSShaderPPLightingProperty::notifyBlockDelete(int blockID) {
	BSShaderLightingProperty::notifyBlockDelete(blockID);

	if (textureSetRef == blockID)
		textureSetRef = 0xFFFFFFFF;
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

void BSShaderPPLightingProperty::GetChildRefs(set<int>& refs) {
	BSShaderLightingProperty::GetChildRefs(refs);

	refs.insert(textureSetRef);
}

bool BSShaderPPLightingProperty::IsSkinTint() {
	return shaderType == 0x0000000e;
}

bool BSShaderPPLightingProperty::IsSkinned() {
	return (shaderFlags & (1 << 1)) != 0;
}

void BSShaderPPLightingProperty::SetSkinned(const bool& enable) {
	if (enable)
		shaderFlags |= 1 << 1;
	else
		shaderFlags &= ~(1 << 1);
}

int BSShaderPPLightingProperty::GetTextureSetRef() {
	return textureSetRef;
}

void BSShaderPPLightingProperty::SetTextureSetRef(const int& texSetRef) {
	textureSetRef = texSetRef;
}

int BSShaderPPLightingProperty::CalcBlockSize() {
	BSShaderLightingProperty::CalcBlockSize();

	blockSize += 4;

	if (header->GetUserVersion() == 11)
		blockSize += 16;

	if (header->GetUserVersion() >= 12)
		blockSize += 16;

	return blockSize;
}


BSShaderTextureSet::BSShaderTextureSet(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSSHADERTEXTURESET;

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() >= 130)
		numTextures = 10;
	else if (header->GetUserVersion() == 12)
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
		blockSize += tex.GetLength();
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

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)) {
		file.read((char*)&colorAmbient, 12);
		file.read((char*)&colorDiffuse, 12);
	}

	file.read((char*)&colorSpecular, 12);
	file.read((char*)&colorEmissive, 12);
	file.read((char*)&glossiness, 4);
	file.read((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)
		file.read((char*)&emitMulti, 4);
}

void NiMaterialProperty::Put(fstream& file) {
	NiProperty::Put(file);

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)) {
		file.write((char*)&colorAmbient, 12);
		file.write((char*)&colorDiffuse, 12);
	}

	file.write((char*)&colorSpecular, 12);
	file.write((char*)&colorEmissive, 12);
	file.write((char*)&glossiness, 4);
	file.write((char*)&alpha, 4);

	if (header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21)
		file.write((char*)&emitMulti, 4);
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

	if (!(header->VerCheck(20, 2, 0, 7, true) && header->GetUserVersion() >= 11 && header->GetUserVersion2() > 21))
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

int NiStencilProperty::CalcBlockSize() {
	NiProperty::CalcBlockSize();

	blockSize += 10;

	return blockSize;
}


NiExtraData::~NiExtraData() {
	header->RemoveStringRef(nameRef);
}

void NiExtraData::Init() {
	NiObject::Init();

	nameRef = 0xFFFFFFFF;
	name.clear();
}

void NiExtraData::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nameRef, 4);
	if (nameRef != 0xFFFFFFFF) {
		name = header->GetStringById(nameRef);
		header->AddStringRef(nameRef);
	}
	else
		name.clear();
}

void NiExtraData::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nameRef, 4);
}

void NiExtraData::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	if (nameRef != 0xFFFFFFFF && nameRef > stringID)
		nameRef--;
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


NiBinaryExtraData::NiBinaryExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIBINARYEXTRADATA;

	size = 0;
}

NiBinaryExtraData::NiBinaryExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIBINARYEXTRADATA;

	Get(file);
}

void NiBinaryExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&size, 4);
	data.resize(size);
	for (int i = 0; i < size; i++)
		file.read((char*)&data[i], 1);
}

void NiBinaryExtraData::Put(fstream& file) {
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


NiFloatExtraData::NiFloatExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIFLOATEXTRADATA;

	floatData = 0.0f;
}

NiFloatExtraData::NiFloatExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIFLOATEXTRADATA;

	Get(file);
}

void NiFloatExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&floatData, 4);
}

void NiFloatExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&floatData, 4);
}

int NiFloatExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


NiStringsExtraData::NiStringsExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NISTRINGSEXTRADATA;

	numStrings = 0;
}

NiStringsExtraData::NiStringsExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NISTRINGSEXTRADATA;

	Get(file);
}

void NiStringsExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numStrings, 4);
	stringsData.resize(numStrings);
	for (int i = 0; i < numStrings; i++)
		stringsData[i].Get(file, 4);
}

void NiStringsExtraData::Put(fstream& file) {
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


NiStringExtraData::~NiStringExtraData() {
	header->RemoveStringRef(stringDataRef);
}

NiStringExtraData::NiStringExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NISTRINGEXTRADATA;

	stringDataRef = 0xFFFFFFFF;
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
	if (stringDataRef != 0xFFFFFFFF) {
		stringData = header->GetStringById(stringDataRef);
		header->AddStringRef(stringDataRef);
	}
}

void NiStringExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&stringDataRef, 4);
}

void NiStringExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	if (stringDataRef != 0xFFFFFFFF && stringDataRef > stringID)
		stringDataRef--;
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


NiBooleanExtraData::NiBooleanExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIBOOLEANEXTRADATA;

	booleanData = false;
}

NiBooleanExtraData::NiBooleanExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NIBOOLEANEXTRADATA;

	Get(file);
}

void NiBooleanExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&booleanData, 1);
}

void NiBooleanExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&booleanData, 1);
}

bool NiBooleanExtraData::GetBooleanData() {
	return booleanData;
}

void NiBooleanExtraData::SetBooleanData(const bool& booleanData) {
	this->booleanData = booleanData;
}

int NiBooleanExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 1;

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
}


BSInvMarker::BSInvMarker(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSINVMARKER;
}

BSInvMarker::BSInvMarker(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSINVMARKER;

	Get(file);
}

void BSInvMarker::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&rotationX, 2);
	file.read((char*)&rotationY, 2);
	file.read((char*)&rotationZ, 2);
	file.read((char*)&zoom, 4);
}

void BSInvMarker::Put(fstream& file) {
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


BSFurnitureMarker::BSFurnitureMarker(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	numPositions = 0;
}

BSFurnitureMarker::BSFurnitureMarker(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;

	Get(file);
}

void BSFurnitureMarker::Get(fstream& file) {
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

void BSFurnitureMarker::Put(fstream& file) {
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


BSFurnitureMarkerNode::BSFurnitureMarkerNode(NiHeader& hdr) : BSFurnitureMarker(hdr) {
	blockType = BSFURNITUREMARKERNODE;
}

BSFurnitureMarkerNode::BSFurnitureMarkerNode(fstream& file, NiHeader& hdr) : BSFurnitureMarker(file, hdr) {
	blockType = BSFURNITUREMARKERNODE;
}


BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSDECALPLACEMENTVECTOREXTRADATA;
	unkFloat1 = 0.0f;
	numVectorBlocks = 0;
}

BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSDECALPLACEMENTVECTOREXTRADATA;

	Get(file);
}

void BSDecalPlacementVectorExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&unkFloat1, 4);
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

void BSDecalPlacementVectorExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&unkFloat1, 4);
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
	NiExtraData::CalcBlockSize();

	blockSize += 6;
	for (int i = 0; i < numVectorBlocks; i++)
		blockSize += 2 + decalVectorBlocks[i].numVectors * 24;

	return blockSize;
}


BSBehaviorGraphExtraData::~BSBehaviorGraphExtraData() {
	header->RemoveStringRef(behaviorGraphFileRef);
}

BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBEHAVIORGRAPHEXTRADATA;

	behaviorGraphFileRef = 0xFFFFFFFF;
}

BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBEHAVIORGRAPHEXTRADATA;

	Get(file);
}

void BSBehaviorGraphExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&behaviorGraphFileRef, 4);
	if (behaviorGraphFileRef != 0xFFFFFFFF) {
		behaviorGraphFile = header->GetStringById(behaviorGraphFileRef);
		header->AddStringRef(behaviorGraphFileRef);
	}

	file.read((char*)&controlsBaseSkel, 1);
}

void BSBehaviorGraphExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&behaviorGraphFileRef, 4);
	file.write((char*)&controlsBaseSkel, 1);
}

void BSBehaviorGraphExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	if (behaviorGraphFileRef != 0xFFFFFFFF && behaviorGraphFileRef > stringID)
		behaviorGraphFileRef--;
}

int BSBehaviorGraphExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSBound::BSBound(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBOUND;
}

BSBound::BSBound(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBOUND;

	Get(file);
}

void BSBound::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&halfExtents, 12);
}

void BSBound::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&halfExtents, 12);
}

int BSBound::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


BSBoneLODExtraData::BSBoneLODExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBONELODEXTRADATA;

	numBoneLODs = 0;
}

BSBoneLODExtraData::BSBoneLODExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = BSBONELODEXTRADATA;

	Get(file);
}

BSBoneLODExtraData::~BSBoneLODExtraData() {
	for (int i = 0; i < numBoneLODs; i++)
		header->RemoveStringRef(boneLODs[i].boneNameRef);
}

void BSBoneLODExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numBoneLODs, 4);
	boneLODs.resize(numBoneLODs);
	for (int i = 0; i < numBoneLODs; i++) {
		file.read((char*)&boneLODs[i].distance, 4);
		file.read((char*)&boneLODs[i].boneNameRef, 4);
		if (boneLODs[i].boneNameRef != 0xFFFFFFFF) {
			boneLODs[i].boneName = header->GetStringById(boneLODs[i].boneNameRef);
			header->AddStringRef(boneLODs[i].boneNameRef);
		}
	}
}

void BSBoneLODExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numBoneLODs, 4);
	for (int i = 0; i < numBoneLODs; i++) {
		file.write((char*)&boneLODs[i].distance, 4);
		file.write((char*)&boneLODs[i].boneNameRef, 4);
	}
}

void BSBoneLODExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	for (int i = 0; i < numBoneLODs; i++) {
		if (boneLODs[i].boneNameRef != 0xFFFFFFFF && boneLODs[i].boneNameRef > stringID)
			boneLODs[i].boneNameRef--;
	}
}

int BSBoneLODExtraData::CalcBlockSize() {
	NiExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBoneLODs * 8;

	return blockSize;
}


NiTextKeyExtraData::NiTextKeyExtraData(NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NITEXTKEYEXTRADATA;

	numTextKeys = 0;
}

NiTextKeyExtraData::NiTextKeyExtraData(fstream& file, NiHeader& hdr) {
	NiExtraData::Init();

	header = &hdr;
	blockType = NITEXTKEYEXTRADATA;

	Get(file);
}

NiTextKeyExtraData::~NiTextKeyExtraData() {
	for (int i = 0; i < numTextKeys; i++)
		header->RemoveStringRef(textKeys[i].value);
}

void NiTextKeyExtraData::Get(fstream& file) {
	NiExtraData::Get(file);

	file.read((char*)&numTextKeys, 4);
	textKeys.resize(numTextKeys);
	for (int i = 0; i < numTextKeys; i++) {
		file.read((char*)&textKeys[i].time, 4);
		file.read((char*)&textKeys[i].value, 4);
		header->AddStringRef(textKeys[i].value);
	}
}

void NiTextKeyExtraData::Put(fstream& file) {
	NiExtraData::Put(file);

	file.write((char*)&numTextKeys, 4);
	for (int i = 0; i < numTextKeys; i++) {
		file.write((char*)&textKeys[i].time, 4);
		file.write((char*)&textKeys[i].value, 4);
	}
}

void NiTextKeyExtraData::notifyStringDelete(int stringID) {
	NiExtraData::notifyStringDelete(stringID);

	for (int i = 0; i < numTextKeys; i++) {
		if (textKeys[i].value != 0xFFFFFFFF && textKeys[i].value > stringID)
			textKeys[i].value--;
	}
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
	blockSize += root.GetLength();
	blockSize += variableName.GetLength();
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
		blockSize += targets[i].GetLength();

	return blockSize;
}


BSClothExtraData::BSClothExtraData() {
	BSExtraData::Init();

	blockType = BSCLOTHEXTRADATA;
	numBytes = 0;
	data.clear();
}

BSClothExtraData::BSClothExtraData(NiHeader& hdr, const uint& size) {
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


BSMultiBound::BSMultiBound(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUND;

	dataRef = 0xFFFFFFFF;
}

BSMultiBound::BSMultiBound(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUND;

	Get(file);
}

void BSMultiBound::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&dataRef, 4);
}

void BSMultiBound::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&dataRef, 4);
}

void BSMultiBound::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;
}

void BSMultiBound::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

void BSMultiBound::GetChildRefs(set<int>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(dataRef);
}

int BSMultiBound::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSMultiBoundOBB::BSMultiBoundOBB(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDOBB;
}

BSMultiBoundOBB::BSMultiBoundOBB(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDOBB;

	Get(file);
}

void BSMultiBoundOBB::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&size, 12);
	file.read((char*)rotation, 36);
}

void BSMultiBoundOBB::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&size, 12);
	file.write((char*)rotation, 36);
}

int BSMultiBoundOBB::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 60;

	return blockSize;
}


BSMultiBoundAABB::BSMultiBoundAABB(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDAABB;
}

BSMultiBoundAABB::BSMultiBoundAABB(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = BSMULTIBOUNDAABB;

	Get(file);
}

void BSMultiBoundAABB::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&halfExtent, 12);
}

void BSMultiBoundAABB::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&halfExtent, 12);
}

int BSMultiBoundAABB::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


NiCollisionObject::NiCollisionObject(NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NICOLLISIONOBJECT;

	targetRef = 0xFFFFFFFF;
}

NiCollisionObject::NiCollisionObject(fstream& file, NiHeader& hdr) {
	NiObject::Init();

	header = &hdr;
	blockType = NICOLLISIONOBJECT;

	Get(file);
}

void NiCollisionObject::Get(fstream& file) {
	NiObject::Get(file);

	file.read((char*)&targetRef, 4);
}

void NiCollisionObject::Put(fstream& file) {
	NiObject::Put(file);

	file.write((char*)&targetRef, 4);
}

void NiCollisionObject::notifyBlockDelete(int blockID) {
	NiObject::notifyBlockDelete(blockID);

	if (targetRef == blockID)
		targetRef = 0xFFFFFFFF;
	else if (targetRef > blockID)
		targetRef--;
}

void NiCollisionObject::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (targetRef == blockIndexLo)
		targetRef = blockIndexHi;
	else if (targetRef == blockIndexHi)
		targetRef = blockIndexLo;
}

int NiCollisionObject::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


bhkNiCollisionObject::bhkNiCollisionObject(NiHeader& hdr) : NiCollisionObject(hdr) {
	flags = 1;
	bodyRef = 0xFFFFFFFF;
}

bhkNiCollisionObject::bhkNiCollisionObject(fstream& file, NiHeader& hdr) : NiCollisionObject(file, hdr) {
	Get(file);
}

void bhkNiCollisionObject::Get(fstream& file) {
	file.read((char*)&flags, 2);
	file.read((char*)&bodyRef, 4);
}

void bhkNiCollisionObject::Put(fstream& file) {
	NiCollisionObject::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&bodyRef, 4);
}

void bhkNiCollisionObject::notifyBlockDelete(int blockID) {
	NiCollisionObject::notifyBlockDelete(blockID);

	if (bodyRef == blockID)
		bodyRef = 0xFFFFFFFF;
	else if (bodyRef > blockID)
		bodyRef--;
}

void bhkNiCollisionObject::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	NiCollisionObject::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (bodyRef == blockIndexLo)
		bodyRef = blockIndexHi;
	else if (bodyRef == blockIndexHi)
		bodyRef = blockIndexLo;
}

void bhkNiCollisionObject::GetChildRefs(set<int>& refs) {
	NiCollisionObject::GetChildRefs(refs);

	refs.insert(bodyRef);
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


bhkPCollisionObject::bhkPCollisionObject(NiHeader& hdr) : bhkNiCollisionObject(hdr) {
	blockType = BHKPCOLLISIONOBJECT;
}

bhkPCollisionObject::bhkPCollisionObject(fstream& file, NiHeader& hdr) : bhkNiCollisionObject(file, hdr) {
	blockType = BHKPCOLLISIONOBJECT;
}


bhkSPCollisionObject::bhkSPCollisionObject(NiHeader& hdr) : bhkPCollisionObject(hdr) {
	blockType = BHKSPCOLLISIONOBJECT;
}

bhkSPCollisionObject::bhkSPCollisionObject(fstream& file, NiHeader& hdr) : bhkPCollisionObject(file, hdr) {
	blockType = BHKSPCOLLISIONOBJECT;
}


bhkBlendCollisionObject::bhkBlendCollisionObject(NiHeader& hdr) : bhkCollisionObject(hdr) {
	blockType = BHKBLENDCOLLISIONOBJECT;
}

bhkBlendCollisionObject::bhkBlendCollisionObject(fstream& file, NiHeader& hdr) : bhkCollisionObject(file, hdr) {
	blockType = BHKBLENDCOLLISIONOBJECT;

	Get(file);
}

void bhkBlendCollisionObject::Get(fstream& file) {
	file.read((char*)&heirGain, 4);
	file.read((char*)&velGain, 4);
}

void bhkBlendCollisionObject::Put(fstream& file) {
	bhkCollisionObject::Put(file);

	file.write((char*)&heirGain, 4);
	file.write((char*)&velGain, 4);
}

int bhkBlendCollisionObject::CalcBlockSize() {
	bhkCollisionObject::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


bhkPhysicsSystem::bhkPhysicsSystem() {
	BSExtraData::Init();

	blockType = BHKPHYSICSSYSTEM;
	numBytes = 0;
	data.clear();
}

bhkPhysicsSystem::bhkPhysicsSystem(NiHeader& hdr, const uint& size) {
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

int bhkPhysicsSystem::CalcBlockSize() {
	BSExtraData::CalcBlockSize();

	blockSize += 4;
	blockSize += numBytes;

	return blockSize;
}


bhkPlaneShape::bhkPlaneShape(NiHeader& hdr) {
	bhkHeightFieldShape::Init();

	header = &hdr;
	blockType = BHKPLANESHAPE;
}

bhkPlaneShape::bhkPlaneShape(fstream& file, NiHeader& hdr) {
	bhkHeightFieldShape::Init();

	header = &hdr;
	blockType = BHKPLANESHAPE;

	Get(file);
}

void bhkPlaneShape::Get(fstream& file) {
	bhkHeightFieldShape::Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&unkVec, 12);
	file.read((char*)&direction, 12);
	file.read((char*)&constant, 4);
	file.read((char*)&halfExtents, 16);
	file.read((char*)&center, 16);
}

void bhkPlaneShape::Put(fstream& file) {
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


void bhkSphereRepShape::Get(fstream& file) {
	bhkShape::Get(file);

	file.read((char*)&material, 4);
	file.read((char*)&radius, 4);
}

void bhkSphereRepShape::Put(fstream& file) {
	bhkShape::Put(file);

	file.write((char*)&material, 4);
	file.write((char*)&radius, 4);
}

int bhkSphereRepShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 8;

	return blockSize;
}


bhkConvexVerticesShape::bhkConvexVerticesShape(NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKCONVEXVERTICESSHAPE;

	numVerts = 0;
	numNormals = 0;
}

bhkConvexVerticesShape::bhkConvexVerticesShape(fstream& file, NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKCONVEXVERTICESSHAPE;

	Get(file);
}

void bhkConvexVerticesShape::Get(fstream& file) {
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

void bhkConvexVerticesShape::Put(fstream& file) {
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


bhkBoxShape::bhkBoxShape(NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKBOXSHAPE;
}

bhkBoxShape::bhkBoxShape(fstream& file, NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKBOXSHAPE;

	Get(file);
}

void bhkBoxShape::Get(fstream& file) {
	bhkConvexShape::Get(file);

	file.read((char*)&unkInt1, 4);
	file.read((char*)&unkInt2, 4);
	file.read((char*)&dimensions, 12);
	file.read((char*)&radius2, 4);
}

void bhkBoxShape::Put(fstream& file) {
	bhkConvexShape::Put(file);

	file.write((char*)&unkInt1, 4);
	file.write((char*)&unkInt2, 4);
	file.write((char*)&dimensions, 12);
	file.write((char*)&radius2, 4);
}

int bhkBoxShape::CalcBlockSize() {
	bhkConvexShape::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


bhkSphereShape::bhkSphereShape(NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKSPHERESHAPE;
}

bhkSphereShape::bhkSphereShape(fstream& file, NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKSPHERESHAPE;

	Get(file);
}


bhkTransformShape::bhkTransformShape(NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKTRANSFORMSHAPE;

	shapeRef = 0xFFFFFFFF;
}

bhkTransformShape::bhkTransformShape(fstream& file, NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKTRANSFORMSHAPE;

	Get(file);
}

void bhkTransformShape::Get(fstream& file) {
	bhkShape::Get(file);

	file.read((char*)&shapeRef, 4);
	file.read((char*)&material, 4);
	file.read((char*)&unkFloat, 4);
	file.read((char*)unkBytes, 8);
	file.read((char*)&xform, 64);
}

void bhkTransformShape::Put(fstream& file) {
	bhkShape::Put(file);

	file.write((char*)&shapeRef, 4);
	file.write((char*)&material, 4);
	file.write((char*)&unkFloat, 4);
	file.write((char*)unkBytes, 8);
	file.write((char*)&xform, 64);
}

void bhkTransformShape::notifyBlockDelete(int blockID) {
	bhkShape::notifyBlockDelete(blockID);

	if (shapeRef == blockID)
		shapeRef = 0xFFFFFFFF;
	else if (shapeRef > blockID)
		shapeRef--;
}

void bhkTransformShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkShape::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (shapeRef == blockIndexLo)
		shapeRef = blockIndexHi;
	else if (shapeRef == blockIndexHi)
		shapeRef = blockIndexLo;
}

void bhkTransformShape::GetChildRefs(set<int>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(shapeRef);
}

int bhkTransformShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 84;

	return blockSize;
}


bhkConvexTransformShape::bhkConvexTransformShape(NiHeader& hdr) : bhkTransformShape(hdr) {
	blockType = BHKCONVEXTRANSFORMSHAPE;
}

bhkConvexTransformShape::bhkConvexTransformShape(fstream& file, NiHeader& hdr) : bhkTransformShape(file, hdr) {
	blockType = BHKCONVEXTRANSFORMSHAPE;
}


bhkCapsuleShape::bhkCapsuleShape(NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKCAPSULESHAPE;
}

bhkCapsuleShape::bhkCapsuleShape(fstream& file, NiHeader& hdr) {
	bhkConvexShape::Init();

	header = &hdr;
	blockType = BHKCAPSULESHAPE;

	Get(file);
}

void bhkCapsuleShape::Get(fstream& file) {
	bhkConvexShape::Get(file);

	file.read((char*)&padding1, 4);
	file.read((char*)&padding2, 4);
	file.read((char*)&point1, 12);
	file.read((char*)&radius1, 4);
	file.read((char*)&point2, 12);
	file.read((char*)&radius2, 4);
}

void bhkCapsuleShape::Put(fstream& file) {
	bhkConvexShape::Put(file);

	file.write((char*)&padding1, 4);
	file.write((char*)&padding2, 4);
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


bhkMoppBvTreeShape::bhkMoppBvTreeShape(NiHeader& hdr) {
	bhkBvTreeShape::Init();

	header = &hdr;
	blockType = BHKMOPPBVTREESHAPE;

	shapeRef = 0xFFFFFFFF;
	dataSize = 0;
}

bhkMoppBvTreeShape::bhkMoppBvTreeShape(fstream& file, NiHeader& hdr) {
	bhkBvTreeShape::Init();

	header = &hdr;
	blockType = BHKMOPPBVTREESHAPE;

	Get(file);
}

void bhkMoppBvTreeShape::Get(fstream& file) {
	bhkBvTreeShape::Get(file);

	file.read((char*)&shapeRef, 4);
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

void bhkMoppBvTreeShape::Put(fstream& file) {
	bhkBvTreeShape::Put(file);

	file.write((char*)&shapeRef, 4);
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

void bhkMoppBvTreeShape::notifyBlockDelete(int blockID) {
	bhkBvTreeShape::notifyBlockDelete(blockID);

	if (shapeRef == blockID)
		shapeRef = 0xFFFFFFFF;
	else if (shapeRef > blockID)
		shapeRef--;
}

void bhkMoppBvTreeShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkBvTreeShape::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (shapeRef == blockIndexLo)
		shapeRef = blockIndexHi;
	else if (shapeRef == blockIndexHi)
		shapeRef = blockIndexLo;
}

void bhkMoppBvTreeShape::GetChildRefs(set<int>& refs) {
	bhkBvTreeShape::GetChildRefs(refs);

	refs.insert(shapeRef);
}

int bhkMoppBvTreeShape::CalcBlockSize() {
	bhkBvTreeShape::CalcBlockSize();

	blockSize += 40;

	if (header->GetUserVersion() >= 12)
		blockSize += 1;

	blockSize += dataSize;

	return blockSize;
}


bhkNiTriStripsShape::bhkNiTriStripsShape(NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKNITRISTRIPSSHAPE;

	numParts = 0;
	numFilters = 0;
}

bhkNiTriStripsShape::bhkNiTriStripsShape(fstream& file, NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKNITRISTRIPSSHAPE;

	Get(file);
}

void bhkNiTriStripsShape::Get(fstream& file) {
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

	file.read((char*)&numParts, 4);
	parts.resize(numParts);
	for (int i = 0; i < numParts; i++)
		file.read((char*)&parts[i], 4);

	file.read((char*)&numFilters, 4);
	filters.resize(numFilters);
	for (int i = 0; i < numFilters; i++)
		file.read((char*)&filters[i], 4);
}

void bhkNiTriStripsShape::Put(fstream& file) {
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

	file.write((char*)&numParts, 4);
	for (int i = 0; i < numParts; i++)
		file.write((char*)&parts[i], 4);

	file.write((char*)&numFilters, 4);
	for (int i = 0; i < numFilters; i++)
		file.write((char*)&filters[i], 4);
}

void bhkNiTriStripsShape::notifyBlockDelete(int blockID) {
	bhkShape::notifyBlockDelete(blockID);

	for (int i = 0; i < numParts; i++) {
		if (parts[i] == blockID) {
			parts.erase(parts.begin() + i);
			i--;
			numParts--;
		}
		else if (parts[i] > blockID)
			parts[i]--;
	}
}

void bhkNiTriStripsShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkShape::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numParts; i++) {
		if (parts[i] == blockIndexLo)
			parts[i] = blockIndexHi;
		else if (parts[i] == blockIndexHi)
			parts[i] = blockIndexLo;
	}
}

void bhkNiTriStripsShape::GetChildRefs(set<int>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(parts.begin(), parts.end());
}

int bhkNiTriStripsShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 56;
	blockSize += numParts * 4;
	blockSize += numFilters * 4;

	return blockSize;
}


bhkListShape::bhkListShape(NiHeader& hdr) {
	bhkShapeCollection::Init();

	header = &hdr;
	blockType = BHKLISTSHAPE;

	numSubShapes = 0;
	numUnkInts = 0;
}

bhkListShape::bhkListShape(fstream& file, NiHeader& hdr) {
	bhkShapeCollection::Init();

	header = &hdr;
	blockType = BHKLISTSHAPE;

	Get(file);
}

void bhkListShape::Get(fstream& file) {
	bhkShapeCollection::Get(file);

	file.read((char*)&numSubShapes, 4);
	subShapeRef.resize(numSubShapes);

	for (int i = 0; i < numSubShapes; i++)
		file.read((char*)&subShapeRef[i], 4);

	file.read((char*)&material, 4);
	file.read((char*)unkFloats, 24);

	file.read((char*)&numUnkInts, 4);
	unkInts.resize(numUnkInts);

	for (int i = 0; i < numUnkInts; i++)
		file.read((char*)&unkInts[i], 4);
}

void bhkListShape::Put(fstream& file) {
	bhkShapeCollection::Put(file);

	file.write((char*)&numSubShapes, 4);
	for (int i = 0; i < numSubShapes; i++)
		file.write((char*)&subShapeRef[i], 4);

	file.write((char*)&material, 4);
	file.write((char*)unkFloats, 24);

	file.write((char*)&numUnkInts, 4);
	for (int i = 0; i < numUnkInts; i++)
		file.write((char*)&unkInts[i], 4);
}

void bhkListShape::notifyBlockDelete(int blockID) {
	bhkShapeCollection::notifyBlockDelete(blockID);

	for (int i = 0; i < numSubShapes; i++) {
		if (subShapeRef[i] == blockID) {
			subShapeRef.erase(subShapeRef.begin() + i);
			i--;
			numSubShapes--;
		}
		else if (subShapeRef[i] > blockID)
			subShapeRef[i]--;
	}
}

void bhkListShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkShapeCollection::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numSubShapes; i++) {
		if (subShapeRef[i] == blockIndexLo)
			subShapeRef[i] = blockIndexHi;
		else if (subShapeRef[i] == blockIndexHi)
			subShapeRef[i] = blockIndexLo;
	}
}

void bhkListShape::GetChildRefs(set<int>& refs) {
	bhkShapeCollection::GetChildRefs(refs);

	refs.insert(subShapeRef.begin(), subShapeRef.end());
}

int bhkListShape::CalcBlockSize() {
	bhkShapeCollection::CalcBlockSize();

	blockSize += 36;
	blockSize += 4 * numSubShapes;
	blockSize += 4 * numUnkInts;

	return blockSize;
}


void bhkWorldObject::Init() {
	bhkSerializable::Init();

	shapeRef = 0xFFFFFFFF;
}

void bhkWorldObject::Get(fstream& file) {
	bhkSerializable::Get(file);

	file.read((char*)&shapeRef, 4);
	file.read((char*)&collisionFilter, 4);
	file.read((char*)&unkInt1, 4);
	file.read((char*)&broadPhaseType, 1);
	file.read((char*)unkBytes, 3);
	file.read((char*)&prop, 12);
}

void bhkWorldObject::Put(fstream& file) {
	bhkSerializable::Put(file);

	file.write((char*)&shapeRef, 4);
	file.write((char*)&collisionFilter, 4);
	file.write((char*)&unkInt1, 4);
	file.write((char*)&broadPhaseType, 1);
	file.write((char*)unkBytes, 3);
	file.write((char*)&prop, 12);
}

void bhkWorldObject::notifyBlockDelete(int blockID) {
	bhkSerializable::notifyBlockDelete(blockID);

	if (shapeRef == blockID)
		shapeRef = 0xFFFFFFFF;
	else if (shapeRef > blockID)
		shapeRef--;
}

void bhkWorldObject::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkSerializable::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (shapeRef == blockIndexLo)
		shapeRef = blockIndexHi;
	else if (shapeRef == blockIndexHi)
		shapeRef = blockIndexLo;
}

void bhkWorldObject::GetChildRefs(set<int>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(shapeRef);
}

int bhkWorldObject::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 28;

	return blockSize;
}


bhkSimpleShapePhantom::bhkSimpleShapePhantom(NiHeader& hdr) {
	bhkShapePhantom::Init();

	header = &hdr;
	blockType = BHKSIMPLESHAPEPHANTOM;
}

bhkSimpleShapePhantom::bhkSimpleShapePhantom(fstream& file, NiHeader& hdr) {
	bhkShapePhantom::Init();

	header = &hdr;
	blockType = BHKSIMPLESHAPEPHANTOM;

	Get(file);
}

void bhkSimpleShapePhantom::Get(fstream& file) {
	bhkShapePhantom::Get(file);

	file.read((char*)&padding1, 4);
	file.read((char*)&padding2, 4);
	file.read((char*)&transform, 64);
}

void bhkSimpleShapePhantom::Put(fstream& file) {
	bhkShapePhantom::Put(file);

	file.write((char*)&padding1, 4);
	file.write((char*)&padding2, 4);
	file.write((char*)&transform, 64);
}

int bhkSimpleShapePhantom::CalcBlockSize() {
	bhkShapePhantom::CalcBlockSize();

	blockSize += 72;

	return blockSize;
}


void bhkConstraint::Init() {
	bhkSerializable::Init();

	numEntities = 0;
}

void bhkConstraint::Get(fstream& file) {
	bhkSerializable::Get(file);

	file.read((char*)&numEntities, 4);
	entities.resize(numEntities);
	for (int i = 0; i < numEntities; i++)
		file.read((char*)&entities[i], 4);

	file.read((char*)&priority, 4);
}

void bhkConstraint::Put(fstream& file) {
	bhkSerializable::Put(file);

	file.write((char*)&numEntities, 4);
	for (int i = 0; i < numEntities; i++)
		file.write((char*)&entities[i], 4);

	file.write((char*)&priority, 4);
}

void bhkConstraint::notifyBlockDelete(int blockID) {
	bhkSerializable::notifyBlockDelete(blockID);

	for (int i = 0; i < numEntities; i++) {
		if (entities[i] == blockID) {
			entities.erase(entities.begin() + i);
			i--;
			numEntities--;
		}
		else if (entities[i] > blockID)
			entities[i]--;
	}
}

void bhkConstraint::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkSerializable::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numEntities; i++) {
		if (entities[i] == blockIndexLo)
			entities[i] = blockIndexHi;
		else if (entities[i] == blockIndexHi)
			entities[i] = blockIndexLo;
	}
}

void bhkConstraint::GetChildRefs(set<int>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(entities.begin(), entities.end());
}

int bhkConstraint::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 8;
	blockSize += numEntities * 4;

	return blockSize;
}


bhkHingeConstraint::bhkHingeConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKHINGECONSTRAINT;
}

bhkHingeConstraint::bhkHingeConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKHINGECONSTRAINT;

	Get(file);
}

void bhkHingeConstraint::Get(fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&hinge, 128);
}

void bhkHingeConstraint::Put(fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&hinge, 128);
}

int bhkHingeConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 128;

	return blockSize;
}


bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKLIMITEDHINGECONSTRAINT;
}

bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKLIMITEDHINGECONSTRAINT;

	Get(file);
}

void bhkLimitedHingeConstraint::Get(fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&limitedHinge.hinge, 128);
	file.read((char*)&limitedHinge.minAngle, 4);
	file.read((char*)&limitedHinge.maxAngle, 4);
	file.read((char*)&limitedHinge.maxFriction, 4);
	file.read((char*)&limitedHinge.enableMotor, 1);

	if (limitedHinge.enableMotor)
		file.read((char*)&limitedHinge.motor, 25);
}

void bhkLimitedHingeConstraint::Put(fstream& file) {
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


bhkBreakableConstraint::bhkBreakableConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKBREAKABLECONSTRAINT;
}

bhkBreakableConstraint::bhkBreakableConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKBREAKABLECONSTRAINT;

	Get(file);
}

void bhkBreakableConstraint::Get(fstream& file) {
	bhkConstraint::Get(file);

	subConstraint.Get(file);
	file.read((char*)&threshold, 4);
	file.read((char*)&removeIfBroken, 1);
}

void bhkBreakableConstraint::Put(fstream& file) {
	bhkConstraint::Put(file);

	subConstraint.Put(file);
	file.write((char*)&threshold, 4);
	file.write((char*)&removeIfBroken, 1);
}

void bhkBreakableConstraint::notifyBlockDelete(int blockID) {
	bhkConstraint::notifyBlockDelete(blockID);

	subConstraint.notifyBlockDelete(blockID);
}

void bhkBreakableConstraint::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkConstraint::notifyBlockSwap(blockIndexLo, blockIndexHi);

	subConstraint.notifyBlockSwap(blockIndexLo, blockIndexHi);
}

void bhkBreakableConstraint::GetChildRefs(set<int>& refs) {
	bhkConstraint::GetChildRefs(refs);

	subConstraint.GetChildRefs(refs);
}

int bhkBreakableConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 5;
	blockSize += subConstraint.CalcDescSize();

	return blockSize;
}


bhkRagdollConstraint::bhkRagdollConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKRAGDOLLCONSTRAINT;
}

bhkRagdollConstraint::bhkRagdollConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKRAGDOLLCONSTRAINT;

	Get(file);
}

void bhkRagdollConstraint::Get(fstream& file) {
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

void bhkRagdollConstraint::Put(fstream& file) {
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


bhkStiffSpringConstraint::bhkStiffSpringConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKSTIFFSPRINGCONSTRAINT;
}

bhkStiffSpringConstraint::bhkStiffSpringConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKSTIFFSPRINGCONSTRAINT;

	Get(file);
}

void bhkStiffSpringConstraint::Get(fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&stiffSpring.pivotA, 16);
	file.read((char*)&stiffSpring.pivotB, 16);
	file.read((char*)&stiffSpring.length, 4);
}

void bhkStiffSpringConstraint::Put(fstream& file) {
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


bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKBALLANDSOCKETCONSTRAINT;
}

bhkBallAndSocketConstraint::bhkBallAndSocketConstraint(fstream& file, NiHeader& hdr) {
	bhkConstraint::Init();

	header = &hdr;
	blockType = BHKBALLANDSOCKETCONSTRAINT;

	Get(file);
}

void bhkBallAndSocketConstraint::Get(fstream& file) {
	bhkConstraint::Get(file);

	file.read((char*)&ballAndSocket.translationA, 16);
	file.read((char*)&ballAndSocket.translationB, 16);
}

void bhkBallAndSocketConstraint::Put(fstream& file) {
	bhkConstraint::Put(file);

	file.write((char*)&ballAndSocket.translationA, 16);
	file.write((char*)&ballAndSocket.translationB, 16);
}

int bhkBallAndSocketConstraint::CalcBlockSize() {
	bhkConstraint::CalcBlockSize();

	blockSize += 32;

	return blockSize;
}


bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(NiHeader& hdr) {
	bhkSerializable::Init();

	header = &hdr;
	blockType = BHKBALLSOCKETCONSTRAINTCHAIN;

	numPivots = 0;
	numEntitiesA = 0;
	numEntities = 0;
	entityARef = 0xFFFFFFFF;
	entityBRef = 0xFFFFFFFF;
}

bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(fstream& file, NiHeader& hdr) {
	bhkSerializable::Init();

	header = &hdr;
	blockType = BHKBALLSOCKETCONSTRAINTCHAIN;

	Get(file);
}

void bhkBallSocketConstraintChain::Get(fstream& file) {
	bhkSerializable::Get(file);

	file.read((char*)&numPivots, 4);
	pivots.resize(numPivots);
	for (int i = 0; i < numPivots; i++)
		file.read((char*)&pivots[i], 16);

	file.read((char*)&tau, 4);
	file.read((char*)&damping, 4);
	file.read((char*)&cfm, 4);
	file.read((char*)&maxErrorDistance, 4);

	file.read((char*)&numEntitiesA, 4);
	entityARefs.resize(numEntitiesA);
	for (int i = 0; i < numEntitiesA; i++)
		file.read((char*)&entityARefs[i], 4);

	file.read((char*)&numEntities, 4);
	file.read((char*)&entityARef, 4);
	file.read((char*)&entityBRef, 4);
	file.read((char*)&priority, 4);
}

void bhkBallSocketConstraintChain::Put(fstream& file) {
	bhkSerializable::Put(file);

	file.write((char*)&numPivots, 4);
	for (int i = 0; i < numPivots; i++)
		file.write((char*)&pivots[i], 16);

	file.write((char*)&tau, 4);
	file.write((char*)&damping, 4);
	file.write((char*)&cfm, 4);
	file.write((char*)&maxErrorDistance, 4);

	file.write((char*)&numEntitiesA, 4);
	for (int i = 0; i < numEntitiesA; i++)
		file.write((char*)&entityARefs[i], 4);

	file.write((char*)&numEntities, 4);
	file.write((char*)&entityARef, 4);
	file.write((char*)&entityBRef, 4);
	file.write((char*)&priority, 4);
}

void bhkBallSocketConstraintChain::notifyBlockDelete(int blockID) {
	bhkSerializable::notifyBlockDelete(blockID);

	for (int i = 0; i < numEntitiesA; i++) {
		if (entityARefs[i] == blockID) {
			entityARefs.erase(entityARefs.begin() + i);
			i--;
			numEntitiesA--;
		}
		else if (entityARefs[i] > blockID)
			entityARefs[i]--;
	}

	if (entityARef == blockID)
		entityARef = 0xFFFFFFFF;
	else if (entityARef > blockID)
		entityARef--;

	if (entityBRef == blockID)
		entityBRef = 0xFFFFFFFF;
	else if (entityBRef > blockID)
		entityBRef--;
}

void bhkBallSocketConstraintChain::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkSerializable::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numEntitiesA; i++) {
		if (entityARefs[i] == blockIndexLo)
			entityARefs[i] = blockIndexHi;
		else if (entityARefs[i] == blockIndexHi)
			entityARefs[i] = blockIndexLo;
	}

	if (entityARef == blockIndexLo)
		entityARef = blockIndexHi;
	else if (entityARef == blockIndexHi)
		entityARef = blockIndexLo;

	if (entityBRef == blockIndexLo)
		entityBRef = blockIndexHi;
	else if (entityBRef == blockIndexHi)
		entityBRef = blockIndexLo;
}

void bhkBallSocketConstraintChain::GetChildRefs(set<int>& refs) {
	bhkSerializable::GetChildRefs(refs);

	refs.insert(entityARefs.begin(), entityARefs.end());
	refs.insert(entityARef);
	refs.insert(entityBRef);
}

int bhkBallSocketConstraintChain::CalcBlockSize() {
	bhkSerializable::CalcBlockSize();

	blockSize += 40;
	blockSize += numPivots * 16;
	blockSize += numEntitiesA * 4;

	return blockSize;
}


bhkRigidBody::bhkRigidBody(NiHeader& hdr) {
	bhkEntity::Init();

	header = &hdr;
	blockType = BHKRIGIDBODY;

	numConstraints = 0;
}

bhkRigidBody::bhkRigidBody(fstream& file, NiHeader& hdr) {
	bhkEntity::Init();

	header = &hdr;
	blockType = BHKRIGIDBODY;

	Get(file);
}

void bhkRigidBody::Get(fstream& file) {
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

	file.read((char*)&numConstraints, 4);
	constraints.resize(numConstraints);

	for (int i = 0; i < numConstraints; i++)
		file.read((char*)&constraints[i], 4);

	if (header->GetUserVersion() <= 11)
		file.read((char*)&unkInt5, 4);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&unkShort3, 2);
}

void bhkRigidBody::Put(fstream& file) {
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

	file.write((char*)&numConstraints, 4);
	constraints.resize(numConstraints);

	for (int i = 0; i < numConstraints; i++)
		file.write((char*)&constraints[i], 4);

	if (header->GetUserVersion() <= 11)
		file.write((char*)&unkInt5, 4);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&unkShort3, 2);
}

void bhkRigidBody::notifyBlockDelete(int blockID) {
	bhkEntity::notifyBlockDelete(blockID);

	for (int i = 0; i < numConstraints; i++) {
		if (constraints[i] == blockID) {
			constraints.erase(constraints.begin() + i);
			i--;
			numConstraints--;
		}
		else if (constraints[i] > blockID)
			constraints[i]--;
	}
}

void bhkRigidBody::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkEntity::notifyBlockSwap(blockIndexLo, blockIndexHi);

	for (int i = 0; i < numConstraints; i++) {
		if (constraints[i] == blockIndexLo)
			constraints[i] = blockIndexHi;
		else if (constraints[i] == blockIndexHi)
			constraints[i] = blockIndexLo;
	}
}

void bhkRigidBody::GetChildRefs(set<int>& refs) {
	bhkEntity::GetChildRefs(refs);

	refs.insert(constraints.begin(), constraints.end());
}

int bhkRigidBody::CalcBlockSize() {
	bhkEntity::CalcBlockSize();

	blockSize += 204;
	blockSize += 4 * numConstraints;

	if (header->GetUserVersion() <= 11)
		blockSize += 4;

	if (header->GetUserVersion() >= 12)
		blockSize += 18;

	return blockSize;
}


bhkRigidBodyT::bhkRigidBodyT(NiHeader& hdr) : bhkRigidBody(hdr) {
	blockType = BHKRIGIDBODYT;
}

bhkRigidBodyT::bhkRigidBodyT(fstream& file, NiHeader& hdr) : bhkRigidBody(file, hdr) {
	blockType = BHKRIGIDBODYT;
}


bhkCompressedMeshShape::bhkCompressedMeshShape(NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKCOMPRESSEDMESHSHAPE;

	targetRef = 0xFFFFFFFF;
	dataRef = 0xFFFFFFFF;
}

bhkCompressedMeshShape::bhkCompressedMeshShape(fstream& file, NiHeader& hdr) {
	bhkShape::Init();

	header = &hdr;
	blockType = BHKCOMPRESSEDMESHSHAPE;

	Get(file);
}

void bhkCompressedMeshShape::Get(fstream& file) {
	bhkShape::Get(file);

	file.read((char*)&targetRef, 4);
	file.read((char*)&userData, 4);
	file.read((char*)&radius, 4);
	file.read((char*)&unkFloat, 4);
	file.read((char*)&scaling, 16);
	file.read((char*)&radius2, 4);
	file.read((char*)&scaling2, 16);
	file.read((char*)&dataRef, 4);
}

void bhkCompressedMeshShape::Put(fstream& file) {
	bhkShape::Put(file);

	file.write((char*)&targetRef, 4);
	file.write((char*)&userData, 4);
	file.write((char*)&radius, 4);
	file.write((char*)&unkFloat, 4);
	file.write((char*)&scaling, 16);
	file.write((char*)&radius2, 4);
	file.write((char*)&scaling2, 16);
	file.write((char*)&dataRef, 4);
}

void bhkCompressedMeshShape::notifyBlockDelete(int blockID) {
	bhkShape::notifyBlockDelete(blockID);

	if (targetRef == blockID)
		targetRef = 0xFFFFFFFF;
	else if (targetRef > blockID)
		targetRef--;

	if (dataRef == blockID)
		dataRef = 0xFFFFFFFF;
	else if (dataRef > blockID)
		dataRef--;
}

void bhkCompressedMeshShape::notifyBlockSwap(int blockIndexLo, int blockIndexHi) {
	bhkShape::notifyBlockSwap(blockIndexLo, blockIndexHi);

	if (targetRef == blockIndexLo)
		targetRef = blockIndexHi;
	else if (targetRef == blockIndexHi)
		targetRef = blockIndexLo;

	if (dataRef == blockIndexLo)
		dataRef = blockIndexHi;
	else if (dataRef == blockIndexHi)
		dataRef = blockIndexLo;
}

void bhkCompressedMeshShape::GetChildRefs(set<int>& refs) {
	bhkShape::GetChildRefs(refs);

	refs.insert(targetRef);
	refs.insert(dataRef);
}

int bhkCompressedMeshShape::CalcBlockSize() {
	bhkShape::CalcBlockSize();

	blockSize += 56;

	return blockSize;
}


bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(NiHeader& hdr) {
	bhkRefObject::Init();

	header = &hdr;
	blockType = BHKCOMPRESSEDMESHSHAPEDATA;

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

bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(fstream& file, NiHeader& hdr) {
	bhkRefObject::Init();

	header = &hdr;
	blockType = BHKCOMPRESSEDMESHSHAPEDATA;

	Get(file);
}

void bhkCompressedMeshShapeData::Get(fstream& file) {
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

void bhkCompressedMeshShapeData::Put(fstream& file) {
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


NiUnknown::NiUnknown() {
	NiObject::Init();

	blockType = NIUNKNOWN;
}

NiUnknown::NiUnknown(fstream& file, const uint& size) {
	NiObject::Init();

	blockType = NIUNKNOWN;
	data.resize(size);

	blockSize = size;
	Get(file);
}

NiUnknown::NiUnknown(const uint& size) {
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
