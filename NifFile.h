/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "KDMatcher.h"

#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <regex>

using namespace std;

#pragma warning (disable: 4018)

#ifndef EPSILON
	#define EPSILON (1.0E-4)
#endif

enum BlockType {
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
	BSCLOTHEXTRADATA
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
	SkinWeight(ushort i = 0, float w = 0.0f) : index(i), weight(w) {}
};

struct Color4 {
	float a;
	float r;
	float g;
	float b;

	Color4() {
		r = g = b = a = 0.0f;
	}
	Color4(float R, float G, float B, float A) {
		r = R;
		g = G;
		b = B;
		a = A;
	}

	Color4& operator *= (float val) {
		r *= val;
		g *= val;
		b *= val;
		a *= val;
		return *this;
	}
	Color4 operator * (float val) const {
		Color4 tmp = *this;
		tmp *= val;
		return tmp;
	}

	Color4& operator /= (float val) {
		r /= val;
		g /= val;
		b /= val;
		a /= val;
		return *this;
	}
	Color4 operator / (float val) const {
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
	Default,  EnvironmentMap, GlowShader, Heightmap,
	FaceTint, SkinTint, HairTint, ParallaxOccMaterial,
	WorldMultitexture, WorldMap1, Unknown10, MultiLayerParallax,
	Unknown12, WorldMap2, SparkleSnow, WorldMap3,
	EyeEnvmap, Unknown17, WorldMap4, WorldLODMultitexture
};

class NiString {
public:
	string str;
	bool outputNull;
	NiString(bool wantOutputNull = true);
	NiString(fstream& file, int szSize, bool wantNullOutput = true);
	void Put(fstream& file, int szSize, bool wantNullOutput = true);
	void Get(fstream& file, int szSize);
};

class NiHeader;

class NiObject {
protected:
	uint blockSize;
public:
	NiHeader* header;
	ushort blockType;

	virtual ~NiObject();

	virtual void Init();
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	virtual void Get(fstream& file);
	virtual void Put(fstream& file);

	virtual int CalcBlockSize();
};

class NiHeader : public NiObject {
public:
	/* Minimum supported */
	// Version:				20.2.0.7
	// User Version:		11
	// User Version 2:		26

	/* Maximum supported */
	// Version:				20.2.0.7
	// User Version:		12
	// User Version 2:		130

	char verStr[0x26];
	byte version1;
	byte version2;
	byte version3;
	byte version4;
	uint userVersion;
	uint userVersion2;

	byte unk1;
	byte endian;
	uint numBlocks;
	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
	NiString exportInfo3;
	ushort numBlockTypes;
	vector<NiString> blockTypes;
	vector<ushort> blockIndex;
	vector<uint> blockSizes;
	uint numStrings;
	uint maxStringLen;
	vector<NiString> strings;
	uint unkInt2;

	// Foreign reference to the blocks list in NifFile.
	vector<NiObject*>* blocks;

	NiHeader();
	void Clear();
	void SetVersion(const byte& v1, const byte& v2, const byte& v3, const byte& v4, const uint& userVer, const uint& userVer2);
	bool VerCheck(int v1, int v2, int v3, int v4, bool equal = false);

	void Get(fstream& file);
	void Put(fstream& file);
};


class NiObjectNET : public NiObject {
public:
	uint skyrimShaderType;					// BSLightingShaderProperty && User Version >= 12
	string name;
	uint nameRef;
	int numExtraData;
	vector<int> extraDataRef;
	int controllerRef;

	bool bBSLightingShaderProperty;

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
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

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();

	bool rotToEulerDegrees(float &Y, float& P, float& R) {
		float rx, ry, rz;
		bool canRot = false;
		if (rotation[0].z < 1.0)
		{
			if (rotation[0].z > -1.0)
			{
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
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiNode : public NiAVObject {
public:
	uint numChildren;
	vector<int> children;
	uint numEffects;
	vector<int> effects;

	NiNode(NiHeader& hdr);
	NiNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

// Fallout 4 geometry for non-skinned meshes. Mostly uses half floats for vertex data.
class BSTriShape : public NiAVObject {
public:	
	class TSVertData {
	public:
		Vector3 vert;			// Stored half-float, convert!
		float bitangentX;		// maybe the dotproduct of the vert normal and the z axis?
		Vector2 uv;				// Stored as half-float, convert!
		byte normal[3];
		byte bitangentY;
		byte tangent[3];		// only if flags[6] & 0x1 is true?   some kind of packed normal data ?
		byte bitangentZ;

		byte colorData[4];		// only if flags[6] & 0x2 is true
		float weights[4];		// stored in half-float, convert!
		byte weightBones[4];
	};

	uint unkProps[4];
	int skinInstanceRef;
	int shaderPropertyRef;
	int alphaPropertyRef;

	// flags for vert data look to be stored in here.  byte 0 or byte 6 specifically look promising .  
	//  using byte 6 currently, bit 3 indicating sub index data,  bit 2 indicating the presence of color data.  bit 1 indicating presence of normal data
	byte vertFlags[8];			
	uint numTriangles;
	ushort numVertices;
	uint dataSize;
	uint vertRecSize;				// size of vertex structure calculated with (datasize - (numtris*6)) / numverts;

	vector<Vector3> rawVertices;	// filled by GetRawVerts function and returned.
	vector<Vector3> rawNormals;		// filled by GetNormalData function and returned.
	vector<Vector3> rawTangents;	// filled by calcTangentSpace function and returned.
	vector<Vector3> rawBitangents;	// filled in calcTangentSpace
	vector<Vector2> rawUvs;			// filled by GetUVData function and returned.

	vector<TSVertData> vertData;
	vector<Triangle> triangles;
	BSTriShape(NiHeader& hdr);
	BSTriShape(fstream& file, NiHeader& hdr);
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);
	virtual int CalcBlockSize();

	const vector<Vector3>* GetRawVerts(bool xform = true);
	const vector<Vector3>* GetNormalData(bool xform = true);
	const vector<Vector3>* GetTangentData(bool xform = true);
	const vector<Vector3>* GetBitangentData(bool xform = true);
	const vector<Vector2>* GetUVData();

	void SetNormals(const vector<Vector3>& inNorms);
	void RecalcNormals(const bool& smooth = false, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals = nullptr);
};

// Fallout 4 geometry for non-skinned meshes. Mostly uses half floats for vertex data.
class BSSubIndexTriShape : public BSTriShape {
public:
	uint numTriangles2;
	uint numSubIndexRecordA;
	uint numSubIndexRecordB;

	vector<uint> subIndexRecordsA;
	 
	uint numSubIndexRecordA_2;
	uint numSubIndexRecordB_2;

	vector<uint> sequence;

	class subIndexRecordB {
	public:
		uint unk1;
		uint unk2;
		uint numExtra;
		vector <uint> extraData;
	};

	vector<subIndexRecordB> subIndexRecordsB;

	NiString ssfFile;

	BSSubIndexTriShape(NiHeader& hdr);
	BSSubIndexTriShape(fstream& file, NiHeader& hdr); 
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs, vector<Vector3>* normals=nullptr);
};

class NiGeometry : public NiAVObject {
public:
	int dataRef;
	int skinInstanceRef;
	uint numMaterials;
	vector<NiString> materialNames;
	vector<int> materialExtra;
	int activeMaterial;
	byte dirty;
	int propertiesRef1;						// Version >= 20.2.0.7 && User Version == 12
	int propertiesRef2;						// Version >= 20.2.0.7 && User Version == 12

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiGeometryData : public NiObject {
public:
	int unkInt;
	ushort numVertices;
	byte keepFlags;
	byte compressFlags;
	byte hasVertices;
	vector<Vector3> vertices;
	ushort numUVSets;
	//byte extraVectorsFlags;
	uint unkInt2;							// Version >= 20.2.0.7 && User Version == 12
	byte hasNormals;
	vector<Vector3> normals;
	vector<Vector3> tangents;
	vector<Vector3> bitangents;
	Vector3 center;
	float radius;
	byte hasVertexColors;
	vector<Color4> vertexColors;
	vector<Vector2> uvSets;
	ushort consistencyFlags;
	uint additionalData;

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual void RecalcNormals();
	virtual void CalcTangentSpace();
	virtual int CalcBlockSize();
};

class NiTriBasedGeom : public NiGeometry {
public:
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiTriBasedGeomData : public NiGeometryData {
public:
	ushort numTriangles;

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual void RecalcNormals();
	virtual void CalcTangentSpace();
	virtual int CalcBlockSize();
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
public:
	uint numTrianglePoints;
	byte hasTriangles;
	vector<Triangle> triangles;
	ushort numMatchGroups;
	vector<MatchGroup> matchGroups;

	NiTriShapeData(NiHeader& hdr);
	NiTriShapeData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void RecalcNormals();
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
public:
	ushort numStrips;
	vector<ushort> stripLengths;
	byte hasPoints;
	vector<vector<ushort>> points;

	NiTriStripsData(NiHeader& hdr);
	NiTriStripsData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void StripsToTris(vector<Triangle>* outTris);
	void RecalcNormals();
	void CalcTangentSpace();
	int CalcBlockSize();
};

class NiBoneContainer : public NiObject {
public:
	uint numBones;
	vector<int> bones;
};

class NiSkinInstance : public NiBoneContainer {
public:
	int dataRef;
	int skinPartitionRef;
	int skeletonRootRef;

	NiSkinInstance() { };
	NiSkinInstance(NiHeader& hdr);
	NiSkinInstance(fstream& file, NiHeader& hdr);

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
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
public:
	int targetRef;
	int boneDataRef;
	uint numVertices;
	vector<SkinWeight> vertexWeights;

	BSSkinInstance() : targetRef(-1), boneDataRef(-1), numVertices(0) { numBones = 0; };
	BSSkinInstance(NiHeader& hdr);
	BSSkinInstance(fstream& file, NiHeader& hdr);

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class BSSkinBoneData  : public NiObject {
public:
	uint nBones;
	class BoneData {
	public:
		Vector3 boundSphereOffset;
		float boundSphereRadius;
		SkinTransform boneTransform;

		BoneData() {
			boneTransform.scale = 1.0f;
			boundSphereRadius = 0.0f;
		}
		int CalcSize() {
			return (68);
		}
	};
	vector<BoneData> boneXforms;

	BSSkinBoneData() : nBones(0) { };
	BSSkinBoneData(NiHeader& hdr);
	BSSkinBoneData(fstream& file, NiHeader& hdr);

	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual int CalcBlockSize();
};

class NiSkinData : public NiObject {
public:
	class BoneData {
	public:
		SkinTransform boneTransform;
		Vector3 boundSphereOffset;
		float boundSphereRadius;
		ushort numVertices;
		vector<SkinWeight> vertexWeights;

		BoneData() {
			boneTransform.scale = 1.0f;
			boundSphereRadius = 0.0f;
			numVertices = 0;
		}
		int CalcSize() {
			return (70 + numVertices * 6);
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
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiKeyBasedInterpolator : public NiInterpolator {
public:
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
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

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiInterpController : public NiTimeController {
public:
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiSingleInterpController : public NiInterpController {
public:
	int interpolatorRef;

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class NiFloatInterpController : public NiSingleInterpController {
public:
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
public:
	uint typeOfControlledColor;

	BSLightingShaderPropertyColorController(NiHeader& hdr);
	BSLightingShaderPropertyColorController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiShader : public NiProperty {
public:
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual bool IsSkin();
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
	virtual uint GetWetMaterialNameRef();
	virtual void SetWetMaterialNameRef(uint matRef);
	virtual int CalcBlockSize();
};

class BSLightingShaderProperty : public NiShader {
public:
	uint shaderFlags1;						// User Version == 12
	uint shaderFlags2;						// User Version == 12
	Vector2 uvOffset;
	Vector2 uvScale;
	int textureSetRef;

	Vector3 emissiveColor;
	float emissiveMultiple;
	uint wetMaterialNameRef;
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

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	bool IsSkin();
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
	uint GetWetMaterialNameRef();
	void SetWetMaterialNameRef(uint matRef);
	int CalcBlockSize();
};

class BSShaderProperty : public NiShader {
public:
	ushort smooth;
	uint shaderType;
	uint shaderFlags;
	uint shaderFlags2;
	float environmentMapScale;				// User Version == 11

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual uint GetType();
	virtual void SetType(uint type);
	virtual int CalcBlockSize();
};

class BSEffectShaderProperty : public NiShader {
public:
	uint shaderFlags1;
	uint shaderFlags2;
	Vector2 uvOffset;
	Vector2 uvScale;
	NiString sourceTexture;
	uint textureClampMode;
	float falloffStartAngle;				// userversion2 < 130
	float falloffStopAngle;
	float falloffStartOpacity;
	float falloffStopOpacity;
	Color4 emissiveColor;
	float emissiveMultiple;
	float softFalloffDepth;
	NiString greyscaleTexture;

	//userversion2 >= 130
	float unkdata[11];
	NiString emissiveTex;
	NiString normalTex;
	NiString specularTex;
	float unkdata2[3];

	BSEffectShaderProperty(NiHeader& hdr);
	BSEffectShaderProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	bool IsSkin();
	bool IsDoubleSided();
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
	int CalcBlockSize();
};

class BSShaderLightingProperty : public BSShaderProperty {
public:
	uint textureClampMode;					// User Version <= 11

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual void notifyBlockDelete(int blockID);
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual int CalcBlockSize();
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	bool IsSkin();
	int GetTextureSetRef();
	void SetTextureSetRef(int texSetRef);
	int CalcBlockSize();
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	Vector3 GetSpecularColor();
	void SetSpecularColor(Vector3 color);
	float GetGlossiness();
	void SetGlossiness(float gloss);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
	int CalcBlockSize();
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
};

class NiExtraData : public NiObject {
public:
	uint nameRef;
	string name;

	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual int CalcBlockSize();
};

class NiStringExtraData : public NiExtraData {
public:
	uint stringDataRef;
	string stringData;

	NiStringExtraData(NiHeader& hdr);
	NiStringExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class BSExtraData : public NiObject {
public:
	virtual void Init();
	virtual void Get(fstream& file);
	virtual void Put(fstream& file);
	virtual int CalcBlockSize();
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
};

class NiUnknown : public NiObject {
public:
	vector<char> data;

	NiUnknown();
	NiUnknown(fstream& file, uint size);
	NiUnknown(uint size);
	void Get(fstream& file);
	void Put(fstream& file);
	void Clone(NiUnknown* other);
	int CalcBlockSize();
};

class NifFile
{
	string fileName;
	vector<NiObject*> blocks;
	bool isValid;
	bool hasUnknown;

	int shapeDataIdForName(const string& name, int& outBlockType);
	int shapeIdForName(const string& name);

	NiNode* nodeForName(const string& name);
	int nodeIdForName(const string& name);

	int shapeBoneIndex(const string& shapeName, const string& boneName);

public:
	NifFile();
	NifFile(NifFile& other);
	~NifFile();
	NiHeader hdr;

	void CopyFrom(NifFile& other);

	int AddBlock(NiObject* newBlock, const string& blockTypeName);
	NiObject* GetBlock(int blockId);
	int AddNode(const string& nodeName, vector<Vector3>& rot, Vector3& trans, float scale);
	string NodeName(int blockID);

	int AddStringExtraData(const string& shapeName, const string& name, const string& stringData);

	int Load(const string& filename);
	int Save(const string& filename);

	string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }
	bool HasUnknown() { return hasUnknown; }

	void Clear();

	// Explicitly sets the order of shapes to a new one.
	void SetShapeOrder(vector<string> order);

	// Sorts children block references under the root node so shapes appear first in the list, emulating the order created by nifskope.
	void PrettySortBlocks();

	// Swaps two blocks, updating references in other blocks that may refer to their old indices.
	void SwapBlocks(int blockIndexLo, int blockIndexHi);

	void DeleteBlock(int blockIndex);
	void DeleteBlockByType(string typeStr);

	bool HasBlockType(string typeStr);
	ushort AddOrFindBlockTypeId(const string& blockTypeName);
	int FindStringId(const string& str);
	int AddOrFindStringId(const string& str);

	NiTriBasedGeom* geomForName(const string& name, int dupIndex = 0);
	BSTriShape* geomForNameF4(const string& name, int dupIndex = 0);
	NiAVObject* avObjectForName(const string& name, int dupIndex = 0);

	NiShader* GetShader(const string& shapeName);
	NiShader* GetShaderF4(const string& shapeName);
	bool IsShaderSkin(const string& shapeName);
	NiMaterialProperty* GetMaterialProperty(const string& shapeName);

	int GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex = 0);
	void SetTextureForShape(const string& shapeName, string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(string& nodeName, NifFile& srcNif);
	void CopyShader(const string& shapeDest, int srcShaderRef, NifFile& srcNif, bool addAlpha, int propRef1, int propRef2);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, NiTriBasedGeom* geom);
	void CopyGeometry(const string& shapeDest, NifFile& srcNif, const string& srcShape, BSTriShape* geom);

	int GetShapeType(const string& shapeName);
	int GetShapeList(vector<string>& outList);
	void RenameShape(const string& oldName, const string& newName);
	void RenameDuplicateShape(const string& dupedShape);
	void SetNodeName(int blockID, const string& newName);

	/// GetChildren of a node ... templatized to allow any particular type to be queried.   useful for walking a node tree
	template <class T>
	vector<T*> GetChildren(NiNode* parent, bool searchExtraData = false);

	int GetNodeID(const string& nodeName);
	bool GetNodeTransform(const string& nodeName, vector<Vector3>& outRot, Vector3& outTrans, float& outScale);
	bool SetNodeTransform(const string& nodeName, SkinTransform& inXform);

	int GetShapeBoneList(const string& shapeName, vector<string>& outList);
	int GetShapeBoneIDList(const string& shapeName, vector<int>& outList);
	void SetShapeBoneIDList(const string& shapeName, vector<int>& inList);
	int GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& outWeights);

	// Empty string for the bone name returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, const string& boneName, SkinTransform& outXform, Vector3& outSphereOffset, float& outSphereRadius);
	// -1 for the bone index sets the overall skin transform for the shape.
	bool SetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& inXform, Vector3& inSphereOffset, float inSphereRadius);
	// -1 on the bone index returns the overall skin transform for the shape.
	bool GetShapeBoneTransform(const string& shapeName, int boneIndex, SkinTransform& outXform, Vector3& outSphereOffset, float& outSphereRadius);
	void UpdateShapeBoneID(const string& shapeName, int oldID, int newID);
	void SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<ushort, float>& inWeights);
	void SetShapeVertWeights(const string& shapeName, int vertIndex, vector<unsigned char>& boneids, vector<float>& weights);

	const vector<Vector3>* GetRawVertsForShape(const string& shapeName);
	bool GetTrisForShape(const string& shapeName, vector<Triangle>* outTris);
	const vector<Vector3>* GetNormalsForShape(const string& shapeName, bool transform = true);
	const vector<Vector3>* GetTangentsForShape(const string& shapeName, bool transform = true);
	const vector<Vector3>* GetBitangentsForShape(const string& shapeName, bool transform = true);
	const vector<Vector2>* GetUvsForShape(const string& shapeName);
	bool GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs);
	const vector<vector<int>>* GetSeamVertsForShape(const string& shapeName);
	bool GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts);
	int GetVertCountForShape(const string& shapeName);
	void SetVertsForShape(const string& shapeName, const vector<Vector3>& verts);
	void SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs);
	void SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms);
	void SmoothNormalsForShape(const string& shapeName);
	void CalcNormalsForShape(const string& shapeName);
	void CalcTangentsForShape(const string& shapeName);

	void ClearShapeTransform(const string& shapeName);
	void GetShapeTransform(const string& shapeName, Matrix4& outTransform);

	void ClearRootTransform();
	void GetRootTranslation(Vector3& outVec);
	void SetRootTranslation(const Vector3& newTrans);
	void GetRootScale(float& outScale);
	void SetRootScale(const float& newScale);

	void GetShapeTranslation(const string& shapeName, Vector3& outVec);
	void SetShapeTranslation(const string& shapeName, const Vector3& newTrans);
	void GetShapeScale(const string& shapeName, float& outScale);
	void SetShapeScale(const string& shapeName, const float& newScale);
	void ApplyShapeTranslation(const string& shapeName, const Vector3& offset);

	void MoveVertex(const string& shapeName, const Vector3& pos, const int& id);
	void OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const string& shapeName, const float& scale, unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask = nullptr);

	bool GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold);
	void SetAlphaForShape(const string& shapeName, ushort flags = 4844, ushort threshold = 128);

	void DeleteShape(const string& shapeName);
	void DeleteShader(const string& shapeName);
	void DeleteAlpha(const string& shapeName);
	void DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices);

	int CalcShapeDiff(const string& shapeName, const vector<Vector3>* targetData, unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<ushort, Vector3>& outDiffData);
	// Based on the skin partitioning spell from NifSkope's source. Uses a different enough algorithm that it generates
	// different automatic partitions, but vert and tri order is comparable.
	void BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition = 24);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const string& shapeName);
};

template <class T>
vector<T*> NifFile::GetChildren(NiNode* parent, bool searchExtraData) {
	vector<T*> result;
	T* n;
	if (parent == nullptr) {
		n = dynamic_cast<T*>(blocks[0]);
		if (n)
			result.push_back(n);
		return result;
	}

	for (int i = 0; i < parent->children.size(); i++) {
		n = dynamic_cast<T*>(GetBlock(parent->children[i]));
		if (n) {
			result.push_back(n);
		}
	}

	if (searchExtraData) {
		for (int i = 0; i < parent->extraDataRef.size(); i++) {
			n = dynamic_cast<T*>(GetBlock(parent->extraDataRef[i]));
			if (n) {
				result.push_back(n);
			}
		}
	}

	return result;
}
