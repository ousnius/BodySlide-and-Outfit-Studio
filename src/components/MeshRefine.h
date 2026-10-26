/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

// Picks the edges of a mesh to split where it lacks detail: long edges, edges across curves that look faceted, and
// edges the body pokes through or bulges into because the outfit is coarser than the body underneath. Works on plain
// vertex and triangle data in one coordinate space, so it doesn't depend on the editor.
namespace MeshRefine {

// Why an edge is split, as bits
enum Reason : uint8_t {
	ReasonLength = 1 << 0,
	ReasonCurvature = 1 << 1,
	ReasonClipping = 1 << 2,
	ReasonBodyDetail = 1 << 3,
	// Split only so the triangles next to another split edge don't turn into slivers
	ReasonConforming = 1 << 4,
};

struct Options {
	// Split edges longer than this. 0 for off.
	float maxEdgeLength = 0.0f;
	// Split edges whose vertex normals turn by more than this many degrees, so curves look rounder. 0 for off.
	float maxAngle = 0.0f;
	// Edges between triangles that meet at a sharper angle (in degrees) are creases. Curvature leaves them and the edges
	// touching them alone.
	float creaseAngle = 75.0f;
	// Split edges of triangles the body pokes out through, and edges the body bulges into between their vertices
	bool clipping = false;
	// With clipping: also split edges whose middle is closer to the body than this, and closer than their ends
	float clearance = 0.0f;
	// Split edges near the body that are longer than this many times the body's edges there. 0 for off.
	float bodyDetail = 0.0f;
	// How close to the body an edge's middle has to be for bodyDetail
	float bodyDistance = 1.0f;
	// Edges shorter than this are never split
	float minEdgeLength = 0.05f;
	// Split at most this many edges, the worst first. 0 for no limit.
	size_t maxEdges = 0;
	// Also split the longest edge of each triangle next to a split edge, until the longest edges are split first,
	// so splitting leaves no long thin triangles
	bool conforming = true;
};

// Surface to measure clipping against, in the same coordinate space as the mesh
struct BodySurface {
	const std::vector<nifly::Vector3>* verts = nullptr;
	const std::vector<nifly::Triangle>* tris = nullptr;
};

// An edge to split, by the lowest index of each of its vertices' welded copies, p1 < p2
struct SplitEdge {
	uint16_t p1 = 0;
	uint16_t p2 = 0;
	float score = 0.0f; // How far over its threshold the edge is, clipping first
	uint8_t reasons = 0;
};

struct Selection {
	std::vector<SplitEdge> edges; // Worst first
	size_t byLength = 0;
	size_t byCurvature = 0;
	size_t byClipping = 0;
	size_t byBodyDetail = 0;
	size_t conforming = 0; // Split only for conforming
	size_t overLimit = 0;  // Edges that met a criterion but were left out for maxEdges
	size_t pokeThrough = 0; // Body vertices that poke out through the mesh
};

// Picks the edges to split. weldIndex maps each vertex to the lowest index of its welded copies (split UV seams), so
// both sides of a seam get split together; empty for none. Only edges with both vertices in include are split, all
// when include is empty. Excluded vertices aren't checked for clipping. body is needed for clipping and bodyDetail.
Selection SelectEdges(const std::vector<nifly::Vector3>& verts,
					  const std::vector<nifly::Triangle>& tris,
					  const std::vector<int>& weldIndex,
					  const std::vector<bool>& include,
					  const std::vector<bool>& excluded,
					  const BodySurface& body,
					  const Options& options);

} // namespace MeshRefine
