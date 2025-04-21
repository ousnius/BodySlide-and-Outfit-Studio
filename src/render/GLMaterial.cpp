/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLMaterial.h"

using namespace nifly;

GLMaterial::GLMaterial() {}

GLMaterial::~GLMaterial() {}

// Shader-only material, does not contain texture references, and thus does not use reference to res loader.
GLMaterial::GLMaterial(const std::string& shaderDirectory, const std::string& shaderName) {
	shaderDir = shaderDirectory;
	shaderPath = shaderDir + PathSepChar + shaderName;

	shader = GLShader(shaderPath + ".vert", shaderPath + ".frag", "");
	deferredGeometryShader = GLShader(shaderPath + "_deferred_geom.vert", shaderPath + "_deferred_geom.frag", "");
	wireframeShader = GLShader(shaderDir + PathSepChar + "wireframe.vert", shaderDir + PathSepChar + "wireframe.frag", shaderDir + PathSepChar + "wireframe.geom");
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, std::string texName, const std::string& shaderDirectory, const std::string& shaderName) {
	resLoaderRef = resLoader;
	texNames.push_back(texName);
	resLoader->CacheStamp(cacheTime);
	texCache.push_back(resLoader->GetTexID(texName));

	shaderDir = shaderDirectory;
	shaderPath = shaderDir + PathSepChar + shaderName;

	shader = GLShader(shaderPath + ".vert", shaderPath + ".frag", "");
	deferredGeometryShader = GLShader(shaderPath + "_deferred_geom.vert", shaderPath + "_deferred_geom.frag", "");
	wireframeShader = GLShader(shaderDir + PathSepChar + "wireframe.vert", shaderDir + PathSepChar + "wireframe.frag", shaderDir + PathSepChar + "wireframe.geom");
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, std::vector<std::string> inTexNames, const std::string& shaderDirectory, const std::string& shaderName)
	: texNames(inTexNames) {
	resLoaderRef = resLoader;
	resLoader->CacheStamp(cacheTime);
	texCache.resize(inTexNames.size(), 0);
	for (size_t i = 0; i < inTexNames.size(); i++)
		texCache[i] = resLoader->GetTexID(inTexNames[i]);

	shaderDir = shaderDirectory;
	shaderPath = shaderDir + PathSepChar + shaderName;

	shader = GLShader(shaderPath + ".vert", shaderPath + ".frag", "");
	deferredGeometryShader = GLShader(shaderPath + "_deferred_geom.vert", shaderPath + "_deferred_geom.frag", "");
	wireframeShader = GLShader(shaderDir + PathSepChar + "wireframe.vert", shaderDir + PathSepChar + "wireframe.frag", shaderDir + PathSepChar + "wireframe.geom");
}

GLShader& GLMaterial::GetShader() {
	return shader;
}

GLShader& GLMaterial::GetDeferredGeometryShader() {
	return deferredGeometryShader;
}

GLShader& GLMaterial::GetWireframeShader() {
	return wireframeShader;
}

GLuint GLMaterial::GetTexID(uint32_t index) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.
		for (size_t i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}
	return texCache[index];
}

std::string GLMaterial::GetTexName(uint32_t index) {
	if (index < texNames.size())
		return texNames[index];

	return "";
}

void GLMaterial::BindTextures(GLShader& shaderRef, GLfloat largestAF, const bool hasEnvMapping, const bool hasGlowmap, const bool hasBacklightMap, const bool hasLightmask) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.
		for (size_t i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}

	shaderRef.BindTexture(0, 0, "texDiffuse");
	shaderRef.BindTexture(1, 0, "texNormal");
	shaderRef.BindTexture(2, 0, "texGlowmap");
	shaderRef.BindTexture(3, 0, "texGreyscale");
	shaderRef.BindCubemap(4, 0, "texCubemap");
	shaderRef.BindTexture(5, 0, "texEnvMask");
	shaderRef.BindTexture(7, 0, "texSpecular");
	shaderRef.BindTexture(7, 0, "texBacklight");
	shaderRef.BindTexture(20, 0, "texAlphaMask");

	for (GLint id = 0; id < static_cast<GLint>(texCache.size()); id++) {
		switch (id) {
			case 0:
				if (shaderRef.BindTexture(id, texCache[id], "texDiffuse")) {
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				break;

			case 1:
				if (shaderRef.BindTexture(id, texCache[id], "texNormal")) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);

					shaderRef.SetNormalMapEnabled(true);
				}
				else
					shaderRef.SetNormalMapEnabled(false);
				break;

			case 2:
				if (hasGlowmap) {
					shaderRef.SetRimlightEnabled(false);
					shaderRef.SetSoftlightEnabled(false);

					if (shaderRef.BindTexture(id, texCache[id], "texGlowmap")) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shaderRef.SetGlowmapEnabled(false);
				}
				else if (hasLightmask) {
					if (shaderRef.BindTexture(id, texCache[id], "texLightmask")) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
				}
				break;

			case 3:
				if (shaderRef.BindTexture(id, texCache[id], "texGreyscale")) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				break;

			case 4:
				if (hasEnvMapping) {
					if (shaderRef.BindCubemap(id, texCache[id], "texCubemap")) {
						if (largestAF)
							glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shaderRef.SetCubemapEnabled(false);
				}
				else
					shaderRef.SetCubemapEnabled(false);
				break;

			case 5:
				if (hasEnvMapping) {
					if (shaderRef.BindTexture(id, texCache[id], "texEnvMask")) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);

						shaderRef.SetEnvMaskEnabled(true);
					}
					else
						shaderRef.SetEnvMaskEnabled(false);
				}
				break;

			case 7:
				if (!hasBacklightMap) {
					if (shaderRef.BindTexture(id, texCache[id], "texSpecular")) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
				}
				else {
					if (shaderRef.BindTexture(id, texCache[id], "texBacklight")) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shaderRef.SetBacklightEnabled(false);
				}
				break;

			case 20:
				// Internal use for compositing and postprocessing textures, not represented by game textures.
				if (shaderRef.BindTexture(id, texCache[id], "texAlphaMask")) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);

					shaderRef.SetAlphaMaskEnabled(true);
				}
				else
					shaderRef.SetAlphaMaskEnabled(false);
				break;
		}
	}
}
