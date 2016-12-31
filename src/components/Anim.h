/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../files/NifFile.h"
#include "../utils/ConfigurationManager.h"

using namespace std;

struct VertexBoneWeights {
	vector<byte> boneIds;
	vector<float> weights;

	VertexBoneWeights() { }

	void Add(const byte inBoneId, const float inWeight) {
		if (inWeight == 0.0f)
			return;

		for (int i = 0; i < weights.size(); ++i) {
			if (inWeight < weights[i])
				continue;

			weights.insert(weights.begin() + i, inWeight);
			boneIds.insert(boneIds.begin() + i, inBoneId);
			return;
		}

		weights.push_back(inWeight);
		boneIds.push_back(inBoneId);
	}
};

class AnimBone {
public:
	string boneName;			// bone names are node names in the nif file
	int boneID;					// block id from the original nif file
	Matrix4 rot;				// original node rotation value (total rotation, including parents)
	Vector3 trans;				// original node translation value (total translation, including parents)
	float scale;				// original node scale value

	bool isValidBone;
	AnimBone* parent;
	vector<AnimBone*> children;
	Matrix4 localRot;			// rotation offset from parent bone.
	Vector3 localTrans;			// offset from parent bone

	bool hasSkinXform;
	Matrix4 skinRot;			// skinning rotation transform   (NOT node transform
	Vector3 skinTrans;			// skinning translation transform  (NOT node transform

	int refCount;				// reference count of this bone

	AnimBone() {
		boneName = "bogus";
		boneID = -1;
		refCount = 0;
		parent = nullptr;
		isValidBone = false;
		hasSkinXform = false;
	}

	AnimBone& LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* parent = nullptr);
};

// Vertex to weight value association. Also keeps track of skin transform and bounding sphere.
class AnimWeight {
public:
	unordered_map<ushort, float> weights;
	SkinTransform xform;
	BoundingSphere bounds;

	AnimWeight() {}
	AnimWeight(NifFile* loadFromFile, const string& shape, const int& index) {
		loadFromFile->GetShapeBoneWeights(shape, index, weights);
		loadFromFile->GetShapeBoneTransform(shape, index, xform);
		loadFromFile->GetShapeBoneBounds(shape, index, bounds);
	}
};

// Bone to weight list association.
class AnimSkin {
public:
	unordered_map<int, AnimWeight> boneWeights;
	unordered_map<string, int> boneNames;

	AnimSkin() { }
	AnimSkin(NifFile* loadFromFile, const string& shape) {
		vector<int> idList;
		loadFromFile->GetShapeBoneIDList(shape, idList);

		int newID = 0;
		for (auto &id : idList) {
			auto node = loadFromFile->GetHeader().GetBlock<NiNode>(id);
			if (node) {
				boneWeights[newID] = AnimWeight(loadFromFile, shape, newID);
				boneNames[node->GetName()] = newID;
				newID++;
			}
		}
	}

	void RemoveBone(const int& boneOrder) {
		unordered_map<int, AnimWeight> bwTemp;
		for (auto &bw : boneWeights) {
			if (bw.first > boneOrder)
				bwTemp[bw.first - 1] = move(bw.second);
			else if (bw.first < boneOrder)
				bwTemp[bw.first] = move(bw.second);
		}

		boneWeights.clear();

		for (auto &bw : bwTemp)
			boneWeights[bw.first] = move(bw.second);
	}
};

class AnimPartition {
public:
	int bodypart;						// Body part number (from BSDismembermentSkinInstance/partiitons).
	vector<Triangle> tris;				// Points are indices to the verts list for this partition. (eg. starting at 0).
	vector<int> verts;					// All referenced verts in this partition.
	vector<int> bones;					// All referenced bones in this partition.
	vector<vector<float>> vertWeights;	// Vert order list of weights per vertex.
	vector<vector<int>> vertBones;		// Vert order list of bones per vertex.
};

/* Represents animation weighting to a common skeleton across multiple shapes, sourced from nif files*/
class AnimInfo {
public:
	map<string, vector<string>> shapeBones;
	unordered_map<string, AnimSkin> shapeSkinning;			// Shape to skin association.
	NifFile* refNif;

	AnimInfo() { refNif = nullptr; }

	// Returns true if a new bone is added, false if the bone already exists.
	bool AddShapeBone(const string& shape, AnimBone& boneDataRef);

	bool RemoveShapeBone(const string& shape, const string& boneName);

	void Clear();
	void ClearShape(const string& shape);

	// Loads the skinning information contained in the nif for all shapes.
	// Returns false if there is no skinning information.
	bool LoadFromNif(NifFile* nif);
	bool LoadFromNif(NifFile* nif, const string& shape, bool newRefNif = true);
	int GetShapeBoneIndex(const string& shapeName, const string& boneName);
	void GetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& outVertWeights);
	void GetBoneXForm(const string& boneName, SkinTransform& stransform);
	void SetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& inVertWeights);
	void SetShapeBoneXForm(const string& shape, const string& boneName, SkinTransform& stransform);
	bool CalcShapeSkinBounds(const string& shape, const int& boneIndex);
	void WriteToNif(NifFile* nif, bool synchBoneIDs = true, const string& shapeException = "");

	void RenameShape(const string& shapeName, const string& newShapeName);
};

class AnimSkeleton {
	AnimBone invBone;
	map<string, AnimBone> allBones;
	map<string, AnimBone> customBones;
	string rootBone;
	int unknownCount;
	bool allowCustom;

	AnimSkeleton() { unknownCount = 0; isValid = false; allowCustom = true; }

public:
	static AnimSkeleton& getInstance() {
		static AnimSkeleton instance;
		return instance;
	}

	NifFile refSkeletonNif;
	bool isValid;

	int LoadFromNif(const string& fileName);
	AnimBone& AddBone(const string& boneName, bool bCustom = false);
	string GenerateBoneName();

	bool RefBone(const string& boneName);
	bool ReleaseBone(const string& boneName);
	int GetBoneRefCount(const string& boneName);

	AnimBone* GetBonePtr(const string& boneName = "");
	bool GetBone(const string& boneName, AnimBone& outBone);
	bool GetSkinTransform(const string &boneName, const SkinTransform& skinning, SkinTransform& xform);
	bool GetBoneTransform(const string &boneName, SkinTransform& xform);

	int GetActiveBoneNames(vector<string>& outBoneNames);
};
