/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

// A Community Shaders material (.csm) for Skyrim: a JSON file in Data\materials that a shape uses when
// the name of its BSLightingShaderProperty is the file's path. It overrides values of the shader
// property and its texture set - True PBR (version 2) keeps all of its parameters in one - and can
// build on a root material of its own through RootMaterialPath.
//
// Every key but Version is optional, and a key that is left out keeps the value the root material or
// the NIF has, so each one is held as an optional here: unset is not the same as set to its default.
// A file that cannot be read, or whose merged values are invalid, is ignored and the shape keeps its
// NIF values.
struct CSMaterial {
	// The only version there is so far. A file with any other one is ignored.
	static constexpr int SupportedVersion = 1;

	int version = SupportedVersion;
	std::optional<std::string> schema; // "$schema", kept for editors and otherwise ignored
	std::optional<std::string> rootMaterialPath;

	// Texture set, paths relative to Data\textures (or empty to leave a slot empty)
	std::optional<std::string> diffuseTexture;		// Slot 0
	std::optional<std::string> normalTexture;		// Slot 1
	std::optional<std::string> glowTexture;			// Slot 2
	std::optional<std::string> displacementTexture; // Slot 3
	std::optional<std::string> rmaosTexture;		// Slot 5
	std::optional<std::string> features0Texture;	// Slot 7
	std::optional<std::string> features1Texture;	// Slot 6

	std::optional<bool> tileU;
	std::optional<bool> tileV;
	std::optional<double> uOffset;
	std::optional<double> vOffset;
	std::optional<double> uScale;
	std::optional<double> vScale;
	std::optional<double> alpha;

	std::optional<bool> twoSided;
	std::optional<bool> decal;
	std::optional<bool> dynamicDecal;
	std::optional<bool> modelSpaceNormals;
	std::optional<bool> vertexColors;
	std::optional<bool> castShadows;
	std::optional<bool> receiveShadows;
	std::optional<bool> zBufferTest;
	std::optional<bool> zBufferWrite;

	std::optional<bool> externalEmittance;
	std::optional<nifly::Vector3> emittanceColor;
	std::optional<double> emittanceMult;

	std::optional<bool> pbrEnabled;
	std::optional<double> pbrDisplacementTextureScale;
	std::optional<double> pbrSpecularLevel;
	std::optional<double> pbrRoughnessScale;

	std::optional<bool> pbrSubsurfaceEnabled;
	std::optional<nifly::Vector3> pbrSubsurfaceColor;
	std::optional<double> pbrSubsurfaceOpacity;

	std::optional<bool> pbrThinSurfaceEnabled;
	std::optional<double> pbrThinSurfaceSubsurfaceBalance;
	std::optional<double> pbrThinSurfaceSpecularWeight;
	std::optional<double> pbrThinSurfaceBackSpecularWeight;
	std::optional<double> pbrThinSurfaceBackRoughnessScale;

	std::optional<bool> pbrTwoLayerEnabled;
	std::optional<bool> pbrTwoLayerColoredCoat;
	std::optional<bool> pbrTwoLayerInterlayerParallax;
	std::optional<bool> pbrTwoLayerCoatNormal;
	std::optional<nifly::Vector3> pbrTwoLayerCoatColor;
	std::optional<double> pbrTwoLayerCoatStrength;
	std::optional<double> pbrTwoLayerCoatRoughness;
	std::optional<double> pbrTwoLayerCoatSpecularLevel;

	std::optional<bool> pbrFuzzEnabled;
	std::optional<nifly::Vector3> pbrFuzzColor;
	std::optional<double> pbrFuzzWeight;
	std::optional<double> pbrFuzzRoughness;
	std::optional<bool> pbrFuzzRoughnessTexture;

	std::optional<bool> pbrGlintEnabled;
	std::optional<double> pbrGlintScreenSpaceScale;
	std::optional<double> pbrGlintLogMicrofacetDensity;
	std::optional<double> pbrGlintMicrofacetRoughness;
	std::optional<double> pbrGlintDensityRandomization;

	std::optional<bool> pbrHairMarschner;

	// One key of the file format. Reading, writing, validation and the MCP tools all go through this
	// table, so a key added to the format is added here once.
	enum class KeyKind { Bool, Number, Color, Texture, String };
	using Member = std::variant<std::optional<bool> CSMaterial::*,
								std::optional<double> CSMaterial::*,
								std::optional<nifly::Vector3> CSMaterial::*,
								std::optional<std::string> CSMaterial::*>;
	struct Key {
		const char* name;
		KeyKind kind;
		Member member;
		std::optional<double> min;
		std::optional<double> max;
		// The value a number key has when neither the material nor the NIF sets it, the one the format
		// calls neutral. Unset where that is whatever the NIF has.
		std::optional<double> neutral;
		// The texture set slot a texture key overrides, -1 for the others
		int slot = -1;
	};
	// Every key but $schema and Version, in the order the format lists them. RootMaterialPath is the
	// first, the only key a material doesn't take from its root.
	static const std::vector<Key>& Keys();
	static const Key* FindKey(const std::string& name);

	// Reads a file. Returns false and leaves a reason in "error" when the file isn't a version 1
	// material: not JSON (comments aren't JSON either), not an object, an unknown key, a value of the
	// wrong type, or one out of its range.
	bool Read(std::istream& input, std::string* error = nullptr);
	bool FromJson(const nlohmann::json& json, std::string* error = nullptr);

	// Writes the keys that are set, in the order of Keys(), as the 2-space indented JSON the format's
	// examples use.
	bool Write(std::ostream& output) const;
	nlohmann::ordered_json ToJson() const;

	// Sets one key from a JSON value the way a file would, or clears it with a null value. Returns
	// false, and changes nothing, for an unknown key or a value it doesn't take.
	bool SetKey(const std::string& name, const nlohmann::json& value, std::string* error = nullptr);
	bool HasKey(const std::string& name) const;
	nlohmann::json GetKey(const std::string& name) const; // Null when unset

	// Takes every key this material leaves out from "root", which is how a material builds on its root
	// material: whatever the material itself sets wins.
	void MergeOver(const CSMaterial& root);

	// Checks the rules between keys of the format: hair excludes the other PBR features, a two-layer
	// coat excludes subsurface, thin surface, fuzz and glint, fuzz excludes glint, the coat options need
	// the coat, and a feature that reads a texture needs it set. A key left out takes no part, the way
	// the format's schema checks a single file.
	bool Validate(std::string* error = nullptr) const;

	// The same once the texture set is known, the material's textures put over the NIF's ("slots", by
	// texture set slot): now a PBR feature left out is off and a texture left out is the NIF's, so PBR
	// also needs a base color, normal and RMAOS map to be there.
	bool ValidateResolved(const std::vector<std::string>& slots, std::string* error = nullptr) const;

	// Puts the textures this material sets over a texture set (indexed by slot, grown to 8 if shorter)
	void ApplyTextures(std::vector<std::string>& slots) const;

	// Whether the material renders with True PBR, which a material says with PBREnabled
	bool IsPBR() const { return pbrEnabled.value_or(false); }

	bool operator==(const CSMaterial& other) const;
	bool operator!=(const CSMaterial& other) const { return !(*this == other); }

	// "#rrggbb" for a color and back. ParseColor returns false for anything else.
	static std::string FormatColor(const nifly::Vector3& color);
	static bool ParseColor(const std::string& text, nifly::Vector3& color);
};

// Finds and reads Community Shaders materials, root materials included
class CSMaterialResolver {
public:
	// Opens a file by its data folder relative path ("materials/....csm"), or returns nullptr
	using Loader = std::function<std::unique_ptr<std::istream>(const std::string& relPath)>;

	// The longest chain of root materials followed before the chain counts as broken
	static constexpr int MaxChainDepth = 16;

	// Whether a shader name points to a Community Shaders material at all: it ends in .csm
	static bool IsMaterialPath(const std::string& shaderName);

	// "materials/....csm" for whatever form a shader name or root material path holds the path in
	static std::string NormalizePath(const std::string& path);

	// Reads the material at "path" and merges it over its chain of root materials. Returns false with a
	// reason in "error" when the material or any of its roots can't be found or read, when the chain
	// loops or goes deeper than MaxChainDepth, or when the merged material breaks the rules between
	// keys. "chain", when given, receives the normalized paths read, the material itself first.
	static bool Resolve(const std::string& path,
						const Loader& load,
						CSMaterial& material,
						std::string* error = nullptr,
						std::vector<std::string>* chain = nullptr);

	// Resolves the material a shape's shader names and puts the textures it sets over the shape's
	// texture set ("textures", by slot, as the NIF stores the paths). Returns null and leaves the
	// texture set as it was when the material can't be resolved, or when it is invalid with that
	// texture set - PBR without its maps, say - which is when the game would ignore it too.
	static std::shared_ptr<const CSMaterial> ResolveForShape(const std::string& shaderName,
															  const Loader& load,
															  std::vector<std::string>& textures,
															  std::string* error = nullptr);

	// Looks for a material as a loose file in the data folders first (with "baseDataPath" in place of
	// the game data folder when not empty), then in the loaded archives.
	static Loader GameDataLoader(const std::string& baseDataPath = {});
};
