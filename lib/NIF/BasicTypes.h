/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "utils/Object3d.h"

#include <set>
#include <streambuf>
#include <string>
#include <algorithm>
#include <memory>

#pragma warning (disable : 4100)

class NiVersion {
private:
	std::string vstr;
	uint vfile;
	uint vuser;
	uint vuser2;

public:
	static uint Get(byte v1, byte v2, byte v3, byte v4) {
		return (v1 << 24) | (v2 << 16) | (v3 << 8) | v4;
	}

	std::string GetVersionInfo();
	void Set(byte v1, byte v2, byte v3, byte v4, uint userVer, uint userVer2);

	std::string String() { return vstr; }
	uint File() { return vfile; }
	uint User() { return vuser; }
	uint User2() { return vuser2; }
};

class NiStream {
private:
	std::iostream* stream = nullptr;
	NiVersion* version = nullptr;

public:
	NiStream(std::iostream* stream, NiVersion* version) {
		this->stream = stream;
		this->version = version;
	}

	void write(const char* ptr, std::streamsize count) {
		stream->write(ptr, count);
	}

	void read(char* ptr, std::streamsize count) {
		stream->read(ptr, count);
	}

	void getline(char* ptr, std::streamsize maxCount) {
		stream->getline(ptr, maxCount);
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

	NiVersion& GetVersion() { return *version; }
};

class StringRef {
private:
	// Temporary index storage for load/save
	int index = 0xFFFFFFFF;
	std::string str;

public:
	std::string GetString() { return str; }
	void SetString(const std::string& s) { str = s; }

	int GetIndex() { return index; }
	void SetIndex(const int id) { index = id; }

	void Clear() {
		index = 0xFFFFFFFF;
		str.clear();
	}

	void Get(NiStream& stream) {
		stream >> index;
	}

	void Put(NiStream& stream) {
		stream << index;
	}
};

class Ref {
public:
	int index = 0xFFFFFFFF;
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
	virtual void AddBlockRef(const int id) = 0;
	virtual int GetBlockRef(const int id) = 0;
	virtual void SetBlockRef(const int id, const int index) = 0;
	virtual void RemoveBlockRef(const int id) = 0;
	virtual std::vector<int> GetIndices() = 0;
	virtual void GetIndexPtrs(std::set<int*>& indices) = 0;
	virtual void SetIndices(const std::vector<int>& indices) = 0;
	virtual int CalcBlockSize() = 0;
};

template <typename T>
class BlockRefArray : public RefArray {
protected:
	std::vector<BlockRef<T>> refs;

	void CleanInvalidRefs() {
		refs.erase(std::remove_if(refs.begin(), refs.end(), [](BlockRef<T> r) {
			if (r.index == 0xFFFFFFFF)
				return true;
			else
				return false;
		}), refs.end());

		arraySize = refs.size();
	}

public:
	void Clear() {
		refs.clear();
		arraySize = 0;
	}

	std::vector<BlockRef<T>>& GetBlockRefs() {
		return refs;
	}

	virtual void Get(NiStream& stream) override {
		stream >> arraySize;
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(stream);
	}

	virtual void Put(NiStream& stream) override {
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
			return refs[id].index;

		return 0xFFFFFFFF;
	}

	virtual void SetBlockRef(const int id, const int index) override {
		if (id >= 0 && refs.size() > id)
			refs[id].index = index;
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
			indices.push_back(r.index);

		return indices;
	}

	virtual void GetIndexPtrs(std::set<int*>& indices) override {
		for (auto &r : refs)
			indices.insert(&r.index);
	}

	virtual void SetIndices(const std::vector<int>& indices) override {
		arraySize = indices.size();
		refs.resize(arraySize);

		for (int i = 0; i < arraySize; i++)
			refs[i].index = indices[i];
	}

	virtual int CalcBlockSize() override {
		CleanInvalidRefs();
		return 4 + arraySize * 4;
	}
};

template <typename T>
class BlockRefShortArray : public BlockRefArray<T> {
public:
	virtual void Get(NiStream& stream) override {
		stream.read((char*)&arraySize, 2);
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(stream);
	}

	virtual void Put(NiStream& stream) override {
		stream.write((char*)&arraySize, 2);

		for (auto &r : refs)
			r.Put(stream);
	}

	virtual int CalcBlockSize() override {
		CleanInvalidRefs();
		return 2 + arraySize * 4;
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

class NiObject {
protected:
	uint blockSize = 0;

public:
	virtual ~NiObject() {}

	static constexpr const char* BlockName = "NiUnknown";
	virtual const char* GetBlockName() { return BlockName; }

	virtual void Init() {}

	virtual void notifyVerticesDelete(const std::vector<ushort>&) {}

	virtual void Get(NiStream&) {}
	virtual void Put(NiStream&) {}

	virtual void GetStringRefs(std::set<StringRef*>&) {}
	virtual void GetChildRefs(std::set<int*>&) {}
	virtual void GetPtrs(std::set<int*>&) {}

	virtual int CalcBlockSize(NiVersion& version) {
		blockSize = 0;	// Calculate from the ground up
		return blockSize;
	}

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
	bool valid;
	NiVersion version;
	byte endian;
	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
	NiString exportInfo3;

	// Foreign reference to the blocks list in NifFile.
	std::vector<std::unique_ptr<NiObject>>* blocks;

	uint numBlocks;
	ushort numBlockTypes;
	std::vector<NiString> blockTypes;
	std::vector<ushort> blockTypeIndices;
	std::vector<uint> blockSizes;

	uint numStrings;
	uint maxStringLen;
	std::vector<NiString> strings;

	uint unkInt2;

public:
	NiHeader();

	static constexpr const char* BlockName = "NiHeader";
	virtual const char* GetBlockName() { return BlockName; }

	void Clear();
	bool IsValid() { return valid; }

	NiVersion& GetVersion() { return version; };

	std::string GetCreatorInfo();
	void SetCreatorInfo(const std::string& creatorInfo);

	std::string GetExportInfo();
	void SetExportInfo(const std::string& exportInfo);

	void SetBlockReference(std::vector<std::unique_ptr<NiObject>>* blockRef) {
		blocks = blockRef;
	};

	uint GetNumBlocks() { return numBlocks; }

	template <class T>
	T* GetBlock(const int blockId) {
		if (blockId >= 0 && blockId < numBlocks)
			return dynamic_cast<T*>((*blocks)[blockId].get());

		return nullptr;
	}

	void DeleteBlock(int blockId);
	void DeleteBlockByType(const std::string& blockTypeStr);
	int AddBlock(NiObject* newBlock);
	int ReplaceBlock(int oldBlockId, NiObject* newBlock);

	// Swaps two blocks, updating references in other blocks that may refer to their old indices
	void SwapBlocks(const int blockIndexLo, const int blockIndexHi);
	bool IsBlockReferenced(const int blockId);
	void DeleteUnreferencedBlocks(bool* hadDeletions = nullptr);

	ushort AddOrFindBlockTypeId(const std::string& blockTypeName);
	std::string GetBlockTypeStringById(const int id);
	ushort GetBlockTypeIndex(const int id);

	uint GetBlockSize(const uint blockId) { return blockSizes[blockId]; }
	void CalcAllBlockSizes();

	int GetStringCount() {
		return strings.size();
	}

	int FindStringId(const std::string& str);
	int AddOrFindStringId(const std::string& str);
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
public:
	std::vector<char> data;

	NiUnknown() {}
	NiUnknown(NiStream& stream, const uint size);
	NiUnknown(const uint size);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version) { return blockSize; }
	NiUnknown* Clone() { return new NiUnknown(*this); }
};
