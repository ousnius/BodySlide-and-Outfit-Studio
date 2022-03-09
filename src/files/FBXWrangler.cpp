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
			normElement->GetDirectArray().Add(FbxVector4((*norms)[i].x, (*norms)[i].y, (*norms)[i].z));
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

			if (child->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
				FBXShape shape;
				FbxMesh* m = (FbxMesh*)child->GetNodeAttribute();

				if (!m->IsTriangleMesh()) {
					FbxGeometryConverter converter(sdkManager);
					m = (FbxMesh*)converter.Triangulate((FbxNodeAttribute*)m, true);
				}

				FbxGeometryElementUV* uv = m->GetElementUV(0);
				FbxGeometryElementNormal* normal = m->GetElementNormal(0);

				shape.name = child->GetName();
				int numVerts = m->GetControlPointsCount();
				int numTris = m->GetPolygonCount();

				for (int v = 0; v < numVerts; v++) {
					FbxVector4 vert = m->GetControlPointAt(v);
					shape.verts.emplace_back((float)vert.mData[2], (float)vert.mData[0], (float)vert.mData[1]);
					if (uv && uv->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
						int uIndex = v;
						if (uv->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
							uIndex = uv->GetIndexArray().GetAt(v);

						shape.uvs.emplace_back((float)uv->GetDirectArray().GetAt(uIndex).mData[0], (float)uv->GetDirectArray().GetAt(uIndex).mData[1]);
					}

					if (normal && normal->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
						shape.normals.emplace_back((float)normal->GetDirectArray().GetAt(v).mData[0],
												   (float)normal->GetDirectArray().GetAt(v).mData[1],
												   (float)normal->GetDirectArray().GetAt(v).mData[2]);
					}
				}

				const char* uvName = nullptr;
				if (uv) {
					uvName = uv->GetName();
					shape.uvs.resize(numVerts);
				}

				for (int t = 0; t < numTris; t++) {
					int nverts = m->GetPolygonSize(t);
					std::vector<int> pverts(nverts);
					for (int tvi = 0; tvi < nverts; ++tvi)
						pverts[tvi] = m->GetPolygonVertex(t, tvi);

					for (int tvi = 2; tvi < nverts; ++tvi)
						shape.tris.emplace_back(pverts[0], pverts[tvi - 1], pverts[tvi]);

					if (uv && uv->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
						FbxVector2 v_uv;
						bool isUnmapped;

						for (int tvi = 0; tvi < nverts; ++tvi)
							if (m->GetPolygonVertexUV(t, tvi, uvName, v_uv, isUnmapped))
								shape.uvs[pverts[tvi]] = Vector2(v_uv.mData[0], v_uv.mData[1]);
					}
				}

				for (int iSkin = 0; iSkin < m->GetDeformerCount(FbxDeformer::eSkin); iSkin++) {
					FbxSkin* skin = (FbxSkin*)m->GetDeformer(iSkin, FbxDeformer::eSkin);

					for (int iCluster = 0; iCluster < skin->GetClusterCount(); iCluster++) {
						FbxCluster* cluster = skin->GetCluster(iCluster);
						if (!cluster->GetLink())
							continue;

						std::string bone = cluster->GetLink()->GetName();
						shape.boneNames.insert(bone);
						for (int iPoint = 0; iPoint < cluster->GetControlPointIndicesCount(); iPoint++) {
							int v = cluster->GetControlPointIndices()[iPoint];
							float w = cluster->GetControlPointWeights()[iPoint];
							shape.boneSkin[bone].SetWeight(v, w);
						}
					}
				}

				if (options.InvertU)
					for (auto& u : shape.uvs)
						u.u = 1.0f - u.u;

				if (options.InvertV)
					for (auto& v : shape.uvs)
						v.v = 1.0f - v.v;

				shapes[shape.name] = shape;
			}

			loadNodeChildren(child);
		}
	};

	FbxNode* root = scene->GetRootNode();
	loadNodeChildren(root);

	return true;
}
