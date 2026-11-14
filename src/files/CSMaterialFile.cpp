/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "CSMaterialFile.h"

#include "GameDataStream.h"
#include "SFMaterialResolver.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <regex>

namespace {
using M = CSMaterial;
using K = CSMaterial::KeyKind;

std::string Lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

bool EndsWithInsensitive(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size() && Lower(s.substr(s.size() - suffix.size())) == suffix;
}

void SetError(std::string* error, const std::string& text) {
	if (error)
		*error = text;
}

bool IsTrue(const std::optional<bool>& value) {
	return value.value_or(false);
}
} // namespace

const std::vector<CSMaterial::Key>& CSMaterial::Keys() {
	static const std::vector<Key> keys = {
		{"RootMaterialPath", K::String, &M::rootMaterialPath},

		{"DiffuseTexture", K::Texture, &M::diffuseTexture, {}, {}, {}, 0},
		{"NormalTexture", K::Texture, &M::normalTexture, {}, {}, {}, 1},
		{"GlowTexture", K::Texture, &M::glowTexture, {}, {}, {}, 2},
		{"DisplacementTexture", K::Texture, &M::displacementTexture, {}, {}, {}, 3},
		{"RmaosTexture", K::Texture, &M::rmaosTexture, {}, {}, {}, 5},
		{"Features0Texture", K::Texture, &M::features0Texture, {}, {}, {}, 7},
		{"Features1Texture", K::Texture, &M::features1Texture, {}, {}, {}, 6},

		{"TileU", K::Bool, &M::tileU},
		{"TileV", K::Bool, &M::tileV},
		{"UOffset", K::Number, &M::uOffset},
		{"VOffset", K::Number, &M::vOffset},
		{"UScale", K::Number, &M::uScale},
		{"VScale", K::Number, &M::vScale},
		{"Alpha", K::Number, &M::alpha, 0.0, 128.0},

		{"TwoSided", K::Bool, &M::twoSided},
		{"Decal", K::Bool, &M::decal},
		{"DynamicDecal", K::Bool, &M::dynamicDecal},
		{"ModelSpaceNormals", K::Bool, &M::modelSpaceNormals},
		{"VertexColors", K::Bool, &M::vertexColors},
		{"CastShadows", K::Bool, &M::castShadows},
		{"ReceiveShadows", K::Bool, &M::receiveShadows},
		{"ZBufferTest", K::Bool, &M::zBufferTest},
		{"ZBufferWrite", K::Bool, &M::zBufferWrite},

		{"ExternalEmittance", K::Bool, &M::externalEmittance},
		{"EmittanceColor", K::Color, &M::emittanceColor},
		{"EmittanceMult", K::Number, &M::emittanceMult, 0.0},

		{"PBREnabled", K::Bool, &M::pbrEnabled},
		{"PBRDisplacementTextureScale", K::Number, &M::pbrDisplacementTextureScale, 0.0, {}, 1.0},
		{"PBRSpecularLevel", K::Number, &M::pbrSpecularLevel, 0.0, 1.0, 0.04},
		{"PBRRoughnessScale", K::Number, &M::pbrRoughnessScale, 0.0, {}, 1.0},

		{"PBRSubsurfaceEnabled", K::Bool, &M::pbrSubsurfaceEnabled},
		{"PBRSubsurfaceColor", K::Color, &M::pbrSubsurfaceColor},
		{"PBRSubsurfaceOpacity", K::Number, &M::pbrSubsurfaceOpacity, 0.0, 1.0, 0.0},

		{"PBRThinSurfaceEnabled", K::Bool, &M::pbrThinSurfaceEnabled},
		{"PBRThinSurfaceSubsurfaceBalance", K::Number, &M::pbrThinSurfaceSubsurfaceBalance, 0.0, 1.0, 0.75},
		{"PBRThinSurfaceSpecularWeight", K::Number, &M::pbrThinSurfaceSpecularWeight, 0.0, 1.0, 0.5},
		{"PBRThinSurfaceBackSpecularWeight", K::Number, &M::pbrThinSurfaceBackSpecularWeight, 0.0, 1.0, 0.5},
		{"PBRThinSurfaceBackRoughnessScale", K::Number, &M::pbrThinSurfaceBackRoughnessScale, 0.0, 3.0, 1.0},

		{"PBRTwoLayerEnabled", K::Bool, &M::pbrTwoLayerEnabled},
		{"PBRTwoLayerColoredCoat", K::Bool, &M::pbrTwoLayerColoredCoat},
		{"PBRTwoLayerInterlayerParallax", K::Bool, &M::pbrTwoLayerInterlayerParallax},
		{"PBRTwoLayerCoatNormal", K::Bool, &M::pbrTwoLayerCoatNormal},
		{"PBRTwoLayerCoatColor", K::Color, &M::pbrTwoLayerCoatColor},
		{"PBRTwoLayerCoatStrength", K::Number, &M::pbrTwoLayerCoatStrength, 0.0, 1.0, 1.0},
		{"PBRTwoLayerCoatRoughness", K::Number, &M::pbrTwoLayerCoatRoughness, 0.0, 1.0, 1.0},
		{"PBRTwoLayerCoatSpecularLevel", K::Number, &M::pbrTwoLayerCoatSpecularLevel, 0.0, 1.0, 0.04},

		{"PBRFuzzEnabled", K::Bool, &M::pbrFuzzEnabled},
		{"PBRFuzzColor", K::Color, &M::pbrFuzzColor},
		{"PBRFuzzWeight", K::Number, &M::pbrFuzzWeight, 0.0, 1.0, 0.0},
		{"PBRFuzzRoughness", K::Number, &M::pbrFuzzRoughness, {}, 1.0, -1.0},
		{"PBRFuzzRoughnessTexture", K::Bool, &M::pbrFuzzRoughnessTexture},

		{"PBRGlintEnabled", K::Bool, &M::pbrGlintEnabled},
		{"PBRGlintScreenSpaceScale", K::Number, &M::pbrGlintScreenSpaceScale, 1.0, {}, 1.5},
		{"PBRGlintLogMicrofacetDensity", K::Number, &M::pbrGlintLogMicrofacetDensity, 1.0, 40.0, 40.0},
		{"PBRGlintMicrofacetRoughness", K::Number, &M::pbrGlintMicrofacetRoughness, 0.005, 0.3, 0.015},
		{"PBRGlintDensityRandomization", K::Number, &M::pbrGlintDensityRandomization, 0.0, 5.0, 2.0},

		{"PBRHairMarschner", K::Bool, &M::pbrHairMarschner},
	};

	return keys;
}

const CSMaterial::Key* CSMaterial::FindKey(const std::string& name) {
	for (const Key& key : Keys())
		if (name == key.name)
			return &key;

	return nullptr;
}

std::string CSMaterial::FormatColor(const nifly::Vector3& color) {
	auto channel = [](float c) { return static_cast<int>(std::lround(std::clamp(c, 0.0f, 1.0f) * 255.0f)); };

	char text[8];
	std::snprintf(text, sizeof(text), "#%02x%02x%02x", channel(color.x), channel(color.y), channel(color.z));
	return text;
}

bool CSMaterial::ParseColor(const std::string& text, nifly::Vector3& color) {
	static const std::regex pattern("^#[0-9a-fA-F]{6}$");
	if (!std::regex_match(text, pattern))
		return false;

	auto channel = [&text](size_t offset) { return std::stoi(text.substr(offset, 2), nullptr, 16) / 255.0f; };
	color = nifly::Vector3(channel(1), channel(3), channel(5));
	return true;
}

bool CSMaterial::SetKey(const std::string& name, const nlohmann::json& value, std::string* error) {
	const Key* key = FindKey(name);
	if (!key) {
		SetError(error, "Unknown key '" + name + "'.");
		return false;
	}

	if (value.is_null()) {
		std::visit([this](auto member) { (this->*member).reset(); }, key->member);
		return true;
	}

	switch (key->kind) {
		case K::Bool:
			if (!value.is_boolean()) {
				SetError(error, std::string(key->name) + " must be true or false.");
				return false;
			}
			this->*std::get<std::optional<bool> CSMaterial::*>(key->member) = value.get<bool>();
			return true;

		case K::Number: {
			if (!value.is_number()) {
				SetError(error, std::string(key->name) + " must be a number.");
				return false;
			}

			const double number = value.get<double>();
			if ((key->min && number < *key->min) || (key->max && number > *key->max)) {
				std::string range;
				if (key->min && key->max)
					range = "from " + nlohmann::json(*key->min).dump() + " to " + nlohmann::json(*key->max).dump();
				else if (key->min)
					range = "at least " + nlohmann::json(*key->min).dump();
				else
					range = "at most " + nlohmann::json(*key->max).dump();

				SetError(error, std::string(key->name) + " must be " + range + ", not " + value.dump() + ".");
				return false;
			}

			this->*std::get<std::optional<double> CSMaterial::*>(key->member) = number;
			return true;
		}

		case K::Color: {
			nifly::Vector3 color;
			if (!value.is_string() || !ParseColor(value.get<std::string>(), color)) {
				SetError(error, std::string(key->name) + " must be a color as \"#rrggbb\", not " + value.dump() + ".");
				return false;
			}
			this->*std::get<std::optional<nifly::Vector3> CSMaterial::*>(key->member) = color;
			return true;
		}

		case K::Texture:
		case K::String: {
			const char* extension = key->kind == K::Texture ? ".dds" : ".csm";
			if (!value.is_string()) {
				SetError(error, std::string(key->name) + " must be a string.");
				return false;
			}

			const std::string text = value.get<std::string>();
			if (!text.empty() && !EndsWithInsensitive(text, extension)) {
				SetError(error, std::string(key->name) + " must be empty or a path ending in " + extension + ", not \"" + text + "\".");
				return false;
			}

			this->*std::get<std::optional<std::string> CSMaterial::*>(key->member) = text;
			return true;
		}
	}

	return false;
}

bool CSMaterial::HasKey(const std::string& name) const {
	const Key* key = FindKey(name);
	if (!key)
		return false;

	return std::visit([this](auto member) { return (this->*member).has_value(); }, key->member);
}

nlohmann::json CSMaterial::GetKey(const std::string& name) const {
	const Key* key = FindKey(name);
	if (!key)
		return nullptr;

	switch (key->kind) {
		case K::Bool: {
			auto& value = this->*std::get<std::optional<bool> CSMaterial::*>(key->member);
			return value ? nlohmann::json(*value) : nlohmann::json();
		}
		case K::Number: {
			auto& value = this->*std::get<std::optional<double> CSMaterial::*>(key->member);
			return value ? nlohmann::json(*value) : nlohmann::json();
		}
		case K::Color: {
			auto& value = this->*std::get<std::optional<nifly::Vector3> CSMaterial::*>(key->member);
			return value ? nlohmann::json(FormatColor(*value)) : nlohmann::json();
		}
		case K::Texture:
		case K::String: {
			auto& value = this->*std::get<std::optional<std::string> CSMaterial::*>(key->member);
			return value ? nlohmann::json(*value) : nlohmann::json();
		}
	}

	return nullptr;
}

bool CSMaterial::FromJson(const nlohmann::json& json, std::string* error) {
	*this = CSMaterial();

	if (!json.is_object()) {
		SetError(error, "A material file has to be a JSON object.");
		return false;
	}

	auto versionValue = json.find("Version");
	if (versionValue == json.end()) {
		SetError(error, "The material has no Version.");
		return false;
	}

	if (!versionValue->is_number() || versionValue->get<double>() != static_cast<double>(SupportedVersion)) {
		SetError(error, "Version " + versionValue->dump() + " isn't supported, only version 1 is.");
		return false;
	}
	version = SupportedVersion;

	for (auto& [name, value] : json.items()) {
		if (name == "Version")
			continue;

		if (name == "$schema") {
			if (!value.is_string()) {
				SetError(error, "$schema must be a string.");
				return false;
			}
			schema = value.get<std::string>();
			continue;
		}

		// A null isn't a value the format allows anywhere, unlike in SetKey where it clears a key
		if (value.is_null()) {
			SetError(error, name + " can't be null.");
			return false;
		}

		if (!SetKey(name, value, error))
			return false;
	}

	return true;
}

bool CSMaterial::Read(std::istream& input, std::string* error) {
	*this = CSMaterial();

	// Strict JSON: the parser Community Shaders reads materials with takes no comments either
	nlohmann::json json = nlohmann::json::parse(input, nullptr, false, false);
	if (json.is_discarded()) {
		SetError(error, "The file isn't valid JSON.");
		return false;
	}

	return FromJson(json, error);
}

nlohmann::ordered_json CSMaterial::ToJson() const {
	nlohmann::ordered_json json;
	if (schema)
		json["$schema"] = *schema;

	json["Version"] = version;

	for (const Key& key : Keys()) {
		nlohmann::json value = GetKey(key.name);
		if (!value.is_null())
			json[key.name] = value;
	}

	return json;
}

bool CSMaterial::Write(std::ostream& output) const {
	output << ToJson().dump(2) << "\n";
	return !output.fail();
}

void CSMaterial::MergeOver(const CSMaterial& root) {
	for (const Key& key : Keys()) {
		if (key.member == Member(&M::rootMaterialPath))
			continue;

		std::visit(
			[this, &root](auto member) {
				if (!(this->*member) && (root.*member))
					this->*member = root.*member;
			},
			key.member);
	}
}

bool CSMaterial::Validate(std::string* error) const {
	struct Excludes {
		std::optional<bool> CSMaterial::* feature;
		const char* name;
		std::vector<std::pair<std::optional<bool> CSMaterial::*, const char*>> excluded;
	};

	static const std::vector<Excludes> exclusions = {
		{&M::pbrHairMarschner,
		 "PBRHairMarschner",
		 {{&M::pbrSubsurfaceEnabled, "PBRSubsurfaceEnabled"},
		  {&M::pbrThinSurfaceEnabled, "PBRThinSurfaceEnabled"},
		  {&M::pbrTwoLayerEnabled, "PBRTwoLayerEnabled"},
		  {&M::pbrTwoLayerColoredCoat, "PBRTwoLayerColoredCoat"},
		  {&M::pbrTwoLayerInterlayerParallax, "PBRTwoLayerInterlayerParallax"},
		  {&M::pbrTwoLayerCoatNormal, "PBRTwoLayerCoatNormal"},
		  {&M::pbrFuzzEnabled, "PBRFuzzEnabled"},
		  {&M::pbrGlintEnabled, "PBRGlintEnabled"}}},
		{&M::pbrTwoLayerEnabled,
		 "PBRTwoLayerEnabled",
		 {{&M::pbrSubsurfaceEnabled, "PBRSubsurfaceEnabled"},
		  {&M::pbrThinSurfaceEnabled, "PBRThinSurfaceEnabled"},
		  {&M::pbrFuzzEnabled, "PBRFuzzEnabled"},
		  {&M::pbrGlintEnabled, "PBRGlintEnabled"}}},
		{&M::pbrFuzzEnabled, "PBRFuzzEnabled", {{&M::pbrGlintEnabled, "PBRGlintEnabled"}}},
	};

	for (auto& rule : exclusions) {
		if (!IsTrue(this->*rule.feature))
			continue;

		for (auto& [other, otherName] : rule.excluded) {
			if (IsTrue(this->*other)) {
				SetError(error, std::string(rule.name) + " can't be on together with " + otherName + ".");
				return false;
			}
		}
	}

	// The coat options need the coat. Only a coat that is set to off counts against them here, the
	// same as in the format's schema; one left out is whatever the root or NIF has.
	static const std::vector<std::pair<std::optional<bool> CSMaterial::*, const char*>> coatOptions = {
		{&M::pbrTwoLayerColoredCoat, "PBRTwoLayerColoredCoat"},
		{&M::pbrTwoLayerInterlayerParallax, "PBRTwoLayerInterlayerParallax"},
		{&M::pbrTwoLayerCoatNormal, "PBRTwoLayerCoatNormal"},
	};

	for (auto& [option, optionName] : coatOptions) {
		if (IsTrue(this->*option) && pbrTwoLayerEnabled && !*pbrTwoLayerEnabled) {
			SetError(error, std::string(optionName) + " needs PBRTwoLayerEnabled.");
			return false;
		}
	}

	struct Needs {
		std::optional<bool> CSMaterial::* feature;
		const char* name;
		std::vector<std::pair<std::optional<std::string> CSMaterial::*, const char*>> textures;
	};

	static const std::vector<Needs> textureNeeds = {
		{&M::pbrEnabled,
		 "PBREnabled",
		 {{&M::diffuseTexture, "DiffuseTexture"}, {&M::normalTexture, "NormalTexture"}, {&M::rmaosTexture, "RmaosTexture"}}},
		{&M::pbrTwoLayerInterlayerParallax, "PBRTwoLayerInterlayerParallax", {{&M::displacementTexture, "DisplacementTexture"}}},
		{&M::pbrTwoLayerCoatNormal, "PBRTwoLayerCoatNormal", {{&M::features1Texture, "Features1Texture"}}},
	};

	for (auto& rule : textureNeeds) {
		if (!IsTrue(this->*rule.feature))
			continue;

		for (auto& [texture, textureName] : rule.textures) {
			auto& value = this->*texture;
			if (value && value->empty()) {
				SetError(error, std::string(rule.name) + " needs " + textureName + ".");
				return false;
			}
		}
	}

	return true;
}

bool CSMaterial::ValidateResolved(const std::vector<std::string>& slots, std::string* error) const {
	CSMaterial resolved = *this;

	for (const Key& key : Keys()) {
		if (key.kind == K::Texture) {
			auto& texture = resolved.*std::get<std::optional<std::string> CSMaterial::*>(key.member);
			texture = key.slot >= 0 && static_cast<size_t>(key.slot) < slots.size() ? slots[key.slot] : std::string();
		}
		else if (key.kind == K::Bool && std::string(key.name).rfind("PBR", 0) == 0) {
			// A NIF has nothing to say about True PBR, so whatever the material leaves out is off
			auto& value = resolved.*std::get<std::optional<bool> CSMaterial::*>(key.member);
			if (!value)
				value = false;
		}
	}

	return resolved.Validate(error);
}

void CSMaterial::ApplyTextures(std::vector<std::string>& slots) const {
	if (slots.size() < 8)
		slots.resize(8);

	for (const Key& key : Keys()) {
		if (key.kind != K::Texture)
			continue;

		auto& texture = this->*std::get<std::optional<std::string> CSMaterial::*>(key.member);
		if (texture)
			slots[key.slot] = *texture;
	}
}

bool CSMaterial::operator==(const CSMaterial& other) const {
	if (version != other.version || schema != other.schema)
		return false;

	for (const Key& key : Keys())
		if (GetKey(key.name) != other.GetKey(key.name))
			return false;

	return true;
}

bool CSMaterialResolver::IsMaterialPath(const std::string& shaderName) {
	return EndsWithInsensitive(shaderName, ".csm");
}

std::string CSMaterialResolver::NormalizePath(const std::string& path) {
	// The folder itself in lower case, so that the same material named two ways is one path
	std::string normalized = SFMaterialResolver::NormalizeMaterialPath(path, "");
	normalized.replace(0, 10, "materials/");
	return normalized;
}

bool CSMaterialResolver::Resolve(const std::string& path, const Loader& load, CSMaterial& material, std::string* error, std::vector<std::string>* chain) {
	material = CSMaterial();
	if (chain)
		chain->clear();

	if (!load) {
		SetError(error, "Nothing to read materials from.");
		return false;
	}

	std::vector<std::string> visited;
	std::string current = NormalizePath(path);
	CSMaterial merged;

	for (int depth = 0;; depth++) {
		if (depth > MaxChainDepth) {
			SetError(error, "The chain of root materials of '" + NormalizePath(path) + "' is deeper than " + std::to_string(MaxChainDepth) + ".");
			return false;
		}

		const std::string key = Lower(current);
		if (std::find(visited.begin(), visited.end(), key) != visited.end()) {
			SetError(error, "The root materials of '" + NormalizePath(path) + "' loop back to '" + current + "'.");
			return false;
		}
		visited.push_back(key);

		if (chain)
			chain->push_back(current);

		std::unique_ptr<std::istream> stream = load(current);
		if (!stream) {
			SetError(error, depth == 0 ? "'" + current + "' wasn't found." : "The root material '" + current + "' wasn't found.");
			return false;
		}

		CSMaterial file;
		std::string fileError;
		if (!file.Read(*stream, &fileError) || !file.Validate(&fileError)) {
			SetError(error, "'" + current + "': " + fileError);
			return false;
		}

		if (depth == 0)
			merged = std::move(file);
		else
			merged.MergeOver(file);

		const std::string& root = depth == 0 ? merged.rootMaterialPath.value_or(std::string()) : file.rootMaterialPath.value_or(std::string());
		if (root.empty())
			break;

		current = NormalizePath(root);
	}

	std::string mergedError;
	if (!merged.Validate(&mergedError)) {
		SetError(error, "'" + NormalizePath(path) + "' merged with its root materials: " + mergedError);
		return false;
	}

	material = std::move(merged);
	return true;
}

std::shared_ptr<const CSMaterial> CSMaterialResolver::ResolveForShape(const std::string& shaderName,
																	 const Loader& load,
																	 std::vector<std::string>& textures,
																	 std::string* error) {
	auto material = std::make_shared<CSMaterial>();
	if (!Resolve(shaderName, load, *material, error))
		return nullptr;

	std::vector<std::string> resolvedTextures = textures;
	material->ApplyTextures(resolvedTextures);

	std::string resolvedError;
	if (!material->ValidateResolved(resolvedTextures, &resolvedError)) {
		SetError(error, "'" + NormalizePath(shaderName) + "' with the textures of the shape: " + resolvedError);
		return nullptr;
	}

	textures = std::move(resolvedTextures);
	return material;
}

CSMaterialResolver::Loader CSMaterialResolver::GameDataLoader(const std::string& baseDataPath) {
	return [baseDataPath](const std::string& relPath) -> std::unique_ptr<std::istream> {
		const std::string fullPath = baseDataPath.empty() ? GameDataStream::FindAssetPath(relPath)
														  : GameDataStream::FindAssetPath(relPath, baseDataPath);
		if (auto loose = GameDataStream::OpenLoose(fullPath))
			return loose;

		return GameDataStream::OpenArchive(relPath);
	};
}
