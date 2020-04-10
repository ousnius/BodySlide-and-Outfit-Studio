/*
BodySlide and Outfit Studio
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
	static constexpr const char* BlockName = "BSPackedAdditionalGeometryData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	BSPackedAdditionalGeometryData* Clone() { return new BSPackedAdditionalGeometryData(*this); }
};

class NiGeometryData : public NiObject {
protected:
	bool isPSys = false;

	ushort numVertices = 0;
	int groupID = 0;
	byte compressFlags = 0;
	bool hasVertices = true;
	uint materialCRC = 0;
	bool hasNormals = false;
	bool hasVertexColors = false;
	BlockRef<AdditionalGeomData> additionalDataRef;
	BoundingSphere bounds;

public:
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<Color4> vertexColors;

	byte keepFlags = 0;
	ushort numUVSets = 0;
	std::vector<std::vector<Vector2>> uvSets;

	ushort consistencyFlags = 0;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<Ref*>& refs);

	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	int GetAdditionalDataRef();
	void SetAdditionalDataRef(int dataRef);

	ushort GetNumVertices();
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

	virtual uint GetNumTriangles();
	virtual bool GetTriangles(std::vector<Triangle>& tris);
	virtual void SetTriangles(const std::vector<Triangle>& tris);

	void SetBounds(const BoundingSphere& newBounds) { this->bounds = newBounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	virtual void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* norms);
	virtual void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	virtual void CalcTangentSpace();
};

class NiShape : public NiAVObject {
public:
	virtual NiGeometryData* GetGeomData();
	virtual void SetGeomData(NiGeometryData* geomDataPtr);

	virtual int GetDataRef();
	virtual void SetDataRef(int dataRef);

	virtual int GetSkinInstanceRef();
	virtual void SetSkinInstanceRef(int skinInstanceRef);

	virtual int GetShaderPropertyRef();
	virtual void SetShaderPropertyRef(int shaderPropertyRef);

	virtual int GetAlphaPropertyRef();
	virtual void SetAlphaPropertyRef(int alphaPropertyRef);

	virtual ushort GetNumVertices();
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

	virtual uint GetNumTriangles();
	virtual bool GetTriangles(std::vector<Triangle>& tris);
	virtual void SetTriangles(const std::vector<Triangle>& tris);
	virtual bool ReorderTriangles(const std::vector<uint>& triInds);

	virtual void SetBounds(const BoundingSphere& bounds);
	virtual BoundingSphere GetBounds();
	virtual void UpdateBounds();

	int GetBoneID(NiHeader& hdr, const std::string& boneName);
};


class BSTriShape : public NiShape {
protected:
	BlockRef<NiObject> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;
	BlockRef<NiProperty> alphaPropertyRef;

	BoundingSphere bounds;

	uint numTriangles = 0;
	std::vector<Triangle> triangles;

	ushort numVertices = 0;

public:
	VertexDesc vertexDesc;

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
	std::vector<Color4> rawColors;			// filled by GetColorData function and returned.
	std::vector<float> rawEyeData;

	std::vector<uint> deletedTris;			// temporary storage for BSSubIndexTriShape

	std::vector<BSVertexData> vertData;

	BSTriShape();

	static constexpr const char* BlockName = "BSTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void GetChildRefs(std::set<Ref*>& refs);
	BSTriShape* Clone() { return new BSTriShape(*this); }

	int GetSkinInstanceRef();
	void SetSkinInstanceRef(int skinInstRef);

	int GetShaderPropertyRef();
	void SetShaderPropertyRef(int shaderPropRef);

	int GetAlphaPropertyRef();
	void SetAlphaPropertyRef(int alphaPropRef);

	std::vector<Vector3>* GetRawVerts();
	std::vector<Vector3>* GetNormalData(bool xform = true);
	std::vector<Vector3>* GetTangentData(bool xform = true);
	std::vector<Vector3>* GetBitangentData(bool xform = true);
	std::vector<Vector2>* GetUVData();
	std::vector<Color4>* GetColorData();
	std::vector<float>* GetEyeData();

	ushort GetNumVertices();
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

	uint GetNumTriangles();
	bool GetTriangles(std::vector<Triangle>&);
	void SetTriangles(const std::vector<Triangle>&);

	void SetBounds(const BoundingSphere& newBounds) { bounds = newBounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	void SetVertexData(const std::vector<BSVertexData>& bsVertData);

	void SetNormals(const std::vector<Vector3>& inNorms);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcDataSizes(NiVersion& version);

	void SetTangentData(const std::vector<Vector3> &in);
	void SetBitangentData(const std::vector<Vector3> &in);
	void SetEyeData(const std::vector<float> &in);

	virtual void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals = nullptr);
};


// NifSubSegmentInfo: not in file.  The portion of a subsegment's data
// that has nothing to do with triangle set partitioning.
struct NifSubSegmentInfo {
	// partID: a small nonnegative integer uniquely identifying this
	// subsegment among all the segments and subsegments.  Used as a value
	// in triParts.  Not in the file.
	int partID;
	uint userSlotID;
	uint material;
	std::vector<float> extraData;
};

// NifSegmentInfo: not in file.  The portion of a segment's data that
// has nothing to do with triangle set partitioning.
struct NifSegmentInfo {
	// partID: a small nonnegative integer uniquely identifying this
	// segment among all the segments and subsegments.  Used as a value
	// in triParts.  Not in the file.
	int partID;
	std::vector<NifSubSegmentInfo> subs;
};

// NifSegmentationInfo: not in file.  The portion of a shape's
// segmentation data that has nothing to do with triangle set partitioning.
// The intention is that this data structure can be used for any type of
// segmentation data, both BSSITSSegmentation and BSGeometrySegmentData.
struct NifSegmentationInfo {
	std::vector<NifSegmentInfo> segs;
	std::string ssfFile;
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
		uint userSlotID = 0;
		uint material = 0xFFFFFFFF;
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
	static constexpr const char* BlockName = "BSSubIndexTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	BSSubIndexTriShape* Clone() { return new BSSubIndexTriShape(*this); }

	void GetSegmentation(NifSegmentationInfo &inf, std::vector<int> &triParts);
	void SetSegmentation(const NifSegmentationInfo &inf, const std::vector<int> &triParts);

	void SetDefaultSegments();
	void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals = nullptr);
};

class BSMeshLODTriShape : public BSTriShape {
public:
	uint lodSize0 = 0;
	uint lodSize1 = 0;
	uint lodSize2 = 0;

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

	static constexpr const char* BlockName = "BSDynamicTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void CalcDynamicData();
	BSDynamicTriShape* Clone() { return new BSDynamicTriShape(*this); }

	void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* normals = nullptr);
};

class NiSkinInstance;

class NiGeometry : public NiShape {
protected:
	BlockRef<NiGeometryData> dataRef;
	BlockRef<NiSkinInstance> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;
	BlockRef<NiProperty> alphaPropertyRef;

	uint numMaterials = 0;
	std::vector<StringRef> materialNameRefs;
	std::vector<uint> materials;
	int activeMaterial = 0;
	byte defaultMatNeedsUpdateFlag = 0;

	bool shader = false;
	StringRef shaderName;
	uint implementation = 0;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<Ref*>& refs);

	bool IsSkinned();

	int GetDataRef();
	void SetDataRef(int datRef);

	int GetSkinInstanceRef();
	void SetSkinInstanceRef(int skinInstRef);

	int GetShaderPropertyRef();
	void SetShaderPropertyRef(int shaderPropRef);

	int GetAlphaPropertyRef();
	void SetAlphaPropertyRef(int alphaPropRef);
};

class NiTriBasedGeom : public NiGeometry {
};

class NiTriBasedGeomData : public NiGeometryData {
protected:
	ushort numTriangles = 0;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* norms);
};

struct MatchGroup {
	ushort count = 0;
	std::vector<ushort> matches;
};

class NiTriShapeData : public NiTriBasedGeomData {
protected:
	uint numTrianglePoints = 0;
	bool hasTriangles = false;
	std::vector<Triangle> triangles;

	ushort numMatchGroups = 0;
	std::vector<MatchGroup> matchGroups;

public:
	static constexpr const char* BlockName = "NiTriShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void Create(const std::vector<Vector3>* verts, const std::vector<Triangle>* tris, const std::vector<Vector2>* uvs, const std::vector<Vector3>* norms);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	uint GetNumTriangles();
	bool GetTriangles(std::vector<Triangle>& tris);
	void SetTriangles(const std::vector<Triangle>& tris);

	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	NiTriShapeData* Clone() { return new NiTriShapeData(*this); }
};

class NiTriShape : public NiTriBasedGeom {
protected:
	NiTriShapeData* shapeData = nullptr;

public:
	static constexpr const char* BlockName = "NiTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	NiGeometryData* GetGeomData();
	void SetGeomData(NiGeometryData* geomDataPtr);

	NiTriShape* Clone() { return new NiTriShape(*this); }
};

class NiTriStripsData : public NiTriBasedGeomData {
protected:
	ushort numStrips = 0;
	std::vector<ushort> stripLengths;
	bool hasPoints = false;
	std::vector<std::vector<ushort>> points;

public:
	static constexpr const char* BlockName = "NiTriStripsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	uint GetNumTriangles();
	bool GetTriangles(std::vector<Triangle>& tris);
	void SetTriangles(const std::vector<Triangle>& tris);
	std::vector<Triangle> StripsToTris();

	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	NiTriStripsData* Clone() { return new NiTriStripsData(*this); }
};

class NiTriStrips : public NiTriBasedGeom {
protected:
	NiTriStripsData* stripsData = nullptr;

public:
	static constexpr const char* BlockName = "NiTriStrips";
	virtual const char* GetBlockName() { return BlockName; }

	NiGeometryData* GetGeomData();
	void SetGeomData(NiGeometryData* geomDataPtr);

	virtual bool ReorderTriangles(const std::vector<uint>&) { return false; }

	NiTriStrips* Clone() { return new NiTriStrips(*this); }
};

class NiLinesData : public NiGeometryData {
protected:
	std::deque<bool> lineFlags;

public:
	static constexpr const char* BlockName = "NiLinesData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	NiLinesData* Clone() { return new NiLinesData(*this); }
};

class NiLines : public NiTriBasedGeom {
protected:
	NiLinesData* linesData = nullptr;

public:
	static constexpr const char* BlockName = "NiLines";
	virtual const char* GetBlockName() { return BlockName; }

	NiGeometryData* GetGeomData();
	void SetGeomData(NiGeometryData* geomDataPtr);

	NiLines* Clone() { return new NiLines(*this); }
};

struct PolygonInfo {
	ushort numVertices = 0;
	ushort vertexOffset = 0;
	ushort numTriangles = 0;
	ushort triangleOffset = 0;
};

class NiScreenElementsData : public NiTriShapeData {
protected:
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
	static constexpr const char* BlockName = "NiScreenElementsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);

	NiScreenElementsData* Clone() { return new NiScreenElementsData(*this); }
};

class NiScreenElements : public NiTriShape {
protected:
	NiScreenElementsData* elemData = nullptr;

public:
	static constexpr const char* BlockName = "NiScreenElements";
	virtual const char* GetBlockName() { return BlockName; }

	NiGeometryData* GetGeomData();
	void SetGeomData(NiGeometryData* geomDataPtr);

	NiScreenElements* Clone() { return new NiScreenElements(*this); }
};

class BSLODTriShape : public NiTriBasedGeom {
protected:
	NiTriShapeData* shapeData = nullptr;

	uint level0 = 0;
	uint level1 = 0;
	uint level2 = 0;

public:
	static constexpr const char* BlockName = "BSLODTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	NiGeometryData* GetGeomData();
	void SetGeomData(NiGeometryData* geomDataPtr);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSLODTriShape* Clone() { return new BSLODTriShape(*this); }
};

class BSSegmentedTriShape : public NiTriShape {
public:
	uint numSegments = 0;
	std::vector<BSGeometrySegmentData> segments;

	static constexpr const char* BlockName = "BSSegmentedTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSSegmentedTriShape* Clone() { return new BSSegmentedTriShape(*this); }
};
