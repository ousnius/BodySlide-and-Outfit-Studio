/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Quality measurements of outfit meshes, their morphs and weights. Works on plain vertex
// and triangle data in one coordinate space, so it doesn't depend on the editor.
namespace MeshAnalysis {

struct PenetrationStats {
	// Per vertex: distance outside of the body surface, negative inside it.
	// FLT_MAX where no body surface is within the search radius.
	std::vector<float> signedDistances;
	size_t penetrating = 0; // Vertices inside the body
	size_t nearSurface = 0; // Vertices outside, but closer than the near distance
	float maxDepth = 0.0f;	// Deepest penetration, as a positive value
	float meanDepth = 0.0f; // Mean depth of the penetrating vertices
};

PenetrationStats AnalyzePenetration(const std::vector<nifly::Vector3>& outfitVerts,
									const std::vector<nifly::Vector3>& bodyVerts,
									const std::vector<nifly::Triangle>& bodyTris,
									float nearDistance = 0.1f,
									float searchRadius = 5.0f);

struct MorphStats {
	size_t movedCount = 0; // Vertices moved by more than the threshold
	float meanDelta = 0.0f; // Mean length of the deltas of moved vertices
	float maxDelta = 0.0f;
	// Roughness is the length of the difference between a vertex delta and the mean delta of its
	// neighbors, averaged over the moved vertices. Normalized divides it by the mean delta, so it
	// doesn't depend on morph strength. Always look at both: a morph that only got weaker reads
	// as rougher when normalized.
	float roughness = 0.0f;
	float normalizedRoughness = 0.0f;
	// Highest roughness of a moved vertex next to an unmoved one: a step at the edge of the morph
	float maxBoundaryRoughness = 0.0f;
};

MorphStats AnalyzeMorph(const std::vector<nifly::Triangle>& tris, const std::vector<nifly::Vector3>& deltas, float moveThreshold = 1e-4f);

// Triangles whose normal turns by more than 90 degrees between the two vertex positions
size_t CountFlippedTriangles(const std::vector<nifly::Vector3>& before, const std::vector<nifly::Vector3>& after, const std::vector<nifly::Triangle>& tris);

struct WeightStats {
	size_t unweighted = 0;			// Vertices without any weight
	size_t overInfluenceLimit = 0;	// Vertices with more bones than the limit
	size_t badSum = 0;				// Weighted vertices whose weights don't add up to 1
	float maxSumError = 0.0f;
	int maxInfluences = 0;
};

// boneWeights holds the weights of each bone by vertex index
WeightStats AnalyzeWeights(size_t vertexCount,
						   const std::vector<const std::unordered_map<uint16_t, float>*>& boneWeights,
						   int influenceLimit = 4,
						   float sumTolerance = 0.01f);

struct DeltaComparison {
	size_t compared = 0;
	float meanError = 0.0f;
	float maxError = 0.0f;
	float rmsError = 0.0f;
};

// Per-vertex distance between two sets of deltas of the same vertex count
DeltaComparison CompareDeltas(const std::vector<nifly::Vector3>& a, const std::vector<nifly::Vector3>& b);

} // namespace MeshAnalysis
