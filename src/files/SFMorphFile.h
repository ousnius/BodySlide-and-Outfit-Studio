/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nifly {
class NifFile;
class NiShape;
}

enum SFMorphKey : uint32_t {
	None = 0,
	Morph0 = 1 << 0,
	Morph1 = 1 << 1,
	Morph2 = 1 << 2,
	Morph3 = 1 << 3,
	Morph4 = 1 << 4,
	Morph5 = 1 << 5,
	Morph6 = 1 << 6,
	Morph7 = 1 << 7,
	Morph8 = 1 << 8,
	Morph9 = 1 << 9,
	Morph10 = 1 << 10,
	Morph11 = 1 << 11,
	Morph12 = 1 << 12,
	Morph13 = 1 << 13,
	Morph14 = 1 << 14,
	Morph15 = 1 << 15,
	Morph16 = 1 << 16,
	Morph17 = 1 << 17,
	Morph18 = 1 << 18,
	Morph19 = 1 << 19,
	Morph20 = 1 << 20,
	Morph21 = 1 << 21,
	Morph22 = 1 << 22,
	Morph23 = 1 << 23,
	Morph24 = 1 << 24,
	Morph25 = 1 << 25,
	Morph26 = 1 << 26,
	Morph27 = 1 << 27,
	Morph28 = 1 << 28,
	Morph29 = 1 << 29,
	Morph30 = 1 << 30,
	Morph31 = static_cast<uint32_t>(1) << 31
};

struct SFMorphData {
	uint16_t offset[3]{0, 0, 0};
	uint16_t targetVertColor = 0;
	uint32_t x = 0, y = 0;
};

struct SFMorphDataUnpacked {
	nifly::Vector3 offset;
	nifly::ByteColor3 targetVertColor;
	nifly::Vector3 normal;
	nifly::Vector3 tangent;
};

struct SFMorphOffset {
	uint32_t offset = 0;
	SFMorphKey keyMarker[4];
};

// The per-vertex keyMarker is a fixed 128-bit field (4 x 32-bit words), so the
// morph.dat format can only reference shape keys with indices 0-127.
constexpr uint32_t SFMaxShapeKeys = 128;

class SFMorphFile {
	uint32_t numAxis = 3;
	uint32_t numShapeKeys = 0;
	uint32_t numVertices = 0;

	std::vector<std::string> morphNames;
	uint32_t numMorphData = 0;
	uint32_t numOffsets = 0;

	std::vector<SFMorphData> morphDataRaw;
	std::vector<SFMorphDataUnpacked> morphDataRawUnpacked;
	std::vector<SFMorphOffset> offsets;

	std::vector<std::vector<SFMorphData>> vertexMorphData;
	std::vector<std::vector<SFMorphDataUnpacked>> vertexMorphDataUnpacked;
	std::vector<std::vector<uint32_t>> vertexMorphKeyIndices;

public:
	std::map<std::string, uint32_t> morphNamesCacheMap;
	std::vector<std::unordered_map<uint16_t, nifly::Vector3>> morphOffsetsCache;
	std::vector<std::unordered_map<uint16_t, nifly::Color3>> morphColorsCache;
	std::vector<std::unordered_map<uint16_t, nifly::Vector3>> morphNormalsCache;
	std::vector<std::unordered_map<uint16_t, nifly::Vector3>> morphTangentsCache;

	bool Read(const std::string& fileName);
	bool Write(const std::string& fileName);

	void CacheToFileData();
	bool FileToCacheData();

	void UpdateCachedMorphData();

	uint32_t GetMorphCount();
	std::vector<std::string> GetMorphNames();

	void SetVertexCount(uint32_t vertexCount) {
		numVertices = vertexCount;
		numOffsets = numVertices;
	}

	bool AddMorph(const std::string& morphName,
				  const std::unordered_map<uint16_t, nifly::Vector3>& morphOffsets,
				  const std::unordered_map<uint16_t, nifly::Color3>& morphColors,
				  const std::unordered_map<uint16_t, nifly::Vector3>& morphNormals,
				  const std::unordered_map<uint16_t, nifly::Vector3>& morphTangents);
};

// How a shape's normals are recalculated, from its project settings and NIF
struct SFMorphNormalSettings {
	// Normals of vertices split along seams are smoothed across them below this angle in degrees
	bool smoothSeamNormals = true;
	float smoothSeamNormalsAngle = 60.0f;
	// The shape's normals stay as they are, so morphs don't change them either
	bool lockNormals = false;
	// Vertices whose normals stay as they are (LOCKEDNORM extra data)
	std::unordered_set<uint32_t> lockedNormals;
};

// The shape a morph.dat belongs to. Besides its position offset, every entry of a morph stores how
// much the vertex normal and tangent change and the vertex color the morph blends to. This derives
// them from the shape for morphs that only come with position offsets.
class SFMorphShape {
public:
	// Vertex positions and normals in the same (NIF) space as the morph offsets. Tangents follow
	// the first UV channel's U direction like those of Starfield meshes. Colors may be empty.
	SFMorphShape(const std::vector<nifly::Vector3>& verts,
				 const std::vector<nifly::Triangle>& tris,
				 const std::vector<nifly::Vector2>& uvs,
				 const std::vector<nifly::Color4>& colors,
				 const SFMorphNormalSettings& settings = SFMorphNormalSettings());

	// The current geometry of a shape. The shape's LOCKEDNORM extra data adds to the locked normals of the settings.
	static SFMorphShape FromShape(nifly::NifFile& nif, nifly::NiShape* shape, SFMorphNormalSettings settings = SFMorphNormalSettings());

	// morph.dat stores normals and tangents as deltas. They are the difference between the normals
	// and tangents recalculated for the morphed and for the base positions, so that only the change
	// applies to the shape's own normals and its split edges and seams stay as they are. Locked
	// normals don't change.
	void CalcNormalDeltas(const std::unordered_map<uint16_t, nifly::Vector3>& offsets,
						  std::unordered_map<uint16_t, nifly::Vector3>& outNormals,
						  std::unordered_map<uint16_t, nifly::Vector3>& outTangents) const;

	// Morphs keep the vertex colors, which vanilla morph.dat files store as the shape's own vertex
	// color. Shapes without vertex colors store black.
	std::unordered_map<uint16_t, nifly::Color3> TargetColors(const std::unordered_map<uint16_t, nifly::Vector3>& offsets) const;

	// Adds a morph with these normal deltas and target colors to the file
	bool AddMorph(SFMorphFile& file, const std::string& morphName, const std::unordered_map<uint16_t, nifly::Vector3>& offsets) const;

private:
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<nifly::Vector2> uvs;
	std::vector<nifly::Color4> colors;
	SFMorphNormalSettings settings;

	// Vertices sharing a position (split along seams or hard edges), smoothed across where the angle allows
	std::vector<std::vector<uint32_t>> weldGroups;

	// Per vertex face normals before smoothing seams, which locked normals keep when morphed
	std::vector<nifly::Vector3> baseFaceNormals;
	std::vector<nifly::Vector3> baseNormals;
	std::vector<nifly::Vector3> baseTangents;

	bool IsLocked(const uint32_t vertex) const { return settings.lockedNormals.count(vertex) != 0; }

	// Face normals summed per vertex, before smoothing seams
	std::vector<nifly::Vector3> CalcFaceNormals(const std::vector<nifly::Vector3>& positions) const;
	void CalcNormals(const std::vector<nifly::Vector3>& positions,
					 std::vector<nifly::Vector3> faceNormals,
					 std::vector<nifly::Vector3>& outNormals,
					 std::vector<nifly::Vector3>& outTangents) const;
};
