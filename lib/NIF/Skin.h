/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "VertexData.h"

struct SkinTransform {
	Vector3 rotation[3];
	Vector3 translation;
	float scale;

	SkinTransform() {
		translation = Vector3();
		scale = 1.0f;
		rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}

	bool IsIdentity() {
		return (translation.IsZero() && scale == 1.0f &&
			rotation[0] == Vector3(1.0f, 0.0f, 0.0f) &&
			rotation[1] == Vector3(0.0f, 1.0f, 0.0f) &&
			rotation[2] == Vector3(0.0f, 0.0f, 1.0f));
	}

	Matrix4 ToMatrix() {
		Matrix4 m;
		m[0] = rotation[0].x * scale;
		m[1] = rotation[0].y;
		m[2] = rotation[0].z;
		m[3] = translation.x;
		m[4] = rotation[1].x;
		m[5] = rotation[1].y * scale;
		m[6] = rotation[1].z;
		m[7] = translation.y;
		m[8] = rotation[2].x;
		m[9] = rotation[2].y;
		m[10] = rotation[2].z * scale;
		m[11] = translation.z;
		return m;
	}

	Matrix4 ToMatrixSkin() {
		Matrix4 m;
		// Set the rotation
		Vector3 r1 = rotation[0];
		Vector3 r2 = rotation[1];
		Vector3 r3 = rotation[2];
		m.SetRow(0, r1);
		m.SetRow(1, r2);
		m.SetRow(2, r3);
		// Set the scale
		m.Scale(scale, scale, scale);
		// Set the translation
		m[3] = translation.x;
		m[7] = translation.y;
		m[11] = translation.z;
		return m;
	}
};

struct SkinWeight {
	ushort index;
	float weight;

	SkinWeight(const ushort index = 0, const float weight = 0.0f) {
		this->index = index;
		this->weight = weight;
	}
};

struct VertexWeight {
	float w1;
	float w2;
	float w3;
	float w4;
};

struct BoneIndices {
	byte i1;
	byte i2;
	byte i3;
	byte i4;
};

class NiSkinData : public NiObject {
public:
	struct BoneData {
		SkinTransform boneTransform;
		BoundingSphere bounds;
		ushort numVertices = 0;
		std::vector<SkinWeight> vertexWeights;
	};

	SkinTransform skinTransform;
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
public:
	BlockRefArray<NiNode> boneRefs;
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

class BSDismemberSkinInstance : public NiSkinInstance {
public:
	struct PartitionInfo {
		ushort flags;
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

	class BoneData {
	public:
		BoundingSphere bounds;
		SkinTransform boneTransform;

		BoneData() {
			boneTransform.scale = 1.0f;
		}
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

	uint numUnk = 0;
	std::vector<Vector3> unk;

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
