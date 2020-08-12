/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Mesh.h"	
#include "../components/Anim.h"
#include "../program/FBXImportOptions.h"
#include "../NIF/NifFile.h"
#include <memory>


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

	void AddSkeleton(NifFile* nif, bool onlyNonSkeleton = false);
	void AddNif(NifFile* meshNif, AnimInfo* anim, bool transToGlobal, NiShape* shape = nullptr);
	void AddSkinning(AnimInfo* anim, NiShape* shape = nullptr);

	bool ExportScene(const std::string& fileName);
	bool ImportScene(const std::string& fileName, const FBXImportOptions& options = FBXImportOptions());
};
