#include "TriFile.h"

int TriFile::Read(string fileName) {
	ifstream triFile(fileName.c_str(), ios_base::binary);

	if (triFile.is_open()) {
		bool packed = false;
		int packedBytes = 4;

		char hdr[4];
		triFile.read(hdr, 4);
		if (memcmp(hdr, "PIRT", 4) == 0) {
			packed = true;
			packedBytes = 2;
		}
		else if (memcmp(hdr, "\0IRT", 4) != 0)
			return false;

		uint shapeCount = 0;
		triFile.read((char*)&shapeCount, packedBytes);

		for (int i = 0; i < shapeCount; i++) {
			byte shapeLength = 0;
			string shapeName = "";
			triFile.read((char*)&shapeLength, 1);
			shapeName.resize(shapeLength + 1);
			triFile.read((char*)&shapeName, shapeLength);

			uint morphCount = 0;
			triFile.read((char*)&morphCount, packedBytes);

			for (int j = 0; j < morphCount; j++) {
				byte morphLength = 0;
				string morphName = "";
				triFile.read((char*)&morphLength, 1);
				morphName.resize(morphLength + 1);
				triFile.read((char*)&morphName, morphLength);

				map<int, vec3> morphOffsets;
				if (packed) {
					float mult = 0.0f;
					ushort morphVertCount = 0;
					triFile.read((char*)&mult, 4);
					triFile.read((char*)&morphVertCount, packedBytes);

					for (int k = 0; k < morphVertCount; k++) {
						ushort id = 0;
						short x = 0;
						short y = 0;
						short z = 0;
						triFile.read((char*)&id, 2);
						triFile.read((char*)&x, 2);
						triFile.read((char*)&y, 2);
						triFile.read((char*)&z, 2);

						vec3 offset = vec3(x * mult, y * mult, z * mult);
						if (!offset.IsZero(true))
							morphOffsets.emplace(id, offset);
					}
				}
				else {
					uint morphVertCount = 0;
					triFile.read((char*)&morphVertCount, packedBytes);

					for (int k = 0; k < morphVertCount; k++) {
						uint id = 0;
						vec3 offset = vec3();
						triFile.read((char*)&id, 4);
						triFile.read((char*)&offset, 12);

						if (!offset.IsZero(true))
							morphOffsets.emplace(id, offset);
					}
				}

				if (morphOffsets.size() > 0) {
					MorphDataPtr morph = make_shared<MorphData>();
					morph->name = morphName;
					morph->offsets = morphOffsets;
					AddMorph(shapeName, morph);
				}
			}
		}
	}
	else
		return false;

	return true;
}

int TriFile::Write(string fileName, bool packed) {
	ofstream triFile(fileName.c_str(), ios_base::binary);

	if (triFile.is_open()) {
		int packedBytes = 2;
		uint hdr = 'TRIP';

		if (!packed) {
			hdr = 'TRI\0';
			packedBytes = 4;
		}
		triFile.write((char*)&hdr, 4);

		uint shapeCount = shapeMorphs.size();
		triFile.write((char*)&shapeCount, packedBytes);

		for (auto& shape : shapeMorphs) {
			byte shapeLength = shape.first.length();
			string shapeName = shape.first;
			triFile.write((char*)&shapeLength, 1);
			triFile.write(shapeName.c_str(), shapeLength);

			uint morphCount = shape.second.size();
			triFile.write((char*)&morphCount, packedBytes);

			for (auto& morph : shape.second) {
				byte morphLength = morph->name.length();
				string morphName = morph->name;
				triFile.write((char*)&morphLength, 1);
				triFile.write(morphName.c_str(), morphLength);

				if (packed) {
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
					triFile.write((char*)&morphVertCount, packedBytes);

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
				else {
					uint morphVertCount = morph->offsets.size();
					triFile.write((char*)&morphVertCount, packedBytes);

					for (auto& v : morph->offsets) {
						uint id = v.first;
						vec3 offset = vec3(v.second.x, v.second.y, v.second.z);
						triFile.write((char*)&id, 4);
						triFile.write((char*)&offset, 12);
					}
				}
			}
		}
	}
	else
		return false;

	return true;
}

void TriFile::AddMorph(string shapeName, MorphDataPtr data) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end()) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == data->name) return true; return false; });
		if (morph == shape->second.end())
			shape->second.push_back(data);
	}
	else {
		shapeMorphs.emplace(shapeName, vector<MorphDataPtr>());
		AddMorph(shapeName, data);
	}
}

void TriFile::DeleteMorph(string shapeName, string morphName) {
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

void TriFile::DeleteMorphs(string shapeName) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end())
			shape->second.clear();
}

void TriFile::DeleteMorphFromAll(string morphName) {
	for (auto shape = shapeMorphs.begin(); shape != shapeMorphs.end();) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == morphName) return true; return false; });
		if (morph != shape->second.end())
			shapeMorphs.erase(shape);
		++shape;
	}
}

MorphDataPtr TriFile::GetMorph(string shapeName, string morphName) {
	auto shape = shapeMorphs.find(shapeName);
	if (shape != shapeMorphs.end()) {
		auto morph = find_if(shape->second.begin(), shape->second.end(), [&](MorphDataPtr searchData){ if (searchData->name == morphName) return true; return false; });
		if (morph != shape->second.end())
			return *morph;
	}

	return NULL;
}
