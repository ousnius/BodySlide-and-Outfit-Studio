/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../utils/KDMatcher.h"

#include <fstream>
#include <unordered_map>
#include <string>

using namespace std;

enum BlockType : ushort {
	NIHEADER,
	NIUNKNOWN,
	NITRISHAPE,
	NITRISHAPEDATA,
	NINODE,
	BSDISMEMBERSKININSTANCE,
	NISKINPARTITION,
	BSLIGHTINGSHADERPROPERTY,
	NIALPHAPROPERTY,
	NISKINDATA,
	BSSHADERTEXTURESET,
	NITRISTRIPS,
	NITRISTRIPSDATA,
	NISKININSTANCE,
	BSSKININSTANCE,
	BSBONEDATA,
	NISTRINGEXTRADATA,
	BSSHADERPPLIGHTINGPROPERTY,
	NIMATERIALPROPERTY,
	NISTENCILPROPERTY,
	BSEFFECTSHADERPROPERTY,
	NIFLOATINTERPOLATOR,
	NITRANSFORMINTERPOLATOR,
	NIPOINT3INTERPOLATOR,
	BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER,
	BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER,
	BSEFFECTSHADERPROPERTYCOLORCONTROLLER,
	BSEFFECTSHADERPROPERTYFLOATCONTROLLER,
	BSTRISHAPE,
	BSSUBINDEXTRISHAPE,
	BSMESHLODTRISHAPE,
	BSCLOTHEXTRADATA,
	NIINTEGEREXTRADATA,
	BSXFLAGS,
	BSCONNECTPOINTPARENTS,
	BSCONNECTPOINTCHILDREN,
	NICOLLISIONOBJECT,
	BHKNICOLLISIONOBJECT,
	BHKCOLLISIONOBJECT,
	BHKNPCOLLISIONOBJECT,
	BHKPHYSICSSYSTEM
};

struct VertexWeight {
	float w1;
	float w2;
	float w3;
	float w4;
};

struct BoneIndices {
	byte i1;
	byte i2;
	byte i3;
	byte i4;
};

struct SkinTransform {
	Vector3 rotation[3];
	Vector3 translation;
	float scale;

	SkinTransform() {
		translation = Vector3();
		scale = 1.0f;
		rotation[0] = Vector3(1.0f, 0.0f, 0.0f);
		rotation[1] = Vector3(0.0f, 1.0f, 0.0f);
		rotation[2] = Vector3(0.0f, 0.0f, 1.0f);
	}

	Matrix4 ToMatrix() {
		Matrix4 m;
		m[0] = rotation[0].x * scale;
		m[1] = rotation[0].y;
		m[2] = rotation[0].z;
		m[3] = translation.x;
		m[4] = rotation[1].x;
		m[5] = rotation[1].y * scale;
		m[6] = rotation[1].z;
		m[7] = translation.y;
		m[8] = rotation[2].x;
		m[9] = rotation[2].y;
		m[10] = rotation[2].z * scale;
		m[11] = translation.z;
		return m;
	}

	Matrix4 ToMatrixSkin() {
		Matrix4 m;
		// Set the rotation
		Vector3 r1 = rotation[0];
		Vector3 r2 = rotation[1];
		Vector3 r3 = rotation[2];
		m.SetRow(0, r1);
		m.SetRow(1, r2);
		m.SetRow(2, r3);
		// Set the scale
		m.Scale(scale, scale, scale);
		// Set the translation
		m[3] = translation.x;
		m[7] = translation.y;
		m[11] = translation.z;
		return m;
	}
};

struct SkinWeight {
	ushort index;
	float weight;

	SkinWeight(const ushort& index = 0, const float& weight = 0.0f) {
		this->index = index;
		this->weight = weight;
	}
};

struct Color4 {
	float a;
	float r;
	float g;
	float b;

	Color4() {
		r = g = b = a = 0.0f;
	}
	Color4(const float& r, const float& g, const float& b, const float& a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	Color4& operator *= (const float& val) {
		r *= val;
		g *= val;
		b *= val;
		a *= val;
		return *this;
	}
	Color4 operator * (const float& val) const {
		Color4 tmp = *this;
		tmp *= val;
		return tmp;
	}

	Color4& operator /= (const float& val) {
		r /= val;
		g /= val;
		b /= val;
		a /= val;
		return *this;
	}
	Color4 operator / (const float& val) const {
		Color4 tmp = *this;
		tmp /= val;
		return tmp;
	}
};

struct MatchGroup {
	ushort count;
	vector<ushort> matches;
};

enum BSShaderType : uint {
	SHADER_TALL_GRASS, SHADER_DEFAULT, SHADER_SKY = 10, SHADER_SKIN = 14,
	SHADER_WATER = 17, SHADER_LIGHTING30 = 29, SHADER_TILE = 32, SHADER_NOLIGHTING
};

enum BSLightingShaderPropertyShaderType : uint {
	Default, EnvironmentMap, GlowShader, Heightmap,
	FaceTint, SkinTint, HairTint, ParallaxOccMaterial,
	WorldMultitexture, WorldMap1, Unknown10, MultiLayerParallax,
	Unknown12, WorldMap2, SparkleSnow, WorldMap3,
	EyeEnvmap, Unknown17, WorldMap4, WorldLODMultitexture
};

class NiString {
private:
	string str;

public:
	string GetString() {
		return str;
	}
	void SetString(const string& str) {
		this->str = str;
	}

	size_t GetLength() {
		return str.length();
	}

	void Clear() {
		str.clear();
	}

	NiString() {};
	NiString(fstream& file, const int& szSize);

	void Put(fstream& file, const int& szSize, const bool& wantNullOutput = true);
	void Get(fstream& file, const int& szSize);
};


class NiHeader;

class NiObject {
protected:
	uint blockSize;

public:
	NiHeader* header;
	BlockType blockType;

	NiObject();
	virtual ~NiObject();

	virtual void Init();
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);
	virtual void notifyStringDelete(int stringID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	virtual void Get(fstream& file);
	virtual void Put(fstream& file);

	virtual int CalcBlockSize();
};

class NiHeader : public NiObject {
	/*
	Minimum supported
		Version:			20.2.0.7
		User Version:		11
		User Version 2:		26

	Maximum supported
		Version:			20.2.0.7
		User Version:		12
		User Version 2:		130
	*/

private:
	bool valid;

	char verStr[0x26];
	byte version1;
	byte version2;
	byte version3;
	byte version4;
	uint userVersion;
	uint userVersion2;

	byte unk1;
	byte endian;

	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
	NiString exportInfo3;

	// Foreign reference to the blocks list in NifFile.
	vector<NiObject*>* blocks;

	uint numBlocks;
	ushort numBlockTypes;
	vector<NiString> blockTypes;
	vector<ushort> blockTypeIndices;
	vector<uint> blockSizes;

	uint numStrings;
	uint maxStringLen;
	vector<NiString> strings;
	vector<int> stringRefCount;

	uint unkInt2;

public:
	NiHeader();

	void Clear();
	bool IsValid() { return valid; }

	string GetVersionInfo();
	void SetVersion(const byte& v1, const byte& v2, const byte& v3, const byte& v4, const uint& userVer, const uint& userVer2);
	bool VerCheck(int v1, int v2, int v3, int v4, bool equal = false);

	uint GetUserVersion() { return userVersion; };
	uint GetUserVersion2() { return userVersion2; };

	string GetCreatorInfo();
	void SetCreatorInfo(const string& creatorInfo);

	string GetExportInfo();
	void SetExportInfo(const string& exportInfo);

	void SetBlockReference(vector<NiObject*>* blocks) { this->blocks = blocks; };
	uint GetNumBlocks() { return numBlocks; }

	template <class T>
	T* GetBlock(const int& blockId);

	void DeleteBlock(int blockId);
	void DeleteBlockByType(const string& blockTypeStr);
	int AddBlock(NiObject* newBlock, const string& blockTypeStr);

	// Swaps two blocks, updating references in other blocks that may refer to their old indices
	void SwapBlocks(const int& blockIndexLo, const int& blockIndexHi);

	ushort AddOrFindBlockTypeId(const string& blockTypeName);
	string GetBlockTypeStringById(const int& id);
	ushort GetBlockTypeIndex(const int& id);

	uint GetBlockSize(const uint& blockId) { return blockSizes[blockId]; }
	void CalcAllBlockSizes();

	int FindStringId(const string& str);
	int AddOrFindStringId(const string& str);
	string GetStringById(const int& id);
	void SetStringById(const int& id, const string& str);

	void AddStringRef(const int& id);
	void RemoveStringRef(const int& id);
	void RemoveUnusedStrings();

	void Get(fstream& file);
	void Put(fstream& file);
};


class NiObjectNET : public NiObject {
private:
	uint nameRef;
	string name;
	int controllerRef;
	int numExtraData;
	vector<int> extraDataRef;

public:
	uint skyrimShaderType;					// BSLightingShaderProperty && User Version >= 12
	bool bBSLightingShaderProperty;

	~NiObjectNET();

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyStringDelete(int stringID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	string GetName();
	void SetName(const string& propertyName, bool renameExisting = false);
	void ClearName();

	int GetControllerRef() { return controllerRef; }
	void SetControllerRef(int controllerRef) { this->controllerRef = controllerRef; }

	int GetNumExtraData() { return numExtraData; }
	int GetExtraDataRef(int id);
	void AddExtraDataRef(int id);

	int CalcBlockSize();
};

class NiAVObject : public NiObjectNET {
public:
	uint flags;
	ushort unkShort1;						// Version >= 20.2.0.7 && User Version >= 11 && User Version 2 > 26
	Vector3 translation;
	Vector3 rotation[3];
	float scale;
	uint numProperties;
	vector<int> propertiesRef;
	int collisionRef;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	bool rotToEulerDegrees(float &Y, float& P, float& R) {
		float rx, ry, rz;
		bool canRot = false;

		if (rotation[0].z < 1.0) {
			if (rotation[0].z > -1.0) {
				rx = atan2(-rotation[1].z, rotation[2].z);
				ry = asin(rotation[0].z);
				rz = atan2(-rotation[0].y, rotation[0].x);
				canRot = true;
			}
			else {
				rx = -atan2(-rotation[1].x, rotation[1].y);
				ry = -PI / 2;
				rz = 0.0f;
			}
		}
		else {
			rx = atan2(rotation[1].x, rotation[1].y);
			ry = PI / 2;
			rz = 0.0f;
		}

		Y = rx * 180 / PI;
		P = ry * 180 / PI;
		R = rz * 180 / PI;
		return canRot;
	}
};

class NiProperty : public NiObjectNET {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyStringDelete(int stringID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiNode : public NiAVObject {
private:
	uint numChildren;
	vector<int> children;
	uint numEffects;
	vector<int> effects;

public:
	NiNode(NiHeader& hdr);
	NiNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	int GetNumChildren() { return numChildren; }
	int GetChildRef(int id);
	void AddChildRef(int id);
	void ClearChildren();
	vector<int>::iterator ChildrenBegin() { return children.begin(); }
	vector<int>::iterator ChildrenEnd() { return children.end(); }

	int GetNumEffects() { return numEffects; }
	int GetEffectRef(int id);
	void AddEffectRef(int id);
};

class NiGeometryData : public NiObject {
private:
	int unkInt;
	byte keepFlags;
	byte compressFlags;
	bool hasVertices;
	//byte extraVectorsFlags;
	uint unkInt2;							// Version >= 20.2.0.7 && User Version == 12
	bool hasNormals;
	bool hasVertexColors;
	vector<Color4> vertexColors;
	ushort consistencyFlags;
	uint additionalData;
	BoundingSphere bounds;

public:
	ushort numVertices;
	vector<Vector3> vertices;
	vector<Vector3> normals;
	vector<Vector3> tangents;
	vector<Vector3> bitangents;

	ushort numUVSets;
	vector<Vector2> uvSets;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	void SetVertices(bool enable);
	bool HasVertices() { return hasVertices; }

	void SetNormals(bool enable);
	bool HasNormals() { return hasNormals; }

	void SetVertexColors(bool enable);
	bool HasVertexColors() { return hasVertexColors; }

	void SetUVs(bool enable);
	bool HasUVs() { return (numUVSets & (1 << 0)) != 0; }

	void SetTangents(bool enable);
	bool HasTangents() { return (numUVSets & (1 << 12)) != 0; }

	void SetBounds(const BoundingSphere& bounds) { this->bounds = bounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	virtual void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	virtual void CalcTangentSpace();
};

class NiShape : public NiAVObject {
private:
	NiGeometryData* GetGeomData() { return header->GetBlock<NiGeometryData>(GetDataRef()); }

public:
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	virtual int GetDataRef();
	virtual void SetDataRef(int dataRef);

	virtual int GetSkinInstanceRef();
	virtual void SetSkinInstanceRef(int skinInstanceRef);

	virtual int GetShaderPropertyRef();
	virtual void SetShaderPropertyRef(int shaderPropertyRef);

	virtual int GetAlphaPropertyRef();
	virtual void SetAlphaPropertyRef(int alphaPropertyRef);

	virtual void SetVertices(bool enable);
	virtual bool HasVertices();

	virtual void SetUVs(bool enable);
	virtual bool HasUVs();

	virtual void SetNormals(bool enable);
	virtual bool HasNormals();

	virtual void SetTangents(bool enable);
	virtual bool HasTangents();

	virtual void SetVertexColors(bool enable);
	virtual bool HasVertexColors();

	virtual void SetSkinned(bool enable);
	virtual bool IsSkinned();

	virtual void SetBounds(const BoundingSphere& bounds);
	virtual BoundingSphere GetBounds();
	virtual void UpdateBounds();

	int GetBoneID(const string& boneName);
};


class BSTriShape : public NiShape {
private:
	int skinInstanceRef;
	int shaderPropertyRef;
	int alphaPropertyRef;

	BoundingSphere bounds;

public:
	class BSVertexData {
	public:
		// Single- or half-precision depending on IsFullPrecision() being true
		Vector3 vert;
		float bitangentX;	// Maybe the dot product of the vert normal and the z-axis?

		Vector2 uv;

		byte normal[3];
		byte bitangentY;
		byte tangent[3];
		byte bitangentZ;

		byte colorData[4];

		float weights[4];
		byte weightBones[4];
	};

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

	vector<Vector3> rawVertices;	// filled by GetRawVerts function and returned.
	vector<Vector3> rawNormals;		// filled by GetNormalData function and returned.
	vector<Vector3> rawTangents;	// filled by CalcTangentSpace function and returned.
	vector<Vector3> rawBitangents;	// filled in CalcTangentSpace
	vector<Vector2> rawUvs;			// filled by GetUVData function and returned.

	vector<uint> deletedTris;		// temporary storage for BSSubIndexTriShape

	vector<BSVertexData> vertData;
	vector<Triangle> triangles;
	BSTriShape(NiHeader& hdr);
	BSTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();

	int GetSkinInstanceRef() { return skinInstanceRef; }
	void SetSkinInstanceRef(int skinInstanceRef) { this->skinInstanceRef = skinInstanceRef; }

	int GetShaderPropertyRef() { return shaderPropertyRef; }
	void SetShaderPropertyRef(int shaderPropertyRef) { this->shaderPropertyRef = shaderPropertyRef; }

	int GetAlphaPropertyRef() { return alphaPropertyRef; }
	void SetAlphaPropertyRef(int alphaPropertyRef) { this->alphaPropertyRef = alphaPropertyRef; }

	const vector<Vector3>* GetRawVerts();
	const vector<Vector3>* GetNormalData(bool xform = true);
	const vector<Vector3>* GetTangentData(bool xform = true);
	const vector<Vector3>* GetBitangentData(bool xform = true);
	const vector<Vector2>* GetUVData();

	void SetVertices(bool enable);
	bool HasVertices() { return (vertFlags6 & (1 << 4)) != 0; }

	void SetUVs(bool enable);
	bool HasUVs() { return (vertFlags6 & (1 << 5)) != 0; }

	void SetNormals(bool enable);
	bool HasNormals() { return (vertFlags6 & (1 << 7)) != 0; }

	void SetTangents(bool enable);
	bool HasTangents() { return (vertFlags7 & (1 << 0)) != 0; }

	void SetVertexColors(bool enable);
	bool HasVertexColors() { return (vertFlags7 & (1 << 1)) != 0; }

	void SetSkinned(bool enable);
	bool IsSkinned() { return (vertFlags7 & (1 << 2)) != 0; }

	void SetFullPrecision(bool enable);
	bool IsFullPrecision() { return (vertFlags7 & (1 << 6)) != 0; }
	bool CanChangePrecision() { return (HasVertices() && HasTangents() && HasNormals()); }

	void SetBounds(const BoundingSphere& bounds) { this->bounds = bounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	void SetNormals(const vector<Vector3>& inNorms);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();

	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals = nullptr);
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
		vector<BSSITSSubSegment> subSegments;
	};

	class BSSITSSubSegmentDataRecord {
	public:
		uint segmentUser = 0;
		uint unkInt2 = 0xFFFFFFFF;
		uint numData = 0;
		vector<float> extraData;
	};

	class BSSITSSubSegmentData {
	public:
		uint numSegments = 0;
		uint numTotalSegments = 0;
		vector<uint> arrayIndices;
		vector<BSSITSSubSegmentDataRecord> dataRecords;
		NiString ssfFile;
	};

	class BSSITSSegmentation {
	public:
		uint numPrimitives = 0;
		uint numSegments = 0;
		uint numTotalSegments = 0;
		vector<BSSITSSegment> segments;
		BSSITSSubSegmentData subSegmentData;
	};

private:
	BSSITSSegmentation segmentation;

public:
	BSSubIndexTriShape(NiHeader& hdr);
	BSSubIndexTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();

	BSSITSSegmentation GetSegmentation() { return segmentation; }
	void SetSegmentation(const BSSITSSegmentation& segmentation) { this->segmentation = segmentation; }

	void SetDefaultSegments();
	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals = nullptr);
};

class BSMeshLODTriShape : public BSTriShape {
public:
	uint lodSize0;
	uint lodSize1;
	uint lodSize2;

	BSMeshLODTriShape(NiHeader& hdr);
	BSMeshLODTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();

	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals = nullptr);
};

class NiGeometry : public NiShape {
private:
	int dataRef;
	int skinInstanceRef;
	int shaderPropertyRef;					// Version >= 20.2.0.7 && User Version == 12
	int alphaPropertyRef;					// Version >= 20.2.0.7 && User Version == 12

	uint numMaterials;
	vector<NiString> materialNames;
	vector<int> materialExtra;
	int activeMaterial;
	byte dirty;

public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	bool IsSkinned() { return skinInstanceRef != 0xFFFFFFFF; }

	int GetDataRef() { return dataRef; }
	void SetDataRef(int dataRef) { this->dataRef = dataRef; }

	int GetSkinInstanceRef() { return skinInstanceRef; }
	void SetSkinInstanceRef(int skinInstanceRef) { this->skinInstanceRef = skinInstanceRef; }

	int GetShaderPropertyRef() { return shaderPropertyRef; }
	void SetShaderPropertyRef(int shaderPropertyRef) { this->shaderPropertyRef = shaderPropertyRef; }

	int GetAlphaPropertyRef() { return alphaPropertyRef; }
	void SetAlphaPropertyRef(int alphaPropertyRef) { this->alphaPropertyRef = alphaPropertyRef; }
};

class NiTriBasedGeom : public NiGeometry {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiTriBasedGeomData : public NiGeometryData {
public:
	ushort numTriangles;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
};

class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape(NiHeader& hdr);
	NiTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiTriShapeData : public NiTriBasedGeomData {
private:
	bool hasTriangles;
	uint numTrianglePoints;
	ushort numMatchGroups;
	vector<MatchGroup> matchGroups;

public:
	vector<Triangle> triangles;

	NiTriShapeData(NiHeader& hdr);
	NiTriShapeData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcBlockSize();
};

class NiTriStrips : public NiTriBasedGeom {
public:
	NiTriStrips(NiHeader& hdr);
	NiTriStrips(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiTriStripsData : public NiTriBasedGeomData {
private:
	ushort numStrips;
	vector<ushort> stripLengths;
	byte hasPoints;
	vector<vector<ushort>> points;

public:
	NiTriStripsData(NiHeader& hdr);
	NiTriStripsData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void StripsToTris(vector<Triangle>* outTris);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcBlockSize();
};

class NiBoneContainer : public NiObject {
public:
	uint numBones;
	vector<int> bones;
};

class NiSkinInstance : public NiBoneContainer {
private:
	int dataRef;
	int skinPartitionRef;
	int skeletonRootRef;

public:
	NiSkinInstance() { };
	NiSkinInstance(NiHeader& hdr);
	NiSkinInstance(fstream& file, NiHeader& hdr);

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	int GetDataRef() { return dataRef; }
	void SetDataRef(int dataRef) { this->dataRef = dataRef; }

	int GetSkinPartitionRef() { return skinPartitionRef; }
	void SetSkinPartitionRef(int skinPartitionRef) { this->skinPartitionRef = skinPartitionRef; }

	int GetSkeletonRootRef() { return skeletonRootRef; }
	void SetSkeletonRootRef(int skeletonRootRef) { this->skeletonRootRef = skeletonRootRef; }
};

class BSDismemberSkinInstance : public NiSkinInstance {
public:
	struct Partition {
		short flags;
		short partID;
	};

	int numPartitions;
	vector<Partition> partitions;

	BSDismemberSkinInstance(NiHeader& hdr);
	BSDismemberSkinInstance(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSSkinInstance : public NiBoneContainer  {
private:
	int targetRef;
	int dataRef;

public:
	uint numVertices;
	vector<SkinWeight> vertexWeights;

	BSSkinInstance() : targetRef(0xFFFFFFFF), dataRef(0xFFFFFFFF), numVertices(0) { numBones = 0; };
	BSSkinInstance(NiHeader& hdr);
	BSSkinInstance(fstream& file, NiHeader& hdr);

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	int GetTargetRef() { return targetRef; }
	void SetTargetRef(int targetRef) { this->dataRef = targetRef; }

	int GetDataRef() { return dataRef; }
	void SetDataRef(int dataRef) { this->dataRef = dataRef; }
};

class BSSkinBoneData : public NiObject {
public:
	uint nBones;

	class BoneData {
	public:
		BoundingSphere bounds;
		SkinTransform boneTransform;

		BoneData() {
			boneTransform.scale = 1.0f;
		}
	};

	vector<BoneData> boneXforms;

	BSSkinBoneData() : nBones(0) { };
	BSSkinBoneData(NiHeader& hdr);
	BSSkinBoneData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class NiSkinData : public NiObject {
public:
	class BoneData {
	public:
		SkinTransform boneTransform;
		BoundingSphere bounds;
		ushort numVertices;
		vector<SkinWeight> vertexWeights;

		BoneData() {
			boneTransform.scale = 1.0f;
			numVertices = 0;
		}
	};

	SkinTransform skinTransform;
	uint numBones;
	byte hasVertWeights;
	vector<BoneData> bones;

	NiSkinData(NiHeader& hdr);
	NiSkinData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();
};

class NiSkinPartition : public NiObject {
public:
	class PartitionBlock {
	public:
		ushort numVertices;
		ushort numTriangles;
		ushort numBones;
		ushort numStrips;
		ushort numWeightsPerVertex;
		vector<ushort> bones;
		byte hasVertexMap;
		vector<ushort> vertexMap;
		byte hasVertexWeights;
		vector<VertexWeight> vertexWeights;
		vector<ushort> stripLengths;
		byte hasFaces;
		vector<vector<ushort>> strips;
		vector<Triangle> triangles;
		byte hasBoneIndices;
		vector<BoneIndices> boneIndices;
		ushort unkShort;					// User Version >= 12
	};

	uint numPartitions;
	vector<PartitionBlock> partitions;

	bool needsBuild;

	NiSkinPartition(NiHeader& hdr);
	NiSkinPartition(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(vector<int>& outDeletedIndices);
	int CalcBlockSize();
};

class NiInterpolator : public NiObject {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiKeyBasedInterpolator : public NiInterpolator {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiFloatInterpolator : public NiKeyBasedInterpolator {
public:
	float floatValue;
	int dataRef;

	NiFloatInterpolator(NiHeader& hdr);
	NiFloatInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiTransformInterpolator : public NiKeyBasedInterpolator {
public:
	Vector3 translation;
	Quaternion rotation;
	float scale;
	int dataRef;

	NiTransformInterpolator(NiHeader& hdr);
	NiTransformInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiPoint3Interpolator : public NiKeyBasedInterpolator {
public:
	Vector3 point3Value;
	int dataRef;

	NiPoint3Interpolator(NiHeader& hdr);
	NiPoint3Interpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiTimeController : public NiObject {
public:
	int nextControllerRef;
	uint flags;
	float frequency;
	float phase;
	float startTime;
	float stopTime;
	int targetRef;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiInterpController : public NiTimeController {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiSingleInterpController : public NiInterpController {
public:
	int interpolatorRef;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiFloatInterpController : public NiSingleInterpController {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
public:
	uint typeOfControlledColor;

	BSLightingShaderPropertyColorController(NiHeader& hdr);
	BSLightingShaderPropertyColorController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSLightingShaderPropertyFloatController : public NiFloatInterpController {
public:
	uint typeOfControlledVariable;

	BSLightingShaderPropertyFloatController(NiHeader& hdr);
	BSLightingShaderPropertyFloatController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSEffectShaderPropertyColorController : public NiFloatInterpController {
public:
	uint typeOfControlledColor;

	BSEffectShaderPropertyColorController(NiHeader& hdr);
	BSEffectShaderPropertyColorController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSEffectShaderPropertyFloatController : public NiFloatInterpController {
public:
	uint typeOfControlledVariable;

	BSEffectShaderPropertyFloatController(NiHeader& hdr);
	BSEffectShaderPropertyFloatController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiShader : public NiProperty {
public:
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyStringDelete(int stringID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	virtual bool IsSkinTint();
	virtual bool IsSkinned();
	virtual void SetSkinned(bool enable);
	virtual bool IsDoubleSided();
	virtual uint GetType();
	virtual void SetType(uint type);
	virtual Vector3 GetSpecularColor();
	virtual void SetSpecularColor(Vector3 color);
	virtual float GetSpecularStrength();
	virtual void SetSpecularStrength(float strength);
	virtual float GetGlossiness();
	virtual void SetGlossiness(float gloss);
	virtual int GetTextureSetRef();
	virtual void SetTextureSetRef(int texSetRef);
	virtual Color4 GetEmissiveColor();
	virtual void SetEmissiveColor(Color4 color);
	virtual float GetEmissiveMultiple();
	virtual void SetEmissiveMultiple(float emissive);
	virtual string GetWetMaterialName();
	virtual void SetWetMaterialName(const string& matName);
};

class BSLightingShaderProperty : public NiShader {
private:
	uint wetMaterialNameRef;
	string wetMaterialName;

public:
	uint shaderFlags1;						// User Version == 12
	uint shaderFlags2;						// User Version == 12
	Vector2 uvOffset;
	Vector2 uvScale;
	int textureSetRef;

	Vector3 emissiveColor;
	float emissiveMultiple;
	uint textureClampMode;
	float alpha;
	float refractionStrength;
	float glossiness;
	Vector3 specularColor;
	float specularStrength;
	float lightingEffect1;
	float lightingEffect2;					// User version == 12, userversion2 < 130
	uint unk1;								// user version == 12, userversion2 >= 130
	uint unk2;								// user version == 12, userversion2 >= 130

	float environmentMapScale;
	Vector3 skinTintColor;
	Vector3 hairTintColor;
	float maxPasses;
	float scale;
	float parallaxInnerLayerThickness;
	float parallaxRefractionScale;
	Vector2 parallaxInnerLayerTextureScale;
	float parallaxEnvmapStrength;
	Color4 sparkleParameters;
	float eyeCubemapScale;
	Vector3 eyeLeftReflectionCenter;
	Vector3 eyeRightReflectionCenter;

	float unk[8];						// unkown shader float params in shadertype 5 for fallout4
	byte  pad[16];						// up to 16 bytes of uknown padding.  clearly this isn't the right format.


	BSLightingShaderProperty(NiHeader& hdr);
	BSLightingShaderProperty(fstream& file, NiHeader& hdr);
	~BSLightingShaderProperty();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyStringDelete(int stringID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(bool enable);
	bool IsDoubleSided();
	uint GetType();
	void SetType(uint type);
	Vector3 GetSpecularColor();
	void SetSpecularColor(Vector3 color);
	float GetSpecularStrength();
	void SetSpecularStrength(float strength);
	float GetGlossiness();
	void SetGlossiness(float gloss);
	int GetTextureSetRef();
	void SetTextureSetRef(int texSetRef);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
	string GetWetMaterialName();
	void SetWetMaterialName(const string& matName);
};

class BSShaderProperty : public NiShader {
public:
	ushort smooth;
	uint shaderType;
	uint shaderFlags;
	uint shaderFlags2;
	float environmentMapScale;				// User Version == 11

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	uint GetType();
	void SetType(uint type);
};

class BSEffectShaderProperty : public NiShader {
public:
	uint shaderFlags1;
	uint shaderFlags2;
	Vector2 uvOffset;
	Vector2 uvScale;
	NiString sourceTexture;
	uint textureClampMode;
	float falloffStartAngle;
	float falloffStopAngle;
	float falloffStartOpacity;
	float falloffStopOpacity;
	Color4 emissiveColor;
	float emissiveMultiple;
	float softFalloffDepth;
	NiString greyscaleTexture;

	NiString envMapTexture;
	NiString normalTexture;
	NiString envMaskTexture;
	float envMapScale;

	BSEffectShaderProperty(NiHeader& hdr);
	BSEffectShaderProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(bool enable);
	bool IsDoubleSided();
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
};

class BSShaderLightingProperty : public BSShaderProperty {
public:
	uint textureClampMode;					// User Version <= 11

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class BSShaderPPLightingProperty : public BSShaderLightingProperty {
public:
	int textureSetRef;
	float refractionStrength;				// User Version == 11 && User Version 2 > 14
	int refractionFirePeriod;				// User Version == 11 && User Version 2 > 14
	float unkFloat4;						// User Version == 11 && User Version 2 > 24
	float unkFloat5;						// User Version == 11 && User Version 2 > 24
	Color4 emissiveColor;					// User Version >= 12

	BSShaderPPLightingProperty(NiHeader& hdr);
	BSShaderPPLightingProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(bool enable);
	int GetTextureSetRef();
	void SetTextureSetRef(int texSetRef);
};

class BSShaderTextureSet : public NiObject {
public:
	int numTextures;
	vector<NiString> textures;

	BSShaderTextureSet(NiHeader& hdr);
	BSShaderTextureSet(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class NiAlphaProperty : public NiProperty {
public:
	ushort flags;
	byte threshold;

	NiAlphaProperty(NiHeader& hdr);
	NiAlphaProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};


class NiMaterialProperty : public NiProperty {
public:
	Vector3 colorAmbient;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)
	Vector3 colorDiffuse;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)
	Vector3 colorSpecular;
	Vector3 colorEmissive;
	float glossiness;
	float alpha;
	float emitMulti;						// Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21

	NiMaterialProperty(NiHeader& hdr);
	NiMaterialProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();

	Vector3 GetSpecularColor();
	void SetSpecularColor(Vector3 color);
	float GetGlossiness();
	void SetGlossiness(float gloss);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
};

class NiStencilProperty : public NiProperty {
public:
	ushort flags;
	uint stencilRef;
	uint stencilMask;

	NiStencilProperty(NiHeader& hdr);
	NiStencilProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiExtraData : public NiObject {
private:
	uint nameRef;
	string name;

public:
	~NiExtraData();

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();

	string GetName();
	void SetName(const string& extraDataName);
};

class NiStringExtraData : public NiExtraData {
private:
	uint stringDataRef;
	string stringData;

public:
	NiStringExtraData(NiHeader& hdr);
	NiStringExtraData(fstream& file, NiHeader& hdr);
	~NiStringExtraData();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();

	string GetStringData();
	void SetStringData(const string& str);
};

class NiIntegerExtraData : public NiExtraData {
private:
	uint integerData;

public:
	NiIntegerExtraData(NiHeader& hdr);
	NiIntegerExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();

	uint GetIntegerData();
	void SetIntegerData(const uint& integerData);
};

class BSXFlags : public NiIntegerExtraData {
public:
	BSXFlags(NiHeader& hdr);
	BSXFlags(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSConnectPoint {
public:
	NiString root;
	NiString variableName;
	Quaternion rotation;
	Vector3 translation;
	float scale;

	BSConnectPoint();
	BSConnectPoint(fstream& file);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSConnectPointParents : public NiExtraData {
private:
	uint numConnectPoints;
	vector<BSConnectPoint> connectPoints;

public:
	BSConnectPointParents(NiHeader& hdr);
	BSConnectPointParents(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSConnectPointChildren : public NiExtraData {
private:
	byte unkByte;
	uint numTargets;
	vector<NiString> targets;

public:
	BSConnectPointChildren(NiHeader& hdr);
	BSConnectPointChildren(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSExtraData : public NiObject {
public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSClothExtraData : public BSExtraData {
public:
	uint numBytes;
	vector<char> data;

	BSClothExtraData();
	BSClothExtraData(NiHeader& hdr, uint size = 0);
	BSClothExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void Clone(BSClothExtraData* other);
	int CalcBlockSize();

	bool ToHKX(const string& fileName);
	bool FromHKX(const string& fileName);
};

class NiCollisionObject : public NiObject {
public:
	int target;

	NiCollisionObject(NiHeader& hdr);
	NiCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class bhkNiCollisionObject : public NiCollisionObject {
public:
	ushort flags;
	int body;

	bhkNiCollisionObject(NiHeader& hdr);
	bhkNiCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class bhkCollisionObject : public bhkNiCollisionObject {
public:
	bhkCollisionObject(NiHeader& hdr);
	bhkCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class bhkNPCollisionObject : public bhkCollisionObject {
public:
	uint unkInt;

	bhkNPCollisionObject(NiHeader& hdr);
	bhkNPCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class bhkPhysicsSystem : public BSExtraData {
public:
	uint numBytes;
	vector<char> data;

	bhkPhysicsSystem();
	bhkPhysicsSystem(NiHeader& hdr, uint size = 0);
	bhkPhysicsSystem(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();

	void Clone(bhkPhysicsSystem* other);
};

class NiUnknown : public NiObject {
public:
	vector<char> data;

	NiUnknown();
	NiUnknown(fstream& file, uint size);
	NiUnknown(uint size);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();

	void Clone(NiUnknown* other);
};

template <class T>
T* NiHeader::GetBlock(const int& blockId) {
	if (blockId >= 0 && blockId < numBlocks)
		return dynamic_cast<T*>((*blocks)[blockId]);

	return nullptr;
}
