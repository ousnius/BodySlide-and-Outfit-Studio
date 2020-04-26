/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Geometry.h"
#include "Skin.h"
#include "Nodes.h"

#include "utils/KDMatcher.h"
#include "NifUtil.h"

void NiAdditionalGeometryData::Get(NiStream & stream) {
	AdditionalGeomData::Get(stream);

	stream >> numVertices;

	stream >> numBlockInfos;
	blockInfos.resize(numBlockInfos);
	for (int i = 0; i < numBlockInfos; i++)
		blockInfos[i].Get(stream);

	stream >> numBlocks;
	blocks.resize(numBlocks);
	for (int i = 0; i < numBlocks; i++)
		blocks[i].Get(stream);
}

void NiAdditionalGeometryData::Put(NiStream & stream) {
	AdditionalGeomData::Put(stream);

	stream << numVertices;

	stream << numBlockInfos;
	for (int i = 0; i < numBlockInfos; i++)
		blockInfos[i].Put(stream);

	stream << numBlocks;
	for (int i = 0; i < numBlocks; i++)
		blocks[i].Put(stream);
}


void BSPackedAdditionalGeometryData::Get(NiStream & stream) {
	AdditionalGeomData::Get(stream);

	stream >> numVertices;

	stream >> numBlockInfos;
	blockInfos.resize(numBlockInfos);
	for (int i = 0; i < numBlockInfos; i++)
		blockInfos[i].Get(stream);

	stream >> numBlocks;
	blocks.resize(numBlocks);
	for (int i = 0; i < numBlocks; i++)
		blocks[i].Get(stream);
}

void BSPackedAdditionalGeometryData::Put(NiStream & stream) {
	AdditionalGeomData::Put(stream);

	stream << numVertices;

	stream << numBlockInfos;
	for (int i = 0; i < numBlockInfos; i++)
		blockInfos[i].Put(stream);

	stream << numBlocks;
	for (int i = 0; i < numBlocks; i++)
		blocks[i].Put(stream);
}


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
	if (stream.GetVersion().Stream() >= 34)
		numTextureSets = numUVSets & 0x1;

	if (stream.GetVersion().Stream() > 34)
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
		uvSets.resize(numTextureSets);
		for (int i = 0; i < numTextureSets; i++) {
			uvSets[i].resize(numVertices);
			for (int j = 0; j < numVertices; j++)
				stream >> uvSets[i][j];
		}
	}

	stream >> consistencyFlags;
	additionalDataRef.Get(stream);
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
	if (stream.GetVersion().Stream() >= 34)
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
		for (int i = 0; i < numTextureSets; i++)
			for (int j = 0; j < numVertices; j++)
				stream << uvSets[i][j];
	}

	stream << consistencyFlags;
	additionalDataRef.Put(stream);
}

void NiGeometryData::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&additionalDataRef);
}

int NiGeometryData::GetAdditionalDataRef() {
	return additionalDataRef.GetIndex();
}

void NiGeometryData::SetAdditionalDataRef(int dataRef) {
	additionalDataRef.SetIndex(dataRef);
}

ushort NiGeometryData::GetNumVertices() {
	return numVertices;
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
		vertexColors.resize(numVertices, Color4(1.0f, 1.0f, 1.0f, 1.0f));
	else
		vertexColors.clear();
}

void NiGeometryData::SetUVs(const bool enable) {
	if (enable) {
		numUVSets |= 1 << 0;
		uvSets.resize(1);
		uvSets[0].resize(numVertices);
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

uint NiGeometryData::GetNumTriangles() { return 0; }
bool NiGeometryData::GetTriangles(std::vector<Triangle>&) { return false; }
void NiGeometryData::SetTriangles(const std::vector<Triangle>&) { };

void NiGeometryData::UpdateBounds() {
	bounds = BoundingSphere(vertices);
}

void NiGeometryData::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>*, const std::vector<Vector2>* texcoords, const std::vector<Vector3>* norms) {
	size_t vertCount = verts->size();
	ushort maxIndex = std::numeric_limits<ushort>().max();

	if (vertCount > maxIndex)
		numVertices = 0;
	else
		numVertices = ushort(vertCount);

	vertices.resize(numVertices);
	for (int v = 0; v < numVertices; v++)
		vertices[v] = (*verts)[v];

	bounds = BoundingSphere(*verts);

	if (texcoords) {
		size_t uvCount = texcoords->size();
		if (uvCount == numVertices) {
			SetUVs(true);

			for (size_t uv = 0; uv < uvSets[0].size(); uv++)
				uvSets[0][uv] = (*texcoords)[uv];
		}
		else {
			SetUVs(false);
		}
	}
	else {
		SetUVs(false);
	}

	if (norms && norms->size() == numVertices) {
		SetNormals(true);
		normals = (*norms);
		CalcTangentSpace();
	}
	else {
		SetNormals(false);
		SetTangents(false);
	}
}

void NiGeometryData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	EraseVectorIndices(vertices, vertIndices);
	numVertices = vertices.size();
	if (!normals.empty())
		EraseVectorIndices(normals, vertIndices);
	if (!tangents.empty())
		EraseVectorIndices(tangents, vertIndices);
	if (!bitangents.empty())
		EraseVectorIndices(bitangents, vertIndices);
	if (!vertexColors.empty())
		EraseVectorIndices(vertexColors, vertIndices);
	for (int j = 0; j < uvSets.size(); j++)
		EraseVectorIndices(uvSets[j], vertIndices);
}

void NiGeometryData::RecalcNormals(const bool, const float) {
	SetNormals(true);
}

void NiGeometryData::CalcTangentSpace() {
	SetTangents(true);
}


NiGeometryData* NiShape::GetGeomData() { return nullptr; };
void NiShape::SetGeomData(NiGeometryData*) { };

int NiShape::GetSkinInstanceRef() { return 0xFFFFFFFF; }
void NiShape::SetSkinInstanceRef(int) { }

int NiShape::GetShaderPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetShaderPropertyRef(int) { }

int NiShape::GetAlphaPropertyRef() { return 0xFFFFFFFF; }
void NiShape::SetAlphaPropertyRef(int) { }

int NiShape::GetDataRef() { return 0xFFFFFFFF; }
void NiShape::SetDataRef(int) { }

ushort NiShape::GetNumVertices() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->GetNumVertices();

	return 0;
}

void NiShape::SetVertices(const bool enable) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetVertices(enable);
};

bool NiShape::HasVertices() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertices();

	return false;
};

void NiShape::SetUVs(const bool enable) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetUVs(enable);
};

bool NiShape::HasUVs() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->HasUVs();

	return false;
};

void NiShape::SetNormals(const bool enable) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetNormals(enable);
};

bool NiShape::HasNormals() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->HasNormals();

	return false;
};

void NiShape::SetTangents(const bool enable) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetTangents(enable);
};

bool NiShape::HasTangents() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->HasTangents();

	return false;
};

void NiShape::SetVertexColors(const bool enable) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetVertexColors(enable);
};

bool NiShape::HasVertexColors() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->HasVertexColors();

	return false;
};

void NiShape::SetSkinned(const bool) { };
bool NiShape::IsSkinned() { return false; };

uint NiShape::GetNumTriangles() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->GetNumTriangles();

	return 0;
}

bool NiShape::GetTriangles(std::vector<Triangle>& tris) {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->GetTriangles(tris);

	return false;
};

void NiShape::SetTriangles(const std::vector<Triangle>& tris) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetTriangles(tris);
};

void NiShape::SetBounds(const BoundingSphere& bounds) {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->SetBounds(bounds);
}

BoundingSphere NiShape::GetBounds() {
	auto geomData = GetGeomData();
	if (geomData)
		return geomData->GetBounds();

	return BoundingSphere();
}

void NiShape::UpdateBounds() {
	auto geomData = GetGeomData();
	if (geomData)
		geomData->UpdateBounds();
}

int NiShape::GetBoneID(NiHeader& hdr, const std::string& boneName) {
	auto boneCont = hdr.GetBlock<NiBoneContainer>(GetSkinInstanceRef());
	if (boneCont) {
		int i = 0;
		for (auto& bone : boneCont->GetBones()) {
			auto node = hdr.GetBlock<NiNode>(bone.GetIndex());
			if (node && node->GetName() == boneName)
				return i;
			++i;
		}
	}

	return 0xFFFFFFFF;
}

bool NiShape::ReorderTriangles(const std::vector<uint>& triInds) {
	std::vector<Triangle> trisOrdered;
	std::vector<Triangle> tris;
	if (!GetTriangles(tris))
		return false;

	if (tris.size() != triInds.size())
		return false;

	for (uint id : triInds)
		if (id < tris.size())
			trisOrdered.push_back(tris[id]);

	if (trisOrdered.size() != tris.size())
		return false;

	SetTriangles(trisOrdered);
	return true;
}


BSTriShape::BSTriShape() {
	vertexDesc.SetFlag(VF_VERTEX);
	vertexDesc.SetFlag(VF_UV);
	vertexDesc.SetFlag(VF_NORMAL);
	vertexDesc.SetFlag(VF_TANGENT);
	vertexDesc.SetFlag(VF_SKINNED);
}

void BSTriShape::Get(NiStream& stream) {
	// The order of definition deviates slightly from previous versions, so can't directly use the super Get... instead
	// that code is duplicated here and the super super get is called.
	NiObjectNET::Get(stream);

	stream >> flags;
	stream >> transform.translation;
	stream >> transform.rotation;
	stream >> transform.scale;

	collisionRef.Get(stream);

	stream >> bounds;

	skinInstanceRef.Get(stream);
	shaderPropertyRef.Get(stream);
	alphaPropertyRef.Get(stream);

	vertexDesc.Get(stream);

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() < 130) {
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
				if (IsFullPrecision() || stream.GetVersion().Stream() == 100) {
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

			if (HasEyeData())
				stream >> vertex.eyeData;
		}
	}

	triangles.resize(numTriangles);

	if (dataSize > 0) {
		for (int i = 0; i < numTriangles; i++)
			stream >> triangles[i];
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() == 100) {
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
	stream << transform.translation;
	stream << transform.rotation;
	stream << transform.scale;

	collisionRef.Put(stream);

	stream << bounds;

	skinInstanceRef.Put(stream);
	shaderPropertyRef.Put(stream);
	alphaPropertyRef.Put(stream);

	vertexDesc.Put(stream);

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() < 130 && IsSkinned()) {
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
		if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() < 130) {
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
					if (IsFullPrecision() || stream.GetVersion().Stream() == 100) {
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

				if (HasEyeData())
					stream << vertex.eyeData;
			}
		}

		if (dataSize > 0) {
			for (int i = 0; i < numTriangles; i++)
				stream << triangles[i];
		}
	}

	if (stream.GetVersion().User() == 12 && stream.GetVersion().Stream() == 100) {
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
	deletedTris.clear();

	std::vector<int> indexCollapse = GenerateIndexCollapseMap(vertIndices, vertData.size());

	EraseVectorIndices(vertData, vertIndices);
	numVertices = vertData.size();

	ApplyMapToTriangles(triangles, indexCollapse, &deletedTris);
	numTriangles = triangles.size();

	std::sort(deletedTris.begin(), deletedTris.end(), std::greater<>());
}

void BSTriShape::GetChildRefs(std::set<Ref*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&skinInstanceRef);
	refs.insert(&shaderPropertyRef);
	refs.insert(&alphaPropertyRef);
}

int BSTriShape::GetSkinInstanceRef() {
	return skinInstanceRef.GetIndex();
}

void BSTriShape::SetSkinInstanceRef(int skinInstRef) {
	skinInstanceRef.SetIndex(skinInstRef);
}

int BSTriShape::GetShaderPropertyRef() {
	return shaderPropertyRef.GetIndex();
}

void BSTriShape::SetShaderPropertyRef(int shaderPropRef) {
	shaderPropertyRef.SetIndex(shaderPropRef);
}

int BSTriShape::GetAlphaPropertyRef() {
	return alphaPropertyRef.GetIndex();
}

void BSTriShape::SetAlphaPropertyRef(int alphaPropRef) {
	alphaPropertyRef.SetIndex(alphaPropRef);
}

std::vector<Vector3>* BSTriShape::GetRawVerts() {
	rawVertices.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawVertices[i] = vertData[i].vert;

	return &rawVertices;
}

std::vector<Vector3>* BSTriShape::GetNormalData(bool xform) {
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

std::vector<Vector3>* BSTriShape::GetTangentData(bool xform) {
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

std::vector<Vector3>* BSTriShape::GetBitangentData(bool xform) {
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

std::vector<Vector2>* BSTriShape::GetUVData() {
	if (!HasUVs())
		return nullptr;

	rawUvs.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		rawUvs[i] = vertData[i].uv;

	return &rawUvs;
}

std::vector<Color4>* BSTriShape::GetColorData() {
	if (!HasVertexColors())
		return nullptr;

	rawColors.resize(numVertices);
	for (int i = 0; i < numVertices; i++) {
		rawColors[i].r = vertData[i].colorData[0] / 255.0f;
		rawColors[i].g = vertData[i].colorData[1] / 255.0f;
		rawColors[i].b = vertData[i].colorData[2] / 255.0f;
		rawColors[i].a = vertData[i].colorData[3] / 255.0f;
	}

	return &rawColors;
}

std::vector<float>* BSTriShape::GetEyeData() {
	if (!HasEyeData())
		return nullptr;

	rawEyeData.resize(numVertices);
	for (int i = 0; i < numVertices; ++i)
		rawEyeData[i] = vertData[i].eyeData;

	return &rawEyeData;
}

ushort BSTriShape::GetNumVertices() {
	return numVertices;
}

void BSTriShape::SetVertices(const bool enable) {
	if (enable) {
		vertexDesc.SetFlag(VF_VERTEX);
		vertData.resize(numVertices);
	}
	else {
		vertexDesc.RemoveFlag(VF_VERTEX);
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
		vertexDesc.SetFlag(VF_UV);
	else
		vertexDesc.RemoveFlag(VF_UV);
}

void BSTriShape::SetSecondUVs(const bool enable) {
	if (enable)
		vertexDesc.SetFlag(VF_UV_2);
	else
		vertexDesc.RemoveFlag(VF_UV_2);
}

void BSTriShape::SetNormals(const bool enable) {
	if (enable)
		vertexDesc.SetFlag(VF_NORMAL);
	else
		vertexDesc.RemoveFlag(VF_NORMAL);
}

void BSTriShape::SetTangents(const bool enable) {
	if (enable)
		vertexDesc.SetFlag(VF_TANGENT);
	else
		vertexDesc.RemoveFlag(VF_TANGENT);
}

void BSTriShape::SetVertexColors(const bool enable) {
	if (enable) {
		if (!vertexDesc.HasFlag(VF_COLORS)) {
			for (auto &v : vertData) {
				v.colorData[0] = 255;
				v.colorData[1] = 255;
				v.colorData[2] = 255;
				v.colorData[3] = 255;
			}
		}

		vertexDesc.SetFlag(VF_COLORS);
	}
	else
		vertexDesc.RemoveFlag(VF_COLORS);
}

void BSTriShape::SetSkinned(const bool enable) {
	if (enable)
		vertexDesc.SetFlag(VF_SKINNED);
	else
		vertexDesc.RemoveFlag(VF_SKINNED);
}

void BSTriShape::SetEyeData(const bool enable) {
	if (enable)
		vertexDesc.SetFlag(VF_EYEDATA);
	else
		vertexDesc.RemoveFlag(VF_EYEDATA);
}

void BSTriShape::SetFullPrecision(const bool enable) {
	if (!CanChangePrecision())
		return;

	if (enable)
		vertexDesc.SetFlag(VF_FULLPREC);
	else
		vertexDesc.RemoveFlag(VF_FULLPREC);
}

uint BSTriShape::GetNumTriangles() {
	return numTriangles;
}

bool BSTriShape::GetTriangles(std::vector<Triangle>& tris) {
	tris = triangles;
	return true;
}

void BSTriShape::SetTriangles(const std::vector<Triangle>& tris) {
	triangles = tris;
	numTriangles = triangles.size();
}

void BSTriShape::UpdateBounds() {
	GetRawVerts();
	bounds = BoundingSphere(rawVertices);
}

void BSTriShape::SetVertexData(const std::vector<BSVertexData>& bsVertData) {
	vertData = bsVertData;
	numVertices = vertData.size();
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

void BSTriShape::SetTangentData(const std::vector<Vector3>& in) {
	SetTangents(true);

	for (int i = 0; i < numVertices; i++) {
		vertData[i].tangent[0] = (unsigned char)round((((in[i].x + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[1] = (unsigned char)round((((in[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].tangent[2] = (unsigned char)round((((in[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::SetBitangentData(const std::vector<Vector3>& in) {
	SetTangents(true);

	for (int i = 0; i < numVertices; i++) {
		vertData[i].bitangentX = in[i].x;
		vertData[i].bitangentY = (unsigned char)round((((in[i].y + 1.0f) / 2.0f) * 255.0f));
		vertData[i].bitangentZ = (unsigned char)round((((in[i].z + 1.0f) / 2.0f) * 255.0f));
	}
}

void BSTriShape::SetEyeData(const std::vector<float>& in) {
	SetEyeData(true);

	for (int i = 0; i < numVertices; i++)
		vertData[i].eyeData = in[i];
}

static void CalculateNormals(const std::vector<Vector3> &verts, const std::vector<Triangle> &tris, std::vector<Vector3> &norms, const bool smooth, float smoothThresh) {
	// Zero norms
	norms.clear();
	norms.resize(verts.size());

	// Face normals
	for (const Triangle &t : tris) {
		Vector3 tn;
		t.trinormal(verts, &tn);
		norms[t.p1] += tn;
		norms[t.p2] += tn;
		norms[t.p3] += tn;
	}

	for (Vector3 &n : norms)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		smoothThresh *= DEG2RAD;
		std::vector<Vector3> seamNorms;
		SortingMatcher matcher(verts.data(), verts.size());
		for (const std::vector<int> &matchset : matcher.matches) {
			seamNorms.resize(matchset.size());
			for (int j = 0; j < matchset.size(); ++j) {
				const Vector3 &n = norms[matchset[j]];
				Vector3 sn = n;
				for (int k = 0; k < matchset.size(); ++k) {
					if (j == k)
						continue;
					const Vector3 &mn = norms[matchset[k]];
					if (n.angle(mn) >= smoothThresh)
						continue;
					sn += mn;
				}
				sn.Normalize();
				seamNorms[j] = sn;
			}
			for (int j = 0; j < matchset.size(); ++j)
				norms[matchset[j]] = seamNorms[j];
		}
	}
}

void BSTriShape::RecalcNormals(const bool smooth, const float smoothThresh) {
	GetRawVerts();
	SetNormals(true);

	std::vector<Vector3> verts(numVertices);
	for (int i = 0; i < numVertices; i++) {
		verts[i].x = rawVertices[i].x * -0.1f;
		verts[i].z = rawVertices[i].y * 0.1f;
		verts[i].y = rawVertices[i].z * 0.1f;
	}

	std::vector<Vector3> norms;
	CalculateNormals(verts, triangles, norms, smooth, smoothThresh);

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

		if (i1 >= numVertices || i2 >= numVertices || i3 >= numVertices)
			continue;

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

int BSTriShape::CalcDataSizes(NiVersion& version) {
	vertexSize = 0;
	dataSize = 0;

	VertexFlags vf = vertexDesc.GetFlags();
	vertexDesc.ClearAttributeOffsets();

	uint attributeSizes[VA_COUNT] = {};
	if (HasVertices()) {
		if (IsFullPrecision() || version.Stream() == 100)
			attributeSizes[VA_POSITION] = 4;
		else
			attributeSizes[VA_POSITION] = 2;
	}

	if (HasUVs())
		attributeSizes[VA_TEXCOORD0] = 1;

	if (HasSecondUVs())
		attributeSizes[VA_TEXCOORD1] = 1;

	if (HasNormals()) {
		attributeSizes[VA_NORMAL] = 1;

		if (HasTangents())
			attributeSizes[VA_BINORMAL] = 1;
	}

	if (HasVertexColors())
		attributeSizes[VA_COLOR] = 1;

	if (IsSkinned())
		attributeSizes[VA_SKINNING] = 3;

	if (HasEyeData())
		attributeSizes[VA_EYEDATA] = 1;

	for (int va = 0; va < VA_COUNT; va++) {
		if (attributeSizes[va] != 0) {
			vertexDesc.SetAttributeOffset(VertexAttribute(va), vertexSize);
			vertexSize += attributeSizes[va] * 4;
		}
	}

	vertexDesc.SetSize(vertexSize);
	vertexDesc.SetFlags(vf);

	if (HasType<BSDynamicTriShape>())
		vertexDesc.MakeDynamic();

	dataSize = vertexSize * numVertices + 6 * numTriangles;

	return dataSize;
}

void BSTriShape::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals) {
	flags = 14;

	ushort maxVertIndex = std::numeric_limits<ushort>().max();
	size_t vertCount = verts->size();
	if (vertCount > maxVertIndex)
		numVertices = 0;
	else
		numVertices = ushort(vertCount);

	uint maxTriIndex = std::numeric_limits<uint>().max();
	size_t triCount = tris ? tris->size() : 0;
	if (triCount > maxTriIndex || numVertices == 0)
		numTriangles = 0;
	else
		numTriangles = uint(triCount);

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
		std::memset(vertex.colorData, 255, 4);
		std::memset(vertex.weights, 0, sizeof(float) * 4);
		std::memset(vertex.weightBones, 0, 4);
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


void BSSubIndexTriShape::Get(NiStream& stream) {
	BSTriShape::Get(stream);

	if (stream.GetVersion().Stream() >= 130 && dataSize > 0) {
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
				stream >> dataRecord.userSlotID;
				stream >> dataRecord.material;
				stream >> dataRecord.numData;

				dataRecord.extraData.resize(dataRecord.numData);
				for (auto &data : dataRecord.extraData)
					stream >> data;
			}

			segmentation.subSegmentData.ssfFile.Get(stream, 2);
		}
	}
	else if (stream.GetVersion().Stream() == 100) {
		stream >> numSegments;
		segments.resize(numSegments);

		for (auto &segment : segments)
			segment.Get(stream);
	}
}

void BSSubIndexTriShape::Put(NiStream& stream) {
	BSTriShape::Put(stream);

	if (stream.GetVersion().Stream() >= 130 && dataSize > 0) {
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
				stream << dataRecord.userSlotID;
				stream << dataRecord.material;
				stream << dataRecord.numData;

				for (auto &data : dataRecord.extraData)
					stream << data;
			}

			segmentation.subSegmentData.ssfFile.Put(stream, 2, false);
		}
	}
	else if (stream.GetVersion().Stream() == 100) {
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

void BSSubIndexTriShape::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	// Skinned most of the time
	SetSkinned(true);
	SetDefaultSegments();
}

void BSSubIndexTriShape::GetSegmentation(NifSegmentationInfo &inf, std::vector<int> &triParts) {
	inf.segs.clear();
	inf.ssfFile = segmentation.subSegmentData.ssfFile.GetString();
	inf.segs.resize(segmentation.segments.size());
	triParts.clear();

	uint numTris = GetNumTriangles();
	triParts.resize(numTris, -1);

	int partID = 0;
	int arrayIndex = 0;

	for (int i = 0; i < segmentation.segments.size(); ++i) {
		BSSITSSegment &seg = segmentation.segments[i];
		uint startIndex = seg.startIndex / 3;
		uint endIndex = std::min(numTris, startIndex + seg.numPrimitives);

		for (uint id = startIndex; id < endIndex; id++)
			triParts[id] = partID;

		inf.segs[i].partID = partID++;
		inf.segs[i].subs.resize(seg.subSegments.size());

		for (int j = 0; j < seg.subSegments.size(); ++j) {
			BSSITSSubSegment &sub = seg.subSegments[j];
			startIndex = sub.startIndex / 3;

			endIndex = std::min(numTris, startIndex + sub.numPrimitives);
			for (uint id = startIndex; id < endIndex; id++)
				triParts[id] = partID;

			inf.segs[i].subs[j].partID = partID++;
			arrayIndex++;

			BSSITSSubSegmentDataRecord &rec = segmentation.subSegmentData.dataRecords[arrayIndex];
			inf.segs[i].subs[j].userSlotID = rec.userSlotID < 30 ? 0 : rec.userSlotID;
			inf.segs[i].subs[j].material = rec.material;
			inf.segs[i].subs[j].extraData = rec.extraData;
		}
		arrayIndex++;
	}
}

void BSSubIndexTriShape::SetSegmentation(const NifSegmentationInfo &inf, const std::vector<int> &inTriParts) {
	uint numTris = GetNumTriangles();
	if (inTriParts.size() != numTris)
		return;

	// Renumber partitions so that the partition IDs are increasing.
	int newPartID = 0;
	std::vector<int> oldToNewPartIDs;
	for (const NifSegmentInfo &seg : inf.segs) {
		if (seg.partID >= oldToNewPartIDs.size())
			oldToNewPartIDs.resize(seg.partID + 1);

		oldToNewPartIDs[seg.partID] = newPartID++;
		for (const NifSubSegmentInfo &sub : seg.subs) {
			if (sub.partID >= oldToNewPartIDs.size())
				oldToNewPartIDs.resize(sub.partID + 1);
			oldToNewPartIDs[sub.partID] = newPartID++;
		}
	}

	std::vector<int> triParts(numTris);
	for (int i = 0; i < numTris; ++i)
		if (triParts[i] >= 0)
			triParts[i] = oldToNewPartIDs[inTriParts[i]];

	// Sort triangles by partition ID
	std::vector<uint> triInds(numTris);
	for (int i = 0; i < numTris; ++i)
		triInds[i] = i;

	std::stable_sort(triInds.begin(), triInds.end(), [&triParts](int i, int j) {
		return triParts[i] < triParts[j];
	});

	ReorderTriangles(triInds);
	// Note that triPart's indexing no longer matches triangle indexing.

	// Find first triangle of each partition
	std::vector<int> partTriInds(newPartID + 1);
	for (int i = 0, j = 0; i < triInds.size(); ++i)
		while (triParts[triInds[i]] >= j)
			partTriInds[j++] = i;

	partTriInds.back() = triInds.size();

	segmentation = std::move(BSSITSSegmentation());
	uint parentArrayIndex = 0;
	uint segmentIndex = 0;
	int partID = 0;

	for (const NifSegmentInfo &seg : inf.segs) {
		// Create new segment
		segmentation.segments.emplace_back();
		BSSITSSegment &segment = segmentation.segments.back();
		int childCount = seg.subs.size();
		segment.numPrimitives = partTriInds[partID + childCount + 1] - partTriInds[partID];
		segment.startIndex = partTriInds[partID] * 3;
		segment.numSubSegments = childCount;
		++partID;

		// Create new segment data record
		BSSITSSubSegmentDataRecord segmentDataRecord;
		segmentDataRecord.userSlotID = segmentIndex;
		segmentation.subSegmentData.arrayIndices.push_back(parentArrayIndex);
		segmentation.subSegmentData.dataRecords.push_back(segmentDataRecord);

		uint subSegmentNumber = 1;
		for (const NifSubSegmentInfo &sub : seg.subs) {
			// Create new subsegment
			segment.subSegments.emplace_back();
			BSSITSSubSegment &subSegment = segment.subSegments.back();
			subSegment.arrayIndex = parentArrayIndex;
			subSegment.numPrimitives = partTriInds[partID + 1] - partTriInds[partID];
			subSegment.startIndex = partTriInds[partID] * 3;
			++partID;

			// Create new subsegment data record
			BSSITSSubSegmentDataRecord subSegmentDataRecord;
			if (sub.userSlotID < 30)
				subSegmentDataRecord.userSlotID = subSegmentNumber++;
			else
				subSegmentDataRecord.userSlotID = sub.userSlotID;

			subSegmentDataRecord.material = sub.material;
			subSegmentDataRecord.numData = sub.extraData.size();
			subSegmentDataRecord.extraData = sub.extraData;
			segmentation.subSegmentData.dataRecords.push_back(subSegmentDataRecord);
		}

		parentArrayIndex += childCount + 1;
		++segmentIndex;
	}

	segmentation.numPrimitives = numTris;
	segmentation.numSegments = segmentIndex;
	segmentation.numTotalSegments = parentArrayIndex;
	segmentation.subSegmentData.numSegments = segmentIndex;
	segmentation.subSegmentData.numTotalSegments = parentArrayIndex;
	segmentation.subSegmentData.ssfFile.SetString(inf.ssfFile);
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


BSDynamicTriShape::BSDynamicTriShape() {
	vertexDesc.RemoveFlag(VF_VERTEX);
	vertexDesc.SetFlag(VF_FULLPREC);

	dynamicDataSize = 0;
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

	EraseVectorIndices(dynamicData, vertIndices);
	dynamicDataSize = dynamicData.size();
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

void BSDynamicTriShape::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	uint maxIndex = std::numeric_limits<uint>().max();
	size_t vertCount = verts->size();
	if (vertCount > maxIndex)
		dynamicDataSize = 0;
	else
		dynamicDataSize = uint(vertCount);

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

	if (stream.GetVersion().File() >= V20_2_0_5) {
		stream >> numMaterials;
		materialNameRefs.resize(numMaterials);
		materials.resize(numMaterials);

		for (int i = 0; i < numMaterials; i++) {
			materialNameRefs[i].Get(stream);
			stream >> materials[i];
		}

		stream >> activeMaterial;
	}
	else {
		stream >> shader;
		
		if (shader) {
			shaderName.Get(stream);
			stream >> implementation;
		}
	}

	if (stream.GetVersion().File() >= V20_2_0_7)
		stream >> defaultMatNeedsUpdateFlag;

	if (stream.GetVersion().Stream() > 34) {
		shaderPropertyRef.Get(stream);
		alphaPropertyRef.Get(stream);
	}
}

void NiGeometry::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	dataRef.Put(stream);
	skinInstanceRef.Put(stream);

	if (stream.GetVersion().File() >= V20_2_0_5) {
		stream << numMaterials;
		for (int i = 0; i < numMaterials; i++) {
			materialNameRefs[i].Put(stream);
			stream << materials[i];
		}
		stream << activeMaterial;
	}
	else {
		stream << shader;

		if (shader) {
			shaderName.Put(stream);
			stream << implementation;
		}
	}

	if (stream.GetVersion().File() >= V20_2_0_7)
		stream << defaultMatNeedsUpdateFlag;

	if (stream.GetVersion().Stream() > 34) {
		shaderPropertyRef.Put(stream);
		alphaPropertyRef.Put(stream);
	}
}

void NiGeometry::GetStringRefs(std::set<StringRef*>& refs) {
	NiAVObject::GetStringRefs(refs);

	for (auto &m : materialNameRefs)
		refs.insert(&m);
}

void NiGeometry::GetChildRefs(std::set<Ref*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&dataRef);
	refs.insert(&skinInstanceRef);
	refs.insert(&shaderPropertyRef);
	refs.insert(&alphaPropertyRef);
}

bool NiGeometry::IsSkinned() {
	return skinInstanceRef.GetIndex() != 0xFFFFFFFF;
}

int NiGeometry::GetDataRef() {
	return dataRef.GetIndex();
}

void NiGeometry::SetDataRef(int datRef) {
	dataRef.SetIndex(datRef);
}

int NiGeometry::GetSkinInstanceRef() {
	return skinInstanceRef.GetIndex();
}

void NiGeometry::SetSkinInstanceRef(int skinInstRef) {
	skinInstanceRef.SetIndex(skinInstRef);
}

int NiGeometry::GetShaderPropertyRef() {
	return shaderPropertyRef.GetIndex();
}

void NiGeometry::SetShaderPropertyRef(int shaderPropRef) {
	shaderPropertyRef.SetIndex(shaderPropRef);
}

int NiGeometry::GetAlphaPropertyRef() {
	return alphaPropertyRef.GetIndex();
}

void NiGeometry::SetAlphaPropertyRef(int alphaPropRef) {
	alphaPropertyRef.SetIndex(alphaPropRef);
}


void NiTriBasedGeomData::Get(NiStream& stream) {
	NiGeometryData::Get(stream);

	stream >> numTriangles;
}

void NiTriBasedGeomData::Put(NiStream& stream) {
	NiGeometryData::Put(stream);

	stream << numTriangles;
}

void NiTriBasedGeomData::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* inTris, const std::vector<Vector2>* texcoords, const std::vector<Vector3>* norms) {
	NiGeometryData::Create(verts, inTris, texcoords, norms);

	if (inTris) {
		ushort maxIndex = std::numeric_limits<ushort>().max();
		size_t triCount = inTris ? inTris->size() : 0;
		if (triCount <= maxIndex && numVertices != 0)
			numTriangles = ushort(triCount);
		else
			numTriangles = 0;
	}
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

void NiTriShapeData::Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* inTris, const std::vector<Vector2>* texcoords, const std::vector<Vector3>* norms) {
	NiTriBasedGeomData::Create(verts, inTris, texcoords, norms);

	if (numTriangles > 0) {
		numTrianglePoints = numTriangles * 3;
		hasTriangles = true;
	}
	else {
		numTrianglePoints = 0;
		hasTriangles = false;
	}

	if (inTris) {
		triangles.resize(numTriangles);
		for (ushort t = 0; t < numTriangles; t++)
			triangles[t] = (*inTris)[t];
	}

	numMatchGroups = 0;
}

void NiTriShapeData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	std::vector<int> indexCollapse = GenerateIndexCollapseMap(vertIndices, vertices.size());
	ApplyMapToTriangles(triangles, indexCollapse);
	numTriangles = triangles.size();
	numTrianglePoints = 3 * numTriangles;

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);
}

uint NiTriShapeData::GetNumTriangles() {
	return numTriangles;
}

bool NiTriShapeData::GetTriangles(std::vector<Triangle>& tris) {
	tris = triangles;
	return hasTriangles;
}

void NiTriShapeData::SetTriangles(const std::vector<Triangle>& tris) {
	hasTriangles = true;
	triangles = tris;
	numTriangles = triangles.size();
	numTrianglePoints = numTriangles * 3;
}

void NiTriShapeData::RecalcNormals(const bool smooth, const float smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	CalculateNormals(vertices, triangles, normals, smooth, smoothThresh);
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

		if (i1 >= numVertices || i2 >= numVertices || i3 >= numVertices)
			continue;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[0][i1];
		Vector2 w2 = uvSets[0][i2];
		Vector2 w3 = uvSets[0][i3];

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


NiGeometryData* NiTriShape::GetGeomData() {
	return shapeData;
};

void NiTriShape::SetGeomData(NiGeometryData* geomDataPtr) {
	auto geomData = dynamic_cast<NiTriShapeData*>(geomDataPtr);
	if (geomData)
		shapeData = geomData;
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
	std::vector<int> indexCollapse = GenerateIndexCollapseMap(vertIndices, vertices.size());

	NiTriBasedGeomData::notifyVerticesDelete(vertIndices);

	// This is not a healthy way to delete strip data. Probably need to restrip the shape.
	for (int i = 0; i < numStrips; i++) {
		for (int j = 0; j < stripLengths[i]; j++) {
			if (indexCollapse[points[i][j]] == -1) {
				points[i].erase(points[i].begin() + j);
				stripLengths[i]--;
				--j;
			}
			else
				points[i][j] = indexCollapse[points[i][j]];
		}
	}

	numTriangles = 0;
	for (auto len : stripLengths)
		if (len - 2 > 0)
			numTriangles += len - 2;
}

uint NiTriStripsData::GetNumTriangles() {
	return StripsToTris().size();
}

bool NiTriStripsData::GetTriangles(std::vector<Triangle>& tris) {
	tris = StripsToTris();
	return hasPoints;
}

void NiTriStripsData::SetTriangles(const std::vector<Triangle>&) {
	// Not implemented, stripify here
}

std::vector<Triangle> NiTriStripsData::StripsToTris() {
	return GenerateTrianglesFromStrips(points);
}

void NiTriStripsData::RecalcNormals(const bool smooth, const float smoothThresh) {
	if (!HasNormals())
		return;

	NiTriBasedGeomData::RecalcNormals();

	std::vector<Triangle> tris = StripsToTris();

	CalculateNormals(vertices, tris, normals, smooth, smoothThresh);
}

void NiTriStripsData::CalcTangentSpace() {
	if (!HasNormals() || !HasUVs())
		return;

	NiTriBasedGeomData::CalcTangentSpace();

	std::vector<Vector3> tan1;
	std::vector<Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	std::vector<Triangle> tris = StripsToTris();

	for (int i = 0; i < tris.size(); i++) {
		int i1 = tris[i].p1;
		int i2 = tris[i].p2;
		int i3 = tris[i].p3;

		if (i1 >= numVertices || i2 >= numVertices || i3 >= numVertices)
			continue;

		Vector3 v1 = vertices[i1];
		Vector3 v2 = vertices[i2];
		Vector3 v3 = vertices[i3];

		Vector2 w1 = uvSets[0][i1];
		Vector2 w2 = uvSets[0][i2];
		Vector2 w3 = uvSets[0][i3];

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


NiGeometryData* NiTriStrips::GetGeomData() {
	return stripsData;
};

void NiTriStrips::SetGeomData(NiGeometryData* geomDataPtr) {
	auto geomData = dynamic_cast<NiTriStripsData*>(geomDataPtr);
	if (geomData)
		stripsData = geomData;
}


void NiLinesData::Get(NiStream& stream) {
	NiGeometryData::Get(stream);

	lineFlags.resize(numVertices);
	for (int i = 0; i < numVertices; i++)
		stream >> lineFlags[i];
}

void NiLinesData::Put(NiStream& stream) {
	NiGeometryData::Put(stream);

	for (int i = 0; i < numVertices; i++)
		stream << lineFlags[i];
}

void NiLinesData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	NiGeometryData::notifyVerticesDelete(vertIndices);

	EraseVectorIndices(lineFlags, vertIndices);
}


NiGeometryData* NiLines::GetGeomData() {
	return linesData;
}

void NiLines::SetGeomData(NiGeometryData* geomDataPtr) {
	auto geomData = dynamic_cast<NiLinesData*>(geomDataPtr);
	if (geomData)
		linesData = geomData;
}


void NiScreenElementsData::Get(NiStream& stream) {
	NiTriShapeData::Get(stream);

	stream >> maxPolygons;
	polygons.resize(maxPolygons);
	for (int i = 0; i < maxPolygons; i++)
		stream >> polygons[i];

	polygonIndices.resize(maxPolygons);
	for (int i = 0; i < maxPolygons; i++)
		stream >> polygonIndices[i];

	stream >> unkShort1;
	stream >> numPolygons;
	stream >> usedVertices;
	stream >> unkShort2;
	stream >> usedTrianglePoints;
	stream >> unkShort3;
}

void NiScreenElementsData::Put(NiStream& stream) {
	NiTriShapeData::Put(stream);

	stream << maxPolygons;
	for (int i = 0; i < maxPolygons; i++)
		stream << polygons[i];

	for (int i = 0; i < maxPolygons; i++)
		stream << polygonIndices[i];

	stream << unkShort1;
	stream << numPolygons;
	stream << usedVertices;
	stream << unkShort2;
	stream << usedTrianglePoints;
	stream << unkShort3;
}

void NiScreenElementsData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	NiTriShapeData::notifyVerticesDelete(vertIndices);

	// Clearing as workaround
	maxPolygons = 0;
	polygons.clear();
	polygonIndices.clear();
	numPolygons = 0;
	usedVertices = 0;
	usedTrianglePoints = 0;
}


NiGeometryData* NiScreenElements::GetGeomData() {
	return elemData;
}

void NiScreenElements::SetGeomData(NiGeometryData* geomDataPtr) {
	auto geomData = dynamic_cast<NiScreenElementsData*>(geomDataPtr);
	if (geomData)
		elemData = geomData;
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

NiGeometryData* BSLODTriShape::GetGeomData() {
	return shapeData;
}

void BSLODTriShape::SetGeomData(NiGeometryData* geomDataPtr) {
	auto geomData = dynamic_cast<NiTriShapeData*>(geomDataPtr);
	if (geomData)
		shapeData = geomData;
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
