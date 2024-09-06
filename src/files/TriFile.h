/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>

enum MorphType : uint8_t { MORPHTYPE_POSITION, MORPHTYPE_UV };

struct MorphData {
	std::string name;
	MorphType type = MORPHTYPE_POSITION;
	std::map<uint16_t, nifly::Vector3> offsets;
};

typedef std::shared_ptr<MorphData> MorphDataPtr;

bool IsBodyTriFile(const std::string& fileName);

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

	uint16_t GetShapeCount(MorphType morphType);
	uint16_t GetMorphCount(const std::string& shapeName, MorphType morphType);
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
	uint32_t numVertices = 0;
	uint32_t numTriangles = 0;
	uint32_t numQuads = 0;
	uint32_t unknown2 = 0;
	uint32_t unknown3 = 0;
	uint32_t numUV = 0;
	uint32_t flags = 1;
	uint32_t numMorphs = 0;
	uint32_t numModifiers = 0;
	uint32_t numModVertices = 0;
	uint32_t unknown7 = 0;
	uint32_t unknown8 = 0;
	uint32_t unknown9 = 0;
	uint32_t unknown10 = 0;
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
