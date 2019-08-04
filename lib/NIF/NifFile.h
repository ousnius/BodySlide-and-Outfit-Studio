/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Factory.h"

struct OptOptions {
	NiVersion targetVersion;
	bool headParts = false;
	bool removeParallax = true;
	bool calcBounds = true;
    bool bsTriShape = false;
};

struct OptResult {
	bool versionMismatch = false;
	bool dupesRenamed = false;
	std::vector<std::string> shapesVColorsRemoved;
	std::vector<std::string> shapesNormalsRemoved;
	std::vector<std::string> shapesPartTriangulated;
	std::vector<std::string> shapesTangentsAdded;
	std::vector<std::string> shapesParallaxRemoved;
};

// Sort bone weights with indices
struct BoneWeightsSort {
	bool operator()(const SkinWeight& lhs, const SkinWeight& rhs) {
		if (lhs.weight == rhs.weight)
			return false;

		return rhs.weight < lhs.weight;
	}
};

struct NifLoadOptions {
	bool isTerrain = false;
};

struct NifSaveOptions {
	bool optimize = true;
	bool sortBlocks = true;
};

class NifFile {
private:
	NiHeader hdr;
	std::vector<std::unique_ptr<NiObject>> blocks;
	bool isValid = false;
	bool hasUnknown = false;
	bool isTerrain = false;

public:
	NifFile() {}

	NifFile(const std::string& fileName, const NifLoadOptions& options = NifLoadOptions()) {
		Load(fileName, options);
	}

	NifFile(std::fstream& file, const NifLoadOptions& options = NifLoadOptions()) {
		Load(file, options);
	}

	NifFile(const NifFile& other) {
		CopyFrom(other);
	}

	NifFile& operator=(const NifFile& other) {
		CopyFrom(other);
	}

	NiHeader& GetHeader() { return hdr; }
	void CopyFrom(const NifFile& other);

	int Load(const std::string& fileName, const NifLoadOptions& options = NifLoadOptions());
	int Load(std::fstream& file, const NifLoadOptions& options = NifLoadOptions());
	int Save(const std::string& fileName, const NifSaveOptions& options = NifSaveOptions());
	int Save(std::fstream& file, const NifSaveOptions& options = NifSaveOptions());

	void Optimize();
	OptResult OptimizeFor(OptOptions& options);

	void PrepareData();
	void FinalizeData();

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }
	bool IsTerrain() { return isTerrain; }

	void Create(const NiVersion& version);
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

	template<class T = NiObject>
	T* FindBlockByName(const std::string& name);
	int GetBlockID(NiObject* block);
	NiNode* GetParentNode(NiObject* block);

	NiShader* GetShader(NiShape* shape);
	NiMaterialProperty* GetMaterialProperty(NiShape* shape);
	NiStencilProperty* GetStencilProperty(NiShape* shape);

	int GetTextureSlot(NiShader* shader, std::string& outTexFile, int texIndex = 0);
	void SetTextureSlot(NiShader* shader, std::string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	void CloneChildren(NiObject* block, NifFile* srcNif = nullptr);
	NiShape* CloneShape(NiShape* srcShape, const std::string& destShapeName, NifFile* srcNif = nullptr);
	int CloneNamedNode(const std::string& nodeName, NifFile* srcNif = nullptr);

	std::vector<std::string> GetShapeNames();
	std::vector<NiShape*> GetShapes();
	bool RenameShape(NiShape* shape, const std::string& newName);
	bool RenameDuplicateShapes();
	void TriangulateShape(NiShape* shape);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	std::vector<T*> GetChildren(NiNode* parent = nullptr, bool searchExtraData = false);

	NiNode* GetRootNode();
	bool GetNodeTransform(const std::string& nodeName, MatTransform& outTransform);
	bool GetAbsoluteNodeTransform(const std::string& nodeName, MatTransform& outTransform);
	bool SetNodeTransform(const std::string& nodeName, MatTransform& inTransform, const bool rootChildrenOnly = false);

	int GetShapeBoneList(NiShape* shape, std::vector<std::string>& outList);
	int GetShapeBoneIDList(NiShape* shape, std::vector<int>& outList);
	void SetShapeBoneIDList(NiShape* shape, std::vector<int>& inList);
	int GetShapeBoneWeights(NiShape* shape, const int boneIndex, std::unordered_map<ushort, float>& outWeights);

	// Empty std::string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(NiShape* shape, const std::string& boneName, MatTransform& outTransform);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(NiShape* shape, const int boneIndex, MatTransform& inTransform);
	bool SetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& inBounds);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(NiShape* shape, const int boneIndex, MatTransform& outTransform);
	bool GetShapeBoneBounds(NiShape* shape, const int boneIndex, BoundingSphere& outBounds);
	void UpdateShapeBoneID(const std::string& shapeName, const int oldID, const int newID);
	void SetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const std::string& shapeName, const int vertIndex, std::vector<byte>& boneids, std::vector<float>& weights);
	void ClearShapeVertWeights(const std::string& shapeName);

	bool GetShapeSegments(NiShape* shape, BSSubIndexTriShape::BSSITSSegmentation& segmentation);
	void SetShapeSegments(NiShape* shape, const BSSubIndexTriShape::BSSITSSegmentation& segmentation);

	bool GetShapePartitions(NiShape* shape, std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, std::vector<std::vector<ushort>>& verts, std::vector<std::vector<Triangle>>& tris);
	void SetShapePartitions(NiShape* shape, const std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const std::vector<std::vector<ushort>>& verts, const std::vector<std::vector<Triangle>>& tris);
	void SetDefaultPartition(NiShape* shape);

	const std::vector<Vector3>* GetRawVertsForShape(NiShape* shape);
	bool ReorderTriangles(NiShape* shape, const std::vector<uint>& triangleIndices);
	const std::vector<Vector3>* GetNormalsForShape(NiShape* shape, bool transform = true);
	const std::vector<Vector2>* GetUvsForShape(NiShape* shape);
	const std::vector<Color4>* GetColorsForShape(const std::string& shapeName);
	bool GetUvsForShape(NiShape* shape, std::vector<Vector2>& outUvs);
	bool GetVertsForShape(NiShape* shape, std::vector<Vector3>& outVerts);
	void SetVertsForShape(NiShape* shape, const std::vector<Vector3>& verts);
	void SetUvsForShape(NiShape* shape, const std::vector<Vector2>& uvs);
	void SetColorsForShape(const std::string& shapeName, const std::vector<Color4>& colors);
	void InvertUVsForShape(NiShape* shape, bool invertX, bool invertY);
	void MirrorShape(NiShape* shape, bool mirrorX, bool mirrorY, bool mirrorZ);
	void SetNormalsForShape(NiShape* shape, const std::vector<Vector3>& norms);
	void CalcNormalsForShape(NiShape* shape, const bool smooth = true, const float smoothThresh = 60.0f);
	void CalcTangentsForShape(NiShape* shape);

	void GetRootTranslation(Vector3& outVec);

	void MoveVertex(NiShape* shape, const Vector3& pos, const int id);
	void OffsetShape(NiShape* shape, const Vector3& offset, std::unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(NiShape* shape, const Vector3& scale, std::unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(NiShape* shape, const Vector3& angle, std::unordered_map<ushort, float>* mask = nullptr);

	NiAlphaProperty* GetAlphaProperty(NiShape* shape);
	int AssignAlphaProperty(NiShape* shape, NiAlphaProperty* alphaProp); // ushort flags = 4844, ushort threshold = 128
	void RemoveAlphaProperty(NiShape* shape);

	void DeleteShape(NiShape* shape);
	void DeleteShader(NiShape* shape);
	void DeleteSkinning(NiShape* shape);
	bool DeleteVertsForShape(NiShape* shape, const std::vector<ushort>& indices);

	int CalcShapeDiff(NiShape* shape, const std::vector<Vector3>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int CalcUVDiff(NiShape* shape, const std::vector<Vector2>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);

	void CreateSkinning(NiShape* shape);
	void UpdateBoundingSphere(const std::string& shapeName);
	void SetShapeDynamic(const std::string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
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
