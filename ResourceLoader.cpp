/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "ResourceLoader.h"
#include "GLShader.h"
#include "ConfigurationManager.h"
#include "FSManager.h"
#include "FSEngine.h"

#include "SOIL.h"
#ifdef _DEBUG
#pragma comment (lib, "SOIL_d.lib")
#else
#pragma comment (lib, "SOIL.lib")
#endif

using std::string;

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	std::hash<std::string> strHash;
	return (strHash(std::get<0>(key)) ^ strHash(std::get<1>(key)) ^ strHash(std::get<2>(key)));
}

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

GLMaterial* ResourceLoader::AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile) {
	MaterialKey key(textureFile, vShaderFile, fShaderFile);
	auto it = materials.find(key);
	if (it != materials.end()) {
		return it->second.get();
	}

	// TODO: The textureFile string usually doesn't match the
	// uppercase/lowercase spelling of the file on disk.  On windows this
	// isn't an issue since the filesystem is case insensitive.
	// On Linux we should add code here that does a case-insensitive search
	// for the desired file.

	uint textureID = SOIL_load_OGL_texture(textureFile.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS);
	if (!textureID && Config.MatchValue("BSATextureScan", "true")) {
		if (Config["GameDataPath"].empty())
			return nullptr;

		wxMemoryBuffer data;
		wxString texFile = textureFile;
		texFile.Replace(Config["GameDataPath"], "");
		texFile.Replace("\\", "/");
		for (FSArchiveFile *archive : FSManager::archiveList()) {
			if (archive) {
				if (archive->hasFile(texFile)) {
					wxMemoryBuffer outData;
					archive->fileContents(texFile, outData);

					if (!outData.IsEmpty()) {
						data = outData;
						break;
					}
				}
			}
		}

		if (!data.IsEmpty()) {
			byte* texBuffer = static_cast<byte*>(data.GetData());
			textureID = SOIL_load_OGL_texture_from_memory(texBuffer, data.GetBufSize(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS);
		}
		else
			return nullptr;
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(textureID, vShaderFile.c_str(), fShaderFile.c_str()));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}
