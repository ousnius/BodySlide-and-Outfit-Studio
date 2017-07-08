/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Keys.h"

class NiExtraData : public NiObject {
private:
	StringRef name;

public:
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);

	std::string GetName();
	void SetName(const std::string& extraDataName);
};

class NiBinaryExtraData : public NiExtraData {
private:
	uint size = 0;
	std::vector<byte> data;

public:
	NiBinaryExtraData() {}
	NiBinaryExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiBinaryExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBinaryExtraData* Clone() { return new NiBinaryExtraData(*this); }
};

class NiFloatExtraData : public NiExtraData {
private:
	float floatData = 0.0f;

public:
	NiFloatExtraData() {}
	NiFloatExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiFloatExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiFloatExtraData* Clone() { return new NiFloatExtraData(*this); }
};

class NiStringExtraData : public NiExtraData {
private:
	StringRef stringData;

public:
	NiStringExtraData() {}
	NiStringExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiStringExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	NiStringExtraData* Clone() { return new NiStringExtraData(*this); }

	std::string GetStringData();
	void SetStringData(const std::string& str);
};

class NiStringsExtraData : public NiExtraData {
private:
	uint numStrings = 0;
	std::vector<NiString> stringsData;

public:
	NiStringsExtraData() {}
	NiStringsExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiStringsExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiStringsExtraData* Clone() { return new NiStringsExtraData(*this); }
};

class NiBooleanExtraData : public NiExtraData {
private:
	bool booleanData = false;

public:
	NiBooleanExtraData() {}
	NiBooleanExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiBooleanExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBooleanExtraData* Clone() { return new NiBooleanExtraData(*this); }

	bool GetBooleanData();
	void SetBooleanData(const bool boolData);
};

class NiIntegerExtraData : public NiExtraData {
private:
	uint integerData = 0;

public:
	NiIntegerExtraData() {}
	NiIntegerExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiIntegerExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiIntegerExtraData* Clone() { return new NiIntegerExtraData(*this); }

	uint GetIntegerData();
	void SetIntegerData(const uint intData);
};

class BSXFlags : public NiIntegerExtraData {
public:
	BSXFlags();
	BSXFlags(NiStream& stream);

	static constexpr const char* BlockName = "BSXFlags";
	virtual const char* GetBlockName() { return BlockName; }

	BSXFlags* Clone() { return new BSXFlags(*this); }
};

class BSInvMarker : public NiExtraData {
private:
	ushort rotationX = 4712;
	ushort rotationY = 6283;
	ushort rotationZ = 0;
	float zoom = 1.0f;

public:
	BSInvMarker() {}
	BSInvMarker(NiStream& stream);

	static constexpr const char* BlockName = "BSInvMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSInvMarker* Clone() { return new BSInvMarker(*this); }
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

class BSFurnitureMarker : public NiExtraData {
private:
	uint numPositions = 0;
	std::vector<FurniturePosition> positions;

public:
	BSFurnitureMarker() {}
	BSFurnitureMarker(NiStream& stream);

	static constexpr const char* BlockName = "BSFurnitureMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSFurnitureMarker* Clone() { return new BSFurnitureMarker(*this); }
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {
public:
	BSFurnitureMarkerNode();
	BSFurnitureMarkerNode(NiStream& stream);

	static constexpr const char* BlockName = "BSFurnitureMarkerNode";
	virtual const char* GetBlockName() { return BlockName; }

	BSFurnitureMarkerNode* Clone() { return new BSFurnitureMarkerNode(*this); }
};

class BSDecalPlacementVectorExtraData : public NiFloatExtraData {
public:
	struct DecalVectorBlock {
		ushort numVectors;
		std::vector<Vector3> points;
		std::vector<Vector3> normals;
	};

private:
	ushort numVectorBlocks = 0;
	std::vector<DecalVectorBlock> decalVectorBlocks;

public:
	BSDecalPlacementVectorExtraData();
	BSDecalPlacementVectorExtraData(NiStream& stream);

	static constexpr const char* BlockName = "BSDecalPlacementVectorExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSDecalPlacementVectorExtraData* Clone() { return new BSDecalPlacementVectorExtraData(*this); }
};

class BSBehaviorGraphExtraData : public NiExtraData {
private:
	StringRef behaviorGraphFile;
	bool controlsBaseSkel = false;

public:
	BSBehaviorGraphExtraData() {}
	BSBehaviorGraphExtraData(NiStream& stream);

	static constexpr const char* BlockName = "BSBehaviorGraphExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	BSBehaviorGraphExtraData* Clone() { return new BSBehaviorGraphExtraData(*this); }
};

class BSBound : public NiExtraData {
private:
	Vector3 center;
	Vector3 halfExtents;

public:
	BSBound() {}
	BSBound(NiStream& stream);

	static constexpr const char* BlockName = "BSBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSBound* Clone() { return new BSBound(*this); }
};

struct BoneLOD {
	uint distance;
	StringRef boneName;
};

class BSBoneLODExtraData : public NiExtraData {
private:
	uint numBoneLODs = 0;
	std::vector<BoneLOD> boneLODs;

public:
	BSBoneLODExtraData() {}
	BSBoneLODExtraData(NiStream& stream);

	static constexpr const char* BlockName = "BSBoneLODExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	BSBoneLODExtraData* Clone() { return new BSBoneLODExtraData(*this); }
};

class NiTextKeyExtraData : public NiExtraData {
private:
	uint numTextKeys = 0;
	std::vector<Key<StringRef>> textKeys;

public:
	NiTextKeyExtraData() {}
	NiTextKeyExtraData(NiStream& stream);

	static constexpr const char* BlockName = "NiTextKeyExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	NiTextKeyExtraData* Clone() { return new NiTextKeyExtraData(*this); }
};

class BSDistantObjectLargeRefExtraData : public NiExtraData {
private:
	bool largeRef = true;

public:
	BSDistantObjectLargeRefExtraData() {}
	BSDistantObjectLargeRefExtraData(NiStream& stream);

	static constexpr const char* BlockName = "BSDistantObjectLargeRefExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSDistantObjectLargeRefExtraData* Clone() { return new BSDistantObjectLargeRefExtraData(*this); }
};

class BSConnectPoint {
public:
	NiString root;
	NiString variableName;
	Quaternion rotation;
	Vector3 translation;
	float scale = 1.0f;

	BSConnectPoint();
	BSConnectPoint(NiStream& stream);

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSConnectPoint* Clone() { return new BSConnectPoint(*this); }
};

class BSConnectPointParents : public NiExtraData {
private:
	uint numConnectPoints = 0;
	std::vector<BSConnectPoint> connectPoints;

public:
	BSConnectPointParents() {}
	BSConnectPointParents(NiStream& stream);

	static constexpr const char* BlockName = "BSConnectPoint::Parents";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSConnectPointParents* Clone() { return new BSConnectPointParents(*this); }
};

class BSConnectPointChildren : public NiExtraData {
private:
	byte unkByte = 1;
	uint numTargets = 0;
	std::vector<NiString> targets;

public:
	BSConnectPointChildren() {}
	BSConnectPointChildren(NiStream& stream);

	static constexpr const char* BlockName = "BSConnectPoint::Children";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSConnectPointChildren* Clone() { return new BSConnectPointChildren(*this); }
};

class BSExtraData : public NiObject {
};

class BSClothExtraData : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	BSClothExtraData() {}
	BSClothExtraData(const uint size);
	BSClothExtraData(NiStream& stream);

	static constexpr const char* BlockName = "BSClothExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSClothExtraData* Clone() { return new BSClothExtraData(*this); }

	bool ToHKX(const std::string& fileName);
	bool FromHKX(const std::string& fileName);
};
