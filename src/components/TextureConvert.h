/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <vector>

// Making game textures from the maps other programs export: PBR base color, normal (OpenGL or DirectX style),
// roughness, metallic, ambient occlusion and opacity maps, turned into the maps a game's shader reads. Works on plain
// pixel data; loading and saving files is up to the caller.
namespace TextureConvert {

// RGBA pixels from 0 to 1, row by row from the top
struct Image {
	int width = 0;
	int height = 0;
	std::vector<float> rgba;

	Image() = default;
	Image(int inWidth, int inHeight, float fill = 0.0f);

	bool Empty() const { return width <= 0 || height <= 0; }
	float* At(int x, int y) { return &rgba[(static_cast<size_t>(y) * width + x) * 4]; }
	const float* At(int x, int y) const { return &rgba[(static_cast<size_t>(y) * width + x) * 4]; }
	// The pixel at texture coordinates from 0 to 1, the nearest one
	const float* Sample(float u, float v) const;
};

// Pixels of 8 or 16 bits per channel with 1 to 4 channels (gray, gray and alpha, RGB, RGBA), averaged down to the
// size, which can't be larger than the pixels'. Gray goes into red, green and blue; missing alpha is 1.
Image FromPixels(const uint8_t* pixels, int bitsPerChannel, int width, int height, int channels, int outWidth, int outHeight);

// The largest power of two size not over maxSize, keeping the aspect ratio of width and height
void FitSize(int width, int height, int maxSize, int& outWidth, int& outHeight);

// Each level half the size of the one before, down to 1 by 1, the image itself first. Normal maps are made unit
// length again on every level.
std::vector<Image> MipChain(const Image& image, bool normalMap);

// RGBA, 8 bits per channel
std::vector<uint8_t> ToBytes(const Image& image);

// The value of one channel, by texture coordinates, for maps of another size
float Channel(const Image& image, int x, int y, int width, int height, int channel);

struct SkyrimOptions {
	// The normal map's green points up the image, as OpenGL and Blender have it. Skyrim's goes down.
	bool openGLNormal = true;
	// Scales the specular mask
	float specularScale = 1.0f;
	// Share of the specular mask that only metal gets, 0 to 1: other parts shine with the rest of it
	float metalSpecular = 0.6f;
};

// Skyrim's tangent space normal map, with the specular mask in alpha: from gloss (1 - roughness) and metallic. Either
// can be empty.
Image SkyrimNormal(const Image& normal, const Image& roughness, const Image& metallic, const SkyrimOptions& options);

// Skyrim's environment mask (slot 5): reflections where the surface is metallic and smooth
Image SkyrimEnvironmentMask(const Image& metallic, const Image& roughness, int width, int height);

// Fallout 4's _s map: specular in red, smoothness in green
Image FalloutSmoothSpec(const Image& roughness, const Image& metallic, int width, int height, const SkyrimOptions& options);

// The diffuse map: base color with the opacity as alpha when there is one, and ambient occlusion multiplied in
Image Diffuse(const Image& baseColor, const Image& opacity, const Image& occlusion);

// The mean gloss (1 - roughness) of a roughness map, 0 to 1
float MeanGloss(const Image& roughness);

// Skyrim's glossiness (SpecularPower) for a mean gloss: 2^(gloss * 10 + 1), at least 2
float SpecularPowerFromGloss(float gloss);

} // namespace TextureConvert
