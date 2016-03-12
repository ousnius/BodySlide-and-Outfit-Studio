/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "ResourceLoader.h"
#include "GLShader.h"
#include "ConfigurationManager.h"
#include "FSManager.h"
#include "FSEngine.h"
#include "SOIL2.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

unsigned int ResourceLoader::currentTextureID = 0;

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	hash<string> strHash;
	return (strHash(get<0>(key)) ^ strHash(get<1>(key)) ^ strHash(get<2>(key)));
}

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

GLMaterial* ResourceLoader::AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile) {
	MaterialKey key(textureFile, vShaderFile, fShaderFile);
	auto it = materials.find(key);
	if (it != materials.end())
		return it->second.get();

	uint textureID = SOIL_load_OGL_texture(textureFile.c_str(), SOIL_LOAD_AUTO, ++currentTextureID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
	if (!textureID && Config.MatchValue("BSATextureScan", "true")) {
		if (Config["GameDataPath"].empty()) {
			wxLogWarning("Texture file '%s' not found.", textureFile);
			return nullptr;
		}

		wxMemoryBuffer data;
		wxString texFile = textureFile;
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
			SOIL_load_OGL_texture_from_memory(texBuffer, data.GetBufSize(), SOIL_LOAD_AUTO, ++currentTextureID, SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_GL_MIPMAPS);
		}
		else {
			wxLogWarning("Texture file '%s' not found.", textureFile);
			return nullptr;
		}
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(currentTextureID, vShaderFile.c_str(), fShaderFile.c_str()));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}
