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

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

using std::string;

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	std::hash<std::string> strHash;
	return (strHash(std::get<0>(key)) ^ strHash(std::get<1>(key)) ^ strHash(std::get<2>(key)));
}

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

void ResourceLoader::GetArchiveFiles(vector<string>& outList) {
	string cp = "GameDataFiles";
	int targ = Config.GetIntValue("TargetGame");

	switch (targ) {
	case 0:
		cp += "/Fallout3";
		break;
	case 1:
		cp += "/FalloutNewVegas";
		break;
	case 2:
		cp += "/Skyrim";
		break;
	case 3:
		cp += "/Fallout4";
		break;
	}

	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false);
		val = val.Trim();
		std::transform(val.begin(), val.end(), val.begin(), ::tolower);
		fsearch[val] = true;
	}
	wxString dataDir = Config["GameDataPath"];
	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& f : files) {
		f = f.AfterLast('\\');
		std::transform(f.begin(), f.end(), f.begin(), ::tolower);
		if (fsearch.find(f) == fsearch.end()) {
			outList.push_back(dataDir.ToStdString() + f.ToStdString());
		}
	}
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
		if (Config["GameDataPath"].empty()) {
			wxLogWarning("Texture file '%s' not found.", textureFile);
			return nullptr;
		}

		// Auto-detect archives
		if (!FSManager::exists()) {
			vector<string> fileList;
			GetArchiveFiles(fileList);

			vector<string> archives;
			for (auto &file : fileList)
					archives.push_back(file);

			FSManager::addArchives(archives);
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
			textureID = SOIL_load_OGL_texture_from_memory(texBuffer, data.GetBufSize(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS);
		}
		else {
			wxLogWarning("Texture file '%s' not found.", textureFile);
			return nullptr;
		}
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(textureID, vShaderFile.c_str(), fShaderFile.c_str()));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}
