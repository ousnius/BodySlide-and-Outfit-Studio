/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Skin.h"
#include "utils/half.hpp"

NiSkinData::NiSkinData(NiHeader* hdr) {
	NiObject::Init(hdr);

	skinTransform.scale = 1.0f;
	numBones = 0;
	hasVertWeights = 1;
}

NiSkinData::NiSkinData(std::fstream& file, NiHeader* hdr) : NiSkinData(hdr) {
	Get(file);
}

void NiSkinData::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&skinTransform, 52);
	file.read((char*)&numBones, 4);
	file.read((char*)&hasVertWeights, 1);

	bones.resize(numBones);
	for (int i = 0; i < numBones; i++) {
		BoneData boneData;
		file.read((char*)&boneData.boneTransform, 52);
		file.read((char*)&boneData.bounds, 16);
		file.read((char*)&boneData.numVertices, 2);

		if (hasVertWeights) {
			boneData.vertexWeights.resize(boneData.numVertices);

			for (int j = 0; j < boneData.numVertices; j++) {
				SkinWeight weight;
				file.read((char*)&weight.index, 2);
				file.read((char*)&weight.weight, 4);
				boneData.vertexWeights[j] = std::move(weight);
			}
		}
		else
			boneData.numVertices = 0;

		bones[i] = std::move(boneData);
	}
}

void NiSkinData::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&skinTransform, 52);
	file.write((char*)&numBones, 4);
	file.write((char*)&hasVertWeights, 1);

	for (int i = 0; i < numBones; i++) {
		file.write((char*)&bones[i].boneTransform, 52);
		file.write((char*)&bones[i].bounds, 16);

		if (hasVertWeights) {
			file.write((char*)&bones[i].numVertices, 2);

			for (int j = 0; j < bones[i].numVertices; j++) {
				file.write((char*)&bones[i].vertexWeights[j].index, 2);
				file.write((char*)&bones[i].vertexWeights[j].weight, 4);
			}
		}
		else {
			ushort numVerts = 0;
			file.write((char*)&numVerts, 2);
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

int NiSkinData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 57;

	for (auto &b : bones) {
		blockSize += 70;

		if (hasVertWeights)
			blockSize += b.numVertices * 6;
	}

	return blockSize;
}


NiSkinPartition::NiSkinPartition(NiHeader* hdr) {
	NiObject::Init(hdr);

	numPartitions = 0;
	dataSize = 0;
	vertexSize = 0;
	vertFlags1 = 0;
	vertFlags2 = 0;
	vertFlags3 = 0;
	vertFlags4 = 0;
	vertFlags5 = 0;
	vertFlags6 = 0;
	vertFlags7 = 0;
	vertFlags8 = 0;
	numVertices = 0;
}

NiSkinPartition::NiSkinPartition(std::fstream& file, NiHeader* hdr) : NiSkinPartition(hdr) {
	Get(file);
}

void NiSkinPartition::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&numPartitions, 4);
	partitions.resize(numPartitions);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		file.read((char*)&dataSize, 4);
		file.read((char*)&vertexSize, 4);
		file.read((char*)&vertFlags1, 1);
		file.read((char*)&vertFlags2, 1);
		file.read((char*)&vertFlags3, 1);
		file.read((char*)&vertFlags4, 1);
		file.read((char*)&vertFlags5, 1);
		file.read((char*)&vertFlags6, 1);
		file.read((char*)&vertFlags7, 1);
		file.read((char*)&vertFlags8, 1);

		if (dataSize > 0) {
			numVertices = dataSize / vertexSize;
			vertData.resize(numVertices);

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
	}

	for (int p = 0; p < numPartitions; p++) {
		PartitionBlock partition;
		file.read((char*)&partition.numVertices, 2);
		file.read((char*)&partition.numTriangles, 2);
		file.read((char*)&partition.numBones, 2);
		file.read((char*)&partition.numStrips, 2);
		file.read((char*)&partition.numWeightsPerVertex, 2);

		partition.bones.resize(partition.numBones);
		for (int i = 0; i < partition.numBones; i++)
			file.read((char*)&partition.bones[i], 2);

		file.read((char*)&partition.hasVertexMap, 1);
		if (partition.hasVertexMap) {
			partition.vertexMap.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.vertexMap[i], 2);
		}

		file.read((char*)&partition.hasVertexWeights, 1);
		if (partition.hasVertexWeights) {
			partition.vertexWeights.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.vertexWeights[i], 16);
		}

		partition.stripLengths.resize(partition.numStrips);
		for (int i = 0; i < partition.numStrips; i++)
			file.read((char*)&partition.stripLengths[i], 2);

		file.read((char*)&partition.hasFaces, 1);
		if (partition.hasFaces) {
			partition.strips.resize(partition.numStrips);
			for (int i = 0; i < partition.numStrips; i++) {
				partition.strips[i].resize(partition.stripLengths[i]);
				for (int j = 0; j < partition.stripLengths[i]; j++)
					file.read((char*)&partition.strips[i][j], 2);
			}
		}

		if (partition.numStrips == 0 && partition.hasFaces) {
			partition.triangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++)
				file.read((char*)&partition.triangles[i], 6);
		}

		file.read((char*)&partition.hasBoneIndices, 1);
		if (partition.hasBoneIndices) {
			partition.boneIndices.resize(partition.numVertices);
			for (int i = 0; i < partition.numVertices; i++)
				file.read((char*)&partition.boneIndices[i], 4);
		}

		if (header->GetUserVersion() >= 12)
			file.read((char*)&partition.unkShort, 2);

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			file.read((char*)&partition.vertFlags1, 1);
			file.read((char*)&partition.vertFlags2, 1);
			file.read((char*)&partition.vertFlags3, 1);
			file.read((char*)&partition.vertFlags4, 1);
			file.read((char*)&partition.vertFlags5, 1);
			file.read((char*)&partition.vertFlags6, 1);
			file.read((char*)&partition.vertFlags7, 1);
			file.read((char*)&partition.vertFlags8, 1);

			partition.trueTriangles.resize(partition.numTriangles);
			for (int i = 0; i < partition.numTriangles; i++) {
				file.read((char*)&partition.trueTriangles[i].p1, 2);
				file.read((char*)&partition.trueTriangles[i].p2, 2);
				file.read((char*)&partition.trueTriangles[i].p3, 2);
			}
		}

		partitions[p] = partition;
	}
}

void NiSkinPartition::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&numPartitions, 4);

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		file.write((char*)&dataSize, 4);
		file.write((char*)&vertexSize, 4);
		file.write((char*)&vertFlags1, 1);
		file.write((char*)&vertFlags2, 1);
		file.write((char*)&vertFlags3, 1);
		file.write((char*)&vertFlags4, 1);
		file.write((char*)&vertFlags5, 1);
		file.write((char*)&vertFlags6, 1);
		file.write((char*)&vertFlags7, 1);
		file.write((char*)&vertFlags8, 1);

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
	}

	for (int p = 0; p < numPartitions; p++) {
		file.write((char*)&partitions[p].numVertices, 2);
		file.write((char*)&partitions[p].numTriangles, 2);
		file.write((char*)&partitions[p].numBones, 2);
		file.write((char*)&partitions[p].numStrips, 2);
		file.write((char*)&partitions[p].numWeightsPerVertex, 2);

		for (int i = 0; i < partitions[p].numBones; i++)
			file.write((char*)&partitions[p].bones[i], 2);

		file.write((char*)&partitions[p].hasVertexMap, 1);
		if (partitions[p].hasVertexMap)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].vertexMap[i], 2);

		file.write((char*)&partitions[p].hasVertexWeights, 1);
		if (partitions[p].hasVertexWeights)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].vertexWeights[i], 16);

		for (int i = 0; i < partitions[p].numStrips; i++)
			file.write((char*)&partitions[p].stripLengths[i], 2);

		file.write((char*)&partitions[p].hasFaces, 1);
		if (partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numStrips; i++)
				for (int j = 0; j < partitions[p].stripLengths[i]; j++)
					file.write((char*)&partitions[p].strips[i][j], 2);

		if (partitions[p].numStrips == 0 && partitions[p].hasFaces)
			for (int i = 0; i < partitions[p].numTriangles; i++)
				file.write((char*)&partitions[p].triangles[i].p1, 6);

		file.write((char*)&partitions[p].hasBoneIndices, 1);
		if (partitions[p].hasBoneIndices)
			for (int i = 0; i < partitions[p].numVertices; i++)
				file.write((char*)&partitions[p].boneIndices[i], 4);

		if (header->GetUserVersion() >= 12)
			file.write((char*)&partitions[p].unkShort, 2);

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			file.write((char*)&partitions[p].vertFlags1, 1);
			file.write((char*)&partitions[p].vertFlags2, 1);
			file.write((char*)&partitions[p].vertFlags3, 1);
			file.write((char*)&partitions[p].vertFlags4, 1);
			file.write((char*)&partitions[p].vertFlags5, 1);
			file.write((char*)&partitions[p].vertFlags6, 1);
			file.write((char*)&partitions[p].vertFlags7, 1);
			file.write((char*)&partitions[p].vertFlags8, 1);

			if (partitions[p].trueTriangles.size() != partitions[p].numTriangles)
				partitions[p].trueTriangles = partitions[p].triangles;

			for (int i = 0; i < partitions[p].numTriangles; i++) {
				file.write((char*)&partitions[p].trueTriangles[i].p1, 2);
				file.write((char*)&partitions[p].trueTriangles[i].p2, 2);
				file.write((char*)&partitions[p].trueTriangles[i].p3, 2);
			}
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

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
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
		else {
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

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
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

int NiSkinPartition::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
		blockSize += 16;

		dataSize = vertexSize * numVertices;
		blockSize += dataSize;
	}

	for (auto &p : partitions) {
		if (header->GetUserVersion() >= 12)				// Plain data size
			blockSize += 16;
		else
			blockSize += 14;

		blockSize += 2 * p.numBones;				// Bone list size
		blockSize += 2 * p.numVertices;				// Vertex map size
		blockSize += 4 * p.numVertices;				// Bone index size
		blockSize += 16 * p.numVertices;			// Vertex weights size
		blockSize += 2 * p.numStrips;				// Strip lengths size
		for (int i = 0; i < p.numStrips; i++)
			blockSize += 2 * p.stripLengths[i];		// Strip list size

		if (p.numStrips == 0)
			blockSize += 6 * p.numTriangles;		// Triangle list size

		if (header->GetUserVersion() >= 12 && header->GetUserVersion2() == 100) {
			blockSize += 8;
			blockSize += p.numTriangles * 6;
		}
	}

	return blockSize;
}


NiSkinInstance::NiSkinInstance(NiHeader* hdr) {
	NiObject::Init(hdr);
}

NiSkinInstance::NiSkinInstance(std::fstream& file, NiHeader* hdr) : NiSkinInstance(hdr) {
	Get(file);
}

void NiSkinInstance::Get(std::fstream& file) {
	NiObject::Get(file);

	dataRef.Get(file);
	skinPartitionRef.Get(file);
	targetRef.Get(file);
	boneRefs.Get(file);
}

void NiSkinInstance::Put(std::fstream& file) {
	NiObject::Put(file);

	dataRef.Put(file);
	skinPartitionRef.Put(file);
	targetRef.Put(file);
	boneRefs.Put(file);
}

void NiSkinInstance::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
	refs.insert(&skinPartitionRef.index);
	boneRefs.GetIndexPtrs(refs);
}

int NiSkinInstance::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 16;
	blockSize += boneRefs.GetSize() * 4;
	return blockSize;
}


BSDismemberSkinInstance::BSDismemberSkinInstance(NiHeader* hdr) : NiSkinInstance(hdr) {
}

BSDismemberSkinInstance::BSDismemberSkinInstance(std::fstream& file, NiHeader* hdr) : BSDismemberSkinInstance(hdr) {
	Get(file);
}

void BSDismemberSkinInstance::Get(std::fstream& file) {
	NiSkinInstance::Get(file);

	file.read((char*)&numPartitions, 4);
	partitions.resize(numPartitions);

	for (int i = 0; i < numPartitions; i++) {
		PartitionInfo part;
		file.read((char*)&part.flags, 2);
		file.read((char*)&part.partID, 2);
		partitions[i] = part;
	}
}

void BSDismemberSkinInstance::Put(std::fstream& file) {
	NiSkinInstance::Put(file);

	file.write((char*)&numPartitions, 4);
	for (int i = 0; i < numPartitions; i++) {
		file.write((char*)&partitions[i].flags, 2);
		file.write((char*)&partitions[i].partID, 2);
	}
}

int BSDismemberSkinInstance::CalcBlockSize() {
	NiSkinInstance::CalcBlockSize();

	blockSize += 4;
	blockSize += numPartitions * 4;
	return blockSize;
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


BSSkinBoneData::BSSkinBoneData(NiHeader* hdr) {
	NiObject::Init(hdr);

	nBones = 0;
}

BSSkinBoneData::BSSkinBoneData(std::fstream& file, NiHeader* hdr) : BSSkinBoneData(hdr) {
	Get(file);
}

void BSSkinBoneData::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&nBones, 4);
	boneXforms.resize(nBones);
	for (int i = 0; i < nBones; i++) {
		file.read((char*)&boneXforms[i].bounds, 16);
		file.read((char*)&boneXforms[i].boneTransform, 52);
	}
}

void BSSkinBoneData::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&nBones, 4);

	for (int i = 0; i < nBones; i++) {
		file.write((char*)&boneXforms[i].bounds, 16);
		file.write((char*)&boneXforms[i].boneTransform, 52);
	}
}

int BSSkinBoneData::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;
	blockSize += 68 * nBones;

	return blockSize;
}


BSSkinInstance::BSSkinInstance(NiHeader* hdr) {
	NiObject::Init(hdr);
}

BSSkinInstance::BSSkinInstance(std::fstream& file, NiHeader* hdr) : BSSkinInstance(hdr) {
	Get(file);
}

void BSSkinInstance::Get(std::fstream& file) {
	NiObject::Get(file);

	targetRef.Get(file);
	dataRef.Get(file);
	boneRefs.Get(file);

	file.read((char*)&numUnk, 4);
	unk.resize(numUnk);
	for (int i = 0; i < numUnk; i++)
		file.read((char*)&unk[i], 12);
}

void BSSkinInstance::Put(std::fstream& file) {
	NiObject::Put(file);

	targetRef.Put(file);
	dataRef.Put(file);
	boneRefs.Put(file);

	file.write((char*)&numUnk, 4);
	for (int i = 0; i < numUnk; i++)
		file.write((char*)&unk[i], 12);
}

void BSSkinInstance::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
	boneRefs.GetIndexPtrs(refs);
}

int BSSkinInstance::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 16;
	blockSize += boneRefs.GetSize() * 4;
	blockSize += numUnk * 12;

	return blockSize;
}
