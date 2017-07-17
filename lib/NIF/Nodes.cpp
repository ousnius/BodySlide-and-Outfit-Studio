/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Nodes.h"

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


BSMultiBoundOBB::BSMultiBoundOBB(NiStream& stream) : BSMultiBoundOBB() {
	Get(stream);
}

void BSMultiBoundOBB::Get(NiStream& stream) {
	BSMultiBoundData::Get(stream);

	stream >> center;
	stream >> size;
	stream >> rotation;
}

void BSMultiBoundOBB::Put(NiStream& stream) {
	BSMultiBoundData::Put(stream);

	stream << center;
	stream << size;
	stream << rotation;
}


BSMultiBoundAABB::BSMultiBoundAABB(NiStream& stream) : BSMultiBoundAABB() {
	Get(stream);
}

void BSMultiBoundAABB::Get(NiStream& stream) {
	BSMultiBoundData::Get(stream);

	stream >> center;
	stream >> halfExtent;
}

void BSMultiBoundAABB::Put(NiStream& stream) {
	BSMultiBoundData::Put(stream);

	stream << center;
	stream << halfExtent;
}


BSMultiBoundSphere::BSMultiBoundSphere(NiStream& stream) : BSMultiBoundSphere() {
	Get(stream);
}

void BSMultiBoundSphere::Get(NiStream& stream) {
	BSMultiBoundData::Get(stream);

	stream >> center;
	stream >> radius;
}

void BSMultiBoundSphere::Put(NiStream& stream) {
	BSMultiBoundData::Put(stream);

	stream << center;
	stream << radius;
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


BSRangeNode::BSRangeNode() : NiNode() {
}

BSRangeNode::BSRangeNode(NiStream& stream) : BSRangeNode() {
	Get(stream);
}

void BSRangeNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> min;
	stream >> max;
	stream >> current;
}

void BSRangeNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << min;
	stream << max;
	stream << current;
}


BSDebrisNode::BSDebrisNode() : BSRangeNode() {
}

BSDebrisNode::BSDebrisNode(NiStream& stream) : BSDebrisNode() {
	Get(stream);
}


BSBlastNode::BSBlastNode() : BSRangeNode() {
}

BSBlastNode::BSBlastNode(NiStream& stream) : BSBlastNode() {
	Get(stream);
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


NiRangeLODData::NiRangeLODData() : NiLODData() {
}

NiRangeLODData::NiRangeLODData(NiStream& stream) : NiRangeLODData() {
	Get(stream);
}

void NiRangeLODData::Get(NiStream& stream) {
	NiLODData::Get(stream);

	stream >> lodCenter;
	stream >> numLODLevels;

	lodLevels.resize(numLODLevels);
	for (int i = 0; i < numLODLevels; i++) {
		stream >> lodLevels[i].nearExtent;
		stream >> lodLevels[i].farExtent;
	}
}

void NiRangeLODData::Put(NiStream& stream) {
	NiLODData::Put(stream);

	stream << lodCenter;
	stream << numLODLevels;

	for (int i = 0; i < numLODLevels; i++) {
		stream << lodLevels[i].nearExtent;
		stream << lodLevels[i].farExtent;
	}
}


NiScreenLODData::NiScreenLODData() : NiLODData() {
}

NiScreenLODData::NiScreenLODData(NiStream& stream) : NiScreenLODData() {
	Get(stream);
}

void NiScreenLODData::Get(NiStream& stream) {
	NiLODData::Get(stream);

	stream >> boundCenter;
	stream >> boundRadius;
	stream >> worldCenter;
	stream >> worldRadius;
	stream >> numProportions;

	proportionLevels.resize(numProportions);
	for (int i = 0; i < numProportions; i++)
		stream >> proportionLevels[i];
}

void NiScreenLODData::Put(NiStream& stream) {
	NiLODData::Put(stream);

	stream << boundCenter;
	stream << boundRadius;
	stream << worldCenter;
	stream << worldRadius;
	stream << numProportions;

	for (int i = 0; i < numProportions; i++)
		stream << proportionLevels[i];
}


NiLODNode::NiLODNode() : NiSwitchNode() {
}

NiLODNode::NiLODNode(NiStream& stream) : NiLODNode() {
	Get(stream);
}

void NiLODNode::Get(NiStream& stream) {
	NiSwitchNode::Get(stream);

	lodLevelData.Get(stream);
}

void NiLODNode::Put(NiStream& stream) {
	NiSwitchNode::Put(stream);

	lodLevelData.Put(stream);
}

void NiLODNode::GetChildRefs(std::set<int*>& refs) {
	NiSwitchNode::GetChildRefs(refs);

	refs.insert(&lodLevelData.index);
}


NiBone::NiBone() : NiNode() {
}

NiBone::NiBone(NiStream& stream) : NiBone() {
	Get(stream);
}


NiSortAdjustNode::NiSortAdjustNode() : NiNode() {
}

NiSortAdjustNode::NiSortAdjustNode(NiStream& stream) : NiSortAdjustNode() {
	Get(stream);
}

void NiSortAdjustNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> sortingMode;
}

void NiSortAdjustNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << sortingMode;
}
