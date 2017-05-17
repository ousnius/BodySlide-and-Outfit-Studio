/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Animation.h"
#include "ExtraData.h"

class NiObjectNET : public NiObject {
private:
	StringRef name;
	BlockRef<NiTimeController> controllerRef;
	BlockRefArray<NiExtraData> extraDataRefs;

public:
	uint skyrimShaderType;					// BSLightingShaderProperty && User Version >= 12
	bool bBSLightingShaderProperty;

	void Init();
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<int*>& refs);

	std::string GetName(NiHeader* hdr);
	void SetName(NiHeader* hdr, const std::string& str, const bool rename = false);
	void ClearName();

	int GetControllerRef();
	void SetControllerRef(int ctlrRef);

	int GetNumExtraData();
	void SetExtraDataRef(const int id, const int blockId);
	int GetExtraDataRef(const int id);
	void AddExtraDataRef(const int id);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
};

class NiProperty;
class NiCollisionObject;

class NiAVObject : public NiObjectNET {
protected:
	BlockRef<NiCollisionObject> collisionRef;

public:
	uint flags;
	Vector3 translation;
	Vector3 rotation[3];
	float scale;
	BlockRefArray<NiProperty> propertyRefs;

	void Init();
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);

	int GetCollisionRef() { return collisionRef.index; }
	void SetCollisionRef(const int colRef) { collisionRef.index = colRef; }

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

struct AVObject {
	NiString name;
	BlockRef<NiAVObject> objectRef;
};

class NiAVObjectPalette : public NiObject {
};

class NiDefaultAVObjectPalette : public NiAVObjectPalette {
private:
	BlockRef<NiAVObject> sceneRef;
	uint numObjects = 0;
	std::vector<AVObject> objects;

public:
	NiDefaultAVObjectPalette();
	NiDefaultAVObjectPalette(NiStream& stream);

	static constexpr const char* BlockName = "NiDefaultAVObjectPalette";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	NiDefaultAVObjectPalette* Clone() { return new NiDefaultAVObjectPalette(*this); }
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

	BlockRef<NiAVObject> sceneRef;
	uint numScreenPolygons = 0;
	uint numScreenTextures = 0;

public:
	NiCamera();
	NiCamera(NiStream& stream);

	static constexpr const char* BlockName = "NiCamera";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	NiCamera* Clone() { return new NiCamera(*this); }
};

class NiNode : public NiAVObject {
private:
	BlockRefArray<NiAVObject> childRefs;
	BlockRefArray<NiAVObject> effectRefs;	// should be NiDynamicEffect

public:
	NiNode();
	NiNode(NiStream& stream);

	static constexpr const char* BlockName = "NiNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	NiNode* Clone() { return new NiNode(*this); }

	int GetNumChildren() { return childRefs.GetSize(); }
	int GetChildRef(const int id);
	void AddChildRef(const int id);
	void ClearChildren();
	std::vector<BlockRef<NiAVObject>>& GetChildren() { return childRefs.GetRefs(); }

	int GetNumEffects() { return effectRefs.GetSize(); }
	int GetEffectRef(const int id);
	void AddEffectRef(const int id);
};

class BSFadeNode : public NiNode {
public:
	BSFadeNode();
	BSFadeNode(NiStream& stream);

	static constexpr const char* BlockName = "BSFadeNode";
	virtual const char* GetBlockName() { return BlockName; }

	BSFadeNode* Clone() { return new BSFadeNode(*this); }
};

class BSValueNode : public NiNode {
private:
	int value;
	byte valueFlags;

public:
	BSValueNode();
	BSValueNode(NiStream& stream);

	static constexpr const char* BlockName = "BSValueNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	int CalcBlockSize(NiVersion& version);
	BSValueNode* Clone() { return new BSValueNode(*this); }
};

class BSLeafAnimNode : public NiNode {
public:
	BSLeafAnimNode();
	BSLeafAnimNode(NiStream& stream);

	static constexpr const char* BlockName = "BSLeafAnimNode";
	virtual const char* GetBlockName() { return BlockName; }
};

class BSTreeNode : public NiNode {
private:
	BlockRefArray<NiNode> bones1;
	BlockRefArray<NiNode> bones2;

public:
	BSTreeNode();
	BSTreeNode(NiStream& stream);

	static constexpr const char* BlockName = "BSTreeNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	BSTreeNode* Clone() { return new BSTreeNode(*this); }
};

class BSOrderedNode : public NiNode {
private:
	Vector4 alphaSortBound;
	bool isStaticBound;

public:
	BSOrderedNode();
	BSOrderedNode(NiStream& stream);

	static constexpr const char* BlockName = "BSOrderedNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	int CalcBlockSize(NiVersion& version);
	BSOrderedNode* Clone() { return new BSOrderedNode(*this); }
};

class BSMultiBoundData : public NiObject {
};

class BSMultiBoundOBB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 size;
	float rotation[9];

public:
	BSMultiBoundOBB();
	BSMultiBoundOBB(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundOBB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSMultiBoundOBB* Clone() { return new BSMultiBoundOBB(*this); }
};

class BSMultiBoundAABB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 halfExtent;

public:
	BSMultiBoundAABB();
	BSMultiBoundAABB(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundAABB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSMultiBoundAABB* Clone() { return new BSMultiBoundAABB(*this); }
};

class BSMultiBound : public NiObject {
private:
	BlockRef<BSMultiBoundData> dataRef;

public:
	BSMultiBound();
	BSMultiBound(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	BSMultiBound* Clone() { return new BSMultiBound(*this); }
};

class BSMultiBoundNode : public NiNode {
private:
	BlockRef<BSMultiBound> multiBoundRef;
	uint cullingMode;

public:
	BSMultiBoundNode();
	BSMultiBoundNode(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	BSMultiBoundNode* Clone() { return new BSMultiBoundNode(*this); }
};

class BSBlastNode : public NiNode {
private:
	byte min;
	byte max;
	byte current;

public:
	BSBlastNode();
	BSBlastNode(NiStream& stream);

	static constexpr const char* BlockName = "BSBlastNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	int CalcBlockSize(NiVersion& version);
	BSBlastNode* Clone() { return new BSBlastNode(*this); }
};

class BSDamageStage : public BSBlastNode {
public:
	BSDamageStage();
	BSDamageStage(NiStream& stream);

	static constexpr const char* BlockName = "BSDamageStage";
	virtual const char* GetBlockName() { return BlockName; }

	BSDamageStage* Clone() { return new BSDamageStage(*this); }
};

class NiBillboardNode : public NiNode {
private:
	ushort billboardMode;

public:
	NiBillboardNode();
	NiBillboardNode(NiStream& stream);

	static constexpr const char* BlockName = "NiBillboardNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	int CalcBlockSize(NiVersion& version);
	NiBillboardNode* Clone() { return new NiBillboardNode(*this); }
};

class NiSwitchNode : public NiNode {
private:
	ushort flags;
	uint index;

public:
	NiSwitchNode();
	NiSwitchNode(NiStream& stream);

	static constexpr const char* BlockName = "NiSwitchNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	int CalcBlockSize(NiVersion& version);
	NiSwitchNode* Clone() { return new NiSwitchNode(*this); }
};
