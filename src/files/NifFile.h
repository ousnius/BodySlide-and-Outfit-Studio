/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "NifBlock.h"

#pragma warning (disable: 4018)

class NifFile {
private:
	string fileName;
	vector<NiObject*> blocks;
	bool isValid;
	bool hasUnknown;

	NiHeader hdr;

	int shapeDataIdForName(const string& name, int& outBlockType);
	int shapeIdForName(const string& name);
	int shapeBoneIndex(const string& shapeName, const string& boneName);
	NiNode* nodeForName(const string& name);

public:
	NifFile();
	NifFile(NifFile& other);
	~NifFile();

	void CopyFrom(NifFile& other);
	NiHeader& GetHeader() { return hdr; }

	int AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale);
	void DeleteNode(const string& nodeName);
	string NodeName(int blockID);

	int AssignExtraData(const string& blockName, const int& extraDataId, bool isNode);
	int AddStringExtraData(const string& blockName, const string& name, const string& stringData, bool isNode = false);
	int AddIntegerExtraData(const string& blockName, const string& name, const int& integerData, bool isNode = false);

	int Load(const string& filename);
	int Save(const string& filename);

	string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }

	void Clear();

	// Explicitly sets the order of shapes to a new one.
	void SetShapeOrder(const vector<string>& order);

	// Sorts children block references under the root node so shapes appear first in the list, emulating the order created by nifskope.
	void PrettySortBlocks();

	NiTriBasedGeom* geomForName(const string& name, int dupIndex = 0);
	BSTriShape* geomForNameF4(const string& name, int dupIndex = 0);
	NiAVObject* avObjectForName(const string& name, int dupIndex = 0);

	NiShader* GetShader(const string& shapeName);
	NiShader* GetShaderF4(const string& shapeName);
	bool IsShaderSkin(const string& shapeName);
	NiMaterialProperty* GetMaterialProperty(const string& shapeName);

	int GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex = 0);
	void SetTextureForShape(const string& shapeName, string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(string& nodeName, NifFile& srcNif);
	void CopyShader(const string& shapeDest, NifFile& srcNif);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, NiTriBasedGeom* geom);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, BSTriShape* geom);

	int GetShapeType(const string& shapeName);
	int GetShapeList(vector<string>& outList);
	void RenameShape(const string& oldName, const string& newName);
	void RenameDuplicateShape(const string& dupedShape);
	void SetNodeName(int blockID, const string& newName);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	vector<T*> GetChildren(NiNode* parent, bool searchExtraData = false);

	int GetNodeID(const string& nodeName);
	int GetRootNodeID();
	bool GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale);
	bool SetNodeTransform(const string& nodeName, SkinTransform& inXform, const bool& rootChildrenOnly = false);

	int GetShapeBoneList(const string& shapeName, vector<string>& outList);
	int GetShapeBoneIDList(const string& shapeName, vector<int>& outList);
	void SetShapeBoneIDList(const string& shapeName, vector<int>& inList);
	int GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights);

	// Empty string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform, BoundingSphere& outBounds);
	// 0xFFFFFFFF for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& inXform, BoundingSphere& inBounds);
	// 0xFFFFFFFF on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& outXform, BoundingSphere& outBounds);
	void UpdateShapeBoneID(const string& shapeName, int oldID, int newID);
	void SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const string& shapeName, int vertIndex, vector<byte>& boneids, vector<float>& weights);

	bool GetShapeSegments(const string& shapeName, BSSubIndexTriShape::BSSITSSegmentation& segmentation);
	void SetShapeSegments(const string& shapeName, const BSSubIndexTriShape::BSSITSSegmentation& segmentation);

	const vector<Vector3>* GetRawVertsForShape(const string& shapeName);
	bool GetTrisForShape(const string& shapeName, vector<Triangle>* outTris);
	bool ReorderTriangles(const string& shapeName, const vector<ushort>& triangleIndices);
	const vector<Vector3>* GetNormalsForShape(const string& shapeName, bool transform = true);
	const vector<Vector3>* GetTangentsForShape(const string& shapeName, bool transform = true);
	const vector<Vector3>* GetBitangentsForShape(const string& shapeName, bool transform = true);
	const vector<Vector2>* GetUvsForShape(const string& shapeName);
	bool GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs);
	const vector<vector<int>>* GetSeamVertsForShape(const string& shapeName);
	bool GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts);
	int GetVertCountForShape(const string& shapeName);
	void SetVertsForShape(const string& shapeName, const vector<Vector3>& verts);
	void SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs);
	void InvertUVsForShape(const string& shapeName, bool invertX, bool invertY);
	void SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms);
	void SmoothNormalsForShape(const string& shapeName);
	void CalcNormalsForShape(const string& shapeName);
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
	int ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<ushort, Vector3>& outDiffData);
	// Based on the skin partitioning spell from NifSkope's source. Uses a different enough algorithm that it generates
	// different automatic partitions, but vert and tri order is comparable.
	void BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition = 24);
	void UpdateBoundingSphere(const string& shapeName);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const string& shapeName);
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
		n = dynamic_cast<T*>(hdr.GetBlock(parent->GetChildRef(i)));
		if (n)
			result.push_back(n);
	}

	if (searchExtraData) {
		for (int i = 0; i < parent->GetNumExtaData(); i++) {
			n = dynamic_cast<T*>(hdr.GetBlock(parent->GetExtraDataRef(i)));
			if (n)
				result.push_back(n);
		}
	}

	return result;
}
