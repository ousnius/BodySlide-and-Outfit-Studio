/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../utils/AABBTree.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

enum UndoType {
	UT_VERTPOS,
	UT_MASK,
	UT_WEIGHT,
	UT_COLOR,
	UT_ALPHA
};

struct UndoStateVertexWeight {
	float startVal, endVal;
};

struct UndoStateBoneWeights {
	std::string boneName;
	std::unordered_map<ushort, UndoStateVertexWeight> weights;
};

struct UndoStateShape {
	std::string shapeName;
	// pointStartState and pointEndState are only meaningful for not UT_WEIGHT.
	// For UT_MASK and UT_ALPHA, only the x coordinate is meaningful.
	std::unordered_map<int, Vector3> pointStartState;
	std::unordered_map<int, Vector3> pointEndState;
	// boneWeights is only meaningful for UT_WEIGHT.  Index 0 is the selected
	// bone and all others are normalize bones.
	std::vector<UndoStateBoneWeights> boneWeights;
	// startBVH, endBVH, and affectedNodes are only meaningful for UT_VERTPOS
	std::shared_ptr<AABBTree> startBVH;
	std::shared_ptr<AABBTree> endBVH;
	std::unordered_set<AABBTree::AABBTreeNode*> affectedNodes;
};

struct UndoStateProject {
	UndoType undoType;
	std::vector<UndoStateShape> usss;
	// if undoType is UT_VERTPOS and sliderName is not empty, this is
	// a slider shape edit rather than a base shape edit.
	std::string sliderName;
	// sliderscale is only set if this is a slider shape edit.  Non-zero.
	float sliderscale;
};
