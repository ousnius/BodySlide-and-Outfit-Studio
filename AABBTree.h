#pragma once

#include "Object3d.h"

struct IntersectResult;

struct AABB {
	Vector3 min;
	Vector3 max;

	AABB();
	AABB(const Vector3 newMin, const Vector3 newMax);

	AABB(Vector3* points, int nPoints);

	AABB(Vertex* points, int nPoints);

	AABB(Vertex* points, ushort* indices, int nPoints);

	void AddBoxToMesh(vector<Vertex>& verts, vector<Edge>& edges);

	void Merge(Vertex* points, ushort* indices, int nPoints);
	void Merge(AABB& other);

	bool IntersectAABB(AABB& other);

	bool IntersectRay(Vector3& Origin, Vector3& Direction, Vector3* outCoord);

	bool IntersectSphere(Vector3& Origin, float radius);
};

class AABBTree {
	int max_depth;
	int min_facets;
	Vertex* vertexRef;
	Triangle* triRef;

public:
	bool bFlag;
	int depthCounter;
	int sentinel;

	class AABBTreeNode {
		AABBTreeNode* N;
		AABBTreeNode* P;
		AABBTreeNode* parent;
		AABB mBB;
		AABBTree* tree;
		int* mIFacets;
		int nFacets;
		int id;

	public:
		AABBTreeNode();
		~AABBTreeNode();

		// Recursively generates AABB Tree nodes using the referenced data.
		AABBTreeNode(vector<int>& facetIndices, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		// As above, but facetIndices is modified with in-place sorting rather than using vector::push_back to generate sub lists.
		// Sorting swaps from front of list to end when pos midpoints are found at the beginning of the list.
		AABBTreeNode(vector<int>& facetIndices, int start, int end, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		Vector3 Center();

		void AddDebugFrames(vector<Vertex>& verts, vector<Edge>& edges, int maxdepth = 0, int curdepth = 0);
		void AddRayIntersectFrames(Vector3& origin, Vector3& direction, vector<Vertex>& verts, vector<Edge>& edges);
		bool IntersectRay(Vector3& origin, Vector3& direction, vector<IntersectResult>* results);
		bool IntersectSphere(Vector3& origin, float radius, vector<IntersectResult>* results);
		void UpdateAABB(AABB* childBB = nullptr);
	};

	AABBTreeNode* root;

public:
	AABBTree();
	AABBTree(Vertex* vertices, Triangle* facets, int nFacets, int maxDepth, int minFacets);
	~AABBTree();

	int MinFacets();
	int MaxDepth();

	Vector3 Center();

	// Calculate bounding box and geometric average.
	void CalcAABBandGeoAvg(vector<int>& forFacets, AABB& outBB, Vector3& outAxisAvg);

	// Calculate bounding box and geometric average for sub list.
	void CalcAABBandGeoAvg(vector<int>& forFacets, int start, int end, AABB& outBB, Vector3& outAxisAvg);
	void CalcAABBandGeoAvg(int forFacets[], int start, int end, AABB& outBB, Vector3& outAxisAvg);
	void BuildDebugFrames(Vertex** outVerts, int* outNumVerts, Edge** outEdges, int* outNumEdges);
	void BuildRayIntersectFrames(Vector3& origin, Vector3& direction, Vertex** outVerts, int* outNumVerts, Edge** outEdges, int* outNumEdges);
	bool IntersectRay(Vector3& origin, Vector3& direction, vector<IntersectResult>* results = nullptr);
	bool IntersectSphere(Vector3& origin, float radius, vector<IntersectResult>* results = nullptr);
};

struct IntersectResult {
	int HitFacet;
	float HitDistance;
	Vector3 HitCoord;
	AABBTree::AABBTreeNode* bvhNode;
};
