/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../NIF/NifFile.h"
#include "../utils/ConfigurationManager.h"

#include <map>

struct VertexBoneWeights {
	std::vector<byte> boneIds;
	std::vector<float> weights;

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
	std::string boneName;		// bone names are node names in the nif file
	int boneID;					// block id from the original nif file
	Matrix4 rot;				// original node rotation value (total rotation, including parents)
	Vector3 trans;				// original node translation value (total translation, including parents)
	float scale;				// original node scale value

	bool isValidBone;
	AnimBone* parent;
	std::vector<AnimBone*> children;
	Matrix4 localRot;			// rotation offset from parent bone.
	Vector3 localTrans;			// offset from parent bone

	int refCount;				// reference count of this bone

	AnimBone() {
		boneName = "bogus";
		boneID = -1;
		refCount = 0;
		parent = nullptr;
		isValidBone = false;
	}

	AnimBone& LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* parent = nullptr);
};

// Vertex to weight value association. Also keeps track of skin transform and bounding sphere.
class AnimWeight {
public:
	std::unordered_map<ushort, float> weights;
	SkinTransform xform;
	BoundingSphere bounds;

	AnimWeight() {}
	AnimWeight(NifFile* loadFromFile, const std::string& shape, const int& index) {
		loadFromFile->GetShapeBoneWeights(shape, index, weights);
		loadFromFile->GetShapeBoneTransform(shape, index, xform);
		loadFromFile->GetShapeBoneBounds(shape, index, bounds);
	}
};

// Bone to weight list association.
class AnimSkin {
public:
	std::unordered_map<int, AnimWeight> boneWeights;
	std::unordered_map<std::string, int> boneNames;

	AnimSkin() { }
	AnimSkin(NifFile* loadFromFile, const std::string& shape) {
		std::vector<int> idList;
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
		std::unordered_map<int, AnimWeight> bwTemp;
		for (auto &bw : boneWeights) {
			if (bw.first > boneOrder)
				bwTemp[bw.first - 1] = std::move(bw.second);
			else if (bw.first < boneOrder)
				bwTemp[bw.first] = std::move(bw.second);
		}

		boneWeights.clear();

		for (auto &bw : bwTemp)
			boneWeights[bw.first] = std::move(bw.second);
	}
};

class AnimPartition {
public:
	int bodypart;									// Body part number (from BSDismembermentSkinInstance/partiitons).
	std::vector<Triangle> tris;						// Points are indices to the verts list for this partition. (eg. starting at 0).
	std::vector<int> verts;							// All referenced verts in this partition.
	std::vector<int> bones;							// All referenced bones in this partition.
	std::vector<std::vector<float>> vertWeights;	// Vert order list of weights per vertex.
	std::vector<std::vector<int>> vertBones;		// Vert order list of bones per vertex.
};

/* Represents animation weighting to a common skeleton across multiple shapes, sourced from nif files*/
class AnimInfo {
public:
	std::map<std::string, std::vector<std::string>> shapeBones;
	std::unordered_map<std::string, AnimSkin> shapeSkinning;		// Shape to skin association.
	NifFile* refNif;

	AnimInfo() { refNif = nullptr; }

	// Returns true if a new bone is added, false if the bone already exists.
	bool AddShapeBone(const std::string& shape, const std::string& boneName);
	bool RemoveShapeBone(const std::string& shape, const std::string& boneName);

	void Clear();
	void ClearShape(const std::string& shape);
	void DeleteVertsForShape(const std::string& shape, const std::vector<ushort>& indices);

	// Loads the skinning information contained in the nif for all shapes.
	// Returns false if there is no skinning information.
	bool LoadFromNif(NifFile* nif);
	bool LoadFromNif(NifFile* nif, const std::string& shape, bool newRefNif = true);
	int GetShapeBoneIndex(const std::string& shapeName, const std::string& boneName);
	void GetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& outVertWeights);
	void GetBoneXForm(const std::string& boneName, SkinTransform& stransform);
	void SetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& inVertWeights);
	void SetShapeBoneXForm(const std::string& shape, const std::string& boneName, SkinTransform& stransform);
	bool CalcShapeSkinBounds(const std::string& shape, const int& boneIndex);
	void WriteToNif(NifFile* nif, const std::string& shapeException = "");

	void RenameShape(const std::string& shapeName, const std::string& newShapeName);
};

class AnimSkeleton {
	AnimBone invBone;
	std::map<std::string, AnimBone> allBones;
	std::map<std::string, AnimBone> customBones;
	std::string rootBone;
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

	int LoadFromNif(const std::string& fileName);
	AnimBone& AddBone(const std::string& boneName, bool bCustom = false);
	std::string GenerateBoneName();

	bool RefBone(const std::string& boneName);
	bool ReleaseBone(const std::string& boneName);
	int GetBoneRefCount(const std::string& boneName);

	AnimBone* GetBonePtr(const std::string& boneName = "");
	bool GetBone(const std::string& boneName, AnimBone& outBone);
	bool GetBoneTransform(const std::string& boneName, SkinTransform& xform);
	bool GetSkinTransform(const std::string& boneName, const SkinTransform& skinning, SkinTransform& xform);

	int GetActiveBoneNames(std::vector<std::string>& outBoneNames);
};
