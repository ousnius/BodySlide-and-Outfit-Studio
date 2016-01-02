/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

using namespace std;

class GLMaterial;

class ResourceLoader {
public:
	ResourceLoader();
	virtual ~ResourceLoader();

	GLMaterial* AddMaterial(const string& textureFile,
				const string& vShaderFile,
				const string& fShaderFile);

	void Cleanup();

private:
	// If N3983 gets accepted into a future C++ standard then
	// we wouldn't have to explicitly define our own hash here.
	typedef tuple<string, string, string> MaterialKey;
	struct MatKeyHash {
		size_t operator()(const MaterialKey& key) const;
	};
	typedef unordered_map<MaterialKey, unique_ptr<GLMaterial>, MatKeyHash> MaterialCache;

	MaterialCache materials;
};
