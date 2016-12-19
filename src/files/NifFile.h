/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "NifBlock.h"

#pragma warning (disable: 4018)

struct OptOptionsSSE {
	bool headParts = false;
};

struct OptResultSSE {
	bool versionMismatch = false;
	vector<string> shapesRenamed;
	vector<string> shapesVColorsRemoved;
	vector<string> shapesNormalsRemoved;
	vector<string> shapesPartTriangulated;
	vector<string> shapesTangentsAdded;
};

class NifFile {
private:
	string fileName;
	vector<NiObject*> blocks;
	bool isValid;
	bool hasUnknown;

	NiHeader hdr;

public:
	NifFile();
	NifFile(NifFile& other);
	~NifFile();

	void CopyFrom(NifFile& other);
	NiHeader& GetHeader() { return hdr; }

	int AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale);
	void DeleteNode(const string& nodeName);
	string GetNodeName(const int& blockID);
	void SetNodeName(const int& blockID, const string& newName);

	int AssignExtraData(const string& blockName, const int& extraDataId, bool isNode);
	int AddStringExtraData(const string& blockName, const string& name, const string& stringData, bool isNode = false);
	int AddIntegerExtraData(const string& blockName, const string& name, const int& integerData, bool isNode = false);

	int Load(const string& filename);
	int Save(const string& filename, bool optimize = true, bool sortBlocks = true);
	void Optimize();
	OptResultSSE OptimizeForSSE(const OptOptionsSSE& options = OptOptionsSSE());
	void PrepareData();
	void FinalizeData();

	string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }

	void Clear();

	// Explicitly sets the order of shapes to a new one.
	void SetShapeOrder(const vector<string>& order);

	// Sorts children block references under the root node so shapes appear first in the list, emulating the order created by nifskope.
	void PrettySortBlocks();
	int RemoveUnusedStrings();

	NiShape* FindShapeByName(const string& name, int dupIndex = 0);
	NiAVObject* FindAVObjectByName(const string& name, int dupIndex = 0);
	NiNode* FindNodeByName(const string& name);
	int GetBlockID(NiObject* block);
	NiNode* GetParentNode(NiObject* block);

	NiShader* GetShader(const string& shapeName);
	bool IsShaderSkin(const string& shapeName);
	NiMaterialProperty* GetMaterialProperty(const string& shapeName);

	int GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex = 0);
	void SetTextureForShape(const string& shapeName, string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(string& nodeName, NifFile& srcNif);
	void CopyShader(const string& shapeDest, NifFile& srcNif);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape);

	BlockType GetShapeType(const string& shapeName);
	int GetShapeList(vector<string>& outList);
	void RenameShape(const string& oldName, const string& newName);
	bool RenameDuplicateShape(const string& dupedShape);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	vector<T*> GetChildren(NiNode* parent, bool searchExtraData = false);

	int GetRootNodeID();
	bool GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale);
	bool SetNodeTransform(const string& nodeName, SkinTransform& inXform, const bool rootChildrenOnly = false);

	int GetShapeBoneList(const string& shapeName, vector<string>& outList);
	int GetShapeBoneIDList(const string& shapeName, vector<int>& outList);
	void SetShapeBoneIDList(const string& shapeName, vector<int>& inList);
	int GetShapeBoneWeights(const string& shapeName, const int& boneIndex, unordered_map<ushort, float>& outWeights);

	// Empty string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(const string& shapeName, const int& boneIndex, SkinTransform& inXform);
	bool SetShapeBoneBounds(const string& shapeName, const int& boneIndex, BoundingSphere& inBounds);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, const int& boneIndex, SkinTransform& outXform);
	bool GetShapeBoneBounds(const string& shapeName, const int& boneIndex, BoundingSphere& outBounds);
	void UpdateShapeBoneID(const string& shapeName, const int& oldID, const int& newID);
	void SetShapeBoneWeights(const string& shapeName, const int& boneIndex, unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const string& shapeName, const int& vertIndex, vector<byte>& boneids, vector<float>& weights);

	bool GetShapeSegments(const string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation);
	void SetShapeSegments(const string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation);

	bool GetShapePartitions(const string& shapeName, vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, vector<vector<ushort>>& verts, vector<vector<Triangle>>& tris);
	void SetShapePartitions(const string& shapeName, const vector<BSDismemberSkinInstance::PartitionInfo>& partitionInfo, const vector<vector<ushort>>& verts, const vector<vector<Triangle>>& tris);
	void SetDefaultPartition(const string& shapeName);

	const vector<Vector3>* GetRawVertsForShape(const string& shapeName);
	bool GetTrisForShape(const string& shapeName, vector<Triangle>* outTris);
	bool ReorderTriangles(const string& shapeName, const vector<uint>& triangleIndices);
	const vector<Vector3>* GetNormalsForShape(const string& shapeName, bool transform = true);
	const vector<Vector2>* GetUvsForShape(const string& shapeName);
	bool GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs);
	bool GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts);
	int GetVertCountForShape(const string& shapeName);
	void SetVertsForShape(const string& shapeName, const vector<Vector3>& verts);
	void SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs);
	void InvertUVsForShape(const string& shapeName, bool invertX, bool invertY);
	void SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms);
	void CalcNormalsForShape(const string& shapeName, const bool smooth = true, const float& smoothThresh = 60.0f);
	void CalcTangentsForShape(const string& shapeName);

	void ClearShapeTransform(const string& shapeName);
	void GetShapeTransform(const string& shapeName, Matrix4& outTransform);

	void ClearRootTransform();
	void GetRootTranslation(Vector3& outVec);
	void SetRootTranslation(const Vector3& newTrans);
	void GetRootScale(float& outScale);
	void SetRootScale(const float& newScale);

	void GetShapeTranslation(const string& shapeName, Vector3& outVec);
	void SetShapeTranslation(const string& shapeName, const Vector3& newTrans);
	void GetShapeScale(const string& shapeName, float& outScale);
	void SetShapeScale(const string& shapeName, const float& newScale);
	void ApplyShapeTranslation(const string& shapeName, const Vector3& offset);

	void MoveVertex(const string& shapeName, const Vector3& pos, const int& id);
	void OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const string& shapeName, const Vector3& scale, unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask = nullptr);

	bool GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold);
	void SetAlphaForShape(const string& shapeName, ushort flags = 4844, ushort threshold = 128);

	bool IsShapeSkinned(const string& shapeName);

	void DeleteShape(const string& shapeName);
	void DeleteShader(const string& shapeName);
	void DeleteAlpha(const string& shapeName);
	void DeleteSkinning(const string& shapeName);
	void DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices);

	int CalcShapeDiff(const string& shapeName, const vector<Vector3>* targetData, unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int CalcUVDiff(const string& shapeName, const vector<Vector2>* targetData, unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<ushort, Vector3>& outDiffData);

	void CreateSkinning(const string& shapeName);
	void UpdateBoundingSphere(const string& shapeName);
	void SetShapeDynamic(const string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const string& shapeName);
	bool TriangulatePartitions(const string& shapeName);
	// Update bone set flags
	void UpdatePartitionFlags(const string& shapeName);
};

template <class T>
vector<T*> NifFile::GetChildren(NiNode* parent, bool searchExtraData) {
	vector<T*> result;
	T* n;

	if (parent == nullptr) {
		n = dynamic_cast<T*>(blocks[0]);
		if (n)
			result.push_back(n);

		return result;
	}

	for (int i = 0; i < parent->GetNumChildren(); i++) {
		n = hdr.GetBlock<T>(parent->GetChildRef(i));
		if (n)
			result.push_back(n);
	}

	if (searchExtraData) {
		for (int i = 0; i < parent->GetNumExtraData(); i++) {
			n = hdr.GetBlock<T>(parent->GetExtraDataRef(i));
			if (n)
				result.push_back(n);
		}
	}

	return result;
}
