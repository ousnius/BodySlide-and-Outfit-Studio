#include "../src/components/TextureConvert.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

using namespace TextureConvert;
using Catch::Approx;

TEST_CASE("Sizes fit as powers of two", "[textures]") {
	int w = 0, h = 0;
	FitSize(4096, 4096, 2048, w, h);
	REQUIRE((w == 2048 && h == 2048));
	FitSize(1000, 500, 2048, w, h);
	REQUIRE((w == 512 && h == 256));
	FitSize(768, 768, 4096, w, h);
	REQUIRE((w == 512 && h == 512));
}

TEST_CASE("Pixels are averaged down and gray fills the color channels", "[textures]") {
	// 4x2 gray image, left half 0, right half 255, scaled to 2x1
	std::vector<uint8_t> pixels = {0, 0, 255, 255, 0, 0, 255, 255};
	Image image = FromPixels(pixels.data(), 8, 4, 2, 1, 2, 1);
	REQUIRE(image.width == 2);
	REQUIRE(image.At(0, 0)[0] == Approx(0.0f));
	REQUIRE(image.At(1, 0)[1] == Approx(1.0f));
	REQUIRE(image.At(1, 0)[3] == Approx(1.0f));

	// 16 bits per channel
	std::vector<uint16_t> wide = {0, 65535, 32768};
	Image rgb = FromPixels(reinterpret_cast<const uint8_t*>(wide.data()), 16, 1, 1, 3, 1, 1);
	REQUIRE(rgb.At(0, 0)[1] == Approx(1.0f));
	REQUIRE(rgb.At(0, 0)[2] == Approx(0.5f).margin(0.001f));
}

TEST_CASE("Skyrim normal maps turn green and carry the specular mask", "[textures]") {
	// A normal leaning up the image in OpenGL style: green above the middle
	Image normal(1, 1);
	float* n = normal.At(0, 0);
	n[0] = 0.5f;
	n[1] = 0.75f;
	n[2] = 0.933f;
	n[3] = 1.0f;

	Image rough(1, 1, 0.0f);
	Image metal(1, 1, 0.0f);
	rough.At(0, 0)[3] = metal.At(0, 0)[3] = 1.0f;

	SkyrimOptions options;
	Image out = SkyrimNormal(normal, rough, metal, options);
	REQUIRE(out.At(0, 0)[1] < 0.5f);
	REQUIRE(out.At(0, 0)[0] == Approx(0.5f).margin(0.01f));
	// Fully smooth, not metal: the share of the specular every surface gets
	REQUIRE(out.At(0, 0)[3] == Approx(1.0f - options.metalSpecular).margin(0.01f));

	// Metal shines fully, rough surfaces barely
	metal.At(0, 0)[0] = 1.0f;
	REQUIRE(SkyrimNormal(normal, rough, metal, options).At(0, 0)[3] == Approx(1.0f).margin(0.01f));
	for (int c = 0; c < 3; c++)
		rough.At(0, 0)[c] = 1.0f;
	REQUIRE(SkyrimNormal(normal, rough, metal, options).At(0, 0)[3] == Approx(0.0f).margin(0.01f));

	// DirectX style maps keep their green
	options.openGLNormal = false;
	REQUIRE(SkyrimNormal(normal, Image(), Image(), options).At(0, 0)[1] > 0.5f);
}

TEST_CASE("Environment masks and specular power follow metal and gloss", "[textures]") {
	Image metal(2, 1, 0.0f), rough(2, 1, 0.0f);
	metal.At(1, 0)[0] = 1.0f;
	Image mask = SkyrimEnvironmentMask(metal, rough, 2, 1);
	REQUIRE(mask.At(0, 0)[0] == Approx(0.0f));
	REQUIRE(mask.At(1, 0)[0] == Approx(1.0f));

	REQUIRE(MeanGloss(rough) == Approx(1.0f));
	REQUIRE(SpecularPowerFromGloss(0.5f) == Approx(64.0f));
	REQUIRE(SpecularPowerFromGloss(0.0f) == Approx(2.0f));
}

TEST_CASE("Mipmaps go down to one pixel and normal maps stay unit length", "[textures]") {
	Image normal(8, 4);
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 8; x++) {
			float* p = normal.At(x, y);
			// Alternating normals that average to a short vector
			p[0] = x % 2 ? 0.9f : 0.1f;
			p[1] = 0.5f;
			p[2] = 0.7f;
			p[3] = 1.0f;
		}
	auto levels = MipChain(normal, true);
	REQUIRE(levels.size() == 4);
	REQUIRE((levels.back().width == 1 && levels.back().height == 1));
	const float* p = levels[1].At(0, 0);
	float x = p[0] * 2 - 1, y = p[1] * 2 - 1, z = p[2] * 2 - 1;
	REQUIRE(std::sqrt(x * x + y * y + z * z) == Approx(1.0f).margin(0.01f));
}
