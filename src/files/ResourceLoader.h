/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../utils/StringStuff.h"

#include <Object3d.hpp>
#include <SOIL2/SOIL2.h>
#include <gli.hpp>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
class GLMaterial;

class ResourceLoader {
public:
	ResourceLoader();
	~ResourceLoader();

	// useDefaultTexture substitutes the "no image" placeholder for a missing diffuse. Only pass true for shapes that
	// have a shader to begin with - a shape without one has no textures by definition and is meant to render untextured.
	GLMaterial* AddMaterial(const std::vector<std::string>& textureFiles,
							const std::string& vShaderFile,
							const std::string& fShaderFile,
							const bool reloadTextures = false,
							const bool useDefaultTexture = true);


	//Central Point for loading texture files.  Calls appropriate resource loading subroutine, and
	// tracks the resulting GL texture identifier so subsequent access to the same texture does not result
	// in a new load.
	GLuint LoadTexture(const std::string& fileName, bool isCubeMap = false, bool reloadTextures = false);

	// The following functions manage non-file-sourced texture ids.  This facilitates named textures generated
	//  within the program either for temporary use (generate/delete) or persistent use
	GLuint GenerateTextureID(const std::string& texName);

	GLuint GetTexID(const std::string& texName);


	/* The following functions update cacheTime, which will cause any linked material to re-search for texture ids.
		while this is not a tremendous performance impact, these functions should not be called every frame.
		Functions above (that add new textures) do NOT update the cacheTime, because old Texture IDs remain valid.
		*/

	// Deletes all materials and textures.
	void Cleanup();
	// Deletes a specfic texture and removes it from video memory.
	void DeleteTexture(const std::string& texName);
	// rename texture moves the internal texture id to a new name. this allows a function to safely 'steal' a
	//  texture generated elsewhere.  EG: in GLOffscreenBuffer, while the buffer is active, one of the textures
	//  generated can be renamed to prevent the texture from being deleted when the object is deleted, so it can
	//  be used elsewhere.
	//  Fails if texture already exist and overwrite is not set to true.
	bool RenameTexture(const std::string& texNameSrc, const std::string& texNameDest, bool overwrite = false);


	// Whether the texture looks like a Skyrim "Complex Material" mask: glossiness in green and
	//  metalness in blue, instead of the greyscale reflection mask vanilla puts in the same slot.
	//  Classified once when the texture is loaded into the slot that carries it, since the verdict
	//  is about the map as a whole and can't be made from the texel a fragment happens to sample.
	bool IsComplexMaterialTexture(const std::string& texName) const;

	// Highest mip level the texture actually has, 0 for one without a mip chain. Reflections pick
	//  their mip from roughness, so the shader needs the real number rather than an assumed one.
	int GetTextureMaxMipLevel(const std::string& texName) const;

	// Edge length of the cube map's base level, 0 for a texture that isn't one. A 1x1 cube map holds
	//  no reflection anybody wants; it is how Community Shaders and ENB mods mark a slot as wanting
	//  a dynamic cube map instead, which is the only reason the size is worth remembering.
	int GetCubemapSize(const std::string& texName) const;

	// What a 1x1 cube map's single texel stood for, read as an sRGB F0 reflectance and returned
	//  linear. Black means full reflectance rather than a black reflection, so it comes back as 1.0,
	//  which is also what any cube map that isn't 1x1 returns.
	nifly::Vector3 GetCubemapF0Color(const std::string& texName) const;

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
	GLuint GLI_create_texture(gli::texture& texture, GLuint textureID = 0);
	GLuint GLI_load_texture(const std::string& fileName, GLuint textureID = 0);
	GLuint GLI_load_texture_from_memory(const char* buffer, size_t size, GLuint textureID = 0);

	// Highest level with actual storage for the currently bound texture. GL_TEXTURE_MAX_LEVEL isn't
	//  usable here: the SOIL path never sets it and leaves it at its 1000 default, so the levels are
	//  walked until one comes back with no width. Pass the cube map's first face for a cube map.
	static int GetBoundMaxMipLevel(GLenum levelTarget);
	// Reads the smallest mip - the average of the whole texture - and decides from it.
	bool ClassifyComplexMaterial(GLuint textureID) const;
	// Records the bound cube map's size, and for a 1x1 one the color its texel stands for.
	void ClassifyCubemap(const std::string& texName, GLuint textureID);

	// If N3983 gets accepted into a future C++ standard then
	// we wouldn't have to explicitly define our own hash here.
	// The default texture flag is part of the key: the same (empty) texture list resolves to a different
	// material depending on whether the "no image" placeholder was substituted for the diffuse.
	typedef std::tuple<std::vector<std::string>, std::string, std::string, bool> MaterialKey;
	struct MatKeyHash {
		size_t operator()(const MaterialKey& key) const;
	};
	typedef std::unordered_map<MaterialKey, std::unique_ptr<GLMaterial>, MatKeyHash> MaterialCache;
	// defining texture cache like this both for consitency, might also make it easier to add features like
	// reference tracking later.  For now, Textures are only unloaded when ResourceLoader is destroyed.
	typedef std::map<std::string, GLuint, case_insensitive_compare> TextureCache;

	TextureCache textures;
	MaterialCache materials;
	// Keyed by texture name like the texture cache, so a verdict outlives the material that
	// triggered it and survives the material cache handing back an existing entry.
	std::map<std::string, bool, case_insensitive_compare> complexMaterialTextures;
	std::map<std::string, int, case_insensitive_compare> textureMaxMipLevels;
	std::map<std::string, int, case_insensitive_compare> cubemapSizes;
	std::map<std::string, nifly::Vector3, case_insensitive_compare> cubemapF0Colors;

	int64_t cacheTime = 1;
};
