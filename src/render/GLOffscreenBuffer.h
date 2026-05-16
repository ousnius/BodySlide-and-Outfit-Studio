/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "GLSurface.h"

#include <string>
#include <vector>


/*
Class for managing offscreen rendering buffers.
Allows more than one buffer to be created, and automatically cycled for a multipass rendering chain
Enables exporting buffers to disk files
*/
class GLOffScreenBuffer {
	GLSurface* glsRef = nullptr;
	GLuint* pmfbo = nullptr;
	GLuint* pmtex = nullptr;
	GLuint mrbo = 0;
	bool isBound = false;
	int current = -1;
	int numBuffers = 0;
	int w = 0, h = 0;
	int GLOBCount = 0;
	int msaaSamples = 0;

	// MSAA resources (only used when msaaSamples > 0)
	GLuint msaaFBO = 0;
	GLuint msaaColorRBO = 0;
	GLuint msaaDepthRBO = 0;

	void createTextures() {
		for (int i = 0; i < numBuffers; i++) {
			pmtex[i] = glsRef->GetResourceLoader()->GenerateTextureID(texName(i));
		}
	}

	// Delete textures calls resource loader to remove textures with the appropriate name, and is only
	// called during destruction.
	//  note, you can save textures by renaming them while GLOB exists.
	void deleteTextures() {
		for (int i = 0; i < numBuffers; i++) {
			glsRef->GetResourceLoader()->DeleteTexture(texName(i));
		}
	}

public:
	GLOffScreenBuffer(GLSurface* gls, int width, int height, int count = 1, const std::vector<GLuint>& texIds = std::vector<GLuint>(), int samples = 0);

	std::string texName(int index = -1);

	bool SetCurrentBuffer(int bufferIndex = 0);
	bool NextBuffer(bool cycle = true);
	void Start();

	// Retrieves the current texture ID.  Useful for externally selecting as a texture source before moving to another
	// buffer in a multi rendering chain.
	GLuint GetTexID();

	void SaveTexture(const std::string& filename);
	void Resolve();
	void End();

	~GLOffScreenBuffer();
};
