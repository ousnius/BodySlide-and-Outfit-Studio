#include "ResourceLoader.h"
#include "GLShader.h"

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

	unsigned int texid = SOIL_load_OGL_texture(textureFile.c_str(), SOIL_LOAD_AUTO, 0, SOIL_FLAG_TEXTURE_REPEATS);
	if (!texid) {
		auto errstr = SOIL_last_result();
		return nullptr;
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(texid, vShaderFile.c_str(), fShaderFile.c_str()));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}
