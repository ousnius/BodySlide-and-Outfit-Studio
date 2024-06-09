/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMorphFile.h"
#include "../utils/PlatformUtil.h"

#include "half.hpp"

using namespace nifly;

static std::vector<uint32_t> BinaryPositions(uint32_t* n, uint32_t length) {
	std::vector<uint32_t> positions;
	uint32_t count = 0;
	for (size_t i = 0; i < length; i++) {
		uint32_t _n = n[i];
		for (int j = 0; j < 32; j++) {
			if (_n & 1) {
				positions.push_back(count);
			}
			count++;
			_n >>= 1;
		}
	}
	return positions;
}

static Vector3 DecodeUDEC3(const uint32_t data) {
	Vector3 vec;
	vec.x = (float)(((data & 1023) / 511.5) - 1.0);
	vec.y = (float)((((data >> 10) & 1023) / 511.5) - 1.0);
	vec.z = (float)((((data >> 20) & 1023) / 511.5) - 1.0);
	return vec;
}

static uint32_t EncodeUDEC3(const Vector3& vec) {
	uint32_t data;
	data = (((uint32_t)((vec.z + 1.0) * 511.5)) & 1023) << 20;
	data &= (((uint32_t)((vec.y + 1.0) * 511.5)) & 1023) << 10;
	data &= (((uint32_t)((vec.x + 1.0) * 511.5)) & 1023);
	return data;
}

bool SFMorphFile::Read(const std::string& fileName) {
	std::fstream morphFile;
	PlatformUtil::OpenFileStream(morphFile, fileName, std::ios::in | std::ios::binary);

	if (!morphFile.is_open())
		return false;

	char hdr[4];
	morphFile.read(hdr, 4);

	uint32_t magic = "TADM"_mci; // MDAT
	if (memcmp(hdr, &magic, 4) != 0)
		return false;

	morphFile.read((char*)&numAxis, sizeof(numAxis)); // Unknown. Always 3?
	morphFile.read((char*)&numVertices, sizeof(numVertices));
	morphFile.read((char*)&numShapeKeys, sizeof(numShapeKeys));

	for (size_t i = 0; i < numShapeKeys; i++) {
		uint32_t morphNameLength = 0;
		morphFile.read((char*)&morphNameLength, sizeof(morphNameLength));

		std::string morphName;
		morphName.resize(morphNameLength, ' ');
		if (morphNameLength > 0)
			morphFile.read((char*)&morphName.front(), morphNameLength);

		morphNames.push_back(morphName);
	}

	morphFile.read((char*)&numMorphData, sizeof(numMorphData));
	morphFile.read((char*)&numOffsets, sizeof(numOffsets)); // Must always match numVertices?

	morphDataRaw.resize(numMorphData);
	morphFile.read((char*)morphDataRaw.data(), sizeof(SFMorphData) * numMorphData);

	morphDataRawHalf.resize(numMorphData);
	for (size_t i = 0; i < numMorphData; i++) {
		const SFMorphData& morphData = morphDataRaw[i];
		SFMorphDataHalf& morphDataHalf = morphDataRawHalf[i];

		morphDataHalf.offset.x = (float)*((half_float::half*)&morphData.offset[0]);
		morphDataHalf.offset.y = (float)*((half_float::half*)&morphData.offset[1]);
		morphDataHalf.offset.z = (float)*((half_float::half*)&morphData.offset[2]);

		morphDataHalf.targetVertColor = morphData.targetVertColor / float(uint16_t(-1));

		Vector3 n = DecodeUDEC3(morphData.x);
		morphDataHalf.nx = n.x;
		morphDataHalf.ny = n.y;
		morphDataHalf.nz = n.z;

		Vector3 t = DecodeUDEC3(morphData.y);
		morphDataHalf.tx = t.x;
		morphDataHalf.ty = t.y;
		morphDataHalf.tz = t.z;
	}

	offsets.resize(numOffsets);
	morphFile.read((char*)offsets.data(), sizeof(SFMorphOffset) * numOffsets);

	size_t morphDataSize = 0;

	for (size_t i = 0; i < numVertices; i++) {
		SFMorphOffset morphOffset{};
		int size = 0;
		if (numVertices > 0 && i != numVertices - 1)
			size = offsets[i + 1].offset - offsets[i].offset;
		else
			size = numMorphData - offsets[i].offset;

		std::vector<SFMorphData> morphData;
		std::vector<SFMorphDataHalf> morphDataHalf;
		std::vector<uint32_t> morphKeyIndices = BinaryPositions((uint32_t*)offsets[i].keyMarker, 4);

		if (!morphKeyIndices.empty() && morphKeyIndices.back() >= numShapeKeys) {
			// Invalid morph key index
			return false;
		}

		uint32_t t = offsets[i].offset;

		for (size_t j = 0; j < size; j++) {
			morphData.push_back(morphDataRaw[t + j]);
			morphDataHalf.push_back(morphDataRawHalf[t + j]);
		}

		vertexMorphData.push_back(morphData);
		vertexMorphDataHalf.push_back(morphDataHalf);

		vertexMorphKeyIndices.push_back(morphKeyIndices);
		morphDataSize += morphKeyIndices.size();
	}

	if (numMorphData != 1 && morphDataSize != numMorphData) {
		// Invalid morph data size
		return false;
	}

	return true;
}

std::map<std::string, std::unordered_map<uint16_t, nifly::Vector3>>* SFMorphFile::GetCachedMorphData() {
	if (morphOffsetsCache.empty()) {
		// Traditional scale based on havok to unit transform used in skyrim, fallout, etc. In Starfield mesh files are normalized to metric units,
		// this scale makes default vertex positions closely match the older games
		float havokScale = 69.969f;
		// experimentally, the below scale produced very accurate values to SSE mesh sizes (comparing markerxheading.nif)
		// float havokScale = 69.9866f;

		if (numVertices <= std::numeric_limits<uint16_t>().max()) {
			std::unordered_map<uint16_t, Vector3> diff;
			for (uint16_t i = 0; i < numVertices; i++) {
				auto& indices = vertexMorphKeyIndices[i];
				for (int j = 0; j < indices.size(); ++j) {
					auto& id = indices[j];
					auto& data = vertexMorphDataHalf[i][j];
					auto& morphName = morphNames[id];
					morphOffsetsCache[morphName][i] = data.offset * havokScale;
				}
			}
		}
	}

	return &morphOffsetsCache;
}

uint32_t SFMorphFile::GetMorphCount() {
	return numShapeKeys;
}
