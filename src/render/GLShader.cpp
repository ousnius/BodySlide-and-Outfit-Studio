/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "GLShader.h"
#include <wx/msgdlg.h>
#include <fstream>
#include <sstream>

#pragma warning (disable : 4130)

bool GLShader::initComplete = false;

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;

PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;

PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;


GLShader::GLShader() {
	errorstate = -1;
}

GLShader::GLShader(const string& vertexSource, const string& fragmentSource) : GLShader() {
	if (!InitShaders()) {
		wxString error = wxString::Format("%s (state %d)", errorstring, errorstate);
		wxLogError(error);
		wxMessageBox(error, _("OpenGL Error"), wxICON_ERROR);
	}
	else {
		if (!LoadShaders(vertexSource, fragmentSource)) {
			wxString error = wxString::Format("%s (state %d)", errorstring, errorstate);
			wxLogError(error);
			wxMessageBox(error, _("OpenGL Error"), wxICON_ERROR);
		}
	}
}

GLShader::~GLShader() {
}

bool GLShader::InitShaders() {
	if (!initComplete) {
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");

		if (!glCreateShader || !glShaderSource || !glCompileShader) {
			errorstate = 1;
			errorstring = _("OpenGL: One or more shader functions are not supported.");
			return false;
		}

		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");

		if (!glCreateProgram || !glAttachShader || !glLinkProgram || !glUseProgram) {
			errorstate = 1;
			errorstring = _("OpenGL: One or more program functions are not supported.");
			return false;
		}

		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");

		glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");

		initComplete = true;
	}
	return true;
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
		errorstate = 2;
		errorstring = _("OpenGL: Failed to load vertex shader from file: ") + vertexSource;
		return false;
	}

	if (!LoadShaderFile(fragmentSource, fragSrc)) {
		errorstate = 3;
		errorstring = _("OpenGL: Failed to load fragment shader from file: ") + fragmentSource;
		return false;
	}

	return BuildShaders();
}

// Enables lighting calculations in the vertex shader.  This looks for the bLightEnabled uniform in the vertex shader.
// If that value isn't present, nothing happens.
void GLShader::EnableVertexLighting(bool bEnable) {
	GLint loc = glGetUniformLocation(progID, "bLightEnabled");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, (bEnable) ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowMask(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowMask");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, (bShow) ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowWeight(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowWeight");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, (bShow) ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void GLShader::ShowSegments(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowSegments");

	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, (bShow) ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

void  GLShader::ShowTexture(bool bShow) {
	GLint loc = glGetUniformLocation(progID, "bShowTexture");
	if (loc >= 0) {
		glUseProgram(progID);
		glUniform1f(loc, (bShow) ? 1.0f : 0.0f);
		glUseProgram(0);
	}
}

GLint GLShader::GetMaskAttribute() {
	return glGetAttribLocation(progID, "maskValue");
}

GLint GLShader::GetWeightAttribute() {
	return glGetAttribLocation(progID, "weightValue");
}

GLint GLShader::GetSegmentAttribute() {
	return glGetAttribLocation(progID, "segmentValue");
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
		errorstring = _("OpenGL: Vertex shader compile failed: ") + logdata;
		delete[] logdata;

		errorstate = 2;
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
		errorstring = _("OpenGL: Fragment shader compile failed: ") + logdata;
		delete[] logdata;

		errorstate = 3;
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
		errorstring = _("OpenGL: Shader program link failed: ") + logdata;
		delete[] logdata;

		errorstate = 4;
		return false;
	}

	errorstring = _("OpenGL: Shader program ready.");
	errorstate = 0;
	return true;
}

int GLShader::Begin() {
	if (errorstate == 0)
		glUseProgram(progID);

	return !errorstate;
}

void GLShader::End() {
	glUseProgram(0);
}
