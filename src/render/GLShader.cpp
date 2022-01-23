/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GLShader.h"
#include "../utils/PlatformUtil.h"
#include "Object3d.hpp"

#include <fstream>
#include <sstream>

using namespace nifly;

GLShader::GLShader(const std::string& vertexSource, const std::string& fragmentSource)
	: GLShader() {
	if (CheckExtensions() && LoadShaders(vertexSource, fragmentSource)) {
		ShowLighting();
		ShowTexture();
		ShowMask();
		ShowWeight(false);
		ShowVertexColors(false);
		ShowVertexAlpha(false);

		SetColor(Vector3(1.0f, 1.0f, 1.0f));
		SetModelSpace(false);
		SetEmissive(false);
		SetWireframeEnabled(false);
		SetPointsEnabled(false);
		SetLightingEnabled(true);
	}
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

bool GLShader::LoadShaderFile(const std::string& fileName, std::string& text) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios_base::in | std::ios_base::binary);
	if (!file)
		return false;

	std::stringstream buffer;
	buffer << file.rdbuf();

	text = buffer.str();

	if (text.empty())
		return false;

	return true;
}

bool GLShader::LoadShaders(const std::string& vertexSource, const std::string& fragmentSource) {
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

void GLShader::SetSubColor(const Vector3& color) {
	GLint loc = glGetUniformLocation(progID, "subColor");
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

void GLShader::SetMatrixModelView(const glm::mat4x4& matView, const glm::mat4x4& matModel) {
	GLint loc = glGetUniformLocation(progID, "matView");
	if (loc >= 0)
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&matView);

	loc = glGetUniformLocation(progID, "matModel");
	if (loc >= 0)
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&matModel);

	loc = glGetUniformLocation(progID, "matModelView");
	if (loc >= 0) {
		glm::mat4x4 matModelView = matView * matModel;
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&matModelView);
		loc = glGetUniformLocation(progID, "matModelViewInverse");
		if (loc >= 0) {
			glm::mat4x4 matModelViewInverse = glm::inverse(matModelView);
			glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&matModelViewInverse);
		}
		loc = glGetUniformLocation(progID, "mv_normalMatrix");
		if (loc >= 0) {
			glm::mat3x3 mv_normalMatrix = glm::transpose(glm::inverse(glm::mat3(matModelView)));
			glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat*)&mv_normalMatrix);
		}
	}
}

void GLShader::SetAlphaProperties(const uint16_t flags, const float threshold, const float value) {
	static const GLenum blendMap[16] = {GL_ONE,
										GL_ZERO,
										GL_SRC_COLOR,
										GL_ONE_MINUS_SRC_COLOR,
										GL_DST_COLOR,
										GL_ONE_MINUS_DST_COLOR,
										GL_SRC_ALPHA,
										GL_ONE_MINUS_SRC_ALPHA,
										GL_DST_ALPHA,
										GL_ONE_MINUS_DST_ALPHA,
										GL_SRC_ALPHA_SATURATE,
										GL_ONE,
										GL_ONE,
										GL_ONE,
										GL_ONE,
										GL_ONE};

	//static const GLenum testMap[8] = {
	//	GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_NEVER
	//};

	bool alphaBlend = flags & 1;
	bool alphaTest = (flags & (1 << 9)) != 0;
	GLenum alphaSrc = blendMap[(flags >> 1) & 0x0F];
	GLenum alphaDst = blendMap[(flags >> 5) & 0x0F];

	//Always GL_GREATER right now
	//GLenum alphaFunc = testMap[(flags >> 10) & 0x7];

	glDisable(GL_BLEND);

	if (alphaBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(alphaSrc, alphaDst);
	}

	if (!alphaBlend && value < 1.0f) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

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

void GLShader::SetFrontalLight(const DirectionalLight& light) {
	GLint loc = glGetUniformLocation(progID, "frontal.diffuse");
	if (loc >= 0)
		glUniform3f(loc, light.diffuse.x, light.diffuse.y, light.diffuse.z);

	loc = glGetUniformLocation(progID, "frontal.direction");
	if (loc >= 0)
		glUniform3f(loc, 0.0f, 0.0f, 1.0f);
}

void GLShader::SetDirectionalLight(const DirectionalLight& light, const int index) {
	GLint locDiffuse = -1;
	GLint locDirection = -1;

	if (index == 0) {
		locDiffuse = glGetUniformLocation(progID, "directional0.diffuse");
		locDirection = glGetUniformLocation(progID, "directional0.direction");
	}
	else if (index == 1) {
		locDiffuse = glGetUniformLocation(progID, "directional1.diffuse");
		locDirection = glGetUniformLocation(progID, "directional1.direction");
	}
	else if (index == 2) {
		locDiffuse = glGetUniformLocation(progID, "directional2.diffuse");
		locDirection = glGetUniformLocation(progID, "directional2.direction");
	}

	if (locDiffuse >= 0)
		glUniform3f(locDiffuse, light.diffuse.x, light.diffuse.y, light.diffuse.z);

	if (locDirection >= 0)
		glUniform3f(locDirection, light.direction.x, light.direction.y, light.direction.z);
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

	loc = glGetUniformLocation(progID, "prop.backlightPower");
	if (loc >= 0)
		glUniform1f(loc, prop.backlightPower);

	loc = glGetUniformLocation(progID, "prop.rimlightPower");
	if (loc >= 0)
		glUniform1f(loc, prop.rimlightPower);

	loc = glGetUniformLocation(progID, "prop.softlighting");
	if (loc >= 0)
		glUniform1f(loc, prop.softlighting);

	loc = glGetUniformLocation(progID, "prop.subsurfaceRolloff");
	if (loc >= 0)
		glUniform1f(loc, prop.subsurfaceRolloff);

	loc = glGetUniformLocation(progID, "prop.fresnelPower");
	if (loc >= 0)
		glUniform1f(loc, prop.fresnelPower);

	loc = glGetUniformLocation(progID, "prop.paletteScale");
	if (loc >= 0)
		glUniform1f(loc, prop.paletteScale);
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

void GLShader::ShowVertexColors(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowVertexColor");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1i(loc, bShow ? GL_TRUE : GL_FALSE);
		glUseProgram(0);
	}
}

void GLShader::ShowVertexAlpha(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowVertexAlpha");

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

void GLShader::SetAlphaMaskEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bAlphaMask");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetGreyscaleColorEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bGreyscaleColor");
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

void GLShader::SetRimlightEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bRimlight");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetSoftlightEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bSoftlight");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::SetGlowmapEnabled(const bool enable) {
	GLint loc = glGetUniformLocation(progID, "bGlowmap");
	if (loc >= 0)
		glUniform1i(loc, enable ? GL_TRUE : GL_FALSE);
}

void GLShader::BindTexture(const GLint& index, const GLuint& texture, const char* samplerName) {
	GLint texLoc = glGetUniformLocation(progID, samplerName);
	if (texLoc >= 0) {
		glUniform1i(texLoc, index);
		glActiveTexture(GL_TEXTURE0 + index);

		glBindTexture(GL_TEXTURE_2D, texture);
	}
}

void GLShader::BindCubemap(const GLint& index, const GLuint& texture, const char* samplerName) {
	GLint texLoc = glGetUniformLocation(progID, samplerName);
	if (texLoc >= 0) {
		glUniform1i(texLoc, index);
		glActiveTexture(GL_TEXTURE0 + index);

		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	}
}

bool GLShader::GetError(std::string* errorStr) {
	if (!errorState)
		return false;

	if (errorStr)
		(*errorStr) = errorString;

	return true;
}

bool GLShader::BuildShaders() {
	GLint compiled;
	GLint loglength;

	const GLchar* src = vertSrc.c_str();
	vertShadID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShadID, 1, &src, nullptr);

	glCompileShader(vertShadID);
	glGetShaderiv(vertShadID, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glGetShaderiv(vertShadID, GL_INFO_LOG_LENGTH, &loglength);
		GLchar* logdata = new GLchar[loglength];
		glGetShaderInfoLog(vertShadID, loglength, nullptr, logdata);
		errorString = std::string("OpenGL: Vertex shader compile failed: ") + logdata;
		delete[] logdata;

		errorState = 2;
		return false;
	}

	src = fragSrc.c_str();
	fragShadID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShadID, 1, &src, nullptr);

	glCompileShader(fragShadID);
	glGetShaderiv(fragShadID, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glGetShaderiv(fragShadID, GL_INFO_LOG_LENGTH, &loglength);
		GLchar* logdata = new GLchar[loglength];
		glGetShaderInfoLog(fragShadID, loglength, nullptr, logdata);
		errorString = std::string("OpenGL: Fragment shader compile failed: ") + logdata;
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
		errorString = std::string("OpenGL: Shader program link failed: ") + logdata;
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
