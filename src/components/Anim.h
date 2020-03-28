/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../NIF/NifFile.h"
#include "../utils/ConfigurationManager.h"

#include <map>

struct VertexBoneWeights {
	std::vector<byte> boneIds;
	std::vector<float> weights;

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
	std::string boneName = "bogus";		// bone names are node names in the nif file
	bool isStandardBone = false;
	AnimBone* parent = nullptr;
	std::vector<AnimBone*> children;
	// xformToGlobal: transforms from this bone's CS to the global CS.
	MatTransform xformToGlobal;
	// xformToParent: transforms from this bone's CS to its parent's CS.
	MatTransform xformToParent;
	// pose rotation and translation vectors
	Vector3 poseRotVec, poseTranVec;
	MatTransform xformPoseToGlobal;

	int refCount = 0;					// reference count of this bone

	AnimBone& LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* parent = nullptr);
	// AddToNif adds this bone to the given nif, as well as its parent
	// if missing, recursively.  The new bone's NiNode is returned.
	NiNode* AddToNif(NifFile *nif) const;
	// SetTransformBoneToParent sets xformToParent and updates xformToGlobal
	// and xformPoseToGlobal, for this and for descendants.
	void SetTransformBoneToParent(const MatTransform &ttp);
	// UpdateTransformToGlobal updates xformToGlobal for this and for
	// descendants.  This should only be called from itself,
	// SetTransformBoneToParent, and SetParentBone.
	void UpdateTransformToGlobal();
	// UpdatePoseTransform updates xformPoseToGlobal for this and all
	// descendants.  Call it after poseRotVec, poseTranVec, or
	// xformToGlobal is changed.
	void UpdatePoseTransform();
	// SetParentBone updates "parent" of this and "children" of the old
	// and new parents.  It also calls UpdateTransformToGlobal and
	// UpdatePoseTranform.
	void SetParentBone(AnimBone* newParent);
};

// Vertex to weight value association. Also keeps track of skin-to-bone transform and bounding sphere.
class AnimWeight {
public:
	std::unordered_map<ushort, float> weights;
	MatTransform xformSkinToBone;
	BoundingSphere bounds;

	void LoadFromNif(NifFile* loadFromFile, NiShape* shape, const int& index);
};

// Bone to weight list association.
class AnimSkin {
public:
	std::unordered_map<int, AnimWeight> boneWeights;
	std::unordered_map<std::string, int> boneNames;
	MatTransform xformGlobalToSkin;

	void LoadFromNif(NifFile* loadFromFile, NiShape* shape);

	void RemoveBone(const std::string& boneName) {
		auto bone = boneNames.find(boneName);
		if (bone == boneNames.end())
			return;

		int boneID = bone->second;
		std::unordered_map<int, AnimWeight> bwTemp;
		for (auto &bw : boneWeights) {
			if (bw.first > boneID)
				bwTemp[bw.first - 1] = std::move(bw.second);
			else if (bw.first < boneID)
				bwTemp[bw.first] = std::move(bw.second);
		}

		boneWeights.clear();

		for (auto &bw : bwTemp)
			boneWeights[bw.first] = std::move(bw.second);

		boneNames.erase(boneName);
		for (auto &bn : boneNames)
			if (bn.second > boneID)
				bn.second--;
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
private:
	NifFile* refNif = nullptr;

public:
	std::map<std::string, std::vector<std::string>> shapeBones;
	std::unordered_map<std::string, AnimSkin> shapeSkinning;		// Shape to skin association.

	NifFile* GetRefNif() { return refNif; };
	void SetRefNif(NifFile* nif) { refNif = nif; };

	// Returns true if a new bone is added, false if the bone already exists.
	bool AddShapeBone(const std::string& shape, const std::string& boneName);
	bool RemoveShapeBone(const std::string& shape, const std::string& boneName);

	void Clear();
	void ClearShape(const std::string& shape);
	void DeleteVertsForShape(const std::string& shape, const std::vector<ushort>& indices);

	// Loads the skinning information contained in the nif for all shapes.
	// Returns false if there is no skinning information.
	bool LoadFromNif(NifFile* nif);
	bool LoadFromNif(NifFile* nif, NiShape* shape, bool newRefNif = true);
	int GetShapeBoneIndex(const std::string& shapeName, const std::string& boneName);
	std::unordered_map<ushort, float>* GetWeightsPtr(const std::string& shape, const std::string& boneName);
	bool HasWeights(const std::string& shape, const std::string& boneName);
	void GetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& outVertWeights);
	void SetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& inVertWeights);
	bool GetXFormSkinToBone(const std::string& shape, const std::string& boneName, MatTransform& stransform);
	void SetXFormSkinToBone(const std::string& shape, const std::string& boneName, const MatTransform& stransform);
	// RecalcXFormSkinToBone recalculates a shape bone's xformSkinToBone
	// from other transforms.
	void RecalcXFormSkinToBone(const std::string& shape, const std::string& boneName);
	// RecursiveRecalcXFormSkinToBone calls RecalcXFormSkinToBone for the
	// given bone and all its descendants.
	void RecursiveRecalcXFormSkinToBone(const std::string& shape, AnimBone *bPtr);
	bool CalcShapeSkinBounds(const std::string& shapeName, const int& boneIndex);
	void CleanupBones();
	void WriteToNif(NifFile* nif, const std::string& shapeException = "");

	void RenameShape(const std::string& shapeName, const std::string& newShapeName);
};

class AnimSkeleton {
	std::map<std::string, AnimBone> allBones;
	std::map<std::string, AnimBone> customBones;
	std::string rootBone;
	int unknownCount = 0;
	bool allowCustomTransforms = true;

	AnimSkeleton() {}

public:
	static AnimSkeleton& getInstance() {
		static AnimSkeleton instance;
		return instance;
	}

	NifFile refSkeletonNif;

	int LoadFromNif(const std::string& fileName);
	AnimBone& AddStandardBone(const std::string& boneName);
	AnimBone& AddCustomBone(const std::string& boneName);
	std::string GenerateBoneName();
	AnimBone *LoadCustomBoneFromNif(NifFile *nif, const std::string &boneName);

	bool RefBone(const std::string& boneName);
	bool ReleaseBone(const std::string& boneName);
	int GetBoneRefCount(const std::string& boneName);

	AnimBone* GetBonePtr(const std::string& boneName, const bool allowCustom = true);
	AnimBone* GetRootBonePtr();
	bool GetBoneTransformToGlobal(const std::string& boneName, MatTransform& xform);

	int GetActiveBoneNames(std::vector<std::string>& outBoneNames);
	void DisableCustomTransforms();
};
