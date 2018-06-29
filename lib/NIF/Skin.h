/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "VertexData.h"

struct SkinWeight {
	ushort index;
	float weight;

	SkinWeight(const ushort index = 0, const float weight = 0.0f) {
		this->index = index;
		this->weight = weight;
	}
};

struct VertexWeight {
	float w1 = 0.0f;
	float w2 = 0.0f;
	float w3 = 0.0f;
	float w4 = 0.0f;
};

struct BoneIndices {
	byte i1 = 0;
	byte i2 = 0;
	byte i3 = 0;
	byte i4 = 0;
};

class NiSkinData : public NiObject {
public:
	struct BoneData {
		MatTransform boneTransform;
		BoundingSphere bounds;
		ushort numVertices = 0;
		std::vector<SkinWeight> vertexWeights;
	};

	MatTransform skinTransform;
	uint numBones = 0;
	byte hasVertWeights = 1;
	std::vector<BoneData> bones;

	static constexpr const char* BlockName = "NiSkinData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	NiSkinData* Clone() { return new NiSkinData(*this); }
};

class NiSkinPartition : public NiObject {
public:
	struct PartitionBlock {
		ushort numVertices = 0;
		ushort numTriangles = 0;
		ushort numBones = 0;
		ushort numStrips = 0;
		ushort numWeightsPerVertex = 0;
		std::vector<ushort> bones;
		bool hasVertexMap = false;
		std::vector<ushort> vertexMap;
		bool hasVertexWeights = false;
		std::vector<VertexWeight> vertexWeights;
		std::vector<ushort> stripLengths;
		bool hasFaces = false;
		std::vector<std::vector<ushort>> strips;
		std::vector<Triangle> triangles;
		bool hasBoneIndices = false;
		std::vector<BoneIndices> boneIndices;

		ushort unkShort = 0;					// User Version >= 12
		VertexDesc vertexDesc;					// User Version >= 12, User Version 2 == 100
		std::vector<Triangle> trueTriangles;	// User Version >= 12, User Version 2 == 100
	};

	uint numPartitions = 0;
	uint dataSize = 0;						// User Version >= 12, User Version 2 == 100
	uint vertexSize = 0;					// User Version >= 12, User Version 2 == 100
	VertexDesc vertexDesc;					// User Version >= 12, User Version 2 == 100

	uint numVertices = 0;					// Not in file
	std::vector<BSVertexData> vertData;		// User Version >= 12, User Version 2 == 100
	std::vector<PartitionBlock> partitions;

	bool HasVertices() { return vertexDesc.HasFlag(VF_VERTEX); }
	bool HasUVs() { return vertexDesc.HasFlag(VF_UV); }
	bool HasNormals() { return vertexDesc.HasFlag(VF_NORMAL); }
	bool HasTangents() { return vertexDesc.HasFlag(VF_TANGENT); }
	bool HasVertexColors() { return vertexDesc.HasFlag(VF_COLORS); }
	bool IsSkinned() { return vertexDesc.HasFlag(VF_SKINNED); }
	bool HasEyeData() { return vertexDesc.HasFlag(VF_EYEDATA); }
	bool IsFullPrecision() { return true; }

	static constexpr const char* BlockName = "NiSkinPartition";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(std::vector<int>& outDeletedIndices);
	NiSkinPartition* Clone() { return new NiSkinPartition(*this); }
};

class NiNode;

class NiBoneContainer : public NiObject {
protected:
	BlockRefArray<NiNode> boneRefs;

public:
	BlockRefArray<NiNode>& GetBones();
};

class NiSkinInstance : public NiBoneContainer {
private:
	BlockRef<NiSkinData> dataRef;
	BlockRef<NiSkinPartition> skinPartitionRef;
	BlockRef<NiNode> targetRef;

public:
	static constexpr const char* BlockName = "NiSkinInstance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
	NiSkinInstance* Clone() { return new NiSkinInstance(*this); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(const int datRef) { dataRef.SetIndex(datRef); }

	int GetSkinPartitionRef() { return skinPartitionRef.GetIndex(); }
	void SetSkinPartitionRef(const int skinPartRef) { skinPartitionRef.SetIndex(skinPartRef); }

	int GetSkeletonRootRef() { return targetRef.GetIndex(); }
	void SetSkeletonRootRef(const int skelRootRef) { targetRef.SetIndex(skelRootRef); }
};


enum PartitionFlags : ushort {
	PF_NONE = 0,
	PF_EDITOR_VISIBLE = 1 << 0,
	PF_START_NET_BONESET = 1 << 8
};

class BSDismemberSkinInstance : public NiSkinInstance {
public:
	struct PartitionInfo {
		PartitionFlags flags = PF_NONE;
		ushort partID;
	};

private:
	int numPartitions = 0;
	std::vector<PartitionInfo> partitions;

public:
	static constexpr const char* BlockName = "BSDismemberSkinInstance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSDismemberSkinInstance* Clone() { return new BSDismemberSkinInstance(*this); }

	int GetNumPartitions() { return numPartitions; }
	std::vector<PartitionInfo> GetPartitions() { return partitions; }

	void AddPartition(const PartitionInfo& partition);
	void RemovePartition(const int id);
	void ClearPartitions();

	void SetPartitions(const std::vector<PartitionInfo>& parts) {
		this->partitions = parts;
		this->numPartitions = parts.size();
	}
};

class BSSkinBoneData : public NiObject {
public:
	uint nBones = 0;

	struct BoneData {
		BoundingSphere bounds;
		MatTransform boneTransform;
	};

	std::vector<BoneData> boneXforms;

	static constexpr const char* BlockName = "BSSkin::BoneData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSSkinBoneData* Clone() { return new BSSkinBoneData(*this); }
};

class NiAVObject;

class BSSkinInstance : public NiBoneContainer {
private:
	BlockRef<NiAVObject> targetRef;
	BlockRef<BSSkinBoneData> dataRef;

	uint numScales = 0;
	std::vector<Vector3> scales;

public:
	static constexpr const char* BlockName = "BSSkin::Instance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);
	void GetPtrs(std::set<Ref*>& ptrs);
	BSSkinInstance* Clone() { return new BSSkinInstance(*this); }

	int GetTargetRef() { return targetRef.GetIndex(); }
	void SetTargetRef(const int targRef) { targetRef.SetIndex(targRef); }

	int GetDataRef() { return dataRef.GetIndex(); }
	void SetDataRef(const int datRef) { dataRef.SetIndex(datRef); }
};
