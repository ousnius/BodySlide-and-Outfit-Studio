#include "../src/components/MaskTransfer.h"

#include <catch2/catch_test_macros.hpp>

using nifly::Triangle;
using nifly::Vector3;

namespace {
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

TEST_CASE("Mask transfer keeps a sharp border on a denser mesh", "[MaskTransfer]") {
	// Source: 4x4 quads of size 1, the left half (x < 2) is masked
	std::vector<Vector3> srcVerts;
	std::vector<Triangle> srcTris;
	MakeGrid(4, 1.0f, srcVerts, srcTris);

	std::vector<float> srcMask(srcVerts.size(), 0.0f);
	for (size_t i = 0; i < srcVerts.size(); i++)
		if (srcVerts[i].x < 1.5f)
			srcMask[i] = 1.0f;

	// Target: same area with a different, denser layout, slightly offset from the surface
	std::vector<Vector3> dstVerts;
	std::vector<Triangle> dstTris;
	MakeGrid(10, 0.4f, dstVerts, dstTris);
	for (auto& v : dstVerts)
		v.z = 0.1f;

	std::vector<float> outMask;
	std::vector<bool> outMatched;
	MaskTransfer::CopyMask(srcVerts, srcTris, srcMask, dstVerts, 0.5f, outMask, outMatched);

	REQUIRE(outMask.size() == dstVerts.size());
	REQUIRE(outMatched.size() == dstVerts.size());

	for (size_t i = 0; i < dstVerts.size(); i++) {
		REQUIRE(outMatched[i]);
		// Only values of the source are copied, nothing in between
		REQUIRE((outMask[i] == 0.0f || outMask[i] == 1.0f));
		// Clearly inside either half, the result has to follow the source
		if (dstVerts[i].x <= 0.8f)
			REQUIRE(outMask[i] == 1.0f);
		else if (dstVerts[i].x >= 2.2f)
			REQUIRE(outMask[i] == 0.0f);
	}
}

TEST_CASE("Mask transfer copies fractional mask values", "[MaskTransfer]") {
	std::vector<Vector3> srcVerts{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
	std::vector<Triangle> srcTris{{0, 1, 2}};
	std::vector<float> srcMask{0.25f, 0.5f, 0.75f};

	std::vector<Vector3> dstVerts{{0.05f, 0.05f, 0.0f}, {0.9f, 0.05f, 0.05f}, {0.05f, 0.9f, -0.05f}};

	std::vector<float> outMask;
	std::vector<bool> outMatched;
	MaskTransfer::CopyMask(srcVerts, srcTris, srcMask, dstVerts, 1.0f, outMask, outMatched);

	REQUIRE(outMask == std::vector<float>{0.25f, 0.5f, 0.75f});
}

TEST_CASE("Mask transfer leaves vertices beyond the distance unmatched", "[MaskTransfer]") {
	std::vector<Vector3> srcVerts{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
	std::vector<Triangle> srcTris{{0, 1, 2}};
	std::vector<float> srcMask{1.0f, 1.0f, 1.0f};

	std::vector<Vector3> dstVerts{{0.2f, 0.2f, 0.4f}, {0.2f, 0.2f, 0.6f}, {5.0f, 5.0f, 0.0f}};

	std::vector<float> outMask;
	std::vector<bool> outMatched;
	MaskTransfer::CopyMask(srcVerts, srcTris, srcMask, dstVerts, 0.5f, outMask, outMatched);

	REQUIRE(outMatched == std::vector<bool>{true, false, false});
	REQUIRE(outMask == std::vector<float>{1.0f, 0.0f, 0.0f});
}

TEST_CASE("Mask transfer without source triangles matches nothing", "[MaskTransfer]") {
	std::vector<Vector3> srcVerts{{0.0f, 0.0f, 0.0f}};
	std::vector<float> srcMask{1.0f};
	std::vector<Vector3> dstVerts{{0.0f, 0.0f, 0.0f}};

	std::vector<float> outMask;
	std::vector<bool> outMatched;
	MaskTransfer::CopyMask(srcVerts, {}, srcMask, dstVerts, 1.0f, outMask, outMatched);

	REQUIRE(outMatched == std::vector<bool>{false});
	REQUIRE(outMask == std::vector<float>{0.0f});
}
