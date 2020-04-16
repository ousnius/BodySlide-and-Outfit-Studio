/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "utils/Object3d.h"

#include <set>
#include <streambuf>
#include <string>
#include <algorithm>
#include <memory>
#include <iostream>

enum NiFileVersion : uint {
	V2_3 = 0x02030000,
	V3_0 = 0x03000000,
	V3_03 = 0x03000300,
	V3_1 = 0x03010000,
	V3_3_0_13 = 0x0303000D,
	V4_0_0_0 = 0x04000000,
	V4_0_0_2 = 0x04000002,
	V4_1_0_12 = 0x0401000C,
	V4_2_0_2 = 0x04020002,
	V4_2_1_0 = 0x04020100,
	V4_2_2_0 = 0x04020200,
	V5_0_0_1 = 0x05000001,
	V10_0_0_0 = 0x0A000000,
	V10_0_1_0 = 0x0A000100,
	V10_0_1_2 = 0x0A000102,
	V10_0_1_3 = 0x0A000103,
	V10_1_0_0 = 0x0A010000,
	V10_1_0_101 = 0x0A010065,
	V10_1_0_104 = 0x0A010068,
	V10_1_0_106 = 0x0A01006A,
	V10_1_0_114 = 0x0A010072,
	V10_2_0_0 = 0x0A020000,
	V10_2_0_1 = 0x0A020001,
	V10_3_0_1 = 0x0A030001,
	V10_4_0_1 = 0x0A040001,
	V20_0_0_4 = 0x14000004,
	V20_0_0_5 = 0x14000005,
	V20_1_0_1 = 0x14010001,
	V20_1_0_3 = 0x14010003,
	V20_2_0_5 = 0x14020005,
	V20_2_0_7 = 0x14020007,
	V20_2_0_8 = 0x14020008,
	V20_3_0_1 = 0x14030001,
	V20_3_0_2 = 0x14030002,
	V20_3_0_3 = 0x14030003,
	V20_3_0_6 = 0x14030006,
	V20_3_0_9 = 0x14030009,
	V20_5_0_0 = 0x14050000,
	V20_6_0_0 = 0x14060000,
	V20_6_5_0 = 0x14060500,
	V30_0_0_2 = 0x1E000002,
	V30_1_0_3 = 0x1E010003,
	UNKNOWN = 0xFFFFFFFF
};

class NiVersion {
private:
	std::string vstr;
	NiFileVersion file = UNKNOWN;
	uint user = 0;
	uint stream = 0;
	uint nds = 0;

public:
	// Construct a file version enumeration from individual values
	static NiFileVersion ToFile(byte major, byte minor, byte patch, byte internal) {
		return NiFileVersion((major << 24) | (minor << 16) | (patch << 8) | internal);
	}

	// Return file version as individual values
	static std::vector<byte> ToArray(NiFileVersion file) {
		return { byte(file >> 24), byte(file >> 16), byte(file >> 8), byte(file) };
	}

	std::string GetVersionInfo();
	std::string String() { return vstr; }

	NiFileVersion File() { return file; }
	void SetFile(NiFileVersion fileVer);

	uint User() { return user; }
	void SetUser(const uint userVer) { user = userVer; }

	uint Stream() { return stream; }
	void SetStream(const uint streamVer) { stream = streamVer; }

	uint NDS() { return nds; }
	void SetNDS(const uint ndsVer) { nds = ndsVer; }

	bool IsBethesda() { return (file == V20_2_0_7 && user >= 11) || IsOB(); }

	bool IsOB() {
		return ((file == V10_1_0_106 || file == V10_2_0_0) && user >= 3 && user < 11)
			|| (file == V20_0_0_4 && (user == 10 || user == 11))
			|| (file == V20_0_0_5 && user == 11);
	}

	bool IsFO3() { return file == V20_2_0_7 && stream > 11 && stream < 83; }
	bool IsSK() { return file == V20_2_0_7 && stream == 83; }
	bool IsSSE() { return file == V20_2_0_7 && stream == 100; }
	bool IsFO4() { return file == V20_2_0_7 && stream == 130; }
};

enum NiEndian : byte {
	ENDIAN_BIG,
	ENDIAN_LITTLE
};

class NiStream {
private:
	std::iostream* stream = nullptr;
	NiVersion* version = nullptr;
	int blockSize = 0;

public:
	NiStream(std::iostream* stream, NiVersion* version) {
		this->stream = stream;
		this->version = version;
	}

	void write(const char* ptr, std::streamsize count) {
		stream->write(ptr, count);
		blockSize += count;
	}

	void writeline(const char* ptr, std::streamsize count) {
		stream->write(ptr, count);
		stream->write("\n", 1);
		blockSize += count + 1;
	}

	void read(char* ptr, std::streamsize count) {
		stream->read(ptr, count);
	}

	void getline(char* ptr, std::streamsize maxCount) {
		stream->getline(ptr, maxCount);
	}

	std::streampos tellp() {
		return stream->tellp();
	}

	// Be careful with sizes of structs and classes
	template<typename T>
	NiStream& operator<<(const T& t) {
		write((const char*)&t, sizeof(T));
		return *this;
	}

	// Be careful with sizes of structs and classes
	template<typename T>
	NiStream& operator>>(T& t) {
		read((char*)&t, sizeof(T));
		return *this;
	}

	void InitBlockSize() {
		blockSize = 0;
	}

	int GetBlockSize() {
		return blockSize;
	}

	NiVersion& GetVersion() {
		return *version;
	}
};

class NiString {
private:
	std::string str;

public:
	NiString() {};

	std::string GetString() {
		return str;
	}

	void SetString(const std::string& s) {
		this->str = s;
	}

	size_t GetLength() {
		return str.length();
	}

	void Clear() {
		str.clear();
	}

	void Get(NiStream& stream, const int szSize);
	void Put(NiStream& stream, const int szSize, const bool wantNullOutput = true);
};

class StringRef {
private:
	// Temporary index storage for load/save
	int index = 0xFFFFFFFF;
	NiString str;

public:
	std::string GetString() {
		return str.GetString();
	}

	void SetString(const std::string& s) {
		str.SetString(s);
	}

	int GetIndex() {
		return index;
	}

	void SetIndex(const int id) {
		index = id;
	}

	void Clear() {
		index = 0xFFFFFFFF;
		str.Clear();
	}

	void Get(NiStream& stream) {
		if (stream.GetVersion().File() < V20_1_0_1)
			str.Get(stream, 4);
		else
			stream >> index;
	}

	void Put(NiStream& stream) {
		if (stream.GetVersion().File() < V20_1_0_1)
			str.Put(stream, 4, false);
		else
			stream << index;
	}
};

class Ref {
protected:
	int index = 0xFFFFFFFF;

public:
	int GetIndex() {
		return index;
	}

	void SetIndex(const int id) {
		index = id;
	}

	void Clear() {
		index = 0xFFFFFFFF;
	}
};

template <typename T>
class BlockRef : public Ref {
public:
	BlockRef() {}
	BlockRef(const int id) {
		index = id;
	}

	void Get(NiStream& stream) {
		stream >> index;
	}

	void Put(NiStream& stream) {
		stream << index;
	}
};

class RefArray {
protected:
	int arraySize = 0;

public:
	RefArray() {}

	int GetSize() {
		return arraySize;
	}

	virtual void Get(NiStream& stream) = 0;
	virtual void Put(NiStream& stream) = 0;
	virtual void Put(NiStream& stream, const int forcedSize) = 0;
	virtual void AddBlockRef(const int id) = 0;
	virtual int GetBlockRef(const int id) = 0;
	virtual void SetBlockRef(const int id, const int index) = 0;
	virtual void RemoveBlockRef(const int id) = 0;
	virtual std::vector<int> GetIndices() = 0;
	virtual void GetIndexPtrs(std::set<Ref*>& indices) = 0;
	virtual void SetIndices(const std::vector<int>& indices) = 0;
};

template <typename T>
class BlockRefArray : public RefArray {
protected:
	std::vector<BlockRef<T>> refs;

	void CleanInvalidRefs() {
		refs.erase(std::remove_if(refs.begin(), refs.end(), [](BlockRef<T> r) {
			if (r.GetIndex() == 0xFFFFFFFF)
				return true;
			else
				return false;
		}), refs.end());

		arraySize = refs.size();
	}

public:
	typedef typename std::vector<BlockRef<T>>::iterator iterator;
	typedef typename std::vector<BlockRef<T>>::const_iterator const_iterator;

	typename std::vector<BlockRef<T>>::iterator begin() {
		return refs.begin();
	}

	typename std::vector<BlockRef<T>>::iterator end() {
		return refs.end();
	}

	typename std::vector<BlockRef<T>>::const_iterator begin() const {
		return refs.begin();
	}

	typename std::vector<BlockRef<T>>::const_iterator end() const {
		return refs.end();
	}

	void Clear() {
		refs.clear();
		arraySize = 0;
	}

	virtual void Get(NiStream& stream) override {
		stream >> arraySize;
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(stream);
	}

	virtual void Put(NiStream& stream) override {
		CleanInvalidRefs();
		stream << arraySize;

		for (auto &r : refs)
			r.Put(stream);
	}

	virtual void Put(NiStream& stream, const int forcedSize) override {
		CleanInvalidRefs();
		arraySize = forcedSize;
		refs.resize(forcedSize);

		stream << arraySize;

		for (auto &r : refs)
			r.Put(stream);
	}

	virtual void AddBlockRef(const int index) override {
		refs.push_back(BlockRef<T>(index));
		arraySize++;
	}

	virtual int GetBlockRef(const int id) override {
		if (id >= 0 && refs.size() > id)
			return refs[id].GetIndex();

		return 0xFFFFFFFF;
	}

	virtual void SetBlockRef(const int id, const int index) override {
		if (id >= 0 && refs.size() > id)
			refs[id].SetIndex(index);
	}

	virtual void RemoveBlockRef(const int id) override {
		if (id >= 0 && refs.size() > id) {
			refs.erase(refs.begin() + id);
			arraySize--;
		}
	}

	virtual std::vector<int> GetIndices() override {
		std::vector<int> indices;
		indices.reserve(arraySize);
		for (auto &r : refs)
			indices.push_back(r.GetIndex());

		return indices;
	}

	virtual void GetIndexPtrs(std::set<Ref*>& indices) override {
		for (auto &r : refs)
			indices.insert(&r);
	}

	virtual void SetIndices(const std::vector<int>& indices) override {
		arraySize = indices.size();
		refs.resize(arraySize);

		for (int i = 0; i < arraySize; i++)
			refs[i].SetIndex(indices[i]);
	}
};

template <typename T>
class BlockRefShortArray : public BlockRefArray<T> {
public:
	typedef BlockRefArray<T> base;
	using base::arraySize;
	using base::refs;
	virtual void Get(NiStream& stream) override {
		stream.read((char*)&arraySize, 2);
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(stream);
	}

	virtual void Put(NiStream& stream) override {
		base::CleanInvalidRefs();
		stream.write((char*)&arraySize, 2);

		for (auto &r : refs)
			r.Put(stream);
	}
	
	virtual void Put(NiStream& stream, const int forcedSize) override {
		base::CleanInvalidRefs();
		arraySize = forcedSize;
		refs.resize(forcedSize);

		stream.write((char*)&arraySize, 2);

		for (auto &r : refs)
			r.Put(stream);
	}
};

class NiObject {
protected:
	uint blockSize = 0;

public:
	virtual ~NiObject() {}

	static constexpr const char* BlockName = "NiUnknown";
	virtual const char* GetBlockName() { return BlockName; }

	virtual void notifyVerticesDelete(const std::vector<ushort>&) {}

	virtual void Get(NiStream&) {}
	virtual void Put(NiStream&) {}

	virtual void GetStringRefs(std::set<StringRef*>&) {}
	virtual void GetChildRefs(std::set<Ref*>&) {}
	virtual void GetPtrs(std::set<Ref*>&) {}

	virtual NiObject* Clone() { return new NiObject(*this); }

	template <typename T>
	bool HasType() {
		return dynamic_cast<T*>(this) != nullptr;
	}
};

class NiHeader : public NiObject {
	/*
	Minimum supported
	Version:			20.2.0.7
	User Version:		11
	User Version 2:		26

	Maximum supported
	Version:			20.2.0.7
	User Version:		12
	User Version 2:		130
	*/

private:
	bool valid = false;
	std::streampos blockSizePos;

	NiVersion version;
	NiEndian endian = ENDIAN_LITTLE;
	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
	NiString exportInfo3;

	std::string copyright1;
	std::string copyright2;
	std::string copyright3;

	uint embedDataSize = 0;
	std::vector<byte> embedData;

	// Foreign reference to the blocks list in NifFile.
	std::vector<std::unique_ptr<NiObject>>* blocks = nullptr;

	uint numBlocks = 0;
	ushort numBlockTypes = 0;
	std::vector<NiString> blockTypes;
	std::vector<ushort> blockTypeIndices;
	std::vector<uint> blockSizes;

	uint numStrings = 0;
	uint maxStringLen = 0;
	std::vector<NiString> strings;

	uint numGroups = 0;
	std::vector<uint> groupSizes;

public:
	NiHeader() {};

	static constexpr const char* BlockName = "NiHeader";
	virtual const char* GetBlockName() { return BlockName; }

	void Clear();

	bool IsValid() {
		return valid;
	}

	NiVersion& GetVersion() {
		return version;
	};

	void SetVersion(const NiVersion& ver) {
		version = ver;
	}

	std::string GetCreatorInfo();
	void SetCreatorInfo(const std::string& creatorInfo);

	std::string GetExportInfo();
	void SetExportInfo(const std::string& exportInfo);

	void SetBlockReference(std::vector<std::unique_ptr<NiObject>>* blockRef) {
		blocks = blockRef;
	};

	uint GetNumBlocks() {
		return numBlocks;
	}

	template <class T>
	T* GetBlock(const int blockId) {
		if (blockId >= 0 && blockId < numBlocks)
			return dynamic_cast<T*>((*blocks)[blockId].get());

		return nullptr;
	}

	void DeleteBlock(int blockId);
	void DeleteBlockByType(const std::string& blockTypeStr, const bool orphanedOnly = false);
	int AddBlock(NiObject* newBlock);
	int ReplaceBlock(int oldBlockId, NiObject* newBlock);
	void SetBlockOrder(std::vector<std::pair<int, int>>& newIndices);

	// Swaps two blocks, updating references in other blocks that may refer to their old indices
	void SwapBlocks(const int blockIndexLo, const int blockIndexHi);
	bool IsBlockReferenced(const int blockId);
	int GetBlockRefCount(const int blockId);

	template <class T>
	bool DeleteUnreferencedBlocks(const int rootId, int* deletionCount = nullptr) {
		if (rootId == 0xFFFFFFFF)
			return false;

		for (int i = 0; i < numBlocks; i++) {
			if (i != rootId) {
				// Only check blocks of provided template type
				auto block = GetBlock<T>(i);
				if (block && !IsBlockReferenced(i)) {
					DeleteBlock(i);

					if (deletionCount)
						(*deletionCount)++;

					// Deleting a block can cause others to become unreferenced
					return DeleteUnreferencedBlocks<T>(rootId > i ? rootId - 1 : rootId, deletionCount);
				}
			}
		}

		return true;
	}

	ushort AddOrFindBlockTypeId(const std::string& blockTypeName);
	std::string GetBlockTypeStringById(const int blockId);
	ushort GetBlockTypeIndex(const int blockId);

	uint GetBlockSize(const uint blockId);
	std::streampos GetBlockSizeStreamPos();
	void ResetBlockSizeStreamPos();

	int GetStringCount();
	int FindStringId(const std::string& str);
	int AddOrFindStringId(const std::string& str, const bool addEmpty = false);
	std::string GetStringById(const int id);
	void SetStringById(const int id, const std::string& str);

	void ClearStrings();
	void UpdateMaxStringLength();
	void FillStringRefs();
	void UpdateHeaderStrings(const bool hasUnknown);

	static void BlockDeleted(NiObject* o, int blockId);
	static void BlockSwapped(NiObject* o, int blockIndexLo, int blockIndexHi);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class NiUnknown : public NiObject {
private:
	std::vector<char> data;

public:
	NiUnknown() {}
	NiUnknown(NiStream& stream, const uint size);
	NiUnknown(const uint size);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiUnknown* Clone() { return new NiUnknown(*this); }

	std::vector<char> GetData() { return data; }
};
