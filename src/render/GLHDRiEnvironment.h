/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "GLExtensions.h"

#include <glm/glm.hpp>

#include <string>

class GLMaterial;

/*
An HDRi loaded from an equirectangular EXR, projected into a cube map whose mip chain has been
convolved for roughness. It stands in for the environment the viewport doesn't have: it is drawn as
the background, and it is what shapes reflect whose own cube map asked to be replaced.

The whole interface stays available in builds made without OpenEXR support; loading just always
fails there, so callers need no conditionals of their own.
*/
class GLHDRiEnvironment {
public:
	GLHDRiEnvironment() = default;
	~GLHDRiEnvironment();

	// Owns GL names and materials outright, so there is no sensible second copy of one.
	GLHDRiEnvironment(const GLHDRiEnvironment&) = delete;
	GLHDRiEnvironment& operator=(const GLHDRiEnvironment&) = delete;

	// Reads the EXR and builds the cube map from it. A failure leaves whatever was loaded before
	//  alone and puts the reason in outError.
	bool Load(const std::string& fileName, std::string& outError);
	void Clear();

	bool IsActive() const { return cubemapID != 0; }
	GLuint GetCubemapID() const { return cubemapID; }

	// Highest mip of the generated chain, how blurry a fully rough reflection is allowed to get.
	float GetMaxLod() const { return maxLod; }

	// Sharpest mip a reflection off a generated cube map may use. Not zero: a perfect mirror of an
	//  environment this size reads worse than one that keeps a trace of blur.
	static float GetMinLod();

	// Draws the environment behind everything else. The projection has to be a perspective one even
	//  when the viewport isn't: an orthographic matrix has no divide to unproject a ray through, so
	//  every pixel would come out looking the same direction.
	void RenderBackground(const glm::mat4x4& matProjection, const glm::mat4x4& matView);

private:
	// Equirectangular source, GL_RGBA16F. Only alive between the upload and the cube map being
	// rendered out of it; the member outlives that so Clear() can still free it if a load fails.
	GLuint equirectID = 0;
	// GL_TEXTURE_CUBE_MAP, GL_RGBA16F, with every level rendered rather than generated.
	GLuint cubemapID = 0;
	GLuint fbo = 0;
	// The passes are attribute-less, but core profile still refuses to draw without a bound array.
	GLuint vao = 0;

	int maxLevel = 0;
	float maxLod = 0.0f;

	GLMaterial* equirectMat = nullptr;
	GLMaterial* prefilterMat = nullptr;
	GLMaterial* backgroundMat = nullptr;

	// Compiles the passes on first use. False means one of them failed, with the log in outError.
	bool LoadShaders(std::string& outError);
	// Renders all six faces of every level. The base level projects the equirect, the rest convolve
	//  the level above them. False means the framebuffer wouldn't take the cube map as a target,
	//  which leaves it holding nothing anybody should reflect.
	bool BuildCubemap(float exposure);
};
