/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "../files/ResourceLoader.h"
#include "GLShader.h"

#include <cstdint>
#include <string>
#include <vector>

class GLMaterial {
private:
	// texture names linked with this material.  Used to lookup OGL texture ids in Resource Loader.
	std::vector<std::string> texNames;
	// Cache of texture IDs. These are direct OGL texture ids used to bind textures and avoid lookups to
	//  the resource loader's texture database each frame.
	std::vector<GLuint> texCache;
	// A value indicating the last time the cache was updated from the resource loader. This isn't a time,
	//  but instead a numeric indicator of change state. This is checked prior to binding textures, and if
	//  a change has happened, texids are refreshed from ResourceLoader based on texNames.
	int64_t cacheTime = 0;
	GLShader shader;
	ResourceLoader* resLoaderRef = nullptr;

public:
	GLMaterial();
	~GLMaterial();

	// Shader-only material, does not contain texture references, and thus does not use reference to res loader.
	GLMaterial(const std::string& vertShaderProg, const std::string& fragShaderProg);
	GLMaterial(ResourceLoader* resLoader, std::string texName, const std::string& vertShaderProg, const std::string& fragShaderProg);
	GLMaterial(ResourceLoader* resLoader, std::vector<std::string> inTexNames, const std::string& vertShaderProg, const std::string& fragShaderProg);

	GLShader& GetShader();

	GLuint GetTexID(uint32_t index);
	std::string GetTexName(uint32_t index);
	// Whether the slot exists in this material and resolves to a loaded texture.
	bool HasTexture(uint32_t index);

	// Whether the texture in the slot was classified as a Complex Material mask.
	bool IsComplexMaterial(uint32_t index);
	// Highest mip level the texture in the slot has, 0 for one without a mip chain.
	int GetTexMaxMipLevel(uint32_t index);
	// Edge length of the cube map in the slot, 0 for a slot that holds anything else.
	int GetCubemapSize(uint32_t index);
	// F0 reflectance a 1x1 cube map in the slot stands for, 1.0 for any other cube map.
	nifly::Vector3 GetCubemapF0Color(uint32_t index);

	// dynamicCubemapID replaces whatever the cube map slot resolved to, including nothing at all.
	// Whether a shape has earned that is decided by the caller, so passing one here means it has.
	// isPBR re-reads two slots the way Community Shaders' True PBR fills them rather than the way
	// vanilla does: slot 5 as an RMAOS map instead of an environment mask, and slot 2 as an emissive
	// color instead of a glow map. The rest of the layout the two have in common.
	void BindTextures(GLfloat largestAF,
					  const bool hasEnvMapping,
					  const bool hasGlowmap,
					  const bool hasBacklight,
					  const bool hasLightmask,
					  const GLuint dynamicCubemapID = 0,
					  const bool isPBR = false);
};
