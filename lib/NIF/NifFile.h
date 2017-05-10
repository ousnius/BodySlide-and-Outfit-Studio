/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
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

#include <memory>
#include <unordered_map>

struct OptOptionsSSE {
	bool headParts = false;
};

struct OptResultSSE {
	bool versionMismatch = false;
	std::vector<std::string> shapesRenamed;
	std::vector<std::string> shapesVColorsRemoved;
	std::vector<std::string> shapesNormalsRemoved;
	std::vector<std::string> shapesPartTriangulated;
	std::vector<std::string> shapesTangentsAdded;
};


class IFactory {
public:
	virtual NiObject* Create(NiHeader* hdr) = 0;
	virtual NiObject* Load(std::fstream& file, NiHeader* hdr) = 0;
};

template<typename T>
class NiFactory : public IFactory {
public:
	// Create new NiObject
	virtual NiObject* Create(NiHeader* hdr) override {
		return new T(hdr);
	}

	// Load new NiObject from file
	virtual NiObject* Load(std::fstream& file, NiHeader* hdr) override {
		return new T(file, hdr);
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
	std::vector<NiObject*> blocks;
	bool isValid = false;
	bool hasUnknown = false;

	NiHeader* hdr = nullptr;

public:
	NifFile() {
		hdr = new NiHeader();
	};

	NifFile(const NifFile& other) {
		CopyFrom(other);
	}

	~NifFile() {
		Clear();
		delete hdr;
	}

	void CopyFrom(const NifFile& other);
	NiHeader* GetHeader() { return hdr; }

	int AddNode(const std::string& nodeName, std::vector<Vector3>& rot, Vector3& trans, float scale);
	void DeleteNode(const std::string& nodeName);
	std::string GetNodeName(const int blockID);
	void SetNodeName(const int blockID, const std::string& newName);

	int AssignExtraData(const std::string& blockName, const int extraDataId, bool isNode);
	int AddStringExtraData(const std::string& blockName, const std::string& name, const std::string& stringData, bool isNode = false);
	int AddIntegerExtraData(const std::string& blockName, const std::string& name, const int integerData, bool isNode = false);

	int Load(const std::string& filename);
	int Save(const std::string& filename, bool optimize = true, bool sortBlocks = true);
	void Optimize();
	OptResultSSE OptimizeForSSE(const OptOptionsSSE& options = OptOptionsSSE());
	void PrepareData();
	void FinalizeData();

	std::string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }

	void Clear();

	// Explicitly sets the order of shapes to a new one.
	void SetShapeOrder(const std::vector<std::string>& order);

	// Sorts children block references under the root node so shapes appear first in the list, emulating the order created by nifskope.
	void PrettySortBlocks();
	bool DeleteUnreferencedBlocks();
	int RemoveUnusedStrings();

	NiShape* FindShapeByName(const std::string& name, int dupIndex = 0);
	NiAVObject* FindAVObjectByName(const std::string& name, int dupIndex = 0);
	NiNode* FindNodeByName(const std::string& name);
	int GetBlockID(NiObject* block);
	NiNode* GetParentNode(NiObject* block);

	NiShader* GetShader(const std::string& shapeName);
	bool IsShaderSkin(const std::string& shapeName);
	NiMaterialProperty* GetMaterialProperty(const std::string& shapeName);

	int GetTextureForShape(const std::string& shapeName, std::string& outTexFile, int texIndex = 0);
	void SetTextureForShape(const std::string& shapeName, std::string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(std::string& nodeName, NifFile& srcNif);
	void CopyShader(const std::string& shapeDest, NifFile& srcNif);
	void CopyController(NiShader* destShader, NiShader* srcShader);
	void CopyInterpolators(NiTimeController* destController, NiTimeController* srcController);
	int CopyInterpolator(NiHeader* destHeader, NiHeader* srcHeader, int srcInterpId);
	void CopyGeometry(const std::string& shapeDest, NifFile& srcNif, const std::string& srcShape);

	int GetShapeList(std::vector<std::string>& outList);
	void RenameShape(const std::string& oldName, const std::string& newName);
	bool RenameDuplicateShape(const std::string& dupedShape);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	std::vector<T*> GetChildren(NiNode* parent, bool searchExtraData = false);

	int GetRootNodeID();
	bool GetNodeTransform(const std::string& nodeName, std::vector<Vector3>& outRot, Vector3& outTrans, float& outScale);
	bool SetNodeTransform(const std::string& nodeName, SkinTransform& inXform, const bool rootChildrenOnly = false);

	int GetShapeBoneList(const std::string& shapeName, std::vector<std::string>& outList);
	int GetShapeBoneIDList(const std::string& shapeName, std::vector<int>& outList);
	void SetShapeBoneIDList(const std::string& shapeName, std::vector<int>& inList);
	int GetShapeBoneWeights(const std::string& shapeName, const int boneIndex, std::unordered_map<ushort, float>& outWeights);

	// Empty std::string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const std::string& shapeName, const std::string& boneName, SkinTransform& outXform);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(const std::string& shapeName, const int boneIndex, SkinTransform& inXform);
	bool SetShapeBoneBounds(const std::string& shapeName, const int boneIndex, BoundingSphere& inBounds);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const std::string& shapeName, const int boneIndex, SkinTransform& outXform);
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
	bool GetTrisForShape(const std::string& shapeName, std::vector<Triangle>* outTris);
	bool ReorderTriangles(const std::string& shapeName, const std::vector<uint>& triangleIndices);
	const std::vector<Vector3>* GetNormalsForShape(const std::string& shapeName, bool transform = true);
	const std::vector<Vector2>* GetUvsForShape(const std::string& shapeName);
	bool GetUvsForShape(const std::string& shapeName, std::vector<Vector2>& outUvs);
	bool GetVertsForShape(const std::string& shapeName, std::vector<Vector3>& outVerts);
	int GetVertCountForShape(const std::string& shapeName);
	void SetVertsForShape(const std::string& shapeName, const std::vector<Vector3>& verts);
	void SetUvsForShape(const std::string& shapeName, const std::vector<Vector2>& uvs);
	void InvertUVsForShape(const std::string& shapeName, bool invertX, bool invertY);
	void SetNormalsForShape(const std::string& shapeName, const std::vector<Vector3>& norms);
	void CalcNormalsForShape(const std::string& shapeName, const bool smooth = true, const float smoothThresh = 60.0f);
	void CalcTangentsForShape(const std::string& shapeName);

	void ClearShapeTransform(const std::string& shapeName);
	void GetShapeTransform(const std::string& shapeName, Matrix4& outTransform);

	void ClearRootTransform();
	void GetRootTranslation(Vector3& outVec);
	void SetRootTranslation(const Vector3& newTrans);
	void GetRootScale(float& outScale);
	void SetRootScale(const float newScale);

	void GetShapeTranslation(const std::string& shapeName, Vector3& outVec);
	void SetShapeTranslation(const std::string& shapeName, const Vector3& newTrans);
	void GetShapeScale(const std::string& shapeName, float& outScale);
	void SetShapeScale(const std::string& shapeName, const float newScale);
	void ApplyShapeTranslation(const std::string& shapeName, const Vector3& offset);

	void MoveVertex(const std::string& shapeName, const Vector3& pos, const int id);
	void OffsetShape(const std::string& shapeName, const Vector3& offset, std::unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const std::string& shapeName, const Vector3& scale, std::unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const std::string& shapeName, const Vector3& angle, std::unordered_map<ushort, float>* mask = nullptr);

	bool GetAlphaForShape(const std::string& shapeName, ushort& outFlags, byte& outThreshold);
	void SetAlphaForShape(const std::string& shapeName, ushort flags = 4844, ushort threshold = 128);

	bool IsShapeSkinned(const std::string& shapeName);

	void DeleteShape(const std::string& shapeName);
	void DeleteShader(const std::string& shapeName);
	void DeleteAlpha(const std::string& shapeName);
	void DeleteSkinning(const std::string& shapeName);
	bool DeleteVertsForShape(const std::string& shapeName, const std::vector<ushort>& indices);

	int CalcShapeDiff(const std::string& shapeName, const std::vector<Vector3>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int CalcUVDiff(const std::string& shapeName, const std::vector<Vector2>* targetData, std::unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);

	void CreateSkinning(const std::string& shapeName);
	void UpdateBoundingSphere(const std::string& shapeName);
	void SetShapeDynamic(const std::string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const std::string& shapeName);
	bool TriangulatePartitions(const std::string& shapeName);
	// Update bone set flags
	void UpdatePartitionFlags(const std::string& shapeName);
};

template <class T>
std::vector<T*> NifFile::GetChildren(NiNode* parent, bool searchExtraData) {
	std::vector<T*> result;
	T* n;

	if (parent == nullptr) {
		n = dynamic_cast<T*>(blocks[0]);
		if (n)
			result.push_back(n);

		return result;
	}

	for (int i = 0; i < parent->GetNumChildren(); i++) {
		n = hdr->GetBlock<T>(parent->GetChildRef(i));
		if (n)
			result.push_back(n);
	}

	if (searchExtraData) {
		for (int i = 0; i < parent->GetNumExtraData(); i++) {
			n = hdr->GetBlock<T>(parent->GetExtraDataRef(i));
			if (n)
				result.push_back(n);
		}
	}

	return result;
}
