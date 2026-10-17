/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMorphFile.h"
#include "../utils/PlatformUtil.h"

#include "NifFile.hpp"
#include "half.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

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

// Rounds to the nearest of the 1024 steps DecodeUDEC3 maps to, so that decoded values encode
// back to the same bits. Truncating instead drops values one step whenever the float math lands
// just below a step, and casting a negative value to unsigned is undefined.
static uint32_t EncodeUDEC3Component(const float value) {
	const long step = std::lround((static_cast<double>(value) + 1.0) * 511.5);
	return static_cast<uint32_t>(std::clamp(step, 0L, 1023L));
}

static uint32_t EncodeUDEC3(const Vector3& vec) {
	uint32_t data = 0;
	data |= EncodeUDEC3Component(vec.x);
	data |= EncodeUDEC3Component(vec.y) << 10;
	data |= EncodeUDEC3Component(vec.z) << 20;

	data |= static_cast<uint8_t>(1.0f) << 30;
	return data;
}

// Rounds each channel to the nearest of its 32 or 64 steps. Vanilla morph.dat files store the
// vertex color 192 of a shape as 23 (of 31), which shifting the 8 bit value down would make 24.
static std::uint16_t EncodeRGB565(const Color3& color) {
	auto channel = [](const float value, const long maxStep) {
		return static_cast<uint16_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * maxStep));
	};

	uint16_t rgb565 = 0;
	rgb565 |= channel(color.r, 31) << 11;
	rgb565 |= channel(color.g, 63) << 5;
	rgb565 |= channel(color.b, 31) << 0;
	return rgb565;
}

// Spreads the steps over the full 0-255 range, so that white stays white and EncodeRGB565 gives
// back the same bits
static void DecodeRGB565(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b) {
	r = static_cast<uint8_t>(std::lround((rgb565 >> 11) * 255.0 / 31.0));
	g = static_cast<uint8_t>(std::lround(((rgb565 >> 5) & 0x3F) * 255.0 / 63.0));
	b = static_cast<uint8_t>(std::lround((rgb565 & 0x1F) * 255.0 / 31.0));
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
		size_t size = 0;
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

		for (auto& morph : morphNames) {
			auto& morphIndex = morphNamesCacheMap[morph];

			// The 128-bit keyMarker can only address shape keys 0-127, so a morph
			// beyond that index cannot be represented in the morph.dat format. Skip
			// it rather than setting a bit past the end of the 4-word keyMarker.
			if (morphIndex < SFMaxShapeKeys) {
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
						if (morphColorIt != morphColors.end())
							morphData.targetVertColor = EncodeRGB565(morphColorIt->second);

						auto morphNormalIt = morphNormals.find(i);
						if (morphNormalIt != morphNormals.end())
							morphData.x = EncodeUDEC3(morphNormalIt->second);

						auto morphTangentIt = morphTangents.find(i);
						if (morphTangentIt != morphTangents.end())
							morphData.y = EncodeUDEC3(morphTangentIt->second);

						morphDataRaw.push_back(morphData);

						// The key marker bit must be the morph's index in the shape key name
						// table, not a per-vertex sequence number. Consumers pair the vertex's
						// data run with the set bits in ascending order, so a sequence number
						// misattributes every morph's deltas once two or more morphs overlap.
						morphKeyIndices.push_back(morphIndex);
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
		for (size_t p = 0; p < morphKeyIndices.size(); p++)
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
				for (size_t j = 0; j < indices.size(); ++j) {
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

SFMorphShape::SFMorphShape(const std::vector<Vector3>& verts,
						   const std::vector<Triangle>& tris,
						   const std::vector<Vector2>& uvs,
						   const std::vector<Color4>& colors,
						   const SFMorphNormalSettings& settings)
	: verts(verts)
	, tris(tris)
	, uvs(uvs)
	, colors(colors)
	, settings(settings) {
	if (settings.smoothSeamNormals) {
		std::map<std::tuple<float, float, float>, std::vector<uint32_t>> byPosition;
		for (uint32_t i = 0; i < static_cast<uint32_t>(verts.size()); i++)
			byPosition[std::make_tuple(verts[i].x, verts[i].y, verts[i].z)].push_back(i);

		for (auto& position : byPosition)
			if (position.second.size() > 1)
				weldGroups.push_back(std::move(position.second));
	}

	baseFaceNormals = CalcFaceNormals(verts);
	CalcNormals(verts, baseFaceNormals, baseNormals, baseTangents);
}

SFMorphShape SFMorphShape::FromShape(NifFile& nif, NiShape* shape, SFMorphNormalSettings settings) {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	std::vector<Color4> colors;
	nif.GetVertsForShape(shape, verts);
	nif.GetUvsForShape(shape, uvs);
	shape->GetTriangles(tris);
	nif.GetColorsForShape(shape, colors);

	for (auto& extraDataRef : shape->extraDataRefs) {
		auto integersExtraData = nif.GetHeader().GetBlock<NiIntegersExtraData>(extraDataRef);
		if (integersExtraData && integersExtraData->name == "LOCKEDNORM")
			for (auto& i : integersExtraData->integersData)
				settings.lockedNormals.insert(i);
	}

	return SFMorphShape(verts, tris, uvs, colors, settings);
}

std::vector<Vector3> SFMorphShape::CalcFaceNormals(const std::vector<Vector3>& positions) const {
	const size_t numVerts = positions.size();
	std::vector<Vector3> normals(numVerts);

	for (const Triangle& t : tris) {
		if (t.p1 >= numVerts || t.p2 >= numVerts || t.p3 >= numVerts)
			continue;

		// Area weighted
		const Vector3 normal = t.trinormal(positions);
		normals[t.p1] += normal;
		normals[t.p2] += normal;
		normals[t.p3] += normal;
	}

	for (auto& n : normals)
		n.Normalize();

	return normals;
}

void SFMorphShape::CalcNormals(const std::vector<Vector3>& positions,
							   std::vector<Vector3> faceNormals,
							   std::vector<Vector3>& outNormals,
							   std::vector<Vector3>& outTangents) const {
	const size_t numVerts = positions.size();
	outNormals = std::move(faceNormals);
	outTangents.assign(numVerts, Vector3());

	// Smooth across split vertices unless they form a hard edge. Locked normals keep theirs.
	const float smoothAngle = settings.smoothSeamNormalsAngle * DEG2RAD;
	std::vector<Vector3> smoothed;
	for (const auto& group : weldGroups) {
		smoothed.resize(group.size());
		for (size_t j = 0; j < group.size(); j++) {
			const Vector3& n = outNormals[group[j]];
			Vector3 sum = n;
			if (!IsLocked(group[j])) {
				for (size_t k = 0; k < group.size(); k++)
					if (k != j && n.angle(outNormals[group[k]]) < smoothAngle)
						sum += outNormals[group[k]];

				sum.Normalize();
			}
			smoothed[j] = sum;
		}

		for (size_t j = 0; j < group.size(); j++)
			outNormals[group[j]] = smoothed[j];
	}

	if (uvs.size() == numVerts) {
		for (const Triangle& t : tris) {
			if (t.p1 >= numVerts || t.p2 >= numVerts || t.p3 >= numVerts)
				continue;

			const Vector3 e1 = positions[t.p2] - positions[t.p1];
			const Vector3 e2 = positions[t.p3] - positions[t.p1];
			const float s1 = uvs[t.p2].u - uvs[t.p1].u;
			const float s2 = uvs[t.p3].u - uvs[t.p1].u;
			const float t1 = uvs[t.p2].v - uvs[t.p1].v;
			const float t2 = uvs[t.p3].v - uvs[t.p1].v;
			const float r = (s1 * t2 - s2 * t1) >= 0.0f ? 1.0f : -1.0f;

			Vector3 uDir = (e1 * t2 - e2 * t1) * r;
			uDir.Normalize();
			outTangents[t.p1] += uDir;
			outTangents[t.p2] += uDir;
			outTangents[t.p3] += uDir;
		}
	}

	// Perpendicular to the normal
	for (size_t i = 0; i < numVerts; i++) {
		const Vector3& n = outNormals[i];
		Vector3& tangent = outTangents[i];
		tangent -= n * n.dot(tangent);

		if (tangent.IsZero(true)) {
			tangent = n.cross(std::fabs(n.x) < 0.9f ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f));
			if (tangent.IsZero(true))
				continue;
		}

		tangent.Normalize();
	}
}

void SFMorphShape::CalcNormalDeltas(const std::unordered_map<uint16_t, Vector3>& offsets,
									std::unordered_map<uint16_t, Vector3>& outNormals,
									std::unordered_map<uint16_t, Vector3>& outTangents) const {
	if (settings.lockNormals) {
		for (auto& offset : offsets) {
			if (offset.first >= verts.size())
				continue;

			outNormals[offset.first] = Vector3();
			outTangents[offset.first] = Vector3();
		}
		return;
	}

	std::vector<Vector3> morphed = verts;
	for (auto& [vertex, offset] : offsets)
		if (vertex < morphed.size())
			morphed[vertex] += offset;

	// Locked normals stay as they are, and seams smooth across them unchanged
	std::vector<Vector3> faceNormals = CalcFaceNormals(morphed);
	for (uint32_t vertex : settings.lockedNormals)
		if (vertex < faceNormals.size())
			faceNormals[vertex] = baseFaceNormals[vertex];

	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	CalcNormals(morphed, std::move(faceNormals), normals, tangents);

	for (auto& offset : offsets) {
		const uint16_t vertex = offset.first;
		if (vertex >= morphed.size())
			continue;

		if (IsLocked(vertex)) {
			outNormals[vertex] = Vector3();
			outTangents[vertex] = Vector3();
			continue;
		}

		outNormals[vertex] = normals[vertex] - baseNormals[vertex];
		outTangents[vertex] = tangents[vertex] - baseTangents[vertex];
	}
}

std::unordered_map<uint16_t, Color3> SFMorphShape::TargetColors(const std::unordered_map<uint16_t, Vector3>& offsets) const {
	std::unordered_map<uint16_t, Color3> targetColors;
	for (auto& offset : offsets) {
		const uint16_t vertex = offset.first;
		if (vertex < colors.size())
			targetColors[vertex] = Color3(colors[vertex].r, colors[vertex].g, colors[vertex].b);
		else
			targetColors[vertex] = Color3(0.0f, 0.0f, 0.0f);
	}
	return targetColors;
}

bool SFMorphShape::AddMorph(SFMorphFile& file, const std::string& morphName, const std::unordered_map<uint16_t, Vector3>& offsets) const {
	std::unordered_map<uint16_t, Vector3> normals;
	std::unordered_map<uint16_t, Vector3> tangents;
	CalcNormalDeltas(offsets, normals, tangents);
	return file.AddMorph(morphName, offsets, TargetColors(offsets), normals, tangents);
}
