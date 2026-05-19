#pragma once

#include "Object3d.hpp"

#include <cstdint>
#include <memory>
#include <vector>

struct IntersectResult;

struct AABBTriangleDistanceData {
	nifly::Vector3 v1;
	nifly::Vector3 edge12;
	nifly::Vector3 edge13;
	nifly::Vector3 edge23;
	float areaMetric = 0.0f;
};

struct AABB {
	nifly::Vector3 min;
	nifly::Vector3 max;

	AABB() {}
	AABB(const nifly::Vector3& newMin, const nifly::Vector3& newMax);

	AABB(const nifly::Vector3* points, const uint16_t nPoints);
	AABB(const nifly::Vector3* points, const uint16_t* indices, const uint16_t nPoints);

	void AddBoxToMesh(std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges);

	void Merge(const nifly::Vector3* points, const uint16_t* indices, const uint16_t nPoints);
	void Merge(const AABB& other);

	bool IntersectAABB(const AABB& other);

	bool IntersectRay(const nifly::Vector3& Origin, const nifly::Vector3& Direction, nifly::Vector3* outCoord);

	float DistanceSquaredToPoint(const nifly::Vector3& point) const;

	bool IntersectSphere(const nifly::Vector3& Origin, const float radius);
};

class AABBTree {
	uint32_t max_depth = 100;
	uint32_t min_facets = 2;
	nifly::Vector3* vertexRef = nullptr;
	nifly::Triangle* triRef = nullptr;

public:
	bool bFlag = false;
	uint32_t depthCounter = 0;
	uint32_t sentinel = 0;

	class AABBTreeNode {
		std::unique_ptr<AABBTreeNode> N;
		std::unique_ptr<AABBTreeNode> P;
		AABBTreeNode* parent = nullptr;
		AABB mBB;
		AABBTree* tree = nullptr;
		std::unique_ptr<uint32_t[]> mIFacets;
		uint32_t nFacets = 0;

	public:
		AABBTreeNode() {}

		// Recursively generates AABB Tree nodes using the referenced data.
		AABBTreeNode(std::vector<uint32_t>& facetIndices, AABBTree* treeRef, AABBTreeNode* parentRef, const uint32_t depth);

		// As above, but facetIndices is modified with in-place sorting rather than using vector::push_back to generate sub lists.
		// Sorting swaps from front of list to end when pos midpoints are found at the beginning of the list.
		AABBTreeNode(std::vector<uint32_t>& facetIndices, const uint32_t start, const uint32_t end, AABBTree* treeRef, AABBTreeNode* parentRef, const uint32_t depth);

		nifly::Vector3 Center();

		void AddDebugFrames(std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges, const uint32_t maxdepth = 0, const uint32_t curdepth = 0);
		void AddRayIntersectFrames(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<nifly::Vector3>& verts, std::vector<nifly::Edge>& edges);
		bool IntersectRay(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<IntersectResult>* results);
		bool IntersectSphere(nifly::Vector3& origin, const float radius, std::vector<IntersectResult>* results);
		bool ClosestFacetInSphere(nifly::Vector3& origin,
								   const float radiusSquared,
								   float& bestDistSquared,
								   uint32_t& facetIndex,
								   nifly::Vector3* closestPoint,
								   const std::vector<AABBTriangleDistanceData>* triangleDistanceData);
		void UpdateAABB(const AABB* childBB = nullptr);
	};

	std::unique_ptr<AABBTreeNode> root;

public:
	AABBTree() {}
	AABBTree(nifly::Vector3* vertices, nifly::Triangle* facets, const uint32_t nFacets, const uint32_t maxDepth, const uint32_t minFacets);

	uint32_t MinFacets();
	uint32_t MaxDepth();

	nifly::Vector3 Center();

	// Calculate bounding box and geometric average.
	void CalcAABBandGeoAvg(std::vector<uint32_t>& forFacets, AABB& outBB, nifly::Vector3& outAxisAvg);

	// Calculate bounding box and geometric average for sub list.
	void CalcAABBandGeoAvg(std::vector<uint32_t>& forFacets, const uint32_t start, const uint32_t end, AABB& outBB, nifly::Vector3& outAxisAvg);
	void CalcAABBandGeoAvg(const uint32_t forFacets[], const uint32_t start, const uint32_t end, AABB& outBB, nifly::Vector3& outAxisAvg);
	void BuildDebugFrames(nifly::Vector3** outVerts, uint16_t* outNumVerts, nifly::Edge** outEdges, uint32_t* outNumEdges);
	void BuildRayIntersectFrames(nifly::Vector3& origin, nifly::Vector3& direction, nifly::Vector3** outVerts, uint16_t* outNumVerts, nifly::Edge** outEdges, uint32_t* outNumEdges);
	bool IntersectRay(nifly::Vector3& origin, nifly::Vector3& direction, std::vector<IntersectResult>* results = nullptr);
	bool IntersectSphere(nifly::Vector3& origin, const float radius, std::vector<IntersectResult>* results = nullptr);
	bool ClosestFacetInSphere(nifly::Vector3& origin,
						 const float radius,
						 uint32_t& facetIndex,
						 nifly::Vector3* closestPoint = nullptr,
						 const std::vector<AABBTriangleDistanceData>* triangleDistanceData = nullptr);
	static void BuildTriangleDistanceData(const nifly::Vector3* vertices,
									 const nifly::Triangle* facets,
									 uint32_t nFacets,
									 std::vector<AABBTriangleDistanceData>& outData);
};

struct IntersectResult {
	uint32_t HitFacet = 0;
	float HitDistance = 0.0f;
	nifly::Vector3 HitCoord;
	AABBTree::AABBTreeNode* bvhNode = nullptr;
};
