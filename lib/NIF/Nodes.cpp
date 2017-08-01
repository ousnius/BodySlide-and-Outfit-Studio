/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "Nodes.h"

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


void NiBillboardNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> billboardMode;
}

void NiBillboardNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << billboardMode;
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


void NiSortAdjustNode::Get(NiStream& stream) {
	NiNode::Get(stream);

	stream >> sortingMode;
}

void NiSortAdjustNode::Put(NiStream& stream) {
	NiNode::Put(stream);

	stream << sortingMode;
}
