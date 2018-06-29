/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "Skin.h"
#include "utils/half.hpp"

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
	std::vector<int> indexCollapse(highestRemoved + 1, 0);

	NiObject::notifyVerticesDelete(vertIndices);

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

	ushort ival;
	for (auto &b : bones) {
		for (int i = b.numVertices - 1; i >= 0; i--) {
			ival = b.vertexWeights[i].index;
			if (b.vertexWeights[i].index > highestRemoved) {
				b.vertexWeights[i].index -= remCount;
			}
			else if (indexCollapse[ival] == -1) {
				b.vertexWeights.erase(b.vertexWeights.begin() + i);
				b.numVertices--;
			}
			else
				b.vertexWeights[i].index -= indexCollapse[ival];
		}
	}
}


void NiSkinPartition::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> numPartitions;
	partitions.resize(numPartitions);

	if (stream.GetVersion().User() >= 12 && stream.GetVersion().Stream() == 100) {
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

			if (partitions[p].trueTriangles.size() != partitions[p].numTriangles)
				partitions[p].trueTriangles = partitions[p].triangles;

			for (int i = 0; i < partitions[p].numTriangles; i++)
				stream << partitions[p].trueTriangles[i];
		}
	}
}

void NiSkinPartition::notifyVerticesDelete(const std::vector<ushort>& vertIndices) {
	if (vertIndices.empty())
		return;

	ushort highestRemoved = vertIndices.back();
	std::vector<int> indexCollapse(highestRemoved + 1, 0);

	NiObject::notifyVerticesDelete(vertIndices);

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

	size_t maxVertexMapSz = 0;
	for (auto &p : partitions)
		if (p.vertexMap.size() > maxVertexMapSz)
			maxVertexMapSz = p.vertexMap.size();

	std::vector<int> mapCollapse(maxVertexMapSz, 0);
	int mapRemCount = 0;
	int mapHighestRemoved = maxVertexMapSz;

	ushort index;
	for (auto &p : partitions) {
		mapRemCount = 0;
		mapHighestRemoved = maxVertexMapSz;
		for (int i = 0; i < p.vertexMap.size(); i++) {
			if (p.vertexMap[i] <= highestRemoved && indexCollapse[p.vertexMap[i]] == -1) {
				mapRemCount++;
				mapCollapse[i] = -1;
				mapHighestRemoved = i;
			}
			else
				mapCollapse[i] = mapRemCount;
		}

		for (int i = p.vertexMap.size() - 1; i >= 0; i--) {
			index = p.vertexMap[i];
			if (index > highestRemoved) {
				p.vertexMap[i] -= remCount;
			}
			else if (indexCollapse[index] == -1) {
				p.vertexMap.erase(p.vertexMap.begin() + i);
				p.numVertices--;
				if (p.hasVertexWeights)
					p.vertexWeights.erase(p.vertexWeights.begin() + i);
				if (p.hasBoneIndices)
					p.boneIndices.erase(p.boneIndices.begin() + i);
			}
			else
				p.vertexMap[i] -= indexCollapse[index];
		}

		if (!p.trueTriangles.empty()) {
			for (int i = p.numTriangles - 1; i >= 0; i--) {
				if (p.triangles[i].p1 > highestRemoved) {
					p.triangles[i].p1 -= remCount;
				}
				else if (indexCollapse[p.triangles[i].p1] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p1 -= indexCollapse[p.triangles[i].p1];

				if (p.triangles[i].p2 > highestRemoved) {
					p.triangles[i].p2 -= remCount;
				}
				else if (indexCollapse[p.triangles[i].p2] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p2 -= indexCollapse[p.triangles[i].p2];

				if (p.triangles[i].p3 > highestRemoved) {
					p.triangles[i].p3 -= remCount;
				}
				else if (indexCollapse[p.triangles[i].p3] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p3 -= indexCollapse[p.triangles[i].p3];
			}

			p.trueTriangles = p.triangles;
		}
		else if (p.numStrips == 0 && p.hasFaces) {
			for (int i = p.numTriangles - 1; i >= 0; i--) {
				if (p.triangles[i].p1 > mapHighestRemoved) {
					p.triangles[i].p1 -= mapRemCount;
				}
				else if (mapCollapse[p.triangles[i].p1] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p1 -= mapCollapse[p.triangles[i].p1];

				if (p.triangles[i].p2 > mapHighestRemoved) {
					p.triangles[i].p2 -= mapRemCount;
				}
				else if (mapCollapse[p.triangles[i].p2] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p2 -= mapCollapse[p.triangles[i].p2];

				if (p.triangles[i].p3 > mapHighestRemoved) {
					p.triangles[i].p3 -= mapRemCount;
				}
				else if (mapCollapse[p.triangles[i].p3] == -1) {
					p.triangles.erase(p.triangles.begin() + i);
					p.numTriangles--;
					continue;
				}
				else
					p.triangles[i].p3 -= mapCollapse[p.triangles[i].p3];
			}
		}
	}

	if (!vertData.empty()) {
		auto vertBegin = &vertData.front();
		auto pos = remove_if(vertData.begin(), vertData.end(), [&vertBegin, &vertIndices](BSVertexData& v) {
			auto i = std::distance(vertBegin, &v);
			return (std::find(vertIndices.begin(), vertIndices.end(), i) != vertIndices.end());
		});

		vertData.erase(pos, vertData.end());
		numVertices = vertData.size();
	}
}

int NiSkinPartition::RemoveEmptyPartitions(std::vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = partitions.size() - 1; i >= 0; i--) {
		if (partitions[i].numVertices == 0) {
			outDeletedIndices.push_back(i);
			partitions.erase(partitions.begin() + i);
			numPartitions--;
		}
	}
	return outDeletedIndices.size();
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
