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
	uint numBones;
	byte hasVertWeights;
	std::vector<BoneData> bones;

	NiSkinData(NiHeader* hdr);
	NiSkinData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiSkinData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int CalcBlockSize();
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
		byte vertFlags1 = 0;					// User Version >= 12, User Version 2 == 100, Number of uint elements in vertex data
		byte vertFlags2 = 0;					// User Version >= 12, User Version 2 == 100, 4 byte or 2 byte position data
		byte vertFlags3 = 0;					// User Version >= 12, User Version 2 == 100
		byte vertFlags4 = 0;					// User Version >= 12, User Version 2 == 100
		byte vertFlags5 = 0;					// User Version >= 12, User Version 2 == 100
		byte vertFlags6 = 0;					// User Version >= 12, User Version 2 == 100, Vertex, UVs, Normals
		byte vertFlags7 = 0;					// User Version >= 12, User Version 2 == 100, (Bi)Tangents, Vertex Colors, Skinning, Precision
		byte vertFlags8 = 0;					// User Version >= 12, User Version 2 == 100
		std::vector<Triangle> trueTriangles;	// User Version >= 12, User Version 2 == 100
	};

	uint numPartitions;
	uint dataSize;							// User Version >= 12, User Version 2 == 100
	uint vertexSize;						// User Version >= 12, User Version 2 == 100
	byte vertFlags1;						// User Version >= 12, User Version 2 == 100, Number of uint elements in vertex data
	byte vertFlags2;						// User Version >= 12, User Version 2 == 100, 4 byte or 2 byte position data
	byte vertFlags3;						// User Version >= 12, User Version 2 == 100
	byte vertFlags4;						// User Version >= 12, User Version 2 == 100
	byte vertFlags5;						// User Version >= 12, User Version 2 == 100
	byte vertFlags6;						// User Version >= 12, User Version 2 == 100, Vertex, UVs, Normals
	byte vertFlags7;						// User Version >= 12, User Version 2 == 100, (Bi)Tangents, Vertex Colors, Skinning, Precision
	byte vertFlags8;						// User Version >= 12, User Version 2 == 100
	uint numVertices;						// Not in file
	std::vector<BSVertexData> vertData;		// User Version >= 12, User Version 2 == 100
	std::vector<PartitionBlock> partitions;

	bool HasVertices() { return (vertFlags6 & (1 << 4)) != 0; }
	bool HasUVs() { return (vertFlags6 & (1 << 5)) != 0; }
	bool HasNormals() { return (vertFlags6 & (1 << 7)) != 0; }
	bool HasTangents() { return (vertFlags7 & (1 << 0)) != 0; }
	bool HasVertexColors() { return (vertFlags7 & (1 << 1)) != 0; }
	bool IsSkinned() { return (vertFlags7 & (1 << 2)) != 0; }
	bool IsFullPrecision() { return true; }

	NiSkinPartition(NiHeader* hdr);
	NiSkinPartition(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiSkinPartition";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(std::vector<int>& outDeletedIndices);
	int CalcBlockSize();
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
	NiSkinInstance(NiHeader* hdr);
	NiSkinInstance(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiSkinInstance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiSkinInstance* Clone() { return new NiSkinInstance(*this); }

	int GetDataRef() { return dataRef.index; }
	void SetDataRef(const int datRef) { this->dataRef.index = datRef; }

	int GetSkinPartitionRef() { return skinPartitionRef.index; }
	void SetSkinPartitionRef(const int skinPartRef) { this->skinPartitionRef.index = skinPartRef; }

	int GetSkeletonRootRef() { return targetRef.index; }
	void SetSkeletonRootRef(const int skelRootRef) { this->targetRef.index = skelRootRef; }
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
	BSDismemberSkinInstance(NiHeader* hdr);
	BSDismemberSkinInstance(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSDismemberSkinInstance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	uint nBones;

	class BoneData {
	public:
		BoundingSphere bounds;
		SkinTransform boneTransform;

		BoneData() {
			boneTransform.scale = 1.0f;
		}
	};

	std::vector<BoneData> boneXforms;

	BSSkinBoneData() : nBones(0) { };
	BSSkinBoneData(NiHeader* hdr);
	BSSkinBoneData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSSkin::BoneData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	std::vector<SkinWeight> vertexWeights;

	BSSkinInstance(NiHeader* hdr);
	BSSkinInstance(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSSkin::Instance";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSSkinInstance* Clone() { return new BSSkinInstance(*this); }

	int GetTargetRef() { return targetRef.index; }
	void SetTargetRef(const int targRef) { this->targetRef.index = targRef; }

	int GetDataRef() { return dataRef.index; }
	void SetDataRef(const int datRef) { this->dataRef.index = datRef; }
};
