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

enum class UndoType { VertexPosition, Mask, Weight, Color, Alpha, Mesh };

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
	uint16_t index = 0; // index into array of vertices
	nifly::Vector3 pos; // position in skin coordinates
	nifly::Vector2 uv;
	nifly::Color4 color;
	// normal, tangent, and bitangent are directions in skin coordinates
	// (so use ApplyTransformToDir instead of ApplyTransform).
	nifly::Vector3 normal, tangent, bitangent;
	float eyeData = 0.0f;
	float mask = 0.0f;
	std::vector<UndoStateVertexBoneWeight> weights;
	std::vector<UndoStateVertexSliderDiff> diffs;
};

struct UndoStateTriangle {
	uint32_t index = 0; // index into array of triangles
	nifly::Triangle t;
	int partID = -1; // partition ID if there are partitions or segments
};

inline bool operator<(const UndoStateTriangle& t1, const UndoStateTriangle& t2) {
	return t1.index < t2.index;
}

struct UndoStateShape {
	std::string shapeName;
	// pointStartState and pointEndState are only meaningful for
	// UndoType::VertexPosition, UndoType::Mask, UndoType::Color,
	// and UndoType::Alpha.
	// For UndoType::Mask and UndoType::Alpha, only the x coordinate
	// is meaningful.
	std::unordered_map<int, nifly::Vector3> pointStartState;
	std::unordered_map<int, nifly::Vector3> pointEndState;
	// boneWeights is only meaningful for UndoType::Weight.
	std::vector<UndoStateBoneWeights> boneWeights;
	// delVerts, addVerts, delTris, and addTris are only meaningful for
	// UndoType::Mesh.  They are stored in sorted order by index.
	std::vector<UndoStateVertex> delVerts, addVerts;
	std::vector<UndoStateTriangle> delTris, addTris;
};

struct UndoStateProject {
	UndoType undoType = UndoType::VertexPosition;
	std::vector<UndoStateShape> usss;
	// if undoType is UndoType::VertexPosition and sliderName is not empty,
	// this is a slider shape edit rather than a base shape edit.
	std::string sliderName;
	// sliderscale is only set if this is a slider shape edit.  Non-zero.
	float sliderscale = 1.0f;
};
