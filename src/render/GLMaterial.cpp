/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLMaterial.h"

using namespace nifly;

GLMaterial::GLMaterial() {}

GLMaterial::~GLMaterial() {}

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
	for (size_t i = 0; i < inTexNames.size(); i++)
		texCache[i] = resLoader->GetTexID(inTexNames[i]);

	shader = GLShader(vertShaderProg, fragShaderProg);
}

GLShader& GLMaterial::GetShader() {
	return shader;
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

bool GLMaterial::HasTexture(uint32_t index) {
	if (index >= texCache.size())
		return false;

	return GetTexID(index) != 0;
}

bool GLMaterial::IsComplexMaterial(uint32_t index) {
	if (!resLoaderRef || index >= texNames.size())
		return false;

	return resLoaderRef->IsComplexMaterialTexture(texNames[index]);
}

int GLMaterial::GetTexMaxMipLevel(uint32_t index) {
	if (!resLoaderRef || index >= texNames.size())
		return 0;

	return resLoaderRef->GetTextureMaxMipLevel(texNames[index]);
}

int GLMaterial::GetCubemapSize(uint32_t index) {
	if (!resLoaderRef || index >= texNames.size())
		return 0;

	return resLoaderRef->GetCubemapSize(texNames[index]);
}

Vector3 GLMaterial::GetCubemapF0Color(uint32_t index) {
	if (!resLoaderRef || index >= texNames.size())
		return Vector3(1.0f, 1.0f, 1.0f);

	return resLoaderRef->GetCubemapF0Color(texNames[index]);
}

std::string GLMaterial::GetTexName(uint32_t index) {
	if (index < texNames.size())
		return texNames[index];

	return "";
}

void GLMaterial::BindTextures(GLfloat largestAF,
							  const bool hasEnvMapping,
							  const bool hasGlowmap,
							  const bool hasBacklightMap,
							  const bool hasLightmask,
							  const GLuint dynamicCubemapID,
							  const bool isPBR) {
	if (resLoaderRef && !resLoaderRef->CacheStamp(cacheTime)) {
		// outdated cache, rebuild it.
		for (size_t i = 0; i < texCache.size(); i++) {
			texCache[i] = resLoaderRef->GetTexID(texNames[i]);
		}
	}

	shader.BindTexture(0, 0, "texDiffuse");
	shader.BindTexture(1, 0, "texNormal");
	shader.BindTexture(2, 0, "texGlowmap");
	shader.BindTexture(2, 0, "texEmissive");
	shader.BindTexture(3, 0, "texGreyscale");
	shader.BindCubemap(4, 0, "texCubemap");
	shader.BindTexture(5, 0, "texEnvMask");
	shader.BindTexture(5, 0, "texRMAOS");
	shader.BindTexture(6, 0, "texFaceTint");
	shader.BindTexture(7, 0, "texSpecular");
	shader.BindTexture(7, 0, "texBacklight");
	shader.BindTexture(20, 0, "texAlphaMask");

	for (GLint id = 0; id < static_cast<GLint>(texCache.size()); id++) {
		switch (id) {
			case 0:
				shader.BindTexture(id, texCache[id], "texDiffuse");
				if (largestAF)
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				break;

			case 1:
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texNormal");
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetNormalMapEnabled(true);
				}
				else
					shader.SetNormalMapEnabled(false);
				break;

			case 2:
				if (isPBR) {
					// Emissive color rather than a glow map. Community Shaders gates it on the texture
					// being there rather than on a flag, since the patcher that writes these meshes
					// clears the glow map flag on its way past.
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texEmissive");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
						shader.SetPBREmissiveEnabled(true);
					}
					else
						shader.SetPBREmissiveEnabled(false);
				}
				else if (hasGlowmap) {
					shader.SetRimlightEnabled(false);
					shader.SetSoftlightEnabled(false);

					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texGlowmap");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shader.SetGlowmapEnabled(false);
				}
				else if (hasLightmask) {
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texLightmask");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
				}
				break;

			case 3:
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texGreyscale");
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				break;

			case 4:
				if (hasEnvMapping) {
					// A dynamic cube map stands in for whatever the slot resolved to, which includes
					// the nothing an env mapped shape with a missing cube map file would get.
					const GLuint cubemapID = dynamicCubemapID != 0 ? dynamicCubemapID : texCache[id];
					if (cubemapID != 0) {
						shader.BindCubemap(id, cubemapID, "texCubemap");
						// A Complex Material picks its mip from roughness, so the levels have to blend
						// into each other - without this the reflection steps visibly as glossiness
						// varies across the surface.
						glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shader.SetCubemapEnabled(false);
				}
				break;

			case 5:
				if (isPBR) {
					// Roughness, metalness, ambient occlusion and specular level, not an environment
					// mask. Bound whatever the shape says about environment mapping, which True PBR
					// meshes leave switched off - this map is what shades the surface at all.
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texRMAOS");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
						shader.SetRMAOSEnabled(true);
					}
					else
						shader.SetRMAOSEnabled(false);
				}
				else if (hasEnvMapping) {
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texEnvMask");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
						shader.SetEnvMaskEnabled(true);
					}
					else
						shader.SetEnvMaskEnabled(false);
				}
				break;

			case 6:
				// Face tint map (FaceGen), only used by the face tint shader type
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texFaceTint");
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				}
				else
					shader.SetFaceTintEnabled(false);
				break;

			case 7:
				if (!hasBacklightMap) {
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texSpecular");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
				}
				else {
					if (texCache[id] != 0) {
						shader.BindTexture(id, texCache[id], "texBacklight");
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						if (largestAF)
							glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					}
					else
						shader.SetBacklightEnabled(false);
				}
				break;

			case 20:
				// Internal use for compositing and postprocessing textures, not represented by game textures.
				if (texCache[id] != 0) {
					shader.BindTexture(id, texCache[id], "texAlphaMask");
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (largestAF)
						glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetAlphaMaskEnabled(true);
				}
				else
					shader.SetAlphaMaskEnabled(false);
				break;
		}
	}
}
