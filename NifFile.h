#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <regex>
#include <math.h>
#include "KDMatcher.h"
#include "Object3d.h"

using namespace std;

#pragma warning (disable: 4018)

#ifndef EPSILON
	#define EPSILON (1.0E-4)
#endif


#define NIHEADER					0
#define NIUNKNOWN					1
#define NITRISHAPE					2
#define NITRISHAPEDATA				3
#define NINODE						4
#define BSDISMEMBERSKININSTANCE		5
#define NISKINPARTITION				6
#define BSLIGHTINGSHADERPROPERTY	7
#define NIALPHAPROPERTY				8
#define NISKINDATA					9
#define BSSHADERTEXTURESET			10
#define NITRISTRIPS					11
#define NITRISTRIPSDATA				12
#define NISKININSTANCE				13
#define NISTRINGEXTRADATA			14
#define BSSHADERPPLIGHTINGPROPERTY	15


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
};

struct MatchGroup {
	ushort count;
	vector<ushort> matches;
};


class NiString {
public:
	string str;
	bool outputNull;
	NiString(bool wantOutputNull = true);
	NiString(fstream& file, int szSize, bool wantNullOutput = true);
	void Put(fstream& file, int szSize);
	void Get(fstream& file, int szSize);
};

class NiHeader;

class NiObject {
protected:
	uint blockSize;
public:
	ushort blockType;

	virtual ~NiObject();

	virtual void notifyBlockDelete(int blockID, NiHeader& hdr);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);

	virtual void Get(fstream& file, NiHeader& hdr);
	virtual void Put(fstream& file, NiHeader& hdr);

	virtual int CalcBlockSize();
	virtual void SetBlockSize(int sz);
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
	// User Version 2:		83

	char verStr[0x26];
	byte unk1;
	byte ver1;
	byte ver2;
	byte ver3;
	byte ver4;
	byte endian;
	uint userVer;
	uint numBlocks;
	uint userVer2;
	NiString creator;
	NiString exportInfo1;
	NiString exportInfo2;
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

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	bool VerCheck(int v1, int v2, int v3, int v4);
};

class NiUnknown : public NiObject {
public:
	char* data;
	NiUnknown();
	NiUnknown(fstream& file, uint size);
	NiUnknown(uint size);
	virtual ~NiUnknown();
	void Clone(NiUnknown* other);
	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

};

class NiObjectNET : public NiObject {
public:
	uint skyrimShaderType;			// BSLightingShaderProperty && User Version >= 12
	string name;
	uint nameRef;
	int numExtraData;
	vector<int> extraDataRef;
	int controllerRef;
};

class NiAVObject : public NiObjectNET {
public:
	uint flags;
	ushort unkShort1;				// Version >= 20.2.0.7 && User Version >= 11 && User Version 2 > 26
	Vector3 translation;
	Vector3 rotation[3];
	float scale;
	uint numProperties;
	vector<int> propertiesRef;
	int collisionRef;
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
	int propertiesRef1;				// Version >= 20.2.0.7 && User Version == 12
	int propertiesRef2;				// Version >= 20.2.0.7 && User Version == 12
};

class NiTriBasedGeom : public NiGeometry {
};

class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape();
	NiTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);
};

class NiNode : public NiObject {
public:
	string nodeName;
	uint nameID;
	int numExtra;
	vector<int> extradata;
	int controllerRef;
	ushort flags;
	ushort unk;
	Vector3 translation;
	Vector3 rotation[3];
	float scale;
	uint numProps;
	vector<int> propRef;
	int collisionRef;
	uint numChildren;
	vector<int> children;
	uint numEffects;
	vector<int> effects;

	NiNode();
	NiNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);
	virtual int CalcBlockSize();
};


class NifBlockTriStrips : public NiObject {
public:
	string shapeName;
	uint nameID;
	int numExtra;
	vector<int> extradata;
	int controllerRef;
	ushort flags;
	ushort unkShort1;
	Vector3 translation;
	Vector3 rotation[3];
	float scale;
	uint numProperties;
	vector<int> properties;

	int collisionRef;
	int dataRef;
	int skinRef;
	uint numMat;
	vector<NiString> materialNames;
	vector<int> materialExtra;
	int activeMat;
	byte unkByte;
	byte dirty;
	// BSPropertiesRef array
	int propertiesRef1;
	int propertiesRef2;

	NifBlockTriStrips();
	NifBlockTriStrips(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);
};

class NifBlockTriStripsData : public NiObject {
public:
	int unkInt;
	ushort numverts;
	byte keepflags;
	byte compressflags;
	byte hasVertices;
	vector<Vector3> vertices;
	ushort numUVs;
	uint skyrimMaterial;
	byte hasNormals;
	vector<Vector3> normals;
	vector<Vector3> tangents;
	vector<Vector3> binormals;
	Vector3 Center;
	float Radius;
	byte hasVertColors;
	vector<Color4> vertcolors;
	vector<Vector2> uvs;
	ushort consistencyflags;
	uint additionaldata;
	ushort numTriangles;
	ushort numStrips;
	vector<ushort> stripLengths;
	byte hasPoints;
	vector<vector<ushort>>points;

	// virtual values not saved with nif, used for vertex position retrieval
	Vector3 virtOffset;
	float virtScale;
	bool scaleFromCenter;
	int myver;

	NifBlockTriStripsData();
	NifBlockTriStripsData(fstream& file, NiHeader &hdr);

	int CalcBlockSize();

	void Get(fstream& file, NiHeader &hdr);
	void Put(fstream& file, NiHeader &hdr);

	void StripsToTris(vector<Triangle>* outTris);

	// not supported yet -- relies on tri data, most of code will likely port from nitrishapedata, but not yet.
	void RecalcNormals();
	void CalcTangentSpace();

	void notifyVerticesDelete(const vector<ushort>& vertIndices);
};

class NifBlockTriShapeData : public NiObject {
public:
	int unkInt;
	ushort numverts;
	byte keepflags;
	byte compressflags;
	byte hasVertices;
	vector<Vector3> vertices;
	ushort numUVs;
	uint skyrimMaterial;
	byte hasNormals;
	vector<Vector3> normals;
	vector<Vector3> tangents;
	vector<Vector3> binormals;
	Vector3 Center;
	float Radius;
	byte hasVertColors;
	vector<Color4> vertcolors;
	vector<Vector2> uvs;
	ushort consistencyflags;
	uint additionaldata;
	ushort numTriangles;
	uint numTriPoints;
	byte hasTriangles;
	vector<Triangle> tris;
	ushort numMatchGroups;
	vector<MatchGroup> matchGroups;

	// virtual values not saved with nif, used for vertex position retrieval
	Vector3 virtOffset;
	float virtScale;

	bool scaleFromCenter;
	int myver;

	NifBlockTriShapeData();
	NifBlockTriShapeData(fstream& file, NiHeader &hdr);

	void Get(fstream& file, NiHeader &hdr);
	void Put(fstream& file, NiHeader &hdr);
	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
	void RecalcNormals();
	void CalcTangentSpace();
	int CalcBlockSize();
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
};

class NifBlockNiSkinInstance : public NiObject {
public:
	struct Partition{
		short flags;
		short partID;
	};
	int dataRef;
	int skinRef;
	int skeletonRoot;
	uint numBones;
	vector<int> bonePtrs;

	int CalcBlockSize();

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	NifBlockNiSkinInstance();
	NifBlockNiSkinInstance(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);
};

class NifBlockBSDismemberment : public NiObject {
public:
	struct Partition{
		short flags;
		short partID;
	};
	int dataRef;
	int skinRef;
	int skeletonRoot;
	uint numBones;
	vector<int> bonePtrs;
	int numPartitions;
	vector<Partition> partitions;

	NifBlockBSDismemberment();
	NifBlockBSDismemberment(fstream& file, NiHeader& hdr);

	int CalcBlockSize();

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);
};

class NifBlockNiSkinData : public NiObject {
public:
	class BoneData {
	public:
		SkinTransform boneTransform;
		Vector3 boundSphereOffset;
		float boundSphereRadius;
		ushort numVerts;
		vector<SkinWeight> skin_weights;
		BoneData() {
			boundSphereRadius = 0.0f;
			numVerts = 0;
		}
		int CalcSize() {
			return (70 + numVerts * 6);
		}

	};
	SkinTransform rootTransform;
	uint numBones;
	byte hasVertWeights;
	vector<BoneData> Bones;

	NifBlockNiSkinData();
	NifBlockNiSkinData(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();
};

class NifBlockNiSkinPartition : public NiObject {
public:
	class PartitionBlock {
	public:
		ushort numVertices;
		ushort numTriangles;
		ushort numBones;
		ushort numStrips;
		ushort numWeightsPerVert;
		vector<ushort> bones;
		byte hasVertMap;
		vector<ushort> vertMap;
		byte hasVertWeights;
		vector<VertexWeight> vertWeights;
		vector<ushort> stripLengths;
		byte hasFaces;
		vector<vector<ushort>>strips;
		vector<Triangle> tris;
		byte hasBoneIndices;
		vector<BoneIndices> boneindex;
		ushort unknown;
	};
	uint numPartitions;
	vector<PartitionBlock> partitionBlocks;

	bool needsBuild;
	int myver;

	NifBlockNiSkinPartition();
	NifBlockNiSkinPartition(fstream& file, NiHeader& hdr);

	int CalcBlockSize();

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(vector<int>& outDeletedIndices);
};

class NifBlockBSLightShadeProp : public NiObject {
public:
	uint shaderType;
	string shaderName;
	uint nameID;
	uint numExtraData;
	vector<int> extraData;
	int controllerRef;
	uint shaderFlags1;
	uint shaderFlags2;
	Vector2 uvOffset;
	Vector2 uvScale;
	int texsetRef;

	Vector3 emissiveClr;
	float emissivleMult;
	uint texClampMode;
	float alpha;
	float unk;
	float glossiness;
	Vector3 specClr;
	float specStr;
	float lightFX1;
	float lightFX2;

	float envMapScale;
	Vector3 skinTintClr;
	Vector3 hairTintClr;
	float maxPasses;
	float scale;
	float parallaxThickness;
	float parallaxRefrScale;
	Vector2 parallaxTexScale;
	float parallaxEnvMapStr;
	Color4 sparkleParams;
	float eyeCubeScale;
	Vector3 eyeLeftReflectCenter;
	Vector3 eyeRightReflectCenter;

	NifBlockBSLightShadeProp();
	NifBlockBSLightShadeProp(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void Clone(NifBlockBSLightShadeProp* Other);

	bool IsSkinShader();
	bool IsDoubleSided();
};

class NifBlockBSShaderTextureSet : public NiObject {
public:
	uint numTex;
	vector<NiString> textures;

	NifBlockBSShaderTextureSet();
	NifBlockBSShaderTextureSet(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	int CalcBlockSize();
};

class NifBlockAlphaProperty : public NiObject {
public:
	string alphaName;
	uint nameID;
	uint numExtraData;
	vector<int> extraData;
	int controllerRef;
	ushort flags;
	byte threshold;

	NifBlockAlphaProperty();
	NifBlockAlphaProperty(fstream& file, NiHeader& hdr);
	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	int CalcBlockSize();
};

class NifBlockStringExtraData : public NiObject {
public:
	uint nameID;
	string name;
	//int nextExtraData;
	//uint bytesRemaining;
	uint stringDataId;
	string stringData;

	NifBlockStringExtraData();
	NifBlockStringExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);
};

class NifBlockBSShadePPLgtProp : public NiObject {
public:
	string shaderName;
	uint nameID;
	uint numExtraData;
	vector<int> extraData;
	int controllerRef;
	ushort flags;
	uint shaderType;
	uint shaderFlags;
	int unkInt2;
	float envMapScale;
	int unkInt3;
	int texsetRef;
	float unkFloat2;
	int refractionPeriod;
	float unkFloat4;
	float unkFloat5;

	NifBlockBSShadePPLgtProp();
	NifBlockBSShadePPLgtProp(fstream& file, NiHeader& hdr);

	void notifyBlockDelete(int blockID, NiHeader& hdr);

	void Get(fstream& file, NiHeader& hdr);
	void Put(fstream& file, NiHeader& hdr);

	void Clone(NifBlockBSShadePPLgtProp* Other);

	bool IsSkinShader();
};

class NifFile
{
	string fileName;
	NiHeader hdr;
	vector<NiObject*> blocks;
	bool isValid;

	NiTriShape* shapeForName(const string& name, int dupIndex = 0);
	NifBlockTriStrips* stripsForName(const string& name, int dupIndex = 0);

	int shapeDataIdForName(const string& name, int& outBlockType);
	int shapeIdForName(const string& name);

	NiNode* nodeForName(const string& name);
	int nodeIdForName(const string& name);

	int shapeBoneIndex(const string& shapeName, const string& boneName);

public:
	NifFile();
	NifFile(NifFile& other);
	~NifFile();

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

	int ExportShapeObj(const string& filename, const string& shape, float scale = 1.0f, Vector3 offset = Vector3(0.0f, 0.0f, 0.0f));

	void Clear();

	void DeleteBlock(int blockIndex);
	void DeleteBlockByType(string typeStr);

	bool HasBlockType(string typeStr);
	ushort AddOrFindBlockTypeId(const string& blockTypeName);
	int FindStringId(const string& str);
	int AddOrFindStringId(const string& str);

	NifBlockBSLightShadeProp* GetShaderForShape(const string& shapeName);
	NifBlockBSShadePPLgtProp* GetShaderPPForShape(const string& shapeName);
	bool GetTextureForShape(const string& shapeName, string& outTexFile, int texIndex = 0);
	void SetTextureForShape(const string& shapeName, string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(string& nodeName, NifFile& srcNif);
	void CopyShader(const string& shapeDest, NifBlockBSLightShadeProp* srcShader, NifFile& srcNif, bool addAlpha);
	void CopyShaderPP(const string& shapeDest, NifBlockBSShadePPLgtProp* srcShader, NifFile& srcNif, bool addAlpha);
	void CopyShape(const string& shapeDest, NifFile& srcNif, const string& srcShape);
	// Copy strips is a duplicate of copy shape that works for TriStrips blocks. Copy shape will call copy strips as needed.
	void CopyStrips(const string& shapeDest, NifFile& srcNif, const string& srcShape);

	int GetShapeList(vector<string>& outList);
	void RenameShape(const string& oldName, const string& newName);
	void RenameDuplicateShape(const string& dupedShape);
	void SetNodeName(int blockID, const string& newName);

	int GetShaderList(vector<string>& outList);

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

	const vector<Vector3>* GetRawVertsForShape(const string& shapeName);
	const vector<Triangle>* GetTrisForShape(const string& shapeName);
	bool GetTrisForTriStripShape(const string& shapeName, vector<Triangle>* outTris);
	const vector<Vector3>* GetNormalsForShape(const string& shapeName);
	const vector<Vector2>* GetUvsForShape(const string& shapeName);
	bool GetUvsForShape(const string& shapeName, vector<Vector2>& outUvs);
	const vector<vector<int>>* GetSeamVertsForShape(const string& shapeName);
	bool GetVertsForShape(const string& shapeName, vector<Vector3>& outVerts);
	int GetVertCountForShape(const string& shapeName);
	void SetVertsForShape(const string& shapeName, const vector<Vector3>& verts);
	void SetUvsForShape(const string& shapeName, const vector<Vector2>& uvs);
	void SetNormalsForShape(const string& shapeName, const vector<Vector3>& norms);
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

	// Sets a phantom offset value in a nitrishapedata record, which will be added to vertex values when fetched later.
	// if sum is true, the offset is added to the current offset, allowing a series of independant changes.
	void VirtualOffsetShape(const string& shapeName, const Vector3& offset, bool sum = true);

	// Sets a phantom scale value in a NiTriShapeData record, which will be added to vertex values when fetched later.
	void VirtualScaleShape(const string& shapeName, float scale, bool fromCenter = true);

	Vector3 GetShapeVirtualOffset(const string& shapeName);
	void GetShapeVirtualScale(const string& shapeName, float& scale, bool& fromCenterFlag);

	void MoveVertex(const string& shapeName, const Vector3& pos, const int& id);
	void OffsetShape(const string& shapeName, const Vector3& offset, unordered_map<ushort, float>* mask = nullptr);
	void ScaleShape(const string& shapeName, const float& scale, unordered_map<ushort, float>* mask = nullptr);
	void RotateShape(const string& shapeName, const Vector3& angle, unordered_map<ushort, float>* mask = nullptr);

	void GetAlphaForShape(const string& shapeName, ushort& outFlags, byte& outThreshold);
	void SetAlphaForShape(const string& shapeName, ushort flags, ushort threshold);

	void DeleteShape(const string& shapeName);
	void DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices);

	int CalcShapeDiff(const string& shapeName, const vector<Vector3>* targetData, unordered_map<ushort, Vector3>& outDiffData, float scale = 1.0f);
	int ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<ushort, Vector3>& outDiffData);
	// Based on the skin partitioning spell from NifSkope's source. Uses a different enough algorithm that it generates
	// different automatic partitions, but vert and tri order is comparable.
	void BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition = 24);

	// Maintains the number of and makeup of skin partitions, but updates the weighting values
	void UpdateSkinPartitions(const string& shapeName);
};
