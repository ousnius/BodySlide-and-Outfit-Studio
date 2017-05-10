/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "BasicTypes.h"

std::string StringRef::GetString(NiHeader* hdr) {
	return hdr->GetStringById(ref);
}

void StringRef::SetString(NiHeader* hdr, const std::string& str) {
	ref = hdr->AddOrFindStringId(str);
}

void StringRef::RenameString(NiHeader* hdr, const std::string& str) {
	hdr->SetStringById(ref, str);
}

void StringRef::Clear(NiHeader* hdr) {
	hdr->RemoveStringRef(ref);
	ref = 0xFFFFFFFF;
}

void StringRef::Put(std::fstream& file) {
	file.write((char*)&ref, 4);
}

void StringRef::Get(std::fstream& file, NiHeader* hdr) {
	file.read((char*)&ref, 4);
	hdr->AddStringRef(ref);
}

void StringRef::notifyStringDelete(int stringID) {
	if (ref != 0xFFFFFFFF && ref > stringID)
		ref--;
}


void NiString::Put(std::fstream& file, const int szSize, const bool wantNullOutput) {
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

void NiString::Get(std::fstream& file, const int szSize) {
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


NiHeader::NiHeader() {
	valid = false;

	header = this;

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

std::string NiHeader::GetVersionInfo() {
	return std::string(verStr, 0x26) +
		"\nUser Version: " + std::to_string(userVersion) +
		"\nUser Version 2: " + std::to_string(userVersion2);
}

void NiHeader::SetVersion(const byte v1, const byte v2, const byte v3, const byte v4, const uint userVer, const uint userVer2) {
	std::string verString = "Gamebryo File Format, Version " + std::to_string(v1) + '.' + std::to_string(v2) + '.' + std::to_string(v3) + '.' + std::to_string(v4);
	strncpy(verStr, verString.c_str(), 0x26);

	version4 = v1;
	version3 = v2;
	version2 = v3;
	version1 = v4;
	userVersion = userVer;
	userVersion2 = userVer2;
}

bool NiHeader::VerCheck(const int v1, const int v2, const int v3, const int v4, const bool equal) {
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

std::string NiHeader::GetCreatorInfo() {
	return creator.GetString();
}

void NiHeader::SetCreatorInfo(const std::string& creatorInfo) {
	creator.SetString(creatorInfo);
}

std::string NiHeader::GetExportInfo() {
	std::string exportInfo = exportInfo1.GetString();

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

void NiHeader::SetExportInfo(const std::string& exportInfo) {
	exportInfo1.Clear();
	exportInfo2.Clear();
	exportInfo3.Clear();

	std::vector<NiString*> exportStrings(3);
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

	// Next tell all the blocks that the deletion happened
	for (auto &b : (*blocks))
		BlockDeleted(b, blockId);
}

void NiHeader::DeleteBlockByType(const std::string& blockTypeStr) {
	ushort blockTypeId;
	for (blockTypeId = 0; blockTypeId < numBlockTypes; blockTypeId++)
		if (blockTypes[blockTypeId].GetString() == blockTypeStr)
			break;

	if (blockTypeId == numBlockTypes)
		return;

	std::vector<int> indices;
	for (int i = 0; i < numBlocks; i++)
		if (blockTypeIndices[i] == blockTypeId)
			indices.push_back(i);

	for (int j = indices.size() - 1; j >= 0; j--)
		DeleteBlock(indices[j]);
}

int NiHeader::AddBlock(NiObject* newBlock) {
	ushort btID = AddOrFindBlockTypeId(newBlock->GetBlockName());
	blockTypeIndices.push_back(btID);
	blockSizes.push_back(newBlock->CalcBlockSize());
	(*blocks).push_back(newBlock);
	numBlocks = (*blocks).size();
	return numBlocks - 1;
}

int NiHeader::ReplaceBlock(int oldBlockId, NiObject* newBlock) {
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

	ushort btID = AddOrFindBlockTypeId(newBlock->GetBlockName());
	blockTypeIndices[oldBlockId] = btID;
	blockSizes[oldBlockId] = newBlock->CalcBlockSize();
	(*blocks)[oldBlockId] = newBlock;
	return oldBlockId;
}

void NiHeader::SwapBlocks(const int blockIndexLo, const int blockIndexHi) {
	if (blockIndexLo == 0xFFFFFFFF || blockIndexHi == 0xFFFFFFFF)
		return;

	// First swap data
	iter_swap(blockTypeIndices.begin() + blockIndexLo, blockTypeIndices.begin() + blockIndexHi);
	iter_swap(blockSizes.begin() + blockIndexLo, blockSizes.begin() + blockIndexHi);
	iter_swap((*blocks).begin() + blockIndexLo, (*blocks).begin() + blockIndexHi);

	// Next tell all the blocks that the swap happened
	for (auto &b : (*blocks))
		BlockSwapped(b, blockIndexLo, blockIndexHi);
}

bool NiHeader::IsBlockReferenced(const int blockId) {
	if (blockId == 0xFFFFFFFF)
		return false;

	for (auto &block : (*blocks)) {
		std::set<int*> references;
		block->GetChildRefs(references);

		for (auto &ref : references)
			if ((*ref) == blockId)
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

ushort NiHeader::AddOrFindBlockTypeId(const std::string& blockTypeName) {
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

std::string NiHeader::GetBlockTypeStringById(const int id) {
	return blockTypes[blockTypeIndices[id]].GetString();
}

ushort NiHeader::GetBlockTypeIndex(const int id) {
	return blockTypeIndices[id];
}

void NiHeader::CalcAllBlockSizes() {
	for (int i = 0; i < numBlocks; i++)
		blockSizes[i] = (*blocks)[i]->CalcBlockSize();
}

int NiHeader::FindStringId(const std::string& str) {
	for (int i = 0; i < strings.size(); i++)
		if (strings[i].GetString().compare(str) == 0)
			return i;

	return 0xFFFFFFFF;
}

int NiHeader::AddOrFindStringId(const std::string& str) {
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

std::string NiHeader::GetStringById(const int id) {
	if (id >= 0 && id < numStrings)
		return strings[id].GetString();

	return std::string();
}

void NiHeader::SetStringById(const int id, const std::string& str) {
	if (id >= 0 && id < numStrings) {
		strings[id].SetString(str);
		if (maxStringLen < str.length())
			maxStringLen = str.length();
	}
}

void NiHeader::AddStringRef(const int id) {
	if (id >= 0 && id < numStrings)
		stringRefCount[id]++;
}

void NiHeader::RemoveStringRef(const int id) {
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

void NiHeader::BlockDeleted(NiObject* p, int blockId) {
	std::set<int*> childRefs;
	p->GetChildRefs(childRefs);

	for (auto &r : childRefs) {
		auto& index = (*r);
		if (index == blockId)
			index = 0xFFFFFFFF;
		else if (index > blockId)
			index--;
	}
}

void NiHeader::BlockSwapped(NiObject* o, int blockIndexLo, int blockIndexHi) {
	std::set<int*> childRefs;
	o->GetChildRefs(childRefs);

	for (auto &r : childRefs) {
		auto& index = (*r);
		if (index == blockIndexLo)
			index = blockIndexHi;
		else if (index == blockIndexHi)
			index = blockIndexLo;
	}
}

void NiHeader::Get(std::fstream& file) {
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
	blockTypes.resize(numBlockTypes);
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes[i].Get(file, 4);

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
		strings[i].Get(file, 4);

	file.read((char*)&unkInt2, 4);
	valid = true;
}

void NiHeader::Put(std::fstream& file) {
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


NiUnknown::NiUnknown(std::fstream& file, const uint size) {
	data.resize(size);

	blockSize = size;
	Get(file);
}

NiUnknown::NiUnknown(const uint size) {
	data.resize(size);

	blockSize = size;
}

void NiUnknown::Get(std::fstream& file) {
	if (data.empty())
		return;

	file.read(&data[0], blockSize);
}

void NiUnknown::Put(std::fstream& file) {
	if (data.empty())
		return;

	file.write(&data[0], blockSize);
}
