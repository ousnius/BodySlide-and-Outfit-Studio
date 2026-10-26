/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "TextureConvert.h"

#include <algorithm>
#include <cmath>

namespace TextureConvert {

namespace {

float Clamp01(float v) {
	return std::clamp(v, 0.0f, 1.0f);
}

void Normalize(float* p) {
	// Stored from 0 to 1 for -1 to 1
	float x = p[0] * 2.0f - 1.0f, y = p[1] * 2.0f - 1.0f, z = p[2] * 2.0f - 1.0f;
	float length = std::sqrt(x * x + y * y + z * z);
	if (length < 1e-6f) {
		p[0] = p[1] = 0.5f;
		p[2] = 1.0f;
		return;
	}
	p[0] = (x / length + 1.0f) * 0.5f;
	p[1] = (y / length + 1.0f) * 0.5f;
	p[2] = (z / length + 1.0f) * 0.5f;
}

} // namespace

Image::Image(int inWidth, int inHeight, float fill)
	: width(inWidth)
	, height(inHeight)
	, rgba(static_cast<size_t>(std::max(inWidth, 0)) * std::max(inHeight, 0) * 4, fill) {}

const float* Image::Sample(float u, float v) const {
	int x = std::clamp(static_cast<int>(u * width), 0, width - 1);
	int y = std::clamp(static_cast<int>(v * height), 0, height - 1);
	return At(x, y);
}

Image FromPixels(const uint8_t* pixels, int bitsPerChannel, int width, int height, int channels, int outWidth, int outHeight) {
	outWidth = std::clamp(outWidth, 1, std::max(width, 1));
	outHeight = std::clamp(outHeight, 1, std::max(height, 1));
	Image image(outWidth, outHeight);
	if (!pixels || width <= 0 || height <= 0 || channels < 1 || channels > 4)
		return image;

	std::vector<float> counts(static_cast<size_t>(outWidth) * outHeight, 0.0f);
	const bool wide = bitsPerChannel == 16;
	const float scale = wide ? 1.0f / 65535.0f : 1.0f / 255.0f;
	auto read = [&](size_t index) {
		if (wide) {
			uint16_t value;
			std::copy(pixels + index * 2, pixels + index * 2 + 2, reinterpret_cast<uint8_t*>(&value));
			return value * scale;
		}
		return pixels[index] * scale;
	};

	for (int y = 0; y < height; y++) {
		const int oy = static_cast<int>(static_cast<int64_t>(y) * outHeight / height);
		for (int x = 0; x < width; x++) {
			const int ox = static_cast<int>(static_cast<int64_t>(x) * outWidth / width);
			const size_t base = (static_cast<size_t>(y) * width + x) * channels;
			float px[4];
			if (channels <= 2) {
				px[0] = px[1] = px[2] = read(base);
				px[3] = channels == 2 ? read(base + 1) : 1.0f;
			}
			else {
				px[0] = read(base);
				px[1] = read(base + 1);
				px[2] = read(base + 2);
				px[3] = channels == 4 ? read(base + 3) : 1.0f;
			}
			float* out = image.At(ox, oy);
			for (int c = 0; c < 4; c++)
				out[c] += px[c];
			counts[static_cast<size_t>(oy) * outWidth + ox] += 1.0f;
		}
	}

	for (int y = 0; y < outHeight; y++)
		for (int x = 0; x < outWidth; x++) {
			float count = counts[static_cast<size_t>(y) * outWidth + x];
			if (count > 0.0f) {
				float* out = image.At(x, y);
				for (int c = 0; c < 4; c++)
					out[c] /= count;
			}
		}
	return image;
}

void FitSize(int width, int height, int maxSize, int& outWidth, int& outHeight) {
	auto powerOfTwoBelow = [](int v) {
		int p = 1;
		while (p * 2 <= v)
			p *= 2;
		return p;
	};
	outWidth = powerOfTwoBelow(std::max(width, 1));
	outHeight = powerOfTwoBelow(std::max(height, 1));
	while (std::max(outWidth, outHeight) > std::max(maxSize, 1)) {
		outWidth = std::max(1, outWidth / 2);
		outHeight = std::max(1, outHeight / 2);
	}
}

std::vector<Image> MipChain(const Image& image, bool normalMap) {
	std::vector<Image> levels;
	levels.push_back(image);
	while (levels.back().width > 1 || levels.back().height > 1) {
		const Image& prev = levels.back();
		Image next(std::max(1, prev.width / 2), std::max(1, prev.height / 2));
		for (int y = 0; y < next.height; y++)
			for (int x = 0; x < next.width; x++) {
				float* out = next.At(x, y);
				int samples = 0;
				for (int dy = 0; dy < 2; dy++)
					for (int dx = 0; dx < 2; dx++) {
						int sx = std::min(x * 2 + dx, prev.width - 1);
						int sy = std::min(y * 2 + dy, prev.height - 1);
						const float* in = prev.At(sx, sy);
						for (int c = 0; c < 4; c++)
							out[c] += in[c];
						samples++;
					}
				for (int c = 0; c < 4; c++)
					out[c] /= static_cast<float>(samples);
				if (normalMap)
					Normalize(out);
			}
		levels.push_back(std::move(next));
	}
	return levels;
}

std::vector<uint8_t> ToBytes(const Image& image) {
	std::vector<uint8_t> bytes(image.rgba.size());
	for (size_t i = 0; i < bytes.size(); i++)
		bytes[i] = static_cast<uint8_t>(std::lround(Clamp01(image.rgba[i]) * 255.0f));
	return bytes;
}

float Channel(const Image& image, int x, int y, int width, int height, int channel) {
	if (image.Empty())
		return 0.0f;
	return image.Sample((x + 0.5f) / width, (y + 0.5f) / height)[channel];
}

Image SkyrimNormal(const Image& normal, const Image& roughness, const Image& metallic, const SkyrimOptions& options) {
	Image out = normal;
	for (int y = 0; y < out.height; y++)
		for (int x = 0; x < out.width; x++) {
			float* p = out.At(x, y);
			if (options.openGLNormal)
				p[1] = 1.0f - p[1];
			Normalize(p);

			// Smooth parts shine, rough ones barely; metal a bit more
			float gloss = roughness.Empty() ? 0.5f : 1.0f - Channel(roughness, x, y, out.width, out.height, 0);
			float metal = metallic.Empty() ? 0.0f : Channel(metallic, x, y, out.width, out.height, 0);
			float specular = std::pow(Clamp01(gloss), 1.5f) * ((1.0f - options.metalSpecular) + options.metalSpecular * metal);
			p[3] = Clamp01(specular * options.specularScale);
		}
	return out;
}

Image SkyrimEnvironmentMask(const Image& metallic, const Image& roughness, int width, int height) {
	Image out(width, height, 0.0f);
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++) {
			float metal = metallic.Empty() ? 0.0f : Channel(metallic, x, y, width, height, 0);
			float gloss = roughness.Empty() ? 0.5f : 1.0f - Channel(roughness, x, y, width, height, 0);
			float mask = Clamp01(metal * std::sqrt(Clamp01(gloss)));
			float* p = out.At(x, y);
			p[0] = p[1] = p[2] = mask;
			p[3] = 1.0f;
		}
	return out;
}

Image FalloutSmoothSpec(const Image& roughness, const Image& metallic, int width, int height, const SkyrimOptions& options) {
	Image out(width, height, 0.0f);
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++) {
			float gloss = roughness.Empty() ? 0.5f : 1.0f - Channel(roughness, x, y, width, height, 0);
			float metal = metallic.Empty() ? 0.0f : Channel(metallic, x, y, width, height, 0);
			float* p = out.At(x, y);
			p[0] = Clamp01((0.3f + 0.7f * metal) * Clamp01(gloss) * options.specularScale);
			p[1] = Clamp01(gloss);
			p[2] = 0.0f;
			p[3] = 1.0f;
		}
	return out;
}

Image Diffuse(const Image& baseColor, const Image& opacity, const Image& occlusion) {
	Image out = baseColor;
	for (int y = 0; y < out.height; y++)
		for (int x = 0; x < out.width; x++) {
			float* p = out.At(x, y);
			if (!occlusion.Empty()) {
				float ao = Channel(occlusion, x, y, out.width, out.height, 0);
				for (int c = 0; c < 3; c++)
					p[c] *= ao;
			}
			if (!opacity.Empty())
				p[3] = Channel(opacity, x, y, out.width, out.height, 0);
		}
	return out;
}

float MeanGloss(const Image& roughness) {
	if (roughness.Empty())
		return 0.5f;
	double sum = 0.0;
	for (int y = 0; y < roughness.height; y++)
		for (int x = 0; x < roughness.width; x++)
			sum += 1.0 - roughness.At(x, y)[0];
	return static_cast<float>(sum / (static_cast<double>(roughness.width) * roughness.height));
}

float SpecularPowerFromGloss(float gloss) {
	return std::max(2.0f, std::pow(2.0f, Clamp01(gloss) * 10.0f + 1.0f));
}

} // namespace TextureConvert
