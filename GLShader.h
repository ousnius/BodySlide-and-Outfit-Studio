/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "stdafx.h"
#include "Object3d.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <fstream>

#define GLSHADER_PASSTHROUGH "__PASSTHRU__"		// Specifies the basic "do nothing" shader.
#define GLSHADER_DEFAULT	 "__DEFAULT__"		// Specifies default shaders that emulate the OGL fixed pipeline.
#define GLSHADER_NONE        "__NONE__"			// Specifies no shader - fall back to OGL fixed pipeline.

class GLShader {
	static bool initComplete;
	char* vertSrc;			// Source text for vertex shader
	GLint vertSrcLength;
	char* fragSrc;			// Source text for fragment shader
	GLint fragSrcLength;

	bool bHasVertShader;	// Does the program have a vertex shader, or fall back to the default?
	bool bHasFragShader;	// Does the program have a fragment shader, or fall back to the default?

	// Compiled shader IDs after compile.
	GLuint vertShadID;
	GLuint fragShadID;

	// Linked Program ID after program creation.
	GLuint progID;

	GLuint maskAttribID;

	/* error state, set if compile/link fails.  check errorstring for compile log
		-1 = initial state -- not ready
		0 = no error, shader ready
		1 = shaders not supported,
		2 = vertex shader compile failed,
		3 = fragment shader compile failed,
		4 = program link failed.
		*/
	int errorstate;
	string errorstring;

	// Pass thru shader generators.
	void generatePassThruVert();
	void generatePassThruFrag();

	// Default shader generators.
	void generateDefaultVert();
	void generateDefaultFrag();

	bool initShaders();

	bool loadShaderFile(const char* filename, char** buffer, int* outLength);

public:
	// Arguments for use when generating default shaders.
	int nLights;		// default 3
	int nTextures;		// default 1

	// Default constructor generates passthrough shaders, but does not build them (run BuildShaders);
	GLShader();
	~GLShader();

	// Creates the shader object, and runs LoadShaders (and optionally, BuildShaders).
	GLShader(const char* vertexSource, const char* fragmentSource, bool build = true);

	// Attempts to load the specified source files (in text format).
	// If the passthrough or default shader type is specified for either, the appropriate shader is generated.
	// If GLSHADER_NONE is specified for either, then the fixed pipeline is used for it.
	// Set build to false to defer shader compile and link until a call to BuildShaders
	// (useful to allow time to modify generation parameters such as number of lights).
	bool LoadShaders(const char* vertexSource, const char* fragmentSource, bool build = true);

	// Compiles and links the loaded shaders, generating a program that can be activated.
	bool BuildShaders();

	// Enables lighting calculations in the vertex shader. This looks for the bLightEnabled uniform in the vertex
	// shader. If that value isn't present, nothing happens.
	void EnableVertexLighting(bool bEnable = true);

	void ShowMask(bool bShow = true);
	void ShowWeight(bool bShow = true);
	void ShowTexture(bool bShow = true);
	GLint GetMaskAttribute();
	GLint GetWeightAttribute();

	// Activates the stored program for subsequent GL rendering calls.
	// Compiles and links the program if it has not yet been done.
	void Begin();

	// Turns off the stored program.
	void End();
};

class GLMaterial {
public:
	vector<GLuint> texRef;
	GLShader* shader;

	GLMaterial() {
		texRef.resize(10, 0);
		shader = nullptr;
	}

	~GLMaterial() {
		glDeleteTextures(texRef.size(), texRef.data());

		if (shader)
			delete shader;
	}

	GLMaterial(GLuint texID, GLShader* inShader) : GLMaterial() {
		texRef[0] = texID;
		shader = inShader;
	}

	GLMaterial(GLuint texID, const char* vertShaderProg, const char*  fragShaderProg) : GLMaterial() {
		texRef[0] = texID;
		shader = new GLShader(vertShaderProg, fragShaderProg);
	}

	GLMaterial(GLuint texID[], int texCount, const char* vertShaderProg, const char* fragShaderProg) : GLMaterial() {
		texRef.resize(texCount, 0);
		for (int i = 0; i < texCount; i++)
			texRef[i] = texID[i];

		shader = new GLShader(vertShaderProg, fragShaderProg);
	}

	void ActivateTextures(Vector2* pTexCoord, GLfloat largestAF = 0) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texRef[0]);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2), pTexCoord);
		if (largestAF)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestAF);
	}

	void DeactivateTextures() {
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
};
