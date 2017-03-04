/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#pragma warning (push, 0)
#include "gli/glm/gtc/matrix_transform.hpp"
#pragma warning (pop)

#include "../utils/Object3d.h"
#include "../components/Mesh.h"
#include "GLExtensions.h"

class GLShader {
	static bool extChecked;

	// Source text for shaders
	string vertSrc;
	string fragSrc;

	// Compiled shader IDs after compile.
	GLuint vertShadID;
	GLuint fragShadID;
	GLint vertShadLength;
	GLint fragShadLength;

	// Linked Program ID after program creation.
	GLuint progID;

	/* error state, set if compile/link fails.  check errorstring for compile log
		-1 = initial state -- not ready
		0 = no error, shader ready
		1 = shaders not supported,
		2 = vertex shader compile failed,
		3 = fragment shader compile failed,
		4 = program link failed.
		*/
	int errorState;
	string errorString;

	bool CheckExtensions();
	bool LoadShaderFile(const string& fileName, string& text);

	// Attempts to load the specified source files (in text format).
	bool LoadShaders(const string& vertexSource, const string& fragmentSource);

	// Compiles and links the loaded shaders, generating a program that can be activated.
	bool BuildShaders();

public:
	struct FrontalLight {
		Vector3 diffuse;
	};

	struct DirectionalLight {
		Vector3 diffuse;
		Vector3 direction;
	};

	GLShader();

	// Creates the shader object and runs LoadShaders followed by BuildShaders.
	GLShader(const string& vertexSource, const string& fragmentSource);
	~GLShader();

	void SetColor(const Vector3& color);
	void SetModelSpace(const bool enable);
	void SetEmissive(const bool enable);
	void SetWireframeEnabled(const bool enable);
	void SetPointsEnabled(const bool enable);
	void SetLightingEnabled(const bool enable);
	void SetMatrixProjection(const glm::mat4x4& mat);
	void SetMatrixModelView(const glm::mat4x4& mat);
	void SetAlphaProperties(const ushort flags, const float threshold);
	void SetAlphaThreshold(const float threshold);

	void SetFrontalLight(const FrontalLight& light);
	void SetDirectionalLight(const DirectionalLight& light, const int index);
	void SetAmbientLight(const float light);
	void SetProperties(const mesh::ShaderProperties& prop);

	void ShowLighting(bool bShow = true);
	void ShowMask(bool bShow = true);
	void ShowWeight(bool bShow = true);
	void ShowSegments(bool bShow = true);
	void ShowTexture(bool bShow = true);

	void SetNormalMapEnabled(const bool enable);
	void SetAlphaMaskEnabled(const bool enable);
	void SetCubemapEnabled(const bool enable);
	void SetEnvMaskEnabled(const bool enable);
	void SetSpecularEnabled(const bool enable);
	void SetBacklightEnabled(const bool enable);
	void BindTexture(const GLint& index, const GLuint& texture, const string& samplerName);
	void BindCubemap(const GLint& index, const GLuint& texture, const string& samplerName);

	bool GetError(string* errorStr = nullptr);

	// Activates the stored program for subsequent GL rendering calls.
	int Begin();

	// Turns off the stored program.
	void End();
};

class ResourceLoader;

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
	_int64 cacheTime;
	GLShader shader;
	ResourceLoader* resLoaderRef;

public:
	GLMaterial();
	~GLMaterial();

	// Shader-only material, does not contain texture references, and thus does not use reference to res loader.
	GLMaterial(const string& vertShaderProg, const string& fragShaderProg);


	GLMaterial(ResourceLoader* resLoader, string texName, const string& vertShaderProg, const string& fragShaderProg);

	GLMaterial(ResourceLoader* resLoader, vector<string> inTexNames, const string& vertShaderProg, const string& fragShaderProg);

	GLShader& GetShader();

	GLuint GetTexID(uint index);
	const string& GetTexName(uint index) {
		if (index < texNames.size()) {
			return texNames[index];
		}
	}

	void BindTextures(GLfloat largestAF, const bool hasBacklight);
};
