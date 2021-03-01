/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <map>
#include <fstream>
#include <memory>
#include <algorithm>

enum MorphType : nifly::byte {
	MORPHTYPE_POSITION,
	MORPHTYPE_UV
};

struct MorphData {
	std::string name;
	MorphType type = MORPHTYPE_POSITION;
	std::map<int, nifly::Vector3> offsets;
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

	nifly::ushort GetShapeCount(MorphType morphType);
	nifly::ushort GetMorphCount(const std::string& shapeName, MorphType morphType);
};

struct TriHeadMorph {
	std::string morphName;
	float multiplier = 1.0f;
	std::vector<nifly::Vector3> vertices;
};

class TriHeadFile {
	std::string identifier = "FR";
	std::string fileType = "TRI";
	std::string version = "003";
	nifly::uint numVertices = 0;
	nifly::uint numTriangles = 0;
	nifly::uint numQuads = 0;
	nifly::uint unknown2 = 0;
	nifly::uint unknown3 = 0;
	nifly::uint numUV = 0;
	nifly::uint flags = 1;
	nifly::uint numMorphs = 0;
	nifly::uint numModifiers = 0;
	nifly::uint numModVertices = 0;
	nifly::uint unknown7 = 0;
	nifly::uint unknown8 = 0;
	nifly::uint unknown9 = 0;
	nifly::uint unknown10 = 0;
	std::vector<nifly::Vector3> vertices;
	std::vector<nifly::Vector3> modVertices;
	std::vector<nifly::Triangle> triangles;
	std::vector<nifly::Vector2> uv;
	std::vector<nifly::Triangle> tex;
	std::vector<TriHeadMorph> morphs;

public:
	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);

	std::vector<nifly::Vector3> GetVertices();
	std::vector<nifly::Triangle> GetTriangles();
	std::vector<nifly::Vector2> GetUV();

	void SetVertices(const std::vector<nifly::Vector3> verts);
	void SetTriangles(const std::vector<nifly::Triangle> tris);
	void SetUV(const std::vector<nifly::Vector2> uvs);

	void AddMorph(const TriHeadMorph& morph);
	void DeleteMorph(const std::string& morphName);

	TriHeadMorph* GetMorph(const std::string& morphName);
	std::vector<TriHeadMorph> GetMorphs();
};
