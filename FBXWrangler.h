/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <fbxsdk.h>
#include <string>
#include "Mesh.h"	
#include "NifFile.h"
#include "Anim.h"


class FBXShape {
public:
	class FBXSkin {
	public:
		unordered_map<ushort, float> vertweights;
		void add(ushort vert, float wt) {
			vertweights[vert] = wt;
		}
	};
	string name;
	int numverts;
	int numtris;
	vector<Vector3> verts;
	vector<Triangle> tris;
	vector<Vector2> uvs;
	vector<Vector3> normals;
	
	unordered_map<string, FBXSkin> boneSkin;
	set<string> boneNames;
	
};

class FBXWrangler
{
	FbxManager* pSdkManager;
	FbxScene* pCurrentScene;

	map<string, FBXShape> inShapes;
public:
	FBXWrangler();
	~FBXWrangler();
	void GetShapeNames(vector<string>& outNames) {
		for (auto is : inShapes) {
			outNames.push_back(is.first);
		}
	}

	FBXShape* InShape(const string& shapeName) {
		return &(inShapes[shapeName]);
	}

	void NewScene();

	void CloseScene();

	void AddMesh(mesh * m);
	void AddSkeleton(NifFile* skeletonNif);
	// Recursively add bones to the skeleton in a depth-first manner
	FbxNode* AddLimb(NifFile* skeletonNif, NiNode* nifBone);

	void AddNif(NifFile* meshNif, const string& shapeName = "", bool addSkeleton=true);
	void AddSkinning(AnimInfo* anim, const string& shapeName = "");
	void AddGeometry(const string& name, const vector<Vector3>* verts, const vector<Vector3>* norms, vector<Triangle>* tris, const vector<Vector2>* uvs);

	bool ExportScene(const std::string& fileName);

	bool ImportScene(const std::string& filenName);

	bool LoadMeshes();

};

