/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../components/Mesh.h"	
#include "../components/Anim.h"
#include "../program/FBXImportOptions.h"
#include "../NIF/NifFile.h"

#include <fbxsdk.h>


class FBXShape {
public:
	class FBXSkin {
	private:
		std::unordered_map<ushort, float> vertWeights;

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

		std::unordered_map<ushort, float>& GetWeights() {
			return vertWeights;
		}
	};

	std::string name;
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	std::vector<Vector3> normals;

	std::unordered_map<std::string, FBXSkin> boneSkin;
	std::set<std::string> boneNames;
};

class FBXWrangler {
private:
	FbxManager* sdkManager = nullptr;
	FbxScene* scene = nullptr;

	std::string comName;
	std::map<std::string, FBXShape> shapes;

public:
	FBXWrangler();
	~FBXWrangler();

	void NewScene();
	void CloseScene();

	void GetShapeNames(std::vector<std::string>& outNames) {
		for (auto &s : shapes)
			outNames.push_back(s.first);
	}

	FBXShape* GetShape(const std::string& shapeName) {
		return &(shapes[shapeName]);
	}

	void AddSkeleton(NifFile* nif, bool onlyNonSkeleton = false);

	// Recursively add bones to the skeleton in a depth-first manner
	FbxNode* AddLimb(NifFile* nif, NiNode* nifBone);
	void AddLimbChildren(FbxNode* node, NifFile* nif, NiNode* nifBone);

	void AddNif(NifFile* meshNif, const std::string& shapeName = "");
	void AddSkinning(AnimInfo* anim, const std::string& shapeName = "");
	void AddGeometry(const std::string& shapeName, const std::vector<Vector3>* verts, const std::vector<Vector3>* norms, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs);

	bool ExportScene(const std::string& fileName);
	bool ImportScene(const std::string& fileName, const FBXImportOptions& options = FBXImportOptions());

	bool LoadMeshes(const FBXImportOptions& options);
};
