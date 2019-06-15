/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLMaterial.h"

GLMaterial::GLMaterial() {
}

GLMaterial::~GLMaterial() {
}

// Shader-only material, does not contain texture references, and thus does not use reference to res loader.
GLMaterial::GLMaterial(const std::string& vertShaderProg, const std::string& fragShaderProg) {
	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, std::string texName, const std::string& vertShaderProg, const std::string& fragShaderProg) {
	resLoaderRef = resLoader;
	texNames.push_back(texName);
	resLoader->CacheStamp(cacheTime);
	texCache.push_back(resLoader->GetTexID(texName));
	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, std::vector<std::string> inTexNames, const std::string& vertShaderProg, const std::string& fragShaderProg)
	: texNames(inTexNames) {
	resLoaderRef = resLoader;
	resLoader->CacheStamp(cacheTime);
	texCache.resize(inTexNames.size(), 0);
	for (int i = 0; i < inTexNames.size(); i++)
		texCache[i] = resLoader->GetTexID(inTexNames[i]);

	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLShader& GLMaterial::GetShader() {
	return shader;
}

GLuint GLMaterial::GetTexID(uint index) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.  
		for (int i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}
	return texCache[index];
}

std::string GLMaterial::GetTexName(uint index) {
	if (index < texNames.size())
		return texNames[index];

	return "";
}

void GLMaterial::BindTextures(GLfloat largestAF, const bool hasEnvMapping, const bool hasGlowmap, const bool hasBacklightMap, const bool hasLightmask) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.  
		for (int i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}

	shader.BindTexture(0, 0, "texDiffuse");
	shader.BindTexture(1, 0, "texNormal");
	shader.BindTexture(2, 0, "texGlowmap");
	shader.BindTexture(3, 0, "texGreyscale");
	shader.BindCubemap(4, 0, "texCubemap");
	shader.BindTexture(5, 0, "texEnvMask");
	shader.BindTexture(7, 0, "texSpecular");
	shader.BindTexture(7, 0, "texBacklight");
	shader.BindTexture(20, 0, "texAlphaMask");

	for (int id = 0; id < texCache.size(); id++) {
		switch (id) {
		case 0:
			shader.BindTexture(id, texCache[id], "texDiffuse");
			if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
			break;

		case 1:
			if (texCache[id] != 0) {
				shader.BindTexture(id, texCache[id], "texNormal");
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				shader.SetNormalMapEnabled(true);
			}
			else
				shader.SetNormalMapEnabled(false);
			break;

		case 2:
			if (hasGlowmap) {
				shader.SetRimlightEnabled(false);
				shader.SetSoftlightEnabled(false);

				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texGlowmap");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				else
					shader.SetGlowmapEnabled(false);
			}
			else if (hasLightmask) {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texLightmask");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
			}
			break;

		case 3:
			if (texCache[id] != 0) {
				shader.BindTexture(id, texCache[id], "texGreyscale");
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
			}
			break;

		case 4:
			if (hasEnvMapping) {
				if (texCache[id] != 0) {
					shader.BindCubemap(id, texCache[id], "texCubemap");
					if (largestAF) glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				else
					shader.SetCubemapEnabled(false);
			}
			break;

		case 5:
			if (hasEnvMapping) {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texEnvMask");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetEnvMaskEnabled(true);
				}
				else
					shader.SetEnvMaskEnabled(false);
			}
			break;

		case 7:
			if (!hasBacklightMap) {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texSpecular");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
			}
			else {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texBacklight");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				else
					shader.SetBacklightEnabled(false);
			}
			break;

		case 20:
			// Internal use for compositing and postprocessing textures, not represented by game textures.
			if (texCache[id] != 0) {
				shader.BindTexture(id, texCache[id], "texAlphaMask");
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				shader.SetAlphaMaskEnabled(true);
			}
			else
				shader.SetAlphaMaskEnabled(false);
			break;
		}
	}
}
