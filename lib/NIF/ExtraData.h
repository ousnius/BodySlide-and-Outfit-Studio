/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Keys.h"
#include "VertexData.h"
#include "utils/half.hpp"

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
	static constexpr const char* BlockName = "NiBinaryExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiBinaryExtraData* Clone() { return new NiBinaryExtraData(*this); }

	std::vector<byte> GetData();
	void SetData(const std::vector<byte>& dat);
};

class NiFloatExtraData : public NiExtraData {
private:
	float floatData = 0.0f;

public:
	static constexpr const char* BlockName = "NiFloatExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiFloatExtraData* Clone() { return new NiFloatExtraData(*this); }

	float GetFloatData();
	void SetFloatData(const float fltData);
};

class NiFloatsExtraData : public NiExtraData {
private:
	uint numFloats = 0;
	std::vector<float> floatsData;

public:
	static constexpr const char* BlockName = "NiFloatsExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiFloatsExtraData* Clone() { return new NiFloatsExtraData(*this); }

	std::vector<float> GetFloatsData();
	void SetFloatsData(const std::vector<float>& fltsData);
};

class NiStringExtraData : public NiExtraData {
private:
	StringRef stringData;

public:
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
	static constexpr const char* BlockName = "NiStringsExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiStringsExtraData* Clone() { return new NiStringsExtraData(*this); }

	std::vector<NiString> GetStringsData();
	void SetStringsData(const std::vector<NiString>& strsData);
};

class NiBooleanExtraData : public NiExtraData {
private:
	bool booleanData = false;

public:
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
	static constexpr const char* BlockName = "NiIntegerExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiIntegerExtraData* Clone() { return new NiIntegerExtraData(*this); }

	uint GetIntegerData();
	void SetIntegerData(const uint intData);
};

class NiIntegersExtraData : public NiExtraData {
private:
	uint numIntegers = 0;
	std::vector<uint> integersData;

public:
	static constexpr const char* BlockName = "NiIntegersExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiIntegersExtraData* Clone() { return new NiIntegersExtraData(*this); }

	std::vector<uint> GetIntegersData();
	void SetIntegersData(const std::vector<uint>& intData);
};

class NiVectorExtraData : public NiExtraData {
private:
	Vector4 vectorData;

public:
	static constexpr const char* BlockName = "NiVectorExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiVectorExtraData* Clone() { return new NiVectorExtraData(*this); }

	Vector4 GetVectorData();
	void SetVectorData(const Vector4& vecData);
};

class NiColorExtraData : public NiExtraData {
private:
	Color4 colorData;

public:
	static constexpr const char* BlockName = "NiColorExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	NiColorExtraData* Clone() { return new NiColorExtraData(*this); }

	Color4 GetColorData();
	void SetColorData(const Color4& colData);
};

class BSXFlags : public NiIntegerExtraData {
public:
	static constexpr const char* BlockName = "BSXFlags";
	virtual const char* GetBlockName() { return BlockName; }

	BSXFlags* Clone() { return new BSXFlags(*this); }
};

class BSWArray : public NiExtraData {
private:
	uint numData = 0;
	std::vector<uint> data;

public:
	static constexpr const char* BlockName = "BSWArray";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSWArray* Clone() { return new BSWArray(*this); }

	std::vector<uint> GetData();
	void SetData(const std::vector<uint>& dat);
};

class BSPositionData : public NiExtraData {
private:
	uint numData = 0;
	std::vector<half_float::half> data;

public:
	static constexpr const char* BlockName = "BSPositionData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPositionData* Clone() { return new BSPositionData(*this); }

	std::vector<half_float::half> GetData();
	void SetData(const std::vector<half_float::half>& dat);
};

class BSEyeCenterExtraData : public NiExtraData {
private:
	uint numData = 0;
	std::vector<float> data;

public:
	static constexpr const char* BlockName = "BSEyeCenterExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSEyeCenterExtraData* Clone() { return new BSEyeCenterExtraData(*this); }

	std::vector<float> GetData();
	void SetData(const std::vector<float>& dat);
};

struct BSPackedGeomObject {
	uint unkInt1 = 0;
	uint objectHash = 0;
};

struct BSPackedGeomDataLOD {
	uint triangleCount = 0;
	uint triangleOffset = 0;
};

struct BSPackedGeomDataCombined {
	float grayscaleToPaletteScale = 1.0f;
	Matrix3 rotation;
	Vector3 translation;
	float scale = 1.0f;
	BoundingSphere bounds;
};

struct BSPackedGeomData {
	uint numVerts = 0;
	uint lodLevels = 0;
	std::vector<BSPackedGeomDataLOD> lod;
	uint numCombined = 0;
	std::vector<BSPackedGeomDataCombined> combined;
	uint unkInt1 = 0;
	uint unkInt2 = 0;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class BSPackedCombinedSharedGeomDataExtra : public NiExtraData {
private:
	VertexDesc vertDesc;
	uint numVertices = 0;
	uint numTriangles = 0;
	uint unkFlags1 = 0;
	uint unkFlags2 = 0;
	uint numData = 0;
	std::vector<BSPackedGeomObject> objects;
	std::vector<BSPackedGeomData> data;

public:
	static constexpr const char* BlockName = "BSPackedCombinedSharedGeomDataExtra";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSPackedCombinedSharedGeomDataExtra* Clone() { return new BSPackedCombinedSharedGeomDataExtra(*this); }
};

class BSInvMarker : public NiExtraData {
private:
	ushort rotationX = 4712;
	ushort rotationY = 6283;
	ushort rotationZ = 0;
	float zoom = 1.0f;

public:
	static constexpr const char* BlockName = "BSInvMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSInvMarker* Clone() { return new BSInvMarker(*this); }

	float GetZoom() { return zoom; }
	void SetZoom(const float z) { zoom = z; }
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
	static constexpr const char* BlockName = "BSFurnitureMarker";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSFurnitureMarker* Clone() { return new BSFurnitureMarker(*this); }

	std::vector<FurniturePosition> GetPositions();
	void SetPositions(const std::vector<FurniturePosition>& pos);
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {
public:
	static constexpr const char* BlockName = "BSFurnitureMarkerNode";
	virtual const char* GetBlockName() { return BlockName; }

	BSFurnitureMarkerNode* Clone() { return new BSFurnitureMarkerNode(*this); }
};

struct DecalVectorBlock {
	ushort numVectors;
	std::vector<Vector3> points;
	std::vector<Vector3> normals;
};

class BSDecalPlacementVectorExtraData : public NiFloatExtraData {
private:
	ushort numVectorBlocks = 0;
	std::vector<DecalVectorBlock> decalVectorBlocks;

public:
	static constexpr const char* BlockName = "BSDecalPlacementVectorExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSDecalPlacementVectorExtraData* Clone() { return new BSDecalPlacementVectorExtraData(*this); }

	std::vector<DecalVectorBlock> GetDecalVectorBlocks();
	void SetDecalVectorBlocks(const std::vector<DecalVectorBlock>& vectorBlocks);
};

class BSBehaviorGraphExtraData : public NiExtraData {
private:
	StringRef behaviorGraphFile;
	bool controlsBaseSkel = false;

public:
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
	static constexpr const char* BlockName = "BSBound";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSBound* Clone() { return new BSBound(*this); }

	Vector3 GetCenter() { return center; }
	void SetCenter(const Vector3& ctr) { center = ctr; }

	Vector3 GetHalfExtents() { return halfExtents; }
	void SetHalfExtents(const Vector3& hExtents) { halfExtents = hExtents; }
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
	static constexpr const char* BlockName = "BSBoneLODExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	BSBoneLODExtraData* Clone() { return new BSBoneLODExtraData(*this); }

	std::vector<BoneLOD> GetBoneLODs();
	void SetBoneLODs(const std::vector<BoneLOD>& lods);
};

class NiTextKeyExtraData : public NiExtraData {
private:
	uint numTextKeys = 0;
	std::vector<Key<StringRef>> textKeys;

public:
	static constexpr const char* BlockName = "NiTextKeyExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	NiTextKeyExtraData* Clone() { return new NiTextKeyExtraData(*this); }

	std::vector<Key<StringRef>> GetTextKeys();
	void SetTextKeys(const std::vector<Key<StringRef>>& keys);
};

class BSDistantObjectLargeRefExtraData : public NiExtraData {
private:
	bool largeRef = true;

public:
	static constexpr const char* BlockName = "BSDistantObjectLargeRefExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSDistantObjectLargeRefExtraData* Clone() { return new BSDistantObjectLargeRefExtraData(*this); }

	bool IsLargeRef() { return largeRef; }
	void SetLargeRef(const bool isLargeRef) { largeRef = isLargeRef; }
};

struct BSConnectPoint {
	NiString root;
	NiString variableName;
	Quaternion rotation;
	Vector3 translation;
	float scale = 1.0f;

	void Get(NiStream& stream);
	void Put(NiStream& stream);
};

class BSConnectPointParents : public NiExtraData {
private:
	uint numConnectPoints = 0;
	std::vector<BSConnectPoint> connectPoints;

public:
	static constexpr const char* BlockName = "BSConnectPoint::Parents";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSConnectPointParents* Clone() { return new BSConnectPointParents(*this); }

	std::vector<BSConnectPoint> GetConnectPoints();
	void SetConnectPoints(const std::vector<BSConnectPoint>& cps);
};

class BSConnectPointChildren : public NiExtraData {
private:
	byte unkByte = 1;
	uint numTargets = 0;
	std::vector<NiString> targets;

public:
	static constexpr const char* BlockName = "BSConnectPoint::Children";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSConnectPointChildren* Clone() { return new BSConnectPointChildren(*this); }

	std::vector<NiString> GetTargets();
	void SetTargets(const std::vector<NiString>& targ);
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

	static constexpr const char* BlockName = "BSClothExtraData";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	BSClothExtraData* Clone() { return new BSClothExtraData(*this); }

	std::vector<char> GetData();
	void SetData(const std::vector<char>& dat);

	bool ToHKX(const std::string& fileName);
	bool FromHKX(const std::string& fileName);
};
