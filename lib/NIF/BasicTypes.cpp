/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BasicTypes.h"
#include <regex>

static const std::string NIF_GAMEBRYO = "Gamebryo File Format";
static const std::string NIF_NETIMMERSE = "NetImmerse File Format";
static const std::string NIF_NDS = "NDSNIF....@....@....";
static const std::string NIF_VERSTRING = ", Version ";

std::string NiVersion::GetVersionInfo() {
	return vstr +
		"\nUser Version: " + std::to_string(user) +
		"\nStream Version: " + std::to_string(stream);
}

void NiVersion::SetFile(NiFileVersion fileVer) {
	std::vector<byte> verArr = ToArray(fileVer);
	std::string verNum;

	if (fileVer > V3_1) {
		verNum =
			std::to_string(verArr[0]) + '.' +
			std::to_string(verArr[1]) + '.' +
			std::to_string(verArr[2]) + '.' +
			std::to_string(verArr[3]);
	}
	else {
		verNum =
			std::to_string(verArr[0]) + '.' +
			std::to_string(verArr[1]);
	}

	if (nds != 0)
		vstr = NIF_NDS;
	else if (fileVer < V10_0_0_0)
		vstr = NIF_NETIMMERSE;
	else
		vstr = NIF_GAMEBRYO;

	vstr += NIF_VERSTRING;
	vstr += verNum.data();

	file = fileVer;
}


void NiString::Get(NiStream& stream, const int szSize) {
	const int bufSize = 2048 + 1;
	char buf[bufSize];

	if (szSize == 1) {
		byte smSize;
		stream >> smSize;
		stream.read(buf, smSize);
		buf[smSize] = 0;
	}
	else if (szSize == 2) {
		ushort medSize;
		stream >> medSize;
		if (medSize < bufSize)
			stream.read(buf, medSize);
		else
			medSize = bufSize - 1;

		buf[medSize] = 0;
	}
	else if (szSize == 4) {
		uint bigSize;
		stream >> bigSize;
		if (bigSize < bufSize)
			stream.read(buf, bigSize);
		else
			bigSize = bufSize - 1;

		buf[bigSize] = 0;
	}
	else
		return;

	str = buf;
}

void NiString::Put(NiStream& stream, const int szSize, const bool wantNullOutput) {
	if (szSize == 1) {
		byte sz = byte(str.length());
		str.resize(sz);

		if (wantNullOutput)
			sz += 1;

		stream << sz;
	}
	else if (szSize == 2) {
		ushort sz = ushort(str.length());
		str.resize(sz);

		if (wantNullOutput)
			sz += 1;

		stream << sz;
	}
	else if (szSize == 4) {
		uint sz = uint(str.length());
		str.resize(sz);

		if (wantNullOutput)
			sz += 1;

		stream << sz;
	}

	stream.write(str.c_str(), str.length());
	if (wantNullOutput)
		stream << byte(0);
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
	for (size_t i = 0; i < exportInfo.length() && it < exportStrings.end(); i += 256, ++it) {
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

void NiHeader::DeleteBlockByType(const std::string& blockTypeStr, const bool orphanedOnly) {
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
		if (!orphanedOnly || !IsBlockReferenced(indices[j]))
			DeleteBlock(indices[j]);
}

int NiHeader::AddBlock(NiObject* newBlock) {
	ushort btID = AddOrFindBlockTypeId(newBlock->GetBlockName());
	blockTypeIndices.push_back(btID);
	blockSizes.push_back(0);
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
	blockSizes[oldBlockId] = 0;
	auto blockPtrSwap = std::unique_ptr<NiObject>(newBlock);
	(*blocks)[oldBlockId].swap(blockPtrSwap);
	return oldBlockId;
}

void NiHeader::SetBlockOrder(std::vector<std::pair<int, int>>& newIndices) {
	std::sort(newIndices.begin(), newIndices.end(), [](auto& l, auto& r) {
		return l.first < r.first && r.first >= 0;
	});

	newIndices.erase(std::remove_if(newIndices.begin(), newIndices.end(), [](auto& p) {
		return p.first == 0xFFFFFFFF;
	}), newIndices.end());

	if (newIndices.empty())
		return;

	for (int i = 0; i < newIndices.size() - 1; i++) {
		bool swapped = false;
		for (int j = 0; j < newIndices.size() - i - 1; j++) {
			if (newIndices[j].second > newIndices[j + 1].second) {
				auto id = newIndices[j].second;
				newIndices[j].second = newIndices[j + 1].second;
				newIndices[j + 1].second = id;

				SwapBlocks(newIndices[j].first, newIndices[j + 1].first);
				swapped = true;
			}
		}

		if (!swapped)
			break;
	}
}

void NiHeader::SwapBlocks(const int blockIndexLo, const int blockIndexHi) {
	if (blockIndexLo == 0xFFFFFFFF || blockIndexHi == 0xFFFFFFFF ||
		blockIndexLo >= numBlocks || blockIndexHi >= numBlocks ||
		blockIndexLo == blockIndexHi)
		return;

	// First swap data
	std::iter_swap(blockTypeIndices.begin() + blockIndexLo, blockTypeIndices.begin() + blockIndexHi);
	std::iter_swap(blockSizes.begin() + blockIndexLo, blockSizes.begin() + blockIndexHi);
	std::iter_swap(blocks->begin() + blockIndexLo, blocks->begin() + blockIndexHi);

	// Next tell all the blocks that the swap happened
	for (auto &b : (*blocks))
		BlockSwapped(b.get(), blockIndexLo, blockIndexHi);
}

bool NiHeader::IsBlockReferenced(const int blockId) {
	if (blockId == 0xFFFFFFFF)
		return false;

	for (auto &block : (*blocks)) {
		std::set<Ref*> refs;
		block->GetChildRefs(refs);
		block->GetPtrs(refs);

		for (auto &ref : refs)
			if (ref->GetIndex() == blockId)
				return true;
	}

	return false;
}

int NiHeader::GetBlockRefCount(const int blockId) {
	if (blockId == 0xFFFFFFFF)
		return 0;

	int refCount = 0;

	for (auto &block : (*blocks)) {
		std::set<Ref*> refs;
		block->GetChildRefs(refs);
		block->GetPtrs(refs);

		for (auto &ref : refs)
			if (ref->GetIndex() == blockId)
				refCount++;
	}

	return refCount;
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

std::string NiHeader::GetBlockTypeStringById(const int blockId) {
	if (blockId >= 0 && blockId < numBlocks) {
		ushort typeIndex = blockTypeIndices[blockId];
		if (typeIndex >= 0 && typeIndex < numBlockTypes)
			return blockTypes[typeIndex].GetString();
	}
	
	return std::string();
}

ushort NiHeader::GetBlockTypeIndex(const int blockId) {
	if (blockId >= 0 && blockId < numBlocks)
		return blockTypeIndices[blockId];

	return 0xFFFF;
}

uint NiHeader::GetBlockSize(const uint blockId) {
	if (blockId >= 0 && blockId < numBlocks)
		return blockSizes[blockId];

	return 0xFFFFFFFF;
}

std::streampos NiHeader::GetBlockSizeStreamPos() {
	return blockSizePos;
}

void NiHeader::ResetBlockSizeStreamPos() {
	blockSizePos = std::streampos();
}

int NiHeader::GetStringCount() {
	return strings.size();
}

int NiHeader::FindStringId(const std::string& str) {
	for (int i = 0; i < strings.size(); i++)
		if (strings[i].GetString().compare(str) == 0)
			return i;

	return 0xFFFFFFFF;
}

int NiHeader::AddOrFindStringId(const std::string& str, const bool addEmpty) {
	for (int i = 0; i < strings.size(); i++)
		if (strings[i].GetString().compare(str) == 0)
			return i;

	if (!addEmpty && str.empty())
		return 0xFFFFFFFF;

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
	if (version.File() < V20_1_0_1)
		return;

	for (auto &b : (*blocks)) {
		std::set<StringRef*> stringRefs;
		b->GetStringRefs(stringRefs);

		for (auto &r : stringRefs) {
			int stringId = r->GetIndex();

			// Check if string index is overflowing
			if (stringId != 0xFFFFFFFF && stringId >= numStrings) {
				stringId -= numStrings;
				r->SetIndex(stringId);
			}

			std::string str = GetStringById(stringId);
			r->SetString(str);
		}
	}
}

void NiHeader::UpdateHeaderStrings(const bool hasUnknown) {
	if (!hasUnknown)
		ClearStrings();

	if (version.File() < V20_1_0_1)
		return;

	for (auto &b : (*blocks)) {
		std::set<StringRef*> stringRefs;
		b->GetStringRefs(stringRefs);

		for (auto &r : stringRefs) {
			bool addEmpty = (r->GetIndex() != 0xFFFFFFFF);
			int stringId = AddOrFindStringId(r->GetString(), addEmpty);
			r->SetIndex(stringId);
		}
	}

	UpdateMaxStringLength();
}

void NiHeader::BlockDeleted(NiObject* o, int blockId) {
	std::set<Ref*> refs;
	o->GetChildRefs(refs);
	o->GetPtrs(refs);

	for (auto &r : refs) {
		int index = r->GetIndex();
		if (index == blockId)
			r->Clear();
		else if (index > blockId)
			r->SetIndex(index - 1);
	}
}

void NiHeader::BlockSwapped(NiObject* o, int blockIndexLo, int blockIndexHi) {
	std::set<Ref*> refs;
	o->GetChildRefs(refs);
	o->GetPtrs(refs);

	for (auto &r : refs) {
		int index = r->GetIndex();
		if (index == blockIndexLo)
			r->SetIndex(blockIndexHi);
		else if (index == blockIndexHi)
			r->SetIndex(blockIndexLo);
	}
}

void NiHeader::Get(NiStream& stream) {
	char ver[128] = { 0 };
	stream.getline(ver, sizeof(ver));

	bool isNetImmerse = std::strstr(ver, NIF_NETIMMERSE.c_str()) != nullptr;
	bool isGamebryo = std::strstr(ver, NIF_GAMEBRYO.c_str()) != nullptr;
	bool isNDS = std::strstr(ver, NIF_NDS.c_str()) != nullptr;

	if (!isNetImmerse && !isGamebryo && !isNDS)
		return;

	NiFileVersion vfile = UNKNOWN;
	uint vuser = 0;
	uint vstream = 0;

	auto verStrPtr = std::strstr(ver, NIF_VERSTRING.c_str());
	if (verStrPtr) {
		std::string verStr = verStrPtr + 10;
		std::regex reg("25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9]");
		std::smatch matches;

		byte v[4] = { 0 };
		size_t m = 0;
		while (std::regex_search(verStr, matches, reg) && m < 4) {
			v[m] = std::stoi(matches[0]);
			verStr = matches.suffix();
			m++;
		}

		vfile = NiVersion::ToFile(v[0], v[1], v[2], v[3]);
	}

	if (vfile > V3_1 && !isNDS) {
		stream >> vfile;
	}
	else if (isNDS) {
		uint versionNDS = 0;
		stream >> versionNDS;
		version.SetNDS(versionNDS);
	}
	else {
		char cr1[128] = { 0 };
		stream.getline(cr1, sizeof(cr1));
		copyright1 = cr1;

		char cr2[128] = { 0 };
		stream.getline(cr2, sizeof(cr2));
		copyright2 = cr2;

		char cr3[128] = { 0 };
		stream.getline(cr3, sizeof(cr3));
		copyright3 = cr3;
	}

	version.SetFile(vfile);

	if (version.File() >= NiVersion::ToFile(20, 0, 0, 3))
		stream >> endian;
	else
		endian = ENDIAN_LITTLE;

	if (version.File() >= NiVersion::ToFile(10, 0, 1, 8)) {
		stream >> vuser;
		version.SetUser(vuser);
	}

	stream >> numBlocks;

	if (version.IsBethesda()) {
		stream >> vstream;
		version.SetStream(vstream);

		creator.Get(stream, 1);
		exportInfo1.Get(stream, 1);
		exportInfo2.Get(stream, 1);

		if (version.Stream() >= 130)
			exportInfo3.Get(stream, 1);
	}
	else if (version.File() >= V30_0_0_2) {
		stream >> embedDataSize;
		embedData.resize(embedDataSize);
		for (int i = 0; i < embedDataSize; i++)
			stream >> embedData[i];
	}

	if (version.File() >= V5_0_0_1) {
		stream >> numBlockTypes;
		blockTypes.resize(numBlockTypes);
		for (int i = 0; i < numBlockTypes; i++)
			blockTypes[i].Get(stream, 4);

		blockTypeIndices.resize(numBlocks);
		for (int i = 0; i < numBlocks; i++)
			stream >> blockTypeIndices[i];
	}

	if (version.File() >= V20_2_0_5) {
		blockSizes.resize(numBlocks);
		for (int i = 0; i < numBlocks; i++)
			stream >> blockSizes[i];
	}

	if (version.File() >= V20_1_0_1) {
		stream >> numStrings;
		stream >> maxStringLen;

		strings.resize(numStrings);
		for (int i = 0; i < numStrings; i++)
			strings[i].Get(stream, 4);
	}

	if (version.File() >= NiVersion::ToFile(5, 0, 0, 6)) {
		stream >> numGroups;
		groupSizes.resize(numGroups);
		for (int i = 0; i < numGroups; i++)
			stream >> groupSizes[i];
	}

	valid = true;
}

void NiHeader::Put(NiStream& stream) {
	std::string ver = version.String();
	stream.write(ver.data(), ver.size());

	// Newline to end header string
	stream << byte(0x0A);

	bool isNDS = version.NDS() != 0;
	if (version.File() > V3_1 && !isNDS) {
		stream << version.File();
	}
	else if (isNDS) {
		stream << version.NDS();
	}
	else {
		stream.writeline(copyright1.data(), copyright1.size());
		stream.writeline(copyright2.data(), copyright2.size());
		stream.writeline(copyright3.data(), copyright3.size());
	}

	if (version.File() >= NiVersion::ToFile(20, 0, 0, 3))
		stream << endian;

	if (version.File() >= NiVersion::ToFile(10, 0, 1, 8))
		stream << version.User();

	stream << numBlocks;

	if (version.IsBethesda()) {
		stream << version.Stream();

		creator.Put(stream, 1);
		exportInfo1.Put(stream, 1);
		exportInfo2.Put(stream, 1);
		if (version.Stream() >= 130)
			exportInfo3.Put(stream, 1);
	}
	else if (version.File() >= V30_0_0_2) {
		stream << embedDataSize;
		for (int i = 0; i < embedDataSize; i++)
			stream << embedData[i];
	}

	if (version.File() >= V5_0_0_1) {
		stream << numBlockTypes;
		for (int i = 0; i < numBlockTypes; i++)
			blockTypes[i].Put(stream, 4, false);

		for (int i = 0; i < numBlocks; i++)
			stream << blockTypeIndices[i];
	}

	if (version.File() >= V20_2_0_5) {
		blockSizePos = stream.tellp();
		for (int i = 0; i < numBlocks; i++)
			stream << blockSizes[i];
	}

	if (version.File() >= V20_1_0_1) {
		stream << numStrings;
		stream << maxStringLen;
		for (int i = 0; i < numStrings; i++)
			strings[i].Put(stream, 4, false);
	}

	if (version.File() >= NiVersion::ToFile(5, 0, 0, 6)) {
		stream << numGroups;
		for (int i = 0; i < numGroups; i++)
			stream << groupSizes[i];
	}
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
