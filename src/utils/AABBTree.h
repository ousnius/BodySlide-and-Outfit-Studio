#pragma once

#include "../NIF/utils/Object3d.h"

struct IntersectResult;

struct AABB {
	Vector3 min;
	Vector3 max;

	AABB();
	AABB(const Vector3 newMin, const Vector3 newMax);

	AABB(Vector3* points, int nPoints);
	AABB(Vector3* points, ushort* indices, int nPoints);

	void AddBoxToMesh(std::vector<Vector3>& verts, std::vector<Edge>& edges);

	void Merge(Vector3* points, ushort* indices, int nPoints);
	void Merge(AABB& other);

	bool IntersectAABB(AABB& other);

	bool IntersectRay(Vector3& Origin, Vector3& Direction, Vector3* outCoord);

	bool IntersectSphere(Vector3& Origin, float radius);
};

class AABBTree {
	int max_depth;
	int min_facets;
	Vector3* vertexRef;
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
		AABBTreeNode(std::vector<int>& facetIndices, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		// As above, but facetIndices is modified with in-place sorting rather than using vector::push_back to generate sub lists.
		// Sorting swaps from front of list to end when pos midpoints are found at the beginning of the list.
		AABBTreeNode(std::vector<int>& facetIndices, int start, int end, AABBTree* treeRef, AABBTreeNode* parent, int depth);

		Vector3 Center();

		void AddDebugFrames(std::vector<Vector3>& verts, std::vector<Edge>& edges, int maxdepth = 0, int curdepth = 0);
		void AddRayIntersectFrames(Vector3& origin, Vector3& direction, std::vector<Vector3>& verts, std::vector<Edge>& edges);
		bool IntersectRay(Vector3& origin, Vector3& direction, std::vector<IntersectResult>* results);
		bool IntersectSphere(Vector3& origin, float radius, std::vector<IntersectResult>* results);
		void UpdateAABB(AABB* childBB = nullptr);
	};

	AABBTreeNode* root;

public:
	AABBTree();
	AABBTree(Vector3* vertices, Triangle* facets, int nFacets, int maxDepth, int minFacets);
	~AABBTree();

	int MinFacets();
	int MaxDepth();

	Vector3 Center();

	// Calculate bounding box and geometric average.
	void CalcAABBandGeoAvg(std::vector<int>& forFacets, AABB& outBB, Vector3& outAxisAvg);

	// Calculate bounding box and geometric average for sub list.
	void CalcAABBandGeoAvg(std::vector<int>& forFacets, int start, int end, AABB& outBB, Vector3& outAxisAvg);
	void CalcAABBandGeoAvg(int forFacets[], int start, int end, AABB& outBB, Vector3& outAxisAvg);
	void BuildDebugFrames(Vector3** outVerts, int* outNumVerts, Edge** outEdges, int* outNumEdges);
	void BuildRayIntersectFrames(Vector3& origin, Vector3& direction, Vector3** outVerts, int* outNumVerts, Edge** outEdges, int* outNumEdges);
	bool IntersectRay(Vector3& origin, Vector3& direction, std::vector<IntersectResult>* results = nullptr);
	bool IntersectSphere(Vector3& origin, float radius, std::vector<IntersectResult>* results = nullptr);
};

struct IntersectResult {
	int HitFacet;
	float HitDistance;
	Vector3 HitCoord;
	AABBTree::AABBTreeNode* bvhNode;
};
