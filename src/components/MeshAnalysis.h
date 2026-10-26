/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
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

// Indices of the triangles whose normal turns by more than 90 degrees between the two vertex positions.
// Degenerate triangles are left out, they have no facing to flip.
std::vector<size_t> FlippedTriangles(const std::vector<nifly::Vector3>& before, const std::vector<nifly::Vector3>& after, const std::vector<nifly::Triangle>& tris);

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

// Color for a value between 0 and 1 on a blue, cyan, green, yellow, red ramp. Values outside are clamped.
nifly::Vector3 HeatmapColor(float t);

// Roughness of each vertex as AnalyzeMorph measures it: the length of the difference between its delta and
// the mean delta of its neighbors. 0 for vertices that don't move.
std::vector<float> VertexRoughness(const std::vector<nifly::Triangle>& tris, const std::vector<nifly::Vector3>& deltas, float moveThreshold = 1e-4f);

// Groups of two or more points closer to each other than tolerance, e.g. the copies of a vertex along a
// split UV or normal seam, or the vertices where two shapes meet. Points of several meshes can be passed
// together. Each point is in at most one group.
std::vector<std::vector<int>> CoincidentGroups(const std::vector<nifly::Vector3>& points, float tolerance);

// Connected pieces of a mesh: vertices joined by triangles, or closer than weldDistance (split seams).
// Only vertices with include set take part, when it's given; the others get -1. Parts are numbered by
// size, largest first. Returns the part of each vertex.
std::vector<int> ConnectedParts(const std::vector<nifly::Vector3>& verts,
								const std::vector<nifly::Triangle>& tris,
								float weldDistance,
								const std::vector<bool>* include = nullptr,
								int* outPartCount = nullptr);

// Which vertices of a mesh a surface covers, e.g. the body under an outfit: a line along the vertex normal
// meets the surface no further than outside in front of the vertex or inside behind it (where the surface
// sinks into the mesh). Both sides of the surface's triangles count.
std::vector<bool> CoveredVertices(const std::vector<nifly::Vector3>& verts,
								  const std::vector<nifly::Triangle>& tris,
								  const std::vector<nifly::Vector3>& surfaceVerts,
								  const std::vector<nifly::Triangle>& surfaceTris,
								  float outside,
								  float inside);

// Rotation, uniform scale and translation, applied in that order
struct SimilarityTransform {
	double rotation[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
	double scale = 1.0;
	nifly::Vector3 translation;

	nifly::Vector3 Apply(const nifly::Vector3& p) const;
};

// The rotation and translation, and with allowScale the uniform scale, that best map the points from onto
// the points to, by weighted least squares. Weights are optional, one per point.
SimilarityTransform BestFitTransform(const std::vector<nifly::Vector3>& from,
									 const std::vector<nifly::Vector3>& to,
									 const std::vector<float>* weights,
									 bool allowScale);

// Deltas of a morph with each part moving as a whole, without bending: the vertices of a part get the
// motion of the transform that best fits their deltas. influence (0 to 1 per vertex) blends between the
// morph and that motion, and weights the fit. Vertices with a negative part or no influence keep their
// deltas.
std::vector<nifly::Vector3> RigidifyDeltas(const std::vector<nifly::Vector3>& verts,
										   const std::vector<nifly::Vector3>& deltas,
										   const std::vector<int>& parts,
										   const std::vector<float>& influence,
										   bool allowScale);

// The closest point of a surface to a point, as a triangle and barycentric coordinates
struct SurfacePoint {
	int triangle = -1; // -1 when no surface is within the search radius
	float u = 0.0f;
	float v = 0.0f;
	float w = 0.0f;
	float distance = 0.0f;
};

std::vector<SurfacePoint> ClosestSurfacePoints(const std::vector<nifly::Vector3>& surfaceVerts,
											   const std::vector<nifly::Triangle>& surfaceTris,
											   const std::vector<nifly::Vector3>& points,
											   float searchRadius);

// Bone weights of a mesh by bone name, each by vertex index
using BoneWeightMap = std::unordered_map<std::string, const std::unordered_map<uint16_t, float>*>;

// How much the weights of each point differ from the weights of the body at its closest surface point:
// half the summed difference over all bones, from 0 (the same) to 1 (no bone in common). -1 where the
// point has no surface point.
std::vector<float> WeightDeviation(const std::vector<SurfacePoint>& closest,
								   const std::vector<nifly::Triangle>& bodyTris,
								   const BoneWeightMap& bodyWeights,
								   const BoneWeightMap& pointWeights);

struct StrainStats {
	size_t edges = 0;
	size_t stretched = 0;	 // Edges whose strain is above the threshold
	float maxStrain = 0.0f;
	float meanStrain = 0.0f;
	// Per vertex: the largest strain of its edges
	std::vector<float> vertexStrain;
};

// How much the edges of a mesh change length between two vertex positions, e.g. unposed and posed. The strain of an
// edge is the longer length divided by the shorter one, minus 1: 0 for an edge that only moves or turns, 1 for one
// that doubles or halves. Weights that change abruptly between neighbors tear a mesh apart when bones turn, which
// shows as a few edges with a very high strain.
StrainStats AnalyzeStrain(const std::vector<nifly::Vector3>& before,
						  const std::vector<nifly::Vector3>& after,
						  const std::vector<nifly::Triangle>& tris,
						  float threshold = 0.5f);

// Weights of a mesh as one row per vertex and one column per bone
using DenseWeights = std::vector<std::vector<float>>;

// Blends the weights of each vertex with those of its neighbors, the way a smoothing brush does: each pass moves
// a vertex's weights towards the mean of its weighted neighbors by strength times its influence (0 to 1, empty for
// all 1). Vertices without influence stay as they are and anchor their neighbors. With bones, only those columns are
// blended, and each vertex keeps the total weight it had on them. The weights of a vertex keep adding up to what they
// did when the neighbors' do.
DenseWeights SmoothWeights(const DenseWeights& weights,
						   const std::vector<std::vector<int>>& neighbors,
						   const std::vector<float>& influence,
						   int iterations,
						   float strength,
						   const std::vector<bool>* bones = nullptr);

// Drops weights below pruneBelow, keeps the maxInfluences largest (0 for no limit) and makes them add up to 1.
// The largest weight is kept when all are below pruneBelow. Returns whether anything changed.
bool LimitWeights(std::vector<float>& weights, int maxInfluences, float pruneBelow);

// How abruptly the weights change at each vertex: the largest difference to a neighbor's weights, as half the summed
// difference over all bones, from 0 (the same) to 1 (no bone in common)
std::vector<float> WeightJump(const DenseWeights& weights, const std::vector<std::vector<int>>& neighbors);

} // namespace MeshAnalysis
