/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsChains.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <unordered_map>

using namespace nifly;

namespace PhysicsChains {

namespace {

constexpr float Pi = 3.14159265358979f;

float Azimuth(const Vector3& p, const Vector3& axisOrigin) {
	float az = std::atan2(p.x - axisOrigin.x, p.y - axisOrigin.y) * 180.0f / Pi;
	return az < 0.0f ? az + 360.0f : az;
}

std::string SideName(float azimuth) {
	static const char* names[] = {"Front", "FrontR", "R", "BackR", "Back", "BackL", "L", "FrontL"};
	int sector = int(std::floor((azimuth + 22.5f) / 45.0f)) % 8;
	return names[sector];
}

struct GridKey {
	int x, y, z;
	bool operator==(const GridKey& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct GridKeyHash {
	size_t operator()(const GridKey& k) const { return (size_t(k.x) * 73856093u) ^ (size_t(k.y) * 19349663u) ^ (size_t(k.z) * 83492791u); }
};

// A group of hanging vertices that becomes one chain
struct Cluster {
	std::vector<int> vertices;
	float maxDistance = 0.0f;
	Vector3 originCenter;
	float azimuth = 0.0f;
};

void Finish(Cluster& cluster, const std::vector<Vector3>& verts, const std::vector<float>& distance, const std::vector<int>& origin, const Vector3& axisOrigin) {
	cluster.maxDistance = 0.0f;
	Vector3 sum;
	for (int v : cluster.vertices) {
		cluster.maxDistance = std::max(cluster.maxDistance, distance[v]);
		sum += verts[origin[v] >= 0 ? origin[v] : v];
	}
	cluster.originCenter = cluster.vertices.empty() ? Vector3() : sum / float(cluster.vertices.size());
	cluster.azimuth = Azimuth(cluster.originCenter, axisOrigin);
}

// Splits the vertices of a hanging part by the angle around the axis where they start hanging, into k groups
std::vector<std::vector<int>> SplitByAzimuth(const std::vector<int>& part, const std::vector<float>& originAzimuth, int k) {
	// Unwrap the angles from the largest gap, so a part spanning the back (around 180) or the front (around 0)
	// gets continuous values
	std::vector<float> angles;
	angles.reserve(part.size());
	for (int v : part)
		angles.push_back(originAzimuth[v]);
	std::vector<float> sorted = angles;
	std::sort(sorted.begin(), sorted.end());
	float gapStart = sorted.back();
	float largestGap = sorted.front() + 360.0f - sorted.back();
	for (size_t i = 1; i < sorted.size(); i++) {
		if (sorted[i] - sorted[i - 1] > largestGap) {
			largestGap = sorted[i] - sorted[i - 1];
			gapStart = sorted[i - 1];
		}
	}
	for (float& a : angles)
		if (a <= gapStart)
			a += 360.0f;

	std::vector<float> unwrapped = angles;
	std::sort(unwrapped.begin(), unwrapped.end());
	std::vector<float> centers(k);
	for (int c = 0; c < k; c++)
		centers[c] = unwrapped[std::min(unwrapped.size() - 1, size_t((c + 0.5f) / k * unwrapped.size()))];

	std::vector<int> assignment(part.size(), 0);
	for (int iteration = 0; iteration < 20; iteration++) {
		for (size_t i = 0; i < part.size(); i++) {
			int best = 0;
			for (int c = 1; c < k; c++)
				if (std::fabs(angles[i] - centers[c]) < std::fabs(angles[i] - centers[best]))
					best = c;
			assignment[i] = best;
		}
		std::vector<double> sum(k, 0.0);
		std::vector<size_t> count(k, 0);
		for (size_t i = 0; i < part.size(); i++) {
			sum[assignment[i]] += angles[i];
			count[assignment[i]]++;
		}
		for (int c = 0; c < k; c++)
			if (count[c])
				centers[c] = float(sum[c] / count[c]);
	}

	std::vector<std::vector<int>> groups(k);
	for (size_t i = 0; i < part.size(); i++)
		groups[assignment[i]].push_back(part[i]);
	groups.erase(std::remove_if(groups.begin(), groups.end(), [](const std::vector<int>& g) { return g.empty(); }), groups.end());
	return groups;
}

// A connected piece of a band of vertices: its middle and size
struct BandPiece {
	Vector3 center;
	size_t count = 0;
};

// The connected pieces of a band of vertices, largest first; pieces of a few vertices only count when there is
// nothing else
std::vector<BandPiece> BandPieces(const std::vector<int>& band, const std::vector<std::vector<int>>& neighbors, const std::vector<Vector3>& verts) {
	std::unordered_map<int, int> piece;
	for (int v : band)
		piece[v] = -1;
	std::vector<BandPiece> pieces;
	for (int start : band) {
		if (piece[start] >= 0)
			continue;
		const int id = int(pieces.size());
		pieces.emplace_back();
		std::vector<int> stack{start};
		piece[start] = id;
		while (!stack.empty()) {
			int u = stack.back();
			stack.pop_back();
			pieces[id].center += verts[u];
			pieces[id].count++;
			for (int w : neighbors[u]) {
				auto it = piece.find(w);
				if (it != piece.end() && it->second < 0) {
					it->second = id;
					stack.push_back(w);
				}
			}
		}
	}
	for (auto& p : pieces)
		p.center = p.center / float(p.count);
	std::sort(pieces.begin(), pieces.end(), [](const BandPiece& a, const BandPiece& b) { return a.count > b.count; });
	constexpr size_t MinPiece = 6;
	while (pieces.size() > 1 && pieces.back().count < MinPiece)
		pieces.pop_back();
	return pieces;
}

// Angular extent of where the vertices of a part start hanging around the axis
float AzimuthExtent(const std::vector<int>& part, const std::vector<float>& originAzimuth) {
	std::vector<float> sorted;
	sorted.reserve(part.size());
	for (int v : part)
		sorted.push_back(originAzimuth[v]);
	std::sort(sorted.begin(), sorted.end());
	float largestGap = sorted.front() + 360.0f - sorted.back();
	for (size_t i = 1; i < sorted.size(); i++)
		largestGap = std::max(largestGap, sorted[i] - sorted[i - 1]);
	return 360.0f - largestGap;
}

} // namespace

std::vector<std::vector<int>> Neighbors(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float linkDistance) {
	std::vector<std::vector<int>> neighbors(verts.size());
	auto link = [&](int a, int b) {
		if (a == b || a < 0 || b < 0 || size_t(a) >= verts.size() || size_t(b) >= verts.size())
			return;
		auto& na = neighbors[a];
		if (std::find(na.begin(), na.end(), b) == na.end()) {
			na.push_back(b);
			neighbors[b].push_back(a);
		}
	};
	for (auto& t : tris) {
		link(t.p1, t.p2);
		link(t.p2, t.p3);
		link(t.p3, t.p1);
	}

	if (linkDistance <= 0.0f)
		return neighbors;

	// Join vertices closer than linkDistance through a grid, the closest few per vertex
	constexpr size_t MaxLinks = 6;
	const float cell = linkDistance;
	std::unordered_map<GridKey, std::vector<int>, GridKeyHash> grid;
	auto keyOf = [cell](const Vector3& p) { return GridKey{int(std::floor(p.x / cell)), int(std::floor(p.y / cell)), int(std::floor(p.z / cell))}; };
	for (size_t i = 0; i < verts.size(); i++)
		grid[keyOf(verts[i])].push_back(int(i));

	const float maxSq = linkDistance * linkDistance;
	std::vector<std::pair<float, int>> candidates;
	for (size_t i = 0; i < verts.size(); i++) {
		candidates.clear();
		GridKey k = keyOf(verts[i]);
		for (int dx = -1; dx <= 1; dx++)
			for (int dy = -1; dy <= 1; dy++)
				for (int dz = -1; dz <= 1; dz++) {
					auto it = grid.find(GridKey{k.x + dx, k.y + dy, k.z + dz});
					if (it == grid.end())
						continue;
					for (int j : it->second) {
						if (size_t(j) == i)
							continue;
						float d = verts[i].DistanceSquaredTo(verts[j]);
						if (d <= maxSq)
							candidates.emplace_back(d, j);
					}
				}
		if (candidates.size() > MaxLinks) {
			std::partial_sort(candidates.begin(), candidates.begin() + MaxLinks, candidates.end());
			candidates.resize(MaxLinks);
		}
		for (auto& c : candidates)
			link(int(i), c.second);
	}
	return neighbors;
}

Result Detect(const std::vector<Vector3>& verts,
			  const std::vector<std::vector<int>>& neighbors,
			  const std::vector<bool>& roots,
			  const Vector3& axisOrigin,
			  const Options& options,
			  const std::vector<bool>* blocked) {
	Result result;
	const size_t n = verts.size();
	result.distance.assign(n, -1.0f);
	result.chain.assign(n, -1);
	if (n == 0 || neighbors.size() != n || roots.size() != n)
		return result;
	if (blocked && blocked->size() != n)
		blocked = nullptr;
	auto isBlocked = [blocked](size_t v) { return blocked && (*blocked)[v]; };

	// Distance along the mesh from the nearest root, the root it comes from and the first vertex on the way
	// that is past hangStart (where the vertex's strand starts hanging)
	std::vector<int> source(n, -1);
	std::vector<int> origin(n, -1);
	using Entry = std::pair<float, int>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
	for (size_t v = 0; v < n; v++) {
		if (roots[v] && !isBlocked(v)) {
			result.distance[v] = 0.0f;
			source[v] = int(v);
			queue.emplace(0.0f, int(v));
			result.rootVertices++;
		}
	}
	while (!queue.empty()) {
		auto [d, u] = queue.top();
		queue.pop();
		if (d > result.distance[u])
			continue;
		for (int w : neighbors[u]) {
			if (isBlocked(w))
				continue;
			float nd = d + verts[u].DistanceTo(verts[w]);
			if (result.distance[w] >= 0.0f && nd >= result.distance[w])
				continue;
			result.distance[w] = nd;
			source[w] = source[u];
			if (d >= options.hangStart)
				origin[w] = origin[u];
			else
				origin[w] = nd >= options.hangStart ? w : -1;
			queue.emplace(nd, w);
		}
	}

	std::vector<float> originAzimuth(n, 0.0f);
	std::vector<bool> hanging(n, false);
	for (size_t v = 0; v < n; v++) {
		if (result.distance[v] < 0.0f) {
			result.unreachableVertices++;
			continue;
		}
		if (result.distance[v] >= options.hangStart) {
			hanging[v] = true;
			result.hangingVertices++;
			if (origin[v] < 0)
				origin[v] = int(v);
			originAzimuth[v] = Azimuth(verts[origin[v]], axisOrigin);
		}
	}

	// Connected hanging parts
	std::vector<Cluster> clusters;
	std::vector<bool> visited(n, false);
	for (size_t start = 0; start < n; start++) {
		if (!hanging[start] || visited[start])
			continue;
		std::vector<int> part;
		std::vector<int> stack{int(start)};
		visited[start] = true;
		while (!stack.empty()) {
			int u = stack.back();
			stack.pop_back();
			part.push_back(u);
			for (int w : neighbors[u]) {
				if (hanging[w] && !visited[w]) {
					visited[w] = true;
					stack.push_back(w);
				}
			}
		}

		float maxDistance = 0.0f;
		for (int v : part)
			maxDistance = std::max(maxDistance, result.distance[v]);
		if (maxDistance < options.minHang || part.size() < size_t(std::max(1, options.minVertices))) {
			result.shortParts++;
			continue;
		}
		// A part whose tip runs into vertices an earlier pass chained is the upper end of that strand (a lock made
		// of stacked pieces): a chain of its own would pull the strand apart where the two meet
		if (blocked) {
			bool continues = false;
			for (int v : part) {
				if (result.distance[v] < maxDistance - options.boneSpacing * 0.5f)
					continue;
				for (int w : neighbors[v])
					if ((*blocked)[w]) {
						continues = true;
						break;
					}
				if (continues)
					break;
			}
			if (continues) {
				result.continuedParts++;
				continue;
			}
		}

		// Split parts that start hanging over a wide angle around the head (open hair over the back and
		// the shoulders) into groups that are long and large enough on their own
		const float extent = AzimuthExtent(part, originAzimuth);
		int k = std::max(1, int(std::ceil(extent / std::max(10.0f, options.sectorAngle))));
		std::vector<std::vector<int>> groups;
		for (; k >= 1; k--) {
			groups = k > 1 ? SplitByAzimuth(part, originAzimuth, k) : std::vector<std::vector<int>>{part};
			bool valid = true;
			std::vector<Vector3> tips;
			for (auto& g : groups) {
				float groupMax = 0.0f;
				for (int v : g)
					groupMax = std::max(groupMax, result.distance[v]);
				if (groupMax < options.minHang || g.size() < size_t(std::max(1, options.minVertices)))
					valid = false;
				Vector3 tip;
				size_t count = 0;
				for (int v : g)
					if (result.distance[v] >= groupMax - options.boneSpacing * 0.5f) {
						tip += verts[v];
						count++;
					}
				tips.push_back(count ? tip / float(count) : tip);
			}
			// Groups that come together again at their tips are one strand
			for (size_t a = 0; a < tips.size() && valid && groups.size() > 1; a++)
				for (size_t b = a + 1; b < tips.size(); b++)
					if (tips[a].DistanceTo(tips[b]) < options.minTipGap)
						valid = false;
			if (valid)
				break;
		}

		for (auto& g : groups) {
			Cluster cluster;
			cluster.vertices = std::move(g);
			Finish(cluster, verts, result.distance, origin, axisOrigin);
			if (!options.sides.empty() && std::find(options.sides.begin(), options.sides.end(), SideName(cluster.azimuth)) == options.sides.end()) {
				result.otherSides++;
				continue;
			}
			// The tip: the vertices within a joint spacing of the farthest
			float tipZ = 0.0f;
			size_t tipCount = 0;
			for (int v : cluster.vertices) {
				if (result.distance[v] >= cluster.maxDistance - options.boneSpacing * 0.5f) {
					tipZ += verts[v].z;
					tipCount++;
				}
			}
			const float span = cluster.maxDistance - options.hangStart;
			if (tipCount && span > 0.0f && cluster.originCenter.z - tipZ / tipCount < options.minDrop * span) {
				result.flatParts++;
				continue;
			}
			clusters.push_back(std::move(cluster));
		}
	}

	// Too many: merge the smallest into the one starting nearest to it
	const size_t maxChains = size_t(std::max(1, options.maxChains));
	while (clusters.size() > maxChains) {
		size_t smallest = 0;
		for (size_t c = 1; c < clusters.size(); c++)
			if (clusters[c].vertices.size() < clusters[smallest].vertices.size())
				smallest = c;
		size_t nearest = smallest == 0 ? 1 : 0;
		for (size_t c = 0; c < clusters.size(); c++) {
			if (c == smallest)
				continue;
			if (clusters[c].originCenter.DistanceTo(clusters[smallest].originCenter) < clusters[nearest].originCenter.DistanceTo(clusters[smallest].originCenter))
				nearest = c;
		}
		auto& into = clusters[nearest].vertices;
		into.insert(into.end(), clusters[smallest].vertices.begin(), clusters[smallest].vertices.end());
		Finish(clusters[nearest], verts, result.distance, origin, axisOrigin);
		clusters.erase(clusters.begin() + smallest);
		result.mergedParts++;
	}

	// Front first, then around the right side to the back and the left
	std::sort(clusters.begin(), clusters.end(), [](const Cluster& a, const Cluster& b) { return a.azimuth < b.azimuth; });

	// Each chain starts where its part starts hanging: hangStart, or lower under the hang ceiling
	for (size_t c = 0; c < clusters.size(); c++) {
		Cluster& cluster = clusters[c];
		Chain chain;
		chain.azimuth = cluster.azimuth;
		chain.side = SideName(cluster.azimuth);
		chain.length = cluster.maxDistance;
		chain.vertexCount = cluster.vertices.size();

		// Only the part under the hang ceiling moves; the chain starts where that part starts
		const float ceiling = axisOrigin.z + options.hangCeiling;
		std::vector<int> moving;
		float hangStart = 1e9f;
		for (int v : cluster.vertices) {
			if (verts[v].z <= ceiling) {
				moving.push_back(v);
				hangStart = std::min(hangStart, result.distance[v]);
			}
		}
		if (cluster.maxDistance - hangStart < options.boneSpacing * 0.5f || moving.size() < size_t(std::max(1, options.minVertices))) {
			result.shortParts++;
			continue;
		}
		// The part that moves has to go down as well: below a hang ceiling, the side hair of a short cut often runs
		// back around the head, and a chain on it would swing down and stretch it
		if (options.hangCeiling < 1e8f) {
			Vector3 top, tip;
			size_t topCount = 0, tipCount = 0;
			for (int v : moving) {
				if (result.distance[v] <= hangStart + 1.0f) {
					top += verts[v];
					topCount++;
				}
				if (result.distance[v] >= cluster.maxDistance - options.boneSpacing * 0.5f) {
					tip += verts[v];
					tipCount++;
				}
			}
			const float span = cluster.maxDistance - hangStart;
			if (topCount && tipCount && top.z / topCount - tip.z / tipCount < options.minDrop * span) {
				result.flatParts++;
				continue;
			}
		}
		cluster.vertices = std::move(moving);
		const float span = std::max(0.0f, cluster.maxDistance - hangStart);
		const float baseSpacing = std::max(0.5f, options.boneSpacing);
		int joints = std::clamp(int(std::ceil(span / baseSpacing)), 1, std::max(1, options.maxBones));
		const float spacing = std::max(baseSpacing, joints > 0 ? span / std::max(1, options.maxBones) : baseSpacing);

		// Root on the attachment: where the roots the strand comes from are
		Vector3 rootSum;
		size_t rootCount = 0;
		for (int v : cluster.vertices) {
			int o = origin[v] >= 0 ? origin[v] : v;
			if (source[o] >= 0) {
				rootSum += verts[source[o]];
				rootCount++;
			}
		}
		chain.joints.push_back(rootCount ? rootSum / float(rootCount) : cluster.originCenter);
		chain.jointDistance.push_back(0.0f);

		// The joints at the middle of bands of the strand's vertices around their distance: the first where the
		// strand starts hanging, each following one connected piece of the band, the one continuing from the joint
		// above, so a chain over two locks stays in one of them instead of zig-zagging between
		for (int j = 0; j < joints; j++) {
			const float s = hangStart + j * spacing;
			const float from = j == 0 ? hangStart : s - spacing * 0.5f;
			const float to = s + spacing * 0.5f;
			std::vector<int> band;
			for (int v : cluster.vertices)
				if (result.distance[v] >= from && result.distance[v] <= to)
					band.push_back(v);
			if (band.empty())
				break;
			auto pieces = BandPieces(band, neighbors, verts);
			// The largest piece first, else the one nearest the joint above among those of a similar size (locks),
			// never a small wisp at the edge of the hair
			// Pieces whose middles lie close together are one strand split in the band (a braid's twists, layered
			// cards): the joint goes in the middle of the band. Only pieces apart are separate locks.
			Vector3 middle;
			size_t total = 0;
			float spread = 0.0f;
			for (auto& piece : pieces) {
				middle += piece.center * float(piece.count);
				total += piece.count;
			}
			middle = total ? middle / float(total) : pieces[0].center;
			for (auto& piece : pieces)
				if (piece.count * 4 >= pieces[0].count)
					spread = std::max(spread, piece.center.DistanceTo(middle));
			if (spread <= options.lockGap) {
				if (j > 0 && options.minJointDrop > 0.0f && chain.joints.size() > 1 &&
					chain.joints.back().z - middle.z < options.minJointDrop * spacing)
					break;
				chain.joints.push_back(middle);
				chain.jointDistance.push_back(s);
				continue;
			}
			const BandPiece* chosen = &pieces[0];
			if (j > 0)
				for (auto& piece : pieces)
					if (piece.count * 4 >= pieces[0].count &&
						piece.center.DistanceTo(chain.joints.back()) < chosen->center.DistanceTo(chain.joints.back()))
						chosen = &piece;
			// A joint that doesn't go down far enough ends the chain: that part of the strand runs around the head,
			// and a joint there would swing down under gravity and stretch what hangs on it
			if (j > 0 && options.minJointDrop > 0.0f && chain.joints.size() > 1 &&
				chain.joints.back().z - chosen->center.z < options.minJointDrop * spacing)
				break;
			chain.joints.push_back(chosen->center);
			chain.jointDistance.push_back(s);
		}
		if (chain.joints.size() < 2) {
			chain.joints.push_back(cluster.originCenter);
			chain.jointDistance.push_back(hangStart);
		}

		chain.vertexCount = cluster.vertices.size();
		for (int v : cluster.vertices)
			result.chain[v] = int(result.chains.size());
		result.chains.push_back(std::move(chain));
	}

	return result;
}

std::vector<VertexWeights> JointWeights(const Result& result, const Options& options) {
	std::vector<VertexWeights> weights(result.distance.size());

	// Where each segment's middle is: a vertex there follows the joint at the segment's upper end fully. The
	// first center is where the chain starts hanging, where vertices still keep their bones.
	std::vector<std::vector<float>> centers(result.chains.size());
	for (size_t c = 0; c < result.chains.size(); c++) {
		const Chain& chain = result.chains[c];
		auto& centersOfChain = centers[c];
		centersOfChain.push_back(chain.jointDistance.size() > 1 ? chain.jointDistance[1] : options.hangStart);
		for (size_t j = 1; j < chain.jointDistance.size(); j++) {
			float segment;
			if (j + 1 < chain.jointDistance.size())
				segment = chain.jointDistance[j + 1] - chain.jointDistance[j];
			else
				segment = std::min(std::max(0.5f, options.boneSpacing), std::max(0.5f, chain.length - chain.jointDistance[j]));
			centersOfChain.push_back(chain.jointDistance[j] + segment * 0.5f);
		}
	}

	for (size_t v = 0; v < result.distance.size(); v++) {
		int c = result.chain[v];
		if (c < 0)
			continue;
		const auto& cc = centers[c];
		const float s = result.distance[v];
		VertexWeights& w = weights[v];
		if (s <= cc.front())
			continue;
		if (s >= cc.back()) {
			w.keep = 0.0f;
			w.joints.emplace_back(int(cc.size() - 1), 1.0f);
			continue;
		}
		for (size_t i = 0; i + 1 < cc.size(); i++) {
			if (s >= cc[i] && s < cc[i + 1]) {
				const float t = (s - cc[i]) / std::max(1e-5f, cc[i + 1] - cc[i]);
				if (i == 0) {
					w.keep = 1.0f - t;
				}
				else {
					w.keep = 0.0f;
					w.joints.emplace_back(int(i), 1.0f - t);
				}
				w.joints.emplace_back(int(i + 1), t);
				break;
			}
		}
	}
	return weights;
}

} // namespace PhysicsChains
