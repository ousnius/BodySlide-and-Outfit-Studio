/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../files/MaterialFile.h"
#include "../render/GLExtensions.h"
#include "../utils/AABBTree.h"

#include "../gli/glm/gtc/matrix_transform.hpp"
#include "../gli/glm/gtx/euler_angles.hpp"

#include <array>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

class GLMaterial;

class Mesh {
private:
	std::array<bool, 10> queueUpdate = {false};

public:
	enum class RenderMode { Normal, UnlitSolid, UnlitWire, UnlitWireDepth, UnlitPoints, UnlitPointsDepth, LitWire };
	enum UpdateType { Position, Normals, Tangents, Bitangents, VertexColors, VertexAlpha, TextureCoordinates, Mask, Weight, Indices };

	struct ShaderProperties {
		nifly::Vector2 uvOffset;
		nifly::Vector2 uvScale = nifly::Vector2(1.0f, 1.0f);
		nifly::Vector3 specularColor = nifly::Vector3(1.0f, 1.0f, 1.0f);
		float specularStrength = 1.0f;
		float shininess = 30.0f;
		float envReflection = 1.0f;
		nifly::Vector3 emissiveColor = nifly::Vector3(1.0f, 1.0f, 1.0f);
		float emissiveMultiple = 1.0f;
		float alpha = 1.0f;
		float backlightPower = 0.0f;
		float rimlightPower = 2.0f;
		float softlighting = 0.3f;
		float subsurfaceRolloff = 0.3f;
		float fresnelPower = 5.0f;
		float paletteScale = 0.0f;
	};

	// Use SetXformMeshToModel or SetXformModelToMesh to set matModel,
	// xformMeshToModel, and xformModelToMesh.
	glm::mat4x4 matModel = glm::identity<glm::mat4x4>();
	nifly::MatTransform xformMeshToModel, xformModelToMesh;

	int nVerts = 0;
	std::unique_ptr<nifly::Vector3[]> verts;
	std::unique_ptr<nifly::Vector3[]> norms;
	std::unique_ptr<nifly::Vector3[]> tangents;
	std::unique_ptr<nifly::Vector3[]> bitangents;
	std::unique_ptr<nifly::Vector3[]> vcolors;
	std::unique_ptr<float[]> valpha;
	std::unique_ptr<nifly::Vector2[]> texcoord;
	std::unique_ptr<float[]> mask;
	std::unique_ptr<float[]> weight;

	std::unique_ptr<nifly::Triangle[]> tris;
	// renderTris is tris re-ordered for rendering with submeshes.  It's
	// created automatically in CreateBuffers as a copy of tris.  If
	// something changes tris, renderTris needs to be updated too.
	std::unique_ptr<nifly::Triangle[]> renderTris;
	int nTris = 0;

	std::unique_ptr<nifly::Edge[]> edges;
	int nEdges = 0;

	bool genBuffers = false;
	GLuint vao = 0;
	std::vector<GLuint> vbo = std::vector<GLuint>(9, 0);
	GLuint ibo = 0;

	std::vector<std::pair<uint32_t, uint32_t>> subMeshes; // Start index and size of each sub mesh
	std::vector<nifly::Vector3> subMeshesColor;			  // Color of each sub mesh

	ShaderProperties prop;
	GLMaterial* material = nullptr;

	uint32_t overlayLayer = 0;					 // Layer for order of rendering overlays
	float smoothThresh = 60.0f * nifly::DEG2RAD; // Smoothing threshold for generating smooth normals.

	std::unique_ptr<std::vector<int>[]> vertTris;		 // Map of triangles for which each vert is a member.
	std::unique_ptr<std::vector<int>[]> vertEdges;		 // Map of edges for which each vert is a member.
	std::unordered_map<int, std::vector<int>> weldVerts; // Verts that are duplicated for UVs but are in the same position.
	bool bGotWeldVerts = false;							 // Whether weldVerts has been calculated yet.
	std::unique_ptr<std::vector<int>[]> adjVerts;		 // Vertices that are adjacent to each vertex.

	std::unordered_set<uint32_t> lockedNormalIndices;

	RenderMode rendermode = RenderMode::Normal;
	bool doublesided = false;
	GLenum cullMode = GL_BACK;
	bool modelSpace = false;
	bool emissive = false;
	bool specular = true;
	bool vertexColors = false;
	bool vertexAlpha = false;
	bool backlight = false;
	bool backlightMap = false;
	bool rimlight = false;
	bool softlight = false;
	bool glowmap = false;
	bool greyscaleColor = false;
	bool cubemap = false;
	bool textured = false;

	std::shared_ptr<AABBTree> bvh = nullptr;

	bool bVisible = true;
	bool bShowPoints = false;
	bool smoothSeamNormals = true;
	bool lockNormals = false;

	uint16_t alphaFlags = 0;
	uint8_t alphaThreshold = 0;

	std::string shapeName;
	nifly::Vector3 color;

	Mesh();
	~Mesh();

	// Creates a new bvh tree for the mesh.
	std::shared_ptr<AABBTree> CreateBVH();

	void MakeEdges(); // Creates the list of edges from the list of triangles.

	void BuildTriAdjacency();	 // Triangle adjacency optional to reduce overhead when it's not needed.
	void BuildVertexAdjacency(); // Vertex adjacency optional to reduce overhead when it's not needed.
	void BuildEdgeList();		 // Edge list optional to reduce overhead when it's not needed.

	void CalcWeldVerts();

	void CreateBuffers();
	void UpdateBuffers();
	void QueueUpdate(const UpdateType& type);
	void UpdateFromMaterialFile(const MaterialFile& matFile);
	bool HasAlphaBlend();

	void ScaleVertices(const nifly::Vector3& center, const float& factor);

	void SetSmoothThreshold(float degrees);
	float GetSmoothThreshold();

	void FacetNormals();
	void SmoothNormals(const std::unordered_set<int>& vertices = std::unordered_set<int>());
	static void SmoothNormalsStatic(Mesh* m) { m->SmoothNormals(); }
	static void SmoothNormalsStaticArray(Mesh* m, int* vertices, int nVertices) {
		std::unordered_set<int> verts;
		verts.reserve(nVertices * 2);
		verts.insert(vertices, vertices + nVertices);

		for (int i = 0; i < nVertices; i++)
			m->GetAdjacentPoints(vertices[i], verts);

		m->SmoothNormals(verts);
	}
	static void SmoothNormalsStaticMap(Mesh* m, const std::unordered_map<int, nifly::Vector3>& vertices) {
		std::unordered_set<int> verts;
		verts.reserve(vertices.size());

		for (auto& v : vertices) {
			verts.insert(v.first);
			m->GetAdjacentPoints(v.first, verts);
		}

		m->SmoothNormals(verts);
	}

	nifly::Vector3 GetOneVertexNormal(int vertind);

	void CalcTangentSpace();

	// List connected points within the squared radius of the center.
	// Requires vertEdges and edges.  pointvisit.size() must be nVerts,
	// and it must be initialized to false.  nOutPoints must be initialized
	// to zero.
	void ConnectedPointsInSphere(const nifly::Vector3& center, float sqradius, int startTri, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints);

	// List connected points within the squared radius of either center.
	// Requires vertEdges and edges.  pointvisit.size() must be nVerts,
	// and it must be initialized to false.  nOutPoints must be initialized
	// to zero.
	void ConnectedPointsInTwoSpheres(const nifly::Vector3& center1, const nifly::Vector3& center2, float sqradius, int startTri1, int startTri2, std::vector<bool>& pointvisit, int outPoints[], int& nOutPoints);

	// Convenience function to gather connected points, taking into account "welded" vertices. Does not clear the output set.
	void GetAdjacentPoints(int querypoint, std::unordered_set<int>& outPoints);

	// More optimized adjacency fetch, using edge adjacency and storing the output in a static array.
	// Requires that BuildEdgeList() be called prior to use.
	int GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints);
	// As above, but also checks if each point has already been looked at (according to vispoint).
	int GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint);

	// Creates the vertex color array (if necessary) and sets all the colors to the provided value.
	void ColorFill(const nifly::Vector3& color);
	void AlphaFill(float alpha);
	void MaskFill(float maskValue);
	void WeightFill(float weightValue);

	void ColorChannelFill(int channel, float value);

	static constexpr nifly::MatTransform xformNifToMesh{nifly::Vector3(),nifly::Matrix3(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f),0.1f};
	static constexpr nifly::MatTransform xformMeshToNif{nifly::Vector3(),nifly::Matrix3(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f),10.0f};

	static constexpr nifly::Vector3 TransformPosNifToMesh(const nifly::Vector3& vec) {
		// This function efficiently calculates xformNifToMesh.ApplyTransform(vec)
		return nifly::Vector3(
			vec.x / -10.0f,
			vec.z / 10.0f,
			vec.y / 10.0f);
	}

	static constexpr nifly::Vector3 TransformPosMeshToNif(const nifly::Vector3& vec) {
		// This function efficiently calculates xformMeshToNif.ApplyTransform(vec)
		return nifly::Vector3(
			vec.x * -10.0f,
			vec.z * 10.0f,
			vec.y * 10.0f);
	}

	static constexpr nifly::Vector3 TransformDiffNifToMesh(const nifly::Vector3& diff) {
		// This function efficiently calculates xformNifToMesh::ApplyTransformToDiff(diff)
		return nifly::Vector3(
			diff.x / -10.0f,
			diff.z / 10.0f,
			diff.y / 10.0f);
	}

	static constexpr nifly::Vector3 TransformDiffMeshToNif(const nifly::Vector3& diff) {
		// This function efficiently calculates xformMeshToNif::ApplyTransformToDiff(diff)
		return nifly::Vector3(
			diff.x * -10.0f,
			diff.z * 10.0f,
			diff.y * 10.0f);
	}

	static constexpr nifly::Vector3 TransformDirNifToMesh(const nifly::Vector3& dir) {
		// This function efficiently calculates xformNifToMesh::ApplyTransformToDir(dir)
		return nifly::Vector3(
			-dir.x,
			dir.z,
			dir.y);
	}

	static constexpr nifly::Vector3 TransformDirMeshToNif(const nifly::Vector3& dir) {
		// This function efficiently calculates xformMeshToNif::ApplyTransformToDir(dir)
		return nifly::Vector3(
			-dir.x,
			dir.z,
			dir.y);
	}

	static constexpr float TransformDistNifToMesh(float d) {
		// This function calculates xformMeshToNif::ApplyTransformToDist(d)
		return d / 10.0f;
	}

	static constexpr float TransformDistMeshToNif(float d) {
		// This function calculates xformMeshToNif::ApplyTransformToDist(d)
		return d * 10.0f;
	}

	void SetXformMeshToModel(const nifly::MatTransform& tMeshToModel) {
		xformMeshToModel = tMeshToModel;
		xformModelToMesh = tMeshToModel.InverseTransform();
		matModel = xformMeshToModel.ToGLMMatrix<glm::mat4x4>();
	}

	void SetXformModelToMesh(const nifly::MatTransform& tModelToMesh) {
		xformModelToMesh = tModelToMesh;
		xformMeshToModel = tModelToMesh.InverseTransform();
		matModel = xformMeshToModel.ToGLMMatrix<glm::mat4x4>();
	}

	nifly::Vector3 TransformPosMeshToModel(const nifly::Vector3 &pos) {
		return xformMeshToModel.ApplyTransform(pos);
	}

	nifly::Vector3 TransformPosModelToMesh(const nifly::Vector3 &pos) {
		return xformModelToMesh.ApplyTransform(pos);
	}

	nifly::Vector3 TransformDirMeshToModel(const nifly::Vector3 &dir) {
		return xformMeshToModel.ApplyTransformToDir(dir);
	}

	nifly::Vector3 TransformDirModelToMesh(const nifly::Vector3 &dir) {
		return xformModelToMesh.ApplyTransformToDir(dir);
	}

	nifly::Vector3 TransformDiffMeshToModel(const nifly::Vector3 &diff) {
		return xformMeshToModel.ApplyTransformToDiff(diff);
	}

	nifly::Vector3 TransformDiffModelToMesh(const nifly::Vector3 &diff) {
		return xformModelToMesh.ApplyTransformToDiff(diff);
	}

	float TransformDistMeshToModel(float dist) {
		return xformMeshToModel.ApplyTransformToDist(dist);
	}

	float TransformDistModelToMesh(float dist) {
		return xformModelToMesh.ApplyTransformToDist(dist);
	}

	static nifly::Vector3 ApplyMatrix4(const glm::mat4x4& mat, const nifly::Vector3& p) {
		glm::vec3 gp(mat * glm::vec4(p.x, p.y, p.z, 1.0f));
		return nifly::Vector3(gp.x, gp.y, gp.z);
	}
};
