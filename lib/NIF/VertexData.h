/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"

struct BSVertexData {
	// Single- or half-precision depending on IsFullPrecision() being true
	Vector3 vert;
	float bitangentX;	// Maybe the dot product of the vert normal and the z-axis?

	Vector2 uv;

	byte normal[3];
	byte bitangentY;
	byte tangent[3];
	byte bitangentZ;

	byte colorData[4];

	float weights[4];
	byte weightBones[4];

	float eyeData;
};
