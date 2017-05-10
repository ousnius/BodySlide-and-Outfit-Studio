/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "utils/Object3d.h"

#include <set>
#include <fstream>
#include <string>
#include <algorithm>

#pragma warning (disable : 4100)

class NiHeader;

class StringRef {
private:
	int ref = 0xFFFFFFFF;

public:
	std::string GetString(NiHeader* hdr);
	void SetString(NiHeader* hdr, const std::string& str);
	void RenameString(NiHeader* hdr, const std::string& str);
	void Clear(NiHeader* hdr);
	void Put(std::fstream& file);
	void Get(std::fstream& file, NiHeader* hdr);
	void notifyStringDelete(int stringID);
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

	void operator=(const BlockRef& other) {
		index = other.index;
	}

	void Get(std::fstream& file) {
		file.read((char*)&index, 4);
	}

	void Put(std::fstream& file) {
		file.write((char*)&index, 4);
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

	virtual void Get(std::fstream& file) = 0;
	virtual void Put(std::fstream& file) = 0;
	virtual void AddBlockRef(const int id) = 0;
	virtual int GetBlockRef(const int id) = 0;
	virtual void SetBlockRef(const int id, const int index) = 0;
	virtual void RemoveBlockRef(const int id) = 0;
	virtual std::vector<int> GetIndices() = 0;
	virtual void GetIndexPtrs(std::set<int*>& indices) = 0;
	virtual void SetIndices(const std::vector<int>& indices) = 0;
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

	std::vector<BlockRef<T>>& GetRefs() {
		return refs;
	}

	virtual void Get(std::fstream& file) override {
		file.read((char*)&arraySize, 4);
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(file);
	}

	virtual void Put(std::fstream& file) override {
		CleanInvalidRefs();

		file.write((char*)&arraySize, 4);

		for (auto &r : refs)
			r.Put(file);
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
};

template <typename T>
class BlockRefShortArray : public BlockRefArray<T> {
public:
	virtual void Get(std::fstream& file) override {
		file.read((char*)&arraySize, 2);
		refs.resize(arraySize);

		for (auto &r : refs)
			r.Get(file);
	}

	virtual void Put(std::fstream& file) override {
		file.write((char*)&arraySize, 2);

		for (auto &r : refs)
			r.Put(file);
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

	void Put(std::fstream& file, const int szSize, const bool wantNullOutput = true);
	void Get(std::fstream& file, const int szSize);
};

class NiObject {
protected:
	uint blockSize = 0;

public:
	NiHeader* header = nullptr;

	virtual ~NiObject() {}

	static constexpr const char* BlockName = "NiUnknown";
	virtual const char* GetBlockName() { return BlockName; }

	virtual void Init(NiHeader* hdr) {
		header = hdr;
	}

	virtual void notifyVerticesDelete(const std::vector<ushort>) {}
	virtual void notifyStringDelete(int) {}

	virtual void Get(std::fstream&) {}
	virtual void Put(std::fstream&) {}

	virtual void GetChildRefs(std::set<int*>&) {}

	virtual int CalcBlockSize() {
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

	char verStr[0x26];
	byte version1;
	byte version2;
	byte version3;
	byte version4;
	uint userVersion;
	uint userVersion2;

	byte unk1;
	byte endian;

	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
	NiString exportInfo3;

	// Foreign reference to the blocks list in NifFile.
	std::vector<NiObject*>* blocks;

	uint numBlocks;
	ushort numBlockTypes;
	std::vector<NiString> blockTypes;
	std::vector<ushort> blockTypeIndices;
	std::vector<uint> blockSizes;

	uint numStrings;
	uint maxStringLen;
	std::vector<NiString> strings;
	std::vector<int> stringRefCount;

	uint unkInt2;

public:
	NiHeader();

	static constexpr const char* BlockName = "NiHeader";
	virtual const char* GetBlockName() { return BlockName; }

	void Clear();
	bool IsValid() { return valid; }

	std::string GetVersionInfo();
	void SetVersion(const byte v1, const byte v2, const byte v3, const byte v4, const uint userVer, const uint userVer2);
	bool VerCheck(const int v1, const int v2, const int v3, const int v4, const bool equal = false);

	uint GetUserVersion() { return userVersion; };
	uint GetUserVersion2() { return userVersion2; };

	std::string GetCreatorInfo();
	void SetCreatorInfo(const std::string& creatorInfo);

	std::string GetExportInfo();
	void SetExportInfo(const std::string& exportInfo);

	void SetBlockReference(std::vector<NiObject*>* blockRef) {
		blocks = blockRef;
	};

	uint GetNumBlocks() { return numBlocks; }

	template <class T>
	T* GetBlock(const int blockId) {
		if (blockId >= 0 && blockId < numBlocks)
			return dynamic_cast<T*>((*blocks)[blockId]);

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

	void AddStringRef(const int id);
	void RemoveStringRef(const int id);
	int RemoveUnusedStrings();

	static void BlockDeleted(NiObject* o, int blockId);
	static void BlockSwapped(NiObject* o, int blockIndexLo, int blockIndexHi);

	void Get(std::fstream& file);
	void Put(std::fstream& file);
};

class NiUnknown : public NiObject {
public:
	std::vector<char> data;

	NiUnknown() {}
	NiUnknown(std::fstream& file, const uint size);
	NiUnknown(const uint size);

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize() { return blockSize; }
	NiUnknown* Clone() { return new NiUnknown(*this); }
};
