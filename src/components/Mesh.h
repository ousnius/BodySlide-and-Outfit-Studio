/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../utils/KDMatcher.h"
#include "../utils/AABBTree.h"
#include "../render/GLExtensions.h"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>

enum RenderMode {
	Normal,
	UnlitSolid,
	UnlitWire,
	UnlitPoints,
	LitWire
};

class GLMaterial;

class mesh {
private:
	vector<bool> queueUpdate;

public:
	enum UpdateType {
		Position,
		Normals,
		VertexColors,
		TextureCoordinates,
		Indices
	};

	int nVerts;
	Vector3* verts;
	Vector3* norms;
	Vector3* vcolors;
	Vector2* texcoord;

	Triangle* tris;
	int nTris;

	Edge* edges;
	int nEdges;

	bool genBuffers = false;
	GLuint vao = 0;
	vector<GLuint> vbo;
	GLuint ibo = 0;

	GLMaterial* material = nullptr;

	float scale;				// Information only, does not cause verts to be scaled during render (except point/lines).
	float smoothThresh;			// Smoothing threshold for generating smooth normals.

	vector<int>* vertTris;		// Map of triangles for which each vert is a member.
	vector<int>* vertEdges;		// Map of edges for which each vert is a member.
	unordered_map <int, vector<int>> weldVerts; // Verts that are duplicated for UVs but are in the same position.

	RenderMode rendermode;		// 0 = normal mesh render, 1 = line based visualizer render, 2 = point based vis render.
	bool doublesided;
	bool textured;

	shared_ptr<AABBTree> bvh;

	bool bVisible;
	bool bShowPoints;
	bool smoothSeamNormals;

	string shapeName;
	Vector3 color;

	mesh();

	// Creates a new bvh tree for the mesh.
	shared_ptr<AABBTree> CreateBVH();

	void MakeEdges();			// Creates the list of edges from the list of triangles.

	void BuildTriAdjacency();	// Triangle adjacency optional to reduce overhead when it's not needed.
	void BuildEdgeList();		// Edge list optional to reduce overhead when it's not needed.

	void CreateBuffers();
	void UpdateBuffers();
	void QueueUpdate(const UpdateType& type);

	void ScaleVertices(const Vector3& center, const float& factor);

	void SetSmoothThreshold(float degrees);
	float GetSmoothThreshold();

	void FacetNormals();
	void SmoothNormals(const set<int>& vertices = set<int>());
	static void SmoothNormalsStatic(mesh* m) {
		m->SmoothNormals();
	}
	static void SmoothNormalsStaticArray(mesh* m, int* vertices, int nVertices) {
		set<int> verts;
		for (int i = 0; i < nVertices; i++)
			verts.insert(vertices[i]);

		m->SmoothNormals(verts);
	}
	static void SmoothNormalsStaticMap(mesh* m, const unordered_map<int, Vector3>& vertices) {
		set<int> verts;
		for (auto &v : vertices)
			verts.insert(v.first);

		m->SmoothNormals(verts);
	}


	// Retrieve connected points in a sphere's radius (squared, requires tri adjacency to be set up).
	// Also requires trivisit to be allocated by the caller - one slot for every tri in the mesh.
	// Recursive - large query will overflow the stack!
	bool ConnectedPointsInSphere(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets);

	// Similar to above, but uses an edge list to determine adjacency, with less risk of stack problems.
	bool ConnectedPointsInSphere2(Vector3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets);

	// Convenience function to gather connected points, taking into account "welded" vertices.
	// Optionally sorts the results by distance. Does not clear the output set.
	void GetAdjacentPoints(int querypoint, set<int>& outPoints, bool sort = false);

	// More optimized adjacency fetch, using edge adjacency and storing the output in a static array.
	// Requires that BuildEdgeList() be called prior to use.
	int GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints);
	// As above, but also checks if each point has already been looked at (according to vispoint).
	int GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint);

	// Creates the vertex color array (if necessary) and sets all the colors to the provided value.
	void ColorFill(const Vector3& color);

	void ColorChannelFill(int channel, float value);

	~mesh();
};
