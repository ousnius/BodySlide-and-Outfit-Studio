#pragma once

#include "Object3d.hpp"

#include <memory>

struct IntersectResult;

struct AABB {
	nifly::Vector3 min;
	nifly::Vector3 max;

	AABB() {}
	AABB(const nifly::Vector3& newMin, const nifly::Vector3& newMax);

	AABB(nifly::Vector3* points, int nPoints);
	AABB(nifly::Vector3* points, nifly::ushort* indices, int nPoints);

	void AddBoxToMesh(std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges);

	void Merge(nifly::Vector3* points, nifly::ushort* indices, int nPoints);
	void Merge(AABB& other);

	bool IntersectAABB(AABB& other);

	bool IntersectRay(nifly::Vector3& Origin, nifly::Vector3& Direction, nifly::Vector3* outCoord);

	bool IntersectSphere(nifly::Vector3& Origin, float radius);
};

class AABBTree {
	int max_depth = 100;
	int min_facets = 2;
	nifly::Vector3* vertexRef = nullptr;
	nifly::Triangle* triRef = nullptr;

public:
	bool bFlag = false;
	int depthCounter = 0;
	int sentinel = 0;

	class AABBTreeNode {
		std::unique_ptr<AABBTreeNode> N;
		std::unique_ptr<AABBTreeNode> P;
		AABBTreeNode* parent = nullptr;
		AABB mBB;
		AABBTree* tree = nullptr;
		std::unique_ptr<int[]> mIFacets;
		int nFacets = 0;

	public:
		AABBTreeNode() {}

		// Recursively generates AABB Tree nodes using the referenced data.
		AABBTreeNode(std::vector<int>& facetIndices, AABBTree* treeRef, AABBTreeNode* parentRef, int depth);

		// As above, but facetIndices is modified with in-place sorting rather than using vector::push_back to generate sub lists.
		// Sorting swaps from front of list to end when pos midpoints are found at the beginning of the list.
		AABBTreeNode(std::vector<int>& facetIndices, int start, int end, AABBTree* treeRef, AABBTreeNode* parentRef, int depth);

		nifly::Vector3 Center();

		void AddDebugFrames(std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges, int maxdepth = 0, int curdepth = 0);
		void AddRayIntersectFrames(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges);
		bool IntersectRay(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<IntersectResult>* results);
		bool IntersectSphere(nifly::Vector3& origin, float radius, std::vector<IntersectResult>* results);
		void UpdateAABB(AABB* childBB = nullptr);
	};

	std::unique_ptr<AABBTreeNode> root;

public:
	AABBTree() {}
	AABBTree(nifly::Vector3* vertices, nifly::Triangle* facets, int nFacets, int maxDepth, int minFacets);

	int MinFacets();
	int MaxDepth();

	nifly::Vector3 Center();

	// Calculate bounding box and geometric average.
	void CalcAABBandGeoAvg(std::vector<int>& forFacets, AABB& outBB, nifly::Vector3& outAxisAvg);

	// Calculate bounding box and geometric average for sub list.
	void CalcAABBandGeoAvg(std::vector<int>& forFacets, int start, int end, AABB& outBB, nifly::Vector3& outAxisAvg);
	void CalcAABBandGeoAvg(int forFacets[], int start, int end, AABB& outBB, nifly::Vector3& outAxisAvg);
	void BuildDebugFrames(nifly::Vector3** outVerts, int* outNumVerts, nifly::Edge** outEdges, int* outNumEdges);
	void BuildRayIntersectFrames(nifly::Vector3& origin, nifly::Vector3& direction, nifly::Vector3** outVerts, int* outNumVerts, nifly::Edge** outEdges, int* outNumEdges);
	bool IntersectRay(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<IntersectResult>* results = nullptr);
	bool IntersectSphere(nifly::Vector3& origin, float radius, std::vector<IntersectResult>* results = nullptr);
};

struct IntersectResult {
	int HitFacet = 0;
	float HitDistance = 0.0f;
	nifly::Vector3 HitCoord;
	AABBTree::AABBTreeNode* bvhNode = nullptr;
};
