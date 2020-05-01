/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Skin.h"
#include "utils/half.hpp"
#include "NifUtil.h"

#include <unordered_map>

void NiSkinData::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> skinTransform.rotation;
	stream >> skinTransform.translation;
	stream >> skinTransform.scale;
	stream >> numBones;
	stream >> hasVertWeights;

	if (hasVertWeights > 1)
		hasVertWeights = 1;

	bones.resize(numBones);
	for (int i = 0; i < numBones; i++) {
		BoneData boneData;
		stream >> boneData.boneTransform.rotation;
		stream >> boneData.boneTransform.translation;
		stream >> boneData.boneTransform.scale;
		stream >> boneData.bounds;
		stream >> boneData.numVertices;

		if (hasVertWeights) {
			boneData.vertexWeights.resize(boneData.numVertices);

			for (int j = 0; j < boneData.numVertices; j++) {
				stream >> boneData.vertexWeights[j].index;
				stream >> boneData.vertexWeights[j].weight;
			}
		}
		else
			boneData.numVertices = 0;

		bones[i] = std::move(boneData);
	}
}

void NiSkinData::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << skinTransform.rotation;
	stream << skinTransform.translation;
	stream << skinTransform.scale;
	stream << numBones;
	stream << hasVertWeights;

	for (int i = 0; i < numBones; i++) {
		stream << bones[i].boneTransform.rotation;
		stream << bones[i].boneTransform.translation;
		stream << bones[i].boneTransform.scale;
		stream << bones[i].bounds;

		ushort numVerts = 0;
		if (hasVertWeights)
			numVerts = bones[i].numVertices;

		stream << numVerts;

		for (int j = 0; j < numVerts; j++) {
			stream << bones[i].vertexWeights[j].index;
			stream << bones[i].vertexWeights[j].weight;
		}
	}
}

void NiSkinData::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	ushort highestRemoved = vertIndices.back();
	std::vector<int> indexCollapse = GenerateIndexCollapseMap(vertIndices, highestRemoved + 1);

	NiObject::notifyVerticesDelete(vertIndices);

	ushort ival;
	for (auto &b : bones) {
		for (int i = b.numVertices - 1; i >= 0; i--) {
			ival = b.vertexWeights[i].index;
			if (b.vertexWeights[i].index > highestRemoved) {
				b.vertexWeights[i].index -= vertIndices.size();
			}
			else if (indexCollapse[ival] == -1) {
				b.vertexWeights.erase(b.vertexWeights.begin() + i);
				b.numVertices--;
			}
			else
				b.vertexWeights[i].index = indexCollapse[ival];
		}
	}
}


void NiSkinPartition::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> numPartitions;
	partitions.resize(numPartitions);

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() == 100) {
		bMappedIndices = false;
		stream >> dataSize;
		stream >> vertexSize;
		vertexDesc.Get(stream);

		if (dataSize > 0) {
			numVertices = dataSize / vertexSize;
			vertData.resize(numVertices);

			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				auto& vertex = vertData[i];
				if (HasVertices()) {
					if (IsFullPrecision()) {
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
	}

	for (int p = 0; p < numPartitions; p++) {
		PartitionBlock partition;
		stream >> partition.numVertices;
		stream >> partition.numTriangles;
		stream >> partition.numBones;
		stream >> partition.numStrips;
		stream >> partition.numWeightsPerVertex;

		partition.bones.resize(partition.numBones);
		for (int i = 0; i < partition.numBones; i++)
			stream >> partition.bones[i];

		stream >> partition.hasVertexMap;
		if (partition.hasVertexMap) {
			partition.vertexMap.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				stream >> partition.vertexMap[i];
		}

		stream >> partition.hasVertexWeights;
		if (partition.hasVertexWeights) {
			partition.vertexWeights.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				stream >> partition.vertexWeights[i];
		}

		partition.stripLengths.resize(partition.numStrips);
		for (int i = 0; i < partition.numStrips; i++)
			stream >> partition.stripLengths[i];

		stream >> partition.hasFaces;
		if (partition.hasFaces) {
			partition.strips.resize(partition.numStrips);
			for (int i = 0; i < partition.numStrips; i++) {
				partition.strips[i].resize(partition.stripLengths[i]);
				for (int j = 0; j < partition.stripLengths[i]; j++)
					stream >> partition.strips[i][j];
			}
		}

		if (partition.numStrips == 0 && partition.hasFaces) {
			partition.triangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++)
				stream >> partition.triangles[i];
		}

		stream >> partition.hasBoneIndices;
		if (partition.hasBoneIndices) {
			partition.boneIndices.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				stream >> partition.boneIndices[i];
		}

		if (stream.GetVersion().User() >= 12)
			stream >> partition.unkShort;

		if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() == 100) {
			partition.vertexDesc.Get(stream);

			partition.trueTriangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++)
				stream >> partition.trueTriangles[i];
		}

		partitions[p] = partition;
	}
}

void NiSkinPartition::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << numPartitions;

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() == 100) {
		dataSize = vertexSize * numVertices;

		stream << dataSize;
		stream << vertexSize;
		vertexDesc.Put(stream);

		if (dataSize > 0) {
			half_float::half halfData;
			for (int i = 0; i < numVertices; i++) {
				auto& vertex = vertData[i];
				if (HasVertices()) {
					if (IsFullPrecision()) {
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
	}

	// This call of PrepareVertexMapsAndTriangles should be completely
	// unnecessary.  But it doesn't hurt to be safe.
	PrepareVertexMapsAndTriangles();

	for (int p = 0; p < numPartitions; p++) {
		stream << partitions[p].numVertices;
		stream << partitions[p].numTriangles;
		stream << partitions[p].numBones;
		stream << partitions[p].numStrips;
		stream << partitions[p].numWeightsPerVertex;

		for (int i = 0; i < partitions[p].numBones; i++)
			stream << partitions[p].bones[i];

		stream << partitions[p].hasVertexMap;
		if (partitions[p].hasVertexMap)
			for (int i = 0; i < partitions[p].numVertices; i++)
				stream << partitions[p].vertexMap[i];

		stream << partitions[p].hasVertexWeights;
		if (partitions[p].hasVertexWeights)
			for (int i = 0; i < partitions[p].numVertices; i++)
				stream << partitions[p].vertexWeights[i];

		for (int i = 0; i < partitions[p].numStrips; i++)
			stream << partitions[p].stripLengths[i];

		stream << partitions[p].hasFaces;
		if (partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numStrips; i++)
				for (int j = 0; j < partitions[p].stripLengths[i]; j++)
					stream << partitions[p].strips[i][j];

		if (partitions[p].numStrips == 0 && partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numTriangles; i++)
				stream << partitions[p].triangles[i];

		stream << partitions[p].hasBoneIndices;
		if (partitions[p].hasBoneIndices)
			for (int i = 0; i < partitions[p].numVertices; i++)
				stream << partitions[p].boneIndices[i];

		if (stream.GetVersion().User() >= 12)
			stream << partitions[p].unkShort;

		if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() == 100) {
			partitions[p].vertexDesc.Put(stream);

			for (int i = 0; i < partitions[p].numTriangles; i++)
				stream << partitions[p].trueTriangles[i];
		}
	}
}

void NiSkinPartition::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	if (vertIndices.empty())
		return;

	NiObject::notifyVerticesDelete(vertIndices);

	// Prepare vertexMap and triangles.
	ConvertStripsToTriangles();
	PrepareVertexMapsAndTriangles();
	triParts.clear();

	// Determine maximum vertex index used so we can make a complete
	// collapse map.  (It would be nice if notifyVerticesDelete had a
	// numVertices parameter so we didn't have to calculate this.)
	ushort maxVertInd = 0;
	for (auto &p : partitions) {
		for (auto i : p.vertexMap)
			maxVertInd = std::max(maxVertInd, i);
		if (!bMappedIndices)
			maxVertInd = std::max(maxVertInd, CalcMaxTriangleIndex(p.triangles));
	}

	// Make collapse map for shape vertex indices
	std::vector<int> indexCollapse = GenerateIndexCollapseMap(vertIndices, maxVertInd + 1);

	for (auto &p : partitions) {
		int oldNumVertices = p.vertexMap.size();

		// Make list of deleted vertexMap indices
		std::vector<int> vertexMapDelList;
		for (int i = 0; i < p.vertexMap.size(); i++)
			if (indexCollapse[p.vertexMap[i]] == -1)
				vertexMapDelList.push_back(i);

		// Erase indices of vertexMap, vertexWeights, and boneIndices
		EraseVectorIndices(p.vertexMap, vertexMapDelList);
		if (p.hasVertexWeights)
			EraseVectorIndices(p.vertexWeights, vertexMapDelList);
		if (p.hasBoneIndices)
			EraseVectorIndices(p.boneIndices, vertexMapDelList);
		p.numVertices = p.vertexMap.size();

		// Compose vertexMap with indexCollapse to get new vertexMap
		for (int i = 0; i < p.vertexMap.size(); i++)
			p.vertexMap[i] = indexCollapse[p.vertexMap[i]];

		if (!bMappedIndices) {
			// Apply shape vertex index collapse map to true triangles
			ApplyMapToTriangles(p.triangles, indexCollapse);
			p.trueTriangles = p.triangles;
		}
		else {
			// Generate collapse map for indices into (old) vertexMap.
			std::vector<int> mapCollapse = GenerateIndexCollapseMap(vertexMapDelList, oldNumVertices);
			// Apply vertexMap index collapse to mapped triangles
			ApplyMapToTriangles(p.triangles, mapCollapse);
			p.trueTriangles.clear();
		}
		p.numTriangles = p.triangles.size();
	}

	if (!vertData.empty()) {
		EraseVectorIndices(vertData, vertIndices);
		numVertices = vertData.size();
	}
}

void NiSkinPartition::DeletePartitions(const std::vector<int> &partInds) {
	if (partInds.empty())
		return;
	if (!triParts.empty()) {
		std::vector<int> piMap = GenerateIndexCollapseMap(partInds, numPartitions);
		for (int &pi : triParts) {
			if (pi >= 0 && pi < piMap.size())
				pi = piMap[pi];
		}
	}
	EraseVectorIndices(partitions, partInds);
	numPartitions = partitions.size();
}

int NiSkinPartition::RemoveEmptyPartitions(std::vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = 0; i < partitions.size(); ++i)
		if (partitions[i].numTriangles == 0)
			outDeletedIndices.push_back(i);
	if (!outDeletedIndices.empty())
		DeletePartitions(outDeletedIndices);
	return outDeletedIndices.size();
}

bool NiSkinPartition::PartitionBlock::ConvertStripsToTriangles() {
	if (numStrips == 0)
		return false;
	hasFaces = true;
	triangles = GenerateTrianglesFromStrips(strips);
	numTriangles = triangles.size();
	numStrips = 0;
	strips.clear();
	stripLengths.clear();
	trueTriangles.clear();
	return true;
}

bool NiSkinPartition::ConvertStripsToTriangles() {
	bool triangulated = false;
	for (PartitionBlock &p : partitions) {
		if (p.ConvertStripsToTriangles())
			triangulated = true;
	}
	return triangulated;
}

void NiSkinPartition::PartitionBlock::GenerateTrueTrianglesFromMappedTriangles() {
	if (vertexMap.empty() || triangles.empty()) {
		trueTriangles.clear();
		if (numStrips == 0)
			numTriangles = 0;
		return;
	}

	trueTriangles = triangles;
	ApplyMapToTriangles(trueTriangles, vertexMap);

	for (Triangle &t : trueTriangles)
		t.rot();

	if (triangles.size() != trueTriangles.size()) {
		triangles.clear();
		numTriangles = trueTriangles.size();
	}
}

void NiSkinPartition::PartitionBlock::GenerateMappedTrianglesFromTrueTrianglesAndVertexMap() {
	if (vertexMap.empty() || trueTriangles.empty()) {
		triangles.clear();
		if (numStrips == 0)
			numTriangles = 0;
		return;
	}

	std::vector<ushort> invmap(vertexMap.back() + 1);
	for (unsigned int mi = 0; mi < vertexMap.size(); ++mi) {
		if (vertexMap[mi] >= invmap.size())
			invmap.resize(vertexMap[mi] + 1);

		invmap[vertexMap[mi]] = mi;
	}

	triangles = trueTriangles;
	ApplyMapToTriangles(triangles, invmap);

	for (Triangle &t : triangles)
		t.rot();

	if (triangles.size() != trueTriangles.size()) {
		trueTriangles.clear();
		numTriangles = triangles.size();
	}
}

void NiSkinPartition::PartitionBlock::GenerateVertexMapFromTrueTriangles() {
	std::vector<bool> vertUsed(CalcMaxTriangleIndex(trueTriangles) + 1, false);
	for (unsigned int i = 0; i < trueTriangles.size(); ++i) {
		vertUsed[trueTriangles[i].p1] = true;
		vertUsed[trueTriangles[i].p2] = true;
		vertUsed[trueTriangles[i].p3] = true;
	}

	vertexMap.clear();

	for (unsigned int i = 0; i < vertUsed.size(); ++i) {
		if (vertUsed[i])
			vertexMap.push_back(i);
	}

	numVertices = vertexMap.size();
}

void NiSkinPartition::PrepareTrueTriangles() {
	for (PartitionBlock &p : partitions) {
		if (!p.trueTriangles.empty())
			continue;

		if (p.numStrips)
			p.ConvertStripsToTriangles();

		if (bMappedIndices)
			p.GenerateTrueTrianglesFromMappedTriangles();
		else
			p.trueTriangles = p.triangles;
	}
}

void NiSkinPartition::PrepareVertexMapsAndTriangles() {
	for (PartitionBlock &p : partitions) {
		if (p.vertexMap.empty())
			p.GenerateVertexMapFromTrueTriangles();

		if (p.triangles.empty()) {
			if (bMappedIndices)
				p.GenerateMappedTrianglesFromTrueTrianglesAndVertexMap();
			else
				p.triangles = p.trueTriangles;
		}
	}
}

void NiSkinPartition::GenerateTriPartsFromTrueTriangles(const std::vector<Triangle> &shapeTris) {
	triParts.clear();
	triParts.resize(shapeTris.size());

	// Make a map from Triangles to their indices in shapeTris
	std::unordered_map<Triangle, int> shapeTriInds;
	for (int triInd = 0; triInd < shapeTris.size(); ++triInd) {
		Triangle t = shapeTris[triInd];
		t.rot();
		shapeTriInds[t] = triInd;
	}

	// Set triParts for each partition triangle
	for (int partInd = 0; partInd < partitions.size(); ++partInd) {
		for (const Triangle &pt : partitions[partInd].trueTriangles) {
			Triangle t = pt;
			t.rot();
			auto it = shapeTriInds.find(t);
			if (it != shapeTriInds.end())
				triParts[it->second] = partInd;
		}
	}
}

void NiSkinPartition::GenerateTrueTrianglesFromTriParts(const std::vector<Triangle> &shapeTris) {
	if (shapeTris.size() != triParts.size())
		return;
	for (PartitionBlock &p : partitions) {
		p.trueTriangles.clear();
		p.triangles.clear();
		p.numStrips = 0;
		p.strips.clear();
		p.stripLengths.clear();
		p.hasFaces = true;
		p.vertexMap.clear();
		p.vertexWeights.clear();
		p.boneIndices.clear();
	}
	for (int triInd = 0; triInd < shapeTris.size(); ++triInd) {
		int partInd = triParts[triInd];
		if (partInd >= 0 && partInd < partitions.size())
			partitions[partInd].trueTriangles.push_back(shapeTris[triInd]);
	}
	for (PartitionBlock &p : partitions)
		p.numTriangles = p.trueTriangles.size();
}

void NiSkinPartition::PrepareTriParts(const std::vector<Triangle> &shapeTris) {
	if (shapeTris.size() == triParts.size())
		return;
	PrepareTrueTriangles();
	GenerateTriPartsFromTrueTriangles(shapeTris);
}


BlockRefArray<NiNode>& NiBoneContainer::GetBones() {
	return boneRefs;
}


void NiSkinInstance::Get(NiStream& stream) {
	NiObject::Get(stream);

	dataRef.Get(stream);
	skinPartitionRef.Get(stream);
	targetRef.Get(stream);
	boneRefs.Get(stream);
}

void NiSkinInstance::Put(NiStream& stream) {
	NiObject::Put(stream);

	dataRef.Put(stream);
	skinPartitionRef.Put(stream);
	targetRef.Put(stream);
	boneRefs.Put(stream);
}

void NiSkinInstance::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef);
	refs.insert(&skinPartitionRef);
}

void NiSkinInstance::GetPtrs(std::set<Ref*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef);
	boneRefs.GetIndexPtrs(ptrs);
}


void BSDismemberSkinInstance::Get(NiStream& stream) {
	NiSkinInstance::Get(stream);

	stream >> numPartitions;
	partitions.resize(numPartitions);

	for (int i = 0; i < numPartitions; i++)
		stream >> partitions[i];
}

void BSDismemberSkinInstance::Put(NiStream& stream) {
	NiSkinInstance::Put(stream);

	stream << numPartitions;
	for (int i = 0; i < numPartitions; i++)
		stream << partitions[i];
}

void BSDismemberSkinInstance::AddPartition(const BSDismemberSkinInstance::PartitionInfo& part) {
	partitions.push_back(part);
	numPartitions++;
}

void BSDismemberSkinInstance::DeletePartitions(const std::vector<int> &partInds) {
	if (partInds.empty())
		return;
	EraseVectorIndices(partitions, partInds);
	numPartitions = partitions.size();
}

void BSDismemberSkinInstance::RemovePartition(const int id) {
	if (id >= 0 && id < numPartitions) {
		partitions.erase(partitions.begin() + id);
		numPartitions--;
	}
}

void BSDismemberSkinInstance::ClearPartitions() {
	partitions.clear();
	numPartitions = 0;
}


void BSSkinBoneData::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> nBones;
	boneXforms.resize(nBones);
	for (int i = 0; i < nBones; i++) {
		stream >> boneXforms[i].bounds;
		stream >> boneXforms[i].boneTransform.rotation;
		stream >> boneXforms[i].boneTransform.translation;
		stream >> boneXforms[i].boneTransform.scale;
	}
}

void BSSkinBoneData::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << nBones;
	for (int i = 0; i < nBones; i++) {
		stream << boneXforms[i].bounds;
		stream << boneXforms[i].boneTransform.rotation;
		stream << boneXforms[i].boneTransform.translation;
		stream << boneXforms[i].boneTransform.scale;
	}
}


void BSSkinInstance::Get(NiStream& stream) {
	NiObject::Get(stream);

	targetRef.Get(stream);
	dataRef.Get(stream);
	boneRefs.Get(stream);

	stream >> numScales;
	scales.resize(numScales);
	for (int i = 0; i < numScales; i++)
		stream >> scales[i];
}

void BSSkinInstance::Put(NiStream& stream) {
	NiObject::Put(stream);

	targetRef.Put(stream);
	dataRef.Put(stream);
	boneRefs.Put(stream);

	stream << numScales;
	for (int i = 0; i < numScales; i++)
		stream << scales[i];
}

void BSSkinInstance::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef);
}

void BSSkinInstance::GetPtrs(std::set<Ref*>& ptrs) {
	NiObject::GetPtrs(ptrs);

	ptrs.insert(&targetRef);
	boneRefs.GetIndexPtrs(ptrs);
}
