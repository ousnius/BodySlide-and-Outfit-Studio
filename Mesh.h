#pragma once
#include "Object3d.h"
#include "KDMatcher.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
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
public:
	vtx* verts;
	int nVerts;

	tri* tris;
	int nTris;

	edge* edges;
	int nEdges;

	float scale;				// Information only, does not cause verts to be scaled during render (except point/lines).
	float smoothThresh;			// Smoothing threshold for generating smooth normals.

	vector<int>* vertTris;		// Map of triangles for which each vert is a member.
	vector<int>* vertEdges;		// Map of edges for which each vert is a member.
	unordered_map <int, vector<int>> weldVerts; // Verts that are duplicated for UVs but are in the same position.

	RenderMode rendermode;		// 0 = normal mesh render, 1 = line based visualizer render, 2 = point based vis render.
	bool doublesided;
	bool textured;
	vec2* texcoord;
	GLMaterial* material{nullptr};

	vec3* vcolors;				// Vertex colors.

	shared_ptr<AABBTree> bvh;
	kd_tree* kdtree;

	GLuint VboBufName;
	GLuint IboBufName;

	bool bBuffersLoaded;
	bool bVisible;
	bool smoothSeamNormals;

	string shapeName;

	vec3 color;

	Mat4 Transform;				// Transformation matrix for OpenGL display.

	mesh();
	mesh(mesh* m);				// Copy from existing mesh.

	// Creates a new bvh tree for the mesh. The old tree is deleted if doDelete is true, otherwise, the old tree stays
	// allocated (it is assumed an external function will later delete the old tree as needed).
	shared_ptr<AABBTree> CreateBVH(bool doDelete = true);
	// Swaps in a new BVH tree and returns the old one.
	shared_ptr<AABBTree> SetBVH(shared_ptr<AABBTree> myNewTree);


	void CreateKDTree();
	void TransformPoints(const Mat4& mat);
	void TransformPoints(vector<int>& indices, const Mat4& mat);
	void TransformPoints(set<int>& indices, const Mat4& mat);

	void MakeEdges();			// Creates the list of edges from the list of triangles.

	void BuildTriAdjacency();	// Triangle adjacency optional to reduce overhead when it's not needed.
	void BuildEdgeList();		// Edge list optional to reduce overhead when it's not needed.

	void SetSmoothThreshold(float degrees);
	float GetSmoothThreshold();
	void SmoothNormals();
	void FacetNormals();


	// Retrieve connected points in a sphere's radius (squared, requires tri adjacency to be set up).
	// Also requires trivisit to be allocated by the caller - one slot for every tri in the mesh.
	// Recursive - large query will overflow the stack!
	bool ConnectedPointsInSphere(vec3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets);

	// Similar to above, but uses an edge list to determine adjacency, with less risk of stack problems.
	bool ConnectedPointsInSphere2(vec3 center, float sqradius, int startTri, bool* trivisit, bool* pointvisit, int outPoints[], int& nOutPoints, vector<int>& outFacets);

	// Convenience function to gather connected points, taking into account "welded" vertices.
	// Optionally sorts the results by distance. Does not clear the output set.
	void GetAdjacentPoints(int querypoint, set<int>& outPoints, bool sort = false);

	// More optimized adjacency fetch, using edge adjacency and storing the output in a static array.
	// Requires that BuildEdgeList() be called prior to use.
	int GetAdjacentPoints(int querypoint, int outPoints[], int maxPoints);
	// As above, but also checks if each point has already been looked at (according to vispoint).
	int GetAdjacentUnvisitedPoints(int querypoint, int outPoints[], int maxPoints, bool* visPoint);

	// Creates the vertex color array (if necessary) and sets all the colors to the provided value.
	void ColorFill(const vec3& color);

	void ColorChannelFill(int channel, float value);

	~mesh();
};
