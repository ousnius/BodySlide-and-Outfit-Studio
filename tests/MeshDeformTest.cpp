#include "../src/components/MeshAnalysis.h"
#include "../src/components/MeshDeform.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using nifly::Triangle;
using nifly::Vector3;

namespace {
constexpr float Pi = 3.14159265358979f;

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

// Sphere around the origin with rings of segments vertices between the poles, normals pointing outwards
void MakeSphere(float radius, int rings, int segments, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts.clear();
	tris.clear();
	verts.emplace_back(0.0f, 0.0f, radius);
	for (int r = 1; r < rings; r++) {
		float theta = Pi * r / rings;
		for (int s = 0; s < segments; s++) {
			float phi = 2.0f * Pi * s / segments;
			verts.emplace_back(radius * std::sin(theta) * std::cos(phi), radius * std::sin(theta) * std::sin(phi), radius * std::cos(theta));
		}
	}
	const uint16_t south = static_cast<uint16_t>(verts.size());
	verts.emplace_back(0.0f, 0.0f, -radius);

	auto ring = [segments](int r, int s) { return static_cast<uint16_t>(1 + (r - 1) * segments + (s % segments)); };
	for (int s = 0; s < segments; s++) {
		tris.emplace_back(0, ring(1, s), ring(1, s + 1));
		for (int r = 1; r < rings - 1; r++) {
			tris.emplace_back(ring(r, s), ring(r + 1, s), ring(r + 1, s + 1));
			tris.emplace_back(ring(r, s), ring(r + 1, s + 1), ring(r, s + 1));
		}
		tris.emplace_back(south, ring(rings - 1, s + 1), ring(rings - 1, s));
	}
}
} // namespace

TEST_CASE("Flipped triangles are turned back by smoothing the change around them", "[MeshDeform]") {
	std::vector<Vector3> before;
	std::vector<Triangle> tris;
	MakeGrid(12, 1.0f, before, tris);

	// A gentle bump everywhere, and one vertex pushed past its neighbor, which folds the triangles around it
	std::vector<Vector3> after = before;
	for (auto& v : after)
		v.z += 0.5f * std::sin(v.x * 0.5f) * std::cos(v.y * 0.5f);
	const int folded = 6 * 13 + 6;
	after[folded].x += 1.7f;

	std::vector<Vector3> fixed = after;
	MeshDeform::UntangleResult result = MeshDeform::UntangleFlips(before, fixed, tris, nullptr, MeshDeform::UntangleOptions());
	CHECK(result.flippedBefore > 0);
	CHECK(result.flippedAfter == 0);
	CHECK(MeshAnalysis::CountFlippedTriangles(before, fixed, tris) == 0);
	CHECK(result.movedVertices > 0);

	// The bump stays where the fold wasn't
	CHECK(fixed[0].z == Catch::Approx(after[0].z));
	CHECK(fixed[12 * 13 + 12].z == Catch::Approx(after[12 * 13 + 12].z));
}

TEST_CASE("Flipped triangles stay when every vertex is masked", "[MeshDeform]") {
	std::vector<Vector3> before;
	std::vector<Triangle> tris;
	MakeGrid(8, 1.0f, before, tris);
	std::vector<Vector3> after = before;
	after[4 * 9 + 4].x += 1.7f;

	std::vector<float> freedom(before.size(), 0.0f);
	std::vector<Vector3> fixed = after;
	MeshDeform::UntangleResult result = MeshDeform::UntangleFlips(before, fixed, tris, &freedom, MeshDeform::UntangleOptions());
	CHECK(result.flippedBefore > 0);
	CHECK(result.flippedAfter == result.flippedBefore);
	CHECK(result.movedVertices == 0);
	CHECK(fixed[4 * 9 + 4].x == Catch::Approx(after[4 * 9 + 4].x));
}

TEST_CASE("A cover presses the body in under it and makes it swell past its edge", "[MeshDeform]") {
	std::vector<Vector3> body;
	std::vector<Triangle> bodyTris;
	MakeSphere(10.0f, 40, 64, body, bodyTris);

	// A cap over the top of the sphere, a little outside it
	std::vector<Vector3> cover;
	std::vector<Triangle> coverTris;
	MakeSphere(10.2f, 40, 64, cover, coverTris);
	std::vector<Triangle> cap;
	for (auto& t : coverTris)
		if (cover[t.p1].z > 7.0f && cover[t.p2].z > 7.0f && cover[t.p3].z > 7.0f)
			cap.push_back(t);

	MeshDeform::ContactMorphOptions options;
	MeshDeform::ContactMorph morph = MeshDeform::MakeContactMorph(body, bodyTris, cover, cap, nullptr, options);
	REQUIRE(morph.covered > 0);

	// The top sinks in by the full depth, along the inward normal
	CHECK(morph.deltas[0].z == Catch::Approx(-options.depth).margin(0.05f));
	CHECK(morph.maxDepth == Catch::Approx(options.depth).margin(0.05f));

	// Past the edge of the cap the body swells out, and far away it stays
	CHECK(morph.maxBulge > options.bulge * 0.5f);
	CHECK(morph.maxBulge <= options.bulge + 1e-4f);
	const size_t south = body.size() - 1;
	CHECK(morph.deltas[south].length() < 1e-4f);

	bool swellsBelowEdge = false;
	for (size_t i = 0; i < body.size(); i++)
		if (body[i].z < 7.0f && body[i].z > 4.0f && morph.offsets[i] > options.bulge * 0.5f)
			swellsBelowEdge = true;
	CHECK(swellsBelowEdge);

	// Masked vertices stay
	std::vector<float> freedom(body.size(), 1.0f);
	freedom[0] = 0.0f;
	MeshDeform::ContactMorph masked = MeshDeform::MakeContactMorph(body, bodyTris, cover, cap, &freedom, options);
	CHECK(masked.deltas[0].length() < 1e-6f);
}

TEST_CASE("Shapes on a body follow its morph and those far from it don't", "[MeshDeform]") {
	std::vector<Vector3> body;
	std::vector<Triangle> tris;
	MakeGrid(10, 1.0f, body, tris);
	std::vector<Vector3> bodyDeltas(body.size(), Vector3(0.0f, 0.0f, -1.0f));

	std::vector<Vector3> points = {Vector3(5.0f, 5.0f, 0.3f), Vector3(5.5f, 4.5f, 0.1f), Vector3(5.0f, 5.0f, 6.0f)};
	std::vector<Vector3> deltas = MeshDeform::FollowDeltas(body, tris, bodyDeltas, points, {}, 2.0f);
	CHECK(deltas[0].z == Catch::Approx(-1.0f).margin(1e-4f));
	CHECK(deltas[1].z == Catch::Approx(-1.0f).margin(1e-4f));
	CHECK(deltas[2].length() < 1e-6f);

	// A point moves like the body right under it, not like the body around it
	for (size_t i = 0; i < body.size(); i++)
		bodyDeltas[i] = Vector3(0.0f, 0.0f, -body[i].x * 0.1f);
	deltas = MeshDeform::FollowDeltas(body, tris, bodyDeltas, {Vector3(3.0f, 5.0f, 0.2f)}, {}, 2.0f);
	CHECK(deltas[0].z == Catch::Approx(-0.3f).margin(1e-4f));
}

TEST_CASE("A thin strap that barely presses in makes little swell", "[MeshDeform]") {
	std::vector<Vector3> body;
	std::vector<Triangle> bodyTris;
	MakeGrid(40, 0.5f, body, bodyTris);

	// A strap 1 wide across the middle, just above the body
	std::vector<Vector3> strap;
	std::vector<Triangle> strapTris;
	MakeGrid(40, 0.5f, strap, strapTris);
	std::vector<Triangle> band;
	for (auto& t : strapTris)
		if (strap[t.p1].y >= 9.25f && strap[t.p1].y <= 10.25f && strap[t.p2].y >= 9.25f && strap[t.p2].y <= 10.25f && strap[t.p3].y >= 9.25f && strap[t.p3].y <= 10.25f)
			band.push_back(t);
	for (auto& v : strap)
		v.z += 0.1f;

	MeshDeform::ContactMorphOptions options;
	MeshDeform::ContactMorph morph = MeshDeform::MakeContactMorph(body, bodyTris, strap, band, nullptr, options);
	REQUIRE(morph.covered > 0);
	CHECK(morph.maxDepth < options.depth * 0.2f);
	CHECK(morph.maxBulge < options.bulge * 0.2f);
}
