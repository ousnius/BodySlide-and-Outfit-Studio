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
	vector<string> texNames;
	// Cache of texture IDs. These are direct OGL texture ids used to bind textures and avoid lookups to 
	//  the resource loader's texture database each frame.  
	vector<GLuint> texCache;
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
	GLMaterial(const string& vertShaderProg, const string& fragShaderProg);
	GLMaterial(ResourceLoader* resLoader, string texName, const string& vertShaderProg, const string& fragShaderProg);
	GLMaterial(ResourceLoader* resLoader, vector<string> inTexNames, const string& vertShaderProg, const string& fragShaderProg);

	GLShader& GetShader();

	GLuint GetTexID(uint index);
	string GetTexName(uint index);

	void BindTextures(GLfloat largestAF, const bool hasBacklight);
};
