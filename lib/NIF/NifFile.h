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
	bool mandatoryOnly = false;
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
		return *this;
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

	NiNode* AddNode(const std::string& nodeName, const MatTransform& xformToParent, NiNode* parent = nullptr);
	void DeleteNode(const std::string& nodeName);
	bool CanDeleteNode(NiNode* node);
	bool CanDeleteNode(const std::string& nodeName);
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

	template<class T = NiObject>
	int DeleteUnreferencedBlocks() {
		if (hasUnknown)
			return 0;

		int deletionCount = 0;
		hdr.DeleteUnreferencedBlocks<T>(GetBlockID(GetRootNode()), &deletionCount);
		return deletionCount;
	}

	bool DeleteUnreferencedNodes(int* deletionCount = nullptr);

	template<class T = NiObject>
	T* FindBlockByName(const std::string& name);
	int GetBlockID(NiObject* block);
	NiNode* GetParentNode(NiObject* block);
	void SetParentNode(NiObject *block, NiNode *parent);
	std::vector<NiNode*> GetNodes();

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
	bool GetNodeTransformToParent(const std::string& nodeName, MatTransform& outTransform);
	// GetNodeTransform is deprecated.  Use GetNodeTransformToParent instead.
	bool GetNodeTransform(const std::string& nodeName, MatTransform& outTransform) {
		return GetNodeTransformToParent(nodeName, outTransform);
	}
	// GetNodeTransformToGlobal calculates the transform from this node's
	// CS to the global CS by composing transforms up the node tree to the
	// root node.
	bool GetNodeTransformToGlobal(const std::string& nodeName, MatTransform& outTransform);
	// GetAbsoluteNodeTransform is deprecated.  Use GetNodeTransformToGlobal instead.
	bool GetAbsoluteNodeTransform(const std::string& nodeName, MatTransform& outTransform) {
		return GetNodeTransformToGlobal(nodeName, outTransform);
	}
	bool SetNodeTransformToParent(const std::string& nodeName, const MatTransform& inTransform, const bool rootChildrenOnly = false);
	// SetNodeTransform is deprecated.  Use SetNodeTransformToParent instead.
	bool SetNodeTransform(const std::string& nodeName, MatTransform& inTransform, const bool rootChildrenOnly = false) {
		return SetNodeTransformToParent(nodeName, inTransform, rootChildrenOnly);
	}

	int GetShapeBoneList(NiShape* shape, std::vector<std::string>& outList);
	int GetShapeBoneIDList(NiShape* shape, std::vector<int>& outList);
	void SetShapeBoneIDList(NiShape* shape, std::vector<int>& inList);
	int GetShapeBoneWeights(NiShape* shape, const int boneIndex, std::unordered_map<ushort, float>& outWeights);

	// Looks up the shape's global-to-skin transform if it has it.
	// Otherwise, try to calculate it using skin-to-bone and node-to-global
	// transforms.  Returns false on failure.
	bool CalcShapeTransformGlobalToSkin(NiShape* shape, MatTransform& outTransforms);
	// Returns false if no such transform exists in the file, in which
	// case outTransform will not be changed.  Note that, even if this
	// function returns false, you can not assume that the global-to-skin
	// transform is the identity; it almost never is.
	bool GetShapeTransformGlobalToSkin(NiShape* shape, MatTransform& outTransform);
	// Does nothing if the shape has no such transform.
	void SetShapeTransformGlobalToSkin(NiShape* shape, const MatTransform& inTransform);
	bool GetShapeTransformSkinToBone(NiShape* shape, const std::string& boneName, MatTransform& outTransform);
	bool GetShapeTransformSkinToBone(NiShape* shape, int boneIndex, MatTransform& outTransform);
	void SetShapeTransformSkinToBone(NiShape* shape, int boneIndex, const MatTransform& inTransform);
	// Empty std::string for the bone name returns the overall skin transform for the shape.
	// GetShapeBoneTransform is deprecated.  Use GetShapeTransformGlobalToSkin
	// or GetShapeTransfromSkinToBone instead.
	bool GetShapeBoneTransform(NiShape* shape, const std::string& boneName, MatTransform& outTransform);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	// GetShapeBoneTransform is deprecated.  Use GetShapeTransformGlobalToSkin
	// or GetShapeTransfromSkinToBone instead.
	bool GetShapeBoneTransform(NiShape* shape, const int boneIndex, MatTransform& outTransform);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	// SetShapeBoneTransform is deprecated.  Use SetShapeTransformGlobalToSkin
	// or SetShapeTransfromSkinToBone instead.
	bool SetShapeBoneTransform(NiShape* shape, const int boneIndex, MatTransform& inTransform);
	bool SetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& inBounds);
	bool GetShapeBoneBounds(NiShape* shape, const int boneIndex, BoundingSphere& outBounds);
	void UpdateShapeBoneID(const std::string& shapeName, const int oldID, const int newID);
	void SetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const std::string& shapeName, const int vertIndex, std::vector<byte>& boneids, std::vector<float>& weights);
	void ClearShapeVertWeights(const std::string& shapeName);

	bool GetShapeSegments(NiShape* shape, NifSegmentationInfo& inf, std::vector<int>& triParts);
	void SetShapeSegments(NiShape* shape, const NifSegmentationInfo& inf, const std::vector<int>& triParts);

	bool GetShapePartitions(NiShape* shape, std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, std::vector<int> &triParts);
	void SetShapePartitions(NiShape* shape, const std::vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const std::vector<int> &triParts, const bool convertSkinInstance = true);
	void SetDefaultPartition(NiShape* shape);
	// DeletePartitions: partInds must be in sorted ascending order.
	void DeletePartitions(NiShape* shape, std::vector<int> &partInds);

	const std::vector<Vector3>* GetRawVertsForShape(NiShape* shape);
	bool ReorderTriangles(NiShape* shape, const std::vector<uint>& triangleIndices);
	const std::vector<Vector3>* GetNormalsForShape(NiShape* shape, bool transform = true);
	const std::vector<Vector2>* GetUvsForShape(NiShape* shape);
	const std::vector<Color4>* GetColorsForShape(const std::string& shapeName);
	const std::vector<Vector3>* GetTangentsForShape(NiShape* shape, bool transform = true);
	const std::vector<Vector3>* GetBitangentsForShape(NiShape* shape, bool transform = true);
	std::vector<float>* GetEyeDataForShape(NiShape* shape);
	bool GetUvsForShape(NiShape* shape, std::vector<Vector2>& outUvs);
	bool GetVertsForShape(NiShape* shape, std::vector<Vector3>& outVerts);
	void SetVertsForShape(NiShape* shape, const std::vector<Vector3>& verts);
	void SetUvsForShape(NiShape* shape, const std::vector<Vector2>& uvs);
	void SetColorsForShape(const std::string& shapeName, const std::vector<Color4>& colors);
	void SetTangentsForShape(NiShape* shape, const std::vector<Vector3>& in);
	void SetBitangentsForShape(NiShape* shape, const std::vector<Vector3>& in);
	void SetEyeDataForShape(NiShape* shape, const std::vector<float>& in);
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
	void SetShapeDynamic(const std::string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(NiShape* shape);
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
