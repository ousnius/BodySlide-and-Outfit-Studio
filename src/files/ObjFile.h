/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../NIF/utils/Object3d.h"

#include <map>
#include <fstream>
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

	bool operator==(const ObjPoint& other) const {
		return v == other.v && vt == other.vt && vn == other.vn;
	}
};

struct ObjData {
	std::string name;
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	std::vector<Vector3> norms;
};

class ObjFile {
	std::map<std::string, ObjData*> data;

public:
	Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 offset;

	ObjFile();
	~ObjFile();

	int AddGroup(const std::string& name, const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const std::vector<Vector2>& uvs, const std::vector<Vector3>& norms);

	void SetScale(const Vector3& inScale) { scale = inScale; }
	void SetOffset(const Vector3& inOffset) { offset = inOffset; }

	int LoadForNif(const std::string& fileName, const ObjOptionsImport& options = ObjOptionsImport());
	int LoadForNif(std::fstream& base, const ObjOptionsImport& options = ObjOptionsImport());

	int Save(const std::string& fileName);

	bool CopyDataForGroup(const std::string& name, std::vector<Vector3>* v, std::vector<Triangle>* t, std::vector<Vector2>* uv, std::vector<Vector3>* norms);
	std::vector<std::string> GetGroupList();
};
