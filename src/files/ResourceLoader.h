/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#pragma warning (push, 0)
#include "gli/gli.hpp"
#pragma warning (pop)

using namespace std;

typedef unsigned int GLuint;
class GLMaterial;


class ResourceLoader {
public:
	ResourceLoader();
	~ResourceLoader();

	GLMaterial* AddMaterial(const vector<string>& textureFiles,
		const string& vShaderFile,
		const string& fShaderFile);


	//Central Point for loading texture files.  Calls appropriate resource loading subroutine, and 
	// tracks the resulting GL texture identifier so subsequent access to the same texture does not result
	// in a new load. 
	GLuint LoadTexture(const string& fileName, bool isCubeMap = false);

	// The following functions manage non-file-sourced texture ids.  This facilitates named textures generated
	//  within the program either for temporary use (generate/delete) or persistent use
	GLuint GenerateTextureID(const string& texName);

	GLuint GetTexID(const string& texName);


	/* The following functions update cacheTime, which will cause any linked material to re-search for texture ids.
		while this is not a tremendous performance impact, these functions should not be called every frame.
		Functions above (that add new textures) do NOT update the cacheTime, because old Texture IDs remain valid.
		*/

	// Deletes all materials and textures.
	void Cleanup();
	// Deletes a specfic texture and removes it from video memory.
	void DeleteTexture(const string& texName);
	// rename texture moves the internal texture id to a new name. this allows a function to safely 'steal' a 
	//  texture generated elsewhere.  EG: in GLOffscreenBuffer, while the buffer is active, one of the textures
	//  generated can be renamed to prevent the texture from being deleted when the object is deleted, so it can 
	//  be used elsewhere. 
	//  Fails if texture already exist and overwrite is not set to true.
	bool RenameTexture(const string& texNameSrc, const string& texNameDest, bool overwrite = false);


	// compares the incoming cacheTime with the internal cacheTime, and returns true if they match.  
	//  if they do not match, the incoming cacheTime is updated to match and the function returns false.
	bool CacheStamp(int64_t& inCacheTime) {
		if (cacheTime == inCacheTime)
			return true;
		else {
			inCacheTime = cacheTime;
			return false;
		}
	}

private:
	static bool extChecked;
	GLuint GLI_create_texture(gli::texture& texture);
	GLuint GLI_load_texture(const string& fileName);
	GLuint GLI_load_texture_from_memory(const char* buffer, size_t size);

	// If N3983 gets accepted into a future C++ standard then
	// we wouldn't have to explicitly define our own hash here.
	typedef tuple<vector<string>, string, string> MaterialKey;
	struct MatKeyHash {
		size_t operator()(const MaterialKey& key) const;
	};
	typedef unordered_map<MaterialKey, unique_ptr<GLMaterial>, MatKeyHash> MaterialCache;
	// defining texture cache like this both for consitency, might also make it easier to add features like
	// reference tracking later.  For now, Textures are only unloaded when ResourceLoader is destroyed.
	typedef unordered_map<string, GLuint> TextureCache;

	TextureCache textures;
	MaterialCache materials;

	int64_t cacheTime = 1;
};
