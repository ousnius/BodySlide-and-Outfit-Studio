/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Objects.h"
#include "VertexData.h"

#include <deque>

struct AdditionalDataInfo {
	int dataType = 0;
	uint numChannelBytesPerElement = 0;
	uint numChannelBytes = 0;
	uint numTotalBytesPerElement = 0;
	uint blockIndex = 0;
	uint channelOffset = 0;
	byte unkByte1 = 2;

	void Get(NiStream& stream) {
		stream >> dataType;
		stream >> numChannelBytesPerElement;
		stream >> numChannelBytes;
		stream >> numTotalBytesPerElement;
		stream >> blockIndex;
		stream >> channelOffset;
		stream >> unkByte1;
	}

	void Put(NiStream& stream) {
		stream << dataType;
		stream << numChannelBytesPerElement;
		stream << numChannelBytes;
		stream << numTotalBytesPerElement;
		stream << blockIndex;
		stream << channelOffset;
		stream << unkByte1;
	}
};

struct AdditionalDataBlock {
	bool hasData = false;
	uint blockSize = 0;

	uint numBlocks = 0;
	std::vector<uint> blockOffsets;

	uint numData = 0;
	std::vector<uint> dataSizes;
	std::vector<std::vector<byte>> data;

	void Get(NiStream& stream) {
		stream >> hasData;

		if (hasData) {
			stream >> blockSize;

			stream >> numBlocks;
			blockOffsets.resize(numBlocks);
			for (int i = 0; i < numBlocks; i++)
				stream >> blockOffsets[i];

			stream >> numData;
			dataSizes.resize(numData);
			for (int i = 0; i < numData; i++)
				stream >> dataSizes[i];

			data.resize(numData);
			for (int i = 0; i < numData; i++) {
				data[i].resize(blockSize);
				for (int j = 0; j < blockSize; j++)
					stream >> data[i][j];
			}
		}
	}

	void Put(NiStream& stream) {
		stream << hasData;

		if (hasData) {
			stream << blockSize;

			stream << numBlocks;
			for (int i = 0; i < numBlocks; i++)
				stream << blockOffsets[i];

			stream << numData;
			for (int i = 0; i < numData; i++)
				stream << dataSizes[i];

			for (int i = 0; i < numData; i++)
				for (int j = 0; j < blockSize; j++)
					stream << data[i][j];
		}
	}
};

class AdditionalGeomData : public NiObject {
};

class NiAdditionalGeometryData : public AdditionalGeomData {
private:
	ushort numVertices = 0;

	uint numBlockInfos = 0;
	std::vector<AdditionalDataInfo> blockInfos;

	uint numBlocks = 0;
	std::vector<AdditionalDataBlock> blocks;

public:
	NiAdditionalGeometryData();
	NiAdditionalGeometryData(NiStream& stream);

	static constexpr const char* BlockName = "NiAdditionalGeometryData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiAdditionalGeometryData* Clone() { return new NiAdditionalGeometryData(*this); }
};

struct BSPackedAdditionalDataBlock {
	bool hasData = false;
	uint numTotalBytes = 0;

	uint numBlocks = 0;
	std::vector<uint> blockOffsets;

	uint numAtoms = 0;
	std::vector<uint> atomSizes;
	std::vector<byte> data;

	uint unkInt1 = 0;
	uint numTotalBytesPerElement = 0;

	void Get(NiStream& stream) {
		stream >> hasData;

		if (hasData) {
			stream >> numTotalBytes;

			stream >> numBlocks;
			blockOffsets.resize(numBlocks);
			for (int i = 0; i < numBlocks; i++)
				stream >> blockOffsets[i];

			stream >> numAtoms;
			atomSizes.resize(numAtoms);
			for (int i = 0; i < numAtoms; i++)
				stream >> atomSizes[i];

			data.resize(numTotalBytes);
			for (int i = 0; i < numTotalBytes; i++)
				stream >> data[i];
		}

		stream >> unkInt1;
		stream >> numTotalBytesPerElement;
	}

	void Put(NiStream& stream) {
		stream << hasData;

		if (hasData) {
			stream << numTotalBytes;

			stream << numBlocks;
			for (int i = 0; i < numBlocks; i++)
				stream << blockOffsets[i];

			stream << numAtoms;
			for (int i = 0; i < numAtoms; i++)
				stream << atomSizes[i];

			for (int i = 0; i < numTotalBytes; i++)
				stream << data[i];
		}

		stream << unkInt1;
		stream << numTotalBytesPerElement;
	}
};

class BSPackedAdditionalGeometryData : public AdditionalGeomData {
private:
	ushort numVertices = 0;

	uint numBlockInfos = 0;
	std::vector<AdditionalDataInfo> blockInfos;

	uint numBlocks = 0;
	std::vector<BSPackedAdditionalDataBlock> blocks;

public:
	BSPackedAdditionalGeometryData();
	BSPackedAdditionalGeometryData(NiStream& stream);

	static constexpr const char* BlockName = "BSPackedAdditionalGeometryData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	BSPackedAdditionalGeometryData* Clone() { return new BSPackedAdditionalGeometryData(*this); }
};

class NiGeometryData : public NiObject {
protected:
	bool isPSys = false;

private:
	int groupID = 0;
	byte compressFlags = 0;
	bool hasVertices = true;
	uint materialCRC = 0;					// Version >= 20.2.0.7 && User Version == 12
	bool hasNormals = false;
	bool hasVertexColors = false;
	BlockRef<AdditionalGeomData> additionalDataRef;
	BoundingSphere bounds;

public:
	ushort numVertices = 0;
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<Color4> vertexColors;

	byte keepFlags = 0;
	ushort numUVSets = 0;
	std::vector<Vector2> uvSets;

	ushort consistencyFlags = 0;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);

	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	void SetVertices(const bool enable);
	bool HasVertices() { return hasVertices; }

	void SetNormals(const bool enable);
	bool HasNormals() { return hasNormals; }

	void SetVertexColors(const bool enable);
	bool HasVertexColors() { return hasVertexColors; }

	void SetUVs(const bool enable);
	bool HasUVs() { return (numUVSets & (1 << 0)) != 0; }

	void SetTangents(const bool enable);
	bool HasTangents() { return (numUVSets & (1 << 12)) != 0; }

	void SetBounds(const BoundingSphere& newBounds) { this->bounds = newBounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	virtual void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs);
	virtual void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	virtual void CalcTangentSpace();
};

class NiShape : public NiAVObject {
protected:
	NiGeometryData* geomData = nullptr;

public:
	virtual int GetDataRef();
	virtual void SetDataRef(int dataRef);

	virtual int GetSkinInstanceRef();
	virtual void SetSkinInstanceRef(int skinInstanceRef);

	virtual int GetShaderPropertyRef();
	virtual void SetShaderPropertyRef(int shaderPropertyRef);

	virtual int GetAlphaPropertyRef();
	virtual void SetAlphaPropertyRef(int alphaPropertyRef);

	virtual void SetVertices(const bool enable);
	virtual bool HasVertices();

	virtual void SetUVs(const bool enable);
	virtual bool HasUVs();

	virtual void SetNormals(const bool enable);
	virtual bool HasNormals();

	virtual void SetTangents(const bool enable);
	virtual bool HasTangents();

	virtual void SetVertexColors(const bool enable);
	virtual bool HasVertexColors();

	virtual void SetSkinned(const bool enable);
	virtual bool IsSkinned();

	virtual void SetBounds(const BoundingSphere& bounds);
	virtual BoundingSphere GetBounds();
	virtual void UpdateBounds();

	void SetGeomData(NiGeometryData* geomDataPtr) { geomData = geomDataPtr; }
	int GetBoneID(NiHeader& hdr, const std::string& boneName);
};


class BSTriShape : public NiShape {
private:
	BlockRef<NiObject> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;
	BlockRef<NiProperty> alphaPropertyRef;

	BoundingSphere bounds;

public:
	VertexDesc vertexDesc;

	uint numTriangles = 0;
	ushort numVertices = 0;
	uint dataSize = 0;
	uint vertexSize = 0;		// Not in file

	uint particleDataSize = 0;
	std::vector<Vector3> particleVerts;
	std::vector<Vector3> particleNorms;
	std::vector<Triangle> particleTris;

	std::vector<Vector3> rawVertices;		// filled by GetRawVerts function and returned.
	std::vector<Vector3> rawNormals;		// filled by GetNormalData function and returned.
	std::vector<Vector3> rawTangents;		// filled by CalcTangentSpace function and returned.
	std::vector<Vector3> rawBitangents;		// filled in CalcTangentSpace
	std::vector<Vector2> rawUvs;			// filled by GetUVData function and returned.

	std::vector<uint> deletedTris;			// temporary storage for BSSubIndexTriShape

	std::vector<BSVertexData> vertData;
	std::vector<Triangle> triangles;

	BSTriShape();
	BSTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void GetChildRefs(std::set<int*>& refs);
	BSTriShape* Clone() { return new BSTriShape(*this); }

	int GetSkinInstanceRef() { return skinInstanceRef.index; }
	void SetSkinInstanceRef(int skinInstRef) { this->skinInstanceRef.index = skinInstRef; }

	int GetShaderPropertyRef() { return shaderPropertyRef.index; }
	void SetShaderPropertyRef(int shaderPropRef) { this->shaderPropertyRef.index = shaderPropRef; }

	int GetAlphaPropertyRef() { return alphaPropertyRef.index; }
	void SetAlphaPropertyRef(int alphaPropRef) { this->alphaPropertyRef.index = alphaPropRef; }

	const std::vector<Vector3>* GetRawVerts();
	const std::vector<Vector3>* GetNormalData(bool xform = true);
	const std::vector<Vector3>* GetTangentData(bool xform = true);
	const std::vector<Vector3>* GetBitangentData(bool xform = true);
	const std::vector<Vector2>* GetUVData();

	void SetVertices(const bool enable);
	bool HasVertices() { return vertexDesc.HasFlag(VF_VERTEX); }

	void SetUVs(const bool enable);
	bool HasUVs() { return vertexDesc.HasFlag(VF_UV); }

	void SetSecondUVs(const bool enable);
	bool HasSecondUVs() { return vertexDesc.HasFlag(VF_UV_2); }

	void SetNormals(const bool enable);
	bool HasNormals() { return vertexDesc.HasFlag(VF_NORMAL); }

	void SetTangents(const bool enable);
	bool HasTangents() { return vertexDesc.HasFlag(VF_TANGENT); }

	void SetVertexColors(const bool enable);
	bool HasVertexColors() { return vertexDesc.HasFlag(VF_COLORS); }

	void SetSkinned(const bool enable);
	bool IsSkinned() { return vertexDesc.HasFlag(VF_SKINNED); }

	void SetEyeData(const bool enable);
	bool HasEyeData() { return vertexDesc.HasFlag(VF_EYEDATA); }

	void SetFullPrecision(const bool enable);
	bool IsFullPrecision() { return vertexDesc.HasFlag(VF_FULLPREC); }
	bool CanChangePrecision() { return (HasVertices()); }

	void SetBounds(const BoundingSphere& newBounds) { this->bounds = newBounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	void SetNormals(const std::vector<Vector3>& inNorms);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcDataSizes(NiVersion& version);

	virtual void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals = nullptr);
};


class BSGeometrySegmentData {
public:
	byte flags = 0;
	uint index = 0;
	uint numTris = 0;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class BSSubIndexTriShape : public BSTriShape {
public:
	class BSSITSSubSegment {
	public:
		uint startIndex = 0;
		uint numPrimitives = 0;
		uint arrayIndex = 0;
		uint unkInt1 = 0;
	};

	class BSSITSSegment {
	public:
		uint startIndex = 0;
		uint numPrimitives = 0;
		uint parentArrayIndex = 0xFFFFFFFF;
		uint numSubSegments = 0;
		std::vector<BSSITSSubSegment> subSegments;
	};

	class BSSITSSubSegmentDataRecord {
	public:
		uint segmentUser = 0;
		uint unkInt2 = 0xFFFFFFFF;
		uint numData = 0;
		std::vector<float> extraData;
	};

	class BSSITSSubSegmentData {
	public:
		uint numSegments = 0;
		uint numTotalSegments = 0;
		std::vector<uint> arrayIndices;
		std::vector<BSSITSSubSegmentDataRecord> dataRecords;
		NiString ssfFile;
	};

	class BSSITSSegmentation {
	public:
		uint numPrimitives = 0;
		uint numSegments = 0;
		uint numTotalSegments = 0;
		std::vector<BSSITSSegment> segments;
		BSSITSSubSegmentData subSegmentData;
	};

	// SSE
	uint numSegments = 0;
	std::vector<BSGeometrySegmentData> segments;

private:
	// FO4
	BSSITSSegmentation segmentation;

public:
	BSSubIndexTriShape();
	BSSubIndexTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSSubIndexTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	BSSubIndexTriShape* Clone() { return new BSSubIndexTriShape(*this); }

	BSSITSSegmentation GetSegmentation() { return segmentation; }
	void SetSegmentation(const BSSITSSegmentation& seg) { segmentation = seg; }

	void SetDefaultSegments();
	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals = nullptr);
};

class BSMeshLODTriShape : public BSTriShape {
public:
	uint lodSize0 = 0;
	uint lodSize1 = 0;
	uint lodSize2 = 0;

	BSMeshLODTriShape();
	BSMeshLODTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSMeshLODTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	BSMeshLODTriShape* Clone() { return new BSMeshLODTriShape(*this); }
};

class BSDynamicTriShape : public BSTriShape {
public:
	uint dynamicDataSize;
	std::vector<Vector4> dynamicData;

	BSDynamicTriShape();
	BSDynamicTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSDynamicTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void CalcDynamicData();
	BSDynamicTriShape* Clone() { return new BSDynamicTriShape(*this); }

	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals = nullptr);
};

class NiSkinInstance;

class NiGeometry : public NiShape {
private:
	BlockRef<NiGeometryData> dataRef;
	BlockRef<NiSkinInstance> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;				// Version >= 20.2.0.7 && User Version == 12
	BlockRef<NiProperty> alphaPropertyRef;				// Version >= 20.2.0.7 && User Version == 12

	uint numMaterials = 0;
	std::vector<StringRef> materialNameRefs;
	std::vector<uint> materials;
	int activeMaterial = 0;
	byte dirty = 0;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);

	bool IsSkinned() { return skinInstanceRef.index != 0xFFFFFFFF; }

	int GetDataRef() { return dataRef.index; }
	void SetDataRef(int datRef) { this->dataRef.index = datRef; }

	int GetSkinInstanceRef() { return skinInstanceRef.index; }
	void SetSkinInstanceRef(int skinInstRef) { this->skinInstanceRef.index = skinInstRef; }

	int GetShaderPropertyRef() { return shaderPropertyRef.index; }
	void SetShaderPropertyRef(int shaderPropRef) { this->shaderPropertyRef.index = shaderPropRef; }

	int GetAlphaPropertyRef() { return alphaPropertyRef.index; }
	void SetAlphaPropertyRef(int alphaPropRef) { this->alphaPropertyRef.index = alphaPropRef; }
};

class NiTriBasedGeom : public NiGeometry {
};

class NiTriBasedGeomData : public NiGeometryData {
public:
	ushort numTriangles = 0;

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs);
};

class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape() {}
	NiTriShape(NiStream& stream);

	static constexpr const char* BlockName = "NiTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	NiTriShape* Clone() { return new NiTriShape(*this); }
};

struct MatchGroup {
	ushort count = 0;
	std::vector<ushort> matches;
};

class NiTriShapeData : public NiTriBasedGeomData {
private:
	bool hasTriangles = false;
	uint numTrianglePoints = 0;
	ushort numMatchGroups = 0;
	std::vector<MatchGroup> matchGroups;

public:
	std::vector<Triangle> triangles;

	NiTriShapeData() {}
	NiTriShapeData(NiStream& stream);

	static constexpr const char* BlockName = "NiTriShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	NiTriShapeData* Clone() { return new NiTriShapeData(*this); }
};

class NiTriStrips : public NiTriBasedGeom {
public:
	NiTriStrips() {}
	NiTriStrips(NiStream& stream);

	static constexpr const char* BlockName = "NiTriStrips";
	virtual const char* GetBlockName() { return BlockName; }

	NiTriStrips* Clone() { return new NiTriStrips(*this); }
};

class NiTriStripsData : public NiTriBasedGeomData {
private:
	ushort numStrips = 0;
	std::vector<ushort> stripLengths;
	bool hasPoints = false;
	std::vector<std::vector<ushort>> points;

public:
	NiTriStripsData() {}
	NiTriStripsData(NiStream& stream);

	static constexpr const char* BlockName = "NiTriStripsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void StripsToTris(std::vector<Triangle>* outTris);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	NiTriStripsData* Clone() { return new NiTriStripsData(*this); }
};

class NiLines : public NiTriBasedGeom {
public:
	NiLines() {}
	NiLines(NiStream& stream);

	static constexpr const char* BlockName = "NiLines";
	virtual const char* GetBlockName() { return BlockName; }

	NiLines* Clone() { return new NiLines(*this); }
};

class NiLinesData : public NiGeometryData {
private:
	std::deque<bool> lineFlags;

public:
	NiLinesData() {}
	NiLinesData(NiStream& stream);

	static constexpr const char* BlockName = "NiLinesData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	NiLinesData* Clone() { return new NiLinesData(*this); }
};

class NiScreenElements : public NiTriShape {
public:
	NiScreenElements() {}
	NiScreenElements(NiStream& stream);

	static constexpr const char* BlockName = "NiScreenElements";
	virtual const char* GetBlockName() { return BlockName; }

	NiScreenElements* Clone() { return new NiScreenElements(*this); }
};

struct PolygonInfo {
	ushort numVertices = 0;
	ushort vertexOffset = 0;
	ushort numTriangles = 0;
	ushort triangleOffset = 0;
};

class NiScreenElementsData : public NiTriShapeData {
private:
	ushort maxPolygons = 0;
	std::vector<PolygonInfo> polygons;
	std::vector<ushort> polygonIndices;

	ushort unkShort1 = 1;
	ushort numPolygons = 0;
	ushort usedVertices = 0;
	ushort unkShort2 = 1;
	ushort usedTrianglePoints = 0;
	ushort unkShort3 = 1;

public:
	NiScreenElementsData() {}
	NiScreenElementsData(NiStream& stream);

	static constexpr const char* BlockName = "NiScreenElementsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	NiScreenElementsData* Clone() { return new NiScreenElementsData(*this); }
};

class BSLODTriShape : public NiTriBasedGeom {
private:
	uint level0 = 0;
	uint level1 = 0;
	uint level2 = 0;

public:
	BSLODTriShape() {}
	BSLODTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSLODTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLODTriShape* Clone() { return new BSLODTriShape(*this); }
};

class BSSegmentedTriShape : public NiTriShape {
public:
	uint numSegments = 0;
	std::vector<BSGeometrySegmentData> segments;

	BSSegmentedTriShape() {}
	BSSegmentedTriShape(NiStream& stream);

	static constexpr const char* BlockName = "BSSegmentedTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSSegmentedTriShape* Clone() { return new BSSegmentedTriShape(*this); }
};
