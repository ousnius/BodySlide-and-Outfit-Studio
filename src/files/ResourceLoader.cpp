/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "../render/GLMaterial.h"
#include "../utils/ConfigurationManager.h"

#include "../FSEngine/FSManager.h"
#include "../FSEngine/FSEngine.h"

#include <wx/filename.h>
#include <wx/log.h>

extern ConfigurationManager Config;

ResourceLoader::ResourceLoader() {
}

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
		texFile.Replace(wxString(Config["GameDataPath"]).MakeLower(), "");
		texFile.Replace("\\", "/");
		for (FSArchiveFile *archive : FSManager::archiveList()) {
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
			byte* texBuffer = static_cast<byte*>(data.GetData());

			// All textures (GLI)
			if (fileExtStr == "dds" || fileExtStr == "ktx")
				textureID = GLI_load_texture_from_memory((char*)texBuffer, data.GetDataLen(), textureID);

			// Cubemap fallback (SOIL)
			if (!textureID && isCubeMap)
				textureID = SOIL_load_OGL_single_cubemap_from_memory(texBuffer, data.GetDataLen(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO, textureID, SOIL_FLAG_GL_MIPMAPS);

			// Texture and image fallback (SOIL)
			if (!textureID)
				textureID = SOIL_load_OGL_texture_from_memory(texBuffer, data.GetDataLen(), SOIL_LOAD_AUTO, textureID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
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

	std::transform(src.begin(), src.end(), src.begin(), ::tolower);
	std::transform(dst.begin(), dst.end(), dst.begin(), ::tolower);

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
	case gli::TARGET_1D:
		glTexStorage1D(
			target, static_cast<GLint>(texture.levels()), format.Internal, textureExtent.x);
		break;

	case gli::TARGET_1D_ARRAY:
	case gli::TARGET_2D:
	case gli::TARGET_CUBE:
		glTexStorage2D(
			target, static_cast<GLint>(texture.levels()), format.Internal,
			textureExtent.x, texture.target() != gli::TARGET_1D_ARRAY ? textureExtent.y : faceTotal);
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
				glm::tvec3<GLsizei> textureLevelExtent(texture.extent(level));
				target = gli::is_target_cube(texture.target())
					? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face)
					: target;

				switch (texture.target()) {
				case gli::TARGET_1D:
					if (gli::is_compressed(texture.format()))
						glCompressedTexSubImage1D(
						target, static_cast<GLint>(level), 0, textureLevelExtent.x,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage1D(
						target, static_cast<GLint>(level), 0, textureLevelExtent.x,
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
						textureLevelExtent.x,
						texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureLevelExtent.y,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage2D(
						target, static_cast<GLint>(level),
						0, 0,
						textureLevelExtent.x,
						texture.target() == gli::TARGET_1D_ARRAY ? layerGL : textureLevelExtent.y,
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
						textureLevelExtent.x, textureLevelExtent.y,
						texture.target() == gli::TARGET_3D ? textureLevelExtent.z : layerGL,
						format.Internal, static_cast<GLsizei>(texture.size(level)),
						texture.data(layer, face, level));
					else
						glTexSubImage3D(
						target, static_cast<GLint>(level),
						0, 0, 0,
						textureLevelExtent.x, textureLevelExtent.y,
						texture.target() == gli::TARGET_3D ? textureLevelExtent.z : layerGL,
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

GLMaterial* ResourceLoader::AddMaterial(const std::vector<std::string>& textureFiles, const std::string& vShaderFile, const std::string& fShaderFile, const bool reloadTextures) {
	auto texFiles = textureFiles;
	for (auto &f : texFiles)
		std::transform(f.begin(), f.end(), f.begin(), ::tolower);

	MaterialKey key(texFiles, vShaderFile, fShaderFile);
	if (!reloadTextures) {
		auto it = materials.find(key);
		if (it != materials.end())
			return it->second.get();
	}

	std::vector<GLuint> texRefs(texFiles.size(), 0);
	for (int i = 0; i < texFiles.size(); i++) {
		if (texFiles[i].empty())
			continue;

		bool isCubeMap = (i == 4);
		GLuint textureID = LoadTexture(texFiles[i], isCubeMap, reloadTextures);
		if (!textureID)
			continue;

		texRefs[i] = textureID;
	}

	// No diffuse found
	if (texRefs.empty())
		texRefs.resize(1, 0);

	if (texRefs[0] == 0) {
		// Load default image
		std::string defaultTex = Config["AppDir"] + "\\res\\images\\noimg.png";
		std::transform(defaultTex.begin(), defaultTex.end(), defaultTex.begin(), ::tolower);

		texRefs[0] = LoadTexture(defaultTex, false);

		if (!texFiles.empty())
			RenameTexture(defaultTex, texFiles[0]);
		else
			texFiles.resize(1, defaultTex);
	}

	auto& entry = materials[key];
	if (!entry || !reloadTextures)
		entry.reset(new GLMaterial(this, texFiles, vShaderFile, fShaderFile));

	return entry.get();
}

void ResourceLoader::Cleanup() {
	for (auto &tp : textures)
		glDeleteTextures(1, &tp.second);

	textures.clear();
	materials.clear();
}

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	std::hash<std::string> strHash;
	size_t resHash = strHash(std::get<1>(key)) ^ strHash(std::get<2>(key));

	for (int i = 0; i < std::get<0>(key).size(); i++)
		resHash ^= strHash(std::get<0>(key)[i]);

	return resHash;
}
