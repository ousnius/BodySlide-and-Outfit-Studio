/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "../render/GLMaterial.h"
#include "../utils/ConfigurationManager.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <wx/filename.h>
#include <wx/log.h>

#include <cmath>

extern ConfigurationManager Config;

ResourceLoader::ResourceLoader() {}

ResourceLoader::~ResourceLoader() {
	Cleanup();
}

bool ResourceLoader::extChecked = false;

GLuint ResourceLoader::LoadTexture(const std::string& inFileName, bool isCubeMap, bool reloadTextures) {
	auto ti = textures.find(inFileName);
	if (!reloadTextures) {
		// Return existing texture index
		if (ti != textures.end())
			return ti->second;
	}

	wxFileName fileName(inFileName);
	wxString fileExt = fileName.GetExt().Lower();
	std::string fileExtStr = std::string(fileExt.c_str());

	GLuint textureID = 0;

	// Get existing index to overwrite texture data for, otherwise generate new index later
	if (reloadTextures && ti != textures.end())
		textureID = ti->second;

	// All textures (GLI)
	if (fileExtStr == "dds" || fileExtStr == "ktx")
		textureID = GLI_load_texture(inFileName, textureID);

	// Cubemap fallback (SOIL)
	if (!textureID && isCubeMap)
		textureID = SOIL_load_OGL_single_cubemap(inFileName.c_str(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO, textureID, SOIL_FLAG_GL_MIPMAPS);

	// Texture and image fallback (SOIL)
	if (!textureID)
		textureID = SOIL_load_OGL_texture(inFileName.c_str(), SOIL_LOAD_AUTO, textureID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);

	if (!textureID && Config.MatchValue("BSATextureScan", "true")) {
		if (Config["GameDataPath"].empty()) {
			wxLogWarning("Texture file '%s' not found.", inFileName);
			return 0;
		}

		wxMemoryBuffer data;
		wxString texFile = inFileName;
		texFile.Replace(wxString(Config["GameDataPath"]), "");
		texFile.Replace("\\", "/");
		for (FSArchiveFile* archive : FSManager::archiveList()) {
			if (archive) {
				if (archive->hasFile(texFile.ToStdString())) {
					wxMemoryBuffer outData;
					archive->fileContents(texFile.ToStdString(), outData);

					if (!outData.IsEmpty()) {
						data = std::move(outData);
						break;
					}
				}
			}
		}

		if (!data.IsEmpty()) {
			uint8_t* texBuffer = static_cast<uint8_t*>(data.GetData());

			// All textures (GLI)
			if (fileExtStr == "dds" || fileExtStr == "ktx")
				textureID = GLI_load_texture_from_memory((char*)texBuffer, data.GetDataLen(), textureID);

			// Cubemap fallback (SOIL)
			if (!textureID && isCubeMap)
				textureID = SOIL_load_OGL_single_cubemap_from_memory(texBuffer, data.GetDataLen(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO, textureID, SOIL_FLAG_GL_MIPMAPS);

			// Texture and image fallback (SOIL)
			if (!textureID)
				textureID = SOIL_load_OGL_texture_from_memory(texBuffer,
															  data.GetDataLen(),
															  SOIL_LOAD_AUTO,
															  textureID,
															  SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
		}
		else {
			wxLogWarning("Texture file '%s' not found.", inFileName);
			return 0;
		}
	}
	else if (!textureID) {
		wxLogWarning("Texture file '%s' not found.", inFileName);
		return 0;
	}

	textures[inFileName] = textureID;

	return textureID;
}

GLuint ResourceLoader::GenerateTextureID(const std::string& texName) {
	DeleteTexture(texName);

	GLuint textureID;
	glGenTextures(1, &textureID);
	textures[texName] = textureID;

	return textureID;
}

GLuint ResourceLoader::GetTexID(const std::string& texName) {
	auto ti = textures.find(texName);
	if (ti != textures.end())
		return ti->second;

	return 0;
}

void ResourceLoader::DeleteTexture(const std::string& texName) {
	auto ti = textures.find(texName);
	if (ti != textures.end()) {
		cacheTime++;
		glDeleteTextures(1, &ti->second);
		textures.erase(ti);
	}
}

bool ResourceLoader::RenameTexture(const std::string& texNameSrc, const std::string& texNameDest, bool overwrite) {
	std::string src = texNameSrc;
	std::string dst = texNameDest;

	auto tid = textures.find(dst);
	if (tid != textures.end()) {
		if (!overwrite)
			return false;

		DeleteTexture(dst);
	}

	auto ti = textures.find(src);
	if (ti != textures.end()) {
		// If a texture is replaced, cacheTime increment by 2 in this function (DeleteTexture also increments it)
		cacheTime++;
		textures[dst] = ti->second;
		textures.erase(ti);
	}
	return true;
}

// File extension can be KTX or DDS
GLuint ResourceLoader::GLI_create_texture(gli::texture& texture, GLuint textureID) {
	if (!extGLISupported) {
		if (!extChecked) {
			wxLogWarning("OpenGL features required for GLI_create_texture to work aren't there!");
			extChecked = true;
		}
		return textureID;
	}

	gli::gl glProfile(gli::gl::PROFILE_GL33);
	gli::gl::format const format = glProfile.translate(texture.format(), texture.swizzles());
	GLenum target = glProfile.translate(texture.target());

	if (textureID == 0)
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
		case gli::TARGET_1D: glTexStorage1D(target, static_cast<GLint>(texture.levels()), format.Internal, textureExtent.x); break;

		case gli::TARGET_1D_ARRAY:
		case gli::TARGET_2D:
		case gli::TARGET_CUBE:
			glTexStorage2D(target, static_cast<GLint>(texture.levels()), format.Internal, textureExtent.x, texture.target() != gli::TARGET_1D_ARRAY ? textureExtent.y : faceTotal);
			break;

		case gli::TARGET_2D_ARRAY:
		case gli::TARGET_3D:
		case gli::TARGET_CUBE_ARRAY:
			glTexStorage3D(target,
						   static_cast<GLint>(texture.levels()),
						   format.Internal,
						   textureExtent.x,
						   textureExtent.y,
						   texture.target() == gli::TARGET_3D ? textureExtent.z : faceTotal);
			break;

		default: assert(0); break;
	}

	for (size_t layer = 0; layer < texture.layers(); ++layer) {
		for (size_t face = 0; face < texture.faces(); ++face) {
			for (size_t level = 0; level < texture.levels(); ++level) {
				GLsizei const layerGL = static_cast<GLsizei>(layer);
				glm::tvec3<GLsizei> textureLevelExtent(texture.extent(level));
				target = gli::is_target_cube(texture.target()) ? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face) : target;

				switch (texture.target()) {
					case gli::TARGET_1D:
						if (gli::is_compressed(texture.format()))
							glCompressedTexSubImage1D(target,
													  static_cast<GLint>(level),
													  0,
													  textureLevelExtent.x,
													  format.Internal,
													  static_cast<GLsizei>(texture.size(level)),
													  texture.data(layer, face, level));
						else
							glTexSubImage1D(target, static_cast<GLint>(level), 0, textureLevelExtent.x, format.External, format.Type, texture.data(layer, face, level));
						break;

					case gli::TARGET_1D_ARRAY:
					case gli::TARGET_2D:
					case gli::TARGET_CUBE:
						if (gli::is_compressed(texture.format()))
							glCompressedTexSubImage2D(target,
													  static_cast<GLint>(level),
													  0,
													  0,
													  textureLevelExtent.x,
													  texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureLevelExtent.y,
													  format.Internal,
													  static_cast<GLsizei>(texture.size(level)),
													  texture.data(layer, face, level));
						else
							glTexSubImage2D(target,
											static_cast<GLint>(level),
											0,
											0,
											textureLevelExtent.x,
											texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureLevelExtent.y,
											format.External,
											format.Type,
											texture.data(layer, face, level));
						break;

					case gli::TARGET_2D_ARRAY:
					case gli::TARGET_3D:
					case gli::TARGET_CUBE_ARRAY:
						if (gli::is_compressed(texture.format()))
							glCompressedTexSubImage3D(target,
													  static_cast<GLint>(level),
													  0,
													  0,
													  0,
													  textureLevelExtent.x,
													  textureLevelExtent.y,
													  texture.target() == gli::TARGET_3D ? textureLevelExtent.z : layerGL,
													  format.Internal,
													  static_cast<GLsizei>(texture.size(level)),
													  texture.data(layer, face, level));
						else
							glTexSubImage3D(target,
											static_cast<GLint>(level),
											0,
											0,
											0,
											textureLevelExtent.x,
											textureLevelExtent.y,
											texture.target() == gli::TARGET_3D ? textureLevelExtent.z : layerGL,
											format.External,
											format.Type,
											texture.data(layer, face, level));
						break;

					default: assert(0); break;
				}
			}
		}
	}

	return textureID;
}

GLuint ResourceLoader::GLI_load_texture(const std::string& fileName, GLuint textureID) {
	gli::texture texture = gli::load(fileName);
	if (texture.empty())
		return textureID;

	return GLI_create_texture(texture, textureID);
}

GLuint ResourceLoader::GLI_load_texture_from_memory(const char* buffer, size_t size, GLuint textureID) {
	gli::texture texture = gli::load(buffer, size);
	if (texture.empty())
		return textureID;

	return GLI_create_texture(texture, textureID);
}

int ResourceLoader::GetBoundMaxMipLevel(GLenum levelTarget) {
	int maxLevel = 0;

	// 32 is past the largest texture any GL implementation allows, so this terminates either way.
	for (GLint level = 1; level < 32; level++) {
		GLint levelWidth = 0;
		glGetTexLevelParameteriv(levelTarget, level, GL_TEXTURE_WIDTH, &levelWidth);
		if (levelWidth <= 0)
			break;

		maxLevel = level;
	}

	return maxLevel;
}

bool ResourceLoader::ClassifyComplexMaterial(GLuint textureID) const {
	glBindTexture(GL_TEXTURE_2D, textureID);

	const GLint level = GetBoundMaxMipLevel(GL_TEXTURE_2D);

	GLint width = 0;
	GLint height = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &height);
	if (width <= 0 || height <= 0)
		return false;

	// Asking for RGBA8 makes the driver decompress whatever block format the file was in.
	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);

	GLint packAlignment = 4;
	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);

	double sumR = 0.0, sumG = 0.0, sumB = 0.0;
	const size_t texelCount = static_cast<size_t>(width) * height;
	for (size_t i = 0; i < texelCount; i++) {
		sumR += pixels[i * 4 + 0];
		sumG += pixels[i * 4 + 1];
		sumB += pixels[i * 4 + 2];
	}

	const float avgR = static_cast<float>(sumR / texelCount / 255.0);
	const float avgG = static_cast<float>(sumG / texelCount / 255.0);
	const float avgB = static_cast<float>(sumB / texelCount / 255.0);

	// Values at or below 4/255 count as black; the channels can't hold anything smaller reliably
	// once the texture has been block compressed.
	const float threshold = 4.0f / 255.0f;

	// A vanilla environment mask is greyscale, so its green channel is nothing but the mask again.
	// Reading that as a glossiness map would change how every env mapped shape has always looked.
	const bool greyscale = std::fabs(avgR - avgG) < threshold && std::fabs(avgR - avgB) < threshold && std::fabs(avgG - avgB) < threshold;

	return !greyscale && avgG > threshold;
}

void ResourceLoader::ClassifyCubemap(const std::string& texName, GLuint textureID) {
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	GLint width = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &width);
	cubemapSizes[texName] = width;

	if (width != 1)
		return;

	// The texel of a 1x1 cube map isn't a reflection, it's an sRGB F0 reflectance the dynamic cube
	// map replacing it should be tinted with. Asking for RGBA8 makes the driver decompress whatever
	// block format the file was in.
	uint8_t texel[4] = {0, 0, 0, 0};

	GLint packAlignment = 4;
	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel);
	glPixelStorei(GL_PACK_ALIGNMENT, packAlignment);

	// The same threshold the Complex Material classification uses for black. A black cube map is the
	// plain "give me a dynamic one" marker rather than a color, and stands for full reflectance.
	const uint8_t threshold = 4;
	if (texel[0] <= threshold && texel[1] <= threshold && texel[2] <= threshold)
		return;

	auto srgbToLinear = [](const uint8_t value) {
		const float v = value / 255.0f;
		return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
	};

	cubemapF0Colors[texName] = nifly::Vector3(srgbToLinear(texel[0]), srgbToLinear(texel[1]), srgbToLinear(texel[2]));
	wxLogMessage("Texture file '%s' is a 1x1 cube map with an F0 of %.3f, %.3f, %.3f.",
				 texName,
				 cubemapF0Colors[texName].x,
				 cubemapF0Colors[texName].y,
				 cubemapF0Colors[texName].z);
}

bool ResourceLoader::IsComplexMaterialTexture(const std::string& texName) const {
	auto it = complexMaterialTextures.find(texName);
	if (it != complexMaterialTextures.end())
		return it->second;

	return false;
}

int ResourceLoader::GetTextureMaxMipLevel(const std::string& texName) const {
	auto it = textureMaxMipLevels.find(texName);
	if (it != textureMaxMipLevels.end())
		return it->second;

	return 0;
}

int ResourceLoader::GetCubemapSize(const std::string& texName) const {
	auto it = cubemapSizes.find(texName);
	if (it != cubemapSizes.end())
		return it->second;

	return 0;
}

nifly::Vector3 ResourceLoader::GetCubemapF0Color(const std::string& texName) const {
	auto it = cubemapF0Colors.find(texName);
	if (it != cubemapF0Colors.end())
		return it->second;

	return nifly::Vector3(1.0f, 1.0f, 1.0f);
}

GLMaterial* ResourceLoader::AddMaterial(
	const std::vector<std::string>& textureFiles, const std::string& vShaderFile, const std::string& fShaderFile, const bool reloadTextures, const bool useDefaultTexture) {
	auto texFiles = textureFiles;

	MaterialKey key(texFiles, vShaderFile, fShaderFile, useDefaultTexture);
	if (!reloadTextures) {
		auto it = materials.find(key);
		if (it != materials.end())
			return it->second.get();
	}

	std::vector<GLuint> texRefs(texFiles.size(), 0);
	for (size_t i = 0; i < texFiles.size(); i++) {
		if (texFiles[i].empty())
			continue;

		bool isCubeMap = (i == 4);
		GLuint textureID = LoadTexture(texFiles[i], isCubeMap, reloadTextures);
		if (!textureID)
			continue;

		texRefs[i] = textureID;

		// Slot 4 is the environment cube map, whose mip chain is how far a Complex Material
		// reflection can be blurred, and whose size says whether it is a real reflection or the 1x1
		// placeholder that asks for a dynamic one.
		if (isCubeMap && (reloadTextures || textureMaxMipLevels.find(texFiles[i]) == textureMaxMipLevels.end())) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			textureMaxMipLevels[texFiles[i]] = GetBoundMaxMipLevel(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
			ClassifyCubemap(texFiles[i], textureID);
		}

		// Slot 5 is the environment mask, which is also where a Complex Material texture lives.
		if (i == 5 && (reloadTextures || complexMaterialTextures.find(texFiles[i]) == complexMaterialTextures.end())) {
			const bool isComplexMaterial = ClassifyComplexMaterial(textureID);
			complexMaterialTextures[texFiles[i]] = isComplexMaterial;

			if (isComplexMaterial)
				wxLogMessage("Texture file '%s' was detected as a Complex Material.", texFiles[i]);
		}
	}

	// No diffuse found
	if (texRefs.empty())
		texRefs.resize(1, 0);

	// Shapes without a shader have no textures to begin with and are left untextured instead of getting the placeholder.
	if (useDefaultTexture && texRefs[0] == 0) {
		// Load default image
		std::string defaultTex = Config["AppDir"] + "/res/images/NoImg.png";

		texRefs[0] = LoadTexture(defaultTex, false);

		if (!texFiles.empty())
			texFiles[0] = defaultTex;
		else
			texFiles.resize(1, defaultTex);
	}

	auto& entry = materials[key];
	if (!entry || !reloadTextures)
		entry.reset(new GLMaterial(this, texFiles, vShaderFile, fShaderFile));

	return entry.get();
}

void ResourceLoader::Cleanup() {
	for (auto& tp : textures)
		glDeleteTextures(1, &tp.second);

	textures.clear();
	materials.clear();
}

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	std::hash<std::string> strHash;
	size_t resHash = strHash(std::get<1>(key)) ^ strHash(std::get<2>(key));

	for (size_t i = 0; i < std::get<0>(key).size(); i++)
		resHash ^= strHash(std::get<0>(key)[i]);

	resHash ^= std::hash<bool>{}(std::get<3>(key));

	return resHash;
}
