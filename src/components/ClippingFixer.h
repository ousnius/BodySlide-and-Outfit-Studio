/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "NifFile.hpp"

#include <vector>

struct ClippingFixOptions {
	float strength = 0.5f; // 0.0 to 1.0, controls inflation distance
};

class ClippingFixer {
public:
	// Find the first shape in the NIF with a skin-tinted shader (the body/reference shape).
	static nifly::NiShape* FindReferenceShape(nifly::NifFile& nif);

	// Fix clipping of outfit vertices that penetrate the body mesh.
	// Inflates the body surface outward and smoothly displaces nearby outfit
	// vertices along the interpolated body normal, preserving outfit detail.
	static void FixClipping(const std::vector<nifly::Vector3>& bodyVerts,
							const std::vector<nifly::Triangle>& bodyTris,
							std::vector<nifly::Vector3>& outfitVerts,
							const std::vector<nifly::Triangle>& outfitTris,
							const ClippingFixOptions& options);

private:
	// Compute per-vertex normals for a mesh by averaging face normals.
	static std::vector<nifly::Vector3> ComputeVertexNormals(const std::vector<nifly::Vector3>& verts,
															const std::vector<nifly::Triangle>& tris);

	// Find the closest point on a triangle to a given point, and return
	// the signed distance (negative = inside/below the surface).
	static float SignedDistanceToTriangle(const nifly::Vector3& point,
										  const nifly::Triangle& tri,
										  const nifly::Vector3* bodyVerts,
										  nifly::Vector3& outClosestPoint);

	// Compute barycentric coordinates of point p on triangle (v1, v2, v3).
	static void BarycentricCoords(const nifly::Vector3& p,
								  const nifly::Vector3& v1,
								  const nifly::Vector3& v2,
								  const nifly::Vector3& v3,
								  float& u, float& v, float& w);

	// Build vertex adjacency lists from triangle data.
	static std::vector<std::vector<int>> BuildAdjacency(int nVerts, const std::vector<nifly::Triangle>& tris);
};
