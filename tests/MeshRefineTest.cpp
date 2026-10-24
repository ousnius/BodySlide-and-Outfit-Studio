#include "../src/components/MeshRefine.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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

// Octahedron with its corners on the axes at distance size, normals pointing outwards
void MakeOctahedron(float size, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts = {{size, 0, 0}, {-size, 0, 0}, {0, size, 0}, {0, -size, 0}, {0, 0, size}, {0, 0, -size}};
	tris.clear();
	for (int sx : {1, -1})
		for (int sy : {1, -1})
			for (int sz : {1, -1}) {
				uint16_t a = sx > 0 ? 0 : 1, b = sy > 0 ? 2 : 3, c = sz > 0 ? 4 : 5;
				if (sx * sy * sz > 0)
					tris.emplace_back(a, b, c);
				else
					tris.emplace_back(a, c, b);
			}
}

// Half a cylinder around the Y axis, segments of 45 degrees, length long
void MakeHalfCylinder(float radius, float length, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts.clear();
	tris.clear();
	const int segments = 4;
	for (int s = 0; s <= segments; s++) {
		float phi = Pi * s / segments;
		verts.emplace_back(radius * std::cos(phi), 0.0f, radius * std::sin(phi));
		verts.emplace_back(radius * std::cos(phi), length, radius * std::sin(phi));
	}
	for (int s = 0; s < segments; s++) {
		uint16_t a = static_cast<uint16_t>(s * 2), b = a + 1, c = a + 2, d = a + 3;
		tris.emplace_back(a, c, b);
		tris.emplace_back(c, d, b);
	}
}

bool HasEdge(const MeshRefine::Selection& selection, uint16_t a, uint16_t b) {
	return std::any_of(selection.edges.begin(), selection.edges.end(), [&](const MeshRefine::SplitEdge& e) {
		return e.p1 == std::min(a, b) && e.p2 == std::max(a, b);
	});
}

MeshRefine::Selection Select(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const MeshRefine::Options& options) {
	return MeshRefine::SelectEdges(verts, tris, {}, {}, {}, {}, options);
}
} // namespace

TEST_CASE("Edges longer than the limit are split", "[MeshRefine]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	MakeGrid(2, 5.0f, verts, tris);

	MeshRefine::Options options;
	options.maxEdgeLength = 6.0f;
	auto selection = Select(verts, tris, options);

	// Only the diagonals are longer than 6, and they are the longest edges, so nothing more is needed
	CHECK(selection.edges.size() == 4);
	CHECK(selection.byLength == 4);
	CHECK(selection.conforming == 0);
	CHECK(HasEdge(selection, 1, 3));
	CHECK(HasEdge(selection, 5, 7));
	for (auto& edge : selection.edges)
		CHECK(edge.reasons == MeshRefine::ReasonLength);

	SECTION("Edges with a vertex left out of include stay") {
		std::vector<bool> include(verts.size(), true);
		include[4] = false;
		selection = MeshRefine::SelectEdges(verts, tris, {}, include, {}, {}, options);
		CHECK(selection.edges.size() == 2);
		CHECK(HasEdge(selection, 1, 3));
		CHECK(HasEdge(selection, 5, 7));
	}

	SECTION("maxEdges keeps the worst") {
		options.maxEdges = 3;
		selection = Select(verts, tris, options);
		CHECK(selection.edges.size() == 3);
		CHECK(selection.overLimit == 1);
	}
}

TEST_CASE("Both sides of a split seam are one edge", "[MeshRefine]") {
	// Two quads side by side, the right one with its own copies of the shared vertices
	std::vector<Vector3> verts = {
		{0, 0, 0}, {5, 0, 0}, {0, 5, 0}, {5, 5, 0}, {5, 0, 0}, {10, 0, 0}, {5, 5, 0}, {10, 5, 0},
	};
	std::vector<Triangle> tris = {{0, 1, 2}, {1, 3, 2}, {4, 5, 6}, {5, 7, 6}};
	std::vector<int> weldIndex = {0, 1, 2, 3, 1, 5, 3, 7};

	MeshRefine::Options options;
	options.maxEdgeLength = 4.0f;
	CHECK(MeshRefine::SelectEdges(verts, tris, {}, {}, {}, {}, options).edges.size() == 10);

	auto selection = MeshRefine::SelectEdges(verts, tris, weldIndex, {}, {}, {}, options);
	CHECK(selection.edges.size() == 9);
	CHECK(HasEdge(selection, 1, 3));

	SECTION("A seam vertex is left alone when any copy is") {
		std::vector<bool> include(verts.size(), true);
		include[6] = false;
		selection = MeshRefine::SelectEdges(verts, tris, weldIndex, include, {}, {}, options);
		CHECK_FALSE(HasEdge(selection, 1, 3));
		CHECK_FALSE(HasEdge(selection, 3, 7));
	}
}

TEST_CASE("Edges across a curve are split, along it and at creases they aren't", "[MeshRefine]") {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;

	MeshRefine::Options options;
	options.maxAngle = 20.0f;

	SECTION("A half cylinder") {
		MakeHalfCylinder(3.0f, 1.0f, verts, tris);
		auto selection = Select(verts, tris, options);
		CHECK(selection.byCurvature > 0);
		// Edges along the axis join vertices with the same normal
		for (uint16_t s = 0; s <= 4; s++)
			CHECK_FALSE(HasEdge(selection, s * 2, s * 2 + 1));
		// Around it they turn by 45 degrees
		CHECK(HasEdge(selection, 2, 4));
	}

	SECTION("A fold of 90 degrees") {
		verts = {{0, 0, 0}, {5, 0, 0}, {0, 5, 0}, {5, 5, 0}, {5, 0, 5}, {5, 5, 5}};
		tris = {{0, 1, 2}, {1, 3, 2}, {1, 4, 3}, {4, 5, 3}};
		CHECK(Select(verts, tris, options).edges.empty());

		options.creaseAngle = 120.0f;
		CHECK_FALSE(Select(verts, tris, options).edges.empty());
	}
}

TEST_CASE("Edges the body pokes through or bulges into are split", "[MeshRefine]") {
	std::vector<Vector3> bodyVerts, verts;
	std::vector<Triangle> bodyTris, tris;
	MakeSphere(3.0f, 16, 32, bodyVerts, bodyTris);
	MeshRefine::BodySurface body{&bodyVerts, &bodyTris};

	MeshRefine::Options options;
	options.clipping = true;

	SECTION("The corners are outside, the middles of the edges inside") {
		MakeOctahedron(4.0f, verts, tris);
		auto selection = MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options);
		CHECK(selection.edges.size() == 12);
		CHECK(selection.byClipping == 12);
		CHECK(selection.pokeThrough > 0);

		std::vector<bool> excluded(verts.size(), true);
		CHECK(MeshRefine::SelectEdges(verts, tris, {}, {}, excluded, body, options).edges.empty());
	}

	SECTION("Clear of the body") {
		MakeOctahedron(6.0f, verts, tris);
		auto selection = MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options);
		CHECK(selection.edges.empty());
		CHECK(selection.pokeThrough == 0);
	}

	SECTION("Edges much longer than the body's") {
		MakeOctahedron(4.0f, verts, tris);
		options.clipping = false;
		options.bodyDetail = 2.0f;
		auto selection = MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options);
		CHECK(selection.byBodyDetail == 12);

		// Too far from the body to compare
		options.bodyDistance = 0.05f;
		CHECK(MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options).edges.empty());
	}
}

TEST_CASE("Triangles next to a split edge have their longest edge split too", "[MeshRefine]") {
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	MakeSphere(3.0f, 16, 32, bodyVerts, bodyTris);
	MeshRefine::BodySurface body{&bodyVerts, &bodyTris};

	// Just above the top of the body: only the middle of the edge 0-1 is within the clearance. The long thin
	// triangle on its other side would get thinner if only that edge were split.
	std::vector<Vector3> verts = {{-1.5f, 0.0f, 3.1f}, {1.5f, 0.0f, 3.1f}, {0.0f, -1.0f, 3.1f}, {0.0f, 8.0f, 3.1f}};
	std::vector<Triangle> tris = {{0, 2, 1}, {0, 1, 3}};

	MeshRefine::Options options;
	options.clipping = true;
	options.clearance = 0.2f;
	options.conforming = false;
	auto plain = MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options);
	REQUIRE(plain.edges.size() == 1);
	CHECK(HasEdge(plain, 0, 1));
	CHECK(plain.byClipping == 1);
	CHECK(plain.pokeThrough == 0);

	options.conforming = true;
	auto conforming = MeshRefine::SelectEdges(verts, tris, {}, {}, {}, body, options);
	CHECK(conforming.edges.size() == 2);
	CHECK(HasEdge(conforming, 0, 1));
	CHECK((HasEdge(conforming, 0, 3) || HasEdge(conforming, 1, 3)));
	CHECK(conforming.conforming == 1);

	SECTION("Unless that edge is masked") {
		std::vector<bool> include = {true, true, true, false};
		CHECK(MeshRefine::SelectEdges(verts, tris, {}, include, {}, body, options).edges.size() == 1);
	}
}
