/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "Mesh.h"	
#include "NifFile.h"
#include "Anim.h"

#include <fbxsdk.h>


class FBXShape {
public:
	class FBXSkin {
	private:
		unordered_map<ushort, float> vertWeights;

	public:
		void SetWeight(ushort vert, float wt) {
			vertWeights[vert] = wt;
		}

		float GetWeight(ushort vert) {
			auto it = vertWeights.find(vert);
			if (it == vertWeights.end())
				return 0.0f;

			return vertWeights[vert];
		}

		unordered_map<ushort, float>& GetWeights() {
			return vertWeights;
		}
	};

	string name;
	vector<Vector3> verts;
	vector<Triangle> tris;
	vector<Vector2> uvs;
	vector<Vector3> normals;

	unordered_map<string, FBXSkin> boneSkin;
	set<string> boneNames;
};

class FBXWrangler {
private:
	FbxManager* sdkManager = nullptr;
	FbxScene* scene = nullptr;

	NiNode* com = nullptr;
	map<string, FBXShape> shapes;

public:
	FBXWrangler();
	~FBXWrangler();

	void NewScene();
	void CloseScene();

	void GetShapeNames(vector<string>& outNames) {
		for (auto &s : shapes)
			outNames.push_back(s.first);
	}

	FBXShape* GetShape(const string& shapeName) {
		return &(shapes[shapeName]);
	}

	void AddSkeleton(NifFile* nif, bool onlyNonSkeleton = false);

	// Recursively add bones to the skeleton in a depth-first manner
	FbxNode* AddLimb(NifFile* nif, NiNode* nifBone);

	void AddNif(NifFile* meshNif, const string& shapeName = "");
	void AddSkinning(AnimInfo* anim, const string& shapeName = "");
	void AddGeometry(const string& shapeName, const vector<Vector3>& verts, const vector<Vector3>& norms, const vector<Triangle>& tris, const vector<Vector2>& uvs);

	bool ExportScene(const string& fileName);
	bool ImportScene(const string& fileName);

	bool LoadMeshes();
};
