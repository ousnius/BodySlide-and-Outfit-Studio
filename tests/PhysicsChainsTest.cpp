/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "../src/components/PhysicsChains.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace nifly;
using Catch::Matchers::WithinAbs;

namespace {

// A strip of quads from top down: width 1, rows every step along -Z, starting at top
void AddStrip(std::vector<Vector3>& verts, std::vector<Triangle>& tris, const Vector3& top, int rows, float step) {
	const uint16_t base = static_cast<uint16_t>(verts.size());
	for (int r = 0; r <= rows; r++) {
		verts.emplace_back(top.x - 0.5f, top.y, top.z - r * step);
		verts.emplace_back(top.x + 0.5f, top.y, top.z - r * step);
	}
	for (int r = 0; r < rows; r++) {
		uint16_t a = static_cast<uint16_t>(base + r * 2);
		tris.emplace_back(a, a + 1, a + 2);
		tris.emplace_back(a + 1, a + 3, a + 2);
	}
}

} // namespace

TEST_CASE("Two hanging strands get one chain each", "[PhysicsChains]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	// Left and right strands behind the head, 40 long, far apart
	AddStrip(verts, tris, Vector3(-8.0f, -8.0f, 130.0f), 80, 0.5f);
	AddStrip(verts, tris, Vector3(8.0f, -8.0f, 130.0f), 80, 0.5f);

	// The top rows are on the scalp
	std::vector<bool> roots(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		roots[v] = verts[v].z >= 129.0f;

	PhysicsChains::Options options;
	options.linkDistance = 0.6f;
	options.minVertices = 10;
	auto neighbors = PhysicsChains::Neighbors(verts, tris, options.linkDistance);
	auto result = PhysicsChains::Detect(verts, neighbors, roots, Vector3(0.0f, 0.0f, 120.0f), options);

	REQUIRE(result.chains.size() == 2);
	for (auto& chain : result.chains) {
		// Root + moving joints, every boneSpacing down the strand
		REQUIRE(chain.joints.size() >= 5);
		REQUIRE_THAT(chain.jointDistance[1], WithinAbs(options.hangStart, 1e-4));
		for (size_t j = 1; j < chain.joints.size(); j++)
			REQUIRE(chain.joints[j].z < chain.joints[j - 1].z);
	}
	// Sorted around the head from the front over the right: the right strand (+X) first, named from behind
	REQUIRE(result.chains[0].side == "BackR");
	REQUIRE(result.chains[1].side == "BackL");

	auto weights = PhysicsChains::JointWeights(result, options);
	REQUIRE(weights.size() == verts.size());
	float lastChainShare = 0.0f;
	for (size_t v = 0; v < verts.size(); v++) {
		float sum = weights[v].keep;
		for (auto& jw : weights[v].joints) {
			REQUIRE(jw.first >= 1);
			sum += jw.second;
		}
		REQUIRE_THAT(sum, WithinAbs(1.0, 1e-4));
		// Roots and the vertices near them keep their bones
		if (result.distance[v] <= options.hangStart)
			REQUIRE(weights[v].keep == 1.0f);
	}
	// Along one strand, the share on the chain only grows going down
	for (int r = 0; r <= 80; r++) {
		const auto& w = weights[size_t(r) * 2];
		float share = 1.0f - w.keep;
		REQUIRE(share + 1e-5f >= lastChainShare);
		lastChainShare = share;
	}
	REQUIRE(weights[160].keep == 0.0f);
}

TEST_CASE("Short parts and wide curtains", "[PhysicsChains]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	// A bun: 4 long, too short for a chain
	AddStrip(verts, tris, Vector3(0.0f, -10.0f, 128.0f), 16, 0.5f);
	std::vector<bool> roots(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		roots[v] = verts[v].z >= 127.9f;

	PhysicsChains::Options options;
	options.minVertices = 4;
	auto neighbors = PhysicsChains::Neighbors(verts, tris, options.linkDistance);
	auto result = PhysicsChains::Detect(verts, neighbors, roots, Vector3(0.0f, 0.0f, 120.0f), options);
	REQUIRE(result.chains.empty());
	REQUIRE(result.shortParts == 1);

	// A curtain all around the back, joined at the top: split into several chains
	verts.clear();
	tris.clear();
	for (int a = 0; a <= 12; a++) {
		const float angle = (90.0f + a * 15.0f) * 3.14159265f / 180.0f;
		AddStrip(verts, tris, Vector3(10.0f * std::sin(angle), 10.0f * std::cos(angle), 128.0f), 60, 0.5f);
	}
	roots.assign(verts.size(), false);
	for (size_t v = 0; v < verts.size(); v++)
		roots[v] = verts[v].z >= 127.9f;
	options.linkDistance = 3.0f;
	options.sectorAngle = 70.0f;
	neighbors = PhysicsChains::Neighbors(verts, tris, options.linkDistance);
	result = PhysicsChains::Detect(verts, neighbors, roots, Vector3(0.0f, 0.0f, 120.0f), options);
	REQUIRE(result.chains.size() >= 3);
	REQUIRE(result.chains.size() <= size_t(options.maxChains));
}

TEST_CASE("A strand chained lower down by an earlier pass gets no second chain on top", "[PhysicsChains]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	// A lock of stacked pieces: the upper piece from the scalp down, the lower one hanging on under it
	AddStrip(verts, tris, Vector3(5.0f, 5.0f, 130.0f), 30, 0.5f);
	const size_t upper = verts.size();
	AddStrip(verts, tris, Vector3(5.0f, 5.0f, 114.6f), 40, 0.5f);
	// A loose strand of its own beside it
	AddStrip(verts, tris, Vector3(-5.0f, 5.0f, 130.0f), 30, 0.5f);

	std::vector<bool> roots(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		roots[v] = verts[v].z >= 129.0f;
	// The earlier pass chained the lower piece
	std::vector<bool> blocked(verts.size(), false);
	for (size_t v = upper; v < upper + 82; v++)
		blocked[v] = true;

	PhysicsChains::Options options;
	options.hangStart = 1.5f;
	options.minHang = 4.0f;
	options.boneSpacing = 3.5f;
	options.minVertices = 10;
	options.linkDistance = 0.6f;
	auto neighbors = PhysicsChains::Neighbors(verts, tris, options.linkDistance);
	auto result = PhysicsChains::Detect(verts, neighbors, roots, Vector3(0.0f, 0.0f, 120.0f), options, &blocked);

	REQUIRE(result.continuedParts == 1);
	REQUIRE(result.chains.size() == 1);
	REQUIRE(result.chains[0].joints.back().x < 0.0f);
}
