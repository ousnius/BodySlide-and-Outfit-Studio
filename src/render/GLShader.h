/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Mesh.h"
#include "GLExtensions.h"

class GLShader {
	static bool extChecked;

	// Source text for shaders
	std::string vertSrc;
	std::string fragSrc;
	std::string geomSrc;

	// Compiled shader IDs after compile.
	GLuint vertShadID = 0;
	GLuint fragShadID = 0;
	GLuint geomShadID = 0;

	// Linked Program ID after program creation.
	GLuint progID = 0;

	/* error state, set if compile/link fails.  check errorstring for compile log
		-1 = initial state -- not ready
		0 = no error, shader ready
		1 = shaders not supported
		10 = vertex shader file not found
		11 = fragment shader file not found
		12 = geometry shader file not found
		20 = vertex shader compile failed
		21 = fragment shader compile failed
		22 = geometry shader compile failed
		30 = program link failed
		*/
	int errorState = -1;
	std::string errorString;

	GLuint whiteTexID = 0;
	GLuint whiteCubemapTexID = 0;
	GLuint noiseTexID = 0;

	bool CheckExtensions();
	bool LoadShaderFile(const std::string& fileName, std::string& text);

	// Attempts to load the specified source files (in text format).
	bool LoadShaders(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource);

	// Compiles and links the loaded shaders, generating a program that can be activated.
	bool BuildShaders();

	GLuint CreateWhiteTexture();
	GLuint CreateWhiteCubemap();

public:
	struct DirectionalLight {
		nifly::Vector3 diffuse;
		nifly::Vector3 direction;
	};

	GLShader() {}

	// Creates the shader object and runs LoadShaders followed by BuildShaders.
	GLShader(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource);
	~GLShader();

	GLuint CreateNoiseTexture();

	void SetColor(const nifly::Vector3& color);
	void SetSubColor(const nifly::Vector3& color);
	void SetBackgroundColor(const nifly::Vector3& color);
	void SetModelSpace(const bool enable);
	void SetEmissive(const bool enable);
	void SetLightingEnabled(const bool enable);
	void SetMatrixProjection(const glm::mat4x4& mat);
	void SetMatrixModelView(const glm::mat4x4& matView, const glm::mat4x4& matModel);
	void SetAlphaProperties(const uint16_t flags, const float threshold, const float value);
	void SetAlphaThreshold(const float threshold);
	void SetAdjustPointSize(const bool enable);

	void SetFrontalLight(const DirectionalLight& light);
	void SetDirectionalLight(const DirectionalLight& light, const int index);
	void SetAmbientLight(const float light);
	void SetProperties(const Mesh::ShaderProperties& prop);

	void ShowLighting(bool bShow = true);
	void ShowMask(bool bShow = true);
	void ShowWeight(bool bShow = true);
	void ShowVertexColors(bool bShow = true);
	void ShowVertexAlpha(bool bShow = true);
	void ShowTexture(bool bShow = true);

	void SetNormalMapEnabled(const bool enable);
	void SetAlphaMaskEnabled(const bool enable);
	void SetGreyscaleColorEnabled(const bool enable);
	void SetCubemapEnabled(const bool enable);
	void SetEnvMaskEnabled(const bool enable);
	void SetSpecularEnabled(const bool enable);
	void SetBacklightEnabled(const bool enable);
	void SetRimlightEnabled(const bool enable);
	void SetSoftlightEnabled(const bool enable);
	void SetGlowmapEnabled(const bool enable);
	bool BindTexture(const GLint& index, const GLuint& texture, const char* samplerName);
	bool BindTextureMultisample(const GLint& index, const GLuint& texture, const char* samplerName);
	bool BindCubemap(const GLint& index, const GLuint& texture, const char* samplerName);

	void SetInt(const char* uniformName, const GLint index);
	void SetVec2(const char* uniformName, const nifly::Vector2& vec2);
	void SetVec3(const char* uniformName, const nifly::Vector3& vec3);
	void SetVec3Array(const char* uniformName, const std::vector<nifly::Vector3>& vec3Array);

	int GetErrorState();
	bool GetError(std::string* errorStr = nullptr);

	// Activates the stored program for subsequent GL rendering calls.
	bool Begin();

	// Turns off the stored program.
	void End();
};
