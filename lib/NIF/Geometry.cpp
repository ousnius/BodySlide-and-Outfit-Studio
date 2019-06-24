/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Geometry.h"
#include "Skin.h"
#include "Nodes.h"

#include "utils/KDMatcher.h"

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

void NiGeometryData::Create(std::vector<Vector3>* verts, std::vector<Triangle>*, std::vector<Vector2>* texcoords, std::vector<Vector3>* norms) {
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
	numUVSets = 0;

	if (texcoords) {
		size_t uvCount = texcoords->size();
		if (uvCount == numVertices) {
			uvSets.resize(1);
			uvSets[0].resize(uvCount);
			for (size_t uv = 0; uv < uvSets[0].size(); uv++)
				uvSets[0][uv] = (*texcoords)[uv];

			if (uvCount > 0)
				numUVSets = 4097;
		}
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

			if (hasUV) {
				for (int j = 0; j < uvSets.size(); j++)
					uvSets[j].erase(uvSets[j].begin() + i);
			}
		}
	}
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

void BSTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	flags = 14;

	ushort maxVertIndex = std::numeric_limits<ushort>().max();
	size_t vertCount = verts->size();
	if (vertCount > maxVertIndex)
		numVertices = 0;
	else
		numVertices = ushort(vertCount);

	uint maxTriIndex = std::numeric_limits<uint>().max();
	size_t triCount = tris->size();
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
				stream << dataRecord.segmentUser;
				stream << dataRecord.unkInt2;
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

void BSSubIndexTriShape::Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals) {
	BSTriShape::Create(verts, tris, uvs, normals);

	// Skinned most of the time
	SetSkinned(true);
	SetDefaultSegments();
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

void NiTriBasedGeomData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords, std::vector<Vector3>* norms) {
	NiGeometryData::Create(verts, inTris, texcoords, norms);

	if (inTris) {
		ushort maxIndex = std::numeric_limits<ushort>().max();
		size_t triCount = inTris->size();
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

void NiTriShapeData::Create(std::vector<Vector3>* verts, std::vector<Triangle>* inTris, std::vector<Vector2>* texcoords, std::vector<Vector3>* norms) {
	NiTriBasedGeomData::Create(verts, inTris, texcoords, norms);

	if (numTriangles > 0) {
		numTrianglePoints = numTriangles * 3;
		hasTriangles = true;
	}
	else {
		numTrianglePoints = 0;
		hasTriangles = false;
	}

	triangles.resize(numTriangles);
	for (ushort t = 0; t < numTriangles; t++)
		triangles[t] = (*inTris)[t];

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

	numTriangles = 0;
	for (auto len : stripLengths)
		if (len - 2 > 0)
			numTriangles += len - 2;
}

uint NiTriStripsData::GetNumTriangles() {
	std::vector<Triangle> tris;
	StripsToTris(&tris);
	return tris.size();
}

bool NiTriStripsData::GetTriangles(std::vector<Triangle>& tris) {
	StripsToTris(&tris);
	return hasPoints;
}

void NiTriStripsData::SetTriangles(const std::vector<Triangle>&) {
	// Not implemented, stripify here
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

	std::vector<int> indexCollapse(lineFlags.size(), 0);

	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {	// Found one to remove
			indexCollapse[i] = -1;	// Flag delete
			j++;
		}
	}

	for (int i = lineFlags.size() - 1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			lineFlags.erase(lineFlags.begin() + i);
		}
	}
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
