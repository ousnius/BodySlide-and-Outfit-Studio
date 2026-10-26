#include "../src/components/SurfaceFit.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

using nifly::Triangle;
using nifly::Vector3;

namespace {
constexpr float Pi = 3.14159265358979f;

// Ellipsoid around a center with rings of segments vertices between the poles, normals pointing outwards
void MakeEllipsoid(const Vector3& radii, const Vector3& center, int rings, int segments, std::vector<Vector3>& verts, std::vector<Triangle>& tris) {
	verts.clear();
	tris.clear();
	verts.push_back(center + Vector3(0.0f, 0.0f, radii.z));
	for (int r = 1; r < rings; r++) {
		float theta = Pi * r / rings;
		for (int s = 0; s < segments; s++) {
			float phi = 2.0f * Pi * s / segments;
			verts.push_back(center + Vector3(radii.x * std::sin(theta) * std::cos(phi), radii.y * std::sin(theta) * std::sin(phi), radii.z * std::cos(theta)));
		}
	}
	const uint16_t south = static_cast<uint16_t>(verts.size());
	verts.push_back(center + Vector3(0.0f, 0.0f, -radii.z));

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

// Distance of a point outside an ellipsoid along the direction from its center, roughly the gap for a near-sphere
float RadialGap(const Vector3& p, const Vector3& radii) {
	Vector3 d = p;
	float length = d.length();
	d /= length;
	float surface = 1.0f / std::sqrt(d.x * d.x / (radii.x * radii.x) + d.y * d.y / (radii.y * radii.y) + d.z * d.z / (radii.z * radii.z));
	return length - surface;
}

} // namespace

TEST_CASE("A loose shell is fitted onto a body at the gap", "[surfacefit]") {
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	const Vector3 bodyRadii(10.0f, 8.0f, 12.0f);
	MakeEllipsoid(bodyRadii, Vector3(), 24, 32, body, bodyTris);
	MakeEllipsoid(Vector3(13.0f, 13.0f, 13.0f), Vector3(), 20, 28, shell, shellTris);

	SurfaceFit::Options options;
	options.gap = 0.5f;
	// The shell has to shrink by a third along one axis, more than the strain limit lets it
	options.maxStrain = 0.0f;
	SurfaceFit::Result result = SurfaceFit::Fit(shell, shellTris, body, bodyTris, options);

	REQUIRE(result.positions.size() == shell.size());
	REQUIRE(result.pulled > shell.size() * 9 / 10);
	REQUIRE(result.flipped == 0);
	REQUIRE(result.insideAfter == 0);
	REQUIRE(result.meanGapError < 0.15f);

	float worst = 0.0f;
	for (auto& p : result.positions)
		worst = std::max(worst, std::fabs(RadialGap(p, bodyRadii) - 0.5f));
	REQUIRE(worst < 0.6f);
}

TEST_CASE("A shell inside the body is pushed out", "[surfacefit]") {
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 24, 32, body, bodyTris);
	MakeEllipsoid(Vector3(9.0f, 9.5f, 9.0f), Vector3(), 20, 28, shell, shellTris);

	SurfaceFit::Options options;
	options.gap = 0.3f;
	SurfaceFit::Result result = SurfaceFit::Fit(shell, shellTris, body, bodyTris, options);

	REQUIRE(result.insideBefore > shell.size() / 2);
	REQUIRE(result.insideAfter == 0);
	REQUIRE(result.flipped == 0);
}

TEST_CASE("Relief stays when a shell is fitted at a constant gap", "[surfacefit]") {
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 24, 32, body, bodyTris);
	MakeEllipsoid(Vector3(12.0f, 12.0f, 12.0f), Vector3(), 30, 40, shell, shellTris);

	// A ridge around the equator, standing out by 0.8
	std::vector<size_t> ridge;
	for (size_t i = 0; i < shell.size(); i++)
		if (std::fabs(shell[i].z) < 0.3f) {
			shell[i] *= (12.8f / 12.0f);
			ridge.push_back(i);
		}
	REQUIRE(!ridge.empty());

	auto ridgeHeight = [&](const std::vector<Vector3>& positions) {
		// Ridge radius against the rings next to it
		double ridgeSum = 0.0, nearSum = 0.0;
		size_t nearCount = 0;
		for (size_t i : ridge)
			ridgeSum += positions[i].length();
		for (size_t i = 0; i < positions.size(); i++) {
			float z = std::fabs(shell[i].z);
			if (z > 0.6f && z < 2.0f) {
				nearSum += positions[i].length();
				nearCount++;
			}
		}
		return static_cast<float>(ridgeSum / ridge.size() - nearSum / nearCount);
	};

	SurfaceFit::Options flat;
	flat.gap = 0.3f;
	SurfaceFit::Result pressed = SurfaceFit::Fit(shell, shellTris, body, bodyTris, flat);

	SurfaceFit::Options keep = flat;
	keep.reliefSmoothing = 10;
	SurfaceFit::Result kept = SurfaceFit::Fit(shell, shellTris, body, bodyTris, keep);

	const float before = ridgeHeight(shell);
	REQUIRE(ridgeHeight(kept.positions) > before * 0.6f);
	REQUIRE(ridgeHeight(kept.positions) > ridgeHeight(pressed.positions));
	REQUIRE(kept.flipped == 0);
}

TEST_CASE("Masked vertices stay and pieces without a pull follow", "[surfacefit]") {
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 24, 32, body, bodyTris);
	MakeEllipsoid(Vector3(12.0f, 12.0f, 12.0f), Vector3(), 20, 28, shell, shellTris);

	// A small separate piece far out, like a charm hanging off the outfit, out of the body's reach
	std::vector<Vector3> piece;
	std::vector<Triangle> pieceTris;
	MakeEllipsoid(Vector3(0.5f, 0.5f, 0.5f), Vector3(25.0f, 0.0f, 0.0f), 4, 6, piece, pieceTris);
	const uint16_t offset = static_cast<uint16_t>(shell.size());
	shell.insert(shell.end(), piece.begin(), piece.end());
	for (auto& t : pieceTris)
		shellTris.emplace_back(t.p1 + offset, t.p2 + offset, t.p3 + offset);

	SurfaceFit::Options options;
	options.gap = 0.3f;
	options.lock.assign(shell.size(), 0.0f);
	options.lock[1] = 1.0f;
	SurfaceFit::Result result = SurfaceFit::Fit(shell, shellTris, body, bodyTris, options);

	REQUIRE(result.positions[1].DistanceTo(shell[1]) < 1e-4f);
	REQUIRE(result.followed == piece.size());
	// The piece moved as a whole, by the shell's pull nearby (towards the body)
	Vector3 moved = result.positions[offset] - shell[offset];
	REQUIRE(moved.length() > 0.5f);
	for (size_t i = offset; i < shell.size(); i++)
		REQUIRE((result.positions[i] - shell[i]).DistanceTo(moved) < 1e-4f);
}

TEST_CASE("A symmetric fit fills out body parts the mesh doesn't reach", "[surfacefit]") {
	// The target has a bump the source lacks, the way a fuller chest is on another body
	std::vector<Vector3> target, source;
	std::vector<Triangle> targetTris, sourceTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 30, 40, target, targetTris);
	for (auto& p : target)
		if (p.x > 7.0f)
			p.x += (p.x - 7.0f) * 0.8f;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 24, 32, source, sourceTris);

	SurfaceFit::Options oneWay;
	oneWay.gap = 0.0f;
	oneWay.minGap = 0.0f;
	SurfaceFit::Options both = oneWay;
	both.symmetric = true;

	auto tip = [](const std::vector<Vector3>& positions) {
		float x = 0.0f;
		for (auto& p : positions)
			x = std::max(x, p.x);
		return x;
	};

	SurfaceFit::Result plain = SurfaceFit::Fit(source, sourceTris, target, targetTris, oneWay);
	SurfaceFit::Result filled = SurfaceFit::Fit(source, sourceTris, target, targetTris, both);
	REQUIRE(tip(filled.positions) > tip(plain.positions) - 0.05f);
	REQUIRE(tip(filled.positions) > 11.5f);
	REQUIRE(filled.flipped == 0);
}

TEST_CASE("The fit gives way where it would stretch the mesh too much", "[surfacefit]") {
	// A sphere onto a flat ellipsoid: fitting it fully squashes its edges by far more than the limit
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 4.0f), Vector3(), 24, 32, body, bodyTris);
	MakeEllipsoid(Vector3(11.0f, 11.0f, 11.0f), Vector3(), 20, 28, shell, shellTris);

	SurfaceFit::Options free;
	free.gap = 0.3f;
	free.maxStrain = 0.0f;
	free.clippingPasses = 0;
	SurfaceFit::Options limited = free;
	limited.maxStrain = 0.3f;

	SurfaceFit::Result squashed = SurfaceFit::Fit(shell, shellTris, body, bodyTris, free);
	SurfaceFit::Result kept = SurfaceFit::Fit(shell, shellTris, body, bodyTris, limited);
	REQUIRE(kept.relaxed > 0);
	REQUIRE(kept.meanStrain < squashed.meanStrain);
	REQUIRE(kept.strainedEdges < squashed.strainedEdges);
}

TEST_CASE("Clipping left after the fit is pushed out", "[surfacefit]") {
	// A coarse shell partly inside a dense body, with relief that keeps parts of it in
	std::vector<Vector3> body, shell;
	std::vector<Triangle> bodyTris, shellTris;
	MakeEllipsoid(Vector3(10.0f, 10.0f, 10.0f), Vector3(), 40, 56, body, bodyTris);
	MakeEllipsoid(Vector3(9.8f, 9.8f, 9.8f), Vector3(), 8, 10, shell, shellTris);

	SurfaceFit::Options none;
	none.gap = 0.05f;
	none.minGap = 0.05f;
	none.clippingPasses = 0;
	SurfaceFit::Options fixed = none;
	fixed.clippingPasses = 4;

	SurfaceFit::Result left = SurfaceFit::Fit(shell, shellTris, body, bodyTris, none);
	SurfaceFit::Result pushed = SurfaceFit::Fit(shell, shellTris, body, bodyTris, fixed);
	// The dense body bulges out between the coarse shell's vertices
	REQUIRE(left.pokeAfter > 0);
	REQUIRE(pushed.pokeAfter < left.pokeAfter / 4);
	REQUIRE(pushed.insideAfter == 0);
	REQUIRE(pushed.flipped == 0);
}

TEST_CASE("Parts nothing pulls keep the motion of the pulled ones instead of drifting", "[surfacefit]") {
	// A tube over a body that ends halfway along it, like a boot past a body's open ankle
	std::vector<Vector3> body, tube;
	std::vector<Triangle> bodyTris, tubeTris;
	MakeEllipsoid(Vector3(3.0f, 3.0f, 30.0f), Vector3(0.0f, 0.0f, 30.0f), 40, 24, body, bodyTris);
	// Only the upper half of the body: drop triangles below z = 30
	std::vector<Triangle> upper;
	for (auto& t : bodyTris)
		if (body[t.p1].z > 30.0f && body[t.p2].z > 30.0f && body[t.p3].z > 30.0f)
			upper.push_back(t);
	MakeEllipsoid(Vector3(4.0f, 4.0f, 28.0f), Vector3(0.0f, 0.0f, 30.0f), 40, 24, tube, tubeTris);

	SurfaceFit::Options options;
	options.gap = 0.3f;
	options.maxDistance = 3.0f;
	SurfaceFit::Result result = SurfaceFit::Fit(tube, tubeTris, body, upper, options);

	// The lower half moves about as much as the upper one, not more and more further down
	float lowest = 0.0f;
	for (size_t i = 0; i < tube.size(); i++)
		if (tube[i].z < 8.0f)
			lowest = std::max(lowest, tube[i].DistanceTo(result.positions[i]));
	REQUIRE(lowest < 2.0f);
	// Only a few slivers at the tube's narrow tips may flip
	REQUIRE(result.flipped < tubeTris.size() / 100);
}
