/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <wx/treebase.h>

namespace nifly {
class NiShape;
}

// Data behind the items of the shape, segment and partition trees

class ShapeItemData : public wxTreeItemData {
	nifly::NiShape* shape = nullptr;
public:
	ShapeItemData(nifly::NiShape* inShape) { shape = inShape; }

	nifly::NiShape* GetShape() { return shape; }

	void SetShape(nifly::NiShape* newShape) { shape = newShape; }
};

struct ShapeItemState {
	std::string shapeName;
	int state = 0;
	bool selected = false;
};

class SegmentItemData : public wxTreeItemData {
public:
	// partID: a small nonnegative integer uniquely identifying this
	// segment among all the segments and subsegments.  Used as a value
	// in triSParts.  Not in the file.
	int partID;

	SegmentItemData(const int inPartitionID) { partID = inPartitionID; }
};

class SubSegmentItemData : public wxTreeItemData {
public:
	// partID: a small nonnegative integer uniquely identifying this
	// subsegment among all the segments and subsegments.  Used as a value
	// in triSParts.  Not in the file.
	int partID;
	uint32_t userSlotID;
	uint32_t material;
	std::vector<float> extraData;

	SubSegmentItemData(const int inPartitionID, const uint32_t& inUserSlotID, const uint32_t& inMaterial, const std::vector<float>& inExtraData = std::vector<float>()) {
		partID = inPartitionID;
		userSlotID = inUserSlotID;
		material = inMaterial;
		extraData = inExtraData;
	}
};

class PartitionItemData : public wxTreeItemData {
public:
	int index;
	uint16_t type;

	PartitionItemData(const int inIndex, const uint16_t& inType) {
		index = inIndex;
		type = inType;
	}
};
