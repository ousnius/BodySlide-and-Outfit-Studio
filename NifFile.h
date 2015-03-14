#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <regex>
#include <math.h>
#include "KDMatcher.h"
#include "object3d.h"

using namespace std;

#pragma warning (disable: 4018)

#ifndef EPSILON
	#define EPSILON (1.0E-4)
#endif


#define NIFBLOCK_HEADER				0
#define NIFBLOCK_UNKNOWN			1
#define NIFBLOCK_TRISHAPE			2
#define NIFBLOCK_TRISHAPEDATA		3
#define NIFBLOCK_NINODE				4
#define NIFBLOCK_BSDISMEMBER		5
#define NIFBLOCK_NISKINPARTITION	6
#define NIFBLOCK_BSLGTSHADEPROP		7
#define NIFBLOCK_ALPHAPROPERTY		8
#define NIFBLOCK_NISKINDATA			9
#define NIFBLOCK_BSSHADERTEXSET		10
#define NIFBLOCK_TRISTRIPS			11
#define NIFBLOCK_TRISTRIPSDATA		12
#define NIFBLOCK_NISKININSTANCE		13
#define NIFBLOCK_STRINGEXTRADATA	14

typedef unsigned char BYTE;
typedef unsigned int uint;
typedef unsigned short ushort;

struct vertexWeight {
	float w1;
	float w2;
	float w3;
	float w4;
};
struct boneIndices {
	BYTE i1;
	BYTE i2;
	BYTE i3;
	BYTE i4;
};

struct skin_transform {
	vector3 rotation[3];
	vector3 translation;
	float scale;
	skin_transform () {
		rotation[0].x = 1.0f;
		rotation[1].y = 1.0f;
		rotation[2].z = 1.0f;
		scale = 1.0f;
	}

	Mat4 ToMatrix() {
		Mat4 m;
		m[0]=rotation[0].x * scale; m[1]=rotation[0].y;			m[2]=rotation[0].z;			 m[3]=translation.x;	
		m[4]=rotation[1].x;			m[5]=rotation[1].y * scale; m[6]=rotation[1].z;			 m[7]=translation.y;
		m[8]=rotation[2].x;			m[9]=rotation[2].y;			 m[10]=rotation[2].z * scale;m[11]=translation.z;
		return m;
	}

	Mat4 ToMatrixSkin() {
		Mat4 m;
		// Set the rotation
		vec3 r1 = rotation[0];
		vec3 r2 = rotation[1];
		vec3 r3 = rotation[2];
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

struct skin_weight{
	ushort index;
	float weight;
	skin_weight(ushort i = 0, float w = 0.0f) : index(i), weight(w) {}
};

struct color4 {
	float a;
	float r;
	float g;
	float b;
};

struct matchGroup {
	int count;
	vector<ushort> matches;
};


class NifString {
public:
	string str;
	bool outputNull;
	NifString(bool wantOutputNull = true);
	NifString(fstream& file, int szSize, bool wantNullOutput = true);
	void Put(fstream& file, int szSize);
	void Get(fstream& file, int szSize);
};

class NifBlockHeader;

class NifBlock {
protected:
	unsigned int blockSize;
public:
	unsigned short blockType;

	virtual ~NifBlock();

	virtual void notifyBlockDelete(int blockID, NifBlockHeader& hdr);
	virtual void notifyVersionChange(unsigned int ver1, unsigned int ver2);
	virtual void notifyVerticesDelete(const vector<ushort>& vertIndices);

	virtual void Get(fstream& file, NifBlockHeader& hdr);
	virtual void Put(fstream& file, NifBlockHeader& hdr);

	virtual int CalcBlockSize();
	virtual void SetBlockSize(int sz);

};

class NifBlockHeader : public NifBlock {
public:
	char verStr [0x26];
	BYTE unk1;
	BYTE ver1;	BYTE ver2;	BYTE ver3;	BYTE ver4;	
	BYTE endian;
	unsigned int userVer;
	unsigned int numBlocks;
	unsigned int userVer2;
	NifString creator;
	NifString exportInfo1;
	NifString exportInfo2;
	unsigned short numBlockTypes;
	vector<NifString> blockTypes;
	vector<unsigned short> blockIndex;
	vector<unsigned int> blockSizes;
	unsigned int numStrings;
	unsigned int maxStringLen;
	vector<NifString> strings;
	unsigned int unk2;

	// foreign reference to the blocks list in NifFile.
	vector<NifBlock*>* blocks;

	NifBlockHeader();
	void Clear();

	void Get(fstream& file, NifBlockHeader& hdr) ;
	void Put(fstream& file, NifBlockHeader& hdr) ;

	bool VerCheck(int v1, int v2, int v3, int v4);

};
class NifBlockUnknown : public NifBlock {
public:
	char* data;
	NifBlockUnknown();
	NifBlockUnknown(fstream& file, unsigned int size);
	NifBlockUnknown(unsigned int size);
	virtual ~NifBlockUnknown();
	void Clone(NifBlockUnknown* other) ;
	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

};


class NifBlockNiNode : public NifBlock {
public:
	string nodeName;
	uint nameID;
	int numExtra;
	vector<int> extradata;
    int controllerRef;
    ushort flags;
    ushort unk;
    vector3 translation;
    vector3 rotation[3];
    float scale;
   // if(version == 11) {
		uint numProps;
		vector<int> propRef;
   // }
    int collisionRef;
	uint numChildren;
	vector<int> children;
	uint numEffects;
	vector<int> effects;

	NifBlockNiNode();
	NifBlockNiNode(fstream& file, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);
	void notifyVersionChange(unsigned int ver1, unsigned int ver2);
	virtual int CalcBlockSize();

};


class NifBlockTriStrips : public NifBlock {
public: 
	string shapeName;
	uint nameID;
	int numExtra;
	vector<int> extradata;
    int controllerRef;
    ushort flags;
    ushort unk;
    vector3 translation;
    vector3 rotation[3];
    float  scale;

    int collisionRef;
    int dataRef;
    int skinRef;
    uint numMat;
	vector <NifString> materialNames;
	vector <int> materialExtra;
    int activeMat;
    BYTE dirty;
   // BSPropertiesRef array
        int propertiesRef1;
        int propertiesRef2;
   // }
		////////////////
		
	NifBlockTriStrips();
	NifBlockTriStrips(fstream& file, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);
	void notifyVersionChange(unsigned int ver1, unsigned int ver2);
			///////////////
};

class NifBlockTriStripsData  : public NifBlock {
public:
	int unknown;
    ushort numverts;
    BYTE keepflags;
    BYTE compressflags;
    BYTE hasVertices;
	vector<vector3> vertices;  
    ushort numUVs;
	uint unknown2;
    BYTE hasNormals;
	vector<vector3> normals;  
	vector<vector3> tangents;  
	vector<vector3> binormals;  
    vector3 Center;
    float Radius;
    BYTE hasVertColors;
	vector<color4> vertcolors;
	vector<vector2> uvs;
    ushort consistencyflags;
    uint additionaldata;
    ushort numTriangles;
	ushort numStrips;
	vector<ushort> stripLengths;
	BYTE hasPoints;
	vector< vector<ushort> > points;
	///////////////
	
	// virtual values not saved with nif, used for vertex position retrieval
	vector3 virtOffset;
	float virtScale;
	bool scaleFromCenter;
	int myver;
	
	NifBlockTriStripsData();
	NifBlockTriStripsData(fstream& file, NifBlockHeader &hdr);

	int CalcBlockSize();

	void Get(fstream& file, NifBlockHeader &hdr);
	void Put(fstream& file, NifBlockHeader &hdr);

	void StripsToTris(vector<triangle>* outTris);

	// not supported yet -- relies on triangle data, most of code will likely port from nitrishapedata, but not yet.
	void RecalcNormals();
	void CalcTangentSpace();

	void notifyVersionChange(unsigned int ver1, unsigned int ver2);

	void notifyVerticesDelete(const vector<ushort>& vertIndices);
};


class NifBlockTriShape : public NifBlock {
public: 
	string shapeName;
	uint nameID;
	int numExtra;
	vector<int> extradata;
    int controllerRef;
    ushort flags;
    ushort unk;
    vector3 translation;
    vector3 rotation[3];
    float scale;
   // if(version == 11) {
		uint numProps;
		vector<int> propRef;
   // }
    int collisionRef;
    int dataRef;
    int skinRef;
    uint numMat;
	vector <NifString> materialNames;
	vector <int> materialExtra;
    int activeMat;
    BYTE dirty;
   // if(version > 11) {
        int propertiesRef1;
        int propertiesRef2;
   // }
		///////////////
		
	NifBlockTriShape();
	NifBlockTriShape(fstream& file, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);
	void notifyVersionChange(unsigned int ver1, unsigned int ver2);
};

class NifBlockTriShapeData : public NifBlock {
public:
	int unknown;
    ushort numverts;
    BYTE keepflags;
    BYTE compressflags;
    BYTE hasVertices;
	vector<vector3> vertices;  
    ushort numUVs;
	uint unknown2;
    BYTE hasNormals;
	vector<vector3> normals;  
	vector<vector3> tangents;  
	vector<vector3> binormals;  
    vector3 Center;
    float Radius;
    BYTE hasVertColors;
	vector<color4> vertcolors;
	vector<vector2> uvs;
    ushort consistencyflags;
    uint additionaldata;
    ushort numTriangles;
    uint numTriPoints;
    BYTE hasTriangles;
	vector<triangle> tris;
    ushort numMatchGroups;
	vector<matchGroup> matchGroups;
	
	// virtual values not saved with nif, used for vertex position retrieval
	vector3 virtOffset;
	float virtScale;

	bool scaleFromCenter;
	int myver;	

	///////////////
	NifBlockTriShapeData();
	NifBlockTriShapeData(fstream& file, NifBlockHeader &hdr);

	void Get(fstream& file, NifBlockHeader &hdr);
	void Put(fstream& file, NifBlockHeader &hdr);
	void Create(vector<vec3>* verts, vector<triangle>* tris, vector<vec2>* uvs);
	void RecalcNormals();
	void CalcTangentSpace();
	int CalcBlockSize();
	void notifyVersionChange(unsigned int ver1, unsigned int ver2);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
};

class NifBlockNiSkinInstance : public NifBlock {
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
	
	////////////////
	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	NifBlockNiSkinInstance();
	NifBlockNiSkinInstance(fstream& file,NifBlockHeader& hdr);

	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);

	////////////////
	

};

class NifBlockBSDismemberment : public NifBlock {
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

	////////////
	NifBlockBSDismemberment();
	NifBlockBSDismemberment(fstream& file,NifBlockHeader& hdr);

	int CalcBlockSize();

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);
	
	void notifyVersionChange(unsigned int ver1, unsigned int ver2);

	////////////	

};

class NifBlockNiSkinData: public NifBlock {
public:
	class BoneData {
	public:
		skin_transform boneTransform;
		vector3 boundSphereOffset;
		float boundSphereRadius;
		ushort numVerts;
		vector<skin_weight> skin_weights;
		BoneData() {
			boundSphereRadius = 0.0f;
			numVerts = 0;
		}
		int CalcSize() {
			return (70 + numVerts * 6);
		}

	};
	skin_transform rootTransform;
	uint numBones;
	BYTE hasVertWeights;
	vector<BoneData> Bones;

	NifBlockNiSkinData() ;
	NifBlockNiSkinData(fstream& file, NifBlockHeader& hdr);
	
	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();


};

class NifBlockNiSkinPartition : public NifBlock {
public:
	class PartitionBlock {
	public:
		ushort numVertices;
		ushort numTriangles;
		ushort numBones;
		ushort numStrips;
		ushort numWeightsPerVert;
		vector<ushort> bones;
		BYTE hasVertMap;
		vector<ushort> vertMap;
		BYTE hasVertWeights;
		vector<vertexWeight> vertWeights;
		vector<ushort> stripLengths;
		BYTE hasFaces;
		// if hasFaces && numStrips > 0
			vector< vector<ushort> >strips;
		// if hasFaces && numStrips == 0
			vector<triangle> tris;
		BYTE hasBoneIndices;
		vector<boneIndices> boneindex;
		// if(hdr.userVer == 12)
			ushort unknown;
	};
	uint numPartitions;
	vector<PartitionBlock> partitionBlocks;

	bool needsBuild;

	NifBlockNiSkinPartition();
	NifBlockNiSkinPartition(fstream& file, NifBlockHeader& hdr);

	int CalcBlockSize();

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void notifyVersionChange(unsigned int ver1, unsigned int ver2);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(vector<int>& outDeletedIndices);
};

class NifBlockBSLightShadeProp : public NifBlock {
public:
	uint shaderType;
	string shaderName;
	uint nameID;
	uint numExtraData;
	vector<int> extraData;
	int controllerRef;
	uint shaderFlags1;
	uint shaderFlags2;
	vector2 uvOffset;
	vector2 uvScale;
	/*		
	ushort flags;
	uint shaderType;
	uint shaderFlags;
	ushort unkShort1;
	int unkInt2;
	float envmapScale;
	int unkInt3;
	*/
	int texsetRef;

	vector3 emissiveClr;
	float emissivleMult;
	uint texClampMode;
	float alpha;
	float unk;
	float glossiness;
	vector3 specClr;
	float specStr;
	float lightFX1;
	float lightFX2;

	float envMapScale;
	vector3 skinTintClr;
	vector3 hairTintClr;
	float maxPasses;
	float scale;
	float parallaxThickness;
	float parallaxRefrScale;
	vector2 parallaxTexScale;
	float parallaxEnvMapStr;
	color4 sparkleParams;
	float eyeCubeScale;
	vector3 eyeLeftReflectCenter;
	vector3 eyeRightReflectCenter;

/*
	float unkFloat2;  
	int refractionPeriod;
	float unkFloat4;
	float unkFloat5;
	int unkInt1;
	float unkFloats[9];
	vector3 unkArg_5;
	float unkArg_1;	
	float unkArg_16[7];
	vector3 unkArg_6;
	float unkArg_11[5];
	color4 unkArg_14;
	float unkArg_7[2]; */

	NifBlockBSLightShadeProp();
	NifBlockBSLightShadeProp(fstream& file, NifBlockHeader& hdr);
	
	void notifyBlockDelete(int blockID, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);

	void Clone(NifBlockBSLightShadeProp* Other);
	void SetTexSetBlockID(int blockId);

	bool IsSkinShader();
	bool IsDoubleSided();
};

class NifBlockBSShaderTextureSet : public NifBlock {
public:
	uint numTex;
	vector<NifString> textures;

	NifBlockBSShaderTextureSet() ;
	NifBlockBSShaderTextureSet(fstream& file, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);	
	void Put(fstream& file, NifBlockHeader& hdr);

	int CalcBlockSize() ;

};

class NifBlockAlphaProperty : public NifBlock {
public:
	string alphaName;
	uint nameID;
	uint numExtraData;
	vector<int> extraData;
	int controllerRef;
	ushort flags;
	BYTE threshold;

	NifBlockAlphaProperty();
	void Get(fstream& file,NifBlockHeader& hdr);
	void Put(fstream& file,NifBlockHeader& hdr);

};

class NifBlockStringExtraData : public NifBlock {
public:
	uint nameID;
	string name;
	//int nextExtraData;
	//uint bytesRemaining;
	uint stringDataId;
	string stringData;

	NifBlockStringExtraData();
	NifBlockStringExtraData(fstream& file, NifBlockHeader& hdr);

	void Get(fstream& file, NifBlockHeader& hdr);
	void Put(fstream& file, NifBlockHeader& hdr);
};

class NifFile
{
	string fileName;
	NifBlockHeader hdr;
	vector<NifBlock*> blocks;
	bool isValid;

	NifBlockTriShape* shapeForName(const string& name, int dupIndex=0);
	NifBlockTriStrips* stripsForName(const string& name, int dupIndex=0);

	int shapeDataIdForName(const string& name, int& outBlockType);
	int shapeIdForName(const string& name);

	NifBlockNiNode* nodeForName(const string& name);
	int nodeIdForName(const string& name);

	int shapeBoneIndex(const string& shapeName, const string& boneName);

public:
	NifFile(void);
	NifFile(NifFile& other);
	~NifFile(void);

	void CopyFrom(NifFile& other);

	int AddBlock(NifBlock* newBlock, const string& blockTypeName);
	NifBlock* GetBlock(int blockId);	
	int AddNode(const string& nodeName, vector<vector3>& rot, vector3& trans, float scale);
	string NodeName(int blockID);

	int AddStringExtraData(const string& shapeName, const string& name, const string& stringData);
	
	int Load(const string& filename);
	int Save(const string& filename);

	string GetFileName() { return fileName; }

	bool IsValid() { return isValid; }

	int ExportShapeObj(string& filename,string& shape , float scale = 1.0f, vector3 offset = vector3(0.0f,0.0f,0.0f));

	void Clear();

	void DeleteBlock(int blockIndex);
	void DeleteBlockByType(string typeStr);

	bool HasBlockType(string typeStr);
	int AddOrFindBlockTypeId(const string& blockTypeName);
	int FindStringId(const string& str);
	int AddOrFindStringId(const string& str);

	NifBlockBSLightShadeProp*  GetShader(string& shaderName) ;
	NifBlockBSLightShadeProp*  GetShaderForShape(string& shapeName);
	bool GetTextureForShape(string& shapeName, string& outTexFile, int texIndex = 0);
	void SetTextureForShape(string& shapeName, string& inTexFile, int texIndex = 0);
	void TrimTexturePaths();

	int CopyNamedNode(string& nodeName, NifFile& srcNif);
	void CopyShader(const string& shapeDest, string& shaderName, NifFile& srcNif, bool addAlpha);
	void CopyShader(const string& shapeDest, NifBlockBSLightShadeProp* srcShader, NifFile& srcNif, bool addAlpha);
	void CopyShape(const string& shapeDest, NifFile& srcNif, const string& srcShape);
	// copy strips is a duplicate of copy shape that works for tri strips blocks.  copy shape will call copy strips as needed.
	void CopyStrips(const string& shapeDest, NifFile& srcNif, const string& srcShape);

	void RecalculateNormals();

	void SetUserVersions(unsigned int ver1, unsigned int ver2);

	int GetShapeList(vector<string>& outList);
	void RenameShape(const string& oldName, const string& newName);
	void RenameDuplicateShape(const string& dupedShape);
	void SetNodeName(int blockID, const string& newName);

	int GetShaderList(vector<string>& outList);
	
	int GetNodeID(const string& nodeName);
	bool GetNodeTransform(const string& nodeName, vector<vector3>& outRot, vector3& outTrans, float& outScale);
	bool SetNodeTransform(const string& nodeName, skin_transform& inXform);

	int GetShapeBoneList(const string& shapeName, vector<string>& outList);
	int GetShapeBoneIDList(const string& shapeName, vector<int>& outList);
	void SetShapeBoneIDList(const string& shapeName, vector<int>& inList);
	int GetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<int, float>& outWeights);
	
	// Empty string for the bone name returns the overall skin transform for the shape 
	bool GetShapeBoneTransform(const string& shapeName, const string& boneName, skin_transform& outXform, vector3& outSphereOffset, float& outSphereRadius);
	// -1 for the bone index sets the overall skin transform for the shape
	bool SetShapeBoneTransform(const string& shapeName, int boneIndex, skin_transform& inXform, vector3& inSphereOffset, float inSphereRadius);	
	// -1 on the bone index returns the overall skin transform for the shape
	bool GetShapeBoneTransform(const string& shapeName, int boneIndex, skin_transform& outXform, vector3& outSphereOffset, float& outSphereRadius);
	void UpdateShapeBoneID(const string& shapeName,int oldID, int newID);
	void SetShapeBoneWeights(const string& shapeName, int boneIndex, unordered_map<int, float>& inWeights);

	const vector<vector3>* GetRawVertsForShape(const string& shapeName);
	const vector<triangle>* GetTrisForShape(const string& shapeName);
	bool GetTrisForTriStripShape(const string& shapeName, vector<triangle>* outTris);
	const vector<vector3>* GetNormalsForShape(const string& shapeName);
	const vector<vector2>* GetUvsForShape(const string& shapeName);
	bool GetUvsForShape(const string& shapeName, vector<vector2>& outUvs);
	const vector<vector<int>>* GetSeamVertsForShape(const string& shapeName);
	bool GetVertsForShape(const string& shapeName, vector<vector3>& outVerts);
	int GetVertCountForShape(const string& shapeName);
	void SetVertsForShape(const string& shapeName, const vector<vector3>& verts) ;
	void SetUvsForShape(const string& shapeName, const vector<vector2>& uvs) ;
	void SetNormalsForShape(const string& shapeName, const vector<vector3>& norms);	
	void CalcTangentsForShape(const string& shapeName);
	
	void GetRootTranslation(vector3& outVec);
	void SetRootTranslation(vector3& newTrans);
	void GetRootScale(float& outScale);
	void SetRootScale(const float& newScale);

	void GetShapeTranslation(const string& shapeName, vector3& outVec);
	void SetShapeTranslation(const string& shapeName, vector3& newTrans);
	void GetShapeScale(const string& shapeName, float& outScale);
	void SetShapeScale(const string& shapeName, const float& newScale);
	void ApplyShapeTranslation(const string& shapeName, const vector3& offset);
	
	/* sets a phantom offset value in a nitrishapedata record, which will be added to vertex values when fetched later.  if sum is true, the offset is added to the current 
	   offset, allowing a series of independant changes
	*/
	void VirtualOffsetShape(const string& shapeName, const vector3& offset, bool sum = true);
	
	/* sets a phantom scale value in a nitrishapedata record, which will be added to vertex values when fetched later.
	*/
	void VirtualScaleShape(const string& shapeName, float scale, bool fromCenter = true);

	vector3 GetShapeVirtualOffset(const string& shapeName);
	void GetShapeVirtualScale(const string& shapeName, float& scale, bool& fromCenterFlag);

	void MoveVertex(const string& shapeName, const vector3& pos, const int& id);
	void OffsetShape(const string& shapeName, const vector3& offset, unordered_map<int, float>* mask = NULL);
	void ScaleShape(const string& shapeName, const float& scale, unordered_map<int, float>* mask = NULL);
	void RotateShape(const string& shapeName, const vec3& angle, unordered_map<int, float>* mask = NULL);
	
	void GetAlphaForShape(const string& shapeName, unsigned short& outFlags, BYTE& outThreshold);
	void SetAlphaForShape(const string& shapeName, unsigned short flags, unsigned short threshold);

	void DeleteShape(const string& shapeName);
	void DeleteVertsForShape(const string& shapeName, const vector<ushort>& indices);

	int CalcShapeDiff(const string& shapeName,const vector<vector3>* targetData, unordered_map<int,vector3>& outDiffData, float scale = 1.0f);
	int ShapeDiff(const string& baseShapeName, const string& targetShape, unordered_map<int, vector3>& outDiffData) ;
	/* Based on the skin partitioning spell from NifSkope's source.  Uses a diffferent enough algorithm that it generates
		different automatic partitions, but vert and triangle order is comparable */
	void BuildSkinPartitions(const string& shapeName, int maxBonesPerPartition = 24);

	/* Maintains the number of and makeup of skin partitions, but updates the weighting values */
	void UpdateSkinPartitions(const string& shapeName);
};
