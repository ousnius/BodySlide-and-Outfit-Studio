/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../NIF/utils/Object3d.h"

#include <map>
#include <fstream>
#include <memory>
#include <algorithm>

enum MorphType : byte {
	MORPHTYPE_POSITION,
	MORPHTYPE_UV
};

struct MorphData {
	std::string name;
	MorphType type = MORPHTYPE_POSITION;
	std::map<int, Vector3> offsets;
};

typedef std::shared_ptr<MorphData> MorphDataPtr;

class TriFile {
	std::map<std::string, std::vector<MorphDataPtr>> shapeMorphs;

public:
	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);
	
	void AddMorph(const std::string& shapeName, MorphDataPtr data);
	void DeleteMorph(const std::string& shapeName, const std::string& morphName);
	void DeleteMorphs(const std::string& shapeName);
	void DeleteMorphFromAll(const std::string& morphName);

	MorphDataPtr GetMorph(const std::string& shapeName, const std::string& morphName);
	std::map<std::string, std::vector<MorphDataPtr>> GetMorphs();

	ushort GetShapeCount(MorphType morphType);
	ushort GetMorphCount(const std::string& shapeName, MorphType morphType);
};

struct TriHeadMorph {
	std::string morphName;
	float multiplier = 1.0f;
	std::vector<Vector3> vertices;
};

class TriHeadFile {
	std::string identifier = "FR";
	std::string fileType = "TRI";
	std::string version = "003";
	uint numVertices = 0;
	uint numTriangles = 0;
	uint numQuads = 0;
	uint unknown2 = 0;
	uint unknown3 = 0;
	uint numUV = 0;
	uint flags = 1;
	uint numMorphs = 0;
	uint numModifiers = 0;
	uint numModVertices = 0;
	uint unknown7 = 0;
	uint unknown8 = 0;
	uint unknown9 = 0;
	uint unknown10 = 0;
	std::vector<Vector3> vertices;
	std::vector<Vector3> modVertices;
	std::vector<Triangle> triangles;
	std::vector<Vector2> uv;
	std::vector<Triangle> tex;
	std::vector<TriHeadMorph> morphs;

public:
	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);

	std::vector<Vector3> GetVertices();
	std::vector<Triangle> GetTriangles();
	std::vector<Vector2> GetUV();

	void SetVertices(const std::vector<Vector3> verts);
	void SetTriangles(const std::vector<Triangle> tris);
	void SetUV(const std::vector<Vector2> uvs);

	void AddMorph(const TriHeadMorph& morph);
	void DeleteMorph(const std::string& morphName);

	TriHeadMorph* GetMorph(const std::string& morphName);
	std::vector<TriHeadMorph> GetMorphs();
};
