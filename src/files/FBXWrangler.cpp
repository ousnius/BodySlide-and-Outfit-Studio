/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "FBXWrangler.h"

FBXWrangler::FBXWrangler() {
	sdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	NewScene();
}


FBXWrangler::~FBXWrangler() {
	if (scene)
		CloseScene();

	if (sdkManager)
		sdkManager->Destroy();
}

void FBXWrangler::NewScene() {
	if (scene)
		CloseScene();

	scene = FbxScene::Create(sdkManager, "OutfitStudioScene");
}

void FBXWrangler::CloseScene() {
	if (scene)
		scene->Destroy();
	
	scene = nullptr;
	comName.clear();
}

void FBXWrangler::AddGeometry(const std::string& shapeName, const std::vector<Vector3>* verts, const std::vector<Vector3>* norms, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs) {
	if (!verts || verts->empty())
		return;

	FbxMesh* m = FbxMesh::Create(sdkManager, shapeName.c_str());
	
	FbxGeometryElementNormal* normElement = nullptr;
	if (norms && !norms->empty()) {
		normElement = m->CreateElementNormal();
		normElement->SetMappingMode(FbxLayerElement::eByControlPoint);
		normElement->SetReferenceMode(FbxLayerElement::eDirect);
	}
	
	FbxGeometryElementUV* uvElement = nullptr;
	if (uvs && !uvs->empty()) {
		std::string uvName = shapeName + "UV";
		uvElement = m->CreateElementUV(uvName.c_str());
		uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		uvElement->SetReferenceMode(FbxGeometryElement::eDirect);
	}

	m->InitControlPoints((*verts).size());
	FbxVector4* points = m->GetControlPoints();

	for (int i = 0; i < m->GetControlPointsCount(); i++) {
		points[i] = FbxVector4((*verts)[i].x, (*verts)[i].y, (*verts)[i].z);
		if (normElement)
			normElement->GetDirectArray().Add(FbxVector4((*norms)[i].x, (*norms)[i].y, (*norms)[i].z));
		if (uvElement)
			uvElement->GetDirectArray().Add(FbxVector2((*uvs)[i].u, (*uvs)[i].v));
	}

	if (tris) {
		for (auto &t : (*tris)) {
			m->BeginPolygon();
			m->AddPolygon(t.p1);
			m->AddPolygon(t.p2);
			m->AddPolygon(t.p3);
			m->EndPolygon();
		}
	}

	FbxNode* mNode = FbxNode::Create(sdkManager, shapeName.c_str());
	mNode->SetNodeAttribute(m);

	// Intended for Maya
	mNode->LclScaling.Set(FbxDouble3(1, 1, 1));
	mNode->LclRotation.Set(FbxDouble3(-90, 0, 0));
	mNode->LclTranslation.Set(FbxDouble3(0, 120, 0));

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
		comName = com->GetName();

	// Check if skeleton already exists
	std::string skelName = "NifSkeleton";
	FbxNode* skelNode = scene->GetRootNode()->FindChild(skelName.c_str());
	if (skelNode && onlyNonSkeleton) {
		// Add non-skeleton nodes to the existing skeleton
		FbxNode* comNode = skelNode->FindChild(comName.c_str());
		if (comNode) {
			std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(com);
			for (auto &b : boneNodes)
				comNode->AddChild(AddLimb(nif, b));
		}
	}
	else if (!skelNode) {
		// Create new skeleton
		FbxSkeleton* skel = FbxSkeleton::Create(scene, skelName.c_str());
		skel->SetSkeletonType(FbxSkeleton::eRoot);

		skelNode = FbxNode::Create(scene, skelName.c_str());
		skelNode->SetNodeAttribute(skel);
		skelNode->LclTranslation.Set(FbxDouble3(0.0, 0.0, 0.0));
		//skelNode->LclRotation.Set(FbxDouble3(0.0, 0.0, 0.0));
		//skelNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);

		FbxNode* parentNode = skelNode;
		if (root) {
			FbxSkeleton* rootBone = FbxSkeleton::Create(scene, root->GetName().c_str());
			rootBone->SetSkeletonType(FbxSkeleton::eLimbNode);
			rootBone->Size.Set(1.0);

			FbxNode* rootNode = FbxNode::Create(scene, root->GetName().c_str());
			rootNode->SetNodeAttribute(rootBone);

			rootNode->LclTranslation.Set(FbxDouble3(root->transform.translation.x, root->transform.translation.y, root->transform.translation.z));

			float rx, ry, rz;
			root->transform.ToEulerDegrees(rx, ry, rz);
			rootNode->LclRotation.Set(FbxDouble3(rx, ry, rz));
			//rootNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);

			// Add root as first node
			parentNode->AddChild(rootNode);
			parentNode = rootNode;
		}

		if (com) {
			FbxSkeleton* comBone = FbxSkeleton::Create(scene, com->GetName().c_str());
			comBone->SetSkeletonType(FbxSkeleton::eLimbNode);
			comBone->Size.Set(1.0);

			FbxNode* comNode = FbxNode::Create(scene, com->GetName().c_str());
			comNode->SetNodeAttribute(comBone);

			comNode->LclTranslation.Set(FbxDouble3(com->transform.translation.y, com->transform.translation.z, com->transform.translation.x));

			float rx, ry, rz;
			com->transform.ToEulerDegrees(rx, ry, rz);
			comNode->LclRotation.Set(FbxDouble3(rx, ry, rz));
			//comNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);

			// Add COM as child of root
			parentNode->AddChild(comNode);
			parentNode = comNode;
		}

		std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(com);
		for (auto bn : boneNodes)
			parentNode->AddChild(AddLimb(nif, bn));

		scene->GetRootNode()->AddChild(skelNode);
	}
}

FbxNode* FBXWrangler::AddLimb(NifFile* nif, NiNode* nifBone) {
	FbxNode* node = scene->GetRootNode()->FindChild(nifBone->GetName().c_str());
	if (!node) {
		// Add new bone
		FbxSkeleton* bone = FbxSkeleton::Create(scene, nifBone->GetName().c_str());
		bone->SetSkeletonType(FbxSkeleton::eLimbNode);
		bone->Size.Set(1.0f);

		node = FbxNode::Create(scene, nifBone->GetName().c_str());
		node->SetNodeAttribute(bone);

		Vector3 translation = nifBone->transform.translation;
		node->LclTranslation.Set(FbxDouble3(translation.x, translation.y, translation.z));

		float rx, ry, rz;
		nifBone->transform.ToEulerDegrees(rx, ry, rz);
		node->LclRotation.Set(FbxDouble3(rx, ry, rz));
		//myNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);
	}
	else {
		// Bone already exists, but go through children and return nullptr
		AddLimbChildren(node, nif, nifBone);
		return nullptr;
	}

	AddLimbChildren(node, nif, nifBone);
	return node;
}

void FBXWrangler::AddLimbChildren(FbxNode* node, NifFile* nif, NiNode* nifBone) {
	std::vector<NiNode*> boneNodes = nif->GetChildren<NiNode>(nifBone);
	for (auto &b : boneNodes)
		node->AddChild(AddLimb(nif, b));
}

void FBXWrangler::AddNif(NifFile* nif, const std::string& shapeName) {
	AddSkeleton(nif, true);

	for (auto &s : nif->GetShapeNames()) {
		if (s == shapeName || shapeName.empty()) {
			std::vector<Triangle> tris;
			auto shape = nif->FindBlockByName<NiShape>(s);
			if (shape && shape->GetTriangles(tris)) {
				const std::vector<Vector3>* verts = nif->GetRawVertsForShape(s);
				const std::vector<Vector3>* norms = nif->GetNormalsForShape(s, false);
				const std::vector<Vector2>* uvs = nif->GetUvsForShape(s);
				AddGeometry(s, verts, norms, &tris, uvs);
			}
		}
	}
}

void FBXWrangler::AddSkinning(AnimInfo* anim, const std::string& shapeName) {
	FbxNode* rootNode = scene->GetRootNode();
	FbxNode* skelNode = rootNode->FindChild("NifSkeleton");
	if (!skelNode)
		return;

	for (auto &animSkin : anim->shapeSkinning) {
		if (animSkin.first != shapeName && !shapeName.empty())
			continue;

		std::string shape = animSkin.first;
		FbxNode* shapeNode = rootNode->FindChild(shape.c_str());
		if (!shapeNode)
			continue;

		std::string shapeSkin = shape + "_sk";
		FbxSkin* skin = FbxSkin::Create(scene, shapeSkin.c_str());

		for (auto &bone : anim->shapeBones[shape]) {
			FbxNode* jointNode = skelNode->FindChild(bone.c_str());
			if (jointNode) {
				std::string boneSkin = bone + "_sk";
				FbxCluster* aCluster = FbxCluster::Create(scene, boneSkin.c_str());
				aCluster->SetLink(jointNode);
				aCluster->SetLinkMode(FbxCluster::eTotalOne);

				auto weights = anim->GetWeightsPtr(shape, bone);
				if (weights) {
					for (auto &vw : *weights)
						aCluster->AddControlPointIndex(vw.first, vw.second);
				}

				skin->AddCluster(aCluster);
			}
		}

		FbxMesh* shapeMesh = (FbxMesh*)shapeNode->GetNodeAttribute();
		if (shapeMesh)
			shapeMesh->AddDeformer(skin);
	}
}

bool FBXWrangler::ExportScene(const std::string& fileName) {
	FbxExporter* iExporter = FbxExporter::Create(sdkManager, "");
	if (!iExporter->Initialize(fileName.c_str(), -1, sdkManager->GetIOSettings())) {
		iExporter->Destroy();
		return false;
	}

	// Export options determine what kind of data is to be imported.
	// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
	// is true, but here we set the options explicitly.
	FbxIOSettings* ios = sdkManager->GetIOSettings();
	ios->SetBoolProp(EXP_FBX_MATERIAL, true);
	ios->SetBoolProp(EXP_FBX_TEXTURE, true);
	ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
	ios->SetBoolProp(EXP_FBX_SHAPE, true);
	ios->SetBoolProp(EXP_FBX_GOBO, true);
	ios->SetBoolProp(EXP_FBX_ANIMATION, true);
	ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	iExporter->SetFileExportVersion(FBX_2014_00_COMPATIBLE);

	sdkManager->CreateMissingBindPoses(scene);

	FbxAxisSystem axis(FbxAxisSystem::eMax);
	axis.ConvertScene(scene);

	bool status = iExporter->Export(scene);
	iExporter->Destroy();

	return status;
}

bool FBXWrangler::ImportScene(const std::string& fileName, const FBXImportOptions& options) {
	FbxIOSettings* ios = sdkManager->GetIOSettings();
	ios->SetBoolProp(IMP_FBX_MATERIAL, true);
	ios->SetBoolProp(IMP_FBX_TEXTURE, true);
	ios->SetBoolProp(IMP_FBX_LINK, false);
	ios->SetBoolProp(IMP_FBX_SHAPE, true);
	ios->SetBoolProp(IMP_FBX_GOBO, true);
	ios->SetBoolProp(IMP_FBX_ANIMATION, true);
	ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	FbxImporter* iImporter = FbxImporter::Create(sdkManager, "");
	if (!iImporter->Initialize(fileName.c_str(), -1, ios)) {
		iImporter->Destroy();
		return false;
	}

	NewScene();

	bool status = iImporter->Import(scene);
	iImporter->Destroy();

	if (!status)
		return false;

	return LoadMeshes(options);
}

bool FBXWrangler::LoadMeshes(const FBXImportOptions& options) {
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
					shape.verts.emplace_back((float)vert.mData[0], (float)vert.mData[1], (float)vert.mData[2]);
					if (uv && uv->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
						int uIndex = v;
						if (uv->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
							uIndex = uv->GetIndexArray().GetAt(v);

						shape.uvs.emplace_back((float)uv->GetDirectArray().GetAt(uIndex).mData[0],
							(float)uv->GetDirectArray().GetAt(uIndex).mData[1]);
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
					if (m->GetPolygonSize(t) != 3)
						continue;

					int p1 = m->GetPolygonVertex(t, 0);
					int p2 = m->GetPolygonVertex(t, 1);
					int p3 = m->GetPolygonVertex(t, 2);
					shape.tris.emplace_back(p1, p2, p3);

					if (uv && uv->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
						FbxVector2 v_uv;
						bool isUnmapped;

						if (m->GetPolygonVertexUV(t, 0, uvName, v_uv, isUnmapped))
							shape.uvs[p1] = Vector2(v_uv.mData[0], v_uv.mData[1]);

						if (m->GetPolygonVertexUV(t, 1, uvName, v_uv, isUnmapped))
							shape.uvs[p2] = Vector2(v_uv.mData[0], v_uv.mData[1]);

						if (m->GetPolygonVertexUV(t, 2, uvName, v_uv, isUnmapped))
							shape.uvs[p3] = Vector2(v_uv.mData[0], v_uv.mData[1]);
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
					for (auto &u : shape.uvs)
						u.u = 1.0f - u.u;

				if (options.InvertV)
					for (auto &v : shape.uvs)
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
