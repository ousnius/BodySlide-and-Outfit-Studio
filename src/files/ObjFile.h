/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <fstream>
#include <map>
#include <string>

struct ObjOptionsImport {
	bool noFaces = false;
};

struct ObjData {
	std::string name;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<nifly::Vector2> uvs;
	std::vector<nifly::Vector3> norms;
};

class ObjFile {
	std::map<std::string, ObjData> data;

public:
	nifly::Vector3 scale = nifly::Vector3(1.0f, 1.0f, 1.0f);
	nifly::Vector3 offset;

	int AddGroup(const std::string& name,
				 const std::vector<nifly::Vector3>& verts,
				 const std::vector<nifly::Triangle>& tris,
				 const std::vector<nifly::Vector2>& uvs,
				 const std::vector<nifly::Vector3>& norms);

	void SetScale(const nifly::Vector3& inScale) { scale = inScale; }
	void SetOffset(const nifly::Vector3& inOffset) { offset = inOffset; }

	int LoadForNif(const std::string& fileName, const ObjOptionsImport& options = ObjOptionsImport());
	void LoadNoFaces(std::istream& base);
	void LoadForNif(std::istream& base);

	int Save(const std::string& fileName);

	bool CopyDataForGroup(
		const std::string& name, std::vector<nifly::Vector3>* v, std::vector<nifly::Triangle>* t, std::vector<nifly::Vector2>* uv, std::vector<nifly::Vector3>* norms);
	std::vector<std::string> GetGroupList();
};
