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

struct ObjPoint {
	int v = 0;
	int vt = 0;
	int vn = 0;

	ObjPoint(const int inV, const int inVT, const int inVN) {
		v = inV;
		vt = inVT;
		vn = inVN;
	}

	bool operator==(const ObjPoint& other) const { return v == other.v && vt == other.vt && vn == other.vn; }
};

struct ObjData {
	std::string name;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<nifly::Vector2> uvs;
	std::vector<nifly::Vector3> norms;
};

class ObjFile {
	std::map<std::string, ObjData*> data;

public:
	nifly::Vector3 scale = nifly::Vector3(1.0f, 1.0f, 1.0f);
	nifly::Vector3 offset;

	ObjFile();
	~ObjFile();

	int AddGroup(const std::string& name,
				 const std::vector<nifly::Vector3>& verts,
				 const std::vector<nifly::Triangle>& tris,
				 const std::vector<nifly::Vector2>& uvs,
				 const std::vector<nifly::Vector3>& norms);

	void SetScale(const nifly::Vector3& inScale) { scale = inScale; }
	void SetOffset(const nifly::Vector3& inOffset) { offset = inOffset; }

	int LoadForNif(const std::string& fileName, const ObjOptionsImport& options = ObjOptionsImport());
	int LoadForNif(std::fstream& base, const ObjOptionsImport& options = ObjOptionsImport());

	int Save(const std::string& fileName);

	bool CopyDataForGroup(
		const std::string& name, std::vector<nifly::Vector3>* v, std::vector<nifly::Triangle>* t, std::vector<nifly::Vector2>* uv, std::vector<nifly::Vector3>* norms);
	std::vector<std::string> GetGroupList();
};
