/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Objects.h"

void NiObjectNET::Init() {
	NiObject::Init();

	bBSLightingShaderProperty = false;
	skyrimShaderType = 0;
}

void NiObjectNET::Get(NiStream& stream) {
	NiObject::Get(stream);

	if (bBSLightingShaderProperty && stream.GetVersion().User() >= 12)
		stream >> skyrimShaderType;

	name.Get(stream);

	extraDataRefs.Get(stream);
	controllerRef.Get(stream);
}

void NiObjectNET::Put(NiStream& stream) {
	NiObject::Put(stream);

	if (bBSLightingShaderProperty && stream.GetVersion().User() >= 12)
		stream << skyrimShaderType;

	name.Put(stream);

	extraDataRefs.Put(stream);
	controllerRef.Put(stream);
}

void NiObjectNET::GetStringRefs(std::set<int*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name.index);
}

std::string NiObjectNET::GetName(NiHeader* hdr) {
	return name.GetString(hdr);
}

void NiObjectNET::SetName(NiHeader* hdr, const std::string& str, const bool rename) {
	if (rename)
		name.RenameString(hdr, str);
	else
		name.SetString(hdr, str);
}

void NiObjectNET::ClearName() {
	name.Clear();
}

int NiObjectNET::GetControllerRef() {
	return controllerRef.index;
}

void NiObjectNET::SetControllerRef(int ctlrRef) {
	this->controllerRef.index = ctlrRef;
}

int NiObjectNET::GetNumExtraData() {
	return extraDataRefs.GetSize();
}

void NiObjectNET::SetExtraDataRef(const int id, const int blockId) {
	extraDataRefs.SetBlockRef(id, blockId);
}

int NiObjectNET::GetExtraDataRef(const int id) {
	return extraDataRefs.GetBlockRef(id);
}

void NiObjectNET::AddExtraDataRef(const int id) {
	extraDataRefs.AddBlockRef(id);
}

void NiObjectNET::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	extraDataRefs.GetIndexPtrs(refs);
	refs.insert(&controllerRef.index);
}

int NiObjectNET::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	if (bBSLightingShaderProperty && version.User() >= 12)
		blockSize += 4;

	blockSize += 12;
	blockSize += extraDataRefs.GetSize() * 4;

	return blockSize;
}


void NiAVObject::Init() {
	NiObjectNET::Init();

	flags = 524302;
	rotation[0].x = 1.0f;
	rotation[1].y = 1.0f;
	rotation[2].z = 1.0f;
	scale = 1.0f;
}

void NiAVObject::Get(NiStream& stream) {
	NiObjectNET::Get(stream);

	flags = 0;
	if (stream.GetVersion().User2() <= 26)
		stream.read((char*)&flags, 2);
	else
		stream >> flags;

	stream >> translation;

	for (int i = 0; i < 3; i++)
		stream >> rotation[i];

	stream >> scale;

	if (stream.GetVersion().User() <= 11)
		propertyRefs.Get(stream);
	
	collisionRef.Get(stream);
}

void NiAVObject::Put(NiStream& stream) {
	NiObjectNET::Put(stream);

	if (stream.GetVersion().User2() <= 26)
		stream.write((char*)&flags, 2);
	else
		stream << flags;

	stream << translation;

	for (int i = 0; i < 3; i++)
		stream << rotation[i];

	stream << scale;

	if (stream.GetVersion().User() <= 11)
		propertyRefs.Put(stream);

	collisionRef.Put(stream);
}

void NiAVObject::GetChildRefs(std::set<int*>& refs) {
	NiObjectNET::GetChildRefs(refs);

	propertyRefs.GetIndexPtrs(refs);
	refs.insert(&collisionRef.index);
}

int NiAVObject::CalcBlockSize(NiVersion& version) {
	NiObjectNET::CalcBlockSize(version);

	blockSize += 58;

	if (version.User() <= 11) {
		blockSize += 4;
		blockSize += propertyRefs.GetSize() * 4;
	}

	if (version.User2() > 26)
		blockSize += 2;

	return blockSize;
}


NiDefaultAVObjectPalette::NiDefaultAVObjectPalette() {
	NiAVObjectPalette::Init();
}

NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(NiStream& stream) : NiDefaultAVObjectPalette() {
	Get(stream);
}

void NiDefaultAVObjectPalette::Get(NiStream& stream) {
	NiAVObjectPalette::Get(stream);

	sceneRef.Get(stream);

	stream >> numObjects;
	objects.resize(numObjects);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Get(stream, 4);
		objects[i].objectRef.Get(stream);
	}
}

void NiDefaultAVObjectPalette::Put(NiStream& stream) {
	NiAVObjectPalette::Put(stream);

	sceneRef.Put(stream);

	stream << numObjects;
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Put(stream, 4, false);
		objects[i].objectRef.Put(stream);
	}
}

void NiDefaultAVObjectPalette::GetChildRefs(std::set<int*>& refs) {
	NiAVObjectPalette::GetChildRefs(refs);

	refs.insert(&sceneRef.index);

	for (int i = 0; i < numObjects; i++)
		refs.insert(&objects[i].objectRef.index);
}

int NiDefaultAVObjectPalette::CalcBlockSize(NiVersion& version) {
	NiAVObjectPalette::CalcBlockSize(version);

	blockSize += 8;
	blockSize += numObjects * 8;

	for (int i = 0; i < numObjects; i++)
		blockSize += objects[i].name.GetLength();

	return blockSize;
}


NiCamera::NiCamera() {
	NiAVObject::Init();
}

NiCamera::NiCamera(NiStream& stream) : NiCamera() {
	Get(stream);
}

void NiCamera::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	stream >> obsoleteFlags;
	stream >> frustumLeft;
	stream >> frustumRight;
	stream >> frustumTop;
	stream >> frustomBottom;
	stream >> frustumNear;
	stream >> frustumFar;
	stream >> useOrtho;
	stream >> viewportLeft;
	stream >> viewportRight;
	stream >> viewportTop;
	stream >> viewportBottom;
	stream >> lodAdjust;

	sceneRef.Get(stream);
	stream >> numScreenPolygons;
	stream >> numScreenTextures;
}

void NiCamera::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	stream << obsoleteFlags;
	stream << frustumLeft;
	stream << frustumRight;
	stream << frustumTop;
	stream << frustomBottom;
	stream << frustumNear;
	stream << frustumFar;
	stream << useOrtho;
	stream << viewportLeft;
	stream << viewportRight;
	stream << viewportTop;
	stream << viewportBottom;
	stream << lodAdjust;

	sceneRef.Put(stream);
	stream << numScreenPolygons;
	stream << numScreenTextures;
}

void NiCamera::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&sceneRef.index);
}

int NiCamera::CalcBlockSize(NiVersion& version) {
	NiAVObject::CalcBlockSize(version);

	blockSize += 59;

	return blockSize;
}


NiNode::NiNode() {
	NiAVObject::Init();
}

NiNode::NiNode(NiStream& stream) : NiNode() {
	Get(stream);
}

void NiNode::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	childRefs.Get(stream);

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().User2() < 130)
		effectRefs.Get(stream);
}

void NiNode::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	childRefs.Put(stream);

	if (stream.GetVersion().User() <= 12 && stream.GetVersion().User2() < 130)
		effectRefs.Put(stream);
}

void NiNode::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	childRefs.GetIndexPtrs(refs);
	effectRefs.GetIndexPtrs(refs);
}

int NiNode::CalcBlockSize(NiVersion& version) {
	NiAVObject::CalcBlockSize(version);

	blockSize += 4;
	blockSize += childRefs.GetSize() * 4;
	if (version.User() <= 12 && version.User2() < 130) {
		blockSize += 4;
		blockSize += effectRefs.GetSize() * 4;
	}
	return blockSize;
}

int NiNode::GetChildRef(const int id) {
	if (id >= 0 && id < childRefs.GetSize())
		return childRefs.GetBlockRef(id);

	return 0xFFFFFFFF;
}

void NiNode::AddChildRef(const int id) {
	childRefs.AddBlockRef(id);
}

void NiNode::ClearChildren() {
	childRefs.Clear();
}

int NiNode::GetEffectRef(const int id) {
	if (id >= 0 && id < effectRefs.GetSize())
		return effectRefs.GetBlockRef(id);

	return 0xFFFFFFFF;
}

void NiNode::AddEffectRef(const int id) {
	effectRefs.AddBlockRef(id);
}


BSFadeNode::BSFadeNode() : NiNode() {
}

BSFadeNode::BSFadeNode(NiStream& stream) : BSFadeNode() {
	Get(stream);
}


BSValueNode::BSValueNode() : NiNode() {
}

BSValueNode::BSValueNode(NiStream& stream) : BSValueNode() {
	Get(stream);
}

void BSValueNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> value;
	stream >> valueFlags;
}

void BSValueNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << value;
	stream << valueFlags;
}

int BSValueNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 5;

	return blockSize;
}


BSLeafAnimNode::BSLeafAnimNode() : NiNode() {
}

BSLeafAnimNode::BSLeafAnimNode(NiStream& stream) : BSLeafAnimNode() {
	Get(stream);
}


BSTreeNode::BSTreeNode() : NiNode() {
}

BSTreeNode::BSTreeNode(NiStream& stream) : BSTreeNode() {
	Get(stream);
}

void BSTreeNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	bones1.Get(stream);
	bones2.Get(stream);
}

void BSTreeNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	bones1.Put(stream);
	bones2.Put(stream);
}

void BSTreeNode::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	bones1.GetIndexPtrs(refs);
	bones2.GetIndexPtrs(refs);
}

int BSTreeNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 8;
	blockSize += bones1.GetSize() * 4;
	blockSize += bones2.GetSize() * 4;

	return blockSize;
}


BSOrderedNode::BSOrderedNode() : NiNode() {
}

BSOrderedNode::BSOrderedNode(NiStream& stream) : BSOrderedNode() {
	Get(stream);
}

void BSOrderedNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> alphaSortBound;
	stream >> isStaticBound;
}

void BSOrderedNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << alphaSortBound;
	stream << isStaticBound;
}

int BSOrderedNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 17;

	return blockSize;
}


BSMultiBoundOBB::BSMultiBoundOBB() {
	NiObject::Init();
}

BSMultiBoundOBB::BSMultiBoundOBB(NiStream& stream) : BSMultiBoundOBB() {
	Get(stream);
}

void BSMultiBoundOBB::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> center;
	stream >> size;
	stream.read((char*)rotation, 36);
}

void BSMultiBoundOBB::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << center;
	stream << size;
	stream.write((char*)rotation, 36);
}

int BSMultiBoundOBB::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 60;

	return blockSize;
}


BSMultiBoundAABB::BSMultiBoundAABB() {
	NiObject::Init();
}

BSMultiBoundAABB::BSMultiBoundAABB(NiStream& stream) : BSMultiBoundAABB() {
	Get(stream);
}

void BSMultiBoundAABB::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> center;
	stream >> halfExtent;
}

void BSMultiBoundAABB::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << center;
	stream << halfExtent;
}

int BSMultiBoundAABB::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 24;

	return blockSize;
}


BSMultiBound::BSMultiBound() {
	NiObject::Init();
}

BSMultiBound::BSMultiBound(NiStream& stream) : BSMultiBound() {
	Get(stream);
}

void BSMultiBound::Get(NiStream& stream) {
	NiObject::Get(stream);

	dataRef.Get(stream);
}

void BSMultiBound::Put(NiStream& stream) {
	NiObject::Put(stream);

	dataRef.Put(stream);
}

void BSMultiBound::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int BSMultiBound::CalcBlockSize(NiVersion& version) {
	NiObject::CalcBlockSize(version);

	blockSize += 4;

	return blockSize;
}


BSMultiBoundNode::BSMultiBoundNode() : NiNode() {
}

BSMultiBoundNode::BSMultiBoundNode(NiStream& stream) : BSMultiBoundNode() {
	Get(stream);
}

void BSMultiBoundNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	multiBoundRef.Get(stream);

	if (stream.GetVersion().User() >= 12)
		stream >> cullingMode;
}

void BSMultiBoundNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	multiBoundRef.Put(stream);

	if (stream.GetVersion().User() >= 12)
		stream << cullingMode;
}

void BSMultiBoundNode::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	refs.insert(&multiBoundRef.index);
}

int BSMultiBoundNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 4;
	if (version.User() >= 12)
		blockSize += 4;

	return blockSize;
}


BSBlastNode::BSBlastNode() : NiNode() {
}

BSBlastNode::BSBlastNode(NiStream& stream) : BSBlastNode() {
	Get(stream);
}

void BSBlastNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> min;
	stream >> max;
	stream >> current;
}

void BSBlastNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << min;
	stream << max;
	stream << current;
}

int BSBlastNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 3;

	return blockSize;
}


BSDamageStage::BSDamageStage() : BSBlastNode() {
}

BSDamageStage::BSDamageStage(NiStream& stream) : BSDamageStage() {
	Get(stream);
}


NiBillboardNode::NiBillboardNode() : NiNode() {
}

NiBillboardNode::NiBillboardNode(NiStream& stream) : NiBillboardNode() {
	Get(stream);
}

void NiBillboardNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> billboardMode;
}

void NiBillboardNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << billboardMode;
}

int NiBillboardNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 2;

	return blockSize;
}


NiSwitchNode::NiSwitchNode() : NiNode() {
}

NiSwitchNode::NiSwitchNode(NiStream& stream) : NiSwitchNode() {
	Get(stream);
}

void NiSwitchNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> flags;
	stream >> index;
}

void NiSwitchNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << flags;
	stream << index;
}

int NiSwitchNode::CalcBlockSize(NiVersion& version) {
	NiNode::CalcBlockSize(version);

	blockSize += 6;

	return blockSize;
}
