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
	~NiExtraData() {
		name.Clear(header);
	};

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();

	std::string GetName();
	void SetName(const std::string& extraDataName);
};

class NiBinaryExtraData : public NiExtraData {
private:
	uint size;
	std::vector<byte> data;

public:
	NiBinaryExtraData(NiHeader* hdr);
	NiBinaryExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBinaryExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBinaryExtraData* Clone() { return new NiBinaryExtraData(*this); }
};

class NiFloatExtraData : public NiExtraData {
private:
	float floatData;

public:
	NiFloatExtraData(NiHeader* hdr);
	NiFloatExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiFloatExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiFloatExtraData* Clone() { return new NiFloatExtraData(*this); }
};

class NiStringExtraData : public NiExtraData {
private:
	StringRef stringData;

public:
	NiStringExtraData(NiHeader* hdr);
	NiStringExtraData(std::fstream& file, NiHeader* hdr);
	~NiStringExtraData() {
		stringData.Clear(header);
	};

	static constexpr const char* BlockName = "NiStringExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	NiStringExtraData* Clone() { return new NiStringExtraData(*this); }

	std::string GetStringData();
	void SetStringData(const std::string& str);
};

class NiStringsExtraData : public NiExtraData {
private:
	uint numStrings;
	std::vector<NiString> stringsData;

public:
	NiStringsExtraData(NiHeader* hdr);
	NiStringsExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiStringsExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiStringsExtraData* Clone() { return new NiStringsExtraData(*this); }
};

class NiBooleanExtraData : public NiExtraData {
private:
	bool booleanData;

public:
	NiBooleanExtraData(NiHeader* hdr);
	NiBooleanExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiBooleanExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiBooleanExtraData* Clone() { return new NiBooleanExtraData(*this); }

	bool GetBooleanData();
	void SetBooleanData(const bool boolData);
};

class NiIntegerExtraData : public NiExtraData {
private:
	uint integerData;

public:
	NiIntegerExtraData(NiHeader* hdr);
	NiIntegerExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "NiIntegerExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	NiIntegerExtraData* Clone() { return new NiIntegerExtraData(*this); }

	uint GetIntegerData();
	void SetIntegerData(const uint intData);
};

class BSXFlags : public NiIntegerExtraData {
public:
	BSXFlags(NiHeader* hdr);
	BSXFlags(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSXFlags";
	virtual const char* GetBlockName() { return BlockName; }

	BSXFlags* Clone() { return new BSXFlags(*this); }
};

class BSInvMarker : public NiExtraData {
private:
	ushort rotationX;
	ushort rotationY;
	ushort rotationZ;
	float zoom;

public:
	BSInvMarker(NiHeader* hdr);
	BSInvMarker(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSInvMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
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
	uint numPositions;
	std::vector<FurniturePosition> positions;

public:
	BSFurnitureMarker(NiHeader* hdr);
	BSFurnitureMarker(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSFurnitureMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSFurnitureMarker* Clone() { return new BSFurnitureMarker(*this); }
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {
public:
	BSFurnitureMarkerNode(NiHeader* hdr);
	BSFurnitureMarkerNode(std::fstream& file, NiHeader* hdr);

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
	ushort numVectorBlocks;
	std::vector<DecalVectorBlock> decalVectorBlocks;

public:
	BSDecalPlacementVectorExtraData(NiHeader* hdr);
	BSDecalPlacementVectorExtraData(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSDecalPlacementVectorExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSDecalPlacementVectorExtraData* Clone() { return new BSDecalPlacementVectorExtraData(*this); }
};

class BSBehaviorGraphExtraData : public NiExtraData {
private:
	StringRef behaviorGraphFile;
	byte controlsBaseSkel;

public:
	BSBehaviorGraphExtraData(NiHeader* hdr);
	BSBehaviorGraphExtraData(std::fstream& file, NiHeader* hdr);
	~BSBehaviorGraphExtraData() {
		behaviorGraphFile.Clear(header);
	};

	static constexpr const char* BlockName = "BSBehaviorGraphExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	BSBehaviorGraphExtraData* Clone() { return new BSBehaviorGraphExtraData(*this); }
};

class BSBound : public NiExtraData {
private:
	Vector3 center;
	Vector3 halfExtents;

public:
	BSBound(NiHeader* hdr);
	BSBound(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSBound* Clone() { return new BSBound(*this); }
};

struct BoneLOD {
	uint distance;
	StringRef boneName;
};

class BSBoneLODExtraData : public NiExtraData {
private:
	uint numBoneLODs;
	std::vector<BoneLOD> boneLODs;

public:
	BSBoneLODExtraData(NiHeader* hdr);
	BSBoneLODExtraData(std::fstream& file, NiHeader* hdr);
	~BSBoneLODExtraData() {
		for (auto &lod : boneLODs)
			lod.boneName.Clear(header);
	};

	static constexpr const char* BlockName = "BSBoneLODExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	void notifyStringDelete(int stringID);
	int CalcBlockSize();
	BSBoneLODExtraData* Clone() { return new BSBoneLODExtraData(*this); }
};

class NiTextKeyExtraData : public NiExtraData {
private:
	uint numTextKeys;
	std::vector<Key<StringRef>> textKeys;

public:
	NiTextKeyExtraData(NiHeader* hdr);
	NiTextKeyExtraData(std::fstream& file, NiHeader* hdr);
	~NiTextKeyExtraData() {
		for (auto &tk : textKeys)
			tk.value.Clear(header);
	};

	static constexpr const char* BlockName = "NiTextKeyExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
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
	BSConnectPoint(std::fstream& file);

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSConnectPoint* Clone() { return new BSConnectPoint(*this); }
};

class BSConnectPointParents : public NiExtraData {
private:
	uint numConnectPoints;
	std::vector<BSConnectPoint> connectPoints;

public:
	BSConnectPointParents(NiHeader* hdr);
	BSConnectPointParents(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSConnectPoint::Parents";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSConnectPointParents* Clone() { return new BSConnectPointParents(*this); }
};

class BSConnectPointChildren : public NiExtraData {
private:
	byte unkByte;
	uint numTargets;
	std::vector<NiString> targets;

public:
	BSConnectPointChildren(NiHeader* hdr);
	BSConnectPointChildren(std::fstream& file, NiHeader* hdr);

	static constexpr const char* BlockName = "BSConnectPoint::Children";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSConnectPointChildren* Clone() { return new BSConnectPointChildren(*this); }
};

class BSExtraData : public NiObject {
};

class BSClothExtraData : public BSExtraData {
private:
	uint numBytes = 0;
	std::vector<char> data;

public:
	BSClothExtraData(NiHeader* hdr, const uint size = 0);
	BSClothExtraData(std::fstream& file, NiHeader* hdr);

	void Get(std::fstream& file);
	void Put(std::fstream& file);
	int CalcBlockSize();
	BSClothExtraData* Clone() { return new BSClothExtraData(*this); }

	bool ToHKX(const std::string& fileName);
	bool FromHKX(const std::string& fileName);
};
