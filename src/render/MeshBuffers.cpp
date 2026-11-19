/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The GL side of Mesh, kept apart so the mesh data can be used without GL, e.g. in tests

#include "MeshBuffers.h"
#include "components/Mesh.h"

using namespace nifly;

static void DeleteGpuBuffers(MeshGpuBuffers* b) {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteBuffers(static_cast<GLsizei>(b->vbo.size()), b->vbo.data());
	glDeleteBuffers(1, &b->ibo);
	glDeleteVertexArrays(1, &b->vao);
	delete b;
}

void Mesh::CreateBuffers() {
	if (!gpu) {
		// The deleter is set here so Mesh.cpp itself never calls GL, and meshes that never rendered need no GL to go away
		gpu = {new MeshGpuBuffers(), &DeleteGpuBuffers};
		glGenVertexArrays(1, &gpu->vao);
		glGenBuffers(static_cast<GLsizei>(gpu->vbo.size()), gpu->vbo.data());
		glGenBuffers(1, &gpu->ibo);
	}

	// NumVertices * (Position + Normal + Colors + Texture Coordinates)
	glBindVertexArray(gpu->vao);

	if (verts) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), verts.get(), GL_DYNAMIC_DRAW);
	}

	if (norms) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), norms.get(), GL_DYNAMIC_DRAW);
	}

	if (tangents) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), tangents.get(), GL_DYNAMIC_DRAW);
	}

	if (bitangents) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), bitangents.get(), GL_DYNAMIC_DRAW);
	}

	if (vcolors) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[4]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), vcolors.get(), GL_DYNAMIC_DRAW);
	}

	if (valpha) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[5]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), valpha.get(), GL_DYNAMIC_DRAW);
	}

	if (texcoord) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[6]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord.get(), GL_DYNAMIC_DRAW);
	}

	if (mask) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[7]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), mask.get(), GL_DYNAMIC_DRAW);
	}

	if (weight) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[8]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), weight.get(), GL_DYNAMIC_DRAW);
	}

	if (texcoord2) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[9]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord2.get(), GL_STATIC_DRAW);
	}

	if (poseRotations) {
		glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[10]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(glm::vec4), poseRotations.get(), GL_DYNAMIC_DRAW);
		queueUpdate[UpdateType::PoseRotations] = false;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element index array
	if (verts && tris) {
		renderTris = std::make_unique<Triangle[]>(nTris);
		for (int i = 0; i < nTris; ++i)
			renderTris[i] = tris[i];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTris * sizeof(Triangle), renderTris.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else if (verts && edges) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nEdges * sizeof(Edge), edges.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
}

void Mesh::UpdateBuffers() {
	if (gpu) {
		glBindVertexArray(gpu->vao);

		if (verts && queueUpdate[UpdateType::Position]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Position]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), verts.get());
			queueUpdate[UpdateType::Position] = false;
		}

		if (norms && queueUpdate[UpdateType::Normals]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Normals]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), norms.get());
			queueUpdate[UpdateType::Normals] = false;
		}

		if (tangents && queueUpdate[UpdateType::Tangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Tangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), tangents.get());
			queueUpdate[UpdateType::Tangents] = false;
		}

		if (bitangents && queueUpdate[UpdateType::Bitangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Bitangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), bitangents.get());
			queueUpdate[UpdateType::Bitangents] = false;
		}

		if (vcolors && queueUpdate[UpdateType::VertexColors]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::VertexColors]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), vcolors.get());
			queueUpdate[UpdateType::VertexColors] = false;
		}

		if (valpha && queueUpdate[UpdateType::VertexAlpha]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::VertexAlpha]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), valpha.get());
			queueUpdate[UpdateType::VertexAlpha] = false;
		}

		if (texcoord && queueUpdate[UpdateType::TextureCoordinates]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::TextureCoordinates]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector2), texcoord.get());
			queueUpdate[UpdateType::TextureCoordinates] = false;
		}

		if (mask && queueUpdate[UpdateType::Mask]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Mask]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), mask.get());
			queueUpdate[UpdateType::Mask] = false;
		}

		if (weight && queueUpdate[UpdateType::Weight]) {
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::Weight]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), weight.get());
			queueUpdate[UpdateType::Weight] = false;
		}

		if (poseRotations && queueUpdate[UpdateType::PoseRotations]) {
			// Not sub data, the buffer only gets its storage once a pose first turns the mesh
			glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo[UpdateType::PoseRotations]);
			glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(glm::vec4), poseRotations.get(), GL_DYNAMIC_DRAW);
			queueUpdate[UpdateType::PoseRotations] = false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (queueUpdate[UpdateType::Indices]) {
			if (tris) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nTris * sizeof(Triangle), renderTris.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else if (edges) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nEdges * sizeof(Edge), edges.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			queueUpdate[UpdateType::Indices] = false;
		}

		glBindVertexArray(0);
	}
}
