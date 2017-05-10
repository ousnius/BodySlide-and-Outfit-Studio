/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Geometry.h"
#include "Skin.h"

#include "utils/half.hpp"
#include "utils/KDMatcher.h"

void NiGeometryData::Init(NiHeader* hdr) {
	NiObject::Init(hdr);

	groupID = 0;
	numVertices = 0;
	keepFlags = 0;
	compressFlags = 0;
	hasVertices = true;
	numUVSets = 0;
	//extraVectorsFlags = 0;
	materialCRC = 0;
	hasNormals = false;
	hasVertexColors = false;
	consistencyFlags = 0;
	additionalData = 0xFFFFFFFF;
}

void NiGeometryData::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&groupID, 4);
	file.read((char*)&numVertices, 2);
	file.read((char*)&keepFlags, 1);
	file.read((char*)&compressFlags, 1);
	file.read((char*)&hasVertices, 1);

	if (hasVertices && !isPSys) {
		vertices.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&vertices[i], 12);
	}

	file.read((char*)&numUVSets, 2);

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (header->GetUserVersion2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (header->GetUserVersion() == 12)
		file.read((char*)&materialCRC, 4);

	file.read((char*)&hasNormals, 1);
	if (hasNormals && !isPSys) {
		normals.resize(numVertices);

		for (int i = 0; i < numVertices; i++)
			file.read((char*)&normals[i], 12);

		if (nbtMethod) {
			tangents.resize(numVertices);
			bitangents.resize(numVertices);

			for (int i = 0; i < numVertices; i++)
				file.read((char*)&tangents[i], 12);

			for (int i = 0; i < numVertices; i++)
				file.read((char*)&bitangents[i], 12);
		}
	}

	file.read((char*)&bounds, 16);

	file.read((char*)&hasVertexColors, 1);
	if (hasVertexColors && !isPSys) {
		vertexColors.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&vertexColors[i], 16);
	}

	if (numTextureSets > 0 && !isPSys) {
		uvSets.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			file.read((char*)&uvSets[i], 8);
	}

	file.read((char*)&consistencyFlags, 2);
	file.read((char*)&additionalData, 4);
}

void NiGeometryData::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&groupID, 4);
	file.write((char*)&numVertices, 2);
	file.write((char*)&keepFlags, 1);
	file.write((char*)&compressFlags, 1);

	file.write((char*)&hasVertices, 1);

	if (hasVertices && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&vertices[i], 12);
	}

	file.write((char*)&numUVSets, 2);

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (header->GetUserVersion2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (header->GetUserVersion() == 12)
		file.write((char*)&materialCRC, 4);

	file.write((char*)&hasNormals, 1);
	if (hasNormals && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&normals[i], 12);

		if (nbtMethod) {
			for (int i = 0; i < numVertices; i++)
				file.write((char*)&tangents[i], 12);

			for (int i = 0; i < numVertices; i++)
				file.write((char*)&bitangents[i], 12);
		}
	}

	file.write((char*)&bounds, 16);

	file.write((char*)&hasVertexColors, 1);
	if (hasVertexColors && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&vertexColors[i], 16);
	}

	if (numTextureSets > 0 && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			file.write((char*)&uvSets[i], 8);
	}

	file.write((char*)&consistencyFlags, 2);
	file.write((char*)&additionalData, 4);
}

void NiGeometryData::SetVertices(const bool enable) {
	hasVertices = enable;
	if (enable) {
		vertices.resize(numVertices);
	}
	else {
		vertices.clear();
		numVertices = 0;

		SetNormals(false);
		SetVertexColors(false);
		SetUVs(false);
		SetTangents(false);
	}
}

void NiGeometryData::SetNormals(const bool enable) {
	hasNormals = enable;
	if (enable)
		normals.resize(numVertices);
	else
		normals.clear();
}

void NiGeometryData::SetVertexColors(const bool enable) {
	hasVertexColors = enable;
	if (enable)
		vertexColors.resize(numVertices);
	else
		vertexColors.clear();
}

void NiGeometryData::SetUVs(const bool enable) {
	if (enable) {
		numUVSets |= 1 << 0;
		uvSets.resize(numVertices);
	}
	else {
		numUVSets &= ~(1 << 0);
		uvSets.clear();
	}
}

void NiGeometryData::SetTangents(const bool enable) {
	if (enable) {
		numUVSets |= 1 << 12;
		tangents.resize(numVertices);
		bitangents.resize(numVertices);
	}
	else {
		numUVSets &= ~(1 << 12);
		tangents.clear();
		bitangents.clear();
	}
}

void NiGeometryData::UpdateBounds() {
	bounds = BoundingSphere(vertices);
}

void NiGeometryData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords) {
	groupID = 0;
	keepFlags = 0;
	compressFlags = 0;

	hasVertices = true;
	numVertices = verts->size();
	for (auto &v : (*verts))
		vertices.push_back(v);

	if (texcoords->size() > 0)
		numUVSets = 4097;
	else
		numUVSets = 0;

	materialCRC = 0;
	hasNormals = false;
	hasVertexColors = false;

	bounds = BoundingSphere(*verts);

	for (auto &uv : *texcoords)
		uvSets.push_back(uv);

	additionalData = 0xFFFFFFFF;
}

void NiGeometryData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse(vertices.size(), 0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = bitangents.size() > 0;
	bool hasCol = vertexColors.size() > 0;
	bool hasUV = uvSets.size() > 0;

	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			j++;
		}
	}

	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numVertices--;
			if (hasNorm)
				normals.erase(normals.begin() + i);
			if (hasTan)
				tangents.erase(tangents.begin() + i);
			if (hasBin)
				bitangents.erase(bitangents.begin() + i);
			if (hasCol)
				vertexColors.erase(vertexColors.begin() + i);
			if (hasUV)
				uvSets.erase(uvSets.begin() + i);
		}
	}
}

void NiGeometryData::RecalcNormals(const bool smooth, const float smoothThresh) {
	SetNormals(true);
}

void NiGeometryData::CalcTangentSpace() {
	SetTangents(true);
}

int NiGeometryData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 35;
	if (header->GetUserVersion() == 12)
		blockSize += 4;

	if (hasVertices && !isPSys) {
		ushort nbtMethod = numUVSets & 0xF000;
		byte numTextureSets = numUVSets & 0x3F;
		if (header->GetUserVersion2() >= 83)
			numTextureSets = numUVSets & 0x1;

		blockSize += numVertices * 12;

		if (hasNormals) {
			blockSize += normals.size() * 12;

			if (nbtMethod) {
				blockSize += tangents.size() * 12;
				blockSize += bitangents.size() * 12;
			}
		}

		if (hasVertexColors)
			blockSize += vertexColors.size() * 16;

		if (numTextureSets > 0)
			blockSize += uvSets.size() * 8;
	}

	return blockSize;
}


int NiShape::GetSkinInstanceRef() { return 0xFFFFFFFF; }
void NiShape::SetSkinInstanceRef(int skinInstanceRef) { }

int NiShape::GetShaderPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetShaderPropertyRef(int shaderPropertyRef) { }

int NiShape::GetAlphaPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetAlphaPropertyRef(int alphaPropertyRef) { }

int NiShape::GetDataRef() { return 0xFFFFFFFF; }
void NiShape::SetDataRef(int dataRef) { }

void NiShape::SetVertices(const bool enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetVertices(enable);
};

bool NiShape::HasVertices() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertices();

	return false;
};

void NiShape::SetUVs(const bool enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetUVs(enable);
};

bool NiShape::HasUVs() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasUVs();

	return false;
};

void NiShape::SetNormals(const bool enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetNormals(enable);
};

bool NiShape::HasNormals() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasNormals();

	return false;
};

void NiShape::SetTangents(const bool enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetTangents(enable);
};

bool NiShape::HasTangents() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasTangents();

	return false;
};

void NiShape::SetVertexColors(const bool enable) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetVertexColors(enable);
};

bool NiShape::HasVertexColors() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertexColors();

	return false;
};

void NiShape::SetSkinned(const bool enable) { };
bool NiShape::IsSkinned() { return false; };

void NiShape::SetBounds(const BoundingSphere& bounds) {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->SetBounds(bounds);
}

BoundingSphere NiShape::GetBounds() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		return geomData->GetBounds();

	return BoundingSphere();
}

void NiShape::UpdateBounds() {
	NiGeometryData* geomData = GetGeomData();
	if (geomData)
		geomData->UpdateBounds();
}

int NiShape::GetBoneID(const std::string& boneName) {
	auto boneCont = header->GetBlock<NiBoneContainer>(GetSkinInstanceRef());
	if (boneCont) {
		for (int i = 0; i < boneCont->boneRefs.GetSize(); i++) {
			auto node = header->GetBlock<NiNode>(boneCont->boneRefs.GetBlockRef(i));
			if (node && node->GetName() == boneName)
				return i;
		}
	}

	return 0xFFFFFFFF;
}


BSTriShape::BSTriShape(NiHeader* hdr) {
	NiAVObject::Init(hdr);

	vertFlags3 = 0x43;
	vertFlags4 = 0x50;
	vertFlags5 = 0x0;
	vertFlags6 = 0xB0;
	vertFlags7 = 0x1;
	vertFlags8 = 0x0;

	numTriangles = 0;
	numVertices = 0;
	dataSize = 0;
	vertexSize = 0;
	particleDataSize = 0;
}

BSTriShape::BSTriShape(std::fstream& file, NiHeader* hdr) : BSTriShape(hdr) {
	Get(file);
}

void BSTriShape::Get(std::fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Get(file);

	file.read((char*)&flags, 4);

	file.read((char*)&translation.x, 4);
	file.read((char*)&translation.y, 4);
	file.read((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.read((char*)&rotation[i].x, 4);
		file.read((char*)&rotation[i].y, 4);
		file.read((char*)&rotation[i].z, 4);
	}

	file.read((char*)&scale, 4);
	collisionRef.Get(file);

	file.read((char*)&bounds, 16);

	skinInstanceRef.Get(file);
	shaderPropertyRef.Get(file);
	alphaPropertyRef.Get(file);

	file.read((char*)&vertFlags1, 1);
	file.read((char*)&vertFlags2, 1);
	file.read((char*)&vertFlags3, 1);
	file.read((char*)&vertFlags4, 1);
	file.read((char*)&vertFlags5, 1);
	file.read((char*)&vertFlags6, 1);
	file.read((char*)&vertFlags7, 1);
	file.read((char*)&vertFlags8, 1);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130) {
		ushort num = 0;
		file.read((char*)&num, 2);
		numTriangles = num;
	}
	else
		file.read((char*)&numTriangles, 4);

	file.read((char*)&numVertices, 2);
	file.read((char*)&dataSize, 4);

	vertData.resize(numVertices);

	if (dataSize > 0) {
		half_float::half halfData;
		for (int i = 0; i < numVertices; i++) {
			if (HasVertices()) {
				if (IsFullPrecision()) {
					// Full precision
					file.read((char*)&vertData[i].vert.x, 4);
					file.read((char*)&vertData[i].vert.y, 4);
					file.read((char*)&vertData[i].vert.z, 4);

					file.read((char*)&vertData[i].bitangentX, 4);
				}
				else {
					// Half precision
					file.read((char*)&halfData, 2);
					vertData[i].vert.x = halfData;
					file.read((char*)&halfData, 2);
					vertData[i].vert.y = halfData;
					file.read((char*)&halfData, 2);
					vertData[i].vert.z = halfData;

					file.read((char*)&halfData, 2);
					vertData[i].bitangentX = halfData;
				}
			}

			if (HasUVs()) {
				file.read((char*)&halfData, 2);
				vertData[i].uv.u = halfData;
				file.read((char*)&halfData, 2);
				vertData[i].uv.v = halfData;
			}

			if (HasNormals()) {
				for (int j = 0; j < 3; j++)
					file.read((char*)&vertData[i].normal[j], 1);

				file.read((char*)&vertData[i].bitangentY, 1);

				if (HasTangents()) {
					for (int j = 0; j < 3; j++)
						file.read((char*)&vertData[i].tangent[j], 1);

					file.read((char*)&vertData[i].bitangentZ, 1);
				}
			}


			if (HasVertexColors())
				file.read((char*)&vertData[i].colorData, 4);

			if (IsSkinned()) {
				for (int j = 0; j < 4; j++) {
					file.read((char*)&halfData, 2);
					vertData[i].weights[j] = halfData;
				}

				for (int j = 0; j < 4; j++)
					file.read((char*)&vertData[i].weightBones[j], 1);
			}

			if ((vertFlags7 & (1 << 4)) != 0)
				file.read((char*)&vertData[i].eyeData, 4);
		}
	}

	triangles.resize(numTriangles);

	if (dataSize > 0) {
		for (int i = 0; i < numTriangles; i++) {
			file.read((char*)&triangles[i].p1, 2);
			file.read((char*)&triangles[i].p2, 2);
			file.read((char*)&triangles[i].p3, 2);
		}
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		file.read((char*)&particleDataSize, 4);

		if (particleDataSize > 0) {
			particleVerts.resize(numVertices);
			particleNorms.resize(numVertices);
			particleTris.resize(numTriangles);

			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				file.read((char*)&halfData, 2);
				particleVerts[i].x = halfData;
				file.read((char*)&halfData, 2);
				particleVerts[i].y = halfData;
				file.read((char*)&halfData, 2);
				particleVerts[i].z = halfData;
			}

			for (int i = 0; i < numVertices; i++) {
				file.read((char*)&halfData, 2);
				particleNorms[i].x = halfData;
				file.read((char*)&halfData, 2);
				particleNorms[i].y = halfData;
				file.read((char*)&halfData, 2);
				particleNorms[i].z = halfData;
			}

			for (int i = 0; i < numTriangles; i++) {
				file.read((char*)&particleTris[i].p1, 2);
				file.read((char*)&particleTris[i].p2, 2);
				file.read((char*)&particleTris[i].p3, 2);
			}
		}
	}
}

void BSTriShape::Put(std::fstream& file) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Put(file);

	file.write((char*)&flags, 4);

	file.write((char*)&translation.x, 4);
	file.write((char*)&translation.y, 4);
	file.write((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.write((char*)&rotation[i].x, 4);
		file.write((char*)&rotation[i].y, 4);
		file.write((char*)&rotation[i].z, 4);
	}

	file.write((char*)&scale, 4);
	collisionRef.Put(file);

	file.write((char*)&bounds, 16);

	skinInstanceRef.Put(file);
	shaderPropertyRef.Put(file);
	alphaPropertyRef.Put(file);

	file.write((char*)&vertFlags1, 1);
	file.write((char*)&vertFlags2, 1);
	file.write((char*)&vertFlags3, 1);
	file.write((char*)&vertFlags4, 1);
	file.write((char*)&vertFlags5, 1);
	file.write((char*)&vertFlags6, 1);
	file.write((char*)&vertFlags7, 1);
	file.write((char*)&vertFlags8, 1);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130 && IsSkinned()) {
		// Triangle and vertex data is in partition instead
		ushort numUShort = 0;
		uint numUInt = 0;
		file.write((char*)&numUShort, 2);

		if (HasType<BSDynamicTriShape>())
			file.write((char*)&numVertices, 2);
		else
			file.write((char*)&numUShort, 2);

		file.write((char*)&numUInt, 4);
	}
	else {
		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130) {
			ushort numUShort = numTriangles;
			file.write((char*)&numUShort, 2);
		}
		else
			file.write((char*)&numTriangles, 4);

		file.write((char*)&numVertices, 2);
		file.write((char*)&dataSize, 4);

		if (dataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				if (HasVertices()) {
					if (IsFullPrecision()) {
						// Full precision
						file.write((char*)&vertData[i].vert.x, 4);
						file.write((char*)&vertData[i].vert.y, 4);
						file.write((char*)&vertData[i].vert.z, 4);

						file.write((char*)&vertData[i].bitangentX, 4);
					}
					else {
						// Half precision
						halfData = vertData[i].vert.x;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.y;
						file.write((char*)&halfData, 2);
						halfData = vertData[i].vert.z;
						file.write((char*)&halfData, 2);

						halfData = vertData[i].bitangentX;
						file.write((char*)&halfData, 2);
					}
				}

				if (HasUVs()) {
					halfData = vertData[i].uv.u;
					file.write((char*)&halfData, 2);

					halfData = vertData[i].uv.v;
					file.write((char*)&halfData, 2);
				}

				if (HasNormals()) {
					for (int j = 0; j < 3; j++)
						file.write((char*)&vertData[i].normal[j], 1);

					file.write((char*)&vertData[i].bitangentY, 1);

					if (HasTangents()) {
						for (int j = 0; j < 3; j++)
							file.write((char*)&vertData[i].tangent[j], 1);

						file.write((char*)&vertData[i].bitangentZ, 1);
					}
				}

				if (HasVertexColors())
					file.write((char*)&vertData[i].colorData, 4);

				if (IsSkinned()) {
					for (int j = 0; j < 4; j++) {
						halfData = vertData[i].weights[j];
						file.write((char*)&halfData, 2);
					}

					for (int j = 0; j < 4; j++)
						file.write((char*)&vertData[i].weightBones[j], 1);
				}

				if ((vertFlags7 & (1 << 4)) != 0)
					file.write((char*)&vertData[i].eyeData, 4);
			}
		}

		if (dataSize > 0) {
			for (int i = 0; i < numTriangles; i++) {
				file.write((char*)&triangles[i].p1, 2);
				file.write((char*)&triangles[i].p2, 2);
				file.write((char*)&triangles[i].p3, 2);
			}
		}
	}

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		file.write((char*)&particleDataSize, 4);

		if (particleDataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				halfData = particleVerts[i].x;
				file.write((char*)&halfData, 2);
				halfData = particleVerts[i].y;
				file.write((char*)&halfData, 2);
				halfData = particleVerts[i].z;
				file.write((char*)&halfData, 2);
			}

			for (int i = 0; i < numVertices; i++) {
				halfData = particleNorms[i].x;
				file.write((char*)&halfData, 2);
				halfData = particleNorms[i].y;
				file.write((char*)&halfData, 2);
				halfData = particleNorms[i].z;
				file.write((char*)&halfData, 2);
			}

			for (int i = 0; i < numTriangles; i++) {
				file.write((char*)&particleTris[i].p1, 2);
				file.write((char*)&particleTris[i].p2, 2);
				file.write((char*)&particleTris[i].p3, 2);
			}
		}
	}
}

void BSTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse(vertData.size(), 0);
	std::vector<int> indexCollapseTris(vertData.size(), 0);

	deletedTris.clear();

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			indexCollapseTris[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapseTris[i] = remCount;
	}

	for (int i = vertData.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertData.erase(vertData.begin() + i);
			numVertices--;
		}
	}

	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapseTris[triangles[i].p1] == -1 || indexCollapseTris[triangles[i].p2] == -1 || indexCollapseTris[triangles[i].p3] == -1) {
			deletedTris.push_back(i);
			triangles.erase(triangles.begin() + i);
			numTriangles--;
		}
		else {
			triangles[i].p1 = triangles[i].p1 - indexCollapseTris[triangles[i].p1];
			triangles[i].p2 = triangles[i].p2 - indexCollapseTris[triangles[i].p2];
			triangles[i].p3 = triangles[i].p3 - indexCollapseTris[triangles[i].p3];
		}
	}
}

void BSTriShape::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&skinInstanceRef.index);
	refs.insert(&shaderPropertyRef.index);
	refs.insert(&alphaPropertyRef.index);
}

int BSTriShape::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 42;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130)
		blockSize += 2;
	else
		blockSize += 4;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() < 130 && IsSkinned())
		CalcDataSizes();
	else
		blockSize += CalcDataSizes();

	if (header->GetUserVersion() == 12 && header->GetUserVersion2() == 100) {
		blockSize += 4;

		if (particleDataSize > 0) {
			blockSize += 12 * numVertices;
			blockSize += 6 * numTriangles;
		}
	}

	return blockSize;
}

const std::vector<Vector3>* BSTriShape::GetRawVerts() {
	rawVertices.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawVertices[i] = vertData[i].vert;

	return &rawVertices;
}

const std::vector<Vector3>* BSTriShape::GetNormalData(bool xform) {
	if (!HasNormals())
		return nullptr;

	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float q1 = (((float)vertData[i].normal[0]) / 255.0f) * 2.0f - 1.0f;
		float q2 = (((float)vertData[i].normal[1]) / 255.0f) * 2.0f - 1.0f;
		float q3 = (((float)vertData[i].normal[2]) / 255.0f) * 2.0f - 1.0f;

		float x = q1;
		float y = q2;
		float z = q3;

		if (xform) {
			rawNormals[i].x = -x;
			rawNormals[i].z = y;
			rawNormals[i].y = z;
		}
		else {
			rawNormals[i].x = x;
			rawNormals[i].z = z;
			rawNormals[i].y = y;
		}
	}

	return &rawNormals;
}

const std::vector<Vector3>* BSTriShape::GetTangentData(bool xform) {
	if (!HasTangents())
		return nullptr;

	rawTangents.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float q6 = (((float)vertData[i].tangent[0]) / 255.0f) * 2.0f - 1.0f;
		float q7 = (((float)vertData[i].tangent[1]) / 255.0f) * 2.0f - 1.0f;
		float q8 = (((float)vertData[i].tangent[2]) / 255.0f) * 2.0f - 1.0f;
		float x = q6;
		float y = q7;
		float z = q8;

		if (xform) {
			rawTangents[i].x = -x;
			rawTangents[i].z = y;
			rawTangents[i].y = z;
		}
		else {
			rawTangents[i].x = x;
			rawTangents[i].z = z;
			rawTangents[i].y = y;
		}
	}

	return &rawTangents;
}

const std::vector<Vector3>* BSTriShape::GetBitangentData(bool xform) {
	if (!HasTangents())
		return nullptr;

	rawBitangents.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		float x = (vertData[i].bitangentX);
		float y = (((float)vertData[i].bitangentY) / 255.0f) * 2.0f - 1.0f;
		float z = (((float)vertData[i].bitangentZ) / 255.0f) * 2.0f - 1.0f;


		if (xform) {
			rawBitangents[i].x = -x;
			rawBitangents[i].z = y;
			rawBitangents[i].y = z;
		}
		else {
			rawBitangents[i].x = x;
			rawBitangents[i].z = z;
			rawBitangents[i].y = y;
		}
	}
	return &rawBitangents;
}

const std::vector<Vector2>* BSTriShape::GetUVData() {
	if (!HasUVs())
		return nullptr;

	rawUvs.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawUvs[i] = vertData[i].uv;

	return &rawUvs;
}

void BSTriShape::SetVertices(const bool enable) {
	if (enable) {
		vertFlags6 |= 1 << 4;
		vertData.resize(numVertices);
	}
	else {
		vertFlags6 &= ~(1 << 4);
		vertData.clear();
		numVertices = 0;

		SetUVs(false);
		SetNormals(false);
		SetTangents(false);
		SetVertexColors(false);
		SetSkinned(false);
	}
}

void BSTriShape::SetUVs(const bool enable) {
	if (enable)
		vertFlags6 |= 1 << 5;
	else
		vertFlags6 &= ~(1 << 5);
}

void BSTriShape::SetNormals(const bool enable) {
	if (enable)
		vertFlags6 |= 1 << 7;
	else
		vertFlags6 &= ~(1 << 7);
}

void BSTriShape::SetTangents(const bool enable) {
	if (enable)
		vertFlags7 |= 1 << 0;
	else
		vertFlags7 &= ~(1 << 0);
}

void BSTriShape::SetVertexColors(const bool enable) {
	if (enable)
		vertFlags7 |= 1 << 1;
	else
		vertFlags7 &= ~(1 << 1);
}

void BSTriShape::SetSkinned(const bool enable) {
	if (enable)
		vertFlags7 |= 1 << 2;
	else
		vertFlags7 &= ~(1 << 2);
}

void BSTriShape::SetFullPrecision(const bool enable) {
	if (!CanChangePrecision())
		return;

	if (enable)
		vertFlags7 |= 1 << 6;
	else
		vertFlags7 &= ~(1 << 6);
}

void BSTriShape::UpdateBounds() {
	const std::vector<Vector3>* vertices = GetRawVerts();
	if (vertices)
		bounds = BoundingSphere(*vertices);
	else
		bounds = BoundingSphere();
}

void BSTriShape::SetNormals(const std::vector<Vector3>& inNorms) {
	SetNormals(true);

	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawNormals[i] = inNorms[i];
		vertData[i].normal[0] = (unsigned char)round((((inNorms[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[1] = (unsigned char)round((((inNorms[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[2] = (unsigned char)round((((inNorms[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::RecalcNormals(const bool smooth, const float smoothThresh) {
	GetRawVerts();
	SetNormals(true);

	std::vector<Vector3> verts(numVertices);
	std::vector<Vector3> norms(numVertices);
	for (int i = 0; i < numVertices; i++) {
		verts[i].x = rawVertices[i].x * -0.1f;
		verts[i].z = rawVertices[i].y * 0.1f;
		verts[i].y = rawVertices[i].z * 0.1f;
	}

	// Face normals
	Vector3 tn;
	for (int t = 0; t < numTriangles; t++) {
		triangles[t].trinormal(verts, &tn);
		norms[triangles[t].p1] += tn;
		norms[triangles[t].p2] += tn;
		norms[triangles[t].p3] += tn;
	}

	for (auto &n : norms)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(verts.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			std::pair<Vector3*, int>& a = matcher.matches[i].first;
			std::pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = norms[a.second];
			Vector3& bn = norms[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : norms)
			n.Normalize();
	}

	rawNormals.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawNormals[i].x = -norms[i].x;
		rawNormals[i].y = norms[i].z;
		rawNormals[i].z = norms[i].y;
		vertData[i].normal[0] = (unsigned char)round((((rawNormals[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[1] = (unsigned char)round((((rawNormals[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].normal[2] = (unsigned char)round((((rawNormals[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
		return;

	GetNormalData(false);
	SetTangents(true);

	std::vector<Vector3> tan1;
	std::vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	for (int i = 0; i < triangles.size(); i++) {
		int i1 = triangles[i].p1;
		int i2 = triangles[i].p2;
		int i3 = triangles[i].p3;

		Vector3 v1 = vertData[i1].vert;
		Vector3 v2 = vertData[i2].vert;
		Vector3 v3 = vertData[i3].vert;

		Vector2 w1 = vertData[i1].uv;
		Vector2 w2 = vertData[i2].uv;
		Vector2 w3 = vertData[i3].uv;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += tdir;
		tan1[i2] += tdir;
		tan1[i3] += tdir;

		tan2[i1] += sdir;
		tan2[i2] += sdir;
		tan2[i3] += sdir;
	}

	rawBitangents.resize(numVertices);
	rawTangents.resize(numVertices);

	for (int i = 0; i < numVertices; i++) {
		rawTangents[i] = tan1[i];
		rawBitangents[i] = tan2[i];

		if (rawTangents[i].IsZero() || rawBitangents[i].IsZero()) {
			rawTangents[i].x = rawNormals[i].y;
			rawTangents[i].y = rawNormals[i].z;
			rawTangents[i].z = rawNormals[i].x;
			rawBitangents[i] = rawNormals[i].cross(rawTangents[i]);
		}
		else {
			rawTangents[i].Normalize();
			rawTangents[i] = (rawTangents[i] - rawNormals[i] * rawNormals[i].dot(rawTangents[i]));
			rawTangents[i].Normalize();

			rawBitangents[i].Normalize();

			rawBitangents[i] = (rawBitangents[i] - rawNormals[i] * rawNormals[i].dot(rawBitangents[i]));
			rawBitangents[i] = (rawBitangents[i] - rawTangents[i] * rawTangents[i].dot(rawBitangents[i]));

			rawBitangents[i].Normalize();
		}

		vertData[i].tangent[0] = (unsigned char)round((((rawTangents[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[1] = (unsigned char)round((((rawTangents[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[2] = (unsigned char)round((((rawTangents[i].z + 1.0f) / 2.0f) * 255.0f));

		vertData[i].bitangentX = rawBitangents[i].x;
		vertData[i].bitangentY = (unsigned char)round((((rawBitangents[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].bitangentZ = (unsigned char)round((((rawBitangents[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::UpdateFlags() {
	vertFlags3 = 0;
	vertFlags4 = 0;
	vertFlags5 = 0;
	vertFlags8 = 0;

	if (HasType<BSDynamicTriShape>()) {
		if (HasNormals()) {
			vertFlags3 = 1;

			if (HasTangents())
				vertFlags3 = 33;
		}

		if (HasTangents()) {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 67;
			else if (IsSkinned())
				vertFlags4 = 48;
			else if (HasVertexColors())
				vertFlags4 = 3;
		}
		else {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 33;
			else if (IsSkinned())
				vertFlags4 = 16;
			else if (HasVertexColors())
				vertFlags4 = 1;
		}

		if ((vertFlags7 & (1 << 4)) != 0)	// Eye Data
			vertFlags5 = 96;
	}
	else if (IsFullPrecision()) {
		if (HasNormals()) {
			vertFlags3 = 4;

			if (HasTangents())
				vertFlags3 = 101;
		}

		if (HasTangents()) {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 135;
			else if (IsSkinned())
				vertFlags4 = 112;
			else if (HasVertexColors())
				vertFlags4 = 7;
		}
		else {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 101;
			else if (IsSkinned())
				vertFlags4 = 80;
			else if (HasVertexColors())
				vertFlags4 = 5;
		}

		if ((vertFlags7 & (1 << 4)) != 0)	// Eye Data
			vertFlags5 = 144;
	}
	else {
		if (HasNormals()) {
			vertFlags3 = 2;

			if (HasTangents())
				vertFlags3 = 67;
		}

		if (HasTangents()) {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 101;
			else if (IsSkinned())
				vertFlags4 = 80;
			else if (HasVertexColors())
				vertFlags4 = 5;
		}
		else {
			if (IsSkinned() && HasVertexColors())
				vertFlags4 = 67;
			else if (IsSkinned())
				vertFlags4 = 48;
			else if (HasVertexColors())
				vertFlags4 = 3;
		}

		if ((vertFlags7 & (1 << 4)) != 0)	// Eye Data
			vertFlags5 = 144;
	}
}

int BSTriShape::CalcDataSizes() {
	vertexSize = 0;
	dataSize = 0;

	if (HasVertices()) {
		if (IsFullPrecision()) {	// Position + Bitangent X
			vertexSize += 4;
			vertFlags2 = 4;
		}
		else {
			vertexSize += 2;
			vertFlags2 = 2;
		}
	}
	else
		vertFlags2 = 0;

	if (HasUVs())
		vertexSize += 1;		// UVs

	if (HasNormals()) {			// Normals + Bitangent Y
		vertexSize += 1;

		if (HasTangents())		// Tangents + Bitangent Z
			vertexSize += 1;
	}

	if (HasVertexColors())		// Vertex Colors
		vertexSize += 1;

	if (IsSkinned())			// Skinning
		vertexSize += 3;

	if ((vertFlags7 & (1 << 4)) != 0)	// Eye Data
		vertexSize += 1;

	vertFlags1 = vertexSize;

	if (HasType<BSDynamicTriShape>())
		vertFlags1 = (vertFlags1 & 0xF) | 0x40;

	vertexSize *= 4;
	dataSize = vertexSize * numVertices + 6 * numTriangles;

	UpdateFlags();

	return dataSize;
}

void BSTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	flags = 14;
	numVertices = verts->size();
	numTriangles = tris->size();

	vertData.resize(numVertices);

	if (uvs && uvs->size() != numVertices)
		SetUVs(false);

	for (int i = 0; i < numVertices; i++) {
		vertData[i].vert = (*verts)[i];

		if (uvs && uvs->size() == numVertices)
			vertData[i].uv = (*uvs)[i];

		vertData[i].bitangentX = 0.0f;
		vertData[i].bitangentY = 0;
		vertData[i].bitangentZ = 0;
		vertData[i].normal[0] = vertData[i].normal[1] = vertData[i].normal[2] = 0;
		memset(vertData[i].colorData, 255, 4);
		memset(vertData[i].weights, 0, sizeof(float) * 4);
		memset(vertData[i].weightBones, 0, 4);
		vertData[i].eyeData = 0.0f;
	}

	triangles.resize(numTriangles);
	for (int i = 0; i < numTriangles; i++)
		triangles[i] = (*tris)[i];

	bounds = BoundingSphere(*verts);

	if (normals && normals->size() == numVertices) {
		SetNormals(*normals);
		CalcTangentSpace();
	}
	else {
		SetNormals(false);
		SetTangents(false);
	}
}


BSSubIndexTriShape::BSSubIndexTriShape(NiHeader* hdr) : BSTriShape(hdr) {
}

BSSubIndexTriShape::BSSubIndexTriShape(std::fstream& file, NiHeader* hdr) : BSSubIndexTriShape(hdr) {
	Get(file);
}

void BSSubIndexTriShape::Get(std::fstream& file) {
	BSTriShape::Get(file);

	if (dataSize <= 0)
		return;

	file.read((char*)&segmentation.numPrimitives, 4);
	file.read((char*)&segmentation.numSegments, 4);
	file.read((char*)&segmentation.numTotalSegments, 4);

	segmentation.segments.resize(segmentation.numSegments);
	for (auto &segment : segmentation.segments) {
		file.read((char*)&segment.startIndex, 4);
		file.read((char*)&segment.numPrimitives, 4);
		file.read((char*)&segment.parentArrayIndex, 4);
		file.read((char*)&segment.numSubSegments, 4);

		segment.subSegments.resize(segment.numSubSegments);
		for (auto &subSegment : segment.subSegments) {
			file.read((char*)&subSegment.startIndex, 4);
			file.read((char*)&subSegment.numPrimitives, 4);
			file.read((char*)&subSegment.arrayIndex, 4);
			file.read((char*)&subSegment.unkInt1, 4);
		}
	}

	if (segmentation.numSegments < segmentation.numTotalSegments) {
		file.read((char*)&segmentation.subSegmentData.numSegments, 4);
		file.read((char*)&segmentation.subSegmentData.numTotalSegments, 4);

		segmentation.subSegmentData.arrayIndices.resize(segmentation.numSegments);
		for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
			file.read((char*)&arrayIndex, 4);

		segmentation.subSegmentData.dataRecords.resize(segmentation.numTotalSegments);
		for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
			file.read((char*)&dataRecord.segmentUser, 4);
			file.read((char*)&dataRecord.unkInt2, 4);
			file.read((char*)&dataRecord.numData, 4);

			dataRecord.extraData.resize(dataRecord.numData);
			for (auto &data : dataRecord.extraData)
				file.read((char*)&data, 4);
		}

		segmentation.subSegmentData.ssfFile.Get(file, 2);
	}
}

void BSSubIndexTriShape::Put(std::fstream& file) {
	BSTriShape::Put(file);

	if (dataSize <= 0)
		return;

	file.write((char*)&segmentation.numPrimitives, 4);
	file.write((char*)&segmentation.numSegments, 4);
	file.write((char*)&segmentation.numTotalSegments, 4);

	for (auto &segment : segmentation.segments) {
		file.write((char*)&segment.startIndex, 4);
		file.write((char*)&segment.numPrimitives, 4);
		file.write((char*)&segment.parentArrayIndex, 4);
		file.write((char*)&segment.numSubSegments, 4);

		for (auto &subSegment : segment.subSegments) {
			file.write((char*)&subSegment.startIndex, 4);
			file.write((char*)&subSegment.numPrimitives, 4);
			file.write((char*)&subSegment.arrayIndex, 4);
			file.write((char*)&subSegment.unkInt1, 4);
		}
	}

	if (segmentation.numSegments < segmentation.numTotalSegments) {
		file.write((char*)&segmentation.subSegmentData.numSegments, 4);
		file.write((char*)&segmentation.subSegmentData.numTotalSegments, 4);

		for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
			file.write((char*)&arrayIndex, 4);

		for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
			file.write((char*)&dataRecord.segmentUser, 4);
			file.write((char*)&dataRecord.unkInt2, 4);
			file.write((char*)&dataRecord.numData, 4);

			for (auto &data : dataRecord.extraData)
				file.write((char*)&data, 4);
		}

		segmentation.subSegmentData.ssfFile.Put(file, 2, false);
	}
}

void BSSubIndexTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	//Remove triangles from segments and re-fit lists
	segmentation.numPrimitives -= deletedTris.size();
	for (auto &segment : segmentation.segments) {
		// Delete primitives
		for (auto &id : deletedTris)
			if (segment.numPrimitives > 0 && id >= segment.startIndex / 3 && id < segment.startIndex / 3 + segment.numPrimitives)
				segment.numPrimitives--;

		// Align sub segments
		for (auto &subSegment : segment.subSegments)
			for (auto &id : deletedTris)
				if (subSegment.numPrimitives > 0 && id >= subSegment.startIndex / 3 && id < subSegment.startIndex / 3 + subSegment.numPrimitives)
					subSegment.numPrimitives--;
	}

	// Align segments
	int i = 0;
	for (auto &segment : segmentation.segments) {
		// Align sub segments
		int j = 0;
		for (auto &subSegment : segment.subSegments) {
			if (j == 0)
				subSegment.startIndex = segment.startIndex;

			if (j + 1 >= segment.numSubSegments)
				continue;

			BSSITSSubSegment& nextSubSegment = segment.subSegments[j + 1];
			nextSubSegment.startIndex = subSegment.startIndex + subSegment.numPrimitives * 3;
			j++;
		}

		if (i + 1 >= segmentation.numSegments)
			continue;

		BSSITSSegment& nextSegment = segmentation.segments[i + 1];
		nextSegment.startIndex = segment.startIndex + segment.numPrimitives * 3;

		i++;
	}
}

int BSSubIndexTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	if (dataSize > 0) {
		blockSize += 12;

		blockSize += segmentation.numSegments * 16;
		for (auto &segment : segmentation.segments)
			blockSize += segment.numSubSegments * 16;

		if (segmentation.numSegments < segmentation.numTotalSegments) {
			blockSize += 10;
			blockSize += segmentation.subSegmentData.numSegments * 4;
			blockSize += segmentation.subSegmentData.numTotalSegments * 12;

			for (auto &dataRecord : segmentation.subSegmentData.dataRecords)
				blockSize += dataRecord.numData * 4;

			blockSize += segmentation.subSegmentData.ssfFile.GetLength();
		}
	}

	return blockSize;
}

void BSSubIndexTriShape::SetDefaultSegments() {
	segmentation.numPrimitives = numTriangles;
	segmentation.numSegments = 4;
	segmentation.numTotalSegments = 4;

	segmentation.subSegmentData.numSegments = 0;
	segmentation.subSegmentData.numTotalSegments = 0;

	segmentation.subSegmentData.arrayIndices.clear();
	segmentation.subSegmentData.dataRecords.clear();
	segmentation.subSegmentData.ssfFile.Clear();

	segmentation.segments.resize(4);
	for (int i = 0; i < 3; i++) {
		segmentation.segments[i].startIndex = 0;
		segmentation.segments[i].numPrimitives = 0;
		segmentation.segments[i].parentArrayIndex = 0xFFFFFFFF;
		segmentation.segments[i].numSubSegments = 0;
	}

	segmentation.segments[3].startIndex = 0;
	segmentation.segments[3].numPrimitives = numTriangles;
	segmentation.segments[3].parentArrayIndex = 0xFFFFFFFF;
	segmentation.segments[3].numSubSegments = 0;
}

void BSSubIndexTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	// Skinned most of the time
	SetSkinned(true);
	SetDefaultSegments();
}


BSMeshLODTriShape::BSMeshLODTriShape(NiHeader* hdr) : BSTriShape(hdr) {
	lodSize0 = 0;
	lodSize1 = 0;
	lodSize2 = 0;
}

BSMeshLODTriShape::BSMeshLODTriShape(std::fstream& file, NiHeader* hdr) : BSMeshLODTriShape(hdr) {
	Get(file);
}

void BSMeshLODTriShape::Get(std::fstream& file) {
	BSTriShape::Get(file);

	file.read((char*)&lodSize0, 4);
	file.read((char*)&lodSize1, 4);
	file.read((char*)&lodSize2, 4);
}

void BSMeshLODTriShape::Put(std::fstream& file) {
	BSTriShape::Put(file);

	file.write((char*)&lodSize0, 4);
	file.write((char*)&lodSize1, 4);
	file.write((char*)&lodSize2, 4);
}

void BSMeshLODTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	// Force full LOD (workaround)
	lodSize0 = 0;
	lodSize1 = 0;
	lodSize2 = numTriangles;
}

int BSMeshLODTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}


BSDynamicTriShape::BSDynamicTriShape(NiHeader* hdr) : BSTriShape(hdr) {
	vertFlags6 &= ~(1 << 4);
	vertFlags7 |= 1 << 6;

	dynamicDataSize = 0;
}

BSDynamicTriShape::BSDynamicTriShape(std::fstream& file, NiHeader* hdr) : BSDynamicTriShape(hdr) {
	Get(file);
}

void BSDynamicTriShape::Get(std::fstream& file) {
	BSTriShape::Get(file);

	file.read((char*)&dynamicDataSize, 4);

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		file.read((char*)&dynamicData[i].x, 4);
		file.read((char*)&dynamicData[i].y, 4);
		file.read((char*)&dynamicData[i].z, 4);
		file.read((char*)&dynamicData[i].w, 4);
	}
}

void BSDynamicTriShape::Put(std::fstream& file) {
	BSTriShape::Put(file);

	file.write((char*)&dynamicDataSize, 4);

	for (int i = 0; i < numVertices; i++) {
		file.write((char*)&dynamicData[i].x, 4);
		file.write((char*)&dynamicData[i].y, 4);
		file.write((char*)&dynamicData[i].z, 4);
		file.write((char*)&dynamicData[i].w, 4);
	}
}

void BSDynamicTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	std::vector<int> indexCollapse(dynamicData.size(), 0);
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {
			indexCollapse[i] = -1;
			j++;
		}
	}

	for (int i = dynamicData.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			dynamicData.erase(dynamicData.begin() + i);
			dynamicDataSize--;
		}
	}
}

int BSDynamicTriShape::CalcBlockSize() {
	BSTriShape::CalcBlockSize();

	dynamicDataSize = numVertices * 16;

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		dynamicData[i].x = vertData[i].vert.x;
		dynamicData[i].y = vertData[i].vert.y;
		dynamicData[i].z = vertData[i].vert.z;
		dynamicData[i].w = vertData[i].bitangentX;

		if (dynamicData[i].x > 0.0f)
			vertData[i].eyeData = 1.0f;
		else
			vertData[i].eyeData = 0.0f;
	}

	blockSize += 4;
	blockSize += dynamicDataSize;

	return blockSize;
}

void BSDynamicTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	dynamicDataSize = verts->size();

	dynamicData.resize(dynamicDataSize);
	for (int i = 0; i < dynamicDataSize; i++) {
		dynamicData[i].x = (*verts)[i].x;
		dynamicData[i].y = (*verts)[i].y;
		dynamicData[i].z = (*verts)[i].z;
		dynamicData[i].w = 0.0f;
	}
}


void NiGeometry::Init(NiHeader* hdr) {
	NiAVObject::Init(hdr);
}

void NiGeometry::Get(std::fstream& file) {
	NiAVObject::Get(file);

	dataRef.Get(file);
	skinInstanceRef.Get(file);

	file.read((char*)&numMaterials, 4);
	materialNameRefs.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		materialNameRefs[i].Get(file, header);

	materials.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		file.read((char*)&materials[i], 4);

	file.read((char*)&activeMaterial, 4);
	file.read((char*)&dirty, 1);

	if (header->GetUserVersion() > 11) {
		shaderPropertyRef.Get(file);
		alphaPropertyRef.Get(file);
	}
}

void NiGeometry::Put(std::fstream& file) {
	NiAVObject::Put(file);

	dataRef.Put(file);
	skinInstanceRef.Put(file);

	file.write((char*)&numMaterials, 4);
	for (int i = 0; i < numMaterials; i++)
		materialNameRefs[i].Put(file);

	for (int i = 0; i < numMaterials; i++)
		file.write((char*)&materials[i], 4);

	file.write((char*)&activeMaterial, 4);
	file.write((char*)&dirty, 1);

	if (header->GetUserVersion() > 11) {
		shaderPropertyRef.Put(file);
		alphaPropertyRef.Put(file);
	}
}

void NiGeometry::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
	refs.insert(&skinInstanceRef.index);
	refs.insert(&shaderPropertyRef.index);
	refs.insert(&alphaPropertyRef.index);
}

int NiGeometry::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 17;
	blockSize += numMaterials * 8;
	if (header->GetUserVersion() > 11)
		blockSize += 8;

	return blockSize;
}


void NiTriBasedGeomData::Init(NiHeader* hdr) {
	NiGeometryData::Init(hdr);

	numTriangles = 0;
}

void NiTriBasedGeomData::Get(std::fstream& file) {
	NiGeometryData::Get(file);

	file.read((char*)&numTriangles, 2);
}

void NiTriBasedGeomData::Put(std::fstream& file) {
	NiGeometryData::Put(file);

	file.write((char*)&numTriangles, 2);
}

void NiTriBasedGeomData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords) {
	NiGeometryData::Create(verts, inTris, texcoords);

	numTriangles = inTris->size();
}

int NiTriBasedGeomData::CalcBlockSize() {
	NiGeometryData::CalcBlockSize();

	blockSize += 2;
	return blockSize;
}


NiTriShape::NiTriShape(NiHeader* hdr) {
	NiTriBasedGeom::Init(hdr);
}

NiTriShape::NiTriShape(std::fstream& file, NiHeader* hdr) : NiTriShape(hdr) {
	Get(file);
}


NiTriShapeData::NiTriShapeData(NiHeader* hdr) {
	NiTriBasedGeomData::Init(hdr);

	numTrianglePoints = 0;
	hasTriangles = false;
	numMatchGroups = 0;
}

NiTriShapeData::NiTriShapeData(std::fstream& file, NiHeader* hdr) : NiTriShapeData(hdr) {
	Get(file);
}

void NiTriShapeData::Get(std::fstream& file) {
	NiTriBasedGeomData::Get(file);

	file.read((char*)&numTrianglePoints, 4);
	file.read((char*)&hasTriangles, 1);
	if (hasTriangles) {
		Triangle triangle;
		for (int i = 0; i < numTriangles; i++) {
			file.read((char*)&triangle.p1, 2);
			file.read((char*)&triangle.p2, 2);
			file.read((char*)&triangle.p3, 2);
			triangles.push_back(triangle);
		}
	}

	ushort uShort;
	MatchGroup mg;
	file.read((char*)&numMatchGroups, 2);
	for (int i = 0; i < numMatchGroups; i++) {
		file.read((char*)&mg.count, 2);
		mg.matches.clear();
		for (int j = 0; j < mg.count; j++) {
			file.read((char*)&uShort, 2);
			mg.matches.push_back(uShort);
		}
		matchGroups.push_back(mg);
	}

	// Not supported yet, so clear it again after reading
	matchGroups.clear();
	numMatchGroups = 0;
}

void NiTriShapeData::Put(std::fstream& file) {
	NiTriBasedGeomData::Put(file);

	file.write((char*)&numTrianglePoints, 4);
	file.write((char*)&hasTriangles, 1);
	if (hasTriangles) {
		for (int i = 0; i < numTriangles; i++) {
			file.write((char*)&triangles[i].p1, 2);
			file.write((char*)&triangles[i].p2, 2);
			file.write((char*)&triangles[i].p3, 2);
		}
	}
	file.write((char*)&numMatchGroups, 2);
	for (int i = 0; i < numMatchGroups; i++) {
		file.write((char*)&matchGroups[i].count, 2);
		for (int j = 0; j < matchGroups[i].count; j++)
			file.write((char*)&matchGroups[i].matches[j], 2);
	}
}

void NiTriShapeData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords) {
	NiTriBasedGeomData::Create(verts, inTris, texcoords);

	numTrianglePoints = numTriangles * 3;
	hasTriangles = true;
	for (auto &t : *inTris)
		triangles.push_back(t);

	numMatchGroups = 0;
}

void NiTriShapeData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse(vertices.size(), 0);
	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);

	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapse[triangles[i].p1] == -1 || indexCollapse[triangles[i].p2] == -1 || indexCollapse[triangles[i].p3] == -1) {
			triangles.erase(triangles.begin() + i);
			numTriangles--;
			numTrianglePoints -= 3;
		}
		else {
			triangles[i].p1 = triangles[i].p1 - indexCollapse[triangles[i].p1];
			triangles[i].p2 = triangles[i].p2 - indexCollapse[triangles[i].p2];
			triangles[i].p3 = triangles[i].p3 - indexCollapse[triangles[i].p3];
		}
	}
}

void NiTriShapeData::RecalcNormals(const bool smooth, const float smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	// Zero out existing normals
	for (auto &n : normals)
		n.Zero();

	// Face normals
	Vector3 tn;
	for (int t = 0; t < numTriangles; t++) {
		triangles[t].trinormal(vertices, &tn);
		normals[triangles[t].p1] += tn;
		normals[triangles[t].p2] += tn;
		normals[triangles[t].p3] += tn;
	}

	for (auto &n : normals)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(vertices.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			std::pair<Vector3*, int>& a = matcher.matches[i].first;
			std::pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = normals[a.second];
			Vector3& bn = normals[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : normals)
			n.Normalize();
	}
}

void NiTriShapeData::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
		return;

	NiTriBasedGeomData::CalcTangentSpace();

	std::vector<Vector3> tan1;
	std::vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	for (int i = 0; i < numTriangles; i++) {
		int i1 = triangles[i].p1;
		int i2 = triangles[i].p2;
		int i3 = triangles[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[i1];
		Vector2 w2 = uvSets[i2];
		Vector2 w3 = uvSets[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		bitangents[i] = tan1[i];
		tangents[i] = tan2[i];

		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = normals[i].y;
			tangents[i].y = normals[i].z;
			tangents[i].z = normals[i].x;
			bitangents[i] = normals[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - normals[i] * normals[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - normals[i] * normals[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
	}
}

int NiTriShapeData::CalcBlockSize() {
	NiTriBasedGeomData::CalcBlockSize();

	blockSize += 7;
	blockSize += triangles.size() * 6;	// Triangles
	for (auto &mg : matchGroups) {
		blockSize += 4;
		blockSize += mg.count * 2;
	}

	return blockSize;
}


NiTriStrips::NiTriStrips(NiHeader* hdr) {
	NiTriBasedGeom::Init(hdr);
}

NiTriStrips::NiTriStrips(std::fstream& file, NiHeader* hdr) : NiTriStrips(hdr) {
	Get(file);
}


NiTriStripsData::NiTriStripsData(NiHeader* hdr) {
	NiTriBasedGeomData::Init(hdr);

	numStrips = 0;
	hasPoints = false;
}

NiTriStripsData::NiTriStripsData(std::fstream& file, NiHeader* hdr) : NiTriStripsData(hdr) {
	Get(file);
}

void NiTriStripsData::Get(std::fstream& file) {
	NiTriBasedGeomData::Get(file);

	ushort uShort;
	file.read((char*)&numStrips, 2);
	for (int i = 0; i < numStrips; i++) {
		file.read((char*)&uShort, 2);
		stripLengths.push_back(uShort);
	}

	file.read((char*)&hasPoints, 1);
	if (hasPoints) {
		for (int i = 0; i < numStrips; i++) {
			points.push_back(std::vector<ushort>());
			for (int j = 0; j < stripLengths[i]; j++) {
				file.read((char*)&uShort, 2);
				points[i].push_back(uShort);
			}
		}
	}
}

void NiTriStripsData::Put(std::fstream& file) {
	NiTriBasedGeomData::Put(file);

	file.write((char*)&numStrips, 2);
	for (int i = 0; i < numStrips; i++)
		file.write((char*)&stripLengths[i], 2);

	file.write((char*)&hasPoints, 1);

	if (hasPoints) {
		for (int i = 0; i < numStrips; i++)
			for (int j = 0; j < stripLengths[i]; j++)
				file.write((char*)&points[i][j], 2);
	}
}

void NiTriStripsData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse(vertices.size(), 0);
	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			remCount++;
			j++;
		}
		else
			indexCollapse[i] = remCount;
	}

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);

	// This is not a healthy way to delete strip data. Probably need to restrip the shape.
	for (int i = 0; i < numStrips; i++) {
		for (int j = 0; j < stripLengths[i]; j++) {
			if (indexCollapse[points[i][j]] == -1) {
				points[i].erase(points[i].begin() + j);
				stripLengths[i]--;
			}
		}
	}
}

void NiTriStripsData::StripsToTris(std::vector<Triangle>* outTris) {
	Triangle triangle;
	for (int strip = 0; strip < numStrips; strip++) {
		for (int vi = 0; vi < stripLengths[strip] - 2; vi++) {
			if (vi & 1) {
				triangle.p1 = points[strip][vi];
				triangle.p2 = points[strip][vi + 2];
				triangle.p3 = points[strip][vi + 1];
			}
			else {
				triangle.p1 = points[strip][vi];
				triangle.p2 = points[strip][vi + 1];
				triangle.p3 = points[strip][vi + 2];
			}

			if (triangle.p1 == triangle.p2 || triangle.p2 == triangle.p3 || triangle.p3 == triangle.p1)
				continue;

			outTris->push_back(triangle);
		}
	}
}

void NiTriStripsData::RecalcNormals(const bool smooth, const float smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	std::vector<Triangle> tris;
	StripsToTris(&tris);

	// Zero out existing normals
	for (auto &n : normals)
		n.Zero();

	// Face normals
	Vector3 tn;
	for (int t = 0; t < tris.size(); t++) {
		tris[t].trinormal(vertices, &tn);
		normals[tris[t].p1] += tn;
		normals[tris[t].p2] += tn;
		normals[tris[t].p3] += tn;
	}

	for (auto &n : normals)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(vertices.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++) {
			std::pair<Vector3*, int>& a = matcher.matches[i].first;
			std::pair<Vector3*, int>& b = matcher.matches[i].second;

			Vector3& an = normals[a.second];
			Vector3& bn = normals[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : normals)
			n.Normalize();
	}
}

void NiTriStripsData::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
		return;

	NiTriBasedGeomData::CalcTangentSpace();

	std::vector<Vector3> tan1;
	std::vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	std::vector<Triangle> tris;
	StripsToTris(&tris);

	for (int i = 0; i < tris.size(); i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[i1];
		Vector2 w2 = uvSets[i2];
		Vector2 w3 = uvSets[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Vector3 sdir = Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int i = 0; i < numVertices; i++) {
		bitangents[i] = tan1[i];
		tangents[i] = tan2[i];

		if (tangents[i].IsZero() || bitangents[i].IsZero()) {
			tangents[i].x = normals[i].y;
			tangents[i].y = normals[i].z;
			tangents[i].z = normals[i].x;
			bitangents[i] = normals[i].cross(tangents[i]);
		}
		else {
			tangents[i].Normalize();
			tangents[i] = (tangents[i] - normals[i] * normals[i].dot(tangents[i]));
			tangents[i].Normalize();

			bitangents[i].Normalize();

			bitangents[i] = (bitangents[i] - normals[i] * normals[i].dot(bitangents[i]));
			bitangents[i] = (bitangents[i] - tangents[i] * tangents[i].dot(bitangents[i]));

			bitangents[i].Normalize();
		}
	}
}

int NiTriStripsData::CalcBlockSize() {
	NiTriBasedGeomData::CalcBlockSize();

	blockSize += 3;
	blockSize += numStrips * 2;				// Strip Lengths

	for (auto &pl : points)
		blockSize += pl.size() * 2;

	return blockSize;
}


BSLODTriShape::BSLODTriShape(NiHeader* hdr) {
	NiTriBasedGeom::Init(hdr);
}

BSLODTriShape::BSLODTriShape(std::fstream& file, NiHeader* hdr) : BSLODTriShape(hdr) {
	Get(file);
}

void BSLODTriShape::Get(std::fstream& file) {
	NiTriBasedGeom::Get(file);

	file.read((char*)&level0, 4);
	file.read((char*)&level1, 4);
	file.read((char*)&level2, 4);
}

void BSLODTriShape::Put(std::fstream& file) {
	NiTriBasedGeom::Put(file);

	file.write((char*)&level0, 4);
	file.write((char*)&level1, 4);
	file.write((char*)&level2, 4);
}

int BSLODTriShape::CalcBlockSize() {
	NiTriBasedGeom::CalcBlockSize();

	blockSize += 12;

	return blockSize;
}
