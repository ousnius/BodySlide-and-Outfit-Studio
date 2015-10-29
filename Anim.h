/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "NifFile.h"
#include "ConfigurationManager.h"

#include <map>
#include <unordered_map>
#include <vector>

using namespace std;

class AnimBone {
public:
	string boneName;			// bone names are node names in the nif file
	int boneID;					// block id from the original nif file
	int order;					// order of appearance in nif file
	Matrix4 rot;					// original node rotation value (total rotation, including parents)
	Vector3 trans;				// original node translation value (total translation, including parents)
	float scale;				// original node scale value

	bool isValidBone;
	AnimBone* parent;
	vector<AnimBone*> children;
	Matrix4 localRot;				// rotation offset from parent bone.
	Vector3 localTrans;			// offset from parent bone

	int refCount;				// reference count of this bone
	AnimBone() {
		boneName = "bogus";
		boneID = -1;
		order = -1;
		isValidBone = false;
	}

	AnimBone(const string& bn, int bid, int ord) : boneName(bn), boneID(bid), order(ord) {
		refCount = 0;
		parent = nullptr;
		isValidBone = true;
	}
	AnimBone& LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* parent = nullptr);
};

// Vertex to weight value association. Also keeps track of skin transform and bounding sphere.
class AnimWeight {
public:
	//int boneNum;				// Numeric index into the AnimSkin bone list.
	unordered_map<ushort, float> weights;
	SkinTransform xform;
	Vector3 bSphereOffset;
	float bSphereRadius;
	AnimWeight() {}
	AnimWeight(NifFile* loadFromFile, const string& shape, int index) {
		loadFromFile->GetShapeBoneWeights(shape, index, weights);
		loadFromFile->GetShapeBoneTransform(shape, index, xform, bSphereOffset, bSphereRadius);
	}
	bool VertWeight(ushort queryVert, float& weight) {
		if (weights.find(queryVert) != weights.end()) {
			weight = weights[queryVert];
			return true;
		}
		return false;
	}
};

// Bone to weight list association.
class AnimSkin {
public:
	unordered_map<int, AnimWeight> boneWeights;
	AnimSkin() { }
	AnimSkin(NifFile* loadFromFile, const string& shape, const vector<int>& BoneIndices) {
		for (auto &i : BoneIndices)
			boneWeights[i] = AnimWeight(loadFromFile, shape, i);
	}
	void VertexBones(ushort queryvert, vector<int>& outbones, vector<float>& outWeights) {
		float wresult;
		for (auto &bw : boneWeights) {
			if (bw.second.VertWeight(queryvert, wresult)) {
				outbones.push_back(bw.first);
				outWeights.push_back(wresult);
			}
		}
	}
	void RemoveBone(int boneOrder) {
		unordered_map<int, AnimWeight> bwtemp;
		for (auto &bw : boneWeights) {
			if (bw.first > boneOrder)
				bwtemp[bw.first - 1] = move(bw.second);
			else if (bw.first < boneOrder)
				bwtemp[bw.first] = move(bw.second);
		}
		boneWeights.clear();
		for (auto &bw : bwtemp)
			boneWeights[bw.first] = move(bw.second);
	}
};

class AnimPartition {
public:
	int bodypart;						// Body part number (from BSDismembermentSkinInstance/partiitons).
	vector<Triangle> tris;					// Points are indices to the verts list for this partition. (eg. starting at 0).
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
	bool LoadFromNif(NifFile* nif, const string& shape);
	int GetShapeBoneIndex(const string& shapeName, const string& boneName);
	void GetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& outVertWeights);
	void GetBoneXForm(const string& boneName, SkinTransform& stransform);
	void SetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& inVertWeights);
	void SetShapeBoneXForm(const string& shape, const string& boneName, SkinTransform& stransform);
	void WriteToNif(NifFile* nif, bool synchBoneIDs = true);

	void RenameShape(const string& shapeName, const string& newShapeName);	
};

class AnimSkeleton {
	AnimBone invBone;
	AnimSkeleton() { unknownCount = 0; isValid = false; allowCustom = false; }
	map<string, AnimBone> allBones;
	map<string, AnimBone> customBones;
	string rootBone;
	int unknownCount;
	bool allowCustom;

public:
	static AnimSkeleton& getInstance() {
		static AnimSkeleton instance;
		return instance;
	}
	bool isValid;
	int LoadFromNif(const string& filename);
	AnimBone& AddBone(const string& boneName, bool bCustom = false);
	string GenerateBoneName();

	bool RefBone(const string& boneName);
	bool ReleaseBone(const string& boneName);

	AnimBone* GetBonePtr(const string& boneName = "");
	bool GetBone(const string& boneName, AnimBone& outBone);
	bool GetSkinTransform(const string &boneName, SkinTransform& xform);
	bool GetBoneTransform(const string &boneName, SkinTransform& xform);

	int GetActiveBoneNames(vector<string>& outBoneNames);
};
