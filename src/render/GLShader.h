/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../NIF/utils/Object3d.h"
#include "../components/Mesh.h"
#include "GLExtensions.h"

class GLShader {
	static bool extChecked;

	// Source text for shaders
	std::string vertSrc;
	std::string fragSrc;

	// Compiled shader IDs after compile.
	GLuint vertShadID = 0;
	GLuint fragShadID = 0;
	GLint vertShadLength = 0;
	GLint fragShadLength = 0;

	// Linked Program ID after program creation.
	GLuint progID = 0;

	/* error state, set if compile/link fails.  check errorstring for compile log
		-1 = initial state -- not ready
		0 = no error, shader ready
		1 = shaders not supported,
		2 = vertex shader compile failed,
		3 = fragment shader compile failed,
		4 = program link failed.
		*/
	int errorState = -1;
	std::string errorString;

	bool CheckExtensions();
	bool LoadShaderFile(const std::string& fileName, std::string& text);

	// Attempts to load the specified source files (in text format).
	bool LoadShaders(const std::string& vertexSource, const std::string& fragmentSource);

	// Compiles and links the loaded shaders, generating a program that can be activated.
	bool BuildShaders();

public:
	struct DirectionalLight {
		Vector3 diffuse;
		Vector3 direction;
	};

	GLShader() {}

	// Creates the shader object and runs LoadShaders followed by BuildShaders.
	GLShader(const std::string& vertexSource, const std::string& fragmentSource);

	void SetColor(const Vector3& color);
	void SetModelSpace(const bool enable);
	void SetEmissive(const bool enable);
	void SetWireframeEnabled(const bool enable);
	void SetPointsEnabled(const bool enable);
	void SetLightingEnabled(const bool enable);
	void SetMatrixProjection(const glm::mat4x4& mat);
	void SetMatrixModelView(const glm::mat4x4& matView, const glm::mat4x4& matModel);
	void SetAlphaProperties(const ushort flags, const float threshold, const float value);
	void SetAlphaThreshold(const float threshold);

	void SetFrontalLight(const DirectionalLight& light);
	void SetDirectionalLight(const DirectionalLight& light, const int index);
	void SetAmbientLight(const float light);
	void SetProperties(const mesh::ShaderProperties& prop);

	void ShowLighting(bool bShow = true);
	void ShowMask(bool bShow = true);
	void ShowWeight(bool bShow = true);
	void ShowColors(bool bShow = true);
	void ShowSegments(bool bShow = true);
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
	void BindTexture(const GLint& index, const GLuint& texture, const char* samplerName);
	void BindCubemap(const GLint& index, const GLuint& texture, const char* samplerName);

	bool GetError(std::string* errorStr = nullptr);

	// Activates the stored program for subsequent GL rendering calls.
	int Begin();

	// Turns off the stored program.
	void End();
};
