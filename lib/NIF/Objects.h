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

	~NiObjectNET() {
		name.Clear(header);
	};

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);

	std::string GetName();
	void SetName(const std::string& str, const bool rename = false);
	void ClearName();

	int GetControllerRef();
	void SetControllerRef(int ctlrRef);

	int GetNumExtraData();
	void SetExtraDataRef(const int id, const int blockId);
	int GetExtraDataRef(const int id);
	void AddExtraDataRef(const int id);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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

	void Init(NiHeader* hdr);
	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();

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
	NiDefaultAVObjectPalette(NiHeader* hdr);
	NiDefaultAVObjectPalette(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiDefaultAVObjectPalette";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	NiCamera(NiHeader* hdr);
	NiCamera(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiCamera";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	NiCamera* Clone() { return new NiCamera(*this); }
};

class NiNode : public NiAVObject {
private:
	BlockRefArray<NiAVObject> childRefs;
	BlockRefArray<NiAVObject> effectRefs;	// should be NiDynamicEffect

public:
	NiNode(NiHeader* hdr);
	NiNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
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
	BSFadeNode(NiHeader* hdr);
	BSFadeNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSFadeNode";
	virtual const char* GetBlockName() { return BlockName; }

	BSFadeNode* Clone() { return new BSFadeNode(*this); }
};

class BSValueNode : public NiNode {
private:
	int value;
	byte valueFlags;

public:
	BSValueNode(NiHeader* hdr);
	BSValueNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSValueNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	int CalcBlockSize();
	BSValueNode* Clone() { return new BSValueNode(*this); }
};

class BSLeafAnimNode : public NiNode {
public:
	BSLeafAnimNode(NiHeader* hdr);
	BSLeafAnimNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSLeafAnimNode";
	virtual const char* GetBlockName() { return BlockName; }
};

class BSTreeNode : public NiNode {
private:
	BlockRefArray<NiNode> bones1;
	BlockRefArray<NiNode> bones2;

public:
	BSTreeNode(NiHeader* hdr);
	BSTreeNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSTreeNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSTreeNode* Clone() { return new BSTreeNode(*this); }
};

class BSOrderedNode : public NiNode {
private:
	Vector4 alphaSortBound;
	bool isStaticBound;

public:
	BSOrderedNode(NiHeader* hdr);
	BSOrderedNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSOrderedNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	int CalcBlockSize();
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
	BSMultiBoundOBB(NiHeader* hdr);
	BSMultiBoundOBB(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMultiBoundOBB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSMultiBoundOBB* Clone() { return new BSMultiBoundOBB(*this); }
};

class BSMultiBoundAABB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 halfExtent;

public:
	BSMultiBoundAABB(NiHeader* hdr);
	BSMultiBoundAABB(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMultiBoundAABB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSMultiBoundAABB* Clone() { return new BSMultiBoundAABB(*this); }
};

class BSMultiBound : public NiObject {
private:
	BlockRef<BSMultiBoundData> dataRef;

public:
	BSMultiBound(NiHeader* hdr);
	BSMultiBound(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMultiBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSMultiBound* Clone() { return new BSMultiBound(*this); }
};

class BSMultiBoundNode : public NiNode {
private:
	BlockRef<BSMultiBound> multiBoundRef;
	uint cullingMode;

public:
	BSMultiBoundNode(NiHeader* hdr);
	BSMultiBoundNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSMultiBoundNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize();
	BSMultiBoundNode* Clone() { return new BSMultiBoundNode(*this); }
};

class BSBlastNode : public NiNode {
private:
	byte min;
	byte max;
	byte current;

public:
	BSBlastNode(NiHeader* hdr);
	BSBlastNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSBlastNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	int CalcBlockSize();
	BSBlastNode* Clone() { return new BSBlastNode(*this); }
};

class BSDamageStage : public BSBlastNode {
public:
	BSDamageStage(NiHeader* hdr);
	BSDamageStage(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSDamageStage";
	virtual const char* GetBlockName() { return BlockName; }

	BSDamageStage* Clone() { return new BSDamageStage(*this); }
};

class NiBillboardNode : public NiNode {
private:
	ushort billboardMode;

public:
	NiBillboardNode(NiHeader* hdr);
	NiBillboardNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBillboardNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	int CalcBlockSize();
	NiBillboardNode* Clone() { return new NiBillboardNode(*this); }
};

class NiSwitchNode : public NiNode {
private:
	ushort flags;
	uint index;

public:
	NiSwitchNode(NiHeader* hdr);
	NiSwitchNode(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiSwitchNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);

	int CalcBlockSize();
	NiSwitchNode* Clone() { return new NiSwitchNode(*this); }
};
