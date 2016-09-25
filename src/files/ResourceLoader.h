/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
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

	static unsigned int currentTextureID;

	GLMaterial* AddMaterial(const string& textureFile,
				const string& vShaderFile,
				const string& fShaderFile);

	void Cleanup();

private:
	static bool extChecked;
	GLuint GLI_create_texture(gli::texture& texture);
	GLuint GLI_load_texture(const string& fileName);
	GLuint GLI_load_texture_from_memory(const char* buffer, size_t size);

	// If N3983 gets accepted into a future C++ standard then
	// we wouldn't have to explicitly define our own hash here.
	typedef tuple<string, string, string> MaterialKey;
	struct MatKeyHash {
		size_t operator()(const MaterialKey& key) const;
	};
	typedef unordered_map<MaterialKey, unique_ptr<GLMaterial>, MatKeyHash> MaterialCache;

	MaterialCache materials;
};
