/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <set>

struct FBXImportOptions {
	bool InvertU = false;
	bool InvertV = false;
	float Scale = 1.0f;
	float RotateX = 0.0f;
	float RotateY = 0.0f;
	float RotateZ = 0.0f;

	bool ImportAll = true;
	std::set<std::string> ImportShapes;
};
