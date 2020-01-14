/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "TriFile.h"
#include "../utils/PlatformUtil.h"

bool TriFile::Read(const std::string& fileName) {
	std::fstream triFile;
	PlatformUtil::OpenFileStream(triFile, fileName, std::ios::in | std::ios::binary);

	if (triFile.is_open()) {
		char hdr[4];
		triFile.read(hdr, 4);

		uint magic = 'TRIP';
		if (memcmp(hdr, &magic, 4) != 0)
			return false;

		// Load position offsets
		ushort shapeCount = 0;
		triFile.read((char*)&shapeCount, 2);

		for (int i = 0; i < shapeCount; i++) {
			byte shapeLength = 0;
			std::string shapeName;
			triFile.read((char*)&shapeLength, 1);
			shapeName.resize(shapeLength, ' ');
			if (shapeLength > 0)
				triFile.read((char*)&shapeName.front(), shapeLength);

			ushort morphCount = 0;
			triFile.read((char*)&morphCount, 2);

			for (int j = 0; j < morphCount; j++) {
				byte morphLength = 0;
				std::string morphName;
				triFile.read((char*)&morphLength, 1);
				morphName.resize(morphLength, ' ');
				if (morphLength > 0)
					triFile.read((char*)&morphName.front(), morphLength);

				std::map<int, Vector3> morphOffsets;
				float mult = 0.0f;
				ushort morphVertCount = 0;
				triFile.read((char*)&mult, 4);
				triFile.read((char*)&morphVertCount, 2);

				for (int k = 0; k < morphVertCount; k++) {
					ushort id = 0;
					short x = 0;
					short y = 0;
					short z = 0;
					triFile.read((char*)&id, 2);
					triFile.read((char*)&x, 2);
					triFile.read((char*)&y, 2);
					triFile.read((char*)&z, 2);

					Vector3 offset = Vector3(x * mult, y * mult, z * mult);
					if (!offset.IsZero(true))
						morphOffsets.emplace(id, offset);
				}

				if (morphOffsets.size() > 0) {
					MorphDataPtr morph = std::make_shared<MorphData>();
					morph->name = morphName;
					morph->type = MORPHTYPE_POSITION;
					morph->offsets = morphOffsets;
					AddMorph(shapeName, morph);
				}
			}
		}

		// Load UV offsets if EOF wasn't reached
		if (!triFile.eof()) {
			ushort shapeCountUV = 0;
			triFile.read((char*)&shapeCountUV, 2);

			for (int i = 0; i < shapeCountUV; i++) {
				byte shapeLength = 0;
				std::string shapeName;
				triFile.read((char*)&shapeLength, 1);
				shapeName.resize(shapeLength, ' ');
				if (shapeLength > 0)
					triFile.read((char*)&shapeName.front(), shapeLength);

				ushort morphCount = 0;
				triFile.read((char*)&morphCount, 2);

				for (int j = 0; j < morphCount; j++) {
					byte morphLength = 0;
					std::string morphName;
					triFile.read((char*)&morphLength, 1);
					morphName.resize(morphLength, ' ');
					if (morphLength > 0)
						triFile.read((char*)&morphName.front(), morphLength);

					std::map<int, Vector3> morphOffsets;
					float mult = 0.0f;
					ushort morphVertCount = 0;
					triFile.read((char*)&mult, 4);
					triFile.read((char*)&morphVertCount, 2);

					for (int k = 0; k < morphVertCount; k++) {
						ushort id = 0;
						short x = 0;
						short y = 0;
						triFile.read((char*)&id, 2);
						triFile.read((char*)&x, 2);
						triFile.read((char*)&y, 2);

						Vector3 offset = Vector3(x * mult, y * mult, 0.0f);
						if (!offset.IsZero(true))
							morphOffsets.emplace(id, offset);
					}

					if (morphOffsets.size() > 0) {
						MorphDataPtr morph = std::make_shared<MorphData>();
						morph->name = morphName;
						morph->type = MORPHTYPE_UV;
						morph->offsets = morphOffsets;
						AddMorph(shapeName, morph);
					}
				}
			}
		}
	}
	else
		return false;

	return true;
}

bool TriFile::Write(const std::string& fileName) {
	std::fstream triFile;
	PlatformUtil::OpenFileStream(triFile, fileName, std::ios::out | std::ios::binary);

	if (triFile.is_open()) {
		uint hdr = 'TRIP';
		triFile.write((char*)&hdr, 4);

		ushort shapeCount = GetShapeCount(MORPHTYPE_POSITION);
		triFile.write((char*)&shapeCount, 2);

		// Write position offsets
		if (shapeCount > 0) {
			for (auto& shape : shapeMorphs) {
				ushort morphCount = GetMorphCount(shape.first, MORPHTYPE_POSITION);
				if (morphCount > 0) {
					byte shapeLength = shape.first.length();
					std::string shapeName = shape.first;
					triFile.write((char*)&shapeLength, 1);
					if (shapeLength > 0)
						triFile.write(shapeName.c_str(), shapeLength);

					triFile.write((char*)&morphCount, 2);

					for (auto& morph : shape.second) {
						if (morph->type != MORPHTYPE_POSITION)
							continue;

						byte morphLength = morph->name.length();
						std::string morphName = morph->name;
						triFile.write((char*)&morphLength, 1);
						if (morphLength > 0)
							triFile.write(morphName.c_str(), morphLength);

						float mult = 0.0f;
						for (auto& v : morph->offsets) {
							if (std::abs(v.second.x) > mult)
								mult = std::abs(v.second.x);
							if (std::abs(v.second.y) > mult)
								mult = std::abs(v.second.y);
							if (std::abs(v.second.z) > mult)
								mult = std::abs(v.second.z);
						}

						mult /= 0x7FFF;
						triFile.write((char*)&mult, 4);

						ushort morphVertCount = morph->offsets.size();
						triFile.write((char*)&morphVertCount, 2);

						for (auto& v : morph->offsets) {
							ushort id = v.first;
							short x = v.second.x / mult;
							short y = v.second.y / mult;
							short z = v.second.z / mult;
							triFile.write((char*)&id, 2);
							triFile.write((char*)&x, 2);
							triFile.write((char*)&y, 2);
							triFile.write((char*)&z, 2);
						}
					}
				}
			}
		}

		// Write UV offsets
		ushort shapeCountUV = GetShapeCount(MORPHTYPE_UV);
		triFile.write((char*)&shapeCountUV, 2);

		if (shapeCountUV > 0) {
			for (auto& shape : shapeMorphs) {
				ushort morphCount = GetMorphCount(shape.first, MORPHTYPE_UV);
				if (morphCount > 0) {
					byte shapeLength = shape.first.length();
					std::string shapeName = shape.first;
					triFile.write((char*)&shapeLength, 1);
					if (shapeLength > 0)
						triFile.write(shapeName.c_str(), shapeLength);

					triFile.write((char*)&morphCount, 2);

					for (auto& morph : shape.second) {
						if (morph->type != MORPHTYPE_UV)
							continue;

						byte morphLength = morph->name.length();
						std::string morphName = morph->name;
						triFile.write((char*)&morphLength, 1);
						if (morphLength > 0)
							triFile.write(morphName.c_str(), morphLength);

						float mult = 0.0f;
						for (auto& v : morph->offsets) {
							if (std::abs(v.second.x) > mult)
								mult = std::abs(v.second.x);
							if (std::abs(v.second.y) > mult)
								mult = std::abs(v.second.y);
						}

						mult /= 0x7FFF;
						triFile.write((char*)&mult, 4);

						ushort morphVertCount = morph->offsets.size();
						triFile.write((char*)&morphVertCount, 2);

						for (auto& v : morph->offsets) {
							ushort id = v.first;
							short x = v.second.x / mult;
							short y = v.second.y / mult;
							triFile.write((char*)&id, 2);
							triFile.write((char*)&x, 2);
							triFile.write((char*)&y, 2);
						}
					}
				}
			}
		}
	}
	else
		return false;

	return true;
}

void TriFile::AddMorph(const std::string& shapeName, MorphDataPtr data) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end()) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == data->name) return true; return false; });
		if (morph == shape->second.end())
			shape->second.push_back(data);
	}
	else {
		shapeMorphs.emplace(shapeName, std::vector<MorphDataPtr>());
		AddMorph(shapeName, data);
	}
}

void TriFile::DeleteMorph(const std::string& shapeName, const std::string& morphName) {
	for (auto shape = shapeMorphs.begin(); shape != shapeMorphs.end();) {
		if (shape->first == shapeName) {
			auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == morphName) return true; return false; });
			if (morph != shape->second.end()) {
				shapeMorphs.erase(shape);
				return;
			}
		}
		++shape;
	}
}

void TriFile::DeleteMorphs(const std::string& shapeName) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end())
			shape->second.clear();
}

void TriFile::DeleteMorphFromAll(const std::string& morphName) {
	for (auto shape = shapeMorphs.begin(); shape != shapeMorphs.end();) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == morphName) return true; return false; });
		if (morph != shape->second.end())
			shapeMorphs.erase(shape);
		++shape;
	}
}

MorphDataPtr TriFile::GetMorph(const std::string& shapeName, const std::string& morphName) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end()) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == morphName) return true; return false; });
		if (morph != shape->second.end())
			return *morph;
	}

	return nullptr;
}

std::map<std::string, std::vector<MorphDataPtr>> TriFile::GetMorphs() {
	return shapeMorphs;
}

ushort TriFile::GetShapeCount(MorphType morphType) {
	ushort shapeCount = 0;

	for (auto &shape : shapeMorphs) {
		for (auto& morph : shape.second) {
			if (morph->type == morphType) {
				shapeCount++;
				break;
			}
		}
	}

	return shapeCount;
}

ushort TriFile::GetMorphCount(const std::string& shapeName, MorphType morphType) {
	ushort morphCount = 0;

	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end()) {
		for (auto& morph : shape->second) {
			if (morph->type == morphType)
				morphCount++;
		}
	}

	return morphCount;
}

bool TriHeadFile::Read(const std::string& fileName) {
	std::fstream triHeadFile;
	PlatformUtil::OpenFileStream(triHeadFile, fileName, std::ios::in | std::ios::binary);

	if (triHeadFile.is_open()) {
		identifier.resize(2, ' ');
		triHeadFile.read((char*)&identifier.front(), 2);

		const std::string ident = "FR";
		if (identifier != ident)
			return false;

		fileType.resize(3, ' ');
		triHeadFile.read((char*)&fileType.front(), 3);

		const std::string type = "TRI";
		if (fileType != type)
			return false;

		version.resize(3, ' ');
		triHeadFile.read((char*)&version.front(), 3);

		triHeadFile.read((char*)&numVertices, 4);
		triHeadFile.read((char*)&numTriangles, 4);
		triHeadFile.read((char*)&numQuads, 4);
		triHeadFile.read((char*)&unknown2, 4);
		triHeadFile.read((char*)&unknown3, 4);
		triHeadFile.read((char*)&numUV, 4);
		triHeadFile.read((char*)&flags, 4);
		triHeadFile.read((char*)&numMorphs, 4);
		triHeadFile.read((char*)&numModifiers, 4);
		triHeadFile.read((char*)&numModVertices, 4);
		triHeadFile.read((char*)&unknown7, 4);
		triHeadFile.read((char*)&unknown8, 4);
		triHeadFile.read((char*)&unknown9, 4);
		triHeadFile.read((char*)&unknown10, 4);

		vertices.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			triHeadFile.read((char*)&vertices[i], 12);

		modVertices.resize(numModVertices);
		for (int i = 0; i < numModVertices; i++)
			triHeadFile.read((char*)&modVertices[i], 12);

		triangles.resize(numTriangles);
		for (int i = 0; i < numTriangles; i++) {
			uint x = 0;
			triHeadFile.read((char*)&x, 4);
			uint y = 0;
			triHeadFile.read((char*)&y, 4);
			uint z = 0;
			triHeadFile.read((char*)&z, 4);

			triangles[i] = Triangle(x, y, z);
		}

		uv.resize(numUV);
		for (int i = 0; i < numUV; i++)
			triHeadFile.read((char*)&uv[i], 8);

		tex.resize(numTriangles);
		for (int i = 0; i < numTriangles; i++) {
			uint x = 0;
			triHeadFile.read((char*)&x, 4);
			uint y = 0;
			triHeadFile.read((char*)&y, 4);
			uint z = 0;
			triHeadFile.read((char*)&z, 4);

			tex[i] = Triangle(x, y, z);
		}

		morphs.resize(numMorphs);
		for (int i = 0; i < numMorphs; i++) {
			auto& morph = morphs[i];

			uint morphNameLength = 0;
			triHeadFile.read((char*)&morphNameLength, 4);
			morph.morphName.resize(morphNameLength, ' ');

			if (morphNameLength > 0) {
				triHeadFile.read((char*)&morph.morphName.front(), morphNameLength);
				morph.morphName = morph.morphName.c_str();
			}

			triHeadFile.read((char*)&morph.multiplier, 4);

			morph.vertices.resize(numVertices);
			for (int j = 0; j < numVertices; j++) {
				short x = 0;
				triHeadFile.read((char*)&x, 2);
				short y = 0;
				triHeadFile.read((char*)&y, 2);
				short z = 0;
				triHeadFile.read((char*)&z, 2);

				morph.vertices[j] = Vector3(x * morph.multiplier, y * morph.multiplier, z * morph.multiplier);
			}
		}

		// Read but don't store
		for (int i = 0; i < numModifiers; i++) {
			uint morphNameLength = 0;
			triHeadFile.read((char*)&morphNameLength, 4);

			std::string morphName;
			morphName.resize(morphNameLength, ' ');

			if (morphNameLength > 0)
				triHeadFile.read((char*)&morphName.front(), morphNameLength);

			uint blockLength = 0;
			triHeadFile.read((char*)&blockLength, 4);

			for (int j = 0; j < blockLength; j++) {
				uint index = 0;
				triHeadFile.read((char*)&index, 4);
			}
		}
	}
	else
		return false;

	return true;
}

bool TriHeadFile::Write(const std::string& fileName) {
	std::fstream triHeadFile;
	PlatformUtil::OpenFileStream(triHeadFile, fileName, std::ios::out | std::ios::binary);

	if (triHeadFile.is_open()) {
		triHeadFile.write(identifier.c_str(), 2);
		triHeadFile.write(fileType.c_str(), 3);
		triHeadFile.write(version.c_str(), 3);

		// Not supported yet
		numModVertices = 0;
		numModifiers = 0;

		triHeadFile.write((char*)&numVertices, 4);
		triHeadFile.write((char*)&numTriangles, 4);
		triHeadFile.write((char*)&numQuads, 4);
		triHeadFile.write((char*)&unknown2, 4);
		triHeadFile.write((char*)&unknown3, 4);
		triHeadFile.write((char*)&numUV, 4);
		triHeadFile.write((char*)&flags, 4);
		triHeadFile.write((char*)&numMorphs, 4);
		triHeadFile.write((char*)&numModifiers, 4);
		triHeadFile.write((char*)&numModVertices, 4);
		triHeadFile.write((char*)&unknown7, 4);
		triHeadFile.write((char*)&unknown8, 4);
		triHeadFile.write((char*)&unknown9, 4);
		triHeadFile.write((char*)&unknown10, 4);

		for (int i = 0; i < numVertices; i++)
			triHeadFile.write((char*)&vertices[i], 12);

		for (int i = 0; i < numTriangles; i++) {
			uint x = triangles[i].p1;
			triHeadFile.write((char*)&x, 4);
			uint y = triangles[i].p2;
			triHeadFile.write((char*)&y, 4);
			uint z = triangles[i].p3;
			triHeadFile.write((char*)&z, 4);
		}

		for (int i = 0; i < numUV; i++)
			triHeadFile.write((char*)&uv[i], 8);

		for (int i = 0; i < numTriangles; i++) {
			uint x = tex[i].p1;
			triHeadFile.write((char*)&x, 4);
			uint y = tex[i].p2;
			triHeadFile.write((char*)&y, 4);
			uint z = tex[i].p3;
			triHeadFile.write((char*)&z, 4);
		}

		for (int i = 0; i < numMorphs; i++) {
			auto& morph = morphs[i];

			uint morphNameLength = morph.morphName.length() + 1;
			triHeadFile.write((char*)&morphNameLength, 4);
			if (!morph.morphName.empty()) {
				triHeadFile.write(morph.morphName.c_str(), morph.morphName.length());
				triHeadFile.put('\0');
			}

			morph.multiplier = 0.0f;
			for (auto& v : morph.vertices) {
				if (std::abs(v.x) > morph.multiplier)
					morph.multiplier = std::abs(v.x);
				if (std::abs(v.y) > morph.multiplier)
					morph.multiplier = std::abs(v.y);
				if (std::abs(v.z) > morph.multiplier)
					morph.multiplier = std::abs(v.z);
			}

			morph.multiplier /= 0x7FFF;
			triHeadFile.write((char*)&morph.multiplier, 4);

			for (auto& v : morph.vertices) {
				short x = v.x / morph.multiplier;
				short y = v.y / morph.multiplier;
				short z = v.z / morph.multiplier;
				triHeadFile.write((char*)&x, 2);
				triHeadFile.write((char*)&y, 2);
				triHeadFile.write((char*)&z, 2);
			}
		}
	}
	else
		return false;

	return true;
}

std::vector<Vector3> TriHeadFile::GetVertices() {
	return vertices;
}

std::vector<Triangle> TriHeadFile::GetTriangles() {
	return triangles;
}

std::vector<Vector2> TriHeadFile::GetUV() {
	return uv;
}

void TriHeadFile::SetVertices(const std::vector<Vector3> verts) {
	vertices = verts;
	numVertices = vertices.size();
}

void TriHeadFile::SetTriangles(const std::vector<Triangle> tris) {
	triangles = tris;
	tex = triangles;
	numTriangles = triangles.size();
}

void TriHeadFile::SetUV(const std::vector<Vector2> uvs) {
	uv = uvs;
	numUV = uv.size();
}

void TriHeadFile::AddMorph(const TriHeadMorph& morph) {
	morphs.push_back(morph);
	numMorphs++;
}

void TriHeadFile::DeleteMorph(const std::string& morphName) {
	morphs.erase(std::remove_if(morphs.begin(), morphs.end(),
		[&morphName](const TriHeadMorph& morph) { return morph.morphName == morphName; }), morphs.end());

	numMorphs = morphs.size();
}

TriHeadMorph* TriHeadFile::GetMorph(const std::string& morphName) {
	for (auto &m : morphs) {
		if (m.morphName == morphName) {
			return &m;
		}
	}

	return nullptr;
}

std::vector<TriHeadMorph> TriHeadFile::GetMorphs() {
	return morphs;
}
