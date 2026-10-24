/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "NifFile.hpp"

#include <vector>

struct ClippingFixOptions {
	float strength = 0.5f; // 0.0 to 1.0, controls inflation distance
	// Also push outfit triangles the body pokes out through between their vertices, which happens where large
	// outfit triangles span a curve of the body while every outfit vertex stays outside it
	bool fixPokeThrough = false;
};

// A body vertex that pokes out through the outfit: the outfit triangle it went through and how far back
// inwards along the body normal that triangle is
struct PokeThrough {
	int triangle = -1; // -1 where the vertex doesn't poke out
	float depth = 0.0f;
};

class ClippingFixer {
public:
	// Find the first shape in the NIF with a skin-tinted shader (the body/reference shape).
	static nifly::NiShape* FindReferenceShape(nifly::NifFile& nif);

	// Returns true if a shape is eligible for clipping fix.
	// Shapes are excluded when null, hidden, missing a shader, or skin-tinted.
	static bool IsEligibleForFix(nifly::NifFile& nif, nifly::NiShape* shape);

	// Fix clipping of outfit vertices that penetrate the body mesh.
	// Inflates the body surface outward and smoothly displaces nearby outfit
	// vertices along the interpolated body normal, preserving outfit detail.
	static void FixClipping(const std::vector<nifly::Vector3>& bodyVerts,
							const std::vector<nifly::Triangle>& bodyTris,
							std::vector<nifly::Vector3>& outfitVerts,
							const std::vector<nifly::Triangle>& outfitTris,
							const ClippingFixOptions& options);

	// Signed distance of each point from the body surface along the body's outward normal.
	// Negative values are inside the body. Points with no body surface within searchRadius get FLT_MAX.
	static std::vector<float> SignedDistancesToBody(const std::vector<nifly::Vector3>& bodyVerts,
											   const std::vector<nifly::Triangle>& bodyTris,
											   const std::vector<nifly::Vector3>& points,
											   float searchRadius);

	// Body vertices that are outside the outfit where it covers them, so the body shows through it, one per body
	// vertex. Found by looking inwards from each body vertex along its normal for an outfit triangle within
	// maxDepth that lies before the body's other side.
	static std::vector<PokeThrough> BodyPokeThrough(const std::vector<nifly::Vector3>& bodyVerts,
													const std::vector<nifly::Triangle>& bodyTris,
													const std::vector<nifly::Vector3>& outfitVerts,
													const std::vector<nifly::Triangle>& outfitTris,
													float maxDepth);

	// The deepest poke through each outfit vertex's triangles, as a positive distance, 0 where there is none
	static std::vector<float> PokeDepthPerVertex(const std::vector<PokeThrough>& pokes, const std::vector<nifly::Triangle>& outfitTris, size_t outfitVertexCount);

private:
	// Compute per-vertex normals for a mesh by averaging face normals.
	static std::vector<nifly::Vector3> ComputeVertexNormals(const std::vector<nifly::Vector3>& verts,
															const std::vector<nifly::Triangle>& tris);

	// Compute barycentric coordinates of point p on triangle (v1, v2, v3).
	static void BarycentricCoords(const nifly::Vector3& p,
								  const nifly::Vector3& v1,
								  const nifly::Vector3& v2,
								  const nifly::Vector3& v3,
								  float& u, float& v, float& w);

	// Build vertex adjacency lists from triangle data.
	static std::vector<std::vector<int>> BuildAdjacency(int nVerts, const std::vector<nifly::Triangle>& tris);
};
