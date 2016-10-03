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
	if (CheckExtensions())
		LoadShaders(vertexSource, fragmentSource);
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

void GLShader::SetWireframeEnabled(const bool& enable) {
	GLint loc = glGetUniformLocation(progID, "bWireframe");
	if (loc >= 0)
		glUniform1f(loc, enable ? 1.0f : 0.0f);
}

void GLShader::SetPointsEnabled(const bool& enable) {
	GLint loc = glGetUniformLocation(progID, "bPoints");
	if (loc >= 0)
		glUniform1f(loc, enable ? 1.0f : 0.0f);
}

void GLShader::SetLightingEnabled(const bool& enable) {
	GLint loc = glGetUniformLocation(progID, "bLighting");
	if (loc >= 0)
		glUniform1f(loc, enable ? 1.0f : 0.0f);
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

void GLShader::SetLight(const uint& index, const LightSource& lightSource) {
	string light = "lightSource" + to_string(index);
	string lightAmbient = light + ".ambient";
	string lightDiffuse = light + ".diffuse";
	string lightSpecular = light + ".specular";
	string lightPosition = light + ".position";

	GLint loc = glGetUniformLocation(progID, lightAmbient.c_str());
	if (loc >= 0)
		glUniform3f(loc, lightSource.ambient.x, lightSource.ambient.y, lightSource.ambient.z);

	loc = glGetUniformLocation(progID, lightDiffuse.c_str());
	if (loc >= 0)
		glUniform3f(loc, lightSource.diffuse.x, lightSource.diffuse.y, lightSource.diffuse.z);

	loc = glGetUniformLocation(progID, lightSpecular.c_str());
	if (loc >= 0)
		glUniform3f(loc, lightSource.specular.x, lightSource.specular.y, lightSource.specular.z);

	loc = glGetUniformLocation(progID, lightPosition.c_str());
	if (loc >= 0)
		glUniform3f(loc, lightSource.position.x, lightSource.position.y, lightSource.position.z);
}

void GLShader::SetMaterial(const Material& material) {
	GLint loc = glGetUniformLocation(progID, "material.ambient");
	if (loc >= 0)
		glUniform3f(loc, material.ambient.x, material.ambient.y, material.ambient.z);

	loc = glGetUniformLocation(progID, "material.diffuse");
	if (loc >= 0)
		glUniform3f(loc, material.diffuse.x, material.diffuse.y, material.diffuse.z);

	loc = glGetUniformLocation(progID, "material.specular");
	if (loc >= 0)
		glUniform3f(loc, material.specular.x, material.specular.y, material.specular.z);

	loc = glGetUniformLocation(progID, "material.shininess");
	if (loc >= 0)
		glUniform1f(loc, material.shininess);
}

void GLShader::ShowLighting(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bLightEnabled");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, bShow ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowMask(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowMask");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, bShow ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowWeight(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowWeight");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, bShow ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowSegments(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowSegments");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, bShow ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowTexture(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowTexture");
	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, bShow ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::BindTexture(const GLint& index, const GLuint& texture, const string& samplerName) {
	GLint texLoc = glGetUniformLocation(progID, samplerName.c_str());
	if (texLoc >= 0) {
		glUniform1i(texLoc, index);
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_2D, texture);
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
