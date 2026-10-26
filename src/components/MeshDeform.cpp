/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MeshDeform.h"
#include "ClippingFixer.h"
#include "MeshAnalysis.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <functional>
#include <queue>
#include <unordered_map>

using namespace nifly;

namespace MeshDeform {

namespace {

bool ValidTriangle(const Triangle& tri, size_t count) {
	return tri.p1 < count && tri.p2 < count && tri.p3 < count;
}

// Neighbors of each vertex along the triangle edges, and the copies of vertices along split seams
struct Topology {
	std::vector<std::vector<int>> neighbors;
	std::vector<std::vector<int>> welds;
	std::vector<int> weldOf; // Index into welds, or -1
};

Topology BuildTopology(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float weldDistance) {
	Topology topology;
	const size_t count = verts.size();
	topology.neighbors.resize(count);
	for (auto& tri : tris) {
		if (!ValidTriangle(tri, count))
			continue;
		const int corners[3] = {tri.p1, tri.p2, tri.p3};
		for (int a = 0; a < 3; a++)
			for (int b = 0; b < 3; b++)
				if (a != b)
					topology.neighbors[corners[a]].push_back(corners[b]);
	}

	// Sorted, so the sums of the smoothing don't depend on the order of the triangles
	for (auto& list : topology.neighbors) {
		std::sort(list.begin(), list.end());
		list.erase(std::unique(list.begin(), list.end()), list.end());
	}

	topology.welds = MeshAnalysis::CoincidentGroups(verts, weldDistance);
	topology.weldOf.assign(count, -1);
	for (size_t g = 0; g < topology.welds.size(); g++)
		for (int v : topology.welds[g])
			topology.weldOf[v] = static_cast<int>(g);
	return topology;
}

// Gives the copies of each seam vertex in the list their average, so seams don't open
template<typename T>
void AverageWelds(const Topology& topology, const std::vector<int>& vertices, std::vector<T>& values) {
	std::vector<char> done(topology.welds.size(), 0);
	for (int v : vertices) {
		const int group = topology.weldOf[v];
		if (group < 0 || done[group])
			continue;
		done[group] = 1;

		T sum = T();
		for (int copy : topology.welds[group])
			sum = sum + values[copy];
		const T average = sum * (1.0f / static_cast<float>(topology.welds[group].size()));
		for (int copy : topology.welds[group])
			values[copy] = average;
	}
}

// Vertices within a radius, by a grid of cells as large as the radius
class PointGrid {
public:
	PointGrid(const std::vector<Vector3>& inPoints, float inCell)
		: points(inPoints)
		, cell(std::max(inCell, 1e-4f)) {
		for (size_t i = 0; i < points.size(); i++)
			cells[Key(Cell(points[i].x), Cell(points[i].y), Cell(points[i].z))].push_back(static_cast<int>(i));
	}

	void ForEachWithin(const Vector3& p, float radius, const std::function<void(int, float)>& visit) const {
		const float radiusSq = radius * radius;
		const int64_t cx = Cell(p.x), cy = Cell(p.y), cz = Cell(p.z);
		const int64_t reach = static_cast<int64_t>(std::ceil(radius / cell));
		for (int64_t x = cx - reach; x <= cx + reach; x++)
			for (int64_t y = cy - reach; y <= cy + reach; y++)
				for (int64_t z = cz - reach; z <= cz + reach; z++) {
					auto it = cells.find(Key(x, y, z));
					if (it == cells.end())
						continue;
					for (int i : it->second) {
						const float distSq = p.DistanceSquaredTo(points[i]);
						if (distSq <= radiusSq)
							visit(i, std::sqrt(distSq));
					}
				}
	}

private:
	int64_t Cell(float value) const { return static_cast<int64_t>(std::floor(value / cell)); }
	static uint64_t Key(int64_t x, int64_t y, int64_t z) {
		return (static_cast<uint64_t>(x & 0x1FFFFF) << 42) | (static_cast<uint64_t>(y & 0x1FFFFF) << 21) | static_cast<uint64_t>(z & 0x1FFFFF);
	}

	const std::vector<Vector3>& points;
	float cell;
	std::unordered_map<uint64_t, std::vector<int>> cells;
};

} // namespace

UntangleResult UntangleFlips(const std::vector<Vector3>& before,
							 std::vector<Vector3>& after,
							 const std::vector<Triangle>& tris,
							 const std::vector<float>* freedom,
							 const UntangleOptions& options) {
	UntangleResult result;
	const size_t count = before.size();
	if (count == 0 || after.size() != count)
		return result;

	std::vector<size_t> flipped = MeshAnalysis::FlippedTriangles(before, after, tris);
	result.flippedBefore = result.flippedAfter = flipped.size();
	if (flipped.empty())
		return result;

	const Topology topology = BuildTopology(before, tris, options.weldDistance);
	auto freedomOf = [freedom](size_t i) { return freedom && i < freedom->size() ? std::clamp((*freedom)[i], 0.0f, 1.0f) : 1.0f; };
	const float strength = std::clamp(options.strength, 0.01f, 1.0f);
	const int iterations = std::max(options.iterations, 1);

	std::vector<Vector3> change(count);
	for (size_t i = 0; i < count; i++)
		change[i] = after[i] - before[i];

	std::vector<Vector3> current = after;
	std::vector<Vector3> best = after;
	size_t bestCount = flipped.size();

	for (int ring = 1; ring <= std::max(options.maxRings, 1) && !flipped.empty(); ring++) {
		// The corners of the flipped triangles and the rings of neighbors around them, with their seam copies
		std::vector<char> inRegion(count, 0);
		std::vector<int> layer;
		auto add = [&](int v) {
			const int group = topology.weldOf[v];
			if (group < 0) {
				if (!inRegion[v]) {
					inRegion[v] = 1;
					layer.push_back(v);
				}
				return;
			}
			for (int copy : topology.welds[group])
				if (!inRegion[copy]) {
					inRegion[copy] = 1;
					layer.push_back(copy);
				}
		};
		for (size_t t : flipped) {
			add(tris[t].p1);
			add(tris[t].p2);
			add(tris[t].p3);
		}
		for (int r = 0; r < ring; r++) {
			std::vector<int> previous;
			previous.swap(layer);
			for (int v : previous)
				for (int neighbor : topology.neighbors[v])
					add(neighbor);
		}

		// A seam vertex with a locked copy stays with all of its copies, or the seam would open
		auto locked = [&](size_t i) {
			const int group = topology.weldOf[i];
			if (group < 0)
				return freedomOf(i) <= 0.0f;
			for (int copy : topology.welds[group])
				if (freedomOf(copy) <= 0.0f)
					return true;
			return false;
		};
		std::vector<int> region;
		for (size_t i = 0; i < count; i++)
			if (inRegion[i] && !locked(i))
				region.push_back(static_cast<int>(i));
		if (region.empty())
			break;

		// The change of the region relaxes towards the average of its neighbors, the vertices around it hold it in place
		std::vector<Vector3> next = change;
		for (int it = 0; it < iterations; it++) {
			for (int v : region) {
				const auto& neighbors = topology.neighbors[v];
				if (neighbors.empty())
					continue;
				Vector3 average;
				for (int neighbor : neighbors)
					average += change[neighbor];
				average /= static_cast<float>(neighbors.size());
				next[v] = change[v] + (average - change[v]) * (strength * freedomOf(v));
			}
			AverageWelds(topology, region, next);
			for (int v : region)
				change[v] = next[v];

			// Stop as soon as nothing is flipped, so the fix changes no more than it has to
			if ((it + 1) % 10 == 0 || it + 1 == iterations) {
				for (int v : region)
					current[v] = before[v] + change[v];
				if (MeshAnalysis::CountFlippedTriangles(before, current, tris) == 0)
					break;
			}
		}

		for (int v : region)
			current[v] = before[v] + change[v];
		flipped = MeshAnalysis::FlippedTriangles(before, current, tris);
		if (flipped.size() < bestCount) {
			bestCount = flipped.size();
			best = current;
			result.rings = ring;
		}
	}

	if (bestCount >= result.flippedBefore)
		return result;

	for (size_t i = 0; i < count; i++) {
		const float move = best[i].DistanceTo(after[i]);
		if (move > 1e-6f) {
			result.movedVertices++;
			result.maxMove = std::max(result.maxMove, move);
		}
	}
	after = best;
	result.flippedAfter = bestCount;
	return result;
}

ContactMorph MakeContactMorph(const std::vector<Vector3>& bodyVerts,
							  const std::vector<Triangle>& bodyTris,
							  const std::vector<Vector3>& coverVerts,
							  const std::vector<Triangle>& coverTris,
							  const std::vector<float>* freedom,
							  const ContactMorphOptions& options) {
	ContactMorph morph;
	const size_t count = bodyVerts.size();
	morph.offsets.assign(count, 0.0f);
	morph.deltas.assign(count, Vector3());
	morph.edgeDistance.assign(count, -FLT_MAX);
	if (count == 0 || coverVerts.empty() || coverTris.empty())
		return morph;

	const Topology topology = BuildTopology(bodyVerts, bodyTris, options.weldDistance);
	std::vector<int> all(count);
	for (size_t i = 0; i < count; i++)
		all[i] = static_cast<int>(i);

	// The direction the body moves in: its outward normals, smoothed so a small bump like a nipple moves back as a
	// whole instead of spreading out along its own normals
	std::vector<Vector3> normals = ClippingFixer::OutwardVertexNormals(bodyVerts, bodyTris);
	AverageWelds(topology, all, normals);
	std::vector<Vector3> smoothed = normals;
	for (int pass = 0; pass < 10; pass++) {
		for (size_t v = 0; v < count; v++) {
			Vector3 sum = normals[v];
			for (int neighbor : topology.neighbors[v])
				sum += normals[neighbor];
			smoothed[v] = sum / static_cast<float>(topology.neighbors[v].size() + 1);
		}
		AverageWelds(topology, all, smoothed);
		normals.swap(smoothed);
	}
	for (auto& normal : normals)
		normal.Normalize();

	// Under the cover: close to its surface, which includes a margin of contactDistance around its edge where the
	// body barely moves, or with the cover right in front of it where the cover tents over the body, e.g. a cup
	// around the nipple. Copies of a seam vertex are covered together.
	const float contact = std::max(options.contactDistance, 1e-4f);
	const float tentReach = std::max(options.tentDistance, contact);
	std::vector<MeshAnalysis::SurfacePoint> closest = MeshAnalysis::ClosestSurfacePoints(coverVerts, coverTris, bodyVerts, tentReach);
	std::vector<float> covered(count, 0.0f);
	std::vector<int> tentCandidates;
	for (size_t i = 0; i < count; i++) {
		const MeshAnalysis::SurfacePoint& hit = closest[i];
		if (hit.triangle < 0)
			continue;
		if (hit.distance <= contact)
			covered[i] = 1.0f;
		else
			tentCandidates.push_back(static_cast<int>(i));
	}

	// The cover in front: a line out along the normal meets one of its triangles within the tent distance
	if (!tentCandidates.empty()) {
		std::vector<Vector3> centers;
		for (auto& tri : coverTris)
			centers.push_back(ValidTriangle(tri, coverVerts.size()) ? (coverVerts[tri.p1] + coverVerts[tri.p2] + coverVerts[tri.p3]) / 3.0f : Vector3());
		float largest = 0.0f;
		for (size_t t = 0; t < coverTris.size(); t++)
			if (ValidTriangle(coverTris[t], coverVerts.size()))
				for (uint16_t corner : {coverTris[t].p1, coverTris[t].p2, coverTris[t].p3})
					largest = std::max(largest, centers[t].DistanceTo(coverVerts[corner]));
		PointGrid triangleGrid(centers, tentReach + largest);
		for (int i : tentCandidates) {
			const Vector3& origin = bodyVerts[i];
			const Vector3& direction = normals[i];
			bool hit = false;
			triangleGrid.ForEachWithin(origin, tentReach + largest, [&](int t, float) {
				const Triangle& tri = coverTris[t];
				if (hit || !ValidTriangle(tri, coverVerts.size()))
					return;
				const Vector3 e1 = coverVerts[tri.p2] - coverVerts[tri.p1];
				const Vector3 e2 = coverVerts[tri.p3] - coverVerts[tri.p1];
				const Vector3 p = direction.cross(e2);
				const float det = e1.dot(p);
				if (std::fabs(det) < 1e-9f)
					return;
				const float inv = 1.0f / det;
				const Vector3 s = origin - coverVerts[tri.p1];
				const float u = s.dot(p) * inv;
				if (u < 0.0f || u > 1.0f)
					return;
				const Vector3 q = s.cross(e1);
				const float v = direction.dot(q) * inv;
				if (v < 0.0f || u + v > 1.0f)
					return;
				const float along = e2.dot(q) * inv;
				hit = along > 0.0f && along <= tentReach;
			});
			if (hit)
				covered[i] = 1.0f;
		}
	}
	std::vector<float> coveredCopies = covered;
	AverageWelds(topology, all, coveredCopies);
	for (size_t i = 0; i < count; i++) {
		covered[i] = coveredCopies[i] > 0.0f ? 1.0f : 0.0f;
		if (covered[i] > 0.0f)
			morph.covered++;
	}
	if (morph.covered == 0)
		return morph;

	// Distance along the surface to the edge of the covered area, which crosses the edges between covered and free
	// vertices halfway. Seam copies are neighbors at no distance.
	const float depthWidth = std::max(options.depthWidth, 1e-3f);
	const float bulgeWidth = std::max(options.bulgeWidth, 1e-3f);
	const float reach = std::max(depthWidth * 3.0f, bulgeWidth * 5.0f);
	std::vector<float> distance(count, FLT_MAX);
	using Entry = std::pair<float, int>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
	for (size_t a = 0; a < count; a++)
		for (int b : topology.neighbors[a])
			if (covered[a] != covered[b]) {
				const float half = bodyVerts[a].DistanceTo(bodyVerts[b]) * 0.5f;
				if (half < distance[a]) {
					distance[a] = half;
					queue.push({half, static_cast<int>(a)});
				}
			}
	while (!queue.empty()) {
		const float dist = queue.top().first;
		const int v = queue.top().second;
		queue.pop();
		if (dist > distance[v] || dist > reach)
			continue;

		auto relax = [&](int to, float length) {
			const float next = dist + length;
			if (next < distance[to]) {
				distance[to] = next;
				queue.push({next, to});
			}
		};
		for (int neighbor : topology.neighbors[v])
			relax(neighbor, bodyVerts[v].DistanceTo(bodyVerts[neighbor]));
		if (topology.weldOf[v] >= 0)
			for (int copy : topology.welds[topology.weldOf[v]])
				relax(copy, 0.0f);
	}

	// Sinks in under the cover, reaching the full depth inside
	for (size_t i = 0; i < count; i++) {
		const bool under = covered[i] > 0.0f;
		const float dist = distance[i];
		morph.edgeDistance[i] = dist == FLT_MAX ? (under ? FLT_MAX : -FLT_MAX) : (under ? dist : -dist);
		if (under) {
			const float u = dist == FLT_MAX ? FLT_MAX : dist / depthWidth;
			morph.offsets[i] = u == FLT_MAX ? -options.depth : -options.depth * (1.0f - std::exp(-u * u));
		}
	}

	// Swells out past the edge, as much as the covered body next to it sank: a thin strap that barely presses in
	// pushes little out
	if (options.depth > 0.0f && options.bulge != 0.0f) {
		std::vector<int> coveredList;
		std::vector<Vector3> coveredVerts;
		for (size_t i = 0; i < count; i++)
			if (covered[i] > 0.0f) {
				coveredList.push_back(static_cast<int>(i));
				coveredVerts.push_back(bodyVerts[i]);
			}
		// Far enough in to reach the depth the covered area has, not only its shallow edge
		const float nearRadius = (depthWidth + bulgeWidth) * 2.0f;
		PointGrid grid(coveredVerts, nearRadius);
		for (size_t i = 0; i < count; i++) {
			if (covered[i] > 0.0f || distance[i] == FLT_MAX)
				continue;
			float sank = 0.0f;
			grid.ForEachWithin(bodyVerts[i], nearRadius, [&](int c, float) { sank = std::max(sank, -morph.offsets[coveredList[c]]); });
			const float x = distance[i] / bulgeWidth;
			morph.offsets[i] = options.bulge * std::min(sank / options.depth, 1.0f) * x * x * std::exp(2.0f * (1.0f - x));
		}
	}

	// Smoothed over the surface, which rounds the turn at the edge
	std::vector<float> next = morph.offsets;
	for (int pass = 0; pass < options.smoothing; pass++) {
		for (size_t v = 0; v < count; v++) {
			const auto& neighbors = topology.neighbors[v];
			if (neighbors.empty())
				continue;
			float average = 0.0f;
			for (int neighbor : neighbors)
				average += morph.offsets[neighbor];
			average /= static_cast<float>(neighbors.size());
			next[v] = morph.offsets[v] + (average - morph.offsets[v]) * 0.5f;
		}
		AverageWelds(topology, all, next);
		morph.offsets = next;
	}

	// The swell only rises as far as the cover leaves room: none right next to it, where the cover's edge flares away
	// from the body and the body would push through between its vertices, all of it where the cover is far
	if (options.bulge > 0.0f) {
		const std::vector<MeshAnalysis::SurfacePoint> room = MeshAnalysis::ClosestSurfacePoints(coverVerts, coverTris, bodyVerts, contact + options.bulge);
		for (size_t i = 0; i < count; i++)
			if (morph.offsets[i] > 0.0f && room[i].triangle >= 0)
				morph.offsets[i] = std::min(morph.offsets[i], std::max(room[i].distance - contact, 0.0f));
	}

	for (size_t i = 0; i < count; i++) {
		if (freedom && i < freedom->size())
			morph.offsets[i] *= std::clamp((*freedom)[i], 0.0f, 1.0f);
		// The faint tails of the smoothing far away aren't worth keeping as slider data
		if (std::fabs(morph.offsets[i]) < 0.002f)
			morph.offsets[i] = 0.0f;

		morph.deltas[i] = normals[i] * morph.offsets[i];
		morph.maxDepth = std::max(morph.maxDepth, -morph.offsets[i]);
		morph.maxBulge = std::max(morph.maxBulge, morph.offsets[i]);
	}
	return morph;
}

std::vector<Vector3> FollowDeltas(const std::vector<Vector3>& bodyVerts,
								  const std::vector<Triangle>& bodyTris,
								  const std::vector<Vector3>& bodyDeltas,
								  const std::vector<Vector3>& points,
								  const std::vector<Triangle>& pointTris,
								  float radius) {
	std::vector<Vector3> deltas(points.size());
	if (bodyVerts.empty() || bodyDeltas.size() != bodyVerts.size() || radius <= 0.0f)
		return deltas;

	std::vector<MeshAnalysis::SurfacePoint> closest = MeshAnalysis::ClosestSurfacePoints(bodyVerts, bodyTris, points, radius);
	for (size_t p = 0; p < points.size(); p++) {
		const MeshAnalysis::SurfacePoint& hit = closest[p];
		if (hit.triangle < 0 || static_cast<size_t>(hit.triangle) >= bodyTris.size())
			continue;
		const Triangle& tri = bodyTris[hit.triangle];
		if (!ValidTriangle(tri, bodyVerts.size()))
			continue;

		// Full motion up to three quarters of the radius from the body, fading out to none at the radius
		const float t = std::clamp((hit.distance - radius * 0.75f) / (radius * 0.25f), 0.0f, 1.0f);
		const float fade = 1.0f - t * t * (3.0f - 2.0f * t);
		deltas[p] = (bodyDeltas[tri.p1] * hit.u + bodyDeltas[tri.p2] * hit.v + bodyDeltas[tri.p3] * hit.w) * fade;
	}

	// Smoothed over the points' own mesh, so a piece that sits partly on the body and partly away from it, like a cup
	// that tents over the breast, moves as a whole instead of crumpling
	if (!pointTris.empty()) {
		const Topology topology = BuildTopology(points, pointTris, 1e-4f);
		std::vector<int> all(points.size());
		for (size_t i = 0; i < points.size(); i++)
			all[i] = static_cast<int>(i);
		std::vector<Vector3> next = deltas;
		for (int pass = 0; pass < 2; pass++) {
			for (size_t v = 0; v < points.size(); v++) {
				const auto& neighbors = topology.neighbors[v];
				if (neighbors.empty())
					continue;
				Vector3 average;
				for (int neighbor : neighbors)
					average += deltas[neighbor];
				average /= static_cast<float>(neighbors.size());
				next[v] = deltas[v] + (average - deltas[v]) * 0.5f;
			}
			AverageWelds(topology, all, next);
			deltas = next;
		}
	}

	// Where the body swells next to a point that moved with a part that sank, the point would end up closer to the body
	// than it was, or inside it: it goes back out to its old gap, along the morphed body's normal there
	std::vector<Vector3> morphed = bodyVerts;
	for (size_t i = 0; i < morphed.size(); i++)
		morphed[i] += bodyDeltas[i];
	const std::vector<float> gapBefore = ClippingFixer::SignedDistancesToBody(bodyVerts, bodyTris, points, radius);
	const std::vector<Vector3> morphedNormals = ClippingFixer::OutwardVertexNormals(morphed, bodyTris);
	for (int pass = 0; pass < 3; pass++) {
		std::vector<Vector3> moved = points;
		for (size_t p = 0; p < points.size(); p++)
			moved[p] += deltas[p];
		const std::vector<float> gapAfter = ClippingFixer::SignedDistancesToBody(morphed, bodyTris, moved, radius);
		const std::vector<MeshAnalysis::SurfacePoint> closestAfter = MeshAnalysis::ClosestSurfacePoints(morphed, bodyTris, moved, radius);

		bool pushed = false;
		for (size_t p = 0; p < points.size(); p++) {
			if (gapBefore[p] == FLT_MAX || gapAfter[p] == FLT_MAX || deltas[p].IsZero(true))
				continue;
			const float lost = gapBefore[p] - gapAfter[p];
			const MeshAnalysis::SurfacePoint& hit = closestAfter[p];
			if (lost <= 1e-3f || hit.triangle < 0)
				continue;
			const Triangle& tri = bodyTris[hit.triangle];
			Vector3 normal = morphedNormals[tri.p1] * hit.u + morphedNormals[tri.p2] * hit.v + morphedNormals[tri.p3] * hit.w;
			normal.Normalize();
			deltas[p] += normal * lost;
			pushed = true;
		}
		if (!pushed)
			break;
	}
	return deltas;
}

} // namespace MeshDeform
