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
