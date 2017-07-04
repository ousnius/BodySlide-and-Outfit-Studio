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
	uint skyrimShaderType = 0;				// BSLightingShaderProperty && User Version >= 12
	bool bBSLightingShaderProperty = false;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);

	std::string GetName();
	void SetName(const std::string& str);
	void ClearName();

	int GetControllerRef();
	void SetControllerRef(int ctlrRef);

	int GetNumExtraData();
	void SetExtraDataRef(const int id, const int blockId);
	int GetExtraDataRef(const int id);
	void AddExtraDataRef(const int id);

	void GetChildRefs(std::set<int*>& refs);
};

class NiProperty;
class NiCollisionObject;

class NiAVObject : public NiObjectNET {
protected:
	BlockRef<NiCollisionObject> collisionRef;

public:
	uint flags = 524302;
	Vector3 translation;
	Vector3 rotation[3] = { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) };
	float scale = 1.0f;
	BlockRefArray<NiProperty> propertyRefs;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);

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
	NiDefaultAVObjectPalette() {}
	NiDefaultAVObjectPalette(NiStream& stream);

	static constexpr const char* BlockName = "NiDefaultAVObjectPalette";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetPtrs(std::set<int*>& ptrs);
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
	NiCamera() {}
	NiCamera(NiStream& stream);

	static constexpr const char* BlockName = "NiCamera";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	NiCamera* Clone() { return new NiCamera(*this); }
};

class NiNode : public NiAVObject {
private:
	BlockRefArray<NiAVObject> childRefs;
	BlockRefArray<NiAVObject> effectRefs;	// should be NiDynamicEffect

public:
	NiNode() {}
	NiNode(NiStream& stream);

	static constexpr const char* BlockName = "NiNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<int*>& refs);
	NiNode* Clone() { return new NiNode(*this); }

	int GetNumChildren() { return childRefs.GetSize(); }
	int GetChildRef(const int id);
	void AddChildRef(const int id);
	void ClearChildren();
	std::vector<BlockRef<NiAVObject>>& GetChildren() { return childRefs.GetBlockRefs(); }

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

enum BSValueNodeFlags : byte {
	BSVN_NONE = 0x0,
	BSVN_BILLBOARD_WORLD_Z = 0x1,
	BSVN_USE_PLAYER_ADJUST = 0x2
};

class BSValueNode : public NiNode {
private:
	int value = 0;
	BSValueNodeFlags valueFlags = BSVN_NONE;

public:
	BSValueNode();
	BSValueNode(NiStream& stream);

	static constexpr const char* BlockName = "BSValueNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

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
	BSTreeNode* Clone() { return new BSTreeNode(*this); }
};

class BSOrderedNode : public NiNode {
private:
	Vector4 alphaSortBound;
	bool isStaticBound = false;

public:
	BSOrderedNode();
	BSOrderedNode(NiStream& stream);

	static constexpr const char* BlockName = "BSOrderedNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	BSOrderedNode* Clone() { return new BSOrderedNode(*this); }
};

class BSMultiBoundData : public NiObject {
};

class BSMultiBoundOBB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 size;
	float rotation[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

public:
	BSMultiBoundOBB() {}
	BSMultiBoundOBB(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundOBB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSMultiBoundOBB* Clone() { return new BSMultiBoundOBB(*this); }
};

class BSMultiBoundAABB : public BSMultiBoundData {
private:
	Vector3 center;
	Vector3 halfExtent;

public:
	BSMultiBoundAABB() {}
	BSMultiBoundAABB(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundAABB";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSMultiBoundAABB* Clone() { return new BSMultiBoundAABB(*this); }
};

class BSMultiBound : public NiObject {
private:
	BlockRef<BSMultiBoundData> dataRef;

public:
	BSMultiBound() {}
	BSMultiBound(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	BSMultiBound* Clone() { return new BSMultiBound(*this); }
};

enum BSCPCullingType : uint {
	BSCP_CULL_NORMAL,
	BSCP_CULL_ALLPASS,
	BSCP_CULL_ALLFAIL,
	BSCP_CULL_IGNOREMULTIBOUNDS,
	BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE
};

class BSMultiBoundNode : public NiNode {
private:
	BlockRef<BSMultiBound> multiBoundRef;
	BSCPCullingType cullingMode = BSCP_CULL_NORMAL;

public:
	BSMultiBoundNode();
	BSMultiBoundNode(NiStream& stream);

	static constexpr const char* BlockName = "BSMultiBoundNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	void GetChildRefs(std::set<int*>& refs);
	BSMultiBoundNode* Clone() { return new BSMultiBoundNode(*this); }
};

class BSBlastNode : public NiNode {
private:
	byte min = 0;
	byte max = 0;
	byte current = 0;

public:
	BSBlastNode();
	BSBlastNode(NiStream& stream);

	static constexpr const char* BlockName = "BSBlastNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

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

enum BillboardMode : ushort {
	ALWAYS_FACE_CAMERA,
	ROTATE_ABOUT_UP,
	RIGID_FACE_CAMERA,
	ALWAYS_FACE_CENTER,
	RIGID_FACE_CENTER,
	BSROTATE_ABOUT_UP,
	ROTATE_ABOUT_UP2 = 9
};

class NiBillboardNode : public NiNode {
private:
	BillboardMode billboardMode = ALWAYS_FACE_CAMERA;

public:
	NiBillboardNode();
	NiBillboardNode(NiStream& stream);

	static constexpr const char* BlockName = "NiBillboardNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiBillboardNode* Clone() { return new NiBillboardNode(*this); }
};

enum NiSwitchFlags : ushort {
	UPDATE_ONLY_ACTIVE_CHILD,
	UPDATE_CONTROLLERS
};

class NiSwitchNode : public NiNode {
private:
	NiSwitchFlags flags = UPDATE_ONLY_ACTIVE_CHILD;
	uint index = 0;

public:
	NiSwitchNode();
	NiSwitchNode(NiStream& stream);

	static constexpr const char* BlockName = "NiSwitchNode";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);

	NiSwitchNode* Clone() { return new NiSwitchNode(*this); }
};
