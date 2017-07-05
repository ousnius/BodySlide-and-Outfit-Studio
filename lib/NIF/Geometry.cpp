/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Geometry.h"
#include "Skin.h"

#include "utils/half.hpp"
#include "utils/KDMatcher.h"

void NiGeometryData::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> groupID;
	stream >> numVertices;
	stream >> keepFlags;
	stream >> compressFlags;
	stream >> hasVertices;

	if (hasVertices && !isPSys) {
		vertices.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			stream >> vertices[i];
	}

	stream >> numUVSets;

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (stream.GetVersion().User2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (stream.GetVersion().User() == 12)
		stream >> materialCRC;

	stream >> hasNormals;
	if (hasNormals && !isPSys) {
		normals.resize(numVertices);

		for (int i = 0; i < numVertices; i++)
			stream >> normals[i];

		if (nbtMethod) {
			tangents.resize(numVertices);
			bitangents.resize(numVertices);

			for (int i = 0; i < numVertices; i++)
				stream >> tangents[i];

			for (int i = 0; i < numVertices; i++)
				stream >> bitangents[i];
		}
	}

	stream >> bounds;

	stream >> hasVertexColors;
	if (hasVertexColors && !isPSys) {
		vertexColors.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			stream >> vertexColors[i];
	}

	if (numTextureSets > 0 && !isPSys) {
		uvSets.resize(numVertices);
		for (int i = 0; i < numVertices; i++)
			stream >> uvSets[i];
	}

	stream >> consistencyFlags;
	stream >> additionalData;
}

void NiGeometryData::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << groupID;
	stream << numVertices;
	stream << keepFlags;
	stream << compressFlags;
	stream << hasVertices;

	if (hasVertices && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			stream << vertices[i];
	}

	stream << numUVSets;

	ushort nbtMethod = numUVSets & 0xF000;
	byte numTextureSets = numUVSets & 0x3F;
	if (stream.GetVersion().User2() >= 83)
		numTextureSets = numUVSets & 0x1;

	if (stream.GetVersion().User() == 12)
		stream << materialCRC;

	stream << hasNormals;
	if (hasNormals && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			stream << normals[i];

		if (nbtMethod) {
			for (int i = 0; i < numVertices; i++)
				stream << tangents[i];

			for (int i = 0; i < numVertices; i++)
				stream << bitangents[i];
		}
	}

	stream << bounds;

	stream << hasVertexColors;
	if (hasVertexColors && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			stream << vertexColors[i];
	}

	if (numTextureSets > 0 && !isPSys) {
		for (int i = 0; i < numVertices; i++)
			stream << uvSets[i];
	}

	stream << consistencyFlags;
	stream << additionalData;
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


int NiShape::GetSkinInstanceRef() { return 0xFFFFFFFF; }
void NiShape::SetSkinInstanceRef(int skinInstanceRef) { }

int NiShape::GetShaderPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetShaderPropertyRef(int shaderPropertyRef) { }

int NiShape::GetAlphaPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetAlphaPropertyRef(int alphaPropertyRef) { }

int NiShape::GetDataRef() { return 0xFFFFFFFF; }
void NiShape::SetDataRef(int dataRef) { }

void NiShape::SetVertices(const bool enable) {
	if (geomData)
		geomData->SetVertices(enable);
};

bool NiShape::HasVertices() {
	if (geomData)
		return geomData->HasVertices();

	return false;
};

void NiShape::SetUVs(const bool enable) {
	if (geomData)
		geomData->SetUVs(enable);
};

bool NiShape::HasUVs() {
	if (geomData)
		return geomData->HasUVs();

	return false;
};

void NiShape::SetNormals(const bool enable) {
	if (geomData)
		geomData->SetNormals(enable);
};

bool NiShape::HasNormals() {
	if (geomData)
		return geomData->HasNormals();

	return false;
};

void NiShape::SetTangents(const bool enable) {
	if (geomData)
		geomData->SetTangents(enable);
};

bool NiShape::HasTangents() {
	if (geomData)
		return geomData->HasTangents();

	return false;
};

void NiShape::SetVertexColors(const bool enable) {
	if (geomData)
		geomData->SetVertexColors(enable);
};

bool NiShape::HasVertexColors() {
	if (geomData)
		return geomData->HasVertexColors();

	return false;
};

void NiShape::SetSkinned(const bool enable) { };
bool NiShape::IsSkinned() { return false; };

void NiShape::SetBounds(const BoundingSphere& bounds) {
	if (geomData)
		geomData->SetBounds(bounds);
}

BoundingSphere NiShape::GetBounds() {
	if (geomData)
		return geomData->GetBounds();

	return BoundingSphere();
}

void NiShape::UpdateBounds() {
	if (geomData)
		geomData->UpdateBounds();
}

int NiShape::GetBoneID(NiHeader& hdr, const std::string& boneName) {
	auto boneCont = hdr.GetBlock<NiBoneContainer>(GetSkinInstanceRef());
	if (boneCont) {
		for (int i = 0; i < boneCont->boneRefs.GetSize(); i++) {
			auto node = hdr.GetBlock<NiNode>(boneCont->boneRefs.GetBlockRef(i));
			if (node && node->GetName() == boneName)
				return i;
		}
	}

	return 0xFFFFFFFF;
}


BSTriShape::BSTriShape(NiStream& stream) : BSTriShape() {
	Get(stream);
}

void BSTriShape::Get(NiStream& stream) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Get(stream);

	stream >> flags;
	stream >> translation;

	for (int i = 0; i < 3; i++)
		stream >> rotation[i];

	stream >> scale;
	collisionRef.Get(stream);

	stream >> bounds;

	skinInstanceRef.Get(stream);
	shaderPropertyRef.Get(stream);
	alphaPropertyRef.Get(stream);

	stream >> vertFlags1;
	stream >> vertFlags2;
	stream >> vertFlags3;
	stream >> vertFlags4;
	stream >> vertFlags5;
	stream >> vertFlags6;
	stream >> vertFlags7;
	stream >> vertFlags8;

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().User2() < 130) {
		ushort num = 0;
		stream >> num;
		numTriangles = num;
	}
	else
		stream >> numTriangles;

	stream >> numVertices;
	stream >> dataSize;

	vertData.resize(numVertices);

	if (dataSize > 0) {
		half_float::half halfData;
		for (int i = 0; i < numVertices; i++) {
			auto& vertex = vertData[i];
			if (HasVertices()) {
				if (IsFullPrecision() || stream.GetVersion().User2() == 100) {
					// Full precision
					stream >> vertex.vert;
					stream >> vertex.bitangentX;
				}
				else {
					// Half precision
					stream.read((char*)&halfData, 2);
					vertex.vert.x = halfData;
					stream.read((char*)&halfData, 2);
					vertex.vert.y = halfData;
					stream.read((char*)&halfData, 2);
					vertex.vert.z = halfData;

					stream.read((char*)&halfData, 2);
					vertex.bitangentX = halfData;
				}
			}

			if (HasUVs()) {
				stream.read((char*)&halfData, 2);
				vertex.uv.u = halfData;
				stream.read((char*)&halfData, 2);
				vertex.uv.v = halfData;
			}

			if (HasNormals()) {
				for (int j = 0; j < 3; j++)
					stream >> vertex.normal[j];

				stream >> vertex.bitangentY;

				if (HasTangents()) {
					for (int j = 0; j < 3; j++)
						stream >> vertex.tangent[j];

					stream >> vertex.bitangentZ;
				}
			}


			if (HasVertexColors())
				for (int j = 0; j < 4; j++)
					stream >> vertex.colorData[j];

			if (IsSkinned()) {
				for (int j = 0; j < 4; j++) {
					stream.read((char*)&halfData, 2);
					vertex.weights[j] = halfData;
				}

				for (int j = 0; j < 4; j++)
					stream >> vertex.weightBones[j];
			}

			if ((vertFlags7 & (1 << 4)) != 0)
				stream >> vertex.eyeData;
		}
	}

	triangles.resize(numTriangles);

	if (dataSize > 0) {
		for (int i = 0; i < numTriangles; i++)
			stream >> triangles[i];
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() == 100) {
		stream >> particleDataSize;

		if (particleDataSize > 0) {
			particleVerts.resize(numVertices);
			particleNorms.resize(numVertices);
			particleTris.resize(numTriangles);

			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				stream.read((char*)&halfData, 2);
				particleVerts[i].x = halfData;
				stream.read((char*)&halfData, 2);
				particleVerts[i].y = halfData;
				stream.read((char*)&halfData, 2);
				particleVerts[i].z = halfData;
			}

			for (int i = 0; i < numVertices; i++) {
				stream.read((char*)&halfData, 2);
				particleNorms[i].x = halfData;
				stream.read((char*)&halfData, 2);
				particleNorms[i].y = halfData;
				stream.read((char*)&halfData, 2);
				particleNorms[i].z = halfData;
			}

			for (int i = 0; i < numTriangles; i++)
				stream >> particleTris[i];
		}
	}
}

void BSTriShape::Put(NiStream& stream) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Put(stream);

	stream << flags;
	stream << translation;

	for (int i = 0; i < 3; i++)
		stream << rotation[i];

	stream << scale;
	collisionRef.Put(stream);

	stream << bounds;

	skinInstanceRef.Put(stream);
	shaderPropertyRef.Put(stream);
	alphaPropertyRef.Put(stream);

	stream << vertFlags1;
	stream << vertFlags2;
	stream << vertFlags3;
	stream << vertFlags4;
	stream << vertFlags5;
	stream << vertFlags6;
	stream << vertFlags7;
	stream << vertFlags8;

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().User2() < 130 && IsSkinned()) {
		// Triangle and vertex data is in partition instead
		ushort numUShort = 0;
		uint numUInt = 0;
		stream << numUShort;

		if (HasType<BSDynamicTriShape>())
			stream << numVertices;
		else
			stream << numUShort;

		stream << numUInt;
	}
	else {
		if (stream.GetVersion().User() >= 12 && stream.GetVersion().User2() < 130) {
			ushort numUShort = numTriangles;
			stream << numUShort;
		}
		else
			stream << numTriangles;

		stream << numVertices;
		stream << dataSize;

		if (dataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				auto& vertex = vertData[i];
				if (HasVertices()) {
					if (IsFullPrecision() || stream.GetVersion().User2() == 100) {
						// Full precision
						stream << vertex.vert;
						stream << vertex.bitangentX;
					}
					else {
						// Half precision
						halfData = vertex.vert.x;
						stream.write((char*)&halfData, 2);
						halfData = vertex.vert.y;
						stream.write((char*)&halfData, 2);
						halfData = vertex.vert.z;
						stream.write((char*)&halfData, 2);

						halfData = vertex.bitangentX;
						stream.write((char*)&halfData, 2);
					}
				}

				if (HasUVs()) {
					halfData = vertex.uv.u;
					stream.write((char*)&halfData, 2);

					halfData = vertex.uv.v;
					stream.write((char*)&halfData, 2);
				}

				if (HasNormals()) {
					for (int j = 0; j < 3; j++)
						stream << vertex.normal[j];

					stream << vertex.bitangentY;

					if (HasTangents()) {
						for (int j = 0; j < 3; j++)
							stream << vertex.tangent[j];

						stream << vertex.bitangentZ;
					}
				}

				if (HasVertexColors())
					for (int j = 0; j < 4; j++)
						stream << vertex.colorData[j];

				if (IsSkinned()) {
					for (int j = 0; j < 4; j++) {
						halfData = vertex.weights[j];
						stream.write((char*)&halfData, 2);
					}

					for (int j = 0; j < 4; j++)
						stream << vertex.weightBones[j];
				}

				if ((vertFlags7 & (1 << 4)) != 0)
					stream << vertex.eyeData;
			}
		}

		if (dataSize > 0) {
			for (int i = 0; i < numTriangles; i++)
				stream << triangles[i];
		}
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().User2() == 100) {
		stream << particleDataSize;

		if (particleDataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				halfData = particleVerts[i].x;
				stream.write((char*)&halfData, 2);
				halfData = particleVerts[i].y;
				stream.write((char*)&halfData, 2);
				halfData = particleVerts[i].z;
				stream.write((char*)&halfData, 2);
			}

			for (int i = 0; i < numVertices; i++) {
				halfData = particleNorms[i].x;
				stream.write((char*)&halfData, 2);
				halfData = particleNorms[i].y;
				stream.write((char*)&halfData, 2);
				halfData = particleNorms[i].z;
				stream.write((char*)&halfData, 2);
			}

			for (int i = 0; i < numTriangles; i++)
				stream << particleTris[i];
		}
	}
}

void BSTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse(vertData.size(), 0);

	deletedTris.clear();

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

	for (int i = vertData.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertData.erase(vertData.begin() + i);
			numVertices--;
		}
	}

	for (int i = numTriangles - 1; i >= 0; i--) {
		if (indexCollapse[triangles[i].p1] == -1 || indexCollapse[triangles[i].p2] == -1 || indexCollapse[triangles[i].p3] == -1) {
			deletedTris.push_back(i);
			triangles.erase(triangles.begin() + i);
			numTriangles--;
		}
		else {
			triangles[i].p1 = triangles[i].p1 - indexCollapse[triangles[i].p1];
			triangles[i].p2 = triangles[i].p2 - indexCollapse[triangles[i].p2];
			triangles[i].p3 = triangles[i].p3 - indexCollapse[triangles[i].p3];
		}
	}
}

void BSTriShape::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&skinInstanceRef.index);
	refs.insert(&shaderPropertyRef.index);
	refs.insert(&alphaPropertyRef.index);
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
	GetRawVerts();
	bounds = BoundingSphere(rawVertices);
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

void BSTriShape::UpdateFlags(NiVersion& version) {
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
	else if (IsFullPrecision() || version.User2() == 100) {
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

int BSTriShape::CalcDataSizes(NiVersion& version) {
	vertexSize = 0;
	dataSize = 0;

	if (HasVertices()) {		// Position + Bitangent X
		if (IsFullPrecision() || version.User2() == 100) {
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

	UpdateFlags(version);

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
		auto& vertex = vertData[i];
		vertex.vert = (*verts)[i];

		if (uvs && uvs->size() == numVertices)
			vertex.uv = (*uvs)[i];

		vertex.bitangentX = 0.0f;
		vertex.bitangentY = 0;
		vertex.bitangentZ = 0;
		vertex.normal[0] = vertex.normal[1] = vertex.normal[2] = 0;
		memset(vertex.colorData, 255, 4);
		memset(vertex.weights, 0, sizeof(float) * 4);
		memset(vertex.weightBones, 0, 4);
		vertex.eyeData = 0.0f;
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


BSSubIndexTriShape::BSSubIndexTriShape() : BSTriShape() {
}

BSSubIndexTriShape::BSSubIndexTriShape(NiStream& stream) : BSSubIndexTriShape() {
	Get(stream);
}

void BSSubIndexTriShape::Get(NiStream& stream) {
	BSTriShape::Get(stream);

	if (stream.GetVersion().User2() >= 130 && dataSize > 0) {
		stream >> segmentation.numPrimitives;
		stream >> segmentation.numSegments;
		stream >> segmentation.numTotalSegments;

		segmentation.segments.resize(segmentation.numSegments);
		for (auto &segment : segmentation.segments) {
			stream >> segment.startIndex;
			stream >> segment.numPrimitives;
			stream >> segment.parentArrayIndex;
			stream >> segment.numSubSegments;

			segment.subSegments.resize(segment.numSubSegments);
			for (auto &subSegment : segment.subSegments) {
				stream >> subSegment.startIndex;
				stream >> subSegment.numPrimitives;
				stream >> subSegment.arrayIndex;
				stream >> subSegment.unkInt1;
			}
		}

		if (segmentation.numSegments < segmentation.numTotalSegments) {
			stream >> segmentation.subSegmentData.numSegments;
			stream >> segmentation.subSegmentData.numTotalSegments;

			segmentation.subSegmentData.arrayIndices.resize(segmentation.numSegments);
			for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
				stream >> arrayIndex;

			segmentation.subSegmentData.dataRecords.resize(segmentation.numTotalSegments);
			for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
				stream >> dataRecord.segmentUser;
				stream >> dataRecord.unkInt2;
				stream >> dataRecord.numData;

				dataRecord.extraData.resize(dataRecord.numData);
				for (auto &data : dataRecord.extraData)
					stream >> data;
			}

			segmentation.subSegmentData.ssfFile.Get(stream, 2);
		}
	}
	else if (stream.GetVersion().User2() == 100) {
		stream >> numSegments;
		segments.resize(numSegments);

		for (auto &segment : segments)
			segment.Get(stream);
	}
}

void BSSubIndexTriShape::Put(NiStream& stream) {
	BSTriShape::Put(stream);

	if (stream.GetVersion().User2() >= 130 && dataSize > 0) {
		stream << segmentation.numPrimitives;
		stream << segmentation.numSegments;
		stream << segmentation.numTotalSegments;

		for (auto &segment : segmentation.segments) {
			stream << segment.startIndex;
			stream << segment.numPrimitives;
			stream << segment.parentArrayIndex;
			stream << segment.numSubSegments;

			for (auto &subSegment : segment.subSegments) {
				stream << subSegment.startIndex;
				stream << subSegment.numPrimitives;
				stream << subSegment.arrayIndex;
				stream << subSegment.unkInt1;
			}
		}

		if (segmentation.numSegments < segmentation.numTotalSegments) {
			stream << segmentation.subSegmentData.numSegments;
			stream << segmentation.subSegmentData.numTotalSegments;

			for (auto &arrayIndex : segmentation.subSegmentData.arrayIndices)
				stream << arrayIndex;

			for (auto &dataRecord : segmentation.subSegmentData.dataRecords) {
				stream << dataRecord.segmentUser;
				stream << dataRecord.unkInt2;
				stream << dataRecord.numData;

				for (auto &data : dataRecord.extraData)
					stream << data;
			}

			segmentation.subSegmentData.ssfFile.Put(stream, 2, false);
		}
	}
	else if (stream.GetVersion().User2() == 100) {
		stream << numSegments;

		for (auto &segment : segments)
			segment.Put(stream);
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

	// Remove triangles from SSE segments
	for (auto &segment : segments) {
		for (auto &id : deletedTris)
			if (segment.numTris > 0 && id >= segment.index / 3 && id < segment.index / 3 + segment.numTris)
				segment.numTris--;
	}

	// Align SSE segments
	i = 0;
	for (auto &segment : segments) {
		if (i + 1 >= numSegments)
			continue;

		BSGeometrySegmentData& nextSegment = segments[i + 1];
		nextSegment.index = segment.index + segment.numTris * 3;

		i++;
	}
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

	numSegments = 0;
	segments.clear();
}

void BSSubIndexTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	// Skinned most of the time
	SetSkinned(true);
	SetDefaultSegments();
}


BSMeshLODTriShape::BSMeshLODTriShape() : BSTriShape() {
}

BSMeshLODTriShape::BSMeshLODTriShape(NiStream& stream) : BSMeshLODTriShape() {
	Get(stream);
}

void BSMeshLODTriShape::Get(NiStream& stream) {
	BSTriShape::Get(stream);

	stream >> lodSize0;
	stream >> lodSize1;
	stream >> lodSize2;
}

void BSMeshLODTriShape::Put(NiStream& stream) {
	BSTriShape::Put(stream);

	stream << lodSize0;
	stream << lodSize1;
	stream << lodSize2;
}

void BSMeshLODTriShape::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	BSTriShape::notifyVerticesDelete(vertIndices);

	// Force full LOD (workaround)
	lodSize0 = 0;
	lodSize1 = 0;
	lodSize2 = numTriangles;
}


BSDynamicTriShape::BSDynamicTriShape() : BSTriShape() {
	vertFlags6 &= ~(1 << 4);
	vertFlags7 |= 1 << 6;

	dynamicDataSize = 0;
}

BSDynamicTriShape::BSDynamicTriShape(NiStream& stream) : BSDynamicTriShape() {
	Get(stream);
}

void BSDynamicTriShape::Get(NiStream& stream) {
	BSTriShape::Get(stream);

	stream >> dynamicDataSize;

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		stream >> dynamicData[i];
}

void BSDynamicTriShape::Put(NiStream& stream) {
	BSTriShape::Put(stream);

	stream << dynamicDataSize;

	for (int i = 0; i < numVertices; i++)
		stream << dynamicData[i];
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

void BSDynamicTriShape::CalcDynamicData() {
	dynamicDataSize = numVertices * 16;

	dynamicData.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		auto& vertex = vertData[i];
		dynamicData[i].x = vertex.vert.x;
		dynamicData[i].y = vertex.vert.y;
		dynamicData[i].z = vertex.vert.z;
		dynamicData[i].w = vertex.bitangentX;

		if (dynamicData[i].x > 0.0f)
			vertex.eyeData = 1.0f;
		else
			vertex.eyeData = 0.0f;
	}
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


void NiGeometry::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	dataRef.Get(stream);
	skinInstanceRef.Get(stream);

	stream >> numMaterials;
	materialNameRefs.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		materialNameRefs[i].Get(stream);

	materials.resize(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		stream >> materials[i];

	stream >> activeMaterial;
	stream >> dirty;

	if (stream.GetVersion().User() > 11) {
		shaderPropertyRef.Get(stream);
		alphaPropertyRef.Get(stream);
	}
}

void NiGeometry::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	dataRef.Put(stream);
	skinInstanceRef.Put(stream);

	stream << numMaterials;
	for (int i = 0; i < numMaterials; i++)
		materialNameRefs[i].Put(stream);

	for (int i = 0; i < numMaterials; i++)
		stream << materials[i];

	stream << activeMaterial;
	stream << dirty;

	if (stream.GetVersion().User() > 11) {
		shaderPropertyRef.Put(stream);
		alphaPropertyRef.Put(stream);
	}
}

void NiGeometry::GetStringRefs(std::set<StringRef*>& refs) {
	NiAVObject::GetStringRefs(refs);

	for (auto &m : materialNameRefs)
		refs.insert(&m);
}

void NiGeometry::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
	refs.insert(&skinInstanceRef.index);
	refs.insert(&shaderPropertyRef.index);
	refs.insert(&alphaPropertyRef.index);
}


void NiTriBasedGeomData::Get(NiStream& stream) {
	NiGeometryData::Get(stream);

	stream >> numTriangles;
}

void NiTriBasedGeomData::Put(NiStream& stream) {
	NiGeometryData::Put(stream);

	stream << numTriangles;
}

void NiTriBasedGeomData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords) {
	NiGeometryData::Create(verts, inTris, texcoords);

	numTriangles = inTris->size();
}


NiTriShape::NiTriShape(NiStream& stream) : NiTriShape() {
	Get(stream);
}


NiTriShapeData::NiTriShapeData(NiStream& stream) : NiTriShapeData() {
	Get(stream);
}

void NiTriShapeData::Get(NiStream& stream) {
	NiTriBasedGeomData::Get(stream);

	stream >> numTrianglePoints;
	stream >> hasTriangles;

	if (hasTriangles) {
		triangles.resize(numTriangles);
		for (int i = 0; i < numTriangles; i++) 
			stream >> triangles[i];
	}

	MatchGroup mg;
	stream >> numMatchGroups;
	matchGroups.resize(numMatchGroups);
	for (int i = 0; i < numMatchGroups; i++) {
		stream >> mg.count;
		mg.matches.resize(mg.count);
		for (int j = 0; j < mg.count; j++)
			stream >> mg.matches[j];

		matchGroups[i] = mg;
	}

	// Not supported yet, so clear it again after reading
	matchGroups.clear();
	numMatchGroups = 0;
}

void NiTriShapeData::Put(NiStream& stream) {
	NiTriBasedGeomData::Put(stream);

	stream << numTrianglePoints;
	stream << hasTriangles;

	if (hasTriangles) {
		for (int i = 0; i < numTriangles; i++)
			stream << triangles[i];
	}

	stream << numMatchGroups;
	for (int i = 0; i < numMatchGroups; i++) {
		stream << matchGroups[i].count;
		for (int j = 0; j < matchGroups[i].count; j++)
			stream << matchGroups[i].matches[j];
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


NiTriStrips::NiTriStrips(NiStream& stream) : NiTriStrips() {
	Get(stream);
}


NiTriStripsData::NiTriStripsData(NiStream& stream) : NiTriStripsData() {
	Get(stream);
}

void NiTriStripsData::Get(NiStream& stream) {
	NiTriBasedGeomData::Get(stream);

	stream >> numStrips;
	stripLengths.resize(numStrips);
	for (int i = 0; i < numStrips; i++)
		stream >> stripLengths[i];

	stream >> hasPoints;
	if (hasPoints) {
		points.resize(numStrips);
		for (int i = 0; i < numStrips; i++) {
			points[i].resize(stripLengths[i]);
			for (int j = 0; j < stripLengths[i]; j++)
				stream >> points[i][j];
		}
	}
}

void NiTriStripsData::Put(NiStream& stream) {
	NiTriBasedGeomData::Put(stream);

	stream << numStrips;
	for (int i = 0; i < numStrips; i++)
		stream << stripLengths[i];

	stream << hasPoints;
	if (hasPoints) {
		for (int i = 0; i < numStrips; i++)
			for (int j = 0; j < stripLengths[i]; j++)
				stream << points[i][j];
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


BSLODTriShape::BSLODTriShape(NiStream& stream) : BSLODTriShape() {
	Get(stream);
}

void BSLODTriShape::Get(NiStream& stream) {
	NiTriBasedGeom::Get(stream);

	stream >> level0;
	stream >> level1;
	stream >> level2;
}

void BSLODTriShape::Put(NiStream& stream) {
	NiTriBasedGeom::Put(stream);

	stream << level0;
	stream << level1;
	stream << level2;
}


void BSGeometrySegmentData::Get(NiStream& stream) {
	stream >> flags;
	stream >> index;
	stream >> numTris;
}

void BSGeometrySegmentData::Put(NiStream& stream) {
	stream << flags;
	stream << index;
	stream << numTris;
}


BSSegmentedTriShape::BSSegmentedTriShape(NiStream& stream) : NiTriShape() {
	Get(stream);
}

void BSSegmentedTriShape::Get(NiStream& stream) {
	NiTriShape::Get(stream);

	stream >> numSegments;
	segments.resize(numSegments);

	for (auto &segment : segments)
		segment.Get(stream);
}

void BSSegmentedTriShape::Put(NiStream& stream) {
	NiTriShape::Put(stream);

	stream << numSegments;

	for (auto &segment : segments)
		segment.Put(stream);
}
