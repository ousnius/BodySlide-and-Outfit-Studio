/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "ResourceLoader.h"
#include "../render/GLShader.h"
#include "../utils/ConfigurationManager.h"

#include "FSEngine/FSManager.h"
#include "FSEngine/FSEngine.h"
#include "SOIL2/SOIL2.h"

#include <wx/filename.h>
#include <wx/log.h>

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

bool ResourceLoader::extChecked = false;

// File extension can be KTX or DDS
GLuint ResourceLoader::GLI_create_texture(gli::texture& texture) {
	if (!extGLISupported) {
		if (!extChecked) {
			wxLogWarning("OpenGL features required for GLI_create_texture to work aren't there!");
			extChecked = true;
		}
		return 0;
	}

	gli::gl glProfile(gli::gl::PROFILE_GL33);
	gli::gl::format const format = glProfile.translate(texture.format(), texture.swizzles());
	GLenum target = glProfile.translate(texture.target());

	GLuint textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(target, textureID);
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture.levels() - 1));
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, format.Swizzles[0]);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, format.Swizzles[1]);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, format.Swizzles[2]);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, format.Swizzles[3]);

	glm::tvec3<GLsizei> const textureExtent(texture.extent());
	GLsizei const faceTotal = static_cast<GLsizei>(texture.layers() * texture.faces());

	switch (texture.target()) {
	case gli::TARGET_1D:
		glTexStorage1D(
			target, static_cast<GLint>(texture.levels()), format.Internal, textureExtent.x);
		break;

	case gli::TARGET_1D_ARRAY:
	case gli::TARGET_2D:
	case gli::TARGET_CUBE:
		glTexStorage2D(
			target, static_cast<GLint>(texture.levels()), format.Internal,
			textureExtent.x, texture.target() == gli::TARGET_2D ? textureExtent.y : faceTotal);
		break;

	case gli::TARGET_2D_ARRAY:
	case gli::TARGET_3D:
	case gli::TARGET_CUBE_ARRAY:
		glTexStorage3D(
			target, static_cast<GLint>(texture.levels()), format.Internal,
			textureExtent.x, textureExtent.y,
			texture.target() == gli::TARGET_3D ? textureExtent.z : faceTotal);
		break;

	default:
		assert(0);
		break;
	}

	for (size_t layer = 0; layer < texture.layers(); ++layer) {
		for (size_t face = 0; face < texture.faces(); ++face) {
			for (size_t level = 0; level < texture.levels(); ++level) {
				GLsizei const layerGL = static_cast<GLsizei>(layer);
				glm::tvec3<GLsizei> textureExtent(texture.extent(level));
				target = gli::is_target_cube(texture.target())
					? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
					: target;

				switch (texture.target()) {
				case gli::TARGET_1D:
					if (gli::is_compressed(texture.format()))
						glCompressedTexSubImage1D(
						target, static_cast<GLint>(level), 0, textureExtent.x,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage1D(
						target, static_cast<GLint>(level), 0, textureExtent.x,
						format.External, format.Type,
						texture.data(layer, face, level));
					break;

				case gli::TARGET_1D_ARRAY:
				case gli::TARGET_2D:
				case gli::TARGET_CUBE:
					if (gli::is_compressed(texture.format()))
						glCompressedTexSubImage2D(
						target, static_cast<GLint>(level),
						0, 0,
						textureExtent.x,
						texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureExtent.y,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage2D(
						target, static_cast<GLint>(level),
						0, 0,
						textureExtent.x,
						texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureExtent.y,
						format.External, format.Type,
						texture.data(layer, face, level));
					break;

				case gli::TARGET_2D_ARRAY:
				case gli::TARGET_3D:
				case gli::TARGET_CUBE_ARRAY:
					if (gli::is_compressed(texture.format()))
						glCompressedTexSubImage3D(
						target, static_cast<GLint>(level),
						0, 0, 0,
						textureExtent.x, textureExtent.y,
						texture.target() == gli::TARGET_3D ? textureExtent.z : layerGL,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage3D(
						target, static_cast<GLint>(level),
						0, 0, 0,
						textureExtent.x, textureExtent.y,
						texture.target() == gli::TARGET_3D ? textureExtent.z : layerGL,
						format.External, format.Type,
						texture.data(layer, face, level));
					break;

				default:
					assert(0);
					break;
				}
			}
		}
	}

	return textureID;
}

GLuint ResourceLoader::GLI_load_texture(const string& fileName) {
	gli::texture texture = gli::load(fileName);
	if (texture.empty())
		return 0;

	return GLI_create_texture(texture);
}

GLuint ResourceLoader::GLI_load_texture_from_memory(const char* buffer, size_t size) {
	gli::texture texture = gli::load(buffer, size);
	if (texture.empty())
		return 0;

	return GLI_create_texture(texture);
}

GLMaterial* ResourceLoader::AddMaterial(const vector<string>& textureFiles, const string& vShaderFile, const string& fShaderFile) {
	MaterialKey key(textureFiles, vShaderFile, fShaderFile);
	auto it = materials.find(key);
	if (it != materials.end())
		return it->second.get();

	vector<GLuint> texRefs(textureFiles.size(), 0);
	for (int i = 0; i < textureFiles.size(); i++) {
		if (textureFiles[i].empty())
			continue;

		GLuint textureID = 0;
		wxFileName fileName(textureFiles[i]);
		wxString fileExt = fileName.GetExt().Lower();
		string fileExtStr = string(fileExt.c_str());

		// Cubemap
		if (!textureID && i == 4)
			textureID = SOIL_load_OGL_single_cubemap(textureFiles[i].c_str(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS);

		// Normal texture (GLI)
		if (!textureID && fileExtStr == "dds" || fileExtStr == "ktx")
			textureID = GLI_load_texture(textureFiles[i]);

		// Normal texture and image (SOIL fallback)
		if (!textureID)
			textureID = SOIL_load_OGL_texture(textureFiles[i].c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);

		if (!textureID && Config.MatchValue("BSATextureScan", "true")) {
			if (Config["GameDataPath"].empty()) {
				wxLogWarning("Texture file '%s' not found.", textureFiles[i]);
				continue;
			}

			wxMemoryBuffer data;
			wxString texFile = textureFiles[i];
			texFile.Replace(Config["GameDataPath"], "");
			texFile.Replace("\\", "/");
			for (FSArchiveFile *archive : FSManager::archiveList()) {
				if (archive) {
					if (archive->hasFile(texFile.ToStdString())) {
						wxMemoryBuffer outData;
						archive->fileContents(texFile.ToStdString(), outData);

						if (!outData.IsEmpty()) {
							data = outData;
							break;
						}
					}
				}
			}

			if (!data.IsEmpty()) {
				byte* texBuffer = static_cast<byte*>(data.GetData());

				// Cubemap
				if (!textureID && i == 4)
					textureID = SOIL_load_OGL_single_cubemap_from_memory(texBuffer, data.GetBufSize(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS);

				// Normal texture (GLI)
				if (!textureID && fileExtStr == "dds" || fileExtStr == "ktx")
					textureID = GLI_load_texture_from_memory((char*)texBuffer, data.GetBufSize());

				// Normal texture and image (SOIL fallback)
				if (!textureID)
					textureID = SOIL_load_OGL_texture_from_memory(texBuffer, data.GetBufSize(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
			}
			else {
				wxLogWarning("Texture file '%s' not found.", textureFiles[i]);
				continue;
			}
		}
		else if (!textureID) {
			wxLogWarning("Texture file '%s' not found.", textureFiles[i]);
			continue;
		}

		texRefs[i] = textureID;
	}

	// No diffuse found
	if (texRefs.empty() || texRefs[0] == 0) {
		// Set default material key
		vector<string> defaultTex = { "res\\images\\NoImg.png" };
		key = MaterialKey(defaultTex, vShaderFile, fShaderFile);

		// Return existing no image material
		auto it = materials.find(key);
		if (it != materials.end())
			return it->second.get();

		// Reset texture indices
		texRefs.resize(1, 0);

		// Load default image
		texRefs[0] = SOIL_load_OGL_texture(defaultTex[0].c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(texRefs.data(), texRefs.size(), vShaderFile, fShaderFile));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	hash<string> strHash;
	size_t resHash = strHash(get<1>(key)) ^ strHash(get<2>(key));

	for (int i = 0; i < get<0>(key).size(); i++)
		resHash ^= strHash(get<0>(key)[i]);

	return resHash;
}
