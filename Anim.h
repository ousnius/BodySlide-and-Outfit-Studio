#pragma once
#include "NifFile.h"
#include "ConfigurationManager.h"
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

using namespace std;

class AnimBone {
public:
	string boneName;			// bone names are node names in the nif file
	int boneID;					// block id from the original nif file
	int order;					// order of appearance in nif file
	Mat4 rot;					// original node rotation value (total rotation, including parents)
	vector3 trans;				// original node translation value (total translation, including parents)
	float scale;				// original node scale value

	bool isValidBone;
	AnimBone* parent;
	vector<AnimBone*> children;
	Mat4 localRot;				// rotation offset from parent bone.
	vector3 localTrans;			// offset from parent bone

	int refCount;				// reference count of this bone
	AnimBone() {
		boneName = "bogus";
		boneID = -1;
		order = -1;
		isValidBone = false;
	}

	AnimBone(const string& bn, int bid, int ord) : boneName(bn), boneID(bid), order(ord) {
		refCount = 0;
		parent = NULL;
		isValidBone = true;
	}
	AnimBone& LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* parent = NULL);
};

// vertex to weight value association;  Also keeps track of skin transform and bounding sphere.
class AnimWeight {
public:
	//int boneNum;				// numeric index into the AnimSkin bone list.
	unordered_map<ushort, float> weights;	
	skin_transform xform;
	vector3 bSphereOffset;		
	float bSphereRadius;
	AnimWeight() {}
	AnimWeight(NifFile* loadFromFile, const string& shape, int index) {
		loadFromFile->GetShapeBoneWeights(shape, index, weights);
		loadFromFile->GetShapeBoneTransform(shape, index, xform, bSphereOffset, bSphereRadius);
	}
	bool VertWeight(int queryVert, float& weight) {
		if (weights.find(queryVert) != weights.end()) {
			weight = weights[queryVert];
			return true;
		}
		return false;
	}
};

// Bone to weight list association
class AnimSkin {
public:
	unordered_map<ushort, AnimWeight> boneWeights;	
	AnimSkin() {}
	/*
	AnimSkin(NifFile* loadFromFile, const string& shape, const vector<string>& boneNames) {
		for(int i =0; i< boneNames.size(); i++) {
			boneWeights[b.order] = AnimWeight(loadFromFile,shape,b.order);
		}
	}
	*/
	AnimSkin(NifFile* loadFromFile, const string& shape, const vector<int>& boneIndices) {
		for (auto i: boneIndices) {
			boneWeights[i] = AnimWeight(loadFromFile, shape, i);
		}
	}
	void VertexBones(int queryvert, vector<int>& outbones, vector<float>& outWeights) {
		float wresult;
		for (auto bw: boneWeights) {
			if (bw.second.VertWeight(queryvert, wresult)) {
				outbones.push_back(bw.first);
				outWeights.push_back(wresult);
			}
		}		
	}
	void RemoveBone(int boneOrder) {
		unordered_map<ushort, AnimWeight> bwtemp;
		for (auto bw: boneWeights) {
			if (bw.first > boneOrder) {
				bwtemp[bw.first-1] = move(bw.second);
			} else if (bw.first < boneOrder) {
				bwtemp[bw.first] = move(bw.second);
			}
		}
		boneWeights.clear();
		for (auto bw: bwtemp) {
			boneWeights[bw.first] = move(bw.second);
		}
	}
};

class AnimPartition {
public:
	int bodypart;				// bodypart number (from bsdismembermentskininstance/partiitons)
	vector<tri> tris;			// points are indices to the verts list for this partition. (eg. starting at 0)
	vector<int> verts;			// all referenced verts in this partition
	vector<int> bones;			// all referenced bones in this partition
	vector<vector<float>> vertWeights;	// vert order list of weights per vertex
	vector<vector<int>> vertBones;		// vert order list of bones per vertex
};

/* represents animation weighting to a common skeleton across multiple shapes, sourced from nif files*/
class AnimInfo {
public:
	//map<string, vector<AnimBone> > boneList;
	map<string, vector<string>> shapeBones;
	unordered_map<string, AnimSkin> shapeSkinning;			// shape to skin association
	NifFile* refNif;
	AnimInfo() { refNif = NULL; }

	// returns true if a new bone is added, false if the bone already exists.
	bool AddShapeBone(const string& shape, AnimBone& boneDataRef);

	bool RemoveShapeBone(const string& shape, const string& boneName);

	void Clear();
	void ClearShape(const string& shape);

	// loads the skinning information contained in the nif for all shapes.  returns false if there is
	// no skinning information
	bool LoadFromNif(NifFile* nif);
	bool LoadFromNif(NifFile* nif, const string& shape);
	int GetShapeBoneIndex(const string& shapeName, const string& boneName);
	void GetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& outVertWeights);
	void GetShapeBoneXform(const string& shape, const string& boneName, skin_transform& stransform);
	void SetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& inVertWeights);
	void SetShapeBoneXform(const string& shape, const string& boneName, skin_transform& stransform);
	void WriteToNif(NifFile* nif, bool synchBoneIDs = true);
//	AnimBone* GetShapeBone(const string& shape, const string& boneName);

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
	bool GetSkinTransform(const string &boneName, skin_transform& xform);
	bool GetBoneTransform(const string &boneName, skin_transform& xform);

	int GetActiveBoneNames(vector<string>& outBoneNames);
};