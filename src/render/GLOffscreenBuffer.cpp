/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "GLOffscreenBuffer.h"

#include <texture2d.hpp>
#include <convert.hpp>
#include <save.hpp>
#include <make_texture.hpp>

#include <glm/gtx/gradient_paint.hpp>

GLOffScreenBuffer::GLOffScreenBuffer(GLSurface* gls, int width, int height, unsigned int count, const std::vector<GLuint>& texIds) {
	// for naming textures in CreateTextures
	static int globcount = 0;
	globcount++;
	GLOBCount = globcount;

	glsRef = gls;
	w = width;
	h = height;

	// current active buffer index starts at -1 to detect if it's been set yet or not
	current = -1;
	isBound = false;
	numBuffers = count;
	pmtex = new GLuint[count];
	pmfbo = new GLuint[count];

	// Create Texture destination
	createTextures();
	glGenFrameBuffers(count, pmfbo);

	// Create a renderbuffer for depth information, and attach it
	glGenRenderbuffers(1, &mrbo);
	glBindRenderbuffer(GL_RENDERBUFFER, mrbo);
	glRenderbufferstorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);

	for (int i = 0; i < count; i++) {
		if (texIds.size() > i && texIds[i] != 0) {
			pmtex[i] = texIds[i];
		}
		else {
			glBindTexture(GL_TEXTURE_2D, pmtex[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}

		// Create frame buffer for rendering destination, and attach texture to it
		glBindFramebuffer(GL_FRAMEBUFFER, pmfbo[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pmtex[i], 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mrbo);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

std::string GLOffScreenBuffer::texName(int index) {
	if (index == -1)
		index = current;

	std::string texname = "glob_" + std::to_string(GLOBCount) + "_" + std::to_string(index);
	return texname;
}

bool GLOffScreenBuffer::SetCurrentBuffer(int bufferIndex) {
	if (bufferIndex < 0 || bufferIndex >= numBuffers)
		return false;
	current = bufferIndex;
	return true;
}

bool GLOffScreenBuffer::NextBuffer(bool cycle) {
	if (current < -1)
		current = -1;		// setting to -1 because we want the increment to put it at 0.

	current++;

	// cycle buffers automatically.
	if (current >= numBuffers) {
		if (cycle) {
			current = 0;
		}
		else {
			current--;
			return false;
		}
	}

	return true;
}

void GLOffScreenBuffer::Start() {
	if (current < 0)
		NextBuffer();

	glBindFramebuffer(GL_FRAMEBUFFER, pmfbo[current]);
	isBound = true;
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, w, 0.0, h, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Retrieves the current texture ID.  Useful for externally selecting as a texture source before moving to another 
// buffer in a multi rendering chain.
GLuint GLOffScreenBuffer::GetTexID() {
	if (current > -1)
		return pmtex[current];

	return 0;
}

void GLOffScreenBuffer::SaveTexture(const std::string& filename) {
	if (!isBound)
		Start();		//not bound, bind the current framebuffer to read it's pixels.

	GLubyte*  pixels = new GLubyte[w * h * 4];
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	//gli::texture TexA = gli::make_texture2d(gli::FORMAT_RGB8_UNORM_PACK8, gli::extent2d(4), 2);

	//gli::texture2d TexB(gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16, gli::extent2d(32), 1);
	/* 
	gli::texture2d TexB(gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16, gli::texture2d::extent_type(256), 1);
	glm::u8vec4 * DstData = (glm::u8vec4*)TexB.data();
	
	gli::detail::dxt5_block{}

	for (int y = 0; y < TexB.extent().y; ++y)
		for (int x = 0; x < TexB.extent().x; ++x)
		{
			std::size_t Index = x + y * TexB.extent().x;
 			*(DstData + Index) = glm::u8vec4(255, 127, 0, 255);
		}

			float Value = glm::linearGradient(
				0.5f * glm::vec2(256),
				0.7f * glm::vec2(256),
				gli::vec2(x, y));

			std::size_t Index = x + y * TexB.extent().x;

			*(DstData + Index) = glm::u8vec3(glm::u8(glm::clamp(Value * 255.f, 0.f, 255.f)));
		}
	*/
	
	//char cleary[16];
	//memset(cleary, 255, 16);
	//cleary.push_back(gli::u8vec4(255, 127, 0, 255));
	//cleary.push_back(gli::u8vec4(255, 127, 0, 255));
	//cleary.push_back(gli::u8vec4(255, 127, 0, 255));
	//cleary.push_back(gli::u8vec4(255, 127, 0, 255));
	//TexB.clear(cleary);
	//TexB.clear(gli::u8vec4(255, 127, 0, 255));
		
	//gli::load((const char*)pixels, w*h * 4));
	//gli::texture2d outTex = gli::convert(TexB, gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16);

	//gli::save_dds(TexB, filename);

	SOIL_save_image(filename.c_str(), SOIL_SAVE_TYPE_DDS, w, h, 4, pixels);

	delete[] pixels;
}

void GLOffScreenBuffer::End() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	isBound = false;
}

GLOffScreenBuffer::~GLOffScreenBuffer() {
	if (isBound)
		End();

	deleteTextures();
	glDeleteRenderbuffers(1, &mrbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFrameBuffers(numBuffers, pmfbo);

	delete[] pmfbo;
	delete[] pmtex;
}
