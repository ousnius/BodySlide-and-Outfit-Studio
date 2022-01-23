/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../utils/AABBTree.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum UndoType { UT_VERTPOS, UT_MASK, UT_WEIGHT, UT_COLOR, UT_ALPHA, UT_MESH };

struct UndoStateVertexWeight {
	float startVal, endVal;
};

struct UndoStateBoneWeights {
	std::string boneName;
	std::unordered_map<uint16_t, UndoStateVertexWeight> weights;
};

struct UndoStateVertexBoneWeight {
	std::string boneName;
	float w;
};

struct UndoStateVertexSliderDiff {
	std::string sliderName; // NOT the set name
	nifly::Vector3 diff;
};

struct UndoStateVertex {
	uint16_t index;		// index into array of vertices
	nifly::Vector3 pos; // position in skin coordinates
	nifly::Vector2 uv;
	nifly::Color4 color;
	// normal, tangent, and bitangent are in skin CS tangent space (so
	// only the rotation part of transforms affects them).
	nifly::Vector3 normal, tangent, bitangent;
	float eyeData;
	std::vector<UndoStateVertexBoneWeight> weights;
	std::vector<UndoStateVertexSliderDiff> diffs;
};

struct UndoStateTriangle {
	uint32_t index; // index into array of triangles
	nifly::Triangle t;
	int partID = -1; // partition ID if there are partitions or segments
};

inline bool operator<(const UndoStateTriangle& t1, const UndoStateTriangle& t2) {
	return t1.index < t2.index;
}

struct UndoStateShape {
	std::string shapeName;
	// pointStartState and pointEndState are only meaningful for
	// UT_VERTPOS, UT_MASK, UT_COLOR, and UT_ALPHA.
	// For UT_MASK and UT_ALPHA, only the x coordinate is meaningful.
	std::unordered_map<int, nifly::Vector3> pointStartState;
	std::unordered_map<int, nifly::Vector3> pointEndState;
	// boneWeights is only meaningful for UT_WEIGHT.
	std::vector<UndoStateBoneWeights> boneWeights;
	// delVerts, addVerts, delTris, and addTris are only meaningful for
	// UT_MESH.  They are stored in sorted order by index.
	std::vector<UndoStateVertex> delVerts, addVerts;
	std::vector<UndoStateTriangle> delTris, addTris;
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
