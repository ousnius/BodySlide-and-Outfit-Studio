#include "../src/components/MeshAnalysis.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cfloat>

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
