/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "FBXWrangler.h"

FBXWrangler::FBXWrangler(): pSdkManager(nullptr), pCurrentScene(nullptr) {
	
	pSdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(pSdkManager, IOSROOT);
	pSdkManager->SetIOSettings(ios);

}


FBXWrangler::~FBXWrangler()
{
	if (pCurrentScene)
		CloseScene();

	if (pSdkManager)
		pSdkManager->Destroy();
}

void FBXWrangler::NewScene() {
	if (pCurrentScene) 
		CloseScene();
	pCurrentScene = FbxScene::Create(pSdkManager, "OutfitStudioScene");	
}

void FBXWrangler::CloseScene() {
	if (pCurrentScene)
		pCurrentScene->Destroy();

	pCurrentScene = nullptr;
}

void FBXWrangler::AddMesh(mesh* m) {
	FbxMesh* lm = FbxMesh::Create(pSdkManager, m->shapeName.c_str());	

	FbxGeometryElementNormal* normElem = lm->CreateElementNormal();
	normElem->SetMappingMode(FbxLayerElement::eByControlPoint);
	normElem->SetReferenceMode(FbxLayerElement::eDirect);

	FbxGeometryElementUV* lUVDiffuseElement = lm->CreateElementUV(string(m->shapeName+"UV").c_str());
	lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);

	lm->InitControlPoints(m->nVerts);
	FbxVector4* cp = lm->GetControlPoints();

	for (int i = 0; i < m->nVerts; i++) {
		cp[i] = FbxVector4(m->verts[i].x, m->verts[i].y, m->verts[i].z);
		normElem->GetDirectArray().Add(FbxVector4(m->verts[i].nx, m->verts[i].ny, m->verts[i].nz));
		lUVDiffuseElement->GetDirectArray().Add(FbxVector2(m->texcoord[i].u, m->texcoord[i].v));
	}


	for (int i = 0; i < m->nTris; i++) {
		lm->BeginPolygon();
		lm->AddPolygon(m->tris[i].p1);
		lm->AddPolygon(m->tris[i].p2);
		lm->AddPolygon(m->tris[i].p3);
		lm->EndPolygon();
	}	

	FbxNode* rootNode = pCurrentScene->GetRootNode();

	FbxNode* mNode = FbxNode::Create(pSdkManager,m->shapeName.c_str() );

	rootNode->AddChild(mNode);
	
	mNode->SetNodeAttribute(lm);
	
	mNode->LclScaling.Set(FbxVector4(1.0, 1.0, 1.0));
	mNode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));

}

void FBXWrangler::AddGeometry(const string& shapeName, const vector<Vector3>* verts, const vector<Vector3>* norms, vector<Triangle>* tris, const vector<Vector2>* uvs) {

	FbxMesh* lm = FbxMesh::Create(pSdkManager, shapeName.c_str());	

	FbxGeometryElementNormal* normElem = lm->CreateElementNormal();
	normElem->SetMappingMode(FbxLayerElement::eByControlPoint);
	normElem->SetReferenceMode(FbxLayerElement::eDirect);

	FbxGeometryElementUV* lUVDiffuseElement = lm->CreateElementUV(string(shapeName+"UV").c_str());
	lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);

	lm->InitControlPoints(verts->size());
	FbxVector4* cp = lm->GetControlPoints();

	for (int i = 0; i < verts->size(); i++) {
		cp[i] = FbxVector4((*verts)[i].x, (*verts)[i].y, (*verts)[i].z);
		normElem->GetDirectArray().Add(FbxVector4((*norms)[i].x, (*norms)[i].y, (*norms)[i].z));
		lUVDiffuseElement->GetDirectArray().Add(FbxVector2((*uvs)[i].u, (*uvs)[i].v));
	}


	for (auto t:(*tris)) {
		lm->BeginPolygon();
		lm->AddPolygon(t.p1);
		lm->AddPolygon(t.p2);
		lm->AddPolygon(t.p3);
		lm->EndPolygon();
	}	

	FbxNode* rootNode = pCurrentScene->GetRootNode();

	FbxNode* mNode = FbxNode::Create(pSdkManager,shapeName.c_str() );

	rootNode->AddChild(mNode);
	
	mNode->SetNodeAttribute(lm);
	
	// For Blender import
	//mNode->LclScaling.Set(FbxVector4(0.1, 0.1, 0.1));
	//mNode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));

	// For Maya Import
	mNode->LclScaling.Set(FbxVector4(1,1,1));
	mNode->LclRotation.Set(FbxVector4(-90, 0, 0));
	mNode->LclTranslation.Set(FbxVector4(0, 120, 0));

}

void FBXWrangler::AddSkeleton(NifFile* skeletonNif) {

	string skelName = "NifSkeleton";

	FbxSkeleton* skelly = FbxSkeleton::Create(pCurrentScene, skelName.c_str());
	skelly->SetSkeletonType(FbxSkeleton::eRoot);
	FbxNode* skellynode = FbxNode::Create(pCurrentScene, skelName.c_str());
	skellynode->SetNodeAttribute(skelly);
	//skellynode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);
	skellynode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));
	//skellynode->LclRotation.Set(FbxVector4(0.0, 0.0, 0.0));

	float rx, ry, rz;

	NiNode* root = (NiNode*)skeletonNif->GetBlock(skeletonNif->GetNodeID(Config["SkeletonRootName"]));
	NiNode* COM = (NiNode*)skeletonNif->GetBlock(skeletonNif->GetNodeID("COM"));
	if (!COM)
		COM = (NiNode*)skeletonNif->GetBlock(skeletonNif->GetNodeID("NPC COM [COM ]"));

	FbxNode* parentNode = skellynode;
	if (root) {
		FbxSkeleton* rootbone = FbxSkeleton::Create(pCurrentScene, root->name.c_str());
		rootbone->SetSkeletonType(FbxSkeleton::eLimbNode);
		rootbone->Size.Set(1.0f);
		FbxNode* rootboneNode = FbxNode::Create(pCurrentScene, root->name.c_str());
		rootboneNode->SetNodeAttribute(rootbone);
		root->rotToEulerDegrees(rx, ry, rz);
		rootboneNode->LclRotation.Set(FbxVector4(rx, ry, rz));
		rootboneNode->LclTranslation.Set(FbxVector4(root->translation.x, root->translation.y, root->translation.z));
		//rootboneNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);
		parentNode->AddChild(rootboneNode);
		parentNode = rootboneNode;
	}

	if (COM) {
		FbxSkeleton* COMBone = FbxSkeleton::Create(pCurrentScene, COM->name.c_str());
		COMBone->SetSkeletonType(FbxSkeleton::eLimbNode);
		COMBone->Size.Set(1.0f);
		FbxNode* COMBoneNode = FbxNode::Create(pCurrentScene, COM->name.c_str());
		COMBoneNode->SetNodeAttribute(COMBone);
		COM->rotToEulerDegrees(rx, ry, rz);
		COMBoneNode->LclRotation.Set(FbxVector4(rx, ry, rz));
		COMBoneNode->LclTranslation.Set(FbxVector4(COM->translation.y, COM->translation.z, COM->translation.x));
		//COMBoneNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);
		parentNode->AddChild(COMBoneNode);
		parentNode = COMBoneNode;		

	}

	NiNode* top = COM;
	// likely a mesh nif with non hierarchical bone nodes
	if (!COM) {
		top = (NiNode*)skeletonNif->GetBlock(0);
	}

	vector<NiNode*> boneNodes = skeletonNif->GetChildren<NiNode>(top);
		for (auto bn:boneNodes) {
			parentNode->AddChild(AddLimb(skeletonNif, bn));
		}


	pCurrentScene->GetRootNode()->AddChild(skellynode);

}

FbxNode* FBXWrangler::AddLimb(NifFile* skeletonNif, NiNode* nifBone) {

	float rx, ry, rz;
	static map<string,int> boneNames;
	if (boneNames.find(nifBone->name) == boneNames.end()) {		
		boneNames[nifBone->name] = 1;
	}
	else {
		boneNames[nifBone->name]++;
	}

	Vector3 myTranslation = nifBone->translation;
	FbxSkeleton* myBone = FbxSkeleton::Create(pCurrentScene, nifBone->name.c_str());
	myBone->SetSkeletonType(FbxSkeleton::eLimbNode);
	myBone->Size.Set(1.0f);
	FbxNode* myNode = FbxNode::Create(pCurrentScene, nifBone->name.c_str());
	myNode->SetNodeAttribute(myBone);
	nifBone->rotToEulerDegrees(rx, ry, rz);
	myNode->LclRotation.Set(FbxVector4(rx, ry, rz));
	myNode->LclTranslation.Set(FbxVector4(myTranslation.x, myTranslation.y, myTranslation.z));
	//myNode->SetRotationOrder(FbxNode::eSourcePivot, eEulerZYX);

	vector<NiNode*> boneNodes = skeletonNif->GetChildren<NiNode>(nifBone);
	for (auto bn:boneNodes) {
		myNode->AddChild(AddLimb(skeletonNif, bn));
	}


	return myNode;
}

void FBXWrangler::AddNif(NifFile* meshNif, const string& shapeName, bool addSkeleton) {
	
	if (addSkeleton) {		
		AddSkeleton(meshNif);
	}

	vector<string> shapeList;

	meshNif->GetShapeList(shapeList);
	for (auto s : shapeList) {
		if (shapeName == "" || s == shapeName) {
			vector<Triangle> tris;
			if (meshNif->GetTrisForShape(s, &tris)) {
				const vector<Vector3>* verts = meshNif->GetRawVertsForShape(s);
				const vector<Vector3>* norms = meshNif->GetNormalsForShape(s,false);
				const vector<Vector2>* uvs = meshNif->GetUvsForShape(s);
				AddGeometry(s, verts, norms, &tris, uvs);
			}
		}		
	}

}

void FBXWrangler::AddSkinning(AnimInfo* anim, const string& shapeName) {
	FbxNode* rootNode = pCurrentScene->GetRootNode();
	FbxNode* skelNode = rootNode->FindChild("NifSkeleton");
	if (!skelNode)
		return;

	for (auto shapeskin : anim->shapeSkinning) {
		if (shapeName != "" && shapeskin.first != shapeName)
			continue;

		string curShape = shapeskin.first;
		FbxNode* shapeNode = rootNode->FindChild(curShape.c_str());

		FbxSkin* skin = FbxSkin::Create(pCurrentScene, string(curShape + "_sk").c_str());
		unordered_map<ushort, float> outWeights;

		for (auto bn : anim->shapeBones[curShape]) {
			FbxNode* jointNode = skelNode->FindChild(bn.c_str());
			if (jointNode) {
				FbxCluster* aCluster = FbxCluster::Create(pCurrentScene, string(bn + "_sk").c_str());
				aCluster->SetLink(jointNode);
				aCluster->SetLinkMode(FbxCluster::eTotalOne);
				anim->GetWeights(curShape, bn, outWeights);
				for (auto vw : outWeights)
					aCluster->AddControlPointIndex(vw.first, vw.second);

				FbxMatrix xforMat = jointNode->EvaluateGlobalTransform();
				skin->AddCluster(aCluster);
			}
		}

		((FbxMesh*)shapeNode->GetNodeAttribute())->AddDeformer(skin);
	}
}

bool FBXWrangler::ExportScene(const std::string& fileName) {

	int lMajor, lMinor, lRevision;

	FbxExporter* iExporter = FbxExporter::Create(pSdkManager, "");
	
	if (iExporter->Initialize(fileName.c_str(), -1, pSdkManager->GetIOSettings()) == false) {
		iExporter->Destroy();
		return false;
	}

	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);

	auto ios = (pSdkManager->GetIOSettings());

		// Export options determine what kind of data is to be imported.
		// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
		// is true, but here we set the options explicitly.
		ios->SetBoolProp(EXP_FBX_MATERIAL, true);
		ios->SetBoolProp(EXP_FBX_TEXTURE, true);
		ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
		ios->SetBoolProp(EXP_FBX_SHAPE, true);
		ios->SetBoolProp(EXP_FBX_GOBO, true);
		ios->SetBoolProp(EXP_FBX_ANIMATION, true);
		ios->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
	
	iExporter->SetFileExportVersion(FBX_2014_00_COMPATIBLE);

	pSdkManager->CreateMissingBindPoses(pCurrentScene);

	FbxAxisSystem axis	(FbxAxisSystem::eMax);
	axis.ConvertScene(pCurrentScene); 

	// Export the scene.
	bool status = iExporter->Export(pCurrentScene);

	// Destroy the exporter.
	iExporter->Destroy();

	return status;

}

bool FBXWrangler::ImportScene(const std::string& filenName) {

	FbxIOSettings* ios = pSdkManager->GetIOSettings();
	ios->SetBoolProp(IMP_FBX_MATERIAL, true);
	ios->SetBoolProp(IMP_FBX_TEXTURE, true);
	ios->SetBoolProp(IMP_FBX_LINK, false);
	ios->SetBoolProp(IMP_FBX_SHAPE, true);
	ios->SetBoolProp(IMP_FBX_GOBO, true);
	ios->SetBoolProp(IMP_FBX_ANIMATION, true);
	ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);


	FbxImporter* iImporter = FbxImporter::Create(pSdkManager, "");

	if (iImporter->Initialize(filenName.c_str(), -1, ios) == false) {
		iImporter->Destroy();
		return false;
	}

	NewScene();

	bool status = iImporter->Import(pCurrentScene);

	iImporter->Destroy();

	if (!status)
		return false;

	return LoadMeshes();

}

bool FBXWrangler::LoadMeshes() {
	if (!pCurrentScene)
		return false;

	FbxNode* root = pCurrentScene->GetRootNode();

	for (int i = 0; i < root->GetChildCount(); i++) {
		FbxNode* c = root->GetChild(i);

		if (c->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
			FBXShape meshData;
			FbxMesh* m = (FbxMesh*)c->GetNodeAttribute();

			if (!m->IsTriangleMesh()) {
				FbxGeometryConverter converter(pSdkManager);
				m = (FbxMesh*)converter.Triangulate((FbxNodeAttribute*)m, true);
			}

			meshData.name = c->GetName();			
			meshData.numverts = m->GetControlPointsCount();
			meshData.numtris = m->GetPolygonCount();

			for (int v = 0; v < meshData.numverts; v++) {
				FbxVector4 vert = m->GetControlPointAt(v);
				meshData.verts.emplace_back((float)vert.mData[0], (float)vert.mData[1], (float)vert.mData[2]);
				if (m->GetElementUVCount() && m->GetElementUV(0)->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
					int uindex = v;
					if (m->GetElementUV(0)->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
						uindex = m->GetElementUV(0)->GetIndexArray().GetAt(v);
					}
					meshData.uvs.emplace_back((float)m->GetElementUV(0)->GetDirectArray().GetAt(uindex).mData[0],
											  (float)m->GetElementUV(0)->GetDirectArray().GetAt(uindex).mData[1]);
				}
				
				if (m->GetElementNormalCount() && m->GetElementNormal(0)->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
					meshData.normals.emplace_back((float)m->GetElementNormal(0)->GetDirectArray().GetAt(v).mData[0],
												  (float)m->GetElementNormal(0)->GetDirectArray().GetAt(v).mData[1],
												  (float)m->GetElementNormal(0)->GetDirectArray().GetAt(v).mData[2]);
				}
			}

			int p1, p2, p3;const char* uvn = m->GetElementUV(0)->GetName();					
			meshData.uvs.resize(meshData.numverts);	
			FbxVector2 uv;
			bool hasuv;

			for (int t = 0; t < meshData.numtris; t++) {
				if (m->GetPolygonSize(t) != 3)
					continue;

				p1 = m->GetPolygonVertex(t, 0);
				p2 = m->GetPolygonVertex(t, 1);
				p3 = m->GetPolygonVertex(t, 2);
				meshData.tris.emplace_back(p1,p2,p3);

				if (m->GetElementUVCount() && m->GetElementUV(0)->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
				
						m->GetPolygonVertexUV(t, 0, uvn, uv, hasuv);
						meshData.uvs[p1] = Vector2(uv.mData[0], uv.mData[1]);

						m->GetPolygonVertexUV(t, 1, uvn, uv, hasuv);
						meshData.uvs[p2] = Vector2(uv.mData[0], uv.mData[1]);

						m->GetPolygonVertexUV(t, 2, uvn, uv, hasuv);
						meshData.uvs[p3] = Vector2(uv.mData[0], uv.mData[1]);

				}

			}

			for (int iskin = 0; iskin < m->GetDeformerCount(FbxDeformer::eSkin); iskin++) {
				FbxSkin* skin = (FbxSkin*)m->GetDeformer(iskin,FbxDeformer::eSkin);

				for (int icluster = 0; icluster < skin->GetClusterCount(); icluster++) {
					FbxCluster* cluster = skin->GetCluster(icluster);
					if (!cluster->GetLink())
						continue;
					string bn = cluster->GetLink()->GetName();
					meshData.boneNames.insert(bn);
					for (int iv = 0; iv < cluster->GetControlPointIndicesCount(); iv++) {
						int v = cluster->GetControlPointIndices()[iv];
						float w = cluster->GetControlPointWeights()[iv];
						meshData.boneSkin[bn].add(v, w);
					}
				}

			}



			inShapes[meshData.name] = meshData;

		}

	}

	return true;

}
