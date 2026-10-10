/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

// Starfield meshes are normalized to metric units; vertices are scaled by this
// factor during BSGeometryMeshData deserialization to match older-game units.
constexpr float sfHavokScale = 69.969f;

// Shape-to-global transform that places a shape in the shared space used for conforming.
// Skinned Starfield shapes take their skin transform from the skeleton in meters while their
// vertices are havokScale scaled. The editor meshes scale the translation the same way
// (wxGLPanel::AddMeshFromNif), so the outfit and the reference only line up for the proximity
// search when the conform meshes do too. Rotation and scale are left untouched.
inline nifly::MatTransform ShapeToGlobalForConform(nifly::MatTransform shapeToGlobal, const bool isSkinned, const bool isStarfield) {
	if (isSkinned && isStarfield)
		shapeToGlobal.translation *= sfHavokScale;

	return shapeToGlobal;
}
