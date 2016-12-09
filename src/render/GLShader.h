/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
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

class GLMaterial {
private:
	vector<GLuint> texRef;
	GLShader shader;

public:
	GLMaterial();
	~GLMaterial() {
		glDeleteTextures(texRef.size(), texRef.data());
	}

	GLMaterial(const string& vertShaderProg, const string& fragShaderProg) {
		shader = GLShader(vertShaderProg, fragShaderProg);
	}

	GLMaterial(GLuint texID, const string& vertShaderProg, const string& fragShaderProg) {
		texRef.clear();
		texRef.push_back(texID);
		shader = GLShader(vertShaderProg, fragShaderProg);
	}

	GLMaterial(GLuint texID[], int texCount, const string& vertShaderProg, const string& fragShaderProg) {
		texRef.resize(texCount, 0);
		for (int i = 0; i < texCount; i++)
			texRef[i] = texID[i];

		shader = GLShader(vertShaderProg, fragShaderProg);
	}

	GLShader& GetShader() {
		return shader;
	}

	void BindTextures(GLfloat largestAF, const bool hasBacklight) {
		for (int id = 0; id < texRef.size(); id++) {
			switch (id) {
			case 0:
				shader.BindTexture(id, texRef[id], "texDiffuse");
				if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
				break;

			case 1:
				if (texRef[id] != 0) {
					shader.BindTexture(id, texRef[id], "texNormal");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetNormalMapEnabled(true);
				}
				else
					shader.SetNormalMapEnabled(false);
				break;

			case 4:
				if (texRef[id] != 0) {
					shader.BindCubemap(id, texRef[id], "texCubemap");
					if (largestAF) glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetCubemapEnabled(true);
				}
				else
					shader.SetCubemapEnabled(false);
				break;

			case 5:
				if (texRef[id] != 0) {
					shader.BindTexture(id, texRef[id], "texEnvMask");
					if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
					shader.SetEnvMaskEnabled(true);
				}
				else
					shader.SetEnvMaskEnabled(false);
				break;

			case 7:
				if (!hasBacklight) {
					if (texRef[id] != 0) {
						shader.BindTexture(id, texRef[id], "texSpecular");
						if (largestAF) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
						shader.SetSpecularEnabled(true);
					}
					else
						shader.SetSpecularEnabled(false);
				}
				else {
					if (texRef[id] != 0) {
						shader.BindTexture(id, texRef[id], "texBacklight");
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
};
