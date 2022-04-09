/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Anim.h"
#include "../components/Mesh.h"
#include "../program/FBXImportOptions.h"
#include "NifFile.hpp"

#include <memory>
#include <set>

struct FBXImportOptions {
	bool InvertU = false;
	bool InvertV = false;
	float Scale = 1.0f;
	float RotateX = 0.0f;
	float RotateY = 0.0f;
	float RotateZ = 0.0f;

	bool ImportAll = true;
	std::set<std::string> ImportShapes;
};

class FBXShape {
public:
	class FBXSkin {
	private:
		std::unordered_map<uint16_t, float> vertWeights;

	public:
		void SetWeight(uint16_t vert, float wt) { vertWeights[vert] = wt; }

		float GetWeight(uint16_t vert) {
			auto it = vertWeights.find(vert);
			if (it == vertWeights.end())
				return 0.0f;

			return vertWeights[vert];
		}

		std::unordered_map<uint16_t, float>& GetWeights() { return vertWeights; }
	};

	std::string name;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<nifly::Vector2> uvs;
	std::vector<nifly::Vector3> normals;

	std::unordered_map<std::string, FBXSkin> boneSkin;
	std::set<std::string> boneNames;
};

class FBXWrangler {
private:
	struct Priv;
	std::unique_ptr<Priv> priv;

	std::string comName;

public:
	FBXWrangler();
	~FBXWrangler();

	void NewScene();
	void CloseScene();

	void GetShapeNames(std::vector<std::string>& outNames);
	FBXShape* GetShape(const std::string& shapeName);

	void AddSkeleton(nifly::NifFile* nif, bool onlyNonSkeleton = false);
	void AddNif(nifly::NifFile* meshNif, AnimInfo* anim, bool transToGlobal, nifly::NiShape* shape = nullptr);
	void AddSkinning(AnimInfo* anim, nifly::NiShape* shape = nullptr);

	bool ExportScene(const std::string& fileName);
	bool ImportScene(const std::string& fileName, const FBXImportOptions& options = FBXImportOptions());
};
