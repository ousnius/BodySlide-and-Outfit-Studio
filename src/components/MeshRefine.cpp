/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MeshRefine.h"
#include "ClippingFixer.h"
#include "MeshAnalysis.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

using namespace nifly;

namespace MeshRefine {

namespace {

constexpr float DegreesToRadians = 3.14159265358979f / 180.0f;
// Pokes deeper than this are the outfit's own parts inside the body, which more triangles don't help with
constexpr float MaxPokeDepth = 1.0f;
// Clipping goes before the other criteria when the edges are limited
constexpr float ClippingScore = 10.0f;

struct EdgeData {
	uint16_t p1 = 0;
	uint16_t p2 = 0;
	float length = 0.0f;
	std::vector<int> tris;
	float score = 0.0f;
	uint8_t reasons = 0;
};

uint32_t EdgeKey(uint16_t a, uint16_t b) {
	if (a > b)
		std::swap(a, b);
	return (static_cast<uint32_t>(a) << 16) | b;
}

} // namespace

Selection SelectEdges(const std::vector<Vector3>& verts,
					  const std::vector<Triangle>& tris,
					  const std::vector<int>& weldIndex,
					  const std::vector<bool>& include,
					  const std::vector<bool>& excluded,
					  const BodySurface& body,
					  const Options& options) {
	Selection selection;
	const size_t vertexCount = verts.size();
	auto canon = [&](uint16_t v) { return v < weldIndex.size() ? static_cast<uint16_t>(weldIndex[v]) : v; };

	// A vertex can be split away from when all its copies can, and is left out of clipping when any copy is
	std::vector<bool> canSplit(vertexCount, true);
	std::vector<bool> isExcluded(vertexCount, false);
	for (size_t v = 0; v < vertexCount; v++) {
		uint16_t c = canon(static_cast<uint16_t>(v));
		if (c >= vertexCount)
			continue;
		if (!include.empty() && (v >= include.size() || !include[v]))
			canSplit[c] = false;
		if (v < excluded.size() && excluded[v])
			isExcluded[c] = true;
	}

	// Edges by their welded vertices, so both sides of a seam are one edge with the triangles of both
	std::vector<EdgeData> edges;
	std::unordered_map<uint32_t, int> edgeIndex;
	std::vector<std::array<int, 3>> triEdges(tris.size(), {-1, -1, -1});
	std::vector<Vector3> faceNormals(tris.size());
	std::vector<Vector3> vertexNormals(vertexCount);
	for (size_t t = 0; t < tris.size(); t++) {
		const Triangle& tri = tris[t];
		if (tri.p1 >= vertexCount || tri.p2 >= vertexCount || tri.p3 >= vertexCount)
			continue;

		// Area weighted, so slivers don't tip the normals
		Vector3 normal = (verts[tri.p2] - verts[tri.p1]).cross(verts[tri.p3] - verts[tri.p1]);
		const uint16_t c[3] = {canon(tri.p1), canon(tri.p2), canon(tri.p3)};
		for (uint16_t v : c)
			if (v < vertexCount)
				vertexNormals[v] += normal;
		normal.Normalize();
		faceNormals[t] = normal;

		for (int i = 0; i < 3; i++) {
			uint16_t a = c[i];
			uint16_t b = c[(i + 1) % 3];
			if (a == b || a >= vertexCount || b >= vertexCount)
				continue;

			auto inserted = edgeIndex.emplace(EdgeKey(a, b), static_cast<int>(edges.size()));
			if (inserted.second) {
				EdgeData edge;
				edge.p1 = std::min(a, b);
				edge.p2 = std::max(a, b);
				edge.length = verts[a].DistanceTo(verts[b]);
				edges.push_back(std::move(edge));
			}
			int e = inserted.first->second;
			edges[e].tris.push_back(static_cast<int>(t));
			triEdges[t][i] = e;
		}
	}

	for (auto& n : vertexNormals)
		n.Normalize();

	auto splittable = [&](const EdgeData& edge) { return canSplit[edge.p1] && canSplit[edge.p2] && edge.length >= options.minEdgeLength; };
	auto mark = [](EdgeData& edge, uint8_t reason, float score) {
		edge.reasons |= reason;
		edge.score = std::max(edge.score, score);
	};

	if (options.maxEdgeLength > 0.0f) {
		for (auto& edge : edges)
			if (splittable(edge) && edge.length > options.maxEdgeLength)
				mark(edge, ReasonLength, edge.length / options.maxEdgeLength);
	}

	if (options.maxAngle > 0.0f) {
		// A crease (or triangles facing opposite ways) is meant to be sharp, rounding it would blunt it. The normals of
		// its vertices lean halfway across it, so the flat edges next to it look curved too.
		const float cosCrease = std::cos(options.creaseAngle * DegreesToRadians);
		std::vector<bool> onCrease(vertexCount, false);
		for (auto& edge : edges) {
			bool crease = false;
			for (size_t i = 0; i < edge.tris.size() && !crease; i++)
				for (size_t j = i + 1; j < edge.tris.size() && !crease; j++)
					crease = faceNormals[edge.tris[i]].dot(faceNormals[edge.tris[j]]) < cosCrease;
			if (crease) {
				onCrease[edge.p1] = true;
				onCrease[edge.p2] = true;
			}
		}

		for (auto& edge : edges) {
			if (!splittable(edge) || onCrease[edge.p1] || onCrease[edge.p2])
				continue;

			float cosAngle = std::clamp(vertexNormals[edge.p1].dot(vertexNormals[edge.p2]), -1.0f, 1.0f);
			float angle = std::acos(cosAngle) / DegreesToRadians;
			if (angle > options.maxAngle)
				mark(edge, ReasonCurvature, angle / options.maxAngle);
		}
	}

	const bool hasBody = body.verts && body.tris && !body.verts->empty() && !body.tris->empty();
	if (hasBody && (options.clipping || options.bodyDetail > 0.0f)) {
		// Middles of the edges that may be split
		std::vector<int> midEdges;
		std::vector<Vector3> mids;
		for (size_t e = 0; e < edges.size(); e++) {
			const EdgeData& edge = edges[e];
			if (!splittable(edge) || isExcluded[edge.p1] || isExcluded[edge.p2])
				continue;
			midEdges.push_back(static_cast<int>(e));
			mids.push_back((verts[edge.p1] + verts[edge.p2]) * 0.5f);
		}

		if (options.clipping) {
			// The body bulging into the middle of an edge while its ends are farther out: the edge cuts the curve
			std::vector<Vector3> points = verts;
			points.insert(points.end(), mids.begin(), mids.end());
			const float searchRadius = std::max({5.0f, options.clearance * 2.0f, options.bodyDistance * 2.0f});
			std::vector<float> distances = ClippingFixer::SignedDistancesToBody(*body.verts, *body.tris, points, searchRadius);
			for (size_t i = 0; i < midEdges.size(); i++) {
				EdgeData& edge = edges[midEdges[i]];
				float mid = distances[vertexCount + i];
				float d1 = distances[edge.p1];
				float d2 = distances[edge.p2];
				if (mid == FLT_MAX || d1 == FLT_MAX || d2 == FLT_MAX)
					continue;

				float ends = std::min(d1, d2);
				if (mid < options.clearance && mid < ends)
					mark(edge, ReasonClipping, ClippingScore + (ends - mid));
			}

			// The body showing through a triangle between its vertices
			auto pokes = ClippingFixer::BodyPokeThrough(*body.verts, *body.tris, verts, tris, MaxPokeDepth);
			for (auto& poke : pokes) {
				if (poke.triangle < 0 || static_cast<size_t>(poke.triangle) >= tris.size())
					continue;

				const Triangle& tri = tris[poke.triangle];
				if (isExcluded[canon(tri.p1)] || isExcluded[canon(tri.p2)] || isExcluded[canon(tri.p3)])
					continue;

				selection.pokeThrough++;
				for (int e : triEdges[poke.triangle])
					if (e >= 0 && splittable(edges[e]))
						mark(edges[e], ReasonClipping, ClippingScore + poke.depth);
			}
		}

		if (options.bodyDetail > 0.0f) {
			// Edges much longer than the body's edges under them can't follow the body's shape there
			auto closest = MeshAnalysis::ClosestSurfacePoints(*body.verts, *body.tris, mids, options.bodyDistance);
			const auto& bodyVerts = *body.verts;
			for (size_t i = 0; i < midEdges.size(); i++) {
				if (closest[i].triangle < 0)
					continue;

				const Triangle& bt = (*body.tris)[closest[i].triangle];
				float bodyEdge = (bodyVerts[bt.p1].DistanceTo(bodyVerts[bt.p2]) + bodyVerts[bt.p2].DistanceTo(bodyVerts[bt.p3])
								  + bodyVerts[bt.p3].DistanceTo(bodyVerts[bt.p1]))
								 / 3.0f;
				EdgeData& edge = edges[midEdges[i]];
				float limit = options.bodyDetail * bodyEdge;
				if (limit > 0.0f && edge.length > limit)
					mark(edge, ReasonBodyDetail, edge.length / limit);
			}
		}
	}

	// Worst first, the same order every time
	std::vector<int> candidates;
	for (size_t e = 0; e < edges.size(); e++)
		if (edges[e].reasons)
			candidates.push_back(static_cast<int>(e));
	std::sort(candidates.begin(), candidates.end(), [&edges](int a, int b) {
		if (edges[a].score != edges[b].score)
			return edges[a].score > edges[b].score;
		return EdgeKey(edges[a].p1, edges[a].p2) < EdgeKey(edges[b].p1, edges[b].p2);
	});

	auto longer = [&edges](int a, int b) {
		if (edges[a].length != edges[b].length)
			return edges[a].length > edges[b].length;
		return EdgeKey(edges[a].p1, edges[a].p2) < EdgeKey(edges[b].p1, edges[b].p2);
	};
	auto longestOf = [&](int t) {
		int longest = -1;
		for (int e : triEdges[t])
			if (e >= 0 && (longest < 0 || longer(e, longest)))
				longest = e;
		return longest;
	};

	// Each edge comes with the longest edges of the triangles next to it, and theirs in turn (Rivara's longest edge
	// refinement): splitting the longest edge of a triangle first keeps its pieces from getting thinner
	std::vector<bool> chosen(edges.size(), false);
	size_t chosenCount = 0;
	for (int candidate : candidates) {
		if (chosen[candidate])
			continue;

		std::vector<int> add;
		std::unordered_set<int> adding;
		std::vector<int> work = {candidate};
		while (!work.empty()) {
			int e = work.back();
			work.pop_back();
			if (chosen[e] || !adding.insert(e).second)
				continue;

			add.push_back(e);
			if (!options.conforming)
				continue;
			for (int t : edges[e].tris) {
				int longest = longestOf(t);
				if (longest >= 0 && longest != e && !chosen[longest] && !adding.count(longest) && splittable(edges[longest]))
					work.push_back(longest);
			}
		}

		if (options.maxEdges && chosenCount + add.size() > options.maxEdges)
			continue;

		for (int e : add) {
			chosen[e] = true;
			if (!edges[e].reasons)
				edges[e].reasons = ReasonConforming;
			selection.edges.push_back({edges[e].p1, edges[e].p2, edges[e].score, edges[e].reasons});
		}
		chosenCount += add.size();
	}

	for (int candidate : candidates)
		if (!chosen[candidate])
			selection.overLimit++;

	for (auto& edge : selection.edges) {
		selection.byLength += (edge.reasons & ReasonLength) ? 1 : 0;
		selection.byCurvature += (edge.reasons & ReasonCurvature) ? 1 : 0;
		selection.byClipping += (edge.reasons & ReasonClipping) ? 1 : 0;
		selection.byBodyDetail += (edge.reasons & ReasonBodyDetail) ? 1 : 0;
		selection.conforming += edge.reasons == ReasonConforming ? 1 : 0;
	}
	return selection;
}

} // namespace MeshRefine
