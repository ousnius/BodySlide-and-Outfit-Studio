#include "../src/components/MeshAnalysis.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <set>

using nifly::Triangle;
using nifly::Vector3;

namespace {
// Closed axis aligned box from -size to +size, triangles wound so normals point outwards
void MakeBox(float size, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts = {
		{-size, -size, -size}, {size, -size, -size}, {size, size, -size}, {-size, size, -size},
		{-size, -size, size},  {size, -size, size},  {size, size, size},  {-size, size, size},
	};
	tris = {
		{0, 2, 1}, {0, 3, 2}, // -z
		{4, 5, 6}, {4, 6, 7}, // +z
		{0, 1, 5}, {0, 5, 4}, // -y
		{3, 7, 6}, {3, 6, 2}, // +y
		{0, 4, 7}, {0, 7, 3}, // -x
		{1, 2, 6}, {1, 6, 5}, // +x
	};
}

// Flat grid in the XY plane with (n + 1) x (n + 1) vertices spaced by step
void MakeGrid(int n, float step, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts.clear();
	tris.clear();
	for (int y = 0; y <= n; y++)
		for (int x = 0; x <= n; x++)
			verts.emplace_back(x * step, y * step, 0.0f);

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			uint16_t i = static_cast<uint16_t>(y * (n + 1) + x);
			uint16_t right = i + 1;
			uint16_t up = static_cast<uint16_t>(i + n + 1);
			tris.emplace_back(i, right, up);
			tris.emplace_back(right, static_cast<uint16_t>(up + 1), up);
		}
	}
}
} // namespace

TEST_CASE("Penetration depth is measured from the body surface", "[MeshAnalysis]") {
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	MakeBox(1.0f, bodyVerts, bodyTris);

	std::vector<Vector3> outfit = {
		{0.0f, 0.0f, 0.75f},  // 0.25 inside the top face
		{0.0f, 0.0f, 1.05f},  // just outside
		{0.0f, 0.0f, 1.5f},	  // clearly outside
		{0.0f, 0.0f, 20.0f},  // beyond the search radius
	};

	auto stats = MeshAnalysis::AnalyzePenetration(outfit, bodyVerts, bodyTris, 0.1f, 5.0f);
	// Distances are measured along the interpolated vertex normals. On a box those lean towards the
	// corners, so at the middle of a face they come out about 1.5% short.
	REQUIRE(stats.signedDistances.size() == outfit.size());
	CHECK(stats.signedDistances[0] == Catch::Approx(-0.25f).margin(0.01));
	CHECK(stats.signedDistances[1] == Catch::Approx(0.05f).margin(0.01));
	CHECK(stats.signedDistances[2] == Catch::Approx(0.5f).margin(0.01));
	CHECK(stats.signedDistances[3] == FLT_MAX);
	CHECK(stats.penetrating == 1);
	CHECK(stats.nearSurface == 1);
	CHECK(stats.maxDepth == Catch::Approx(0.25f).margin(0.01));
}

TEST_CASE("Points past an open edge of the body are outside", "[MeshAnalysis]") {
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	MakeBox(1.0f, bodyVerts, bodyTris);
	// Without its top, like a body without a head
	bodyTris.erase(bodyTris.begin() + 2, bodyTris.begin() + 4);

	std::vector<Vector3> outfit = {
		{0.9f, 0.0f, 1.2f}, // over the opening, closest to its rim
		{0.9f, 0.0f, 0.0f}, // inside, closest to a side
		{1.0f, 1.0f, 1.3f}, // over a corner of the rim
	};

	auto stats = MeshAnalysis::AnalyzePenetration(outfit, bodyVerts, bodyTris, 0.1f, 5.0f);
	CHECK(stats.signedDistances[0] == Catch::Approx(std::sqrt(0.05f)).margin(0.01));
	CHECK(stats.signedDistances[1] < 0.0f);
	CHECK(stats.signedDistances[2] == Catch::Approx(0.3f).margin(0.01));
	CHECK(stats.penetrating == 1);
}

TEST_CASE("Morph roughness reports both absolute and normalized values", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(6, 1.0f, verts, tris);

	// A uniform lift has no roughness
	std::vector<Vector3> uniform(verts.size(), Vector3(0.0f, 0.0f, 1.0f));
	auto smooth = MeshAnalysis::AnalyzeMorph(tris, uniform);
	CHECK(smooth.movedCount == verts.size());
	CHECK(smooth.meanDelta == Catch::Approx(1.0f));
	CHECK(smooth.roughness == Catch::Approx(0.0f).margin(1e-6));
	CHECK(smooth.maxBoundaryRoughness == Catch::Approx(0.0f).margin(1e-6));

	// The same lift on one vertex is a spike with a hard edge
	std::vector<Vector3> spike(verts.size());
	spike[24] = Vector3(0.0f, 0.0f, 1.0f);
	auto rough = MeshAnalysis::AnalyzeMorph(tris, spike);
	CHECK(rough.movedCount == 1);
	CHECK(rough.roughness == Catch::Approx(1.0f));
	CHECK(rough.normalizedRoughness == Catch::Approx(1.0f));
	CHECK(rough.maxBoundaryRoughness == Catch::Approx(1.0f));

	// Halving the spike halves the absolute roughness, the normalized one stays
	spike[24] = Vector3(0.0f, 0.0f, 0.5f);
	auto weaker = MeshAnalysis::AnalyzeMorph(tris, spike);
	CHECK(weaker.roughness == Catch::Approx(0.5f));
	CHECK(weaker.normalizedRoughness == Catch::Approx(1.0f));
}

TEST_CASE("Flipped triangles are counted", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(2, 1.0f, verts, tris);

	std::vector<Vector3> moved = verts;
	CHECK(MeshAnalysis::CountFlippedTriangles(verts, moved, tris) == 0);

	// Pull the center vertex past its neighbors, folding the triangles around it
	moved[4] = Vector3(3.0f, 3.0f, 0.0f);
	CHECK(MeshAnalysis::CountFlippedTriangles(verts, moved, tris) > 0);

	// Every flipped triangle is one around the pulled vertex
	auto flipped = MeshAnalysis::FlippedTriangles(verts, moved, tris);
	CHECK(flipped.size() == MeshAnalysis::CountFlippedTriangles(verts, moved, tris));
	for (size_t t : flipped) {
		REQUIRE(t < tris.size());
		CHECK((tris[t].p1 == 4 || tris[t].p2 == 4 || tris[t].p3 == 4));
	}
}

TEST_CASE("Weight problems are found per vertex", "[MeshAnalysis]") {
	std::unordered_map<uint16_t, float> boneA = {{0, 1.0f}, {1, 0.5f}, {2, 0.2f}, {3, 0.2f}};
	std::unordered_map<uint16_t, float> boneB = {{1, 0.5f}, {2, 0.2f}, {3, 0.2f}};
	std::unordered_map<uint16_t, float> boneC = {{2, 0.2f}, {3, 0.2f}};
	std::unordered_map<uint16_t, float> boneD = {{2, 0.2f}, {3, 0.2f}};
	std::unordered_map<uint16_t, float> boneE = {{2, 0.2f}};

	auto stats = MeshAnalysis::AnalyzeWeights(5, {&boneA, &boneB, &boneC, &boneD, &boneE}, 4, 0.01f);
	CHECK(stats.unweighted == 1);		  // Vertex 4
	CHECK(stats.overInfluenceLimit == 1); // Vertex 2 has 5 bones
	CHECK(stats.badSum == 1);			  // Vertex 3 adds up to 0.8
	CHECK(stats.maxInfluences == 5);
	CHECK(stats.maxSumError == Catch::Approx(0.2f).margin(1e-5));
}

TEST_CASE("Delta comparison reports mean, max and RMS error", "[MeshAnalysis]") {
	std::vector<Vector3> a = {{0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	std::vector<Vector3> b = {{0, 0, 0}, {0, 0, 0}, {0, 2, 0}, {0, 0, 0}};

	auto cmp = MeshAnalysis::CompareDeltas(a, b);
	CHECK(cmp.compared == 4);
	CHECK(cmp.meanError == Catch::Approx(0.75f));
	CHECK(cmp.maxError == Catch::Approx(2.0f));
	CHECK(cmp.rmsError == Catch::Approx(std::sqrt(5.0f / 4.0f)));
}

TEST_CASE("Heatmap colors run from blue to red", "[MeshAnalysis]") {
	auto low = MeshAnalysis::HeatmapColor(0.0f);
	auto mid = MeshAnalysis::HeatmapColor(0.5f);
	auto high = MeshAnalysis::HeatmapColor(1.0f);

	CHECK(low.x == 0.0f);
	CHECK(low.z == 1.0f);
	CHECK(mid.y == Catch::Approx(1.0f));
	CHECK(high.x == 1.0f);
	CHECK(high.y == 0.0f);

	// Out of range values are clamped
	CHECK(MeshAnalysis::HeatmapColor(-3.0f).z == 1.0f);
	CHECK(MeshAnalysis::HeatmapColor(7.0f).x == 1.0f);
}

TEST_CASE("Vertex roughness is high where a morph has a bump", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(4, 1.0f, verts, tris);

	// Everything moves up by 1, the center vertex by 2
	std::vector<Vector3> deltas(verts.size(), Vector3(0.0f, 0.0f, 1.0f));
	deltas[12] = Vector3(0.0f, 0.0f, 2.0f);

	auto roughness = MeshAnalysis::VertexRoughness(tris, deltas);
	REQUIRE(roughness.size() == verts.size());
	CHECK(roughness[12] == Catch::Approx(1.0f));
	CHECK(roughness[0] == Catch::Approx(0.0f));
	CHECK(roughness[13] > 0.0f);
	CHECK(roughness[13] < roughness[12]);
}

TEST_CASE("Coincident points and connected parts", "[MeshAnalysis]") {
	// Two grids side by side that share their edge at x = 2, and a separate triangle
	std::vector<Vector3> verts, right;
	std::vector<Triangle> tris, rightTris;
	MakeGrid(2, 1.0f, verts, tris);
	MakeGrid(2, 1.0f, right, rightTris);

	const uint16_t offset = static_cast<uint16_t>(verts.size());
	for (auto& v : right)
		verts.push_back(v + Vector3(2.0f, 0.0f, 0.0f));
	for (auto& t : rightTris)
		tris.emplace_back(t.p1 + offset, t.p2 + offset, t.p3 + offset);

	const uint16_t island = static_cast<uint16_t>(verts.size());
	verts.emplace_back(10.0f, 10.0f, 0.0f);
	verts.emplace_back(11.0f, 10.0f, 0.0f);
	verts.emplace_back(10.0f, 11.0f, 0.0f);
	tris.emplace_back(island, island + 1, island + 2);

	auto groups = MeshAnalysis::CoincidentGroups(verts, 1e-4f);
	CHECK(groups.size() == 3); // The shared edge has three vertices
	for (auto& g : groups)
		CHECK(g.size() == 2);

	int count = 0;
	auto parts = MeshAnalysis::ConnectedParts(verts, tris, 1e-4f, nullptr, &count);
	CHECK(count == 2);
	CHECK(parts[0] == 0);
	CHECK(parts[offset + 8] == 0);
	CHECK(parts[island] == 1);

	// Without welding, the grids are separate parts of the same size
	parts = MeshAnalysis::ConnectedParts(verts, tris, 0.0f, nullptr, &count);
	CHECK(count == 3);
	CHECK(parts[0] != parts[offset]);
	CHECK(parts[island] == 2);

	// Left out vertices split the parts they were in
	std::vector<bool> include(verts.size(), true);
	include[island + 1] = false;
	parts = MeshAnalysis::ConnectedParts(verts, tris, 1e-4f, &include, &count);
	CHECK(parts[island + 1] == -1);
	CHECK(parts[island] != parts[island + 2]);
}

TEST_CASE("Covered vertices are found along the normal on either side", "[MeshAnalysis]") {
	// The body faces +Z
	std::vector<Vector3> body;
	std::vector<Triangle> bodyTris;
	MakeGrid(10, 1.0f, body, bodyTris);

	// A patch over the corner from x, y = -0.5 to 4.5, at the given height, wound so it faces the body
	auto cover = [](float z, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
		MakeGrid(5, 1.0f, verts, tris);
		for (auto& v : verts)
			v += Vector3(-0.5f, -0.5f, z);
		for (auto& t : tris)
			std::swap(t.p2, t.p3);
	};
	auto count = [](const std::vector<bool>& covered) { return std::count(covered.begin(), covered.end(), true); };
	auto index = [](int x, int y) { return y * 11 + x; };

	std::vector<Vector3> coverVerts;
	std::vector<Triangle> coverTris;
	cover(1.5f, coverVerts, coverTris);

	// In front of the body: within outside it covers the 5 x 5 vertices under it
	auto covered = MeshAnalysis::CoveredVertices(body, bodyTris, coverVerts, coverTris, 2.0f, 1.0f);
	CHECK(count(covered) == 25);
	CHECK(covered[index(0, 0)]);
	CHECK(covered[index(4, 4)]);
	CHECK_FALSE(covered[index(5, 4)]);
	CHECK(count(MeshAnalysis::CoveredVertices(body, bodyTris, coverVerts, coverTris, 1.0f, 1.0f)) == 0);

	// Sunk into the body: within inside
	cover(-0.5f, coverVerts, coverTris);
	CHECK(count(MeshAnalysis::CoveredVertices(body, bodyTris, coverVerts, coverTris, 2.0f, 1.0f)) == 25);
	CHECK(count(MeshAnalysis::CoveredVertices(body, bodyTris, coverVerts, coverTris, 2.0f, 0.25f)) == 0);

	// Facing away from the body counts the same
	for (auto& t : coverTris)
		std::swap(t.p2, t.p3);
	CHECK(count(MeshAnalysis::CoveredVertices(body, bodyTris, coverVerts, coverTris, 2.0f, 1.0f)) == 25);
}

TEST_CASE("The best fitting transform is found again", "[MeshAnalysis]") {
	std::vector<Vector3> from = {{0, 0, 0}, {1, 0, 0}, {0, 2, 0}, {0, 0, 3}, {1, 1, 1}};

	// Turn 30 degrees about Z, scale by 1.5, move
	const float angle = 30.0f * 3.14159265f / 180.0f;
	auto transform = [&](const Vector3& p, float scale) {
		Vector3 r(p.x * std::cos(angle) - p.y * std::sin(angle), p.x * std::sin(angle) + p.y * std::cos(angle), p.z);
		return r * scale + Vector3(2.0f, -1.0f, 0.5f);
	};

	std::vector<Vector3> to;
	for (auto& p : from)
		to.push_back(transform(p, 1.0f));

	auto rigid = MeshAnalysis::BestFitTransform(from, to, nullptr, false);
	for (size_t i = 0; i < from.size(); i++) {
		Vector3 fitted = rigid.Apply(from[i]);
		CHECK(fitted.x == Catch::Approx(to[i].x).margin(1e-4));
		CHECK(fitted.y == Catch::Approx(to[i].y).margin(1e-4));
		CHECK(fitted.z == Catch::Approx(to[i].z).margin(1e-4));
	}

	to.clear();
	for (auto& p : from)
		to.push_back(transform(p, 1.5f));

	auto similar = MeshAnalysis::BestFitTransform(from, to, nullptr, true);
	CHECK(similar.scale == Catch::Approx(1.5));
	for (size_t i = 0; i < from.size(); i++) {
		Vector3 fitted = similar.Apply(from[i]);
		CHECK(fitted.x == Catch::Approx(to[i].x).margin(1e-4));
		CHECK(fitted.y == Catch::Approx(to[i].y).margin(1e-4));
		CHECK(fitted.z == Catch::Approx(to[i].z).margin(1e-4));
	}

	// Without scale, the size stays: the fit only turns and moves
	auto unscaled = MeshAnalysis::BestFitTransform(from, to, nullptr, false);
	CHECK(unscaled.scale == 1.0);
	CHECK(unscaled.Apply(from[1]).DistanceTo(unscaled.Apply(from[0])) == Catch::Approx(1.0f).margin(1e-4));
}

TEST_CASE("Rigidified parts move without bending", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(4, 1.0f, verts, tris);

	// A morph that bends the plate up in the middle
	std::vector<Vector3> deltas;
	for (auto& v : verts)
		deltas.emplace_back(0.0f, 0.0f, 1.0f + (v.x == 2.0f ? 0.5f : 0.0f));

	std::vector<int> parts(verts.size(), 0);
	std::vector<float> influence(verts.size(), 1.0f);
	// Masked opposite corners keep their deltas, and leave the plate symmetric
	influence[0] = 0.0f;
	influence[24] = 0.0f;

	auto result = MeshAnalysis::RigidifyDeltas(verts, deltas, parts, influence, false);
	CHECK(result[0].z == Catch::Approx(1.0f));
	CHECK(result[24].z == Catch::Approx(1.0f));

	// The others move as one: the same delta for a pure lift
	for (size_t v = 1; v < 24; v++) {
		CHECK(result[v].x == Catch::Approx(result[1].x).margin(1e-4));
		CHECK(result[v].y == Catch::Approx(result[1].y).margin(1e-4));
		CHECK(result[v].z == Catch::Approx(result[1].z).margin(1e-4));
	}
	CHECK(result[1].z == Catch::Approx(1.1f).margin(0.02));
}

TEST_CASE("Weight deviation compares with the closest body point", "[MeshAnalysis]") {
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	MakeGrid(2, 1.0f, bodyVerts, bodyTris);

	// The body is weighted to A on the left half, B on the right
	std::unordered_map<uint16_t, float> bodyA, bodyB;
	for (uint16_t i = 0; i < bodyVerts.size(); i++) {
		float right = bodyVerts[i].x / 2.0f;
		bodyB[i] = right;
		bodyA[i] = 1.0f - right;
	}

	std::vector<Vector3> points = {{0.0f, 1.0f, 0.1f}, {2.0f, 1.0f, 0.1f}, {1.0f, 1.0f, 0.1f}, {50.0f, 0.0f, 0.0f}};
	auto closest = MeshAnalysis::ClosestSurfacePoints(bodyVerts, bodyTris, points, 1.0f);
	REQUIRE(closest.size() == 4);
	CHECK(closest[0].distance == Catch::Approx(0.1f).margin(1e-4));
	CHECK(closest[3].triangle == -1);

	// Point 0 matches the body, point 1 has the opposite bone, point 2 is half off
	std::unordered_map<uint16_t, float> pointA = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};
	auto deviation = MeshAnalysis::WeightDeviation(closest, bodyTris, {{"A", &bodyA}, {"B", &bodyB}}, {{"A", &pointA}});
	CHECK(deviation[0] == Catch::Approx(0.0f).margin(1e-4));
	CHECK(deviation[1] == Catch::Approx(1.0f).margin(1e-4));
	CHECK(deviation[2] == Catch::Approx(0.5f).margin(1e-4));
	CHECK(deviation[3] == -1.0f);
}

namespace {
// Neighbors of each vertex through the triangles
std::vector<std::vector<int>> GridNeighbors(size_t count, const std::vector<Triangle>& tris) {
	std::vector<std::set<int>> sets(count);
	for (auto& t : tris) {
		const uint16_t p[3] = {t.p1, t.p2, t.p3};
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				if (i != j)
					sets[p[i]].insert(p[j]);
	}
	std::vector<std::vector<int>> neighbors(count);
	for (size_t v = 0; v < count; v++)
		neighbors[v].assign(sets[v].begin(), sets[v].end());
	return neighbors;
}
} // namespace

TEST_CASE("Edge strain ignores turns and finds tearing", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(2, 1.0f, verts, tris);

	// Turned by 90 degrees about Z: nothing stretches
	std::vector<Vector3> turned;
	for (auto& v : verts)
		turned.emplace_back(-v.y, v.x, v.z);
	auto stats = MeshAnalysis::AnalyzeStrain(verts, turned, tris);
	CHECK(stats.edges == 16);
	CHECK(stats.maxStrain == Catch::Approx(0.0f).margin(1e-5));
	CHECK(stats.stretched == 0);

	// The right column pulled away to x = 4: its edges to the middle double in length
	std::vector<Vector3> torn = verts;
	for (auto& v : torn)
		if (v.x > 1.5f)
			v.x = 4.0f;
	stats = MeshAnalysis::AnalyzeStrain(verts, torn, tris);
	CHECK(stats.maxStrain > 1.0f);
	CHECK(stats.stretched > 0);
	CHECK(stats.vertexStrain[2] > 1.0f);
	CHECK(stats.vertexStrain[0] == Catch::Approx(0.0f).margin(1e-5));

	// Shrinking counts like stretching
	std::vector<Vector3> shrunk;
	for (auto& v : verts)
		shrunk.push_back(v * 0.5f);
	CHECK(MeshAnalysis::AnalyzeStrain(verts, shrunk, tris).maxStrain == Catch::Approx(1.0f));
}

TEST_CASE("Smoothing weights turns a hard border into a gradient", "[MeshAnalysis]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(8, 1.0f, verts, tris);
	auto neighbors = GridNeighbors(verts.size(), tris);

	// Bone 0 on the left half, bone 1 on the right, bone 2 unused
	MeshAnalysis::DenseWeights weights(verts.size(), std::vector<float>(3, 0.0f));
	for (size_t v = 0; v < verts.size(); v++)
		weights[v][verts[v].x < 4.0f ? 0 : 1] = 1.0f;

	auto jumpBefore = MeshAnalysis::WeightJump(weights, neighbors);
	CHECK(*std::max_element(jumpBefore.begin(), jumpBefore.end()) == Catch::Approx(1.0f));

	auto smoothed = MeshAnalysis::SmoothWeights(weights, neighbors, {}, 20, 0.5f);
	auto jumpAfter = MeshAnalysis::WeightJump(smoothed, neighbors);
	CHECK(*std::max_element(jumpAfter.begin(), jumpAfter.end()) < 0.5f);
	for (auto& w : smoothed) {
		CHECK(w[0] + w[1] + w[2] == Catch::Approx(1.0f).margin(1e-4));
		CHECK(w[2] == 0.0f);
	}
	// The far edges keep most of their own bone
	CHECK(smoothed[0][0] > 0.9f);

	// Vertices without influence stay as they are
	std::vector<float> influence(verts.size(), 1.0f);
	influence[3] = 0.0f;
	auto anchored = MeshAnalysis::SmoothWeights(weights, neighbors, influence, 20, 0.5f);
	CHECK(anchored[3] == weights[3]);

	// Only some bones: the others stay, and each vertex keeps its total on the blended ones
	for (auto& w : weights) {
		w[0] *= 0.5f;
		w[1] *= 0.5f;
		w[2] = 0.5f;
	}
	std::vector<bool> bones = {true, true, false};
	auto partial = MeshAnalysis::SmoothWeights(weights, neighbors, {}, 20, 0.5f, &bones);
	for (auto& w : partial) {
		CHECK(w[2] == 0.5f);
		CHECK(w[0] + w[1] == Catch::Approx(0.5f).margin(1e-4));
	}
}

TEST_CASE("Weights are limited and normalized", "[MeshAnalysis]") {
	std::vector<float> weights = {0.4f, 0.3f, 0.2f, 0.05f, 0.005f};
	CHECK(MeshAnalysis::LimitWeights(weights, 3, 0.01f));
	CHECK(weights[3] == 0.0f);
	CHECK(weights[4] == 0.0f);
	CHECK(weights[0] == Catch::Approx(0.4f / 0.9f));
	CHECK(weights[0] + weights[1] + weights[2] == Catch::Approx(1.0f));

	// The largest stays when all are tiny
	std::vector<float> tiny = {0.002f, 0.001f};
	MeshAnalysis::LimitWeights(tiny, 4, 0.01f);
	CHECK(tiny[0] == Catch::Approx(1.0f));
	CHECK(tiny[1] == 0.0f);

	std::vector<float> fine = {0.5f, 0.5f};
	CHECK_FALSE(MeshAnalysis::LimitWeights(fine, 4, 0.01f));
}
