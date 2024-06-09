/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <map>
#include <memory>

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

struct SFMorphDataHalf {
	nifly::Vector3 offset;
	float targetVertColor = 0.0f;
	float nx = 0.0f, ny = 0.0f, nz = 0.0f;
	float tx = 0.0f, ty = 0.0f, tz = 0.0f;
};

struct SFMorphOffset {
	uint32_t offset = 0;
	SFMorphKey keyMarker[4];
};

class SFMorphFile {
	uint32_t numAxis = 0;
	uint32_t numShapeKeys = 0;
	uint32_t numVertices = 0;

	std::vector<std::string> morphNames;
	uint32_t numMorphData = 0;
	uint32_t numOffsets = 0;

	std::vector<SFMorphData> morphDataRaw;
	std::vector<SFMorphDataHalf> morphDataRawHalf;
	std::vector<SFMorphOffset> offsets;

	std::vector<std::vector<SFMorphData>> vertexMorphData;
	std::vector<std::vector<SFMorphDataHalf>> vertexMorphDataHalf;
	std::vector<std::vector<uint32_t>> vertexMorphKeyIndices;
	std::vector<std::vector<uint8_t>> vertexMorphKeySelection;

	std::map<std::string, std::unordered_map<uint16_t, nifly::Vector3>> morphOffsetsCache;

public:
	bool Read(const std::string& fileName);

	std::map<std::string, std::unordered_map<uint16_t, nifly::Vector3>>* GetCachedMorphData();

	uint32_t GetMorphCount();
};

