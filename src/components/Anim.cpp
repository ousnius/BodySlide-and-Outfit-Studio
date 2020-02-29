/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Anim.h"
#include <wx/log.h>
#include <wx/msgdlg.h>

extern ConfigurationManager Config;

bool AnimInfo::AddShapeBone(const std::string& shape, const std::string& boneName) {
	for (auto &bone : shapeBones[shape])
		if (!bone.compare(boneName))
			return false;

	shapeSkinning[shape].boneNames[boneName] = shapeBones[shape].size();
	shapeBones[shape].push_back(boneName);
	AnimSkeleton::getInstance().RefBone(boneName);
	// Calculate a good default value for xformSkinToBone by:
	// Composing: bone -> global -> skin
	// then inverting
	MatTransform xformGlobalToSkin = shapeSkinning[shape].xformGlobalToSkin;
	MatTransform xformBoneToGlobal;
	AnimSkeleton::getInstance().GetBoneTransformToGlobal(boneName, xformBoneToGlobal);
	MatTransform xformBoneToSkin = xformGlobalToSkin.ComposeTransforms(xformBoneToGlobal);
	SetXFormSkinToBone(shape, boneName, xformBoneToSkin.InverseTransform());
	return true;
}

bool AnimInfo::RemoveShapeBone(const std::string& shape, const std::string& boneName) {
	auto& bones = shapeBones[shape];
	if (std::find(bones.begin(), bones.end(), boneName) == bones.end())
		return false;

	bones.erase(std::remove(bones.begin(), bones.end(), boneName), bones.end());
	shapeSkinning[shape].RemoveBone(boneName);

	AnimSkeleton::getInstance().ReleaseBone(boneName);
	if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0) {
		if (refNif->CanDeleteNode(boneName))
			refNif->DeleteNode(boneName);
	}

	return true;
}

void AnimInfo::Clear() {
	if (refNif && refNif->IsValid()) {
		for (auto &shapeBoneList : shapeBones) {
			for (auto &boneName : shapeBoneList.second) {
				AnimSkeleton::getInstance().ReleaseBone(boneName);
				if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0) {
					if (refNif->CanDeleteNode(boneName))
						refNif->DeleteNode(boneName);
				}
			}
		}

		shapeSkinning.clear();
		for (auto &s : refNif->GetShapeNames())
			shapeBones[s].clear();

		refNif = nullptr;
	}
}

void AnimInfo::ClearShape(const std::string& shape) {
	if (shape.empty())
		return;

	for (auto &boneName : shapeBones[shape]) {
		AnimSkeleton::getInstance().ReleaseBone(boneName);
		if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0) {
			if (refNif->CanDeleteNode(boneName))
				refNif->DeleteNode(boneName);
		}
	}

	shapeBones.erase(shape);
	shapeSkinning.erase(shape);
}

void AnimInfo::DeleteVertsForShape(const std::string& shape, const std::vector<ushort>& indices) {
	if (indices.empty())
		return;

	ushort highestRemoved = indices.back();
	std::vector<int> indexCollapse(highestRemoved + 1, 0);

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < indices.size() && indices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;						// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	auto& skin = shapeSkinning[shape];
	for (auto &w : skin.boneWeights) {
		std::unordered_map<ushort, float> indexCopy;
		for (auto &d : w.second.weights) {
			if (d.first > highestRemoved)
				indexCopy.emplace(d.first - remCount, d.second);
			else if (indexCollapse[d.first] != -1)
				indexCopy.emplace(d.first - indexCollapse[d.first], d.second);
		}

		w.second.weights.clear();
		w.second.weights.reserve(indexCopy.size());
		for (auto &copy : indexCopy)
			w.second.weights[copy.first] = std::move(copy.second);
	}
}

AnimWeight::AnimWeight(NifFile* loadFromFile, NiShape* shape, const int& index) {
	loadFromFile->GetShapeBoneWeights(shape, index, weights);
	loadFromFile->GetShapeTransformSkinToBone(shape, index, xformSkinToBone);
	loadFromFile->GetShapeBoneBounds(shape, index, bounds);
}

AnimSkin::AnimSkin(NifFile* loadFromFile, NiShape* shape) {
	bool gotGTS = loadFromFile->GetShapeTransformGlobalToSkin(shape, xformGlobalToSkin);
	std::vector<int> idList;
	loadFromFile->GetShapeBoneIDList(shape, idList);

	int newID = 0;
	for (auto &id : idList) {
		auto node = loadFromFile->GetHeader().GetBlock<NiNode>(id);
		if (!node) continue;
		boneWeights[newID] = AnimWeight(loadFromFile, shape, newID);
		boneNames[node->GetName()] = newID;
		if (!gotGTS) {
			// We don't have a global-to-skin transform, probably because
			// the NIF has BSSkinBoneData instead of NiSkinData (FO4 or
			// newer).  So calculate by:
			// Compose: skin -> bone -> global
			// and inverting.
			MatTransform xformBoneToGlobal;
			if (AnimSkeleton::getInstance().GetBoneTransformToGlobal(node->GetName(), xformBoneToGlobal)) {
				xformGlobalToSkin = xformBoneToGlobal.ComposeTransforms(boneWeights[newID].xformSkinToBone).InverseTransform();
				gotGTS = true;
			}
		}
		newID++;
	}
}

bool AnimInfo::LoadFromNif(NifFile* nif) {
	Clear();

	for (auto &s : nif->GetShapes())
		LoadFromNif(nif, s);

	refNif = nif;
	return true;
}

bool AnimInfo::LoadFromNif(NifFile* nif, NiShape* shape, bool newRefNif) {
	std::vector<std::string> boneNames;
	std::string nonRefBones;

	if (newRefNif)
		refNif = nif;

	if (!shape)
		return false;

	std::string shapeName = shape->GetName();
	if (!nif->GetShapeBoneList(shape, boneNames)) {
		wxLogWarning("No skinning found in shape '%s'.", shapeName);
		return false;
	}

	for (auto &bn : boneNames) {
		if (!AnimSkeleton::getInstance().RefBone(bn)) {
			AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
			if (!cstm.isValidBone)
				nonRefBones += bn + "\n";

			MatTransform xform;
			nif->GetNodeTransformToGlobal(bn, xform);

			cstm.xformToGlobal = xform;
			cstm.xformToParent = xform;
			cstm.xformPoseToGlobal = xform;

			AnimSkeleton::getInstance().RefBone(bn);
		}

		shapeBones[shapeName].push_back(bn);
	}

	shapeSkinning[shapeName] = AnimSkin(nif, shape);

	if (!nonRefBones.empty())
		wxLogMessage("Bones in shape '%s' not found in reference skeleton and added as custom bones:\n%s", shapeName, nonRefBones);

	return true;
}

int AnimInfo::GetShapeBoneIndex(const std::string& shapeName, const std::string& boneName) {
	auto& skin = shapeSkinning[shapeName];
	auto bone = skin.boneNames.find(boneName);
	if (bone != skin.boneNames.end())
		return bone->second;

	return -1;
}

std::unordered_map<ushort, float>* AnimInfo::GetWeightsPtr(const std::string& shape, const std::string& boneName) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return nullptr;

	return &shapeSkinning[shape].boneWeights[b].weights;
}

bool AnimInfo::HasWeights(const std::string& shape, const std::string& boneName) {
	auto weights = GetWeightsPtr(shape, boneName);
	if (weights && weights->size() > 0)
		return true;
	
	return false;
}

void AnimInfo::GetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& outVertWeights) {
	auto weights = GetWeightsPtr(shape, boneName);
	if (weights)
		outVertWeights = *weights;
}

bool AnimInfo::GetXFormSkinToBone(const std::string& shape, const std::string& boneName, MatTransform& stransform) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return false;

	stransform = shapeSkinning[shape].boneWeights[b].xformSkinToBone;
	return true;
}

void AnimInfo::SetXFormSkinToBone(const std::string& shape, const std::string& boneName, const MatTransform& stransform) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return;

	shapeSkinning[shape].boneWeights[b].xformSkinToBone = stransform;
}

bool AnimInfo::CalcShapeSkinBounds(const std::string& shapeName, const int& boneIndex) {
	if (!refNif || !refNif->IsValid())	// Check for existence of reference nif
		return false;

	if (shapeSkinning.find(shapeName) == shapeSkinning.end())	// Check for shape in skinning data
		return false;

	auto shape = refNif->FindBlockByName<NiShape>(shapeName);

	std::vector<Vector3> verts;
	refNif->GetVertsForShape(shape, verts);
	if (verts.size() == 0)	// Check for empty shape
		return false;

	std::vector<Vector3> boundVerts;
	for (auto &w : shapeSkinning[shapeName].boneWeights[boneIndex].weights) {
		if (w.first >= verts.size())		// Incoming weights have a larger set of possible verts.
			return false;

		boundVerts.push_back(verts[w.first]);
	}

	BoundingSphere bounds(boundVerts);

	const MatTransform &xformSkinToBone = shapeSkinning[shapeName].boneWeights[boneIndex].xformSkinToBone;

	bounds.center = xformSkinToBone.ApplyTransform(bounds.center);
	bounds.radius *= xformSkinToBone.scale;
	shapeSkinning[shapeName].boneWeights[boneIndex].bounds = bounds;
	return true;
}

void AnimInfo::SetWeights(const std::string& shape, const std::string& boneName, std::unordered_map<ushort, float>& inVertWeights) {
	int bid = GetShapeBoneIndex(shape, boneName);
	if (bid == 0xFFFFFFFF)
		return;

	shapeSkinning[shape].boneWeights[bid].weights = inVertWeights;
}

void AnimInfo::CleanupBones() {
	for (auto &skin : shapeSkinning) {
		std::vector<std::string> bonesToDelete;

		for (auto &bone : skin.second.boneNames) {
			bool hasInfluence = false;
			for (auto &bw : skin.second.boneWeights[bone.second].weights) {
				if (bw.second > 0.0f) {
					hasInfluence = true;
					break;
				}
			}

			if (!hasInfluence)
				bonesToDelete.push_back(bone.first);
		}

		for (auto &bone : bonesToDelete)
			RemoveShapeBone(skin.first, bone);
	}
}

void AnimInfo::WriteToNif(NifFile* nif, const std::string& shapeException) {
	for (auto &bones : shapeBones) {
		std::vector<int> bids;
		for (auto &bone : bones.second) {
			AnimBone boneRef;
			if (!AnimSkeleton::getInstance().GetBone(bone, boneRef))
				continue;

			if (bones.first == shapeException) {
				if (boneRef.refCount <= 1) {
					if (nif->CanDeleteNode(bone))
						nif->DeleteNode(bone);
				}
				continue;
			}

			int id = nif->GetBlockID(nif->FindBlockByName<NiNode>(bone));
			if (id == 0xFFFFFFFF) {
				MatTransform xform = boneRef.xformToGlobal;
				//xform.scale = 1.0f;				// Bone scaling is bad!
				id = nif->AddNode(boneRef.boneName, xform);
			}

			bids.push_back(id);
		}

		if (bones.first == shapeException)
			continue;

		auto shape = nif->FindBlockByName<NiShape>(bones.first);
		nif->SetShapeBoneIDList(shape, bids);
	}

	bool incomplete = false;
	bool isFO4 = nif->GetHeader().GetVersion().IsFO4();

	for (auto &shapeBoneList : shapeBones) {
		if (shapeBoneList.first == shapeException)
			continue;

		auto shape = nif->FindBlockByName<NiShape>(shapeBoneList.first);
		if (!shape)
			continue;

		bool isBSShape = shape->HasType<BSTriShape>();

		std::unordered_map<ushort, VertexBoneWeights> vertWeights;
		for (auto &boneName : shapeBoneList.second) {
			MatTransform xForm;
			if (AnimSkeleton::getInstance().GetBoneTransformToGlobal(boneName, xForm))
				nif->SetNodeTransformToParent(boneName, xForm, true);

			AnimBone* bptr = AnimSkeleton::getInstance().GetBonePtr(boneName);
			int bid = GetShapeBoneIndex(shapeBoneList.first, boneName);
			AnimWeight& bw = shapeSkinning[shapeBoneList.first].boneWeights[bid];

			if (isBSShape)
				for (auto vw : bw.weights)
					vertWeights[vw.first].Add(bid, vw.second);

			nif->SetShapeTransformSkinToBone(shape, bid, bw.xformSkinToBone);
			if (!bptr)
				incomplete = true;
			if (!isFO4)
				nif->SetShapeBoneWeights(shapeBoneList.first, bid, bw.weights);

			if (CalcShapeSkinBounds(shapeBoneList.first, bid))
				nif->SetShapeBoneBounds(shapeBoneList.first, bid, bw.bounds);
		}

		if (isBSShape) {
			nif->ClearShapeVertWeights(shapeBoneList.first);

			for (auto &vid : vertWeights)
				nif->SetShapeVertWeights(shapeBoneList.first, vid.first, vid.second.boneIds, vid.second.weights);
		}
	}

	if (incomplete)
		wxMessageBox(_("Bone information incomplete. Exported data will not contain correct bone entries! Be sure to load a reference NIF prior to export."), _("Export Warning"), wxICON_WARNING);
}

void AnimInfo::RenameShape(const std::string& shapeName, const std::string& newShapeName) {
	if (shapeSkinning.find(shapeName) != shapeSkinning.end()) {
		shapeSkinning[newShapeName] = std::move(shapeSkinning[shapeName]);
		shapeSkinning.erase(shapeName);
	}

	if (shapeBones.find(shapeName) != shapeBones.end()) {
		shapeBones[newShapeName] = move(shapeBones[shapeName]);
		shapeBones.erase(shapeName);
	}
}

AnimBone& AnimBone::LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* inParent)  {
	parent = inParent;
	isValidBone = false;
	auto node = skeletonNif->GetHeader().GetBlock<NiNode>(srcBlock);
	if (node)
		isValidBone = true;
	else
		return (*this);

	boneID = srcBlock;
	boneName = node->GetName();
	refCount = 0;

	xformToParent = node->GetTransformToParent();

	if (parent)
		xformToGlobal = parent->xformToGlobal.ComposeTransforms(xformToParent);
	else
		xformToGlobal = xformToParent;
	xformPoseToGlobal = xformToGlobal;

	for (auto& child : node->GetChildren()) {
		std::string name = skeletonNif->GetNodeName(child.GetIndex());
		if (!name.empty()) {
			if (name == "_unnamed_")
				name = AnimSkeleton::getInstance().GenerateBoneName();

			AnimBone& bone = AnimSkeleton::getInstance().AddBone(name).LoadFromNif(skeletonNif, child.GetIndex(), this);
			children.push_back(&bone);
		}
	}

	return (*this);
}

int AnimSkeleton::LoadFromNif(const std::string& fileName) {
	int error = refSkeletonNif.Load(fileName);
	if (error) {
		wxLogError("Failed to load skeleton '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to load skeleton '%s'!"), fileName));
		return 1;
	}

	rootBone = Config["Anim/SkeletonRootName"];
	int nodeID = refSkeletonNif.GetBlockID(refSkeletonNif.FindBlockByName<NiNode>(rootBone));
	if (nodeID == 0xFFFFFFFF) {
		wxLogError("Root '%s' not found in skeleton '%s'!", rootBone, fileName);
		wxMessageBox(wxString::Format(_("Root '%s' not found in skeleton '%s'!"), rootBone, fileName));
		return 2;
	}

	if (isValid) {
		allBones.clear();
		customBones.clear();
	}

	AddBone(rootBone).LoadFromNif(&refSkeletonNif, nodeID, nullptr);
	isValid = true;
	wxLogMessage("Loaded skeleton '%s' with root '%s'.", fileName, rootBone);
	return 0;
}

AnimBone& AnimSkeleton::AddBone(const std::string& boneName, bool bCustom) {
	if (!bCustom)
		return allBones[boneName];
	else {
		AnimBone* cb = &customBones[boneName];
		cb->boneName = boneName;
		return *cb;
	}
}

std::string AnimSkeleton::GenerateBoneName() {
	return wxString::Format("UnnamedBone_%i", unknownCount++).ToStdString();
}
	
bool AnimSkeleton::RefBone(const std::string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount++;
		return true;
	}
	if (customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount++;
		return true;
	}
	return false;
}

bool AnimSkeleton::ReleaseBone(const std::string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount--;
		return true;
	}
	if (customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount--;
		return true;
	}
	return false;
}

int AnimSkeleton::GetBoneRefCount(const std::string& boneName) {
	if (allBones.find(boneName) != allBones.end())
		return allBones[boneName].refCount;

	if (customBones.find(boneName) != customBones.end())
		return customBones[boneName].refCount;

	return 0;
}

AnimBone* AnimSkeleton::GetBonePtr(const std::string& boneName, const bool allowCustom) {
	if (allBones.find(boneName) != allBones.end())
		return &allBones[boneName];

	if (allowCustom && customBones.find(boneName) != customBones.end())
		return &customBones[boneName];

	return nullptr;
}

AnimBone* AnimSkeleton::GetRootBonePtr() {
	return &allBones[rootBone];
}

bool AnimSkeleton::GetBone(const std::string& boneName, AnimBone& outBone) {
	if (allBones.find(boneName) != allBones.end()) {
		outBone = allBones[boneName];
		return true;
	}
	if (customBones.find(boneName) != customBones.end()) {
		outBone = customBones[boneName];
		return true;
	}
	return false;
}

bool AnimSkeleton::GetBoneTransformToGlobal(const std::string &boneName, MatTransform& xform) {
	auto bone = GetBonePtr(boneName, allowCustomTransforms);
	if (!bone)
		return false;

	xform = bone->xformToGlobal;
	//xform.scale = 1.0f; // Scale should be ignored?
	return true;
}

int AnimSkeleton::GetActiveBoneNames(std::vector<std::string>& outBoneNames) {
	int c = 0;
	for (auto &ab : allBones) {
		if (ab.second.refCount > 0) {
			outBoneNames.push_back(ab.first);
			c++;
		}
	}

	for (auto &cb : customBones) {
		if (cb.second.refCount > 0) {
			outBoneNames.push_back(cb.first);
			c++;
		}
	}
	return c;
}

void AnimSkeleton::DisableCustomTransforms() {
	allowCustomTransforms = false;
}

void AnimBone::UpdatePoseTransform() {
	// this bone's pose -> this bone -> parent bone's pose -> global
	MatTransform xformPoseToBone;
	xformPoseToBone.translation = poseTranVec;
	xformPoseToBone.rotation = RotVecToMat(poseRotVec);
	MatTransform xformPoseToParent = xformToParent.ComposeTransforms(xformPoseToBone);
	if (parent)
		xformPoseToGlobal = parent->xformPoseToGlobal.ComposeTransforms(xformPoseToParent);
	else
		xformPoseToGlobal = xformPoseToParent;
	for (AnimBone *cptr : children)
		cptr->UpdatePoseTransform();
}
