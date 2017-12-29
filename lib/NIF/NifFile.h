/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Animation.h"
#include "bhk.h"
#include "ExtraData.h"
#include "Geometry.h"
#include "Keys.h"
#include "Objects.h"
#include "Particles.h"
#include "Shaders.h"
#include "Skin.h"

#include <unordered_map>

struct OptOptionsSSE {
	bool headParts = false;
	bool removeParallax = true;
	bool calcBounds = true;
};

struct OptResultSSE {
	bool versionMismatch = false;
	bool dupesRenamed = false;
	std::vector<std::string> shapesVColorsRemoved;
	std::vector<std::string> shapesNormalsRemoved;
	std::vector<std::string> shapesPartTriangulated;
	std::vector<std::string> shapesTangentsAdded;
	std::vector<std::string> shapesParallaxRemoved;
};


class IFactory {
public:
	virtual NiObject* Create() = 0;
	virtual NiObject* Load(NiStream& stream) = 0;
};

template<typename T>
class NiFactory : public IFactory {
public:
	// Create new NiObject
	virtual NiObject* Create() override {
		return new T();
	}

	// Load new NiObject from file
	virtual NiObject* Load(NiStream& stream) override {
		T* nio = new T();
		nio->Get(stream);
		return nio;
	}
};

class NiFactoryRegister {
public:
	// Constructor registers the block types
	NiFactoryRegister();

	template<typename T>
	void RegisterFactory() {
		// Any NiObject can be registered together with its block name
		m_registrations.emplace(T::BlockName, std::make_shared<NiFactory<T>>());
	}

	// Get block factory via header std::string
	std::shared_ptr<IFactory> GetFactoryByName(const std::string& name) {
		auto it = m_registrations.find(name);
		if (it != m_registrations.end())
			return it->second;

		return nullptr;
	}

	// Get static instance of factory register
	static NiFactoryRegister& GetNiFactoryRegister();

protected:
	std::unordered_map<std::string, std::shared_ptr<IFactory>> m_registrations;
};

// Sort bone weights with indices
struct BoneWeightsSort {
	bool operator()(const SkinWeight& lhs, const SkinWeight& rhs) {
		if (lhs.weight == rhs.weight)
			return false;

		return rhs.weight < lhs.weight;
	}
};


class NifFile {
private:
	std::string fileName;
	std::vector<std::unique_ptr<NiObject>> blocks;
	bool isValid = false;
	bool hasUnknown = false;

	NiHeader hdr;

public:
	NifFile() {}

	NifFile(const NifFile& other) {
		CopyFrom(other);
	}

	NifFile& operator=(const NifFile& other) {
		CopyFrom(other);
	}

	NiHeader& GetHeader() { return hdr; }
	void CopyFrom(const NifFile& other);

	int Load(const std::string& filename);
	int Load(std::fstream& file, const std::string& filename);
	int Save(const std::string& filename, bool optimize = true, bool sortBlocks = true);
	int Save(std::fstream& file, bool optimize = true, bool sortBlocks = true);

	void Optimize();
	OptResultSSE OptimizeForSSE(const OptOptionsSSE& options = OptOptionsSSE());

	void PrepareData();
	void FinalizeData();

	std::string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }

	void Clear();

	// Link NiGeometryData to NiGeometry
	void LinkGeomData();
	void RemoveInvalidTris();

	int AddNode(const std::string& nodeName, const MatTransform& xform);
	void DeleteNode(const std::string& nodeName);
	std::string GetNodeName(const int blockID);
	void SetNodeName(const int blockID, const std::string& newName);

	int AssignExtraData(NiAVObject* target, NiExtraData* extraData);

	// Explicitly sets the order of shapes to a new one.
	void SetShapeOrder(const std::vector<std::string>& order);
	void SetSortIndex(const int id, std::vector<std::pair<int, int>>& newIndices, int& newIndex);
	void SortAVObject(NiAVObject* avobj, std::vector<std::pair<int, int>>& newIndices, int& newIndex);
	void SortShape(NiShape* shape, std::vector<std::pair<int, int>>& newIndices, int& newIndex);
	void SortGraph(NiNode* root, std::vector<std::pair<int, int>>& newIndices, int& newIndex);
	void PrettySortBlocks();
	bool DeleteUnreferencedBlocks();

	NiShape* FindShapeByName(const std::string& name, int dupIndex = 0);
	NiAVObject* FindAVObjectByName(const std::string& name, int dupIndex = 0);
	NiNode* FindNodeByName(const std::string& name);
	int GetBlockID(NiObject* block);
	NiNode* GetParentNode(NiObject* block);

	NiShader* GetShader(NiShape* shape);
	NiMaterialProperty* GetMaterialProperty(NiShape* shape);

	int GetTextureSlot(NiShader* shader, std::string& outTexFile, int texIndex = 0);
	void SetTextureSlot(NiShader* shader, std::string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	void CloneChildren(NiObject* block, NifFile* srcNif = nullptr);
	NiShape* CloneShape(const std::string& srcShapeName, const std::string& destShapeName, NifFile* srcNif = nullptr);
	int CloneNamedNode(const std::string& nodeName, NifFile* srcNif = nullptr);

	std::vector<std::string> GetShapeNames();
	std::vector<NiShape*> GetShapes();
	void RenameShape(const std::string& oldName, const std::string& newName);
	bool RenameDuplicateShapes();
	void TriangulateShape(NiShape* shape);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	std::vector<T*> GetChildren(NiNode* parent = nullptr, bool searchExtraData = false);

	NiNode* GetRootNode();
	bool GetNodeTransform(const std::string& nodeName, MatTransform& outTransform);
	bool SetNodeTransform(const std::string& nodeName, MatTransform& inTransform, const bool rootChildrenOnly = false);

	int GetShapeBoneList(const std::string& shapeName, std::vector<std::string>& outList);
	int GetShapeBoneIDList(const std::string& shapeName, std::vector<int>& outList);
	void SetShapeBoneIDList(const std::string& shapeName, std::vector<int>& inList);
	int GetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& outWeights);

	// Empty std::string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const std::string& shapeName, const std::string& boneName, MatTransform& outTransform);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(const std::string& shapeName, const int boneIndex, MatTransform& inTransform);
	bool SetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& inBounds);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const std::string& shapeName, const int boneIndex, MatTransform& outTransform);
	bool GetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& outBounds);
	void UpdateShapeBoneID(const std::string& shapeName, const int oldID, const int newID);
	void SetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const std::string& shapeName, const int vertIndex, std::vector<byte>& boneids, std::vector<float>& weights);

	bool GetShapeSegments(const std::string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation);
	void SetShapeSegments(const std::string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation);

	bool GetShapePartitions(const std::string& shapeName, std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, std::vector<std::vector<ushort>>& verts, std::vector<std::vector<Triangle>>& tris);
	void SetShapePartitions(const std::string& shapeName, const std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const std::vector<std::vector<ushort>>& verts, const std::vector<std::vector<Triangle>>& tris);
	void SetDefaultPartition(const std::string& shapeName);

	const std::vector<Vector3>* GetRawVertsForShape(const std::string& shapeName);
	bool ReorderTriangles(const std::string& shapeName, const std::vector<uint>& triangleIndices);
	const std::vector<Vector3>* GetNormalsForShape(const std::string& shapeName, bool transform = true);
	const std::vector<Vector2>* GetUvsForShape(const std::string& shapeName);
	bool GetUvsForShape(const std::string& shapeName, std::vector<Vector2>& outUvs);
	bool GetVertsForShape(const std::string& shapeName, std::vector<Vector3>& outVerts);
	void SetVertsForShape(const std::string& shapeName, const std::vector<Vector3>& verts);
	void SetUvsForShape(const std::string& shapeName, const std::vector<Vector2>& uvs);
	void InvertUVsForShape(const std::string& shapeName, bool invertX, bool invertY);
	void SetNormalsForShape(const std::string& shapeName, const std::vector<Vector3>& norms);
	void CalcNormalsForShape(const std::string& shapeName, const bool smooth = true, const float smoothThresh = 60.0f);
	void CalcTangentsForShape(const std::string& shapeName);

	void GetRootTranslation(Vector3& outVec);

	void MoveVertex(const std::string& shapeName, const Vector3& pos, const int id);
	void OffsetShape(const std::string& shapeName, const Vector3& offset, std::unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const std::string& shapeName, const Vector3& scale, std::unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const std::string& shapeName, const Vector3& angle, std::unordered_map<ushort, float>* mask = nullptr);

	NiAlphaProperty* GetAlphaProperty(NiShape* shape);
	int AssignAlphaProperty(NiShape* shape, NiAlphaProperty* alphaProp); // ushort flags = 4844, ushort threshold = 128
	void RemoveAlphaProperty(NiShape* shape);

	void DeleteShape(const std::string& shapeName);
	void DeleteShader(NiShape* shape);
	void DeleteSkinning(NiShape* shape);
	bool DeleteVertsForShape(const std::string& shapeName, const std::vector<ushort>& indices);

	int CalcShapeDiff(const std::string& shapeName, const std::vector<Vector3>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int CalcUVDiff(const std::string& shapeName, const std::vector<Vector2>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);

	void CreateSkinning(NiShape* shape);
	void UpdateBoundingSphere(const std::string& shapeName);
	void SetShapeDynamic(const std::string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const std::string& shapeName);
	void UpdateSkinPartitions(NiShape* shape);
	bool TriangulatePartitions(NiShape* shape);
	// Update bone set flags
	void UpdatePartitionFlags(NiShape* shape);
};

template <class T>
std::vector<T*> NifFile::GetChildren(NiNode* parent, bool searchExtraData) {
	std::vector<T*> result;
	T* n;

	if (parent == nullptr) {
		parent = GetRootNode();
		if (parent == nullptr)
			return result;
	}

	for (auto& child : parent->GetChildren()) {
		n = hdr.GetBlock<T>(child.GetIndex());
		if (n)
			result.push_back(n);
	}

	if (searchExtraData) {
		for (auto& extraData : parent->GetExtraData()) {
			n = hdr.GetBlock<T>(extraData.GetIndex());
			if (n)
				result.push_back(n);
		}
	}

	return result;
}
