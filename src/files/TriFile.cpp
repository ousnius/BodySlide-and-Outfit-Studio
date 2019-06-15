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
			triFile.read((char*)&shapeName.front(), shapeLength);

			ushort morphCount = 0;
			triFile.read((char*)&morphCount, 2);

			for (int j = 0; j < morphCount; j++) {
				byte morphLength = 0;
				std::string morphName;
				triFile.read((char*)&morphLength, 1);
				morphName.resize(morphLength, ' ');
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
				triFile.read((char*)&shapeName.front(), shapeLength);

				ushort morphCount = 0;
				triFile.read((char*)&morphCount, 2);

				for (int j = 0; j < morphCount; j++) {
					byte morphLength = 0;
					std::string morphName;
					triFile.read((char*)&morphLength, 1);
					morphName.resize(morphLength, ' ');
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
					triFile.write(shapeName.c_str(), shapeLength);
					triFile.write((char*)&morphCount, 2);

					for (auto& morph : shape.second) {
						if (morph->type != MORPHTYPE_POSITION)
							continue;

						byte morphLength = morph->name.length();
						std::string morphName = morph->name;
						triFile.write((char*)&morphLength, 1);
						triFile.write(morphName.c_str(), morphLength);

						float mult = 0.0f;
						for (auto& v : morph->offsets) {
							if (abs(v.second.x) > mult)
								mult = abs(v.second.x);
							if (abs(v.second.y) > mult)
								mult = abs(v.second.y);
							if (abs(v.second.z) > mult)
								mult = abs(v.second.z);
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
					triFile.write(shapeName.c_str(), shapeLength);
					triFile.write((char*)&morphCount, 2);

					for (auto& morph : shape.second) {
						if (morph->type != MORPHTYPE_UV)
							continue;

						byte morphLength = morph->name.length();
						std::string morphName = morph->name;
						triFile.write((char*)&morphLength, 1);
						triFile.write(morphName.c_str(), morphLength);

						float mult = 0.0f;
						for (auto& v : morph->offsets) {
							if (abs(v.second.x) > mult)
								mult = abs(v.second.x);
							if (abs(v.second.y) > mult)
								mult = abs(v.second.y);
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
