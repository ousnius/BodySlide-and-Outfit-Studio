/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Objects.h"

void NiObjectNET::Init(NiHeader* hdr) {
	NiObject::Init(hdr);

	bBSLightingShaderProperty = false;
	skyrimShaderType = 0;
}

void NiObjectNET::Get(std::fstream& file) {
	NiObject::Get(file);

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
		file.read((char*)&skyrimShaderType, 4);

	name.Get(file, header);

	extraDataRefs.Get(file);
	controllerRef.Get(file);
}

void NiObjectNET::Put(std::fstream& file) {
	NiObject::Put(file);

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
		file.write((char*)&skyrimShaderType, 4);

	name.Put(file);

	extraDataRefs.Put(file);
	controllerRef.Put(file);
}

void NiObjectNET::notifyStringDelete(int stringID) {
	NiObject::notifyStringDelete(stringID);

	name.notifyStringDelete(stringID);
}

std::string NiObjectNET::GetName() {
	return name.GetString(header);
}

void NiObjectNET::SetName(const std::string& str, const bool rename) {
	if (rename)
		name.RenameString(header, str);
	else
		name.SetString(header, str);
}

void NiObjectNET::ClearName() {
	name.Clear(header);
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

int NiObjectNET::CalcBlockSize() {
	NiObject::CalcBlockSize();

	if (bBSLightingShaderProperty && header->GetUserVersion() >= 12)
		blockSize += 4;

	blockSize += 12;
	blockSize += extraDataRefs.GetSize() * 4;

	return blockSize;
}


void NiAVObject::Init(NiHeader* hdr) {
	NiObjectNET::Init(hdr);

	flags = 524302;
	rotation[0].x = 1.0f;
	rotation[1].y = 1.0f;
	rotation[2].z = 1.0f;
	scale = 1.0f;
}

void NiAVObject::Get(std::fstream& file) {
	NiObjectNET::Get(file);

	flags = 0;
	if (header->GetUserVersion2() <= 26)
		file.read((char*)&flags, 2);
	else
		file.read((char*)&flags, 4);

	file.read((char*)&translation.x, 4);
	file.read((char*)&translation.y, 4);
	file.read((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.read((char*)&rotation[i].x, 4);
		file.read((char*)&rotation[i].y, 4);
		file.read((char*)&rotation[i].z, 4);
	}

	file.read((char*)&scale, 4);

	if (header->GetUserVersion() <= 11)
		propertyRefs.Get(file);
	
	collisionRef.Get(file);
}

void NiAVObject::Put(std::fstream& file) {
	NiObjectNET::Put(file);

	if (header->GetUserVersion2() <= 26)
		file.write((char*)&flags, 2);
	else
		file.write((char*)&flags, 4);

	file.write((char*)&translation.x, 4);
	file.write((char*)&translation.y, 4);
	file.write((char*)&translation.z, 4);

	for (int i = 0; i < 3; i++) {
		file.write((char*)&rotation[i].x, 4);
		file.write((char*)&rotation[i].y, 4);
		file.write((char*)&rotation[i].z, 4);
	}

	file.write((char*)&scale, 4);

	if (header->GetUserVersion() <= 11)
		propertyRefs.Put(file);

	collisionRef.Put(file);
}

void NiAVObject::GetChildRefs(std::set<int*>& refs) {
	NiObjectNET::GetChildRefs(refs);

	propertyRefs.GetIndexPtrs(refs);
	refs.insert(&collisionRef.index);
}

int NiAVObject::CalcBlockSize() {
	NiObjectNET::CalcBlockSize();

	blockSize += 58;

	if (header->GetUserVersion() <= 11) {
		blockSize += 4;
		blockSize += propertyRefs.GetSize() * 4;
	}

	if (header->GetUserVersion2() > 26)
		blockSize += 2;

	return blockSize;
}


NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(NiHeader* hdr) {
	NiAVObjectPalette::Init(hdr);
}

NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(std::fstream& file, NiHeader* hdr) : NiDefaultAVObjectPalette(hdr) {
	Get(file);
}

void NiDefaultAVObjectPalette::Get(std::fstream& file) {
	NiAVObjectPalette::Get(file);

	sceneRef.Get(file);

	file.read((char*)&numObjects, 4);
	objects.resize(numObjects);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Get(file, 4);
		objects[i].objectRef.Get(file);
	}
}

void NiDefaultAVObjectPalette::Put(std::fstream& file) {
	NiAVObjectPalette::Put(file);

	sceneRef.Put(file);

	file.write((char*)&numObjects, 4);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Put(file, 4, false);
		objects[i].objectRef.Put(file);
	}
}

void NiDefaultAVObjectPalette::GetChildRefs(std::set<int*>& refs) {
	NiAVObjectPalette::GetChildRefs(refs);

	refs.insert(&sceneRef.index);

	for (int i = 0; i < numObjects; i++)
		refs.insert(&objects[i].objectRef.index);
}

int NiDefaultAVObjectPalette::CalcBlockSize() {
	NiAVObjectPalette::CalcBlockSize();

	blockSize += 8;
	blockSize += numObjects * 8;

	for (int i = 0; i < numObjects; i++)
		blockSize += objects[i].name.GetLength();

	return blockSize;
}


NiCamera::NiCamera(NiHeader* hdr) {
	NiAVObject::Init(hdr);
}

NiCamera::NiCamera(std::fstream& file, NiHeader* hdr) : NiCamera(hdr) {
	Get(file);
}

void NiCamera::Get(std::fstream& file) {
	NiAVObject::Get(file);

	file.read((char*)&obsoleteFlags, 2);
	file.read((char*)&frustumLeft, 4);
	file.read((char*)&frustumRight, 4);
	file.read((char*)&frustumTop, 4);
	file.read((char*)&frustomBottom, 4);
	file.read((char*)&frustumNear, 4);
	file.read((char*)&frustumFar, 4);
	file.read((char*)&useOrtho, 1);
	file.read((char*)&viewportLeft, 4);
	file.read((char*)&viewportRight, 4);
	file.read((char*)&viewportTop, 4);
	file.read((char*)&viewportBottom, 4);
	file.read((char*)&lodAdjust, 4);

	sceneRef.Get(file);
	file.read((char*)&numScreenPolygons, 4);
	file.read((char*)&numScreenTextures, 4);
}

void NiCamera::Put(std::fstream& file) {
	NiAVObject::Put(file);

	file.write((char*)&obsoleteFlags, 2);
	file.write((char*)&frustumLeft, 4);
	file.write((char*)&frustumRight, 4);
	file.write((char*)&frustumTop, 4);
	file.write((char*)&frustomBottom, 4);
	file.write((char*)&frustumNear, 4);
	file.write((char*)&frustumFar, 4);
	file.write((char*)&useOrtho, 1);
	file.write((char*)&viewportLeft, 4);
	file.write((char*)&viewportRight, 4);
	file.write((char*)&viewportTop, 4);
	file.write((char*)&viewportBottom, 4);
	file.write((char*)&lodAdjust, 4);

	sceneRef.Put(file);
	file.write((char*)&numScreenPolygons, 4);
	file.write((char*)&numScreenTextures, 4);
}

void NiCamera::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&sceneRef.index);
}

int NiCamera::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 59;

	return blockSize;
}


NiNode::NiNode(NiHeader* hdr) {
	NiAVObject::Init(hdr);
}

NiNode::NiNode(std::fstream& file, NiHeader* hdr) : NiNode(hdr) {
	Get(file);
}

void NiNode::Get(std::fstream& file) {
	NiAVObject::Get(file);

	childRefs.Get(file);

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130)
		effectRefs.Get(file);
}

void NiNode::Put(std::fstream& file) {
	NiAVObject::Put(file);

	childRefs.Put(file);

	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130)
		effectRefs.Put(file);
}

void NiNode::GetChildRefs(std::set<int*>& refs) {
	NiAVObject::GetChildRefs(refs);

	childRefs.GetIndexPtrs(refs);
	effectRefs.GetIndexPtrs(refs);
}

int NiNode::CalcBlockSize() {
	NiAVObject::CalcBlockSize();

	blockSize += 4;
	blockSize += childRefs.GetSize() * 4;
	if (header->GetUserVersion() <= 12 && header->GetUserVersion2() < 130) {
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


BSFadeNode::BSFadeNode(NiHeader* hdr) : NiNode(hdr) {
}

BSFadeNode::BSFadeNode(std::fstream& file, NiHeader* hdr) : BSFadeNode(hdr) {
	Get(file);
}


BSValueNode::BSValueNode(NiHeader* hdr) : NiNode(hdr) {
}

BSValueNode::BSValueNode(std::fstream& file, NiHeader* hdr) : BSValueNode(hdr) {
	Get(file);
}

void BSValueNode::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&value, 4);
	file.read((char*)&valueFlags, 1);
}

void BSValueNode::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&value, 4);
	file.write((char*)&valueFlags, 1);
}

int BSValueNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 5;

	return blockSize;
}


BSLeafAnimNode::BSLeafAnimNode(NiHeader* hdr) : NiNode(hdr) {
}

BSLeafAnimNode::BSLeafAnimNode(std::fstream& file, NiHeader* hdr) : BSLeafAnimNode(hdr) {
	Get(file);
}


BSTreeNode::BSTreeNode(NiHeader* hdr) : NiNode(hdr) {
}

BSTreeNode::BSTreeNode(std::fstream& file, NiHeader* hdr) : BSTreeNode(hdr) {
	Get(file);
}

void BSTreeNode::Get(std::fstream& file) {
	NiNode::Get(file);

	bones1.Get(file);
	bones2.Get(file);
}

void BSTreeNode::Put(std::fstream& file) {
	NiNode::Put(file);

	bones1.Put(file);
	bones2.Put(file);
}

void BSTreeNode::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	bones1.GetIndexPtrs(refs);
	bones2.GetIndexPtrs(refs);
}

int BSTreeNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 8;
	blockSize += bones1.GetSize() * 4;
	blockSize += bones2.GetSize() * 4;

	return blockSize;
}


BSOrderedNode::BSOrderedNode(NiHeader* hdr) : NiNode(hdr) {
}

BSOrderedNode::BSOrderedNode(std::fstream& file, NiHeader* hdr) : BSOrderedNode(hdr) {
	Get(file);
}

void BSOrderedNode::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&alphaSortBound, 16);
	file.read((char*)&isStaticBound, 1);
}

void BSOrderedNode::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&alphaSortBound, 16);
	file.write((char*)&isStaticBound, 1);
}

int BSOrderedNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 17;

	return blockSize;
}


BSMultiBoundOBB::BSMultiBoundOBB(NiHeader* hdr) {
	NiObject::Init(hdr);
}

BSMultiBoundOBB::BSMultiBoundOBB(std::fstream& file, NiHeader* hdr) : BSMultiBoundOBB(hdr) {
	Get(file);
}

void BSMultiBoundOBB::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&size, 12);
	file.read((char*)rotation, 36);
}

void BSMultiBoundOBB::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&size, 12);
	file.write((char*)rotation, 36);
}

int BSMultiBoundOBB::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 60;

	return blockSize;
}


BSMultiBoundAABB::BSMultiBoundAABB(NiHeader* hdr) {
	NiObject::Init(hdr);
}

BSMultiBoundAABB::BSMultiBoundAABB(std::fstream& file, NiHeader* hdr) : BSMultiBoundAABB(hdr) {
	Get(file);
}

void BSMultiBoundAABB::Get(std::fstream& file) {
	NiObject::Get(file);

	file.read((char*)&center, 12);
	file.read((char*)&halfExtent, 12);
}

void BSMultiBoundAABB::Put(std::fstream& file) {
	NiObject::Put(file);

	file.write((char*)&center, 12);
	file.write((char*)&halfExtent, 12);
}

int BSMultiBoundAABB::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 24;

	return blockSize;
}


BSMultiBound::BSMultiBound(NiHeader* hdr) {
	NiObject::Init(hdr);
}

BSMultiBound::BSMultiBound(std::fstream& file, NiHeader* hdr) : BSMultiBound(hdr) {
	Get(file);
}

void BSMultiBound::Get(std::fstream& file) {
	NiObject::Get(file);

	dataRef.Get(file);
}

void BSMultiBound::Put(std::fstream& file) {
	NiObject::Put(file);

	dataRef.Put(file);
}

void BSMultiBound::GetChildRefs(std::set<int*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&dataRef.index);
}

int BSMultiBound::CalcBlockSize() {
	NiObject::CalcBlockSize();

	blockSize += 4;

	return blockSize;
}


BSMultiBoundNode::BSMultiBoundNode(NiHeader* hdr) : NiNode(hdr) {
}

BSMultiBoundNode::BSMultiBoundNode(std::fstream& file, NiHeader* hdr) : BSMultiBoundNode(hdr) {
	Get(file);
}

void BSMultiBoundNode::Get(std::fstream& file) {
	NiNode::Get(file);

	multiBoundRef.Get(file);

	if (header->GetUserVersion() >= 12)
		file.read((char*)&cullingMode, 4);
}

void BSMultiBoundNode::Put(std::fstream& file) {
	NiNode::Put(file);

	multiBoundRef.Put(file);

	if (header->GetUserVersion() >= 12)
		file.write((char*)&cullingMode, 4);
}

void BSMultiBoundNode::GetChildRefs(std::set<int*>& refs) {
	NiNode::GetChildRefs(refs);

	refs.insert(&multiBoundRef.index);
}

int BSMultiBoundNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 4;
	if (header->GetUserVersion() >= 12)
		blockSize += 4;

	return blockSize;
}


BSBlastNode::BSBlastNode(NiHeader* hdr) : NiNode(hdr) {
}

BSBlastNode::BSBlastNode(std::fstream& file, NiHeader* hdr) : BSBlastNode(hdr) {
	Get(file);
}

void BSBlastNode::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&min, 1);
	file.read((char*)&max, 1);
	file.read((char*)&current, 1);
}

void BSBlastNode::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&min, 1);
	file.write((char*)&max, 1);
	file.write((char*)&current, 1);
}

int BSBlastNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 3;

	return blockSize;
}


BSDamageStage::BSDamageStage(NiHeader* hdr) : BSBlastNode(hdr) {
}

BSDamageStage::BSDamageStage(std::fstream& file, NiHeader* hdr) : BSDamageStage(hdr) {
	Get(file);
}


NiBillboardNode::NiBillboardNode(NiHeader* hdr) : NiNode(hdr) {
}

NiBillboardNode::NiBillboardNode(std::fstream& file, NiHeader* hdr) : NiBillboardNode(hdr) {
	Get(file);
}

void NiBillboardNode::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&billboardMode, 2);
}

void NiBillboardNode::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&billboardMode, 2);
}

int NiBillboardNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 2;

	return blockSize;
}


NiSwitchNode::NiSwitchNode(NiHeader* hdr) : NiNode(hdr) {
}

NiSwitchNode::NiSwitchNode(std::fstream& file, NiHeader* hdr) : NiSwitchNode(hdr) {
	Get(file);
}

void NiSwitchNode::Get(std::fstream& file) {
	NiNode::Get(file);

	file.read((char*)&flags, 2);
	file.read((char*)&index, 4);
}

void NiSwitchNode::Put(std::fstream& file) {
	NiNode::Put(file);

	file.write((char*)&flags, 2);
	file.write((char*)&index, 4);
}

int NiSwitchNode::CalcBlockSize() {
	NiNode::CalcBlockSize();

	blockSize += 6;

	return blockSize;
}
