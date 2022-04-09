/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "FBXWrangler.h"
#include <fbxsdk.h>

extern ConfigurationManager Config;

using namespace nifly;

// Recursive function to get a node's global default position.
// As a prerequisite, parent node's default local position must be already set.
FbxAMatrix GetGlobalDefaultPosition(FbxNode* pNode) {
	FbxAMatrix lLocalPosition;
	FbxAMatrix lGlobalPosition;
	FbxAMatrix lParentGlobalPosition;
	lLocalPosition.SetT(pNode->LclTranslation.Get());
	lLocalPosition.SetR(pNode->LclRotation.Get());
	lLocalPosition.SetS(pNode->LclScaling.Get());

	if (pNode->GetParent()) {
		lParentGlobalPosition = GetGlobalDefaultPosition(pNode->GetParent());
		lGlobalPosition = lParentGlobalPosition * lLocalPosition;
	}
	else
		lGlobalPosition = lLocalPosition;

	return lGlobalPosition;
}

// Function to set a node's local position from a global position.
// As a prerequisite, parent node's default local position must be already set.
void SetGlobalDefaultPosition(FbxNode* pNode, FbxAMatrix pGlobalPosition) {
	FbxAMatrix lLocalPosition;
	FbxAMatrix lParentGlobalPosition;

	if (pNode->GetParent()) {
		lParentGlobalPosition = GetGlobalDefaultPosition(pNode->GetParent());
		lLocalPosition = lParentGlobalPosition.Inverse() * pGlobalPosition;
	}
	else
		lLocalPosition = pGlobalPosition;

	pNode->LclTranslation.Set(lLocalPosition.GetT());
	pNode->LclRotation.Set(lLocalPosition.GetR());
	pNode->LclScaling.Set(lLocalPosition.GetS());
}

struct FBXWrangler::Priv {
	FbxManager* sdkManager = nullptr;
	FbxScene* scene = nullptr;
	std::map<std::string, FBXShape> shapes;

	// Recursively add bones to the skeleton in a depth-first manner
	FbxNode* AddLimb(FbxNode* parent, NifFile* nif, NiNode* nifBone);
	void AddLimbChildren(FbxNode* node, NifFile* nif, NiNode* nifBone);

	void AddGeometry(NiShape* shape, const std::vector<Vector3>* verts, const std::vector<Vector3>* norms, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs);

	bool LoadMeshes(const FBXImportOptions& options);
	void LoadMesh(const FBXImportOptions& options, FbxNode* node);
};

FBXWrangler::FBXWrangler()
	: priv(new Priv) {
	priv->sdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(priv->sdkManager, IOSROOT);
	priv->sdkManager->SetIOSettings(ios);

	NewScene();
}

FBXWrangler::~FBXWrangler() {
	if (priv->scene)
		CloseScene();

	if (priv->sdkManager)
		priv->sdkManager->Destroy();
}

void FBXWrangler::NewScene() {
	if (priv->scene)
		CloseScene();

	priv->scene = FbxScene::Create(priv->sdkManager, "OutfitStudioScene");
}

void FBXWrangler::CloseScene() {
	if (priv->scene)
		priv->scene->Destroy();

	priv->scene = nullptr;
	comName.clear();
}

void FBXWrangler::GetShapeNames(std::vector<std::string>& outNames) {
	for (auto& s : priv->shapes)
		outNames.push_back(s.first);
}

FBXShape* FBXWrangler::GetShape(const std::string& shapeName) {
	return &(priv->shapes[shapeName]);
}

void FBXWrangler::Priv::AddGeometry(
	NiShape* shape, const std::vector<Vector3>* verts, const std::vector<Vector3>* norms, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs) {
	if (!verts || verts->empty())
		return;

	FbxMesh* m = FbxMesh::Create(sdkManager, shape->name.get().c_str());

	FbxGeometryElementNormal* normElement = nullptr;
	if (norms && !norms->empty()) {
		normElement = m->CreateElementNormal();
		normElement->SetMappingMode(FbxLayerElement::eByControlPoint);
		normElement->SetReferenceMode(FbxLayerElement::eDirect);
	}

	FbxGeometryElementUV* uvElement = nullptr;
	if (uvs && !uvs->empty()) {
		std::string uvName = shape->name.get() + "UV";
		uvElement = m->CreateElementUV(uvName.c_str());
		uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		uvElement->SetReferenceMode(FbxGeometryElement::eDirect);
	}

	m->InitControlPoints((*verts).size());
	FbxVector4* points = m->GetControlPoints();

	for (int i = 0; i < m->GetControlPointsCount(); i++) {
		points[i] = FbxVector4((*verts)[i].y, (*verts)[i].z, (*verts)[i].x);
		if (normElement)
			normElement->GetDirectArray().Add(FbxVector4((*norms)[i].y, (*norms)[i].z, (*norms)[i].x));
		if (uvElement)
			uvElement->GetDirectArray().Add(FbxVector2((*uvs)[i].u, (*uvs)[i].v));
	}

	if (tris) {
		for (auto& t : (*tris)) {
			m->BeginPolygon();
			m->AddPolygon(t.p1);
			m->AddPolygon(t.p2);
			m->AddPolygon(t.p3);
			m->EndPolygon();
		}
	}

	FbxNode* mNode = FbxNode::Create(sdkManager, shape->name.get().c_str());
	mNode->SetNodeAttribute(m);

	// Intended for Maya
	//mNode->LclScaling.Set(FbxDouble3(1, 1, 1));
	//mNode->LclRotation.Set(FbxDouble3(-90, 0, 0));
	//mNode->LclTranslation.Set(FbxDouble3(0, 120, 0));

	FbxNode* rootNode = scene->GetRootNode();
	rootNode->AddChild(mNode);
}

void FBXWrangler::AddSkeleton(NifFile* nif, bool onlyNonSkeleton) {
	auto root = nif->FindBlockByName<NiNode>(Config["Anim/SkeletonRootName"]);
	auto com = nif->FindBlockByName<NiNode>("COM");
	if (!com)
		com = nif->FindBlockByName<NiNode>("NPC COM [COM ]");
	if (!com)
		com = nif->FindBlockByName<NiNode>("Bip01 NonAccum");

	// Likely a NIF with non-hierarchical nodes
	if (!com)
		com = nif->GetRootNode();
	if (!com)
		return;

	if (comName.empty())
		comName = com->name.get();

	// Check if skeleton already exists
	std::string skelName = "NifSkeleton";
	FbxNode* skelNode = priv->scene->GetRootNode()->FindChild(skelName.c_str());
	if (skelNode && onlyNonSkeleton) {
		// Add non-skeleton nodes to the existing skeleton
		FbxNode* comNode = skelNode->FindChild(comName.c_str());
		if (comNode) {
			std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(com);
			for (auto& b : boneNodes)
				priv->AddLimb(comNode, nif, b);
		}
	}
	else if (!skelNode) {
		// Create new skeleton
		FbxSkeleton* skel = FbxSkeleton::Create(priv->scene, skelName.c_str());
		skel->SetSkeletonType(FbxSkeleton::eRoot);

		skelNode = FbxNode::Create(priv->scene, skelName.c_str());
		skelNode->SetNodeAttribute(skel);
		//skelNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
		//skelNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerYZX);

		FbxNode* parentNode = skelNode;
		if (root) {
			FbxSkeleton* rootBone = FbxSkeleton::Create(priv->scene, root->name.get().c_str());
			rootBone->SetSkeletonType(FbxSkeleton::eLimbNode);
			rootBone->Size.Set(1.0);

			FbxNode* rootNode = FbxNode::Create(priv->scene, root->name.get().c_str());
			rootNode->SetNodeAttribute(rootBone);

			const MatTransform& ttp = root->GetTransformToParent();
			rootNode->LclTranslation.Set(FbxDouble3(ttp.translation.y, ttp.translation.z, ttp.translation.x));

			float rx, ry, rz;
			ttp.ToEulerDegrees(rx, ry, rz);
			rootNode->LclRotation.Set(FbxDouble3(ry, rz, rx));
			rootNode->LclScaling.Set(FbxDouble3(ttp.scale));

			//rootNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
			//rootNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerYZX);

			// Add root as first node
			parentNode->AddChild(rootNode);
			parentNode = rootNode;
		}

		if (com) {
			FbxSkeleton* comBone = FbxSkeleton::Create(priv->scene, com->name.get().c_str());
			comBone->SetSkeletonType(FbxSkeleton::eLimbNode);
			comBone->Size.Set(1.0);

			FbxNode* comNode = FbxNode::Create(priv->scene, com->name.get().c_str());
			comNode->SetNodeAttribute(comBone);

			const MatTransform& ttp = com->GetTransformToParent();
			comNode->LclTranslation.Set(FbxDouble3(ttp.translation.y, ttp.translation.z, ttp.translation.x));

			float rx, ry, rz;
			ttp.ToEulerDegrees(rx, ry, rz);
			comNode->LclRotation.Set(FbxDouble3(ry, rz, rx));
			comNode->LclScaling.Set(FbxDouble3(ttp.scale));

			//comNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
			//comNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerYZX);

			// Add COM as child of root
			parentNode->AddChild(comNode);
			parentNode = comNode;
		}

		std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(com);
		for (auto bn : boneNodes)
			priv->AddLimb(parentNode, nif, bn);

		priv->scene->GetRootNode()->AddChild(skelNode);
	}
}

FbxNode* FBXWrangler::Priv::AddLimb(FbxNode* parent, NifFile* nif, NiNode* nifBone) {
	FbxNode* node = scene->GetRootNode()->FindChild(nifBone->name.get().c_str());
	if (!node) {
		// Add new bone
		FbxSkeleton* bone = FbxSkeleton::Create(scene, nifBone->name.get().c_str());
		bone->SetSkeletonType(FbxSkeleton::eLimbNode);
		bone->Size.Set(1.0f);

		node = FbxNode::Create(scene, nifBone->name.get().c_str());
		node->SetNodeAttribute(bone);

		//node->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
		//node->SetRotationOrder(FbxNode::eSourcePivot, eEulerYZX);

		FbxVector4 lT, lR, lS;
		FbxAMatrix lGM;
		MatTransform xformGlobal;
		nif->GetNodeTransformToGlobal(nifBone->name.get(), xformGlobal);

		float rx, ry, rz;
		xformGlobal.ToEulerDegrees(rx, ry, rz);

		lT.Set(xformGlobal.translation.y, xformGlobal.translation.z, xformGlobal.translation.x);
		lR.Set(ry, rz, rx);
		lS.Set(xformGlobal.scale, xformGlobal.scale, xformGlobal.scale);

		lGM.SetT(lT);
		lGM.SetR(lR);
		lGM.SetS(lS);

		parent->AddChild(node);
		SetGlobalDefaultPosition(node, lGM);
	}
	else {
		// Bone already exists, but go through children and return nullptr
		AddLimbChildren(node, nif, nifBone);
		return nullptr;
	}

	AddLimbChildren(node, nif, nifBone);
	return node;
}

void FBXWrangler::Priv::AddLimbChildren(FbxNode* node, NifFile* nif, NiNode* nifBone) {
	std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(nifBone);
	for (auto& b : boneNodes)
		AddLimb(node, nif, b);
}

void FBXWrangler::AddNif(NifFile* nif, AnimInfo* anim, bool transToGlobal, NiShape* shape) {
	AddSkeleton(nif, true);

	for (auto& s : nif->GetShapes()) {
		if (!shape || s == shape) {
			std::vector<Triangle> tris;
			if (s && s->GetTriangles(tris)) {
				const std::vector<Vector3>* verts = nif->GetVertsForShape(s);
				const std::vector<Vector3>* norms = nif->GetNormalsForShape(s);
				const std::vector<Vector2>* uvs = nif->GetUvsForShape(s);

				std::vector<Vector3> gVerts, gNorms;
				if (verts && transToGlobal) {
					MatTransform toGlobal = anim->GetTransformShapeToGlobal(s);

					gVerts.resize(verts->size());
					for (size_t i = 0; i < gVerts.size(); ++i)
						gVerts[i] = toGlobal.ApplyTransform((*verts)[i]);

					verts = &gVerts;

					if (norms) {
						gNorms.resize(norms->size());
						for (size_t i = 0; i < gNorms.size(); ++i)
							gNorms[i] = toGlobal.ApplyTransformToDir((*norms)[i]);

						norms = &gNorms;
					}
				}

				priv->AddGeometry(s, verts, norms, &tris, uvs);
			}
		}
	}
}

void FBXWrangler::AddSkinning(AnimInfo* anim, NiShape* shape) {
	FbxNode* rootNode = priv->scene->GetRootNode();
	FbxNode* skelNode = rootNode->FindChild("NifSkeleton");

	if (!skelNode || !shape)
		return;

	for (auto& animSkin : anim->shapeSkinning) {
		if (shape->name != animSkin.first && !shape->name.get().empty())
			continue;

		std::string shapeName = animSkin.first;
		FbxNode* shapeNode = rootNode->FindChild(shapeName.c_str());
		if (!shapeNode)
			continue;

		std::string shapeSkin = shapeName + "_sk";
		FbxSkin* skin = FbxSkin::Create(priv->scene, shapeSkin.c_str());

		for (auto& bone : anim->shapeBones[shapeName]) {
			FbxNode* jointNode = skelNode->FindChild(bone.c_str());
			if (jointNode) {
				std::string boneSkin = bone + "_sk";
				FbxCluster* aCluster = FbxCluster::Create(priv->scene, boneSkin.c_str());
				aCluster->SetLink(jointNode);
				aCluster->SetLinkMode(FbxCluster::eTotalOne);

				auto weights = anim->GetWeightsPtr(shapeName, bone);
				if (weights) {
					for (auto& vw : *weights)
						aCluster->AddControlPointIndex(vw.first, vw.second);
				}

				FbxAMatrix lXMatrix = rootNode->EvaluateGlobalTransform();
				aCluster->SetTransformMatrix(lXMatrix);

				lXMatrix = jointNode->EvaluateGlobalTransform();
				aCluster->SetTransformLinkMatrix(lXMatrix);

				skin->AddCluster(aCluster);
			}
		}

		FbxMesh* shapeMesh = (FbxMesh*)shapeNode->GetNodeAttribute();
		if (shapeMesh)
			shapeMesh->AddDeformer(skin);
	}
}

bool FBXWrangler::ExportScene(const std::string& fileName) {
	FbxExporter* iExporter = FbxExporter::Create(priv->sdkManager, "");
	if (!iExporter->Initialize(fileName.c_str(), -1, priv->sdkManager->GetIOSettings())) {
		iExporter->Destroy();
		return false;
	}

	// Export options determine what kind of data is to be imported.
	// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
	// is true, but here we set the options explicitly.
	FbxIOSettings* ios = priv->sdkManager->GetIOSettings();
	ios->SetBoolProp(EXP_FBX_MATERIAL, true);
	ios->SetBoolProp(EXP_FBX_TEXTURE, true);
	ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
	ios->SetBoolProp(EXP_FBX_SHAPE, true);
	ios->SetBoolProp(EXP_FBX_GOBO, true);
	ios->SetBoolProp(EXP_FBX_ANIMATION, true);
	ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	iExporter->SetFileExportVersion(FBX_2014_00_COMPATIBLE);

	FbxAxisSystem axis(FbxAxisSystem::eMax);
	axis.ConvertScene(priv->scene);

	bool status = iExporter->Export(priv->scene);
	iExporter->Destroy();

	return status;
}

bool FBXWrangler::ImportScene(const std::string& fileName, const FBXImportOptions& options) {
	FbxIOSettings* ios = priv->sdkManager->GetIOSettings();
	ios->SetBoolProp(IMP_FBX_MATERIAL, true);
	ios->SetBoolProp(IMP_FBX_TEXTURE, true);
	ios->SetBoolProp(IMP_FBX_LINK, false);
	ios->SetBoolProp(IMP_FBX_SHAPE, true);
	ios->SetBoolProp(IMP_FBX_GOBO, true);
	ios->SetBoolProp(IMP_FBX_ANIMATION, true);
	ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	FbxImporter* iImporter = FbxImporter::Create(priv->sdkManager, "");
	if (!iImporter->Initialize(fileName.c_str(), -1, ios)) {
		iImporter->Destroy();
		return false;
	}

	NewScene();

	bool status = iImporter->Import(priv->scene);
	iImporter->Destroy();

	if (!status)
		return false;

	return priv->LoadMeshes(options);
}

bool FBXWrangler::Priv::LoadMeshes(const FBXImportOptions& options) {
	if (!scene)
		return false;

	std::function<void(FbxNode*)> loadNodeChildren = [&](FbxNode* root) {
		for (int i = 0; i < root->GetChildCount(); i++) {
			FbxNode* child = root->GetChild(i);

			if (child->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
				LoadMesh(options, child);

			loadNodeChildren(child);
		}
	};

	FbxNode* root = scene->GetRootNode();
	loadNodeChildren(root);

	return true;
}

void FBXWrangler::Priv::LoadMesh(const FBXImportOptions& options, FbxNode* node) {
	FBXShape shape;
	shape.name = node->GetName();

	if (!options.ImportAll && options.ImportShapes.count(shape.name) == 0)
		return;

	FbxMesh* m = (FbxMesh*)node->GetNodeAttribute();

	if (!m->IsTriangleMesh()) {
		FbxGeometryConverter converter(sdkManager);
		m = (FbxMesh*)converter.Triangulate((FbxNodeAttribute*)m, true);
	}

	FbxGeometryElementUV* geuv = m->GetElementUV(0);
	bool haveUVs = geuv && (
		geuv->GetMappingMode() == FbxGeometryElement::eByControlPoint ||
		geuv->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);
	const char* uvName = nullptr;
	if (geuv)
		uvName = geuv->GetName();

	FbxGeometryElementNormal* genrm = m->GetElementNormal(0);
	bool haveNorms = genrm && (
		genrm->GetMappingMode() == FbxGeometryElement::eByControlPoint ||
		genrm->GetMappingMode() == FbxGeometryElement::eByPolygonVertex);

	int nVerts = m->GetControlPointsCount();

	// Each vertex in the file may need to be duplicated if it's used with
	// different texture coordinates or normals.  vdata keeps track of all
	// the duplicates of a vertex.
	struct VertData {
		int tmpvi, finvi;
		Vector2 uv;
		Vector3 nrm;
	};
	std::vector<std::vector<VertData>> vdata(nVerts);

	// As we find distinct vertices, we assign each a unique temporary vertex
	// index, tmpvi.  Later, we'll renumber the vertices to preserve the file's
	// vertex ordering as well as possible; that final vertex ordering will
	// be called finvi.
	int tmpvi = 0;

	int numTris = m->GetPolygonCount();
	shape.tris.resize(numTris);
	for (int ti = 0; ti < numTris; ti++) {
		if (m->GetPolygonSize(ti) != 3)
			continue;	// Shouldn't be possible since we called Triangulate

		// For now, each of the triangle's vertex indices will be filled in
		// with the tmpvi for the vertex.  We'll update them to finvi later.
		Triangle& t = shape.tris[ti];

		for (int tvi = 0; tvi < 3; ++tvi) {
			int vi = m->GetPolygonVertex(ti, tvi);

			// Find UV
			Vector2 uv;
			if (geuv && geuv->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
				int index = vi;
				if (geuv->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
					index = geuv->GetIndexArray().GetAt(vi);

				uv.u = geuv->GetDirectArray().GetAt(index).mData[0];
				uv.v = geuv->GetDirectArray().GetAt(index).mData[1];
			}
			if (geuv && geuv->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
				FbxVector2 v_uv;
				bool isUnmapped;

				if (m->GetPolygonVertexUV(ti, tvi, uvName, v_uv, isUnmapped)) {
					uv.u = v_uv.mData[0];
					uv.v = v_uv.mData[1];
				}
			}

			// Find normal
			Vector3 nrm;
			if (genrm && genrm->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
				int index = vi;
				if (genrm->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
					index = genrm->GetIndexArray().GetAt(vi);

				nrm.x = genrm->GetDirectArray().GetAt(index).mData[2];
				nrm.y = genrm->GetDirectArray().GetAt(index).mData[0];
				nrm.z = genrm->GetDirectArray().GetAt(index).mData[1];
			}
			if (genrm && genrm->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
				FbxVector4 v_nrm;
				if (m->GetPolygonVertexNormal(ti, tvi, v_nrm)) {
					nrm.x = v_nrm.mData[2];
					nrm.y = v_nrm.mData[0];
					nrm.z = v_nrm.mData[1];
				}
			}

			// Try to find tmpvi by searching VertData
			bool found = false;
			for (VertData& vd : vdata[vi]) {
				if (haveUVs && uv != vd.uv)
					continue;
				if (haveNorms && nrm != vd.nrm)
					continue;
				found = true;
				// Found existing tmpvi
				t[tvi] = vd.tmpvi;
			}
			if (!found) {
				// New vertex: add to vdata and use new tmpvi
				vdata[vi].push_back(VertData{tmpvi, 0, uv, nrm});
				t[tvi] = tmpvi++;
			}
		}	// end tvi loop
	}	// end ti loop

	// vdata now has a complete list of vertices used by this shape.  The
	// number of vertices used is tmpvi.  We create the "final" vertex ordering
	// by going through vdata in order, assigning a finvi to each used vertex.
	// We also prepare a map from tmpvi to finvi.  We store the data
	// in shape.verts, shape.uvs, and shape.normals.
	int finvi = 0;
	std::vector<int> tmpviToFinvi(tmpvi);
	shape.verts.resize(tmpvi);
	if (haveUVs)
		shape.uvs.resize(tmpvi);
	if (haveNorms)
		shape.normals.resize(tmpvi);
	for (int filevi = 0; filevi < nVerts; ++filevi) {
		FbxVector4 fvert = m->GetControlPointAt(filevi);
		Vector3 vert;
		vert.x = fvert.mData[2];
		vert.y = fvert.mData[0];
		vert.z = fvert.mData[1];
		for (VertData& vd : vdata[filevi]) {
			tmpviToFinvi[vd.tmpvi] = finvi;
			shape.verts[finvi] = vert;
			if (haveUVs)
				shape.uvs[finvi] = vd.uv;
			if (haveNorms)
				shape.normals[finvi] = vd.nrm;
			vd.finvi = finvi;
			++finvi;
		}
	}
	// assert(finvi == tmpvi)

	// Map the triangle vertex indices from tmpvi to finvi
	for (Triangle& t : shape.tris)
		for (int tvi = 0; tvi < 3; ++tvi)
			t[tvi] = tmpviToFinvi[t[tvi]];

	// Get bone weights
	for (int iSkin = 0; iSkin < m->GetDeformerCount(FbxDeformer::eSkin); iSkin++) {
		FbxSkin* skin = (FbxSkin*)m->GetDeformer(iSkin, FbxDeformer::eSkin);

		for (int iCluster = 0; iCluster < skin->GetClusterCount(); iCluster++) {
			FbxCluster* cluster = skin->GetCluster(iCluster);
			if (!cluster->GetLink())
				continue;

			std::string bone = cluster->GetLink()->GetName();
			shape.boneNames.insert(bone);
			for (int iPoint = 0; iPoint < cluster->GetControlPointIndicesCount(); iPoint++) {
				int filevi = cluster->GetControlPointIndices()[iPoint];
				float w = cluster->GetControlPointWeights()[iPoint];
				// The single vertex in the file may have been expanded to
				// multiple vertices because of differing UV or normals.
				// Set the weight for all of these vertices.
				for (VertData& vd : vdata[filevi])
					shape.boneSkin[bone].SetWeight(vd.finvi, w);
			}
		}
	}

	if (options.InvertU)
		for (auto& uv : shape.uvs)
			uv.u = 1.0f - uv.u;

	if (options.InvertV)
		for (auto& uv : shape.uvs)
			uv.v = 1.0f - uv.v;

	Matrix4 mat;
	mat.PushScale(options.Scale, options.Scale, options.Scale);
	mat.PushRotate(options.RotateX * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
	mat.PushRotate(options.RotateY * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	mat.PushRotate(options.RotateZ * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));

	Matrix4 matRot;
	matRot.PushRotate(options.RotateX * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
	matRot.PushRotate(options.RotateY * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	matRot.PushRotate(options.RotateZ * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));

	for (auto& v : shape.verts)
		v = mat * v;

	for (auto& n : shape.normals)
		n = matRot * n;

	shapes[shape.name] = std::move(shape);
}
