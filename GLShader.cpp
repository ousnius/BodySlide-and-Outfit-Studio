/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "GLShader.h"
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

void GLShader::generatePassThruVert() {
	string s = 
"void main(void)\
{\
    gl_FrontColor = gl_Color;\
    gl_TexCoord[0] = gl_MultiTexCoord0;\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
}";
	vertSrcLength = s.length();
	vertSrc = new char[vertSrcLength+1];
	_snprintf(vertSrc, vertSrcLength,"%s", s.c_str());
}

void GLShader::generateDefaultVert() {
	string s = 
"vec4 Ambient;\
vec4 Diffuse;\
vec4 Specular;\
\
\
void pointLight(in int i, in Vector3 normal, in Vector3 eye, in Vector3 ecPosition3)\
{\
   float nDotVP;       // normal . light direction\
   float nDotHV;       // normal . light half vector\
   float pf;           // power factor\
   float attenuation;  // computed attenuation factor\
   float d;            // distance from surface to light source\
   Vector3  VP;           // direction from surface to light position\
   Vector3  halfVector;   // direction of maximum highlights\
\
   // Compute vector from surface to light position\
   VP = Vector3 (gl_LightSource[i].position) - ecPosition3;\
\
   // Compute distance between surface and light position\
   d = length(VP);\
\
   // Normalize the vector from surface to light position\
   VP = normalize(VP);\
\
   // Compute attenuation\
   attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +\
       gl_LightSource[i].linearAttenuation * d +\
       gl_LightSource[i].quadraticAttenuation * d * d);\
\
   halfVector = normalize(VP + eye);\
\
   nDotVP = max(0.0, dot(normal, VP));\
   nDotHV = max(0.0, dot(normal, halfVector));\
\
   if (nDotVP == 0.0)\
   {\
       pf = 0.0;\
   }\
   else\
   {\
       pf = pow(nDotHV, gl_FrontMaterial.shininess);\
\
   }\
   Ambient  += gl_LightSource[i].ambient * attenuation;\
   Diffuse  += gl_LightSource[i].diffuse * nDotVP * attenuation;\
   Specular += gl_LightSource[i].specular * pf * attenuation;\
}\
\
void directionalLight(in int i, in Vector3 normal)\
{\
   float nDotVP;         // normal . light direction\
   float nDotHV;         // normal . light half vector\
   float pf;             // power factor\
\
   nDotVP = max(0.0, dot(normal, normalize(Vector3 (gl_LightSource[i].position))));\
   nDotHV = max(0.0, dot(normal, Vector3 (gl_LightSource[i].halfVector)));\
\
   if (nDotVP == 0.0)\
   {\
       pf = 0.0;\
   }\
   else\
   {\
       pf = pow(nDotHV, gl_FrontMaterial.shininess);\
\
   }\
   Ambient  += gl_LightSource[i].ambient;\
   Diffuse  += gl_LightSource[i].diffuse * nDotVP;\
   Specular += gl_LightSource[i].specular * pf;\
}\
\
Vector3 fnormal(void)\
{\
    //Compute the normal \
    Vector3 normal = gl_NormalMatrix * gl_Normal;\
    normal = normalize(normal);\
    return normal;\
}\
\
void ftexgen(in Vector3 normal, in vec4 ecPosition)\
{\
\
    gl_TexCoord[0] = gl_MultiTexCoord0;\
}\
\
void flight(in Vector3 normal, in vec4 ecPosition, float alphaFade)\
{\
    vec4 color;\
    Vector3 ecPosition3;\
    Vector3 eye;\
\
    ecPosition3 = (Vector3 (ecPosition)) / ecPosition.w;\
    eye = Vector3(0.0, 0.0, 1.0);\
\
    // Clear the light intensity accumulators\
    Ambient  = vec4(0.0);\
    Diffuse  = vec4(0.0);\
    Specular = vec4(0.0);\
\
    pointLight(0, normal, eye, ecPosition3);\
\
    pointLight(1, normal, eye, ecPosition3);\
\
    directionalLight(2, normal);\
\
    color = gl_FrontLightModelProduct.sceneColor +\
      Ambient  * gl_FrontMaterial.ambient +\
      Diffuse  * gl_FrontMaterial.diffuse;\
    gl_FrontSecondaryColor = Specular * gl_FrontMaterial.specular;\
    color = clamp(color, 0.0, 1.0);\
    gl_FrontColor = color;\
\
    gl_FrontColor.a *= alphaFade;\
}\
\
\
void main (void)\
{\
    Vector3  transformedNormal;\
    float alphaFade = 1.0;\
\
    // Eye-coordinate position of vertex, needed in various calculations\
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;\
\
    // Do fixed functionality vertex transform\
    gl_Position = ftransform();\
    transformedNormal = fnormal();\
  //  flight(transformedNormal, ecPosition, alphaFade);\
    ftexgen(transformedNormal, ecPosition);\
}";
	vertSrcLength = s.length();
	vertSrc = new char[vertSrcLength+1];
	_snprintf(vertSrc,vertSrcLength, "%s", s.c_str());
}

void GLShader::generatePassThruFrag() {
	string s = 
"void main(void)\
{\
gl_FragColor = gl_Color;\
}";
	fragSrcLength = s.length();
	fragSrc = new char[fragSrcLength+1];
	_snprintf(fragSrc, fragSrcLength, "%s", s.c_str());
}

void GLShader::generateDefaultFrag() {
	string s = 
"uniform sampler2D texUnit0;\
\
void main (void)\
{\
    vec4 color;\
    color = gl_Color;\
    color *= texture2D(texUnit0, gl_TexCoord[0].xy);\
    color += gl_SecondaryColor;\
	color = clamp(color, 0.0, 1.0);\
    gl_FragColor = color;\
}";
	fragSrcLength = s.length();
	fragSrc = new char[fragSrcLength+1];
	_snprintf(fragSrc, fragSrcLength, "%s", s.c_str());
}

GLShader::~GLShader() {
	if (fragSrc) {
		delete[] fragSrc;
		fragSrc = nullptr;
	}
	if (vertSrc) {
		delete[] vertSrc;
		vertSrc = nullptr;
	}
}

GLShader::GLShader() {
	nLights = 3;
	nTextures = 1;
	errorstate = -1;
	fragSrc = nullptr;
	vertSrc = nullptr;

	if (!initShaders())
		wxMessageBox(errorstring, _("Shader Error"));
	else
		if (!LoadShaders(GLSHADER_PASSTHROUGH, GLSHADER_PASSTHROUGH, false))
			wxMessageBox(errorstring, _("Shader Error"));
}

GLShader::GLShader(const char *vertexSource, const char *fragmentSource, bool build) {
	nLights = 3;
	nTextures = 1;
	errorstate = -1;
	fragSrc = nullptr;
	vertSrc = nullptr;

	if (!initShaders()) {
		wxString error = wxString::Format("%s (state %d)", errorstring, errorstate);
		wxLogError(error);
		wxMessageBox(error, _("OpenGL Error"), wxICON_ERROR);
	}
	else {
		if (!LoadShaders(vertexSource, fragmentSource, build)) {
			wxString error = wxString::Format("%s (state %d)", errorstring, errorstate);
			wxLogError(error);
			wxMessageBox(error, _("OpenGL Error"), wxICON_ERROR);
		}
	}
}

bool GLShader::initShaders() {
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

bool GLShader::loadShaderFile(const char* fname, char** buffer, int* outLength) {
	ifstream infile(fname, ios_base::in | ios_base::binary);
	(*outLength) = 0;
	if (!infile.good()) {
		return false;
	}

	infile.seekg(0, ios::end);
	long len = infile.tellg();
	infile.seekg(0, ios::beg);
	if (len == 0) {
		infile.close();
		return false;
	}

	(*buffer) = new char[len + 1];
	if ((*buffer) == 0) {
		infile.close();
		return false;
	}

	infile.read((*buffer), len);
	(*buffer)[len] = 0;
	(*outLength) = len;

	infile.close();
	return true;
}

bool GLShader::LoadShaders(const char *vertexSource, const char *fragmentSource, bool build) {
	bHasVertShader = true;
	bHasFragShader = true;

	if (fragSrc){
		delete[] fragSrc;
		fragSrc = nullptr;
	}
	if (vertSrc) {
		delete[] vertSrc;
		vertSrc = nullptr;
	}
	if (vertexSource == GLSHADER_PASSTHROUGH) {
		generatePassThruVert();
	}
	else if (vertexSource == GLSHADER_DEFAULT) {
		generateDefaultVert();
	}
	else if (vertexSource == GLSHADER_NONE) {
		bHasVertShader = false;
	}
	else {
		if (!loadShaderFile(vertexSource, &vertSrc, &vertSrcLength)) {
			bHasVertShader = false;
			bHasFragShader = false;
			errorstate = 2;
			errorstring = _("OpenGL: Failed to load vertex shader from file: ");
			errorstring += vertexSource;
			return false;
		}
	}

	if (fragmentSource == GLSHADER_PASSTHROUGH) {
		generatePassThruFrag();
	}
	else if (fragmentSource == GLSHADER_DEFAULT) {
		generateDefaultFrag();
	}
	else if (fragmentSource == GLSHADER_NONE) {
		bHasFragShader = false;
	}
	else {
		if (!loadShaderFile(fragmentSource, &fragSrc, &fragSrcLength)) {
			bHasFragShader = false;
			errorstate = 3;
			errorstring = _("OpenGL: Failed to load fragment shader from file: ");
			errorstring += fragmentSource;
			return false;
		}
	}

	if (build)
		return BuildShaders();

	return true;
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

bool GLShader::BuildShaders() {
	GLint compiled;
	GLint loglength;
	GLchar* logdata;
	if (bHasVertShader) {
		vertShadID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShadID, 1, (const GLchar**)&vertSrc, &vertSrcLength);
		glCompileShader(vertShadID);
		glGetShaderiv(vertShadID, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			glGetShaderiv(vertShadID, GL_INFO_LOG_LENGTH, &loglength);
			logdata = new char[loglength];
			glGetShaderInfoLog(vertShadID, loglength, nullptr, logdata);
			errorstate = 2;
			errorstring = _("OpenGL: Vertex shader compile failed: ");
			errorstring += logdata;
			delete[] logdata;
			return false;
		}
	}
	if (bHasFragShader) {
		fragShadID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShadID, 1, (const GLchar**)&fragSrc, &fragSrcLength);
		glCompileShader(fragShadID);
		glGetShaderiv(fragShadID, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			glGetShaderiv(fragShadID, GL_INFO_LOG_LENGTH, &loglength);
			logdata = new char[loglength];
			glGetShaderInfoLog(fragShadID, loglength, nullptr, logdata);
			errorstate = 3;
			errorstring = _("OpenGL: Fragment shader compile failed: ");
			errorstring += logdata;
			delete[] logdata;
			return false;
		}
	}

	progID = glCreateProgram();

	glAttachShader(progID, vertShadID);
	glAttachShader(progID, fragShadID);

	glLinkProgram(progID);
	glGetProgramiv(progID, GL_LINK_STATUS, &compiled);
	if (!compiled) {
		glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &loglength);
		logdata = new char[loglength];
		glGetProgramInfoLog(progID, loglength, nullptr, logdata);
		errorstate = 4;
		errorstring = _("OpenGL: Shader program link failed: ");
		errorstring += logdata;
		delete[] logdata;
		return false;
	}
	int nUniforms;
	glGetProgramiv(progID, GL_ACTIVE_UNIFORMS, &nUniforms);
	errorstate = 0;
	errorstring = _("OpenGL: Shader program ready.");

	return true;
}

void GLShader::Begin() {
	if (errorstate == 0) {
		glUseProgram(progID);
	}
}

void GLShader::End() {
	glUseProgram(0);
}
