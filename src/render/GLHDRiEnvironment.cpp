/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLHDRiEnvironment.h"
#include "../files/EXRImage.h"
#include "../utils/ConfigurationManager.h"
#include "GLMaterial.h"

#include <wx/log.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

extern ConfigurationManager Config;

namespace {
// Edge length of a cube map face. An HDRi is a backdrop rather than something anyone inspects up
// close, and this still leaves a full mip chain for roughness to pick from.
constexpr int kFaceSize = 256;

// Reflections sample no sharper than this, so a generated cube map keeps a trace of blur even at
// full gloss. A perfect mirror of a low resolution environment reads worse than a slightly soft one.
constexpr float kMinLod = 0.75f;

// Mean linear luminance an HDRi is scaled to before it is tone mapped. The renderer has no exposure
// control, so without this a night sky and a studio backdrop would come out decades apart.
constexpr double kTargetLuminance = 0.25;

constexpr double kPi = 3.14159265358979323846;

// Half to float for the mean the exposure is worked out from. Subnormals are below 1e-4 and
// infinities and NaNs aren't radiance, so both are read as zero rather than handled.
float HalfToFloat(const uint16_t h) {
	const uint32_t exponent = (h >> 10) & 0x1Fu;
	if (exponent == 0 || exponent == 31)
		return 0.0f;

	const uint32_t bits = (static_cast<uint32_t>(h & 0x8000u) << 16) | ((exponent + (127 - 15)) << 23) | (static_cast<uint32_t>(h & 0x3FFu) << 13);

	float result = 0.0f;
	std::memcpy(&result, &bits, sizeof(result));
	return result;
}

// Rows of an equirectangular image don't cover equal amounts of sky - the ones near the poles are
// the same few directions stretched out - so they're weighted by the solid angle they stand for.
float ComputeExposure(const std::vector<uint16_t>& pixels, const int width, const int height) {
	double weightedSum = 0.0;
	double weightTotal = 0.0;

	for (int y = 0; y < height; y++) {
		const double weight = std::sin((y + 0.5) / height * kPi);

		double rowSum = 0.0;
		for (int x = 0; x < width; x++) {
			const uint16_t* texel = &pixels[(static_cast<size_t>(y) * width + x) * 4];
			rowSum += 0.2126 * HalfToFloat(texel[0]) + 0.7152 * HalfToFloat(texel[1]) + 0.0722 * HalfToFloat(texel[2]);
		}

		weightedSum += rowSum / width * weight;
		weightTotal += weight;
	}

	if (weightTotal <= 0.0)
		return 1.0f;

	const double mean = weightedSum / weightTotal;
	return std::clamp(static_cast<float>(kTargetLuminance / std::max(mean, 1.0e-4)), 0.01f, 100.0f);
}

GLMaterial* CreateMaterial(const std::string& fragShader, std::string& outError) {
	GLMaterial* material = new GLMaterial(Config["AppDir"] + "/res/shaders/fullscreentri.vert", Config["AppDir"] + "/res/shaders/" + fragShader);

	if (material->GetShader().GetError(&outError)) {
		delete material;
		return nullptr;
	}

	return material;
}
} // namespace

GLHDRiEnvironment::~GLHDRiEnvironment() {
	// Only the materials are freed here. The textures need a current context to delete, which is
	// what Clear() is called with; anything still alive at this point goes with the context anyway.
	delete equirectMat;
	delete prefilterMat;
	delete backgroundMat;
}

void GLHDRiEnvironment::Clear() {
	if (equirectID) {
		glDeleteTextures(1, &equirectID);
		equirectID = 0;
	}

	if (cubemapID) {
		glDeleteTextures(1, &cubemapID);
		cubemapID = 0;
	}

	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}

	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	maxLevel = 0;
	maxLod = 0.0f;
}

bool GLHDRiEnvironment::LoadShaders(std::string& outError) {
	if (equirectMat && prefilterMat && backgroundMat)
		return true;

	if (!equirectMat)
		equirectMat = CreateMaterial("hdri_equirect.frag", outError);

	if (equirectMat && !prefilterMat)
		prefilterMat = CreateMaterial("hdri_prefilter.frag", outError);

	if (prefilterMat && !backgroundMat)
		backgroundMat = CreateMaterial("hdri_background.frag", outError);

	return equirectMat && prefilterMat && backgroundMat;
}

bool GLHDRiEnvironment::Load(const std::string& fileName, std::string& outError) {
	if (fileName.empty()) {
		Clear();
		return true;
	}

	if (!LoadShaders(outError))
		return false;

	std::vector<uint16_t> pixels;
	int width = 0;
	int height = 0;
	if (!LoadEXRImage(fileName, pixels, width, height, outError))
		return false;

	const float exposure = ComputeExposure(pixels, width, height);

	// Everything is rebuilt from scratch, so a second HDRi doesn't inherit the first one's size.
	Clear();

	glGenTextures(1, &equirectID);
	glBindTexture(GL_TEXTURE_2D, equirectID);
	// Longitude wraps around, latitude runs into the poles and must not.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, pixels.data());

	maxLevel = 0;
	while ((kFaceSize >> maxLevel) > 1)
		maxLevel++;

	// The same ceiling the file cube map path applies, so a rough surface blurs to the same degree
	// whichever kind of cube map it ended up with.
	maxLod = static_cast<float>(std::min(maxLevel, 7));

	glGenTextures(1, &cubemapID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxLevel);

	for (int level = 0; level <= maxLevel; level++) {
		const int levelSize = kFaceSize >> level;
		for (int face = 0; face < 6; face++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, GL_RGBA16F, levelSize, levelSize, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
	}

	glGenFramebuffers(1, &fbo);
	glGenVertexArrays(1, &vao);

	BuildCubemap(exposure);

	wxLogMessage("Loaded HDRi '%s' (%dx%d) at exposure %.3f.", fileName, width, height, exposure);
	return true;
}

void GLHDRiEnvironment::BuildCubemap(const float exposure) {
	GLint prevFBO = 0;
	GLint prevViewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindVertexArray(vao);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	// Base level: the equirect projected onto each face.
	GLShader& equirectShader = equirectMat->GetShader();
	equirectShader.Begin();
	equirectShader.SetUniform("exposure", exposure);
	equirectShader.BindTexture(0, equirectID, "texDiffuse");

	glViewport(0, 0, kFaceSize, kFaceSize);
	for (int face = 0; face < 6; face++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cubemapID, 0);
		equirectShader.SetUniform("cubeFace", face);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	equirectShader.End();

	// Remaining levels: each one convolves the level above it, so the blur accumulates down the
	// chain instead of every level having to reach all the way back to the sharpest one.
	GLShader& prefilterShader = prefilterMat->GetShader();
	prefilterShader.Begin();
	prefilterShader.BindCubemap(4, cubemapID, "texCubemap");

	for (int level = 1; level <= maxLevel; level++) {
		// The level being written must not also be readable. Narrowing the sampling window to the
		// one level being read is what keeps this from being a framebuffer feedback loop.
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, level - 1);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, level - 1);

		prefilterShader.SetUniform("roughness", static_cast<float>(level) / static_cast<float>(maxLevel));

		const int levelSize = kFaceSize >> level;
		glViewport(0, 0, levelSize, levelSize);

		for (int face = 0; face < 6; face++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cubemapID, level);
			prefilterShader.SetUniform("cubeFace", face);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxLevel);
	prefilterShader.End();

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
}

void GLHDRiEnvironment::RenderBackground(const glm::mat4x4& matProjection, const glm::mat4x4& matView) {
	if (!cubemapID || !backgroundMat)
		return;

	GLShader& shader = backgroundMat->GetShader();
	shader.Begin();
	shader.SetMatrixProjection(matProjection);
	shader.SetMatrixModelView(matView, glm::mat4x4(1.0f));
	shader.BindCubemap(4, cubemapID, "texCubemap");

	// Covers every pixel, so there is nothing to test or write against - the meshes go on top.
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	shader.End();
	glActiveTexture(GL_TEXTURE0);
}

float GLHDRiEnvironment::GetMinLod() {
	return kMinLod;
}
