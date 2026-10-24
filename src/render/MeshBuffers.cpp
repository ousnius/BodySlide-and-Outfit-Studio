/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The GL side of Mesh, kept apart so the mesh data can be used without GL, e.g. in tests

#include "../components/Mesh.h"

using namespace nifly;

void Mesh::CreateBuffers() {
	if (!genBuffers) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(static_cast<GLsizei>(vbo.size()), vbo.data());
		glGenBuffers(1, &ibo);

		// Set here so Mesh.cpp itself never calls GL, and meshes that never rendered need no GL to go away
		releaseBuffers = [](Mesh& m) {
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			glDeleteBuffers(static_cast<GLsizei>(m.vbo.size()), m.vbo.data());
			glDeleteBuffers(1, &m.ibo);
			glDeleteVertexArrays(1, &m.vao);
		};
	}

	// NumVertices * (Position + Normal + Colors + Texture Coordinates)
	glBindVertexArray(vao);

	if (verts) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), verts.get(), GL_DYNAMIC_DRAW);
	}

	if (norms) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), norms.get(), GL_DYNAMIC_DRAW);
	}

	if (tangents) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), tangents.get(), GL_DYNAMIC_DRAW);
	}

	if (bitangents) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), bitangents.get(), GL_DYNAMIC_DRAW);
	}

	if (vcolors) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector3), vcolors.get(), GL_DYNAMIC_DRAW);
	}

	if (valpha) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), valpha.get(), GL_DYNAMIC_DRAW);
	}

	if (texcoord) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord.get(), GL_DYNAMIC_DRAW);
	}

	if (mask) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), mask.get(), GL_DYNAMIC_DRAW);
	}

	if (weight) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(float), weight.get(), GL_DYNAMIC_DRAW);
	}

	if (texcoord2) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(Vector2), texcoord2.get(), GL_STATIC_DRAW);
	}

	if (poseRotations) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[10]);
		glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(glm::vec4), poseRotations.get(), GL_DYNAMIC_DRAW);
		queueUpdate[UpdateType::PoseRotations] = false;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element index array
	if (verts && tris) {
		renderTris = std::make_unique<Triangle[]>(nTris);
		for (int i = 0; i < nTris; ++i)
			renderTris[i] = tris[i];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTris * sizeof(Triangle), renderTris.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else if (verts && edges) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, nEdges * sizeof(Edge), edges.get(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	glBindVertexArray(0);
	genBuffers = true;
}

void Mesh::UpdateBuffers() {
	if (genBuffers) {
		glBindVertexArray(vao);

		if (verts && queueUpdate[UpdateType::Position]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Position]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), verts.get());
			queueUpdate[UpdateType::Position] = false;
		}

		if (norms && queueUpdate[UpdateType::Normals]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Normals]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), norms.get());
			queueUpdate[UpdateType::Normals] = false;
		}

		if (tangents && queueUpdate[UpdateType::Tangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Tangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), tangents.get());
			queueUpdate[UpdateType::Tangents] = false;
		}

		if (bitangents && queueUpdate[UpdateType::Bitangents]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Bitangents]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), bitangents.get());
			queueUpdate[UpdateType::Bitangents] = false;
		}

		if (vcolors && queueUpdate[UpdateType::VertexColors]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::VertexColors]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector3), vcolors.get());
			queueUpdate[UpdateType::VertexColors] = false;
		}

		if (valpha && queueUpdate[UpdateType::VertexAlpha]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::VertexAlpha]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), valpha.get());
			queueUpdate[UpdateType::VertexAlpha] = false;
		}

		if (texcoord && queueUpdate[UpdateType::TextureCoordinates]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::TextureCoordinates]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(Vector2), texcoord.get());
			queueUpdate[UpdateType::TextureCoordinates] = false;
		}

		if (mask && queueUpdate[UpdateType::Mask]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Mask]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), mask.get());
			queueUpdate[UpdateType::Mask] = false;
		}

		if (weight && queueUpdate[UpdateType::Weight]) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::Weight]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), weight.get());
			queueUpdate[UpdateType::Weight] = false;
		}

		if (poseRotations && queueUpdate[UpdateType::PoseRotations]) {
			// Not sub data, the buffer only gets its storage once a pose first turns the mesh
			glBindBuffer(GL_ARRAY_BUFFER, vbo[UpdateType::PoseRotations]);
			glBufferData(GL_ARRAY_BUFFER, nVerts * sizeof(glm::vec4), poseRotations.get(), GL_DYNAMIC_DRAW);
			queueUpdate[UpdateType::PoseRotations] = false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (queueUpdate[UpdateType::Indices]) {
			if (tris) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nTris * sizeof(Triangle), renderTris.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			else if (edges) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nEdges * sizeof(Edge), edges.get());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			queueUpdate[UpdateType::Indices] = false;
		}

		glBindVertexArray(0);
	}
}
