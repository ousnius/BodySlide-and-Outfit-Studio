/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "GLShader.h"
#include <fstream>
#include <sstream>

#pragma warning (disable : 4130)

GLShader::GLShader() {
	errorState = -1;
}

GLShader::GLShader(const string& vertexSource, const string& fragmentSource) : GLShader() {
	if (CheckExtensions() && LoadShaders(vertexSource, fragmentSource)) {
		ShowLighting();
		ShowTexture();
		ShowMask();
		ShowWeight(false);
		ShowSegments(false);

		SetColor(Vector3(1.0f, 1.0f, 1.0f));
		SetModelSpace(false);
		SetEmissive(false);
		SetWireframeEnabled(false);
		SetPointsEnabled(false);
		SetLightingEnabled(true);
	}
}

GLShader::~GLShader() {
}

bool GLShader::extChecked = false;

bool GLShader::CheckExtensions() {
	if (!extChecked) {
		if (!extSupported) {
			errorState = 1;
			errorString = "OpenGL: One or more extensions are not supported.";
		}

		extChecked = true;
	}

	return extSupported;
}

bool GLShader::LoadShaderFile(const string& fileName, string& text) {
	ifstream file(fileName, ios_base::in | ios_base::binary);
	if (!file)
		return false;

	stringstream buffer;
	buffer << file.rdbuf();

	text = buffer.str();

	if (text.empty())
		return false;

	return true;
}

bool GLShader::LoadShaders(const string& vertexSource, const string& fragmentSource) {
	if (!LoadShaderFile(vertexSource, vertSrc)) {
		errorState = 2;
		errorString = "OpenGL: Failed to load vertex shader from file: " + vertexSource;
		return false;
	}

	if (!LoadShaderFile(fragmentSource, fragSrc)) {
		errorState = 3;
		errorString = "OpenGL: Failed to load fragment shader from file: " + fragmentSource;
		return false;
	}

	return BuildShaders();
}

void GLShader::SetColor(const Vector3& color) {
	GLint loc = glGetUniformLocation(progID, "color");
	if (loc >= 0)
		glUniform3f(loc, color.x, color.y, color.z);
}

void GLShader::SetModelSpace(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bModelSpace");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetEmissive(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bEmissive");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetWireframeEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bWireframe");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetPointsEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bPoints");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetLightingEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bLighting");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetMatrixProjection(const glm::mat4x4& mat) {
	GLint loc = glGetUniformLocation(progID, "matProjection");
	if (loc >= 0)
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&mat);
}

void GLShader::SetMatrixModelView(const glm::mat4x4& mat) {
	GLint loc = glGetUniformLocation(progID, "matModelView");
	if (loc >= 0)
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&mat);
}

void GLShader::SetAlphaProperties(const ushort flags, const float threshold) {
	static const GLenum blendMap[16] = {
		GL_ONE, GL_ZERO, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
		GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA_SATURATE, GL_ONE,
		GL_ONE, GL_ONE, GL_ONE, GL_ONE
	};

	//static const GLenum testMap[8] = {
	//	GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
	//};

	bool alphaBlend = flags & 1;
	bool alphaTest = (flags & (1 << 9)) != 0;
	GLenum alphaSrc = blendMap[(flags >> 1) & 0x0F];
	GLenum alphaDst = blendMap[(flags >> 5) & 0x0F];

	//Always GL_GREATER right now
	//GLenum alphaFunc = testMap[(flags >> 10) & 0x7];

	if (alphaBlend)
		glBlendFunc(alphaSrc, alphaDst);
	else
		glDisable(GL_BLEND);

	if (alphaTest)
		SetAlphaThreshold(threshold);
	else
		SetAlphaThreshold(0.0f);
}

void GLShader::SetAlphaThreshold(const float threshold) {
	GLint loc = glGetUniformLocation(progID, "alphaThreshold");
	if (loc >= 0)
		glUniform1f(loc, threshold);
}

void GLShader::SetFrontalLight(const FrontalLight& light) {
	GLint loc = glGetUniformLocation(progID, "frontal.diffuse");
	if (loc >= 0)
		glUniform3f(loc, light.diffuse.x, light.diffuse.y, light.diffuse.z);
}

void GLShader::SetDirectionalLight(const DirectionalLight& light, const int index) {
	string lightName = "directional" + to_string(index);
	string lightDiffuse = lightName + ".diffuse";
	string lightDirection = lightName + ".direction";

	GLint loc = glGetUniformLocation(progID, lightDiffuse.c_str());
	if (loc >= 0)
		glUniform3f(loc, light.diffuse.x, light.diffuse.y, light.diffuse.z);

	loc = glGetUniformLocation(progID, lightDirection.c_str());
	if (loc >= 0)
		glUniform3f(loc, light.direction.x, light.direction.y, light.direction.z);
}

void GLShader::SetAmbientLight(const float light) {
	GLint loc = glGetUniformLocation(progID, "ambient");
	if (loc >= 0)
		glUniform1f(loc, light);
}

void GLShader::SetProperties(const mesh::ShaderProperties& prop) {
	GLint loc = glGetUniformLocation(progID, "prop.uvOffset");
	if (loc >= 0)
		glUniform2f(loc, prop.uvOffset.u, prop.uvOffset.v);

	loc = glGetUniformLocation(progID, "prop.uvScale");
	if (loc >= 0)
		glUniform2f(loc, prop.uvScale.u, prop.uvScale.v);

	loc = glGetUniformLocation(progID, "prop.specularColor");
	if (loc >= 0)
		glUniform3f(loc, prop.specularColor.x, prop.specularColor.y, prop.specularColor.z);

	loc = glGetUniformLocation(progID, "prop.specularStrength");
	if (loc >= 0)
		glUniform1f(loc, prop.specularStrength);

	loc = glGetUniformLocation(progID, "prop.shininess");
	if (loc >= 0)
		glUniform1f(loc, prop.shininess);

	loc = glGetUniformLocation(progID, "prop.envReflection");
	if (loc >= 0)
		glUniform1f(loc, prop.envReflection);

	loc = glGetUniformLocation(progID, "prop.emissiveColor");
	if (loc >= 0)
		glUniform3f(loc, prop.emissiveColor.x, prop.emissiveColor.y, prop.emissiveColor.z);

	loc = glGetUniformLocation(progID, "prop.emissiveMultiple");
	if (loc >= 0)
		glUniform1f(loc, prop.emissiveMultiple);

	loc = glGetUniformLocation(progID, "prop.alpha");
	if (loc >= 0)
		glUniform1f(loc, prop.alpha);
}

void GLShader::ShowLighting(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bLightEnabled");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::ShowMask(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowMask");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::ShowWeight(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowWeight");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::ShowSegments(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowSegments");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::ShowTexture(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowTexture");
	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::SetNormalMapEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bNormalMap");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetCubemapEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bCubemap");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetEnvMaskEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bEnvMask");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetSpecularEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bSpecular");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetBacklightEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bBacklight");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::BindTexture(const GLint& index, const GLuint& texture, const string& samplerName) {
	GLint texLoc = glGetUniformLocation(progID, samplerName.c_str());
	if (texLoc >= 0) {
		glUniform1i(texLoc, index);
		glActiveTexture(GL_TEXTURE0 + index);

		glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glBindTexture(GL_TEXTURE_2D, texture);
	}
}

void GLShader::BindCubemap(const GLint& index, const GLuint& texture, const string& samplerName) {
	GLint texLoc = glGetUniformLocation(progID, samplerName.c_str());
	if (texLoc >= 0) {
		glUniform1i(texLoc, index);
		glActiveTexture(GL_TEXTURE0 + index);
		
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	}
}

bool GLShader::GetError(string* errorStr) {
	if (!errorState)
		return false;

	if (errorStr)
		(*errorStr) = errorString;

	return true;
}

bool GLShader::BuildShaders() {
	const GLchar* src = nullptr;
	GLint compiled;
	GLint loglength;

	vertShadID = glCreateShader(GL_VERTEX_SHADER);
	src = (const GLchar*)vertSrc.c_str();
	glShaderSource(vertShadID, 1, &src, nullptr);

	glCompileShader(vertShadID);
	glGetShaderiv(vertShadID, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glGetShaderiv(vertShadID, GL_INFO_LOG_LENGTH, &loglength);
		GLchar* logdata = new GLchar[loglength];
		glGetShaderInfoLog(vertShadID, loglength, nullptr, logdata);
		errorString = string("OpenGL: Vertex shader compile failed: ") + logdata;
		delete[] logdata;

		errorState = 2;
		return false;
	}

	fragShadID = glCreateShader(GL_FRAGMENT_SHADER);
	src = (const GLchar*)fragSrc.c_str();
	glShaderSource(fragShadID, 1, &src, nullptr);

	glCompileShader(fragShadID);
	glGetShaderiv(fragShadID, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glGetShaderiv(fragShadID, GL_INFO_LOG_LENGTH, &loglength);
		GLchar* logdata = new GLchar[loglength];
		glGetShaderInfoLog(fragShadID, loglength, nullptr, logdata);
		errorString = string("OpenGL: Fragment shader compile failed: ") + logdata;
		delete[] logdata;

		errorState = 3;
		return false;
	}

	progID = glCreateProgram();

	glAttachShader(progID, vertShadID);
	glAttachShader(progID, fragShadID);

	glLinkProgram(progID);
	glGetProgramiv(progID, GL_LINK_STATUS, &compiled);
	if (!compiled) {
		glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &loglength);
		GLchar* logdata = new GLchar[loglength];
		glGetProgramInfoLog(progID, loglength, nullptr, logdata);
		errorString = string("OpenGL: Shader program link failed: ") + logdata;
		delete[] logdata;

		errorState = 4;
		return false;
	}

	errorString = "OpenGL: Shader program ready.";
	errorState = 0;
	return true;
}

int GLShader::Begin() {
	if (errorState == 0)
		glUseProgram(progID);

	return !errorState;
}

void GLShader::End() {
	glUseProgram(0);
}
