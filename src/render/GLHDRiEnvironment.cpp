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
// Edge length of a face of the reflection cube map. Reflections are small and mostly blurred, and
// this still leaves a full mip chain for roughness to pick from.
constexpr int kFaceSize = 256;

// Limits on the edge length of a face of the background cube map. A face spans a quarter of the
// equirect's width, so a 4K HDRi needs 1024 to keep all of its detail. The ceiling keeps an 8K or
// larger one from costing more video memory than a backdrop is worth.
constexpr int kMinBackgroundFaceSize = 256;
constexpr int kMaxBackgroundFaceSize = 2048;

// Reflections sample no sharper than this, so a generated cube map keeps a trace of blur even at
// full gloss. A perfect mirror of a low resolution environment reads worse than a slightly soft one.
constexpr float kMinLod = 0.75f;

// Mean linear luminance an HDRi is scaled to before it is tone mapped. The renderer has no exposure
// control, so without this a night sky and a studio backdrop would come out decades apart.
constexpr double kTargetLuminance = 0.25;

constexpr double kPi = 3.14159265358979323846;

// The smallest power of two face that holds as much detail as the source has, within the limits.
int BackgroundFaceSize(const int equirectWidth) {
	int size = kMinBackgroundFaceSize;
	while (size < kMaxBackgroundFaceSize && size * 4 < equirectWidth)
		size *= 2;

	return size;
}

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

// The state a full screen pass needs, put in place for as long as it lives and handed back exactly
// as it was found. Restoring matters more than it looks: GLSurface picks a cull mode per mesh and
// leaves it behind, so a frame whose last shape was DRAW_CW ends with GL_FRONT selected - and the
// full screen triangle is wound front facing, so drawing it under that state would cull the whole
// background away.
struct FullScreenPassState {
	GLboolean blend = GL_FALSE;
	GLboolean cullFace = GL_FALSE;
	GLboolean depthTest = GL_FALSE;
	GLboolean depthMask = GL_TRUE;

	FullScreenPassState() {
		blend = glIsEnabled(GL_BLEND);
		cullFace = glIsEnabled(GL_CULL_FACE);
		depthTest = glIsEnabled(GL_DEPTH_TEST);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

		// The triangle covers every pixel of its target, so there is nothing to test, blend or cull
		// against.
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	~FullScreenPassState() {
		if (blend)
			glEnable(GL_BLEND);

		if (cullFace)
			glEnable(GL_CULL_FACE);

		if (depthTest)
			glEnable(GL_DEPTH_TEST);

		glDepthMask(depthMask);
	}

	FullScreenPassState(const FullScreenPassState&) = delete;
	FullScreenPassState& operator=(const FullScreenPassState&) = delete;
};

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

	if (backgroundID) {
		glDeleteTextures(1, &backgroundID);
		backgroundID = 0;
	}

	backgroundSize = 0;

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

bool GLHDRiEnvironment::Load(const std::string& fileName, std::string& outError, const float anisotropy) {
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, pixels.data());

	// Mipmapped, because a source larger than the faces it is projected into gets minified. One
	// sample of the full resolution image skips most of the texels a face texel covers, which
	// turns fine detail into blocks. The poles are squeezed far more across than down, which is
	// what the anisotropy is for.
	if (glGenerateMipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	}

	if (anisotropy > 1.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

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

	backgroundSize = BackgroundFaceSize(width);

	glGenTextures(1, &backgroundID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, backgroundID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glGenerateMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	if (!glGenerateMipmap)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

	// What gets stored is already tone mapped into 0 to 1 and encoded for the screen, so 10 bit
	// fixed point loses nothing a 10 bit framebuffer could show, at half the size of half floats.
	for (int face = 0; face < 6; face++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB10_A2, backgroundSize, backgroundSize, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);

	glGenFramebuffers(1, &fbo);
	glGenVertexArrays(1, &vao);

	if (!BuildCubemaps(exposure)) {
		outError = "The cube map could not be rendered into.";
		Clear();
		return false;
	}

	// The equirect has served its purpose. Everything downstream reads the cube maps, so holding on
	// to a full resolution RGBA16F copy of the source would cost video memory nothing ever reads.
	glDeleteTextures(1, &equirectID);
	equirectID = 0;

	wxLogMessage("Loaded HDRi '%s' (%dx%d) at exposure %.3f, background faces %dx%d.", fileName, width, height, exposure, backgroundSize, backgroundSize);
	return true;
}

bool GLHDRiEnvironment::ProjectEquirect(const GLuint cube, const int faceSize, const float exposure) {
	glViewport(0, 0, faceSize, faceSize);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube, 0);

	// Checked once per cube map, before anything is drawn into it. Every face is the same format
	// and size, so what the driver makes of the first it makes of all of them. Worth asking: were
	// the framebuffer to be left unusable, the passes would draw into the default one instead and
	// paint the viewport with the environment while leaving the cube map empty.
	if (glCheckFramebufferStatus && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	GLShader& equirectShader = equirectMat->GetShader();
	equirectShader.Begin();
	equirectShader.SetUniform("exposure", exposure);
	equirectShader.BindTexture(0, equirectID, "texDiffuse");

	for (int face = 0; face < 6; face++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cube, 0);
		equirectShader.SetUniform("cubeFace", face);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	equirectShader.End();
	return true;
}

bool GLHDRiEnvironment::BuildCubemaps(const float exposure) {
	GLint prevFBO = 0;
	GLint prevViewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindVertexArray(vao);

	FullScreenPassState passState;

	// Reflection cube map. Its base level is the equirect projected onto each face, and every
	// level below it is rendered out of the one above, so a mip stands for a roughness. The
	// levels differ from the base only in size, so its completeness stands for theirs.
	bool complete = ProjectEquirect(cubemapID, kFaceSize, exposure);

	if (complete) {
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
	}

	// Background cube map. Only the base level is projected; the levels below are plain
	// downsamples for when the viewport is too small to show all of it.
	if (complete)
		complete = ProjectEquirect(backgroundID, backgroundSize, exposure);

	// Nothing stays attached, so no later pass can find a cube map it reads from bound as its own
	// target.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0);

	if (complete && glGenerateMipmap) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, backgroundID);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
	glActiveTexture(GL_TEXTURE0);
	return complete;
}

void GLHDRiEnvironment::RenderBackground(const glm::mat4x4& matProjection, const glm::mat4x4& matView) {
	if (!backgroundID || !backgroundMat)
		return;

	GLShader& shader = backgroundMat->GetShader();
	shader.Begin();
	shader.SetMatrixProjection(matProjection);
	shader.SetMatrixModelView(matView, glm::mat4x4(1.0f));
	shader.BindCubemap(4, backgroundID, "texCubemap");

	// The meshes go on top of this, and each of them sets the depth and cull state it wants for
	// itself, so what the pass borrows here is given straight back.
	FullScreenPassState passState;

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	shader.End();
	glActiveTexture(GL_TEXTURE0);
}

float GLHDRiEnvironment::GetMinLod() {
	return kMinLod;
}
