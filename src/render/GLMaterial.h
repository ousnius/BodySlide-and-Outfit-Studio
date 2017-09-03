/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "..\files\ResourceLoader.h"
#include "GLShader.h"

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

	GLuint GetTexID(uint index);
	std::string GetTexName(uint index);

	void BindTextures(GLfloat largestAF, const bool hasEnvMapping, const bool hasGlowmap, const bool hasBacklight, const bool hasLightmask);
};
