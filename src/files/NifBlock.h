/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../utils/KDMatcher.h"

#include <fstream>
#include <unordered_map>
#include <set>
#include <string>

using namespace std;

enum BlockType : ushort {
	NIHEADER,
	NIUNKNOWN,
	NINODE,
	BSFADENODE,
	BSVALUENODE,
	BSLEAFANIMNODE,
	BSTREENODE,
	BSORDEREDNODE,
	BSMULTIBOUNDNODE,
	BSBLASTNODE,
	BSDAMAGESTAGE,
	BSMASTERPARTICLESYSTEM,
	NIBILLBOARDNODE,
	NISWITCHNODE,
	NITRISHAPE,
	NITRISHAPEDATA,
	NITRISTRIPS,
	NITRISTRIPSDATA,
	BSLODTRISHAPE,
	BSTRISHAPE,
	BSSUBINDEXTRISHAPE,
	BSMESHLODTRISHAPE,
	BSDYNAMICTRISHAPE,
	NISKININSTANCE,
	BSDISMEMBERSKININSTANCE,
	NISKINDATA,
	NISKINPARTITION,
	BSSKININSTANCE,
	BSBONEDATA,
	BSSHADERPPLIGHTINGPROPERTY,
	BSLIGHTINGSHADERPROPERTY,
	BSEFFECTSHADERPROPERTY,
	BSWATERSHADERPROPERTY,
	BSSKYSHADERPROPERTY,
	NIALPHAPROPERTY,
	NIMATERIALPROPERTY,
	NISTENCILPROPERTY,
	BSSHADERTEXTURESET,
	NIPARTICLESYSTEM,
	NIMESHPARTICLESYSTEM,
	BSSTRIPPARTICLESYSTEM,
	NIPARTICLESDATA,
	NIROTATINGPARTICLESDATA,
	NIPSYSDATA,
	NIMESHPSYSDATA,
	BSSTRIPPSYSDATA,
	NICAMERA,
	BSPSYSSTRIPUPDATEMODIFIER,
	NIPSYSAGEDEATHMODIFIER,
	BSPSYSLODMODIFIER,
	NIPSYSSPAWNMODIFIER,
	BSPSYSSIMPLECOLORMODIFIER,
	NIPSYSROTATIONMODIFIER,
	BSPSYSSCALEMODIFIER,
	NIPSYSGRAVITYMODIFIER,
	NIPSYSPOSITIONMODIFIER,
	NIPSYSBOUNDUPDATEMODIFIER,
	NIPSYSDRAGMODIFIER,
	BSPSYSINHERITVELOCITYMODIFIER,
	BSPSYSSUBTEXMODIFIER,
	NIPSYSBOMBMODIFIER,
	BSWINDMODIFIER,
	BSPSYSRECYCLEBOUNDMODIFIER,
	BSPSYSHAVOKUPDATEMODIFIER,
	NIPSYSSPHERICALCOLLIDER,
	NIPSYSPLANARCOLLIDER,
	NIPSYSCOLLIDERMANAGER,
	NIPSYSSPHEREEMITTER,
	NIPSYSCYLINDEREMITTER,
	NIPSYSBOXEMITTER,
	NIPSYSMESHEMITTER,
	BSLIGHTINGSHADERPROPERTYCOLORCONTROLLER,
	BSLIGHTINGSHADERPROPERTYFLOATCONTROLLER,
	BSEFFECTSHADERPROPERTYCOLORCONTROLLER,
	BSEFFECTSHADERPROPERTYFLOATCONTROLLER,
	BSFRUSTUMFOVCONTROLLER,
	BSLAGBONECONTROLLER,
	BSPROCEDURALLIGHTNINGCONTROLLER,
	NIBONELODCONTROLLER,
	NIFLOATEXTRADATACONTROLLER,
	NIVISCONTROLLER,
	NIALPHACONTROLLER,
	BSNIALPHAPROPERTYTESTREFCONTROLLER,
	NIKEYFRAMECONTROLLER,
	NITRANSFORMCONTROLLER,
	NIMULTITARGETTRANSFORMCONTROLLER,
	NIPSYSMODIFIERACTIVECTLR,
	NIPSYSEMITTERLIFESPANCTLR,
	NIPSYSEMITTERSPEEDCTLR,
	NIPSYSEMITTERINITIALRADIUSCTLR,
	NIPSYSEMITTERPLANARANGLECTLR,
	NIPSYSEMITTERDECLINATIONCTLR,
	NIPSYSGRAVITYSTRENGTHCTLR,
	NIPSYSINITIALROTSPEEDCTLR,
	NIPSYSEMITTERCTLR,
	NIPSYSMULTITARGETEMITTERCTLR,
	NICONTROLLERMANAGER,
	NISEQUENCE,
	NICONTROLLERSEQUENCE,
	NIDEFAULTAVOBJECTPALETTE,
	NIBLENDBOOLINTERPOLATOR,
	NIBLENDFLOATINTERPOLATOR,
	NIBLENDPOINT3INTERPOLATOR,
	NIBOOLINTERPOLATOR,
	NIBOOLTIMELINEINTERPOLATOR,
	NIFLOATINTERPOLATOR,
	NITRANSFORMINTERPOLATOR,
	NIPOINT3INTERPOLATOR,
	NIPATHINTERPOLATOR,
	NILOOKATINTERPOLATOR,
	NIPSYSUPDATECTLR,
	NIKEYFRAMEDATA,
	NITRANSFORMDATA,
	NIPOSDATA,
	NIBOOLDATA,
	NIFLOATDATA,
	NIBINARYEXTRADATA,
	NIFLOATEXTRADATA,
	NISTRINGEXTRADATA,
	NISTRINGSEXTRADATA,
	NIBOOLEANEXTRADATA,
	NIINTEGEREXTRADATA,
	BSXFLAGS,
	BSINVMARKER,
	BSFURNITUREMARKERNODE,
	BSDECALPLACEMENTVECTOREXTRADATA,
	BSBEHAVIORGRAPHEXTRADATA,
	BSBOUND,
	BSBONELODEXTRADATA,
	NITEXTKEYEXTRADATA,
	BSCLOTHEXTRADATA,
	BSCONNECTPOINTPARENTS,
	BSCONNECTPOINTCHILDREN,
	BSMULTIBOUND,
	BSMULTIBOUNDOBB,
	BSMULTIBOUNDAABB,
	NICOLLISIONOBJECT,
	BHKCOLLISIONOBJECT,
	BHKNPCOLLISIONOBJECT,
	BHKPCOLLISIONOBJECT,
	BHKSPCOLLISIONOBJECT,
	BHKBLENDCOLLISIONOBJECT,
	BHKPHYSICSSYSTEM,
	BHKPLANESHAPE,
	BHKCONVEXVERTICESSHAPE,
	BHKBOXSHAPE,
	BHKSPHERESHAPE,
	BHKTRANSFORMSHAPE,
	BHKCONVEXTRANSFORMSHAPE,
	BHKCAPSULESHAPE,
	BHKNITRISTRIPSSHAPE,
	BHKLISTSHAPE,
	BHKSIMPLESHAPEPHANTOM,
	BHKHINGECONSTRAINT,
	BHKLIMITEDHINGECONSTRAINT,
	BHKRAGDOLLCONSTRAINT,
	BHKBREAKABLECONSTRAINT,
	BHKSTIFFSPRINGCONSTRAINT,
	BHKBALLANDSOCKETCONSTRAINT,
	BHKBALLSOCKETCONSTRAINTCHAIN,
	BHKRIGIDBODY,
	BHKRIGIDBODYT,
	BHKCOMPRESSEDMESHSHAPE,
	BHKCOMPRESSEDMESHSHAPEDATA,
	BHKMOPPBVTREESHAPE
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

	bool IsIdentity() {
		return (translation.IsZero() && scale == 1.0f &&
			rotation[0] == Vector3(1.0f, 0.0f, 0.0f) &&
			rotation[1] == Vector3(0.0f, 1.0f, 0.0f) &&
			rotation[2] == Vector3(0.0f, 0.0f, 1.0f));
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

// Sort bone weights with indices
struct BoneWeightsSort {
	bool operator()(const SkinWeight& lhs, const SkinWeight& rhs) {
		if (lhs.weight == rhs.weight)
			return false;

		return rhs.weight < lhs.weight;
	}
};

struct NiQuatTransform {
	Vector3 translation;
	Quaternion rotation;
	float scale;
};

struct Color4 {
	float r;
	float g;
	float b;
	float a;

	Color4() {
		r = g = b = a = 0.0f;
	}
	Color4(const float& r, const float& g, const float& b, const float& a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	bool operator == (const Color4& other) {
		return (r == other.r && g == other.g && b == other.b && a == other.a);
	}
	bool operator != (const Color4& other) {
		return !(*this == other);
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

struct FurniturePosition {
	Vector3 offset;

	ushort orientation;			// User Version <= 11
	byte posRef1;				// User Version <= 11
	byte posRef2;				// User Version <= 11

	float heading;				// User Version >= 12
	ushort animationType;		// User Version >= 12
	ushort entryPoints;			// User Version >= 12
};

struct BoneLOD {
	uint distance;
	uint boneNameRef;
	string boneName;
};

struct TBC {
	float tension;
	float bias;
	float continuity;
};

template<typename T>
struct Key {
	float time;
	T value;
	T forward;
	T backward;
	TBC tbc;
};

template<typename T>
class KeyGroup {
private:
	uint numKeys;
	uint interpolation;
	vector<Key<T>> keys;

public:
	KeyGroup();
	KeyGroup(fstream& file);

	void Put(fstream& file);
	void Get(fstream& file);
	int CalcGroupSize();
};

struct ControllerLink {
	int interpolatorRef;
	int controllerRef;
	byte priority;

	uint nodeNameRef;
	string nodeName;

	uint propTypeRef;
	string propType;

	uint ctrlTypeRef;
	string ctrlType;

	uint ctrlIDRef;
	string ctrlID;

	uint interpIDRef;
	string interpID;
};

struct MotorDesc {
	float unkFloat[6];
	byte unkByte;
};

struct HingeDesc {
	Vector4 axleA;
	Vector4 axleInA1;
	Vector4 axleInA2;
	Vector4 pivotA;
	Vector4 axleB;
	Vector4 axleInB1;
	Vector4 axleInB2;
	Vector4 pivotB;
};

struct LimitedHingeDesc {
	HingeDesc hinge;
	float minAngle;
	float maxAngle;
	float maxFriction;
	bool enableMotor;
	MotorDesc motor;
};

struct RagdollDesc {
	Vector4 twistA;
	Vector4 planeA;
	Vector4 motorA;
	Vector4 pivotA;
	Vector4 twistB;
	Vector4 planeB;
	Vector4 motorB;
	Vector4 pivotB;
	float coneMaxAngle;
	float planeMinAngle;
	float planeMaxAngle;
	float twistMinAngle;
	float twistMaxAngle;
	float maxFriction;
	bool enableMotor;
	MotorDesc motor;
};

struct StiffSpringDesc {
	Vector4 pivotA;
	Vector4 pivotB;
	float length;
};

struct BallAndSocketDesc {
	Vector4 translationA;
	Vector4 translationB;
};

struct PrismaticDesc {
	Vector4 slidingA;
	Vector4 rotationA;
	Vector4 planeA;
	Vector4 pivotA;
	Vector4 slidingB;
	Vector4 rotationB;
	Vector4 planeB;
	Vector4 pivotB;
	float minDistance;
	float maxDistance;
	float friction;
	byte unkByte;
};

enum hkConstraintType : uint {
	BallAndSocket = 0,
	Hinge = 1,
	LimitedHinge = 2,
	Prismatic = 6,
	Ragdoll = 7,
	StiffSpring = 8
};

class SubConstraintDesc {
private:
	hkConstraintType type;
	uint numEntities;
	vector<int> entities;
	uint priority;

	BallAndSocketDesc desc1;
	HingeDesc desc2;
	LimitedHingeDesc desc3;
	PrismaticDesc desc4;
	RagdollDesc desc5;
	StiffSpringDesc desc6;

public:
	SubConstraintDesc();
	SubConstraintDesc(fstream& file);

	void Put(fstream& file);
	void Get(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcDescSize();
};

struct HavokFilter {
	byte layer;
	byte flagsAndParts;
	ushort group;
};

typedef uint HavokMaterial;

struct hkWorldObjCInfoProperty {
	uint data;
	uint size;
	uint capacityAndFlags;
};

struct bhkCMSDMaterial {
	HavokMaterial material;
	HavokFilter layer;
};

struct bhkCMSDBigTris {
	ushort triangle1;
	ushort triangle2;
	ushort triangle3;
	uint material;
	ushort weldingInfo;
};

struct bhkCMSDTransform {
	Vector4 translation;
	Quaternion rotation;
};

struct bhkCMSDChunk {
	Vector4 translation;
	uint matIndex;
	ushort reference;
	ushort transformIndex;

	uint numVerts;
	vector<ushort> verts;
	uint numIndices;
	vector<ushort> indices;
	uint numStrips;
	vector<ushort> strips;
	uint numWeldingInfo;
	vector<ushort> weldingInfo;
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

struct AVObject {
	NiString name;
	int objectRef;
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
	virtual void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	virtual void notifyStringDelete(int stringID);

	virtual void Get(fstream& file);
	virtual void Put(fstream& file);

	virtual void GetChildRefs(set<int>& refs);
	virtual int CalcBlockSize();
	virtual NiObject* Clone() { return new NiObject(*this); }
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
	bool VerCheck(const int& v1, const int& v2, const int& v3, const int& v4, const bool& equal = false);

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
	int ReplaceBlock(int oldBlockId, NiObject* newBlock, const string& blockTypeStr);

	// Swaps two blocks, updating references in other blocks that may refer to their old indices
	void SwapBlocks(const int& blockIndexLo, const int& blockIndexHi);
	bool IsBlockReferenced(const int& blockId);
	void DeleteUnreferencedBlocks(BlockType type = NIUNKNOWN, bool* hadDeletions = nullptr);

	ushort AddOrFindBlockTypeId(const string& blockTypeName);
	string GetBlockTypeStringById(const int& id);
	ushort GetBlockTypeIndex(const int& id);

	uint GetBlockSize(const uint& blockId) { return blockSizes[blockId]; }
	void CalcAllBlockSizes();

	int GetStringCount() {
		return strings.size();
	}

	int FindStringId(const string& str);
	int AddOrFindStringId(const string& str);
	string GetStringById(const int& id);
	void SetStringById(const int& id, const string& str);

	void AddStringRef(const int& id);
	void RemoveStringRef(const int& id);
	int RemoveUnusedStrings();

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
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);

	string GetName();
	void SetName(const string& propertyName, const bool& renameExisting = false);
	void ClearName();

	int GetControllerRef() { return controllerRef; }
	void SetControllerRef(int controllerRef) { this->controllerRef = controllerRef; }

	int GetNumExtraData() { return numExtraData; }
	int GetExtraDataRef(const int& id);
	void AddExtraDataRef(const int& id);

	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class NiAVObject : public NiObjectNET {
public:
	uint flags;
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
	void GetChildRefs(set<int>& refs);
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

class NiNode : public NiAVObject {
private:
	uint numChildren;
	vector<int> children;
	uint numEffects;
	vector<int> effects;

public:
	NiNode() { };
	NiNode(NiHeader& hdr);
	NiNode(fstream& file, NiHeader& hdr);

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);

	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiNode* Clone() { return new NiNode(*this); }

	int GetNumChildren() { return numChildren; }
	int GetChildRef(const int& id);
	void AddChildRef(const int& id);
	void ClearChildren();
	vector<int>::iterator ChildrenBegin() { return children.begin(); }
	vector<int>::iterator ChildrenEnd() { return children.end(); }

	int GetNumEffects() { return numEffects; }
	int GetEffectRef(const int& id);
	void AddEffectRef(const int& id);
};

class BSFadeNode : public NiNode {
public:
	BSFadeNode(NiHeader& hdr);
	BSFadeNode(fstream& file, NiHeader& hdr);
};

class BSValueNode : public NiNode {
private:
	int value;
	byte unkByte;

public:
	BSValueNode(NiHeader& hdr);
	BSValueNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	int CalcBlockSize();
	BSValueNode* Clone() { return new BSValueNode(*this); }
};

class BSLeafAnimNode : public NiNode {
public:
	BSLeafAnimNode(NiHeader& hdr);
	BSLeafAnimNode(fstream& file, NiHeader& hdr);
};

class BSTreeNode : public NiNode {
private:
	uint numBones1;
	vector<int> bones1;

	uint numBones2;
	vector<int> bones2;

public:
	BSTreeNode(NiHeader& hdr);
	BSTreeNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSTreeNode* Clone() { return new BSTreeNode(*this); }
};

class BSOrderedNode : public NiNode {
private:
	Vector4 alphaSortBound;
	bool isStaticBound;

public:
	BSOrderedNode(NiHeader& hdr);
	BSOrderedNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	int CalcBlockSize();
	BSOrderedNode* Clone() { return new BSOrderedNode(*this); }
};

class BSMultiBoundNode : public NiNode {
private:
	int multiBoundRef;
	uint cullingMode;

public:
	BSMultiBoundNode(NiHeader& hdr);
	BSMultiBoundNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSMultiBoundNode* Clone() { return new BSMultiBoundNode(*this); }
};

class BSBlastNode : public NiNode {
private:
	byte min;
	byte max;
	byte current;

public:
	BSBlastNode(NiHeader& hdr);
	BSBlastNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	int CalcBlockSize();
	BSBlastNode* Clone() { return new BSBlastNode(*this); }
};

class BSDamageStage : public BSBlastNode {
public:
	BSDamageStage(NiHeader& hdr);
	BSDamageStage(fstream& file, NiHeader& hdr);
};

class BSMasterParticleSystem : public NiNode {
private:
	ushort maxEmitterObjs;

	uint numParticleSys;
	vector<int> particleSysRefs;

public:
	BSMasterParticleSystem(NiHeader& hdr);
	BSMasterParticleSystem(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);

	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSMasterParticleSystem* Clone() { return new BSMasterParticleSystem(*this); }
};

class NiBillboardNode : public NiNode {
private:
	ushort billboardMode;

public:
	NiBillboardNode(NiHeader& hdr);
	NiBillboardNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	int CalcBlockSize();
	NiBillboardNode* Clone() { return new NiBillboardNode(*this); }
};

class NiSwitchNode : public NiNode {
private:
	ushort flags;
	uint index;

public:
	NiSwitchNode(NiHeader& hdr);
	NiSwitchNode(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);

	int CalcBlockSize();
	NiSwitchNode* Clone() { return new NiSwitchNode(*this); }
};

class NiGeometryData : public NiObject {
protected:
	bool isPSys = false;

private:
	int unkInt;
	byte compressFlags;
	bool hasVertices;
	uint materialCRC;						// Version >= 20.2.0.7 && User Version == 12
	bool hasNormals;
	bool hasVertexColors;
	ushort consistencyFlags;
	uint additionalData;
	BoundingSphere bounds;

public:
	ushort numVertices;
	vector<Vector3> vertices;
	vector<Vector3> normals;
	vector<Vector3> tangents;
	vector<Vector3> bitangents;
	vector<Color4> vertexColors;

	byte keepFlags;
	ushort numUVSets;
	vector<Vector2> uvSets;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();

	void SetVertices(const bool& enable);
	bool HasVertices() { return hasVertices; }

	void SetNormals(const bool& enable);
	bool HasNormals() { return hasNormals; }

	void SetVertexColors(const bool& enable);
	bool HasVertexColors() { return hasVertexColors; }

	void SetUVs(const bool& enable);
	bool HasUVs() { return (numUVSets & (1 << 0)) != 0; }

	void SetTangents(const bool& enable);
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
	virtual int GetDataRef();
	virtual void SetDataRef(int dataRef);

	virtual int GetSkinInstanceRef();
	virtual void SetSkinInstanceRef(int skinInstanceRef);

	virtual int GetShaderPropertyRef();
	virtual void SetShaderPropertyRef(int shaderPropertyRef);

	virtual int GetAlphaPropertyRef();
	virtual void SetAlphaPropertyRef(int alphaPropertyRef);

	virtual void SetVertices(const bool& enable);
	virtual bool HasVertices();

	virtual void SetUVs(const bool& enable);
	virtual bool HasUVs();

	virtual void SetNormals(const bool& enable);
	virtual bool HasNormals();

	virtual void SetTangents(const bool& enable);
	virtual bool HasTangents();

	virtual void SetVertexColors(const bool& enable);
	virtual bool HasVertexColors();

	virtual void SetSkinned(const bool& enable);
	virtual bool IsSkinned();

	virtual void SetBounds(const BoundingSphere& bounds);
	virtual BoundingSphere GetBounds();
	virtual void UpdateBounds();

	int GetBoneID(const string& boneName);
};


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

	uint eyeData;
};


class BSTriShape : public NiShape {
private:
	int skinInstanceRef;
	int shaderPropertyRef;
	int alphaPropertyRef;

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

	uint someDataSize;
	vector<Vector3> someVerts;
	vector<Triangle> someTris;

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
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSTriShape* Clone() { return new BSTriShape(*this); }

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

	void SetVertices(const bool& enable);
	bool HasVertices() { return (vertFlags6 & (1 << 4)) != 0; }

	void SetUVs(const bool& enable);
	bool HasUVs() { return (vertFlags6 & (1 << 5)) != 0; }

	void SetNormals(const bool& enable);
	bool HasNormals() { return (vertFlags6 & (1 << 7)) != 0; }

	void SetTangents(const bool& enable);
	bool HasTangents() { return (vertFlags7 & (1 << 0)) != 0; }

	void SetVertexColors(const bool& enable);
	bool HasVertexColors() { return (vertFlags7 & (1 << 1)) != 0; }

	void SetSkinned(const bool& enable);
	bool IsSkinned() { return (vertFlags7 & (1 << 2)) != 0; }

	void SetFullPrecision(const bool& enable);
	bool IsFullPrecision() { return (vertFlags7 & (1 << 6)) != 0 || header->GetUserVersion2() == 100; }
	bool CanChangePrecision() { return (HasVertices() && HasTangents() && HasNormals() && header->GetUserVersion2() != 100); }

	void SetBounds(const BoundingSphere& bounds) { this->bounds = bounds; }
	BoundingSphere GetBounds() { return bounds; }
	void UpdateBounds();

	void SetNormals(const vector<Vector3>& inNorms);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
	void UpdateFlags();
	int CalcDataSizes();

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
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();
	BSSubIndexTriShape* Clone() { return new BSSubIndexTriShape(*this); }

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
	int CalcBlockSize();
	BSMeshLODTriShape* Clone() { return new BSMeshLODTriShape(*this); }
};

class BSDynamicTriShape : public BSTriShape {
public:
	uint dynamicDataSize;
	vector<Vector4> dynamicData;

	BSDynamicTriShape(NiHeader& hdr);
	BSDynamicTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int CalcBlockSize();
	BSDynamicTriShape* Clone() { return new BSDynamicTriShape(*this); }

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
	void GetChildRefs(set<int>& refs);
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
};

class NiTriBasedGeomData : public NiGeometryData {
public:
	ushort numTriangles;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();

	void Create(vector<Vector3>* verts, vector<Triangle>* tris, vector<Vector2>* uvs);
};

class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape(NiHeader& hdr);
	NiTriShape(fstream& file, NiHeader& hdr);

	NiTriShape* Clone() { return new NiTriShape(*this); }
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
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
	void CalcTangentSpace();
	int CalcBlockSize();
	NiTriShapeData* Clone() { return new NiTriShapeData(*this); }
};

class NiTriStrips : public NiTriBasedGeom {
public:
	NiTriStrips(NiHeader& hdr);
	NiTriStrips(fstream& file, NiHeader& hdr);

	NiTriStrips* Clone() { return new NiTriStrips(*this); }
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
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	void StripsToTris(vector<Triangle>* outTris);
	void RecalcNormals(const bool& smooth = true, const float& smoothThres = 60.0f);
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
	BSLODTriShape(NiHeader& hdr);
	BSLODTriShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSLODTriShape* Clone() { return new BSLODTriShape(*this); }
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
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiSkinInstance* Clone() { return new NiSkinInstance(*this); }

	int GetDataRef() { return dataRef; }
	void SetDataRef(const int& dataRef) { this->dataRef = dataRef; }

	int GetSkinPartitionRef() { return skinPartitionRef; }
	void SetSkinPartitionRef(const int& skinPartitionRef) { this->skinPartitionRef = skinPartitionRef; }

	int GetSkeletonRootRef() { return skeletonRootRef; }
	void SetSkeletonRootRef(const int& skeletonRootRef) { this->skeletonRootRef = skeletonRootRef; }
};

class BSDismemberSkinInstance : public NiSkinInstance {
public:
	struct PartitionInfo {
		ushort flags;
		ushort partID;
	};

private:
	int numPartitions;
	vector<PartitionInfo> partitions;

public:
	BSDismemberSkinInstance(NiHeader& hdr);
	BSDismemberSkinInstance(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSDismemberSkinInstance* Clone() { return new BSDismemberSkinInstance(*this); }

	int GetNumPartitions() { return numPartitions; }
	vector<PartitionInfo> GetPartitions() { return partitions; }

	void AddPartition(const PartitionInfo& partition);
	void RemovePartition(const int& id);
	void ClearPartitions();

	void SetPartitions(const vector<PartitionInfo>& partitions) {
		this->partitions = partitions;
		this->numPartitions = partitions.size();
	}
};

class BSSkinInstance : public NiBoneContainer  {
private:
	int targetRef;
	int dataRef;

	uint numUnk;
	vector<Vector3> unk;

public:
	vector<SkinWeight> vertexWeights;

	BSSkinInstance() { Init(); };
	BSSkinInstance(NiHeader& hdr);
	BSSkinInstance(fstream& file, NiHeader& hdr);

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSSkinInstance* Clone() { return new BSSkinInstance(*this); }

	int GetTargetRef() { return targetRef; }
	void SetTargetRef(const int& targetRef) { this->targetRef = targetRef; }

	int GetDataRef() { return dataRef; }
	void SetDataRef(const int& dataRef) { this->dataRef = dataRef; }
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
	BSSkinBoneData* Clone() { return new BSSkinBoneData(*this); }
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
	NiSkinData* Clone() { return new NiSkinData(*this); }
};

class NiSkinPartition : public NiObject {
public:
	class PartitionBlock {
	public:
		ushort numVertices = 0;
		ushort numTriangles = 0;
		ushort numBones = 0;
		ushort numStrips = 0;
		ushort numWeightsPerVertex = 0;
		vector<ushort> bones;
		bool hasVertexMap = false;
		vector<ushort> vertexMap;
		bool hasVertexWeights = false;
		vector<VertexWeight> vertexWeights;
		vector<ushort> stripLengths;
		bool hasFaces = false;
		vector<vector<ushort>> strips;
		vector<Triangle> triangles;
		bool hasBoneIndices = false;
		vector<BoneIndices> boneIndices;

		ushort unkShort = 0;				// User Version >= 12
		byte vertFlags1 = 0;				// User Version >= 12, User Version 2 == 100, Number of uint elements in vertex data
		byte vertFlags2 = 0;				// User Version >= 12, User Version 2 == 100, 4 byte or 2 byte position data
		byte vertFlags3 = 0;				// User Version >= 12, User Version 2 == 100
		byte vertFlags4 = 0;				// User Version >= 12, User Version 2 == 100
		byte vertFlags5 = 0;				// User Version >= 12, User Version 2 == 100
		byte vertFlags6 = 0;				// User Version >= 12, User Version 2 == 100, Vertex, UVs, Normals
		byte vertFlags7 = 0;				// User Version >= 12, User Version 2 == 100, (Bi)Tangents, Vertex Colors, Skinning, Precision
		byte vertFlags8 = 0;				// User Version >= 12, User Version 2 == 100
		vector<Triangle> trueTriangles;		// User Version >= 12, User Version 2 == 100
	};

	uint numPartitions;
	uint dataSize;							// User Version >= 12, User Version 2 == 100
	uint vertexSize;						// User Version >= 12, User Version 2 == 100
	byte vertFlags1;						// User Version >= 12, User Version 2 == 100, Number of uint elements in vertex data
	byte vertFlags2;						// User Version >= 12, User Version 2 == 100, 4 byte or 2 byte position data
	byte vertFlags3;						// User Version >= 12, User Version 2 == 100
	byte vertFlags4;						// User Version >= 12, User Version 2 == 100
	byte vertFlags5;						// User Version >= 12, User Version 2 == 100
	byte vertFlags6;						// User Version >= 12, User Version 2 == 100, Vertex, UVs, Normals
	byte vertFlags7;						// User Version >= 12, User Version 2 == 100, (Bi)Tangents, Vertex Colors, Skinning, Precision
	byte vertFlags8;						// User Version >= 12, User Version 2 == 100
	uint numVertices;						// Not in file
	vector<BSVertexData> vertData;			// User Version >= 12, User Version 2 == 100
	vector<PartitionBlock> partitions;

	bool HasVertices() { return (vertFlags6 & (1 << 4)) != 0; }
	bool HasUVs() { return (vertFlags6 & (1 << 5)) != 0; }
	bool HasNormals() { return (vertFlags6 & (1 << 7)) != 0; }
	bool HasTangents() { return (vertFlags7 & (1 << 0)) != 0; }
	bool HasVertexColors() { return (vertFlags7 & (1 << 1)) != 0; }
	bool IsSkinned() { return (vertFlags7 & (1 << 2)) != 0; }
	bool IsFullPrecision() { return true; }

	NiSkinPartition(NiHeader& hdr);
	NiSkinPartition(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyVerticesDelete(const vector<ushort>& vertIndices);
	int RemoveEmptyPartitions(vector<int>& outDeletedIndices);
	int CalcBlockSize();
	NiSkinPartition* Clone() { return new NiSkinPartition(*this); }
};

class NiParticleSystem : public NiAVObject {
private:
	int dataRef;
	int skinInstanceRef;
	int shaderPropertyRef;
	int alphaPropertyRef;

	uint numMaterials;
	uint materialString;
	vector<uint> materials;

	uint activeMaterial;
	byte defaultMatNeedsUpdate;

	BoundingSphere bounds;
	byte vertFlags1;
	byte vertFlags2;
	byte vertFlags3;
	byte vertFlags4;
	byte vertFlags5;
	byte vertFlags6;
	byte vertFlags7;
	byte vertFlags8;

	ushort farBegin;
	ushort farEnd;
	ushort nearBegin;
	ushort nearEnd;

	int psysDataRef;

	bool isWorldSpace;
	uint numModifiers;
	vector<int> modifiers;

public:
	NiParticleSystem(NiHeader& hdr);
	NiParticleSystem(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiParticleSystem* Clone() { return new NiParticleSystem(*this); }
};

class NiMeshParticleSystem : public NiParticleSystem {
public:
	NiMeshParticleSystem(NiHeader& hdr);
	NiMeshParticleSystem(fstream& file, NiHeader& hdr);

	NiMeshParticleSystem* Clone() { return new NiMeshParticleSystem(*this); }
};

class BSStripParticleSystem : public NiParticleSystem {
public:
	BSStripParticleSystem(NiHeader& hdr);
	BSStripParticleSystem(fstream& file, NiHeader& hdr);

	BSStripParticleSystem* Clone() { return new BSStripParticleSystem(*this); }
};

class NiParticlesData : public NiGeometryData {
private:
	bool hasRadii;
	ushort numActive;
	bool hasSizes;
	bool hasRotations;
	bool hasRotationAngles;
	bool hasRotationAxes;
	bool hasTextureIndices;

	uint numSubtexOffsets;
	vector<Vector4> subtexOffsets;

	float aspectRatio;
	ushort aspectFlags;
	float speedToAspectAspect2;
	float speedToAspectSpeed1;
	float speedToAspectSpeed2;

public:
	NiParticlesData(NiHeader& hdr);
	NiParticlesData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiParticlesData* Clone() { return new NiParticlesData(*this); }
};

class NiRotatingParticlesData : public NiParticlesData {
public:
	NiRotatingParticlesData(NiHeader& hdr);
	NiRotatingParticlesData(fstream& file, NiHeader& hdr);

	NiRotatingParticlesData* Clone() { return new NiRotatingParticlesData(*this); }
};

class NiPSysData : public NiRotatingParticlesData {
private:
	bool hasRotationSpeeds;

public:
	NiPSysData(NiHeader& hdr);
	NiPSysData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysData* Clone() { return new NiPSysData(*this); }
};

class NiMeshPSysData : public NiPSysData {
private:
	uint defaultPoolSize;
	bool fillPoolsOnLoad;

	uint numGenerations;
	vector<uint> generationPoolSize;

	int nodeRef;

public:
	NiMeshPSysData(NiHeader& hdr);
	NiMeshPSysData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiMeshPSysData* Clone() { return new NiMeshPSysData(*this); }
};

class BSStripPSysData : public NiPSysData {
private:
	ushort maxPointCount;
	uint startCapSize;
	uint endCapSize;
	bool doZPrepass;

public:
	BSStripPSysData(NiHeader& hdr);
	BSStripPSysData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSStripPSysData* Clone() { return new BSStripPSysData(*this); }
};

class NiCamera : public NiAVObject {
private:
	ushort obsoleteFlags;
	float frustumLeft;
	float frustumRight;
	float frustumTop;
	float frustomBottom;
	float frustumNear;
	float frustumFar;
	bool useOrtho;
	float viewportLeft;
	float viewportRight;
	float viewportTop;
	float viewportBottom;
	float lodAdjust;

	int sceneRef;
	int screenPolygonsRef;
	int screenTexturesRef;

public:
	NiCamera(NiHeader& hdr);
	NiCamera(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiCamera* Clone() { return new NiCamera(*this); }
};

class NiPSysModifier : public NiObject {
private:
	uint nameRef;
	string name;
	uint order;
	int targetRef;
	bool isActive;

public:
	~NiPSysModifier();

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class BSPSysStripUpdateModifier : public NiPSysModifier {
private:
	float updateDeltaTime;

public:
	BSPSysStripUpdateModifier(NiHeader& hdr);
	BSPSysStripUpdateModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSPSysStripUpdateModifier* Clone() { return new BSPSysStripUpdateModifier(*this); }
};

class NiPSysAgeDeathModifier : public NiPSysModifier {
private:
	bool spawnOnDeath;
	int spawnModifierRef;

public:
	NiPSysAgeDeathModifier(NiHeader& hdr);
	NiPSysAgeDeathModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysAgeDeathModifier* Clone() { return new NiPSysAgeDeathModifier(*this); }
};

class BSPSysLODModifier : public NiPSysModifier {
private:
	Vector4 unkVector;

public:
	BSPSysLODModifier(NiHeader& hdr);
	BSPSysLODModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSPSysLODModifier* Clone() { return new BSPSysLODModifier(*this); }
};

class NiPSysSpawnModifier : public NiPSysModifier {
private:
	ushort numSpawnGenerations;
	float percentSpawned;
	ushort minSpawned;
	ushort maxSpawned;
	float spawnSpeedVariation;
	float spawnDirVariation;
	float lifeSpan;
	float lifeSpanVariation;

public:
	NiPSysSpawnModifier(NiHeader& hdr);
	NiPSysSpawnModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysSpawnModifier* Clone() { return new NiPSysSpawnModifier(*this); }
};

class BSPSysSimpleColorModifier : public NiPSysModifier {
private:
	float fadeInPercent;
	float fadeOutPercent;
	float color1EndPercent;
	float color2StartPercent;
	float color2EndPercent;
	float color3StartPercent;
	Color4 color1;
	Color4 color2;
	Color4 color3;

public:
	BSPSysSimpleColorModifier(NiHeader& hdr);
	BSPSysSimpleColorModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSPSysSimpleColorModifier* Clone() { return new BSPSysSimpleColorModifier(*this); }
};

class NiPSysRotationModifier : public NiPSysModifier {
private:
	float initialSpeed;
	float initialSpeedVariation;
	float initialAngle;
	float initialAngleVariation;
	bool randomSpeedSign;
	bool randomInitialAxis;
	Vector3 initialAxis;

public:
	NiPSysRotationModifier(NiHeader& hdr);
	NiPSysRotationModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysRotationModifier* Clone() { return new NiPSysRotationModifier(*this); }
};

class BSPSysScaleModifier : public NiPSysModifier {
private:
	uint numFloats;
	vector<float> floats;

public:
	BSPSysScaleModifier(NiHeader& hdr);
	BSPSysScaleModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSPSysScaleModifier* Clone() { return new BSPSysScaleModifier(*this); }
};

class NiPSysGravityModifier : public NiPSysModifier {
private:
	int gravityObjRef;
	Vector3 gravityAxis;
	float decay;
	float strength;
	uint forceType;
	float turbulence;
	float turbulenceScale;
	bool worldAligned;

public:
	NiPSysGravityModifier(NiHeader& hdr);
	NiPSysGravityModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysGravityModifier* Clone() { return new NiPSysGravityModifier(*this); }
};

class NiPSysPositionModifier : public NiPSysModifier {
public:
	NiPSysPositionModifier(NiHeader& hdr);
	NiPSysPositionModifier(fstream& file, NiHeader& hdr);

	NiPSysPositionModifier* Clone() { return new NiPSysPositionModifier(*this); }
};

class NiPSysBoundUpdateModifier : public NiPSysModifier {
private:
	ushort updateSkip;

public:
	NiPSysBoundUpdateModifier(NiHeader& hdr);
	NiPSysBoundUpdateModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysBoundUpdateModifier* Clone() { return new NiPSysBoundUpdateModifier(*this); }
};

class NiPSysDragModifier : public NiPSysModifier {
private:
	int parentRef;
	Vector3 dragAxis;
	float percentage;
	float range;
	float rangeFalloff;

public:
	NiPSysDragModifier(NiHeader& hdr);
	NiPSysDragModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysDragModifier* Clone() { return new NiPSysDragModifier(*this); }
};

class BSPSysInheritVelocityModifier : public NiPSysModifier {
private:
	int targetNodeRef;
	float changeToInherit;
	float velocityMult;
	float velocityVar;

public:
	BSPSysInheritVelocityModifier(NiHeader& hdr);
	BSPSysInheritVelocityModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSPSysInheritVelocityModifier* Clone() { return new BSPSysInheritVelocityModifier(*this); }
};

class BSPSysSubTexModifier : public NiPSysModifier {
private:
	float startFrame;
	float startFrameVariation;
	float endFrame;
	float loopStartFrame;
	float loopStartFrameVariation;
	float frameCount;
	float frameCountVariation;

public:
	BSPSysSubTexModifier(NiHeader& hdr);
	BSPSysSubTexModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSPSysSubTexModifier* Clone() { return new BSPSysSubTexModifier(*this); }
};

class NiPSysBombModifier : public NiPSysModifier {
private:
	int bombNodeRef;
	Vector3 bombAxis;
	float decay;
	float deltaV;
	uint decayType;
	uint symmetryType;

public:
	NiPSysBombModifier(NiHeader& hdr);
	NiPSysBombModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysBombModifier* Clone() { return new NiPSysBombModifier(*this); }
};

class BSWindModifier : public NiPSysModifier {
private:
	float strength;

public:
	BSWindModifier(NiHeader& hdr);
	BSWindModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSWindModifier* Clone() { return new BSWindModifier(*this); }
};

class BSPSysRecycleBoundModifier : public NiPSysModifier {
private:
	Vector3 boundOffset;
	Vector3 boundExtent;
	int targetNodeRef;

public:
	BSPSysRecycleBoundModifier(NiHeader& hdr);
	BSPSysRecycleBoundModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSPSysRecycleBoundModifier* Clone() { return new BSPSysRecycleBoundModifier(*this); }
};

class BSPSysHavokUpdateModifier : public NiPSysModifier {
private:
	uint numNodes;
	vector<int> nodeRefs;

	int modifierRef;

public:
	BSPSysHavokUpdateModifier(NiHeader& hdr);
	BSPSysHavokUpdateModifier(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSPSysHavokUpdateModifier* Clone() { return new BSPSysHavokUpdateModifier(*this); }
};

class NiPSysCollider : public NiObject {
private:
	float bounce;
	bool spawnOnCollide;
	bool dieOnCollide;
	int spawnModifierRef;
	int managerRef;
	int nextColliderRef;
	int colliderNodeRef;

public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class NiPSysSphericalCollider : public NiPSysCollider {
private:
	float radius;

public:
	NiPSysSphericalCollider(NiHeader& hdr);
	NiPSysSphericalCollider(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysSphericalCollider* Clone() { return new NiPSysSphericalCollider(*this); }
};

class NiPSysPlanarCollider : public NiPSysCollider {
private:
	float width;
	float height;
	Vector3 xAxis;
	Vector3 yAxis;

public:
	NiPSysPlanarCollider(NiHeader& hdr);
	NiPSysPlanarCollider(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysPlanarCollider* Clone() { return new NiPSysPlanarCollider(*this); }
};

class NiPSysColliderManager : public NiPSysModifier {
private:
	int colliderRef;

public:
	NiPSysColliderManager(NiHeader& hdr);
	NiPSysColliderManager(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysColliderManager* Clone() { return new NiPSysColliderManager(*this); }
};

class NiPSysEmitter : public NiPSysModifier {
private:
	float speed;
	float speedVariation;
	float declination;
	float declinationVariation;
	float planarAngle;
	float planarAngleVariation;
	Color4 color;
	float radius;
	float radiusVariation;
	float lifeSpan;
	float lifeSpanVariation;

public:
	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class NiPSysVolumeEmitter : public NiPSysEmitter {
private:
	int emitterNodeRef;

public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class NiPSysSphereEmitter : public NiPSysVolumeEmitter {
private:
	float radius;

public:
	NiPSysSphereEmitter(NiHeader& hdr);
	NiPSysSphereEmitter(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysSphereEmitter* Clone() { return new NiPSysSphereEmitter(*this); }
};

class NiPSysCylinderEmitter : public NiPSysVolumeEmitter {
private:
	float radius;
	float height;

public:
	NiPSysCylinderEmitter(NiHeader& hdr);
	NiPSysCylinderEmitter(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysCylinderEmitter* Clone() { return new NiPSysCylinderEmitter(*this); }
};

class NiPSysBoxEmitter : public NiPSysVolumeEmitter {
private:
	float width;
	float height;
	float depth;

public:
	NiPSysBoxEmitter(NiHeader& hdr);
	NiPSysBoxEmitter(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPSysBoxEmitter* Clone() { return new NiPSysBoxEmitter(*this); }
};

class NiPSysMeshEmitter : public NiPSysEmitter {
private:
	uint numMeshes;
	vector<int> meshRefs;
	uint velocityType;
	uint emissionType;
	Vector3 emissionAxis;

public:
	NiPSysMeshEmitter(NiHeader& hdr);
	NiPSysMeshEmitter(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysMeshEmitter* Clone() { return new NiPSysMeshEmitter(*this); }
};

class NiInterpolator : public NiObject {
};

class NiBlendInterpolator : public NiInterpolator {
private:
	ushort flags;
	uint unkInt;

public:
	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class NiBlendBoolInterpolator : public NiBlendInterpolator {
private:
	bool value;

public:
	NiBlendBoolInterpolator(NiHeader& hdr);
	NiBlendBoolInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBlendBoolInterpolator* Clone() { return new NiBlendBoolInterpolator(*this); }
};

class NiBlendFloatInterpolator : public NiBlendInterpolator {
private:
	float value;

public:
	NiBlendFloatInterpolator(NiHeader& hdr);
	NiBlendFloatInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBlendFloatInterpolator* Clone() { return new NiBlendFloatInterpolator(*this); }
};

class NiBlendPoint3Interpolator : public NiBlendInterpolator {
private:
	Vector3 point;

public:
	NiBlendPoint3Interpolator(NiHeader& hdr);
	NiBlendPoint3Interpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBlendPoint3Interpolator* Clone() { return new NiBlendPoint3Interpolator(*this); }
};

class NiKeyBasedInterpolator : public NiInterpolator {
};

class NiBoolInterpolator : public NiKeyBasedInterpolator {
private:
	byte boolValue;
	int dataRef;

public:
	NiBoolInterpolator(NiHeader& hdr);
	NiBoolInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiBoolInterpolator* Clone() { return new NiBoolInterpolator(*this); }
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {
public:
	NiBoolTimelineInterpolator(NiHeader& hdr);
	NiBoolTimelineInterpolator(fstream& file, NiHeader& hdr);

	NiBoolTimelineInterpolator* Clone() { return new NiBoolTimelineInterpolator(*this); }
};

class NiFloatInterpolator : public NiKeyBasedInterpolator {
private:
	float floatValue;
	int dataRef;

public:
	NiFloatInterpolator(NiHeader& hdr);
	NiFloatInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiFloatInterpolator* Clone() { return new NiFloatInterpolator(*this); }
};

class NiTransformInterpolator : public NiKeyBasedInterpolator {
private:
	Vector3 translation;
	Quaternion rotation;
	float scale;
	int dataRef;

public:
	NiTransformInterpolator(NiHeader& hdr);
	NiTransformInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiTransformInterpolator* Clone() { return new NiTransformInterpolator(*this); }
};

class NiPoint3Interpolator : public NiKeyBasedInterpolator {
private:
	Vector3 point3Value;
	int dataRef;

public:
	NiPoint3Interpolator(NiHeader& hdr);
	NiPoint3Interpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPoint3Interpolator* Clone() { return new NiPoint3Interpolator(*this); }
};

class NiPathInterpolator : public NiKeyBasedInterpolator {
private:
	ushort flags;
	uint bankDir;
	float maxBankAngle;
	float smoothing;
	ushort followAxis;

	int pathDataRef;
	int percentDataRef;

public:
	NiPathInterpolator(NiHeader& hdr);
	NiPathInterpolator(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPathInterpolator* Clone() { return new NiPathInterpolator(*this); }
};

class NiLookAtInterpolator : public NiInterpolator {
private:
	ushort flags;
	int lookAtRef;

	uint lookAtNameRef;
	string lookAtName;

	NiQuatTransform transform;
	int translateInterpRef;
	int rollInterpRef;
	int scaleInterpRef;

public:
	NiLookAtInterpolator(NiHeader& hdr);
	NiLookAtInterpolator(fstream& file, NiHeader& hdr);
	~NiLookAtInterpolator();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiLookAtInterpolator* Clone() { return new NiLookAtInterpolator(*this); }
};

class NiKeyframeData : public NiObject {
private:
	uint numRotationKeys;
	uint rotationType;
	vector<Key<Quaternion>> quaternionKeys;
	KeyGroup<float> xRotations;
	KeyGroup<float> yRotations;
	KeyGroup<float> zRotations;
	KeyGroup<Vector3> translations;
	KeyGroup<float> scales;

public:
	NiKeyframeData(NiHeader& hdr);
	NiKeyframeData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiKeyframeData* Clone() { return new NiKeyframeData(*this); }
};

class NiTransformData : public NiKeyframeData {
public:
	NiTransformData(NiHeader& hdr);
	NiTransformData(fstream& file, NiHeader& hdr);

	NiTransformData* Clone() { return new NiTransformData(*this); }
};

class NiPosData : public NiObject {
private:
	KeyGroup<Vector3> data;

public:
	NiPosData(NiHeader& hdr);
	NiPosData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiPosData* Clone() { return new NiPosData(*this); }
};

class NiBoolData : public NiObject {
private:
	KeyGroup<byte> data;

public:
	NiBoolData(NiHeader& hdr);
	NiBoolData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBoolData* Clone() { return new NiBoolData(*this); }
};

class NiFloatData : public NiObject {
private:
	KeyGroup<float> data;

public:
	NiFloatData(NiHeader& hdr);
	NiFloatData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiFloatData* Clone() { return new NiFloatData(*this); }
};

class NiTimeController : public NiObject {
private:
	int nextControllerRef;
	uint flags;
	float frequency;
	float phase;
	float startTime;
	float stopTime;

public:
	int targetRef;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class BSFrustumFOVController : public NiTimeController {
private:
	int interpolatorRef;

public:
	BSFrustumFOVController(NiHeader& hdr);
	BSFrustumFOVController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSFrustumFOVController* Clone() { return new BSFrustumFOVController(*this); }
};

class BSLagBoneController : public NiTimeController {
private:
	float linearVelocity;
	float linearRotation;
	float maxDistance;

public:
	BSLagBoneController(NiHeader& hdr);
	BSLagBoneController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSLagBoneController* Clone() { return new BSLagBoneController(*this); }
};

class BSProceduralLightningController : public NiTimeController {
private:
	int generationInterpRef;
	int mutationInterpRef;
	int subdivisionInterpRef;
	int numBranchesInterpRef;
	int numBranchesVarInterpRef;
	int lengthInterpRef;
	int lengthVarInterpRef;
	int widthInterpRef;
	int arcOffsetInterpRef;

	ushort subdivisions;
	ushort numBranches;
	ushort numBranchesPerVariation;

	float length;
	float lengthVariation;
	float width;
	float childWidthMult;
	float arcOffset;
	bool fadeMainBolt;
	bool fadeChildBolts;
	bool animateArcOffset;

	int shaderPropertyRef;

public:
	BSProceduralLightningController(NiHeader& hdr);
	BSProceduralLightningController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSProceduralLightningController* Clone() { return new BSProceduralLightningController(*this); }
};

class NiBoneLODController : public NiTimeController {
public:
	struct BoneArray {
		uint nodeSetSize;
		vector<int> nodeRefs;
	};

private:
	uint lod;
	uint numLODs;
	uint boneArraysSize;
	vector<BoneArray> boneArrays;

public:
	NiBoneLODController(NiHeader& hdr);
	NiBoneLODController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiBoneLODController* Clone() { return new NiBoneLODController(*this); }
};

class NiInterpController : public NiTimeController {
};

class NiSingleInterpController : public NiInterpController {
public:
	int interpolatorRef;

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class NiExtraDataController : public NiSingleInterpController {
};

class NiFloatExtraDataController : public NiExtraDataController {
private:
	uint extraDataRef;
	string extraData;

public:
	NiFloatExtraDataController(NiHeader& hdr);
	NiFloatExtraDataController(fstream& file, NiHeader& hdr);
	~NiFloatExtraDataController();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	NiFloatExtraDataController* Clone() { return new NiFloatExtraDataController(*this); }
};

class NiBoolInterpController : public NiSingleInterpController {
};

class NiVisController : public NiBoolInterpController {
public:
	NiVisController(NiHeader& hdr);
	NiVisController(fstream& file, NiHeader& hdr);

	NiVisController* Clone() { return new NiVisController(*this); }
};

class NiFloatInterpController : public NiSingleInterpController {
};

class NiAlphaController : public NiFloatInterpController {
public:
	NiAlphaController(NiHeader& hdr);
	NiAlphaController(fstream& file, NiHeader& hdr);

	NiAlphaController* Clone() { return new NiAlphaController(*this); }
};

class NiPSysUpdateCtlr : public NiTimeController {
public:
	NiPSysUpdateCtlr(NiHeader& hdr);
	NiPSysUpdateCtlr(fstream& file, NiHeader& hdr);

	NiPSysUpdateCtlr* Clone() { return new NiPSysUpdateCtlr(*this); }
};

class BSNiAlphaPropertyTestRefController : public NiAlphaController {
public:
	BSNiAlphaPropertyTestRefController(NiHeader& hdr);
	BSNiAlphaPropertyTestRefController(fstream& file, NiHeader& hdr);

	BSNiAlphaPropertyTestRefController* Clone() { return new BSNiAlphaPropertyTestRefController(*this); }
};

class NiKeyframeController : public NiSingleInterpController {
public:
	NiKeyframeController(NiHeader& hdr);
	NiKeyframeController(fstream& file, NiHeader& hdr);

	NiKeyframeController* Clone() { return new NiKeyframeController(*this); }
};

class NiTransformController : public NiKeyframeController {
public:
	NiTransformController(NiHeader& hdr);
	NiTransformController(fstream& file, NiHeader& hdr);

	NiTransformController* Clone() { return new NiTransformController(*this); }
};

class BSLightingShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor;

public:
	BSLightingShaderPropertyColorController(NiHeader& hdr);
	BSLightingShaderPropertyColorController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSLightingShaderPropertyColorController* Clone() { return new BSLightingShaderPropertyColorController(*this); }
};

class BSLightingShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable;

public:
	BSLightingShaderPropertyFloatController(NiHeader& hdr);
	BSLightingShaderPropertyFloatController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSLightingShaderPropertyFloatController* Clone() { return new BSLightingShaderPropertyFloatController(*this); }
};

class BSEffectShaderPropertyColorController : public NiFloatInterpController {
private:
	uint typeOfControlledColor;

public:
	BSEffectShaderPropertyColorController(NiHeader& hdr);
	BSEffectShaderPropertyColorController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSEffectShaderPropertyColorController* Clone() { return new BSEffectShaderPropertyColorController(*this); }
};

class BSEffectShaderPropertyFloatController : public NiFloatInterpController {
private:
	uint typeOfControlledVariable;

public:
	BSEffectShaderPropertyFloatController(NiHeader& hdr);
	BSEffectShaderPropertyFloatController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSEffectShaderPropertyFloatController* Clone() { return new BSEffectShaderPropertyFloatController(*this); }
};

class NiMultiTargetTransformController : public NiInterpController {
private:
	ushort numTargets;
	vector<int> targets;

public:
	NiMultiTargetTransformController(NiHeader& hdr);
	NiMultiTargetTransformController(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiMultiTargetTransformController* Clone() { return new NiMultiTargetTransformController(*this); }
};

class NiPSysModifierCtlr : public NiSingleInterpController {
private:
	uint modifierNameRef;
	string modifierName;

public:
	~NiPSysModifierCtlr();

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
};

class NiPSysModifierBoolCtlr : public NiPSysModifierCtlr {
};

class NiPSysModifierActiveCtlr : public NiPSysModifierBoolCtlr {
public:
	NiPSysModifierActiveCtlr(NiHeader& hdr);
	NiPSysModifierActiveCtlr(fstream& file, NiHeader& hdr);

	NiPSysModifierActiveCtlr* Clone() { return new NiPSysModifierActiveCtlr(*this); }
};

class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr {
};

class NiPSysEmitterLifeSpanCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterLifeSpanCtlr(NiHeader& hdr);
	NiPSysEmitterLifeSpanCtlr(fstream& file, NiHeader& hdr);

	NiPSysEmitterLifeSpanCtlr* Clone() { return new NiPSysEmitterLifeSpanCtlr(*this); }
};

class NiPSysEmitterSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterSpeedCtlr(NiHeader& hdr);
	NiPSysEmitterSpeedCtlr(fstream& file, NiHeader& hdr);

	NiPSysEmitterSpeedCtlr* Clone() { return new NiPSysEmitterSpeedCtlr(*this); }
};

class NiPSysEmitterInitialRadiusCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterInitialRadiusCtlr(NiHeader& hdr);
	NiPSysEmitterInitialRadiusCtlr(fstream& file, NiHeader& hdr);

	NiPSysEmitterInitialRadiusCtlr* Clone() { return new NiPSysEmitterInitialRadiusCtlr(*this); }
};

class NiPSysEmitterPlanarAngleCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterPlanarAngleCtlr(NiHeader& hdr);
	NiPSysEmitterPlanarAngleCtlr(fstream& file, NiHeader& hdr);

	NiPSysEmitterPlanarAngleCtlr* Clone() { return new NiPSysEmitterPlanarAngleCtlr(*this); }
};

class NiPSysEmitterDeclinationCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysEmitterDeclinationCtlr(NiHeader& hdr);
	NiPSysEmitterDeclinationCtlr(fstream& file, NiHeader& hdr);

	NiPSysEmitterDeclinationCtlr* Clone() { return new NiPSysEmitterDeclinationCtlr(*this); }
};

class NiPSysGravityStrengthCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysGravityStrengthCtlr(NiHeader& hdr);
	NiPSysGravityStrengthCtlr(fstream& file, NiHeader& hdr);

	NiPSysGravityStrengthCtlr* Clone() { return new NiPSysGravityStrengthCtlr(*this); }
};

class NiPSysInitialRotSpeedCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysInitialRotSpeedCtlr(NiHeader& hdr);
	NiPSysInitialRotSpeedCtlr(fstream& file, NiHeader& hdr);

	NiPSysInitialRotSpeedCtlr* Clone() { return new NiPSysInitialRotSpeedCtlr(*this); }
};

class NiPSysEmitterCtlr : public NiPSysModifierCtlr {
private:
	int visInterpolatorRef;

public:
	NiPSysEmitterCtlr(NiHeader& hdr);
	NiPSysEmitterCtlr(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysEmitterCtlr* Clone() { return new NiPSysEmitterCtlr(*this); }
};

class NiPSysMultiTargetEmitterCtlr : public NiPSysModifierCtlr {
private:
	ushort maxEmitters;
	int masterParticleSystemRef;

public:
	NiPSysMultiTargetEmitterCtlr(NiHeader& hdr);
	NiPSysMultiTargetEmitterCtlr(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiPSysMultiTargetEmitterCtlr* Clone() { return new NiPSysMultiTargetEmitterCtlr(*this); }
};

class NiControllerManager : public NiTimeController {
private:
	bool cumulative;
	uint numControllerSequences;
	vector<int> controllerSequenceRefs;
	int objectPaletteRef;

public:
	NiControllerManager(NiHeader& hdr);
	NiControllerManager(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiControllerManager* Clone() { return new NiControllerManager(*this); }
};

class NiSequence : public NiObject {
private:
	uint nameRef;
	string name;
	uint numControlledBlocks;
	uint unkInt1;
	vector<ControllerLink> controlledBlocks;

public:
	NiSequence(NiHeader& hdr);
	NiSequence(fstream& file, NiHeader& hdr);
	~NiSequence();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiSequence* Clone() { return new NiSequence(*this); }
};

class NiControllerSequence : public NiSequence {
private:
	float weight;
	int textKeyRef;
	uint cycleType;
	float frequency;
	float startTime;
	float stopTime;
	int managerRef;

	uint accumRootNameRef;
	string accumRootName;

	ushort flags;

public:
	NiControllerSequence(NiHeader& hdr);
	NiControllerSequence(fstream& file, NiHeader& hdr);
	~NiControllerSequence();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiControllerSequence* Clone() { return new NiControllerSequence(*this); }
};

class NiAVObjectPalette : public NiObject {
};

class NiDefaultAVObjectPalette : public NiAVObjectPalette {
private:
	int sceneRef;
	uint numObjects;
	vector<AVObject> objects;

public:
	NiDefaultAVObjectPalette(NiHeader& hdr);
	NiDefaultAVObjectPalette(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	NiDefaultAVObjectPalette* Clone() { return new NiDefaultAVObjectPalette(*this); }
};

class NiProperty : public NiObjectNET {
};

class NiShader : public NiProperty {
public:
	virtual bool IsSkinTint();
	virtual bool IsSkinned();
	virtual void SetSkinned(const bool& enable);
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
	virtual void SetTextureSetRef(const int& texSetRef);
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
	float lightingEffect1;					// User Version <= 12, User Version 2 < 130
	float lightingEffect2;					// User Version <= 12, User Version 2 < 130

	float subsurfaceRolloff;				// User Version == 12, User Version 2 >= 130
	float unkFloat1;						// User Version == 12, User Version 2 >= 130
	float backlightPower;					// User Version == 12, User Version 2 >= 130
	float grayscaleToPaletteScale;			// User Version == 12, User Version 2 >= 130
	float fresnelPower;						// User Version == 12, User Version 2 >= 130
	float wetnessSpecScale;					// User Version == 12, User Version 2 >= 130
	float wetnessSpecPower;					// User Version == 12, User Version 2 >= 130
	float wetnessMinVar;					// User Version == 12, User Version 2 >= 130
	float wetnessEnvmapScale;				// User Version == 12, User Version 2 >= 130
	float wetnessFresnelPower;				// User Version == 12, User Version 2 >= 130
	float wetnessMetalness;					// User Version == 12, User Version 2 >= 130

	float environmentMapScale;				// Shader Type == 1
	ushort unkEnvmap;						// Shader Type == 1, User Version == 12, User Version 2 >= 130
	Vector3 skinTintColor;					// Shader Type == 5
	uint unkSkinTint;						// Shader Type == 5, User Version == 12, User Version 2 >= 130
	Vector3 hairTintColor;					// Shader Type == 6
	float maxPasses;						// Shader Type == 7
	float scale;							// Shader Type == 7
	float parallaxInnerLayerThickness;		// Shader Type == 11
	float parallaxRefractionScale;			// Shader Type == 11
	Vector2 parallaxInnerLayerTextureScale;	// Shader Type == 11
	float parallaxEnvmapStrength;			// Shader Type == 11
	Color4 sparkleParameters;				// Shader Type == 14
	float eyeCubemapScale;					// Shader Type == 16
	Vector3 eyeLeftReflectionCenter;		// Shader Type == 16
	Vector3 eyeRightReflectionCenter;		// Shader Type == 16


	BSLightingShaderProperty(NiHeader& hdr);
	BSLightingShaderProperty(fstream& file, NiHeader& hdr);
	~BSLightingShaderProperty();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void notifyStringDelete(int stringID);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSLightingShaderProperty* Clone() { return new BSLightingShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool& enable);
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
	void SetTextureSetRef(const int& texSetRef);
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
	int CalcBlockSize();
	BSEffectShaderProperty* Clone() { return new BSEffectShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool& enable);
	bool IsDoubleSided();
	Color4 GetEmissiveColor();
	void SetEmissiveColor(Color4 color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(float emissive);
};

class BSWaterShaderProperty : public NiShader {
private:
	uint shaderFlags1;
	uint shaderFlags2;
	Vector2 uvOffset;
	Vector2 uvScale;
	uint waterFlags;

public:
	BSWaterShaderProperty(NiHeader& hdr);
	BSWaterShaderProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSWaterShaderProperty* Clone() { return new BSWaterShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool& enable);
	bool IsDoubleSided();
};

class BSSkyShaderProperty : public NiShader {
private:
	uint shaderFlags1;
	uint shaderFlags2;
	Vector2 uvOffset;
	Vector2 uvScale;

	NiString baseTexture;
	uint skyFlags;

public:
	BSSkyShaderProperty(NiHeader& hdr);
	BSSkyShaderProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSSkyShaderProperty* Clone() { return new BSSkyShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool& enable);
	bool IsDoubleSided();
};

class BSShaderLightingProperty : public BSShaderProperty {
public:
	uint textureClampMode;					// User Version <= 11

	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
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
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSShaderPPLightingProperty* Clone() { return new BSShaderPPLightingProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool& enable);
	int GetTextureSetRef();
	void SetTextureSetRef(const int& texSetRef);
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
	BSShaderTextureSet* Clone() { return new BSShaderTextureSet(*this); }
};

class NiAlphaProperty : public NiProperty {
public:
	ushort flags;
	byte threshold;

	NiAlphaProperty(NiHeader& hdr);
	NiAlphaProperty(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiAlphaProperty* Clone() { return new NiAlphaProperty(*this); }
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
	int CalcBlockSize();
	NiMaterialProperty* Clone() { return new NiMaterialProperty(*this); }

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
	int CalcBlockSize();
	NiStencilProperty* Clone() { return new NiStencilProperty(*this); }
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

class NiBinaryExtraData : public NiExtraData {
private:
	uint size;
	vector<byte> data;

public:
	NiBinaryExtraData(NiHeader& hdr);
	NiBinaryExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBinaryExtraData* Clone() { return new NiBinaryExtraData(*this); }
};

class NiFloatExtraData : public NiExtraData {
private:
	float floatData;

public:
	NiFloatExtraData(NiHeader& hdr);
	NiFloatExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiFloatExtraData* Clone() { return new NiFloatExtraData(*this); }
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
	NiStringExtraData* Clone() { return new NiStringExtraData(*this); }

	string GetStringData();
	void SetStringData(const string& str);
};

class NiStringsExtraData : public NiExtraData {
private:
	uint numStrings;
	vector<NiString> stringsData;

public:
	NiStringsExtraData(NiHeader& hdr);
	NiStringsExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiStringsExtraData* Clone() { return new NiStringsExtraData(*this); }
};

class NiBooleanExtraData : public NiExtraData {
private:
	bool booleanData;

public:
	NiBooleanExtraData(NiHeader& hdr);
	NiBooleanExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiBooleanExtraData* Clone() { return new NiBooleanExtraData(*this); }

	bool GetBooleanData();
	void SetBooleanData(const bool& booleanData);
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
	NiIntegerExtraData* Clone() { return new NiIntegerExtraData(*this); }

	uint GetIntegerData();
	void SetIntegerData(const uint& integerData);
};

class BSXFlags : public NiIntegerExtraData {
public:
	BSXFlags(NiHeader& hdr);
	BSXFlags(fstream& file, NiHeader& hdr);

	BSXFlags* Clone() { return new BSXFlags(*this); }
};

class BSInvMarker : public NiExtraData {
private:
	ushort rotationX;
	ushort rotationY;
	ushort rotationZ;
	float zoom;

public:
	BSInvMarker(NiHeader& hdr);
	BSInvMarker(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSInvMarker* Clone() { return new BSInvMarker(*this); }
};

class BSFurnitureMarker : public NiExtraData {
private:
	uint numPositions;
	vector<FurniturePosition> positions;

public:
	BSFurnitureMarker(NiHeader& hdr);
	BSFurnitureMarker(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSFurnitureMarker* Clone() { return new BSFurnitureMarker(*this); }
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {
public:
	BSFurnitureMarkerNode(NiHeader& hdr);
	BSFurnitureMarkerNode(fstream& file, NiHeader& hdr);

	BSFurnitureMarkerNode* Clone() { return new BSFurnitureMarkerNode(*this); }
};

class BSDecalPlacementVectorExtraData : public NiExtraData {
public:
	struct DecalVectorBlock {
		ushort numVectors;
		vector<Vector3> points;
		vector<Vector3> normals;
	};

private:
	float unkFloat1;
	ushort numVectorBlocks;
	vector<DecalVectorBlock> decalVectorBlocks;

public:
	BSDecalPlacementVectorExtraData(NiHeader& hdr);
	BSDecalPlacementVectorExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSDecalPlacementVectorExtraData* Clone() { return new BSDecalPlacementVectorExtraData(*this); }
};

class BSBehaviorGraphExtraData : public NiExtraData {
private:
	uint behaviorGraphFileRef;
	string behaviorGraphFile;
	byte controlsBaseSkel;

public:
	BSBehaviorGraphExtraData(NiHeader& hdr);
	BSBehaviorGraphExtraData(fstream& file, NiHeader& hdr);
	~BSBehaviorGraphExtraData();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	BSBehaviorGraphExtraData* Clone() { return new BSBehaviorGraphExtraData(*this); }
};

class BSBound : public NiExtraData {
private:
	Vector3 center;
	Vector3 halfExtents;

public:
	BSBound(NiHeader& hdr);
	BSBound(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSBound* Clone() { return new BSBound(*this); }
};

class BSBoneLODExtraData : public NiExtraData {
private:
	uint numBoneLODs;
	vector<BoneLOD> boneLODs;

public:
	BSBoneLODExtraData(NiHeader& hdr);
	BSBoneLODExtraData(fstream& file, NiHeader& hdr);
	~BSBoneLODExtraData();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	BSBoneLODExtraData* Clone() { return new BSBoneLODExtraData(*this); }
};

class NiTextKeyExtraData : public NiExtraData {
private:
	uint numTextKeys;
	vector<Key<uint>> textKeys;

public:
	NiTextKeyExtraData(NiHeader& hdr);
	NiTextKeyExtraData(fstream& file, NiHeader& hdr);
	~NiTextKeyExtraData();

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	NiTextKeyExtraData* Clone() { return new NiTextKeyExtraData(*this); }
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
	BSConnectPoint* Clone() { return new BSConnectPoint(*this); }
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
	BSConnectPointParents* Clone() { return new BSConnectPointParents(*this); }
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
	BSConnectPointChildren* Clone() { return new BSConnectPointChildren(*this); }
};

class BSExtraData : public NiObject {
};

class BSClothExtraData : public BSExtraData {
private:
	uint numBytes;
	vector<char> data;

public:
	BSClothExtraData();
	BSClothExtraData(NiHeader& hdr, const uint& size = 0);
	BSClothExtraData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSClothExtraData* Clone() { return new BSClothExtraData(*this); }

	bool ToHKX(const string& fileName);
	bool FromHKX(const string& fileName);
};

class BSMultiBound : public NiObject {
private:
	int dataRef;

public:
	BSMultiBound(NiHeader& hdr);
	BSMultiBound(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	BSMultiBound* Clone() { return new BSMultiBound(*this); }
};

class BSMultiBoundData : public NiObject {
};

class BSMultiBoundOBB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 size;
	float rotation[9];

public:
	BSMultiBoundOBB(NiHeader& hdr);
	BSMultiBoundOBB(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSMultiBoundOBB* Clone() { return new BSMultiBoundOBB(*this); }
};

class BSMultiBoundAABB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 halfExtent;

public:
	BSMultiBoundAABB(NiHeader& hdr);
	BSMultiBoundAABB(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	BSMultiBoundAABB* Clone() { return new BSMultiBoundAABB(*this); }
};

class NiCollisionObject : public NiObject {
private:
	int targetRef;

public:
	NiCollisionObject(NiHeader& hdr);
	NiCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	int CalcBlockSize();
	NiCollisionObject* Clone() { return new NiCollisionObject(*this); }
};

class bhkNiCollisionObject : public NiCollisionObject {
private:
	ushort flags;
	int bodyRef;

public:
	bhkNiCollisionObject(NiHeader& hdr);
	bhkNiCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkNiCollisionObject* Clone() { return new bhkNiCollisionObject(*this); }
};

class bhkCollisionObject : public bhkNiCollisionObject {
public:
	bhkCollisionObject(NiHeader& hdr);
	bhkCollisionObject(fstream& file, NiHeader& hdr);

	bhkCollisionObject* Clone() { return new bhkCollisionObject(*this); }
};

class bhkNPCollisionObject : public bhkCollisionObject {
private:
	uint unkInt;

public:
	bhkNPCollisionObject(NiHeader& hdr);
	bhkNPCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkNPCollisionObject* Clone() { return new bhkNPCollisionObject(*this); }
};

class bhkPCollisionObject : public bhkNiCollisionObject {
public:
	bhkPCollisionObject(NiHeader& hdr);
	bhkPCollisionObject(fstream& file, NiHeader& hdr);

	bhkPCollisionObject* Clone() { return new bhkPCollisionObject(*this); }
};

class bhkSPCollisionObject : public bhkPCollisionObject {
public:
	bhkSPCollisionObject(NiHeader& hdr);
	bhkSPCollisionObject(fstream& file, NiHeader& hdr);

	bhkSPCollisionObject* Clone() { return new bhkSPCollisionObject(*this); }
};

class bhkBlendCollisionObject : public bhkCollisionObject {
private:
	float heirGain;
	float velGain;

public:
	bhkBlendCollisionObject(NiHeader& hdr);
	bhkBlendCollisionObject(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkBlendCollisionObject* Clone() { return new bhkBlendCollisionObject(*this); }
};

class bhkPhysicsSystem : public BSExtraData {
private:
	uint numBytes;
	vector<char> data;

public:
	bhkPhysicsSystem();
	bhkPhysicsSystem(NiHeader& hdr, const uint& size = 0);
	bhkPhysicsSystem(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkPhysicsSystem* Clone() { return new bhkPhysicsSystem(*this); }
};

class bhkRefObject : public NiObject {
};

class bhkSerializable : public bhkRefObject {
};

class bhkShape : public bhkSerializable {
};

class bhkHeightFieldShape : public bhkShape {
};

class bhkPlaneShape : public bhkHeightFieldShape {
private:
	HavokMaterial material;
	Vector3 unkVec;
	Vector3 direction;
	float constant;
	Vector4 halfExtents;
	Vector4 center;

public:
	bhkPlaneShape(NiHeader& hdr);
	bhkPlaneShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkPlaneShape* Clone() { return new bhkPlaneShape(*this); }
};

class bhkSphereRepShape : public bhkShape {
private:
	HavokMaterial material;
	float radius;

public:
	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
};

class bhkConvexShape : public bhkSphereRepShape {
};

class bhkConvexVerticesShape : public bhkConvexShape {
private:
	uint mat;
	float radius;
	hkWorldObjCInfoProperty vertsProp;
	hkWorldObjCInfoProperty normalsProp;

	uint numVerts;
	vector<Vector4> verts;

	uint numNormals;
	vector<Vector4> normals;

public:
	bhkConvexVerticesShape(NiHeader& hdr);
	bhkConvexVerticesShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkConvexVerticesShape* Clone() { return new bhkConvexVerticesShape(*this); }
};

class bhkBoxShape : public bhkConvexShape {
private:
	uint unkInt1;
	uint unkInt2;
	Vector3 dimensions;
	float radius2;

public:
	bhkBoxShape(NiHeader& hdr);
	bhkBoxShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkBoxShape* Clone() { return new bhkBoxShape(*this); }
};

class bhkSphereShape : public bhkConvexShape {
public:
	bhkSphereShape(NiHeader& hdr);
	bhkSphereShape(fstream& file, NiHeader& hdr);

	bhkSphereShape* Clone() { return new bhkSphereShape(*this); }
};

class bhkTransformShape : public bhkShape {
private:
	int shapeRef;
	HavokMaterial material;
	float unkFloat;
	byte unkBytes[8];
	Matrix4 xform;

public:
	bhkTransformShape(NiHeader& hdr);
	bhkTransformShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkTransformShape* Clone() { return new bhkTransformShape(*this); }
};

class bhkConvexTransformShape : public bhkTransformShape {
public:
	bhkConvexTransformShape(NiHeader& hdr);
	bhkConvexTransformShape(fstream& file, NiHeader& hdr);

	bhkConvexTransformShape* Clone() { return new bhkConvexTransformShape(*this); }
};

class bhkCapsuleShape : public bhkConvexShape {
private:
	uint padding1;
	uint padding2;
	Vector3 point1;
	float radius1;
	Vector3 point2;
	float radius2;

public:
	bhkCapsuleShape(NiHeader& hdr);
	bhkCapsuleShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkCapsuleShape* Clone() { return new bhkCapsuleShape(*this); }
};

class bhkBvTreeShape : public bhkShape {
};

class bhkMoppBvTreeShape : public bhkBvTreeShape {
private:
	int shapeRef;
	uint userData;
	uint shapeCollection;
	uint code;
	float scale;
	uint dataSize;
	Vector4 offset;
	byte buildType;							// User Version >= 12
	vector<byte> data;

public:
	bhkMoppBvTreeShape(NiHeader& hdr);
	bhkMoppBvTreeShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkMoppBvTreeShape* Clone() { return new bhkMoppBvTreeShape(*this); }
};

class bhkNiTriStripsShape : public bhkShape {
private:
	HavokMaterial material;
	float radius;
	uint unused1;
	uint unused2;
	uint unused3;
	uint unused4;
	uint unused5;
	uint growBy;
	Vector4 scale;

	uint numParts;
	vector<int> parts;

	uint numFilters;
	vector<uint> filters;

public:
	bhkNiTriStripsShape(NiHeader& hdr);
	bhkNiTriStripsShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkNiTriStripsShape* Clone() { return new bhkNiTriStripsShape(*this); }
};

class bhkShapeCollection : public bhkShape {
};

class bhkListShape : public bhkShapeCollection {
private:
	uint numSubShapes;
	vector<int> subShapeRef;
	HavokMaterial material;
	float unkFloats[6];
	uint numUnkInts;
	vector<uint> unkInts;

public:
	bhkListShape(NiHeader& hdr);
	bhkListShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkListShape* Clone() { return new bhkListShape(*this); }
};

class bhkWorldObject : public bhkSerializable {
private:
	int shapeRef;
	HavokFilter collisionFilter;
	int unkInt1;
	byte broadPhaseType;
	byte unkBytes[3];
	hkWorldObjCInfoProperty prop;

public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkWorldObject* Clone() { return new bhkWorldObject(*this); }
};

class bhkPhantom : public bhkWorldObject {
};

class bhkShapePhantom : public bhkPhantom {
};

class bhkSimpleShapePhantom : public bhkShapePhantom {
private:
	uint padding1;
	uint padding2;
	Matrix4 transform;

public:
	bhkSimpleShapePhantom(NiHeader& hdr);
	bhkSimpleShapePhantom(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkSimpleShapePhantom* Clone() { return new bhkSimpleShapePhantom(*this); }
};

class bhkEntity : public bhkWorldObject {
};

class bhkConstraint : public bhkSerializable {
private:
	uint numEntities;
	vector<int> entities;
	uint priority;

public:
	void Init();
	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
};

class bhkHingeConstraint : public bhkConstraint {
private:
	HingeDesc hinge;

public:
	bhkHingeConstraint(NiHeader& hdr);
	bhkHingeConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkHingeConstraint* Clone() { return new bhkHingeConstraint(*this); }
};

class bhkLimitedHingeConstraint : public bhkConstraint {
private:
	LimitedHingeDesc limitedHinge;

public:
	bhkLimitedHingeConstraint(NiHeader& hdr);
	bhkLimitedHingeConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkLimitedHingeConstraint* Clone() { return new bhkLimitedHingeConstraint(*this); }
};

class bhkBreakableConstraint : public bhkConstraint {
private:
	SubConstraintDesc subConstraint;
	float threshold;
	bool removeIfBroken;

public:
	bhkBreakableConstraint(NiHeader& hdr);
	bhkBreakableConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkBreakableConstraint* Clone() { return new bhkBreakableConstraint(*this); }
};

class bhkRagdollConstraint : public bhkConstraint {
private:
	RagdollDesc ragdoll;

public:
	bhkRagdollConstraint(NiHeader& hdr);
	bhkRagdollConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkRagdollConstraint* Clone() { return new bhkRagdollConstraint(*this); }
};

class bhkStiffSpringConstraint : public bhkConstraint {
private:
	StiffSpringDesc stiffSpring;

public:
	bhkStiffSpringConstraint(NiHeader& hdr);
	bhkStiffSpringConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkStiffSpringConstraint* Clone() { return new bhkStiffSpringConstraint(*this); }
};

class bhkBallAndSocketConstraint : public bhkConstraint {
private:
	BallAndSocketDesc ballAndSocket;

public:
	bhkBallAndSocketConstraint(NiHeader& hdr);
	bhkBallAndSocketConstraint(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkBallAndSocketConstraint* Clone() { return new bhkBallAndSocketConstraint(*this); }
};

class bhkBallSocketConstraintChain : public bhkSerializable {
private:
	uint numPivots;
	vector<Vector4> pivots;

	float tau;
	float damping;
	float cfm;
	float maxErrorDistance;

	uint numEntitiesA;
	vector<int> entityARefs;

	uint numEntities;
	int entityARef;
	int entityBRef;
	uint priority;

public:
	bhkBallSocketConstraintChain(NiHeader& hdr);
	bhkBallSocketConstraintChain(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkBallSocketConstraintChain* Clone() { return new bhkBallSocketConstraintChain(*this); }
};

class bhkRigidBody : public bhkEntity {
private:
	byte responseType;
	byte unkByte;
	ushort processContactCallbackDelay;
	ushort unkShorts[2];
	HavokFilter collisionFilterCopy;
	ushort unkShorts2[6];
	Vector4 translation;
	Quaternion rotation;
	Vector4 linearVelocity;
	Vector4 angularVelocity;
	float inertiaMatrix[12];
	Vector4 center;
	float mass;
	float linearDamping;
	float angularDamping;
	float timeFactor;						// User Version >= 12
	float gravityFactor;					// User Version >= 12
	float friction;
	float rollingFrictionMult;				// User Version >= 12
	float restitution;
	float maxLinearVelocity;
	float maxAngularVelocity;
	float penetrationDepth;
	byte motionSystem;
	byte deactivatorType;
	byte solverDeactivation;
	byte qualityType;
	byte autoRemoveLevel;
	byte responseModifierFlag;
	byte numShapeKeysInContactPointProps;
	bool forceCollideOntoPpu;
	uint unkInt2;
	uint unkInt3;
	uint unkInt4;							// User Version >= 12
	uint numConstraints;
	vector<int> constraints;
	uint unkInt5;							// User Version <= 11
	ushort unkShort3;						// User Version >= 12

public:
	bhkRigidBody(NiHeader& hdr);
	bhkRigidBody(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkRigidBody* Clone() { return new bhkRigidBody(*this); }
};

class bhkRigidBodyT : public bhkRigidBody {
public:
	bhkRigidBodyT(NiHeader& hdr);
	bhkRigidBodyT(fstream& file, NiHeader& hdr);

	bhkRigidBodyT* Clone() { return new bhkRigidBodyT(*this); }
};

class bhkCompressedMeshShape : public bhkShape {
private:
	int targetRef;
	uint userData;
	float radius;
	float unkFloat;
	Vector4 scaling;
	float radius2;
	Vector4 scaling2;
	int dataRef;

public:
	bhkCompressedMeshShape(NiHeader& hdr);
	bhkCompressedMeshShape(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	void notifyBlockDelete(int blockID);
	void notifyBlockSwap(int blockIndexLo, int blockIndexHi);
	void GetChildRefs(set<int>& refs);
	int CalcBlockSize();
	bhkCompressedMeshShape* Clone() { return new bhkCompressedMeshShape(*this); }
};

class bhkCompressedMeshShapeData : public bhkRefObject {
private:
	uint bitsPerIndex;
	uint bitsPerWIndex;
	uint maskWIndex;
	uint maskIndex;
	float error;
	Vector4 aabbBoundMin;
	Vector4 aabbBoundMax;
	byte weldingType;
	byte materialType;

	uint numMat32;
	vector<uint> mat32;
	uint numMat16;
	vector<uint> mat16;
	uint numMat8;
	vector<uint> mat8;

	uint numMaterials;
	vector<bhkCMSDMaterial> materials;

	uint numNamedMat;

	uint numTransforms;
	vector<bhkCMSDTransform> transforms;

	uint numBigVerts;
	vector<Vector4> bigVerts;

	uint numBigTris;
	vector<bhkCMSDBigTris> bigTris;

	uint numChunks;
	vector<bhkCMSDChunk> chunks;

	uint numConvexPieceA;

public:
	bhkCompressedMeshShapeData(NiHeader& hdr);
	bhkCompressedMeshShapeData(fstream& file, NiHeader& hdr);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	bhkCompressedMeshShapeData* Clone() { return new bhkCompressedMeshShapeData(*this); }
};

class NiUnknown : public NiObject {
public:
	vector<char> data;

	NiUnknown();
	NiUnknown(fstream& file, const uint& size);
	NiUnknown(const uint& size);

	void Get(fstream& file);
	void Put(fstream& file);
	int CalcBlockSize();
	NiUnknown* Clone() { return new NiUnknown(*this); }
};

template <class T>
T* NiHeader::GetBlock(const int& blockId) {
	if (blockId >= 0 && blockId < numBlocks)
		return dynamic_cast<T*>((*blocks)[blockId]);

	return nullptr;
}
