/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "BasicTypes.h"

std::string NiVersion::GetVersionInfo() {
	return vstr +
		"\nUser Version: " + std::to_string(vuser) +
		"\nUser Version 2: " + std::to_string(vuser2);
}

void NiVersion::Set(byte v1, byte v2, byte v3, byte v4, uint userVer, uint userVer2) {
	std::string verNum = std::to_string(v1) + '.' + std::to_string(v2) + '.' + std::to_string(v3) + '.' + std::to_string(v4);
	vstr = "Gamebryo File Format, Version " + verNum;

	vfile = Get(v1, v2, v3, v4);
	vuser = userVer;
	vuser2 = userVer2;
}


void NiString::Get(NiStream& stream, const int szSize) {
	char buf[1025];

	if (szSize == 1) {
		byte smSize;
		stream >> smSize;
		stream.read(buf, smSize);
		buf[smSize] = 0;
	}
	else if (szSize == 2) {
		ushort medSize;
		stream >> medSize;
		if (medSize < 1025)
			stream.read(buf, medSize);
		buf[medSize] = 0;
	}
	else if (szSize == 4) {
		uint bigSize;
		stream >> bigSize;
		if (bigSize < 1025)
			stream.read(buf, bigSize);
		buf[bigSize] = 0;
	}
	else
		return;

	str = buf;
}

void NiString::Put(NiStream& stream, const int szSize, const bool wantNullOutput) {
	if (szSize == 1) {
		byte smSize = str.length();
		if (wantNullOutput)
			smSize += 1;

		stream << smSize;
	}
	else if (szSize == 2) {
		ushort medSize = str.length();
		if (wantNullOutput)
			medSize += 1;

		stream << medSize;
	}
	else if (szSize == 4) {
		uint bigSize = str.length();
		if (wantNullOutput)
			bigSize += 1;

		stream << bigSize;
	}

	stream.write(str.c_str(), str.length());
	if (wantNullOutput)
		stream << byte(0);
}


NiHeader::NiHeader() {
	valid = false;

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

	blocks->erase(blocks->begin() + blockId);
	numBlocks--;
	blockTypeIndices.erase(blockTypeIndices.begin() + blockId);
	blockSizes.erase(blockSizes.begin() + blockId);

	// Next tell all the blocks that the deletion happened
	for (auto &b : (*blocks))
		BlockDeleted(b.get(), blockId);
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
	blockSizes.push_back(newBlock->CalcBlockSize(version));
	blocks->push_back(std::move(std::unique_ptr<NiObject>(newBlock)));
	numBlocks = blocks->size();
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

	ushort btID = AddOrFindBlockTypeId(newBlock->GetBlockName());
	blockTypeIndices[oldBlockId] = btID;
	blockSizes[oldBlockId] = newBlock->CalcBlockSize(version);
	auto blockPtrSwap = std::unique_ptr<NiObject>(newBlock);
	(*blocks)[oldBlockId].swap(blockPtrSwap);
	return oldBlockId;
}

void NiHeader::SwapBlocks(const int blockIndexLo, const int blockIndexHi) {
	if (blockIndexLo == 0xFFFFFFFF || blockIndexHi == 0xFFFFFFFF)
		return;

	// First swap data
	iter_swap(blockTypeIndices.begin() + blockIndexLo, blockTypeIndices.begin() + blockIndexHi);
	iter_swap(blockSizes.begin() + blockIndexLo, blockSizes.begin() + blockIndexHi);
	iter_swap(blocks->begin() + blockIndexLo, blocks->begin() + blockIndexHi);

	// Next tell all the blocks that the swap happened
	for (auto &b : (*blocks))
		BlockSwapped(b.get(), blockIndexLo, blockIndexHi);
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
		blockSizes[i] = blocks->at(i)->CalcBlockSize(version);
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
			return i;
		}
	}

	int r = strings.size();

	NiString niStr;
	niStr.SetString(str);
	strings.push_back(niStr);
	numStrings++;

	return r;
}

std::string NiHeader::GetStringById(const int id) {
	if (id >= 0 && id < numStrings)
		return strings[id].GetString();

	return std::string();
}

void NiHeader::SetStringById(const int id, const std::string& str) {
	if (id >= 0 && id < numStrings)
		strings[id].SetString(str);
}

void NiHeader::ClearStrings() {
	strings.clear();
	numStrings = 0;
	maxStringLen = 0;
}

void NiHeader::UpdateMaxStringLength() {
	maxStringLen = 0;
	for (auto &s : strings)
		if (maxStringLen < s.GetLength())
			maxStringLen = s.GetLength();
}

void NiHeader::FillStringRefs() {
	for (auto &b : (*blocks)) {
		std::set<StringRef*> stringRefs;
		b->GetStringRefs(stringRefs);

		for (auto &r : stringRefs) {
			std::string str = GetStringById(r->GetIndex());
			r->SetString(str);
		}
	}
}

void NiHeader::UpdateHeaderStrings(const bool hasUnknown) {
	if (!hasUnknown)
		ClearStrings();

	for (auto &b : (*blocks)) {
		std::set<StringRef*> stringRefs;
		b->GetStringRefs(stringRefs);

		for (auto &r : stringRefs) {
			int stringId = AddOrFindStringId(r->GetString());
			r->SetIndex(stringId);
		}
	}

	UpdateMaxStringLength();
}

void NiHeader::BlockDeleted(NiObject* o, int blockId) {
	std::set<int*> refs;
	o->GetChildRefs(refs);
	o->GetPtrs(refs);

	for (auto &r : refs) {
		auto& index = (*r);
		if (index == blockId)
			index = 0xFFFFFFFF;
		else if (index > blockId)
			index--;
	}
}

void NiHeader::BlockSwapped(NiObject* o, int blockIndexLo, int blockIndexHi) {
	std::set<int*> refs;
	o->GetChildRefs(refs);
	o->GetPtrs(refs);

	for (auto &r : refs) {
		auto& index = (*r);
		if (index == blockIndexLo)
			index = blockIndexHi;
		else if (index == blockIndexHi)
			index = blockIndexLo;
	}
}

void NiHeader::Get(NiStream& stream) {
	char ver[256];
	stream.getline(ver, sizeof(ver));
	if (_strnicmp(ver, "Gamebryo", 8) != 0)
		return;

	byte v1, v2, v3, v4;
	uint vuser, vuser2;
	stream >> v1 >> v2 >> v3 >> v4;
	stream >> endian;
	stream >> vuser;
	stream >> numBlocks;
	stream >> vuser2;

	version.Set(v4, v3, v2, v1, vuser, vuser2);

	creator.Get(stream, 1);
	exportInfo1.Get(stream, 1);
	exportInfo2.Get(stream, 1);

	if (version.User2() >= 130)
		exportInfo3.Get(stream, 1);

	stream >> numBlockTypes;
	blockTypes.resize(numBlockTypes);
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes[i].Get(stream, 4);

	blockTypeIndices.resize(numBlocks);
	for (int i = 0; i < numBlocks; i++)
		stream >> blockTypeIndices[i];

	blockSizes.resize(numBlocks);
	for (int i = 0; i < numBlocks; i++)
		stream >> blockSizes[i];

	stream >> numStrings;
	stream >> maxStringLen;

	strings.resize(numStrings);
	for (int i = 0; i < numStrings; i++)
		strings[i].Get(stream, 4);

	stream >> unkInt2;
	valid = true;
}

void NiHeader::Put(NiStream& stream) {
	std::string ver = version.String();
	stream.write(ver.data(), ver.size());

	// Newline to end header string
	stream << byte(0x0A);

	stream << version.File();
	stream << endian;
	stream << version.User();
	stream << numBlocks;
	stream << version.User2();

	creator.Put(stream, 1);
	exportInfo1.Put(stream, 1);
	exportInfo2.Put(stream, 1);
	if (version.User2() >= 130)
		exportInfo3.Put(stream, 1);

	stream << numBlockTypes;
	for (int i = 0; i < numBlockTypes; i++)
		blockTypes[i].Put(stream, 4, false);

	for (int i = 0; i < numBlocks; i++)
		stream << blockTypeIndices[i];

	for (int i = 0; i < numBlocks; i++)
		stream << blockSizes[i];

	stream << numStrings;
	stream << maxStringLen;
	for (int i = 0; i < numStrings; i++)
		strings[i].Put(stream, 4, false);

	stream << unkInt2;
}


NiUnknown::NiUnknown(NiStream& stream, const uint size) {
	data.resize(size);

	blockSize = size;
	Get(stream);
}

NiUnknown::NiUnknown(const uint size) {
	data.resize(size);

	blockSize = size;
}

void NiUnknown::Get(NiStream& stream) {
	if (data.empty())
		return;

	stream.read(&data[0], blockSize);
}

void NiUnknown::Put(NiStream& stream) {
	if (data.empty())
		return;

	stream.write(&data[0], blockSize);
}
