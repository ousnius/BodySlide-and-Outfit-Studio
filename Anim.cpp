#include "Anim.h"
#include <sstream>
#include <wx\wx.h>

bool AnimInfo::AddShapeBone(const string& shape, AnimBone& boneDataRef) {
	for (auto bone: shapeBones[shape]) { //boneList[shape]
		if (!bone.compare(boneDataRef.boneName)) {
			return false;
		}
	}
	shapeBones[shape].push_back(boneDataRef.boneName);
	AnimSkeleton::getInstance().RefBone(boneDataRef.boneName);

	/*	
	for(auto bone: boneList[shape]) {	
		if(!bone.compare(boneDataRef.boneName)) {
			return false;
		}
	}
	boneList[shape].emplace_back(boneDataRef);
	boneList[shape].back().order = boneList[shape].size() - 1;
	boneList[shape].back().boneID = refNif->GetNodeID(boneDataRef.boneName);
	if(boneList[shape].back().boneID == -1) {
		boneList[shape].back().boneID = 
			refNif->AddNode(boneDataRef.boneName,boneDataRef.rot,boneDataRef.trans,boneDataRef.scale);
		// bone doesn't exist in destination nif yet!
	//	DebugBreak();
	}
	*/
	return true;
}

bool AnimInfo::RemoveShapeBone(const string& shape, const string& boneName) {
	int bidx = 0;
	bool found = false;
	for (auto bone: shapeBones[shape]) {
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
	return true;

	/*
	int boneID = -1;
	int boneOrder;
	int pos = 0;
	for(auto bone: boneList[shape]) {
		if(bone.boneName.compare(boneName)==0) {
			boneID = bone.boneID;
			boneOrder = bone.order;
			break;		// found it
		}
		pos ++;
	}
	if(boneID < 0) 
		return false;
	boneList[shape].erase(boneList[shape].begin() + pos);
	shapeSkinning[shape].RemoveBone(boneOrder);
	return true;
	*/
}

void AnimInfo::Clear() {
	vector<string> nifShapes;
	if (refNif && refNif->IsValid()) {
		refNif->GetShapeList(nifShapes);

		for (auto shapeBoneList: shapeBones) {
			for (auto boneName: shapeBoneList.second) {
				AnimSkeleton::getInstance().ReleaseBone(boneName);
			}
		}

		shapeSkinning.clear();
		for (auto s: nifShapes) {
			shapeBones[s].clear();
		}
		refNif = NULL;
	}
}

void AnimInfo::ClearShape(const string& shape) {
	for (auto boneName: shapeBones[shape])
		AnimSkeleton::getInstance().ReleaseBone(boneName);
	
	shapeBones.erase(shape);
	shapeSkinning.erase(shape);
}

bool AnimInfo::LoadFromNif(NifFile* nif) {
	vector<string> nifShapes;
	nif->GetShapeList(nifShapes);

	Clear();

	for (auto s: nifShapes) {
		LoadFromNif(nif, s);
	}

	refNif = nif;
	return true;
}

bool AnimInfo::LoadFromNif(NifFile* nif, const string& shape) {
	vector<string> boneNames;
	vector<int> boneIndices;
	string invalidBones = "";

	if (!nif->GetShapeBoneList(shape, boneNames))
		return false;

	int slot = 0;
	for (auto bn : boneNames) {
		if (!AnimSkeleton::getInstance().RefBone(bn)) {
			AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
			if (!cstm.isValidBone) {
				invalidBones += bn + "\n";
			}
			vector<vec3> r;
			nif->GetNodeTransform(bn, r, cstm.trans, cstm.scale);
			cstm.rot.Set(r);
			cstm.localRot = cstm.rot;
			cstm.localTrans = cstm.trans;
			AnimSkeleton::getInstance().RefBone(bn);
		}
		shapeBones[shape].push_back(bn);
		boneIndices.push_back(slot++);
	}

	shapeSkinning[shape] = AnimSkin(nif, shape, boneIndices);

	if (!invalidBones.empty())
		wxMessageBox("Bones in shape '" + shape + "' not found in reference skeleton:\n\n" + invalidBones, "Invalid Bones");

	return true;
}

void AnimInfo::GetShapeBoneXform(const string& shape, const string& boneName, skin_transform& stransform) {
	AnimBone b;
	if (AnimSkeleton::getInstance().GetBone(boneName, b)) {
		stransform.translation = b.localTrans;
		stransform.scale = b.scale;
		b.rot.GetRow(0, stransform.rotation[0]); // = b.rot[0];
		b.rot.GetRow(1, stransform.rotation[1]); // = b.rot[1];
		b.rot.GetRow(2, stransform.rotation[2]); // = b.rot[2];
	}

	/*
	int bid;
	for(auto ab : boneList[shape] ) {
		if(ab.boneName == boneName) {
			bid = ab.order;
			break;
		}
	}
	stransform = shapeSkinning[shape].boneWeights[bid].xform;
	*/
}

int AnimInfo::GetShapeBoneIndex(const string& shapeName, const string& boneName) {
	/*	
	int bid;
	for(auto ab : boneList[shapeName] ) {
		if(ab.boneName == boneName) {
			bid = ab.order;
			break;
		}
	}
	return bid;
	*/
	int b = -1;
	for (int i = 0; i < shapeBones[shapeName].size(); i++) {
		if (shapeBones[shapeName][i] == boneName) {
			b = i;
			break;
		}
	}

	return b;
}

void AnimInfo::GetWeights(const string& shape, const string& boneName, unordered_map<int, float>& outVertWeights) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0) return;

	outVertWeights = shapeSkinning[shape].boneWeights[b].weights;
}

void AnimInfo::SetShapeBoneXform(const string& shape, const string& boneName, skin_transform& stransform) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0) return;

	shapeSkinning[shape].boneWeights[b].xform = stransform;
}

void AnimInfo::SetWeights(const string& shape, const string& boneName, unordered_map<int, float>& inVertWeights) {
	int bid = GetShapeBoneIndex(shape, boneName);
	
	if (bid == -1) {
		if (inVertWeights.size() != 0) {
			//wxMessageBox("Weights assigned to a bone ("+boneName+") that doesn't exist for a shape ("+shape+")");
		}
		return;
	}
	if (bid == 0xcccccccc) {
		//wxMessageBox("shit! ("+boneName+") got here ("+shape+")");
	}

	shapeSkinning[shape].boneWeights[bid].weights.clear();
	shapeSkinning[shape].boneWeights[bid].weights = inVertWeights;

	if (refNif && refNif->IsValid()) {
		vector <vec3> v;
		vec3 p;
		vec3 a(FLT_MAX,FLT_MAX,FLT_MAX);
		vec3 b(-FLT_MAX,-FLT_MAX,-FLT_MAX);
		vec3 tot;
		float d = 0.0f;
		skin_transform st;
		vec3 center;
		refNif->GetVertsForShape(shape,v);
		Mat4 t(shapeSkinning[shape].boneWeights[bid].xform.ToMatrix());
			
		for (auto w : inVertWeights) {
			p = v[w.first];
			a.x = min(a.x, p.x); a.y = min(a.y, p.y); a.z = min(a.z, p.z);
			b.x = max(b.x, p.x); b.y = max(b.y, p.y); b.z = max(b.z, p.z);
		}
		tot =  (a + b ) / 2.0f;		
		for (auto w: inVertWeights) {
			p = v[w.first];
			d = max(d, tot.DistanceTo(p));
		}
		tot = t * tot;
		shapeSkinning[shape].boneWeights[bid].bSphereOffset = tot;
		shapeSkinning[shape].boneWeights[bid].bSphereRadius = d;
	}
}

void AnimInfo::WriteToNif(NifFile* nif, bool synchBoneIDs) {
	/*int bid;
	for(auto shapeBones: boneList) {
		for(auto &bone: shapeBones.second) {
			bid = nif->GetNodeID(bone.boneName);
			if(bid == -1) {
				bid = nif->AddNode(bone.boneName,bone.rot,bone.trans,bone.scale);
			}
			if(bid != bone.boneID) {
				nif->UpdateShapeBoneID(shapeBones.first,bone.boneID, bid);
				bone.boneID = bid;
			}
		}
	}
	*/
/* old way --
	if(synchBoneIDs) {
		vector<int> bids;
		int id;
		AnimBone boneref;
		for(auto shapeBones : boneList) {
			bids.clear();
			for(auto bone: shapeBones.second) {
				id = nif->GetNodeID(bone.boneName);
				if(id == -1) {
					if(!AnimSkeleton::getInstance().GetBone(bone.boneName,boneref)) {
	//					wxMessageBox("Invalid bone name attempted to install: " + bone.boneName);
						return;
					}
					id =nif->AddNode(boneref.boneName,boneref.rot,boneref.localTrans,boneref.scale);
					return;
				}

				bids.push_back(id);
			}
			nif->SetShapeBoneIDList(shapeBones.first,bids);
		}
	}
 -- */	
	if (synchBoneIDs) {
		vector<int> bids;
		int id;
		AnimBone boneref;
		for (auto bones: shapeBones) {
			bids.clear();
			for (auto bone: bones.second) {
				id = nif->GetNodeID(bone);
				if (id == -1) {
					if (!AnimSkeleton::getInstance().GetBone(bone, boneref)) {
						//wxMessageBox("Could not write bone from reference skeleton: " + bone);
						continue;
					}
					if (boneref.refCount == 0)
						continue;

					vector<vec3> r(3);
					boneref.rot.GetRow(0, r[0]);
					boneref.rot.GetRow(1, r[1]);
					boneref.rot.GetRow(2, r[2]);
					boneref.scale = 1.0f;			// bone scaling is bad!
					id = nif->AddNode(boneref.boneName, r, boneref.trans, boneref.scale);
				}

				bids.push_back(id);
			}
			nif->SetShapeBoneIDList(bones.first, bids);
		}
	}

	skin_transform xform;
	for (auto shapeBoneList: shapeBones) {
		for (auto boneName: shapeBoneList.second) {
			if (!AnimSkeleton::getInstance().GetBoneTransform(boneName, xform)) {
	 			//wxMessageBox("Error! Attempted to access bone that does not exist in reference skeleton: " + boneName);
				continue;
			}
			nif->SetNodeTransform(boneName, xform);

			int bid = GetShapeBoneIndex(shapeBoneList.first, boneName);
			AnimWeight& bw = shapeSkinning[shapeBoneList.first].boneWeights[bid];
			if (AnimSkeleton::getInstance().GetSkinTransform(boneName, xform)) {
				nif->SetShapeBoneTransform(shapeBoneList.first, bid, xform, bw.bSphereOffset, bw.bSphereRadius);
				nif->SetShapeBoneWeights(shapeBoneList.first, bid, bw.weights);
			}
		}
	}

	/* -- old way -- * /
	for(auto sk : shapeSkinning) {
		for(auto bw: sk.second.boneWeights) {
			ab = &boneList[sk.first][bw.first];
			xform = bw.second.xform;
			//xform.rotation[0] = ab->rot[0];  xform.rotation[1] = ab->rot[1]; 	xform.rotation[2] = ab->rot[2];
		//	xform.translation = ab->trans;
		//	xform.scale = ab->scale;
			nif->SetShapeBoneTransform(sk.first, bw.first, xform, bw.second.bSphereOffset, bw.second.bSphereRadius);
			nif->SetShapeBoneWeights(sk.first, bw.first, bw.second.weights);
		}
	}
	*/
}

/*
AnimBone* AnimInfo::GetShapeBone(const string& shape, const string& boneName) {
	if(shapeBones.find(shape) == shapeBones.end()) {
		return NULL;
	}
	for(auto bone: shapeBones[shape]){
		if(bone == boneName) {

		}

	}
	/*

	auto bl = boneList.find(shape);
	if(bl == boneList.end()) return NULL;
	for(int i=0;i< bl->second.size();i++) {
		if(bl->second[i].boneName == boneName) {
			return &(bl->second[i]);
		}
	}
	return NULL;
	* /
}
	
*/

void AnimInfo::RenameShape(const string& shapeName, const string& newShapeName) {
	if (shapeSkinning.find(shapeName) != shapeSkinning.end()) {
		shapeSkinning[newShapeName] = move(shapeSkinning[shapeName]);
		shapeSkinning.erase(shapeName);
	}

	if (shapeBones.find(shapeName) != shapeBones.end()) {
		shapeBones[newShapeName] = move(shapeBones[shapeName]);
		shapeBones.erase(shapeName);
	}

	/*
	if(boneList.find(shapeName) != boneList.end()) {
		boneList[newShapeName] = move(boneList[shapeName]);
		boneList.erase(shapeName);
	}
	*/
}

AnimBone& AnimBone::LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* inParent)  {
	parent = inParent;
	isValidBone = false;
	NifBlockNiNode* node = dynamic_cast<NifBlockNiNode*>(skeletonNif->GetBlock(srcBlock));
	if (node)
		isValidBone = true;
	else
		return (*this);

	boneID = srcBlock;
	order = -1;
	refCount = 0;
	if (inParent && inParent->boneName == "NPC Spine [Spn0]") {
		refCount = 0;
	}
	
	boneName = node->nodeName;
	vector<vec3> m33;
	m33.push_back(node->rotation[0]);
	m33.push_back(node->rotation[1]);
	m33.push_back(node->rotation[2]);
	localRot.Set(m33);
	localTrans = node->translation;
	scale = node->scale;					// Need to ignore scale ?
	if (parent) {
		trans = parent->trans + (parent->rot * localTrans);
		rot = parent->rot * localRot;
	} else {
		trans = localTrans;
		rot = localRot;
	}
	for (auto c: node->children) {
		string name = skeletonNif->NodeName(c);
		if (!name.empty()){
			if (name == "_unnamed_") {
				name = AnimSkeleton::getInstance().GenerateBoneName();
			}

			AnimBone& bone = AnimSkeleton::getInstance().AddBone(name).LoadFromNif(skeletonNif, c, this);
		    children.push_back(&bone);		
		}
	}
	return (*this);
}

int AnimSkeleton::LoadFromNif(const string& filename) {
	NifFile nif;
	nif.Load(filename);
	if (!nif.IsValid()) {
		return 1;
	}

	rootBone = Config.GetCString("Anim/SkeletonRootName", "NPC");
	int nodeid = nif.GetNodeID(rootBone);
	if (nodeid == -1){
		return 2;
	}
	if (isValid) {
		allBones.clear();
	}

	AddBone(rootBone).LoadFromNif(&nif, nodeid, NULL);
	isValid = true;
	return 0;
}

AnimBone& AnimSkeleton::AddBone(const string& boneName, bool bCustom) {
	if (!bCustom) 
		return allBones[boneName];
	else if (allowCustom) 
		return customBones[boneName];
	else
		return invBone;
}

string AnimSkeleton::GenerateBoneName() {
	stringstream ss;
	ss << "UnnamedBone_" << unknownCount ++;

	return ss.str();
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


AnimBone* AnimSkeleton::GetBonePtr(const string& boneName) {
	if (boneName.empty()) {
		return &allBones[rootBone];
	}

	if (allBones.find(boneName) != allBones.end()) {
		return &allBones[boneName];
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		return &customBones[boneName];
	}
	return NULL;
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

bool AnimSkeleton::GetBoneTransform(const string &boneName, skin_transform& xform) {
	if (allBones.find(boneName) == allBones.end()) {
		return false;
	}
	AnimBone* cB = &allBones[boneName];
	Mat4 t = cB->rot;
	t.GetRow(0, xform.rotation[0]);
	t.GetRow(1, xform.rotation[1]);
	t.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f; //cB->scale		// scale should be ignored?
	xform.translation = cB->trans;
//	xform.translation = cB->trans * -1.0f;
//	xform.translation = t * xform.translation;
	return true;

}
bool AnimSkeleton::GetSkinTransform(const string &boneName, skin_transform& xform) {
	if (allBones.find(boneName) == allBones.end()) {
		return false;
	}
	AnimBone* cB = &allBones[boneName];
	Mat4 t = cB->rot.Inverse();
	t.GetRow(0, xform.rotation[0]);
	t.GetRow(1, xform.rotation[1]);
	t.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f;	//cB->scale		// scale should be ignored?
	xform.translation = cB->trans * -1.0f;
	xform.translation = t * xform.translation;
	return true;
}

int AnimSkeleton::GetActiveBoneNames(vector<string>& outBoneNames) {
	int c = 0;
	for (auto ab: allBones) {
		if (ab.second.refCount > 0) {
			outBoneNames.push_back(ab.first);
			c++;
		}
	}
	return c;
}