/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Anim.h"
#include <wx/log.h>
#include <wx/msgdlg.h>

bool AnimInfo::AddShapeBone(const string& shape, AnimBone& boneDataRef) {
	for (auto &bone : shapeBones[shape])
		if (!bone.compare(boneDataRef.boneName))
			return false;

	shapeSkinning[shape].boneNames[boneDataRef.boneName] = shapeBones[shape].size();
	shapeBones[shape].push_back(boneDataRef.boneName);
	AnimSkeleton::getInstance().RefBone(boneDataRef.boneName);
	return true;
}

bool AnimInfo::RemoveShapeBone(const string& shape, const string& boneName) {
	int bidx = 0;
	bool found = false;
	for (auto &bone : shapeBones[shape]) {
		if (bone.compare(boneName) == 0) {
			found = true;
			break;
		}
		bidx++;
	}

	if (!found)
		return false;

	shapeBones[shape].erase(shapeBones[shape].begin() + bidx);
	shapeSkinning[shape].RemoveBone(bidx);

	AnimSkeleton::getInstance().ReleaseBone(boneName);
	if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0)
		refNif->DeleteNode(boneName);

	return true;
}

void AnimInfo::Clear() {
	if (refNif && refNif->IsValid()) {
		vector<string> shapes;
		refNif->GetShapeList(shapes);

		for (auto &shapeBoneList : shapeBones) {
			for (auto &boneName : shapeBoneList.second) {
				AnimSkeleton::getInstance().ReleaseBone(boneName);
				if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0)
					refNif->DeleteNode(boneName);
			}
		}

		shapeSkinning.clear();
		for (auto &s : shapes)
			shapeBones[s].clear();

		refNif = nullptr;
	}
}

void AnimInfo::ClearShape(const string& shape) {
	for (auto &boneName : shapeBones[shape]) {
		AnimSkeleton::getInstance().ReleaseBone(boneName);
		if (AnimSkeleton::getInstance().GetBoneRefCount(boneName) <= 0)
			refNif->DeleteNode(boneName);
	}

	shapeBones.erase(shape);
	shapeSkinning.erase(shape);
}

void AnimInfo::DeleteVertsForShape(const string& shape, const vector<ushort>& indices) {
	ushort highestRemoved = indices.back();
	vector<int> indexCollapse(highestRemoved + 1, 0);

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
		unordered_map<ushort, float> indexCopy;
		for (auto &d : w.second.weights) {
			if (d.first > highestRemoved)
				indexCopy.emplace(d.first - remCount, d.second);
			else if (indexCollapse[d.first] != -1)
				indexCopy.emplace(d.first - indexCollapse[d.first], d.second);
		}

		w.second.weights.clear();
		w.second.weights.reserve(indexCopy.size());
		for (auto &copy : indexCopy)
			w.second.weights[copy.first] = move(copy.second);
	}
}

bool AnimInfo::LoadFromNif(NifFile* nif) {
	vector<string> shapes;
	nif->GetShapeList(shapes);

	Clear();

	for (auto &s : shapes)
		LoadFromNif(nif, s);

	refNif = nif;
	return true;
}

bool AnimInfo::LoadFromNif(NifFile* nif, const string& shape, bool newRefNif) {
	vector<string> boneNames;
	string nonRefBones;

	if (newRefNif)
		refNif = nif;

	if (!nif->GetShapeBoneList(shape, boneNames)) {
		wxLogWarning("No skinning found in shape '%s' of '%s'.", shape, nif->GetFileName());
		return false;
	}

	for (auto &bn : boneNames) {
		if (!AnimSkeleton::getInstance().RefBone(bn)) {
			AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
			if (!cstm.isValidBone)
				nonRefBones += bn + "\n";

			vector<Vector3> r;
			nif->GetNodeTransform(bn, r, cstm.trans, cstm.scale);
			cstm.rot.Set(r);
			cstm.localRot = cstm.rot;
			cstm.localTrans = cstm.trans;

			AnimSkeleton::getInstance().RefBone(bn);
		}

		AnimBone* bonePtr = AnimSkeleton::getInstance().GetBonePtr(bn);
		if (bonePtr && !bonePtr->hasSkinXform) {
			SkinTransform shapeskinxform;
			if (nif->GetShapeBoneTransform(shape, bn, shapeskinxform)) {
				bonePtr->skinRot.Set(shapeskinxform.rotation);
				bonePtr->skinTrans = shapeskinxform.translation;
				bonePtr->hasSkinXform = true;
			}
		}
		shapeBones[shape].push_back(bn);
	}

	shapeSkinning[shape] = AnimSkin(nif, shape);

	if (!nonRefBones.empty())
		wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", shape, nonRefBones);

	return true;
}

void AnimInfo::GetBoneXForm(const string& boneName, SkinTransform& stransform) {
	AnimBone b;
	if (AnimSkeleton::getInstance().GetBone(boneName, b)) {
		stransform.translation = b.localTrans;
		stransform.scale = b.scale;
		b.rot.GetRow(0, stransform.rotation[0]); // = b.rot[0];
		b.rot.GetRow(1, stransform.rotation[1]); // = b.rot[1];
		b.rot.GetRow(2, stransform.rotation[2]); // = b.rot[2];
	}
}

int AnimInfo::GetShapeBoneIndex(const string& shapeName, const string& boneName) {
	int b = 0xFFFFFFFF;
	for (int i = 0; i < shapeBones[shapeName].size(); i++) {
		if (shapeBones[shapeName][i] == boneName) {
			b = i;
			break;
		}
	}

	return b;
}

void AnimInfo::GetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& outVertWeights) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return;

	outVertWeights = shapeSkinning[shape].boneWeights[b].weights;
}

void AnimInfo::SetShapeBoneXForm(const string& shape, const string& boneName, SkinTransform& stransform) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return;

	shapeSkinning[shape].boneWeights[b].xform = stransform;
}

bool AnimInfo::CalcShapeSkinBounds(const string& shape, const int& boneIndex) {
	if (!refNif || !refNif->IsValid())	// Check for existence of reference nif
		return false;

	if (shapeSkinning.find(shape) == shapeSkinning.end())	// Check for shape in skinning data
		return false;

	vector<Vector3> verts;
	refNif->GetVertsForShape(shape, verts);
	if (verts.size() == 0)	// Check for empty shape
		return false;

	vector<Vector3> boundVerts;
	for (auto &w : shapeSkinning[shape].boneWeights[boneIndex].weights) {
		if (w.first > verts.size())		// Incoming weights have a larger set of possible verts.
			return false;

		boundVerts.push_back(verts[w.first]);
	}

	Matrix4 mat;
	Vector3 trans;
	BoundingSphere bounds(boundVerts);

	mat = shapeSkinning[shape].boneWeights[boneIndex].xform.ToMatrix();

	bounds.center = mat * bounds.center;
	bounds.center = bounds.center + trans;
	shapeSkinning[shape].boneWeights[boneIndex].bounds = bounds;
	return true;
}

void AnimInfo::SetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& inVertWeights) {
	int bid = GetShapeBoneIndex(shape, boneName);
	if (bid == 0xFFFFFFFF)
		return;

	shapeSkinning[shape].boneWeights[bid].weights.clear();
	shapeSkinning[shape].boneWeights[bid].weights = inVertWeights;
}

void AnimInfo::WriteToNif(NifFile* nif, bool synchBoneIDs, const string& shapeException) {
	if (synchBoneIDs) {
		for (auto &bones : shapeBones) {
			vector<int> bids;
			for (auto &bone : bones.second) {
				AnimBone boneRef;
				if (!AnimSkeleton::getInstance().GetBone(bone, boneRef))
					continue;

				if (bones.first == shapeException) {
					if (boneRef.refCount <= 1)
						nif->DeleteNode(bone);
					continue;
				}

				int id = nif->GetBlockID(nif->FindNodeByName(bone));
				if (id == 0xFFFFFFFF) {
					vector<Vector3> r(3);
					boneRef.rot.GetRow(0, r[0]);
					boneRef.rot.GetRow(1, r[1]);
					boneRef.rot.GetRow(2, r[2]);
					boneRef.scale = 1.0f;				// Bone scaling is bad!
					id = nif->AddNode(boneRef.boneName, r, boneRef.trans, boneRef.scale);
				}

				bids.push_back(id);
			}

			if (bones.first == shapeException)
				continue;

			nif->SetShapeBoneIDList(bones.first, bids);
		}
	}

	bool incomplete = false;
	bool isFO4 = (nif->GetHeader().GetUserVersion() >= 12 && nif->GetHeader().GetUserVersion2() == 130);

	for (auto &shapeBoneList : shapeBones) {
		if (shapeBoneList.first == shapeException)
			continue;

		int stype = nif->GetShapeType(shapeBoneList.first);
		if (stype == NIUNKNOWN)
			continue;

		bool isBSShape = (stype == BSTRISHAPE || stype == BSSUBINDEXTRISHAPE || stype == BSMESHLODTRISHAPE || stype == BSDYNAMICTRISHAPE);

		unordered_map<ushort, VertexBoneWeights> vertWeights;
		for (auto &boneName : shapeBoneList.second) {
			SkinTransform xForm;
			if (AnimSkeleton::getInstance().GetBoneTransform(boneName, xForm))
				nif->SetNodeTransform(boneName, xForm, true);

			AnimBone* bptr = AnimSkeleton::getInstance().GetBonePtr(boneName);
			int bid = GetShapeBoneIndex(shapeBoneList.first, boneName);
			AnimWeight& bw = shapeSkinning[shapeBoneList.first].boneWeights[bid];

			if (isBSShape)
				for (auto vw : bw.weights)
					vertWeights[vw.first].Add(bid, vw.second);

			if (isFO4) {
				if (!bptr || !bptr->hasSkinXform) {
					incomplete = true;
					nif->SetShapeBoneTransform(shapeBoneList.first, bid, bw.xform);
				}
				else {
					SkinTransform st;
					st.rotation[0] = Vector3(bptr->skinRot[0], bptr->skinRot[1], bptr->skinRot[2]);
					st.rotation[1] = Vector3(bptr->skinRot[4], bptr->skinRot[5], bptr->skinRot[6]);
					st.rotation[2] = Vector3(bptr->skinRot[8], bptr->skinRot[9], bptr->skinRot[10]);
					st.translation = bptr->skinTrans;
					nif->SetShapeBoneTransform(shapeBoneList.first, bid, st);
					bw.xform = st;
				}
			}
			else {
				if (!bptr || !bptr->hasSkinXform) {
					incomplete = true;
					nif->SetShapeBoneTransform(shapeBoneList.first, bid, bw.xform);
				}
				else {
					SkinTransform xFormSkin;
					if (nif->GetShapeBoneTransform(shapeBoneList.first, 0xFFFFFFFF, xFormSkin)) {
						if (AnimSkeleton::getInstance().GetSkinTransform(boneName, xFormSkin, xForm)) {
							nif->SetShapeBoneTransform(shapeBoneList.first, bid, xForm);
							bw.xform = xForm;
						}
					}
				}

				nif->SetShapeBoneWeights(shapeBoneList.first, bid, bw.weights);
			}

			if (CalcShapeSkinBounds(shapeBoneList.first, bid))
				nif->SetShapeBoneBounds(shapeBoneList.first, bid, bw.bounds);
		}

		if (isBSShape)
			for (auto &vid : vertWeights)
				nif->SetShapeVertWeights(shapeBoneList.first, vid.first, vid.second.boneIds, vid.second.weights);
	}

	if (incomplete)
		wxMessageBox(_("Bone information incomplete. Exported data will not contain correct bone entries! Be sure to load a reference NIF prior to export."), _("Export Warning"), wxICON_WARNING);
}

void AnimInfo::RenameShape(const string& shapeName, const string& newShapeName) {
	if (shapeSkinning.find(shapeName) != shapeSkinning.end()) {
		shapeSkinning[newShapeName] = move(shapeSkinning[shapeName]);
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

	localRot.Set(node->rotation);
	localTrans = node->translation;
	scale = node->scale;

	if (parent) {
		trans = parent->trans + (parent->rot * localTrans);
		rot = parent->rot * localRot;
	}
	else {
		trans = localTrans;
		rot = localRot;
	}

	for (int i = 0; i < node->GetNumChildren(); i++) {
		string name = skeletonNif->GetNodeName(node->GetChildRef(i));
		if (!name.empty()) {
			if (name == "_unnamed_")
				name = AnimSkeleton::getInstance().GenerateBoneName();

			AnimBone& bone = AnimSkeleton::getInstance().AddBone(name).LoadFromNif(skeletonNif, node->GetChildRef(i), this);
			children.push_back(&bone);
		}
	}

	return (*this);
}

int AnimSkeleton::LoadFromNif(const string& fileName) {
	int error = refSkeletonNif.Load(fileName);
	if (error) {
		wxLogError("Failed to load skeleton '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to load skeleton '%s'!"), fileName));
		return 1;
	}

	rootBone = Config["Anim/SkeletonRootName"];
	int nodeID = refSkeletonNif.GetBlockID(refSkeletonNif.FindNodeByName(rootBone));
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

AnimBone& AnimSkeleton::AddBone(const string& boneName, bool bCustom) {
	if (!bCustom)
		return allBones[boneName];
	else if (allowCustom) {
		AnimBone* cb = &customBones[boneName];
		cb->boneName = boneName;
		return *cb;
	}
	else
		return invBone;
}

string AnimSkeleton::GenerateBoneName() {
	return wxString::Format("UnnamedBone_%i", unknownCount++).ToStdString();
}
	
bool AnimSkeleton::RefBone(const string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount++;
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount++;
		return true;
	}
	return false;
}

bool AnimSkeleton::ReleaseBone(const string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount--;
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount--;
		return true;
	}
	return false;
}

int AnimSkeleton::GetBoneRefCount(const string& boneName) {
	if (allBones.find(boneName) != allBones.end())
		return allBones[boneName].refCount;

	if (allowCustom && customBones.find(boneName) != customBones.end())
		return customBones[boneName].refCount;

	return 0;
}

AnimBone* AnimSkeleton::GetBonePtr(const string& boneName) {
	if (boneName.empty())
		return &allBones[rootBone];

	if (allBones.find(boneName) != allBones.end())
		return &allBones[boneName];

	if (allowCustom && customBones.find(boneName) != customBones.end())
		return &customBones[boneName];

	return nullptr;
}

bool AnimSkeleton::GetBone(const string& boneName, AnimBone& outBone) {
	if (allBones.find(boneName) != allBones.end()) {
		outBone = allBones[boneName];
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		outBone = customBones[boneName];
		return true;
	}
	return false;
}

bool AnimSkeleton::GetBoneTransform(const string &boneName, SkinTransform& xform) {
	if (allBones.find(boneName) == allBones.end())
		return false;

	AnimBone* cB = &allBones[boneName];
	Matrix4 rot = cB->rot;
	rot.GetRow(0, xform.rotation[0]);
	rot.GetRow(1, xform.rotation[1]);
	rot.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f; //cB->scale					// Scale should be ignored?
	xform.translation = cB->trans;
	//xform.translation = cB->trans * -1.0f;
	//xform.translation = rot * xform.translation;
	return true;
}

bool AnimSkeleton::GetSkinTransform(const string &boneName, const SkinTransform& skinning, SkinTransform& xform) {
	if (allBones.find(boneName) == allBones.end())
		return false;

	AnimBone* cB = &allBones[boneName];
	Matrix4 rot = cB->rot.Inverse();
	rot.GetRow(0, xform.rotation[0]);
	rot.GetRow(1, xform.rotation[1]);
	rot.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f;	//cB->scale					// Scale should be ignored?
	xform.translation = rot * (cB->trans * -1.0f + skinning.translation * -1.0f);
	return true;
}

int AnimSkeleton::GetActiveBoneNames(vector<string>& outBoneNames) {
	int c = 0;
	for (auto &ab : allBones) {
		if (ab.second.refCount > 0) {
			outBoneNames.push_back(ab.first);
			c++;
		}
	}

	if (allowCustom) {
		for (auto &cb : customBones) {
			if (cb.second.refCount > 0) {
				outBoneNames.push_back(cb.first);
				c++;
			}
		}
	}
	return c;
}
