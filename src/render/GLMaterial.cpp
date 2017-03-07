#include "GLMaterial.h"

GLMaterial::GLMaterial() {
}

GLMaterial::~GLMaterial() {
}

// Shader-only material, does not contain texture references, and thus does not use reference to res loader.
GLMaterial::GLMaterial(const string& vertShaderProg, const string& fragShaderProg) {
	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, string texName, const string& vertShaderProg, const string& fragShaderProg) {
	resLoaderRef = resLoader;
	texNames.push_back(texName);
	resLoader->CacheStamp(cacheTime);
	texCache.push_back(resLoader->GetTexID(texName));
	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLMaterial::GLMaterial(ResourceLoader* resLoader, vector<string> inTexNames, const string& vertShaderProg, const string& fragShaderProg) {
	resLoaderRef = resLoader;
	texNames = inTexNames;
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

const string& GLMaterial::GetTexName(uint index) {
	if (index < texNames.size()) {
		return texNames[index];
	}
}

void GLMaterial::BindTextures(GLfloat largestAF, const bool hasBacklight) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.  
		for (int i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}

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
			// Internal use for compositing and postprocessing textures, not represented by game textures.  
		case 2:
			if (texCache[id] != 0) {
				shader.BindTexture(id, texCache[id], "texAlphaMask");
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				shader.SetAlphaMaskEnabled(true);
			}
			else {
				shader.SetAlphaMaskEnabled(false);
			}

		case 4:
			if (texCache[id] != 0) {
				shader.BindCubemap(id, texCache[id], "texCubemap");
				if (largestAF) glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				shader.SetCubemapEnabled(true);
			}
			else
				shader.SetCubemapEnabled(false);
			break;

		case 5:
			if (texCache[id] != 0) {
				shader.BindTexture(id, texCache[id], "texEnvMask");
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				shader.SetEnvMaskEnabled(true);
			}
			else
				shader.SetEnvMaskEnabled(false);
			break;

		case 7:
			if (!hasBacklight) {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texSpecular");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
			}
			else {
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texBacklight");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetBacklightEnabled(true);
				}
				else
					shader.SetBacklightEnabled(false);
			}
			break;
		}
	}
}
