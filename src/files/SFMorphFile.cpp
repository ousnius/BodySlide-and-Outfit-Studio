/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMorphFile.h"
#include "../utils/PlatformUtil.h"

#include "half.hpp"

#include <array>

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
	vec.x = static_cast<float>(((data & 1023) / 511.5) - 1.0);
	vec.y = static_cast<float>((((data >> 10) & 1023) / 511.5) - 1.0);
	vec.z = static_cast<float>((((data >> 20) & 1023) / 511.5) - 1.0);
	return vec;
}

static uint32_t EncodeUDEC3(const Vector3& vec) {
	uint32_t data = 0;
	data |= static_cast<std::uint32_t>((vec.x + 1.0f) * 511.5f) & 1023;
	data |= (static_cast<std::uint32_t>((vec.y + 1.0f) * 511.5f) & 1023) << 10;
	data |= (static_cast<std::uint32_t>((vec.z + 1.0f) * 511.5f) & 1023) << 20;

	data |= static_cast<uint8_t>(1.0f) << 30;
	return data;
}

static std::uint16_t EncodeRGB565(uint8_t r, uint8_t g, uint8_t b) {
	uint16_t rgb565 = 0;

	rgb565 |= (r >> 3) << 11;
	rgb565 |= (g >> 2) << 5;
	rgb565 |= (b >> 3) << 0;

	return rgb565;
}

static void DecodeRGB565(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b) {
	r = (rgb565 >> 11) << 3;
	g = ((rgb565 >> 5) & 0x3F) << 2;
	b = (rgb565 & 0x1F) << 3;
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

	offsets.resize(numOffsets);
	morphFile.read((char*)offsets.data(), sizeof(SFMorphOffset) * numOffsets);

	return true;
}

bool SFMorphFile::Write(const std::string& fileName) {
	std::fstream morphFile;
	PlatformUtil::OpenFileStream(morphFile, fileName, std::ios::out | std::ios::binary);

	if (!morphFile.is_open())
		return false;

	uint32_t hdr = "TADM"_mci;
	morphFile.write((char*)&hdr, sizeof(hdr));

	morphFile.write((char*)&numAxis, sizeof(numAxis)); // Unknown. Always 3?
	morphFile.write((char*)&numVertices, sizeof(numVertices));

	numShapeKeys = static_cast<uint32_t>(morphNames.size());
	morphFile.write((char*)&numShapeKeys, sizeof(numShapeKeys));

	for (auto& morphName : morphNames) {
		uint32_t morphNameLength = static_cast<uint32_t>(morphName.length());
		morphFile.write((char*)&morphNameLength, sizeof(morphNameLength));
		if (morphNameLength > 0)
			morphFile.write(morphName.c_str(), morphNameLength);
	}

	numMorphData = static_cast<uint32_t>(morphDataRaw.size());
	morphFile.write((char*)&numMorphData, sizeof(numMorphData));

	numOffsets = numVertices;
	morphFile.write((char*)&numOffsets, sizeof(numOffsets)); // Must always match numVertices?

	morphFile.write((char*)morphDataRaw.data(), sizeof(SFMorphData) * numMorphData);
	morphFile.write((char*)offsets.data(), sizeof(SFMorphOffset) * numOffsets);

	return true;
}

bool SFMorphFile::FileToCacheData() {
	morphDataRawUnpacked.resize(numMorphData);

	for (size_t i = 0; i < numMorphData; i++) {
		const SFMorphData& morphData = morphDataRaw[i];
		SFMorphDataUnpacked& morphDataUnpacked = morphDataRawUnpacked[i];

		morphDataUnpacked.offset.x = (float)*((half_float::half*)&morphData.offset[0]);
		morphDataUnpacked.offset.y = (float)*((half_float::half*)&morphData.offset[1]);
		morphDataUnpacked.offset.z = (float)*((half_float::half*)&morphData.offset[2]);

		DecodeRGB565(morphData.targetVertColor, morphDataUnpacked.targetVertColor.r, morphDataUnpacked.targetVertColor.g, morphDataUnpacked.targetVertColor.b);

		morphDataUnpacked.normal = DecodeUDEC3(morphData.x);
		morphDataUnpacked.tangent = DecodeUDEC3(morphData.y);
	}

	size_t morphDataSize = 0;

	for (size_t i = 0; i < numVertices; i++) {
		int size = 0;
		if (numVertices > 0 && i != numVertices - 1)
			size = offsets[i + 1].offset - offsets[i].offset;
		else
			size = numMorphData - offsets[i].offset;

		std::vector<SFMorphData> morphData;
		std::vector<SFMorphDataUnpacked> morphDataUnpacked;
		std::vector<uint32_t> morphKeyIndices = BinaryPositions((uint32_t*)offsets[i].keyMarker, 4);

		if (!morphKeyIndices.empty() && morphKeyIndices.back() >= numShapeKeys) {
			// Invalid morph key index
			return false;
		}

		uint32_t t = offsets[i].offset;

		for (size_t j = 0; j < size; j++) {
			morphData.push_back(morphDataRaw[t + j]);
			morphDataUnpacked.push_back(morphDataRawUnpacked[t + j]);
		}

		vertexMorphData.push_back(morphData);
		vertexMorphDataUnpacked.push_back(morphDataUnpacked);

		vertexMorphKeyIndices.push_back(morphKeyIndices);
		morphDataSize += morphKeyIndices.size();
	}

	if (numMorphData != 1 && morphDataSize != numMorphData) {
		// Invalid morph data size
		return false;
	}

	return true;
}

void SFMorphFile::CacheToFileData() {
	// Traditional scale based on havok to unit transform used in skyrim, fallout, etc. In Starfield mesh files are normalized to metric units,
	// this scale makes default vertex positions closely match the older games
	const float havokScale = 69.969f;
	// experimentally, the below scale produced very accurate values to SSE mesh sizes (comparing markerxheading.nif)
	// const float havokScale = 69.9866f;

	morphDataRaw.clear();
	morphDataRawUnpacked.clear();
	vertexMorphData.clear();
	vertexMorphDataUnpacked.clear();
	vertexMorphKeyIndices.clear();

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();

	uint16_t numVerticesShort;
	if (numVertices <= maxVertIndex)
		numVerticesShort = static_cast<uint16_t>(numVertices);
	else
		numVerticesShort = maxVertIndex;

	numShapeKeys = static_cast<uint32_t>(morphNames.size());

	for (uint16_t i = 0; i < numVerticesShort; i++) {
		std::vector<uint32_t> morphKeyIndices;

		uint32_t morphKey = 0;
		for (auto& morph : morphNames) {
			if (morphKeyIndices.size() < 128) {
				auto& morphIndex = morphNamesCacheMap[morph];

				auto& morphOffsets = morphOffsetsCache[morphIndex];
				auto& morphColors = morphColorsCache[morphIndex];
				auto& morphNormals = morphNormalsCache[morphIndex];
				auto& morphTangents = morphTangentsCache[morphIndex];

				auto morphOffsetIt = morphOffsets.find(i);
				if (morphOffsetIt != morphOffsets.end()) {
					if (std::fabs(morphOffsetIt->second.x) > EPSILON || std::fabs(morphOffsetIt->second.y) > EPSILON || std::fabs(morphOffsetIt->second.z) > EPSILON) {
						SFMorphData morphData{};

						half_float::half hx(morphOffsetIt->second.x / havokScale);
						half_float::half hy(morphOffsetIt->second.y / havokScale);
						half_float::half hz(morphOffsetIt->second.z / havokScale);
						morphData.offset[0] = *reinterpret_cast<uint16_t*>(&hx);
						morphData.offset[1] = *reinterpret_cast<uint16_t*>(&hy);
						morphData.offset[2] = *reinterpret_cast<uint16_t*>(&hz);

						auto morphColorIt = morphColors.find(i);
						if (morphColorIt != morphColors.end()) {
							nifly::ByteColor3 color;

							float f = std::max(0.0f, std::min(1.0f, morphColorIt->second.r));
							color.r = static_cast<uint8_t>(std::floor(f == 1.0f ? 255 : f * 256.0));

							f = std::max(0.0f, std::min(1.0f, morphColorIt->second.g));
							color.g = static_cast<uint8_t>(std::floor(f == 1.0f ? 255 : f * 256.0));

							f = std::max(0.0f, std::min(1.0f, morphColorIt->second.b));
							color.b = static_cast<uint8_t>(std::floor(f == 1.0f ? 255 : f * 256.0));

							morphData.targetVertColor = EncodeRGB565(color.r, color.g, color.b);
						}

						auto morphNormalIt = morphNormals.find(i);
						if (morphNormalIt != morphNormals.end())
							morphData.x = EncodeUDEC3(morphNormalIt->second);

						auto morphTangentIt = morphTangents.find(i);
						if (morphTangentIt != morphTangents.end())
							morphData.y = EncodeUDEC3(morphTangentIt->second);

						morphDataRaw.push_back(morphData);

						morphKeyIndices.push_back(morphKey);
						morphKey++;
					}
				}
			}
		}

		vertexMorphKeyIndices.push_back(morphKeyIndices);
	}

	numMorphData = static_cast<uint32_t>(morphDataRaw.size());

	uint32_t offset = 0;
	for (uint16_t i = 0; i < numVerticesShort; i++) {
		SFMorphOffset offsetData{};
		offsetData.offset = offset;

		auto& morphKeyIndices = vertexMorphKeyIndices[i];

		std::array<uint32_t, 4> binaryMarkers{};
		for (int p = 0; p < morphKeyIndices.size(); p++)
			binaryMarkers[morphKeyIndices[p] / 32] |= 1 << (morphKeyIndices[p] % 32);

		offsetData.keyMarker[0] = static_cast<SFMorphKey>(binaryMarkers[0]);
		offsetData.keyMarker[1] = static_cast<SFMorphKey>(binaryMarkers[1]);
		offsetData.keyMarker[2] = static_cast<SFMorphKey>(binaryMarkers[2]);
		offsetData.keyMarker[3] = static_cast<SFMorphKey>(binaryMarkers[3]);

		offsets.push_back(offsetData);
		offset += static_cast<uint32_t>(morphKeyIndices.size());
	}
}

void SFMorphFile::UpdateCachedMorphData() {
	if (morphOffsetsCache.empty()) {
		// Traditional scale based on havok to unit transform used in skyrim, fallout, etc. In Starfield mesh files are normalized to metric units,
		// this scale makes default vertex positions closely match the older games
		const float havokScale = 69.969f;
		// experimentally, the below scale produced very accurate values to SSE mesh sizes (comparing markerxheading.nif)
		// const float havokScale = 69.9866f;

		if (numVertices <= std::numeric_limits<uint16_t>().max()) {
			std::unordered_map<uint16_t, Vector3> diff;
			for (uint16_t i = 0; i < numVertices; i++) {
				auto& indices = vertexMorphKeyIndices[i];
				for (int j = 0; j < indices.size(); ++j) {
					auto& id = indices[j];
					auto& morphName = morphNames[id];
					auto& data = vertexMorphDataUnpacked[i][j];

					Vector3 offset(data.offset * havokScale);
					Color3 targetVertColor(data.targetVertColor.r / 255.0f, data.targetVertColor.g / 255.0f, data.targetVertColor.b / 255.0f);

					auto morphCacheIt = morphNamesCacheMap.find(morphName);
					if (morphCacheIt == morphNamesCacheMap.end()) {
						morphNamesCacheMap[morphName] = static_cast<uint32_t>(morphOffsetsCache.size());
						morphOffsetsCache.emplace_back()[i] = offset;
						morphColorsCache.emplace_back()[i] = targetVertColor;
						morphNormalsCache.emplace_back()[i] = data.normal;
						morphTangentsCache.emplace_back()[i] = data.tangent;
					}
					else {
						morphOffsetsCache[morphCacheIt->second][i] = offset;
						morphColorsCache[morphCacheIt->second][i] = targetVertColor;
						morphNormalsCache[morphCacheIt->second][i] = data.normal;
						morphTangentsCache[morphCacheIt->second][i] = data.tangent;
					}
				}
			}
		}
	}
}

bool SFMorphFile::AddMorph(const std::string& morphName,
						   const std::unordered_map<uint16_t, nifly::Vector3>& morphOffsets,
						   const std::unordered_map<uint16_t, nifly::Color3>& morphColors,
						   const std::unordered_map<uint16_t, nifly::Vector3>& morphNormals,
						   const std::unordered_map<uint16_t, nifly::Vector3>& morphTangents) {
	auto morphIt = morphNamesCacheMap.find(morphName);
	if (morphIt != morphNamesCacheMap.end())
		return false;

	morphNamesCacheMap[morphName] = static_cast<uint32_t>(morphOffsetsCache.size());
	morphNames.push_back(morphName);

	morphOffsetsCache.push_back(morphOffsets);
	morphColorsCache.push_back(morphColors);
	morphNormalsCache.push_back(morphNormals);
	morphTangentsCache.push_back(morphTangents);
	return true;
}

uint32_t SFMorphFile::GetMorphCount() {
	return numShapeKeys;
}

std::vector<std::string> SFMorphFile::GetMorphNames() {
	return morphNames;
}
