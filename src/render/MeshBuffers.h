/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "GLExtensions.h"

#include <array>

// The GL objects of a Mesh, see Mesh::gpu. Made and filled by Mesh::CreateBuffers and Mesh::UpdateBuffers.
struct MeshGpuBuffers {
	GLuint vao = 0;
	std::array<GLuint, 11> vbo{};
	GLuint ibo = 0;
};
