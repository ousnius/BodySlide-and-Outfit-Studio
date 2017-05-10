/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Objects.h"
#include "VertexData.h"

class NiGeometryData : public NiObject {
protected:
	bool isPSys = false;

private:
	int groupID;
	byte compressFlags;
	bool hasVertices;
	uint materialCRC;						// Version >= 20.2.0.7 && User Version == 12
	bool hasNormals;
	bool hasVertexColors;
	uint additionalData;
	BoundingSphere bounds;

public:
	ushort numVertices;
	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<Color4> vertexColors;

	byte keepFlags;
	ushort numUVSets;
	std::vector<Vector2> uvSets;

	ushort consistencyFlags;

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int CalcBlockSize();

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
private:
	NiGeometryData* GetGeomData() { return header->GetBlock<NiGeometryData>(GetDataRef()); }

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

	int GetBoneID(const std::string& boneName);
};


class BSTriShape : public NiShape {
private:
	BlockRef<NiObject> skinInstanceRef;
	BlockRef<NiProperty> shaderPropertyRef;
	BlockRef<NiProperty> alphaPropertyRef;

	BoundingSphere bounds;

public:
	// Set in CalcBlockSize()
	byte vertFlags1;	// Number of uint elements in vertex data
	byte vertFlags2;	// 4 byte or 2 byte position data

	byte vertFlags3;
	byte vertFlags4;
	byte vertFlags5;
	byte vertFlags6;	// Vertex, UVs, Normals
	byte vertFlags7;	// (Bi)Tangents, Vertex Colors, Skinning, Precision
	byte vertFlags8;

	uint numTriangles;
	ushort numVertices;
	uint dataSize;
	uint vertexSize;	// Not in file

	uint particleDataSize;
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

	BSTriShape(NiHeader* hdr);
	BSTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	bool HasVertices() { return (vertFlags6 & (1 << 4)) != 0; }

	void SetUVs(const bool enable);
	bool HasUVs() { return (vertFlags6 & (1 << 5)) != 0; }

	void SetNormals(const bool enable);
	bool HasNormals() { return (vertFlags6 & (1 << 7)) != 0; }

	void SetTangents(const bool enable);
	bool HasTangents() { return (vertFlags7 & (1 << 0)) != 0; }

	void SetVertexColors(const bool enable);
	bool HasVertexColors() { return (vertFlags7 & (1 << 1)) != 0; }

	void SetSkinned(const bool enable);
	bool IsSkinned() { return (vertFlags7 & (1 << 2)) != 0; }

	void SetFullPrecision(const bool enable);
	bool IsFullPrecision() { return (vertFlags7 & (1 << 6)) != 0 || header->GetUserVersion2() == 100; }
	bool CanChangePrecision() { return (HasVertices() && header->GetUserVersion2() != 100); }

	void SetBounds(const BoundingSphere& newBounds) { this->bounds = newBounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	void SetNormals(const std::vector<Vector3>& inNorms);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	void UpdateFlags();
	int CalcDataSizes();

	virtual void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals = nullptr);
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

private:
	BSSITSSegmentation segmentation;

public:
	BSSubIndexTriShape(NiHeader* hdr);
	BSSubIndexTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSSubIndexTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int CalcBlockSize();
	BSSubIndexTriShape* Clone() { return new BSSubIndexTriShape(*this); }

	BSSITSSegmentation GetSegmentation() { return segmentation; }
	void SetSegmentation(const BSSITSSegmentation& segments) { this->segmentation = segments; }

	void SetDefaultSegments();
	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs, std::vector<Vector3>* normals = nullptr);
};

class BSMeshLODTriShape : public BSTriShape {
public:
	uint lodSize0;
	uint lodSize1;
	uint lodSize2;

	BSMeshLODTriShape(NiHeader* hdr);
	BSMeshLODTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMeshLODTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int CalcBlockSize();
	BSMeshLODTriShape* Clone() { return new BSMeshLODTriShape(*this); }
};

class BSDynamicTriShape : public BSTriShape {
public:
	uint dynamicDataSize;
	std::vector<Vector4> dynamicData;

	BSDynamicTriShape(NiHeader* hdr);
	BSDynamicTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSDynamicTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	int CalcBlockSize();
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
	~NiGeometry() {
		for (auto &m : materialNameRefs) {
			m.Clear(header);
		}
	}

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();

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
	ushort numTriangles;

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();

	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs);
};

class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape(NiHeader* hdr);
	NiTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	NiTriShape* Clone() { return new NiTriShape(*this); }
};

struct MatchGroup {
	ushort count;
	std::vector<ushort> matches;
};

class NiTriShapeData : public NiTriBasedGeomData {
private:
	bool hasTriangles;
	uint numTrianglePoints;
	ushort numMatchGroups;
	std::vector<MatchGroup> matchGroups;

public:
	std::vector<Triangle> triangles;

	NiTriShapeData(NiHeader* hdr);
	NiTriShapeData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTriShapeData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void Create(std::vector<Vector3>* verts, std::vector<Triangle>* tris, std::vector<Vector2>* uvs);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcBlockSize();
	NiTriShapeData* Clone() { return new NiTriShapeData(*this); }
};

class NiTriStrips : public NiTriBasedGeom {
public:
	NiTriStrips(NiHeader* hdr);
	NiTriStrips(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTriStrips";
	virtual const char* GetBlockName() { return BlockName; }

	NiTriStrips* Clone() { return new NiTriStrips(*this); }
};

class NiTriStripsData : public NiTriBasedGeomData {
private:
	ushort numStrips;
	std::vector<ushort> stripLengths;
	byte hasPoints;
	std::vector<std::vector<ushort>> points;

public:
	NiTriStripsData(NiHeader* hdr);
	NiTriStripsData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiTriStripsData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyVerticesDelete(const std::vector<ushort>& vertIndices);
	void StripsToTris(std::vector<Triangle>* outTris);
	void RecalcNormals(const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcBlockSize();
	NiTriStripsData* Clone() { return new NiTriStripsData(*this); }
};

class BSLODTriShape : public NiTriBasedGeom {
private:
	uint level0;
	uint level1;
	uint level2;

public:
	BSLODTriShape(NiHeader* hdr);
	BSLODTriShape(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSLODTriShape";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSLODTriShape* Clone() { return new BSLODTriShape(*this); }
};
