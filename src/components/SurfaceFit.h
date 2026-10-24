/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class AABBTree;
struct AABBTriangleDistanceData;

// Fitting a mesh onto another surface without losing its own form: a non-rigid ICP whose displacement field is kept
// smooth by a Laplacian term. The mesh as a whole moves onto the surface while folds, seams, rims and buckles keep
// their shape, since only smooth changes of the displacement are cheap. Used to fit an outfit made for another body
// onto a game's body, or to morph one body onto another of a different topology. Works on plain vertex and triangle
// data in one coordinate space.
namespace SurfaceFit {

struct Options {
	// Distance outside the surface each vertex is fitted to: targetGaps when it has one entry per vertex, else gap
	float gap = 0.3f;
	std::vector<float> targetGaps;
	// Fit each vertex to the gap it has now (at least minGap): only clipping is pushed out, the rest stays
	bool keepGaps = false;
	// Smoothing passes that find the mesh's relief: how far each vertex stands out of the mesh smoothed this much (a
	// seam ridge, a rim, a strap on top) goes onto its target gap, so the relief stays instead of being pressed flat
	// onto the surface. 0 for none.
	int reliefSmoothing = 0;
	// Least target gap, after the relief is added
	float minGap = 0.02f;
	// Vertices farther than this from the surface aren't pulled to it; they follow their neighbors
	float maxDistance = 10.0f;
	// Least cosine between a vertex normal and the surface normal at its closest point. Faces pointing away from the
	// surface, like the inside of a thick garment, aren't pulled and keep their offset to the outside. -1 pulls all.
	float minNormalAgreement = 0.0f;
	// Also pull the mesh onto surface points it doesn't reach yet, e.g. a body onto the fuller breasts of another
	// body. Surface points whose closest point is an open edge of the mesh don't pull, so a partial mesh isn't
	// stretched over the parts of the surface it lacks.
	bool symmetric = false;
	// Weight of the smoothness term in the last stage: higher keeps the form stiffer. Earlier stages are stiffer.
	float stiffness = 1.0f;
	int stages = 4;
	// Correspondence updates per stage
	int iterations = 3;
	int solverIterations = 100;
	// Copies of a vertex closer than this (split UV or normal seams) move as one
	float weldDistance = 1e-4f;
	// Per vertex 0 (free) to 1 (stays), like a mask. Empty for all free.
	std::vector<float> lock;
	// Per vertex part number, -1 for none. Each part moves as a whole after the fit, like rigidify does.
	std::vector<int> rigidParts;
	// Rigid parts may also grow or shrink evenly, e.g. a collar onto a thicker neck
	bool rigidScale = false;
	// Passes at the end that push out what still clips: vertices inside the surface, and the mesh where the surface
	// pokes through between its vertices. 0 for none.
	int clippingPasses = 4;
	// How deep the surface can poke through the mesh to count, like analyze_clipping's poke-through
	float pokeDepth = 1.5f;
	// Where the fit would stretch or squash edges by more than this (longer over shorter, minus 1), the pull gives way
	// and the mesh keeps its form there. 0 for no limit.
	float maxStrain = 0.3f;
};

struct Result {
	std::vector<nifly::Vector3> positions;
	// Vertices pulled to the surface in the last correspondence update
	size_t pulled = 0;
	// Vertices of pieces with no pull at all, moved with the closest pulled vertices
	size_t followed = 0;
	// How far the pulled vertices are from their target gap after the fit
	float meanGapError = 0.0f;
	float maxGapError = 0.0f;
	// Vertices inside the surface, before and after
	size_t insideBefore = 0;
	size_t insideAfter = 0;
	// Surface vertices that poke through the mesh between its vertices, before and after
	size_t pokeBefore = 0;
	size_t pokeAfter = 0;
	// Triangles whose normal turned by more than 90 degrees
	size_t flipped = 0;
	float meanMove = 0.0f;
	float maxMove = 0.0f;
	// How much edges changed length (longer over shorter, minus 1), and how many by more than a quarter
	float maxStrain = 0.0f;
	float meanStrain = 0.0f;
	size_t strainedEdges = 0;
	// Vertex marks where the pull gave way to keep the form (maxStrain), summed over the rounds
	size_t relaxed = 0;
};

Result Fit(const std::vector<nifly::Vector3>& verts,
		   const std::vector<nifly::Triangle>& tris,
		   const std::vector<nifly::Vector3>& surfaceVerts,
		   const std::vector<nifly::Triangle>& surfaceTris,
		   const Options& options);

// Area-weighted vertex normals. Copies of a vertex closer than weldDistance share one.
std::vector<nifly::Vector3> VertexNormals(const std::vector<nifly::Vector3>& verts, const std::vector<nifly::Triangle>& tris, float weldDistance);

// Closest points on a triangle surface, with the interpolated vertex normal there. Safe to query from several threads.
class SurfaceQuery {
public:
	SurfaceQuery(const std::vector<nifly::Vector3>& verts, const std::vector<nifly::Triangle>& tris, const std::vector<nifly::Vector3>& normals);
	~SurfaceQuery();

	struct Hit {
		bool found = false;
		uint32_t triangle = 0;
		nifly::Vector3 point;
		nifly::Vector3 normal;
		float bary[3] = {0.0f, 0.0f, 0.0f};
	};

	Hit Closest(const nifly::Vector3& point, float radius) const;

	// Whether the closest point gives a clean pull: from in front of or behind the surface, not from beside an open
	// edge or sharp rim of it (the open wrist of a body without hands), which would drag a point sideways onto it
	static bool CleanPull(const nifly::Vector3& from, const Hit& hit);

private:
	// The tree keeps pointers to these
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Triangle> tris;
	std::vector<nifly::Vector3> normals;
	std::unique_ptr<AABBTree> tree;
	std::vector<AABBTriangleDistanceData> distanceData;
};

} // namespace SurfaceFit
