/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "McpSnapshots.h"
#include "../components/DiffData.h"
#include "../components/SliderSet.h"
#include "../files/MaterialFile.h"
#include "../program/OutfitStudio.h"
#include "../utils/ConfigurationManager.h"

#include <wx/filename.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <type_traits>
#include <variant>
#include <vector>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace nifly;

namespace {

std::string ToUTF8(const wxString& text) {
	return text.ToUTF8().data();
}

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

std::string GameName(const NiVersion& version) {
	if (version.IsSF())
		return "Starfield";
	if (version.IsFO76())
		return "Fallout 76";
	if (version.IsFO4())
		return "Fallout 4";
	if (version.IsSSE())
		return "Skyrim Special Edition";
	if (version.IsSK())
		return "Skyrim";
	if (version.IsFO3())
		return "Fallout 3";
	if (version.IsOB())
		return "Oblivion";
	return version.GetVersionInfo();
}

// A material path of a shader, found in one of the data folders, with or without the leading "materials"
std::string FindMaterialFile(const std::string& materialPath, const std::vector<std::string>& dataFolders) {
	if (materialPath.empty())
		return {};

	wxString path = wxString::FromUTF8(materialPath);
	path.Replace("\\", "/");
	while (path.StartsWith("/"))
		path.Remove(0, 1);

	for (auto& folder : dataFolders) {
		wxString root = wxString::FromUTF8(McpFilePath(folder));
		for (const wxString& candidate : {root + "/" + path, root + "/materials/" + path}) {
			if (wxFileName::FileExists(candidate))
				return ToUTF8(candidate);
		}
	}
	return {};
}

bool IsBGSM(const MaterialFile& mat) {
	return mat.signature == MaterialFile::BGSM;
}

bool IsBGEM(const MaterialFile& mat) {
	return mat.signature == MaterialFile::BGEM;
}

// A material property as read_material shows it and write_material takes it, with the types and versions of
// the file that store it
using MaterialMember = std::variant<bool MaterialFile::*, float MaterialFile::*, Vector2 MaterialFile::*, Vector3 MaterialFile::*, std::string MaterialFile::*>;

struct MaterialField {
	const char* name;
	MaterialMember member;
	bool (*stored)(const MaterialFile&);
};

const MaterialField MaterialFields[] = {
	{"tileU", &MaterialFile::tileU, [](const MaterialFile&) { return true; }},
	{"tileV", &MaterialFile::tileV, [](const MaterialFile&) { return true; }},
	{"uvOffset", &MaterialFile::uvOffset, [](const MaterialFile&) { return true; }},
	{"uvScale", &MaterialFile::uvScale, [](const MaterialFile&) { return true; }},
	{"alpha", &MaterialFile::alpha, [](const MaterialFile&) { return true; }},
	{"alphaTest", &MaterialFile::alphaTest, [](const MaterialFile&) { return true; }},
	{"zBufferWrite", &MaterialFile::zBufferWrite, [](const MaterialFile&) { return true; }},
	{"zBufferTest", &MaterialFile::zBufferTest, [](const MaterialFile&) { return true; }},
	{"screenSpaceReflections", &MaterialFile::screenSpaceReflections, [](const MaterialFile&) { return true; }},
	{"wetnessControlScreenSpaceReflections", &MaterialFile::wetnessControlScreenSpaceReflections, [](const MaterialFile&) { return true; }},
	{"decal", &MaterialFile::decal, [](const MaterialFile&) { return true; }},
	{"twoSided", &MaterialFile::twoSided, [](const MaterialFile&) { return true; }},
	{"decalNoFade", &MaterialFile::decalNoFade, [](const MaterialFile&) { return true; }},
	{"nonOccluder", &MaterialFile::nonOccluder, [](const MaterialFile&) { return true; }},
	{"refraction", &MaterialFile::refraction, [](const MaterialFile&) { return true; }},
	{"refractionFalloff", &MaterialFile::refractionFalloff, [](const MaterialFile&) { return true; }},
	{"refractionPower", &MaterialFile::refractionPower, [](const MaterialFile&) { return true; }},
	{"environmentMapping", &MaterialFile::environmentMapping, [](const MaterialFile& m) { return m.version < 10 || IsBGEM(m); }},
	{"environmentMappingMaskScale", &MaterialFile::environmentMappingMaskScale, [](const MaterialFile& m) { return m.version < 10 || IsBGEM(m); }},
	{"depthBias", &MaterialFile::depthBias, [](const MaterialFile& m) { return m.version >= 10; }},
	{"grayscaleToPaletteColor", &MaterialFile::grayscaleToPaletteColor, [](const MaterialFile&) { return true; }},

	// BGSM
	{"rootMaterialPath", &MaterialFile::rootMaterialPath, IsBGSM},
	{"enableEditorAlphaRef", &MaterialFile::enableEditorAlphaRef, IsBGSM},
	{"translucency", &MaterialFile::translucency, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"translucencyThickObject", &MaterialFile::translucencyThickObject, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"translucencyMixAlbedoWithSubsurfaceColor", &MaterialFile::translucencyMixAlbedoWithSubsurfaceColor, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"translucencySubsurfaceColor", &MaterialFile::translucencySubsurfaceColor, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"translucencyTransmissiveScale", &MaterialFile::translucencyTransmissiveScale, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"translucencyTurbulence", &MaterialFile::translucencyTurbulence, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 8; }},
	{"rimLighting", &MaterialFile::rimLighting, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"rimPower", &MaterialFile::rimPower, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"backLightPower", &MaterialFile::backLightPower, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"subsurfaceLighting", &MaterialFile::subsurfaceLighting, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"subsurfaceLightingRolloff", &MaterialFile::subsurfaceLightingRolloff, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"specularEnabled", &MaterialFile::specularEnabled, IsBGSM},
	{"specularColor", &MaterialFile::specularColor, IsBGSM},
	{"specularMult", &MaterialFile::specularMult, IsBGSM},
	{"smoothness", &MaterialFile::smoothness, IsBGSM},
	{"fresnelPower", &MaterialFile::fresnelPower, IsBGSM},
	{"wetnessControlSpecScale", &MaterialFile::wetnessControlSpecScale, IsBGSM},
	{"wetnessControlSpecPowerScale", &MaterialFile::wetnessControlSpecPowerScale, IsBGSM},
	{"wetnessControlSpecMinvar", &MaterialFile::wetnessControlSpecMinvar, IsBGSM},
	{"wetnessControlEnvMapScale", &MaterialFile::wetnessControlEnvMapScale, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 10; }},
	{"wetnessControlFresnelPower", &MaterialFile::wetnessControlFresnelPower, IsBGSM},
	{"wetnessControlMetalness", &MaterialFile::wetnessControlMetalness, IsBGSM},
	{"pbr", &MaterialFile::pbr, [](const MaterialFile& m) { return IsBGSM(m) && m.version > 2; }},
	{"customPorosity", &MaterialFile::customPorosity, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 9; }},
	{"porosityValue", &MaterialFile::porosityValue, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 9; }},
	{"anisoLighting", &MaterialFile::anisoLighting, IsBGSM},
	{"emitEnabled", &MaterialFile::emitEnabled, IsBGSM},
	{"emittanceColor", &MaterialFile::emittanceColor, [](const MaterialFile& m) { return IsBGSM(m) ? m.emitEnabled : m.version >= 11; }},
	{"emittanceMult", &MaterialFile::emittanceMult, IsBGSM},
	{"modelSpaceNormals", &MaterialFile::modelSpaceNormals, IsBGSM},
	{"externalEmittance", &MaterialFile::externalEmittance, IsBGSM},
	{"lumEmittance", &MaterialFile::lumEmittance, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 12; }},
	{"useAdaptiveEmissive", &MaterialFile::useAdaptativeEmissive, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 13; }},
	{"adaptiveEmissiveExposureOffset", &MaterialFile::adaptativeEmissive_ExposureOffset, [](const MaterialFile& m) { return m.version >= (IsBGSM(m) ? 13u : 15u); }},
	{"adaptiveEmissiveFinalExposureMin", &MaterialFile::adaptativeEmissive_FinalExposureMin, [](const MaterialFile& m) { return m.version >= (IsBGSM(m) ? 13u : 15u); }},
	{"adaptiveEmissiveFinalExposureMax", &MaterialFile::adaptativeEmissive_FinalExposureMax, [](const MaterialFile& m) { return m.version >= (IsBGSM(m) ? 13u : 15u); }},
	{"backLighting", &MaterialFile::backLighting, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 8; }},
	{"receiveShadows", &MaterialFile::receiveShadows, IsBGSM},
	{"hideSecret", &MaterialFile::hideSecret, IsBGSM},
	{"castShadows", &MaterialFile::castShadows, IsBGSM},
	{"dissolveFade", &MaterialFile::dissolveFade, IsBGSM},
	{"assumeShadowmask", &MaterialFile::assumeShadowmask, IsBGSM},
	{"glowMap", &MaterialFile::glowMap, [](const MaterialFile& m) { return IsBGSM(m) || m.version >= 16; }},
	{"environmentMappingWindow", &MaterialFile::environmentMappingWindow, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 7; }},
	{"environmentMappingEye", &MaterialFile::environmentMappingEye, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 7; }},
	{"hair", &MaterialFile::hair, IsBGSM},
	{"hairTintColor", &MaterialFile::hairTintColor, IsBGSM},
	{"tree", &MaterialFile::tree, IsBGSM},
	{"facegen", &MaterialFile::facegen, IsBGSM},
	{"skinTint", &MaterialFile::skinTint, IsBGSM},
	{"tessellate", &MaterialFile::tessellate, IsBGSM},
	{"displacementTextureBias", &MaterialFile::displacementTextureBias, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 3; }},
	{"displacementTextureScale", &MaterialFile::displacementTextureScale, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 3; }},
	{"tessellationPNScale", &MaterialFile::tessellationPNScale, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 3; }},
	{"tessellationBaseFactor", &MaterialFile::tessellationBaseFactor, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 3; }},
	{"tessellationFadeDistance", &MaterialFile::tessellationFadeDistance, [](const MaterialFile& m) { return IsBGSM(m) && m.version < 3; }},
	{"grayscaleToPaletteScale", &MaterialFile::grayscaleToPaletteScale, IsBGSM},
	{"skewSpecularAlpha", &MaterialFile::skewSpecularAlpha, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 1; }},
	{"terrain", &MaterialFile::terrain, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 3; }},
	{"terrainThresholdFalloff", &MaterialFile::terrainThresholdFalloff, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 3 && m.terrain; }},
	{"terrainTilingDistance", &MaterialFile::terrainTilingDistance, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 3 && m.terrain; }},
	{"terrainRotationAngle", &MaterialFile::terrainRotationAngle, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 3 && m.terrain; }},

	// BGEM
	{"bloodEnabled", &MaterialFile::bloodEnabled, IsBGEM},
	{"effectLightingEnabled", &MaterialFile::effectLightingEnabled, IsBGEM},
	{"falloffEnabled", &MaterialFile::falloffEnabled, IsBGEM},
	{"falloffColorEnabled", &MaterialFile::falloffColorEnabled, IsBGEM},
	{"grayscaleToPaletteAlpha", &MaterialFile::grayscaleToPaletteAlpha, IsBGEM},
	{"softEnabled", &MaterialFile::softEnabled, IsBGEM},
	{"baseColor", &MaterialFile::baseColor, IsBGEM},
	{"baseColorScale", &MaterialFile::baseColorScale, IsBGEM},
	{"falloffStartAngle", &MaterialFile::falloffStartAngle, IsBGEM},
	{"falloffStopAngle", &MaterialFile::falloffStopAngle, IsBGEM},
	{"falloffStartOpacity", &MaterialFile::falloffStartOpacity, IsBGEM},
	{"falloffStopOpacity", &MaterialFile::falloffStopOpacity, IsBGEM},
	{"lightingInfluence", &MaterialFile::lightingInfluence, IsBGEM},
	{"softDepth", &MaterialFile::softDepth, IsBGEM},
	{"effectPbrSpecular", &MaterialFile::effectPbrSpecular, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 20; }},
	{"glassEnabled", &MaterialFile::glassEnabled, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21; }},
	{"glassFresnelColor", &MaterialFile::glassFresnelColor, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21 && m.glassEnabled; }},
	{"glassBlurScaleBase", &MaterialFile::glassBlurScaleBase, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21 && m.glassEnabled; }},
	{"glassBlurScaleFactor", &MaterialFile::glassBlurScaleFactor, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 22 && m.glassEnabled; }},
	{"glassRefractionScaleBase", &MaterialFile::glassRefractionScaleBase, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21 && m.glassEnabled; }},
};

// Texture paths, relative to the textures folder, in the "textures" object
const std::vector<MaterialField> BGSMTextures = {
	{"diffuse", &MaterialFile::diffuseTexture, IsBGSM},
	{"normal", &MaterialFile::normalTexture, IsBGSM},
	{"smoothSpec", &MaterialFile::smoothSpecTexture, IsBGSM},
	{"greyscale", &MaterialFile::greyscaleTexture, IsBGSM},
	{"envmap", &MaterialFile::envmapTexture, [](const MaterialFile& m) { return IsBGSM(m) && m.version <= 2; }},
	{"glow", &MaterialFile::glowTexture, [](const MaterialFile& m) { return IsBGSM(m) || m.version >= 11; }},
	{"innerLayer", &MaterialFile::innerLayerTexture, [](const MaterialFile& m) { return IsBGSM(m) && m.version <= 2; }},
	{"wrinkles", &MaterialFile::wrinklesTexture, IsBGSM},
	{"displacement", &MaterialFile::displacementTexture, [](const MaterialFile& m) { return IsBGSM(m) && m.version <= 2; }},
	{"specular", &MaterialFile::specularTexture, [](const MaterialFile& m) { return m.version > 2 && (IsBGSM(m) || m.version >= 11); }},
	{"lighting", &MaterialFile::lightingTexture, [](const MaterialFile& m) { return m.version > 2 && (IsBGSM(m) || m.version >= 11); }},
	{"flow", &MaterialFile::flowTexture, [](const MaterialFile& m) { return IsBGSM(m) && m.version > 2; }},
	{"distanceFieldAlpha", &MaterialFile::distanceFieldAlphaTexture, [](const MaterialFile& m) { return IsBGSM(m) && m.version >= 17; }},
};

const std::vector<MaterialField> BGEMTextures = {
	{"base", &MaterialFile::baseTexture, IsBGEM},
	{"grayscale", &MaterialFile::grayscaleTexture, IsBGEM},
	{"envmap", &MaterialFile::fxEnvmapTexture, IsBGEM},
	{"normal", &MaterialFile::fxNormalTexture, IsBGEM},
	{"envmapMask", &MaterialFile::envmapMaskTexture, IsBGEM},
	{"glassRoughnessScratch", &MaterialFile::glassRoughnessScratchTexture, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21; }},
	{"glassDirtOverlay", &MaterialFile::glassDirtOverlayTexture, [](const MaterialFile& m) { return IsBGEM(m) && m.version >= 21; }},
};

const std::vector<MaterialField>& MaterialTextures(const MaterialFile& mat) {
	return IsBGEM(mat) ? BGEMTextures : BGSMTextures;
}

const char* const AlphaBlendModeNames[] = {"unknown", "none", "standard", "additive", "multiplicative"};

json MaterialValue(const MaterialFile& mat, const MaterialMember& member) {
	return std::visit(
		[&mat](auto ptr) -> json {
			const auto& value = mat.*ptr;
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, Vector2>)
				return {value.u, value.v};
			else if constexpr (std::is_same_v<T, Vector3>)
				return Vec(value);
			else
				return value;
		},
		member);
}

void SetMaterialValue(MaterialFile& mat, const MaterialMember& member, const std::string& name, const json& value) {
	std::visit(
		[&](auto ptr) {
			auto& target = mat.*ptr;
			using T = std::decay_t<decltype(target)>;
			auto numbers = [&](size_t count) {
				if (!value.is_array() || value.size() != count)
					throw McpToolError("The material property '" + name + "' has to be a list of " + std::to_string(count) + " numbers.");
				std::vector<float> list;
				for (auto& item : value) {
					if (!item.is_number())
						throw McpToolError("The material property '" + name + "' has to be a list of " + std::to_string(count) + " numbers.");
					list.push_back(item.get<float>());
				}
				return list;
			};

			if constexpr (std::is_same_v<T, Vector2>) {
				auto list = numbers(2);
				target = Vector2(list[0], list[1]);
			}
			else if constexpr (std::is_same_v<T, Vector3>) {
				auto list = numbers(3);
				target = Vector3(list[0], list[1], list[2]);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				if (!value.is_boolean())
					throw McpToolError("The material property '" + name + "' has to be true or false.");
				target = value.get<bool>();
			}
			else if constexpr (std::is_same_v<T, float>) {
				if (!value.is_number())
					throw McpToolError("The material property '" + name + "' has to be a number.");
				target = value.get<float>();
			}
			else {
				if (!value.is_string())
					throw McpToolError("The material property '" + name + "' has to be text.");
				target = value.get<std::string>();
			}
		},
		member);
}

// All properties the file stores, for its type and version
json MaterialJson(const MaterialFile& mat) {
	json data = {
		{"type", IsBGEM(mat) ? "BGEM" : "BGSM"},
		{"version", mat.version},
		{"alphaBlendMode", AlphaBlendModeNames[std::clamp(static_cast<int>(mat.alphaBlendMode), 0, 4)]},
		{"alphaTestRef", static_cast<int>(static_cast<uint8_t>(mat.alphaTestRef))},
	};
	if (mat.version >= 6)
		data["maskWrites"] = static_cast<int>(mat.maskWrites);
	if (IsBGEM(mat))
		data["envmapMinLOD"] = static_cast<int>(mat.envmapMinLOD);

	for (auto& field : MaterialFields)
		if (field.stored(mat))
			data[field.name] = MaterialValue(mat, field.member);

	json textures = json::object();
	for (auto& field : MaterialTextures(mat))
		if (field.stored(mat))
			textures[field.name] = MaterialValue(mat, field.member);
	data["textures"] = textures;
	return data;
}

int ByteValue(const json& value, const std::string& name, int max) {
	if (!value.is_number_integer() || value.get<int>() < 0 || value.get<int>() > max)
		throw McpToolError("The material property '" + name + "' has to be a whole number from 0 to " + std::to_string(max) + ".");
	return value.get<int>();
}

// Sets properties named as MaterialJson names them. Properties the file's type and version don't store are
// noted and left out.
void SetMaterialProperties(MaterialFile& mat, const json& properties, json& notes) {
	if (!properties.is_object())
		McpArgs::Wrong("properties", "an object of material property names to values");

	std::vector<std::string> given;
	std::vector<std::string> givenTextures;
	for (auto& item : properties.items()) {
		const std::string& name = item.key();
		const json& value = item.value();

		// The fields read_material returns that aren't properties, so its result can be passed back
		if (name == "type" || name == "version" || name == "file")
			continue;

		if (name == "textures") {
			if (!value.is_object())
				throw McpToolError("The material property 'textures' has to be an object of texture names to paths.");
			for (auto& texture : value.items()) {
				auto& fields = MaterialTextures(mat);
				auto field = std::find_if(fields.begin(), fields.end(), [&](const MaterialField& f) { return texture.key() == f.name; });
				if (field == fields.end())
					throw McpToolError("A " + std::string(IsBGEM(mat) ? "BGEM" : "BGSM") + " has no texture '" + texture.key() + "'.");
				SetMaterialValue(mat, field->member, "textures." + texture.key(), texture.value());
				givenTextures.push_back(texture.key());
			}
			continue;
		}

		if (name == "alphaBlendMode") {
			if (value.is_string()) {
				auto it = std::find(std::begin(AlphaBlendModeNames), std::end(AlphaBlendModeNames), value.get<std::string>());
				if (it == std::end(AlphaBlendModeNames))
					throw McpToolError("The material property 'alphaBlendMode' has to be one of " + json(AlphaBlendModeNames).dump() + ".");
				mat.alphaBlendMode = static_cast<MaterialFile::AlphaBlendModeType>(it - std::begin(AlphaBlendModeNames));
			}
			else
				mat.alphaBlendMode = static_cast<MaterialFile::AlphaBlendModeType>(ByteValue(value, name, 4));
			continue;
		}
		if (name == "alphaTestRef") {
			mat.alphaTestRef = static_cast<char>(ByteValue(value, name, 255));
			continue;
		}
		if (name == "maskWrites") {
			mat.maskWrites = static_cast<MaterialFile::MaskWriteFlags>(ByteValue(value, name, 63));
			given.push_back(name);
			continue;
		}
		if (name == "envmapMinLOD") {
			mat.envmapMinLOD = static_cast<uint8_t>(ByteValue(value, name, 255));
			given.push_back(name);
			continue;
		}

		auto field = std::find_if(std::begin(MaterialFields), std::end(MaterialFields), [&](const MaterialField& f) { return name == f.name; });
		if (field == std::end(MaterialFields))
			throw McpToolError("There is no material property '" + name + "'. read_material shows the properties of a file.");
		SetMaterialValue(mat, field->member, name, value);
		given.push_back(name);
	}

	// Checked after all are set, since some are stored only when another is on (emitEnabled, terrain, glassEnabled)
	const std::string kind = std::string(IsBGEM(mat) ? "BGEM" : "BGSM") + " of version " + std::to_string(mat.version);
	for (auto& name : given) {
		bool stored = name == "maskWrites" ? mat.version >= 6 : name == "envmapMinLOD" ? IsBGEM(mat) : false;
		for (auto& field : MaterialFields)
			if (name == field.name)
				stored = field.stored(mat);
		if (!stored)
			notes.push_back("'" + name + "' isn't stored in a " + kind + " and was left out.");
	}
	for (auto& name : givenTextures) {
		bool stored = false;
		for (auto& field : MaterialTextures(mat))
			if (name == field.name && field.stored(mat))
				stored = true;
		if (!stored)
			notes.push_back("Texture '" + name + "' isn't stored in a " + kind + " and was left out.");
	}
}

// Specular power (Skyrim glossiness) for a Fallout 4 smoothness. Fallout 4 scales the gloss map with the smoothness,
// Skyrim has one specular power for the whole shape: 0.5 gives 22, 0.7 gives 59, 1 gives 256.
float GlossinessFromSmoothness(float smoothness) {
	return std::clamp(std::exp2(smoothness * 7.0f + 1.0f), 2.0f, 256.0f);
}

// Sets up a Skyrim lighting shader from a Fallout 4 material. Returns what was taken over and what has no place in
// the target shader.
json ApplyMaterial(NifFile& nif, NiShape* shape, MaterialFile& mat) {
	json applied = json::object();
	json notes = json::array();

	auto* shader = dynamic_cast<BSLightingShaderProperty*>(nif.GetShader(shape));
	if (!shader) {
		notes.push_back("The shape has no lighting shader, nothing was set.");
		return {{"notes", notes}};
	}

	auto setTexture = [&](const std::string& path, uint32_t slot) {
		std::string texture = path;
		if (texture.empty())
			return;
		// Material texture paths are relative to the textures folder
		wxString lower = wxString::FromUTF8(texture).Lower();
		if (!lower.StartsWith("textures\\") && !lower.StartsWith("textures/"))
			texture = "textures\\" + texture;
		nif.SetTextureSlot(shape, texture, slot);
		applied["textures"][std::to_string(slot)] = texture;
	};

	if (mat.signature == MaterialFile::BGEM) {
		setTexture(mat.baseTexture, 0);
		setTexture(mat.fxNormalTexture, 1);
		notes.push_back("BGEM effect materials need a BSEffectShaderProperty in the target game; only the textures were set on the lighting shader.");
		return {{"applied", applied}, {"notes", notes}};
	}

	setTexture(mat.diffuseTexture, 0);
	setTexture(mat.normalTexture, 1);

	uint32_t type = BSLSP_DEFAULT;
	if (mat.glowMap && !mat.glowTexture.empty()) {
		setTexture(mat.glowTexture, 2);
		type = BSLSP_GLOWMAP;
	}
	else if (mat.skinTint)
		type = BSLSP_SKINTINT;
	else if (mat.hair)
		type = BSLSP_HAIRTINT;
	shader->SetShaderType(type);
	applied["shaderType"] = type;

	uint32_t& f1 = shader->shaderFlags1;
	uint32_t& f2 = shader->shaderFlags2;
	auto setFlag = [](uint32_t& flags, uint32_t flag, bool on) {
		flags = on ? (flags | flag) : (flags & ~flag);
	};

	setFlag(f1, SLSF1_SKINNED, shape->IsSkinned());
	setFlag(f1, SLSF1_SPECULAR, mat.specularEnabled);
	setFlag(f1, SLSF1_MODEL_SPACE_NORMALS, mat.modelSpaceNormals);
	// Fallout 4 materials leave shadows to the renderer, Skyrim armor casts and receives them
	setFlag(f1, SLSF1_RECEIVE_SHADOWS, true);
	setFlag(f1, SLSF1_CAST_SHADOWS, true);
	setFlag(f1, SLSF1_DECAL, mat.decal);
	setFlag(f1, SLSF1_ZBUFFER_TEST, mat.zBufferTest);
	setFlag(f1, SLSF1_OWN_EMIT, mat.emitEnabled || mat.glowMap);
	setFlag(f1, SLSF1_ENVIRONMENT_MAPPING, false);
	setFlag(f2, SLSF2_ZBUFFER_WRITE, mat.zBufferWrite);
	setFlag(f2, SLSF2_DOUBLE_SIDED, mat.twoSided);
	setFlag(f2, SLSF2_GLOW_MAP, type == BSLSP_GLOWMAP);
	setFlag(f2, SLSF2_RIM_LIGHTING, mat.rimLighting);
	setFlag(f2, SLSF2_BACK_LIGHTING, mat.backLighting);
	setFlag(f2, SLSF2_SOFT_LIGHTING, mat.subsurfaceLighting);
	setFlag(f2, SLSF2_VERTEX_COLORS, shape->HasVertexColors());

	shader->specularColor = mat.specularColor;
	shader->specularStrength = mat.specularMult;
	shader->glossiness = GlossinessFromSmoothness(mat.smoothness);
	shader->alpha = mat.alpha;
	shader->uvOffset = mat.uvOffset;
	shader->uvScale = mat.uvScale;
	shader->textureClampMode = static_cast<TexClampMode>((mat.tileU ? 2 : 0) | (mat.tileV ? 1 : 0));
	if (mat.emitEnabled) {
		shader->emissiveColor = mat.emittanceColor;
		shader->emissiveMultiple = mat.emittanceMult;
	}
	else {
		shader->emissiveColor = Vector3();
		shader->emissiveMultiple = 1.0f;
	}
	if (mat.rimLighting)
		shader->rimlightPower = mat.rimPower;
	if (mat.backLighting)
		shader->backlightPower = mat.backLightPower;
	if (mat.subsurfaceLighting)
		shader->softlighting = mat.subsurfaceLightingRolloff;

	applied["specular"] = mat.specularEnabled;
	applied["specularColor"] = Vec(shader->specularColor);
	applied["specularStrength"] = shader->specularStrength;
	applied["glossiness"] = shader->glossiness;
	applied["doubleSided"] = mat.twoSided;
	applied["alpha"] = shader->alpha;

	// Alpha blending and testing go into an alpha property, which Fallout 4 keeps in the material
	const bool blend = mat.alphaBlendMode != MaterialFile::AlphaBlendModeType::None && mat.alphaBlendMode != MaterialFile::AlphaBlendModeType::Unknown;
	if (blend || mat.alphaTest) {
		uint8_t mode0 = 0;
		uint32_t src = 6, dst = 7;
		if (blend)
			mat.ConvertAlphaBlendMode(mat.alphaBlendMode, mode0, src, dst);

		auto alphaProp = std::make_unique<NiAlphaProperty>();
		alphaProp->flags = static_cast<uint16_t>((blend ? 1 : 0) | ((src & 0xF) << 1) | ((dst & 0xF) << 5) | (mat.alphaTest ? (1 << 9) : 0) | (4 << 10));
		alphaProp->threshold = static_cast<uint8_t>(mat.alphaTestRef);
		applied["alphaProperty"] = {{"flags", alphaProp->flags}, {"threshold", alphaProp->threshold}};
		nif.AssignAlphaProperty(shape, std::move(alphaProp));
	}
	else
		nif.RemoveAlphaProperty(shape);

	const std::string smoothSpec = mat.smoothSpecTexture;
	if (!smoothSpec.empty())
		notes.push_back("Smoothness/specular map '" + smoothSpec
						+ "' has no slot in the Skyrim shader: Skyrim reads specular from the alpha of the normal map, so "
						  "build a normal map with the specular map's red channel as alpha. Glossiness came from the smoothness "
						  "alone, which Fallout 4 multiplies with the gloss map (green channel): lower SpecularPower when "
						  "that map is darker.");
	if (mat.environmentMapping || !mat.envmapTexture.empty())
		notes.push_back("Environment mapping wasn't set up: Skyrim needs its own cube map (slot 4) and an environment mask (slot 5).");
	if (!mat.greyscaleTexture.empty() || mat.grayscaleToPaletteColor)
		notes.push_back("Greyscale-to-palette coloring has no Skyrim equivalent.");
	if (mat.pbr)
		notes.push_back("The material is PBR; Skyrim's lighting shader isn't.");

	return {{"applied", applied}, {"notes", notes}};
}

// Smoothness of a Fallout 4 material for a Skyrim specular power, the inverse of GlossinessFromSmoothness
float SmoothnessFromGlossiness(float glossiness) {
	return std::clamp((std::log2(std::max(glossiness, 1.0f)) - 1.0f) / 7.0f, 0.0f, 1.0f);
}

// A NIF texture path as a material names it: relative to the textures folder
std::string MaterialTexturePath(const std::string& path) {
	wxString texture = wxString::FromUTF8(path);
	texture.Replace("\\", "/");
	if (texture.Lower().StartsWith("textures/"))
		texture.Remove(0, 9);
	return ToUTF8(texture);
}

// Alpha blending and testing of the shape's alpha property, which materials keep themselves
void MaterialAlphaFromShape(NifFile& nif, NiShape* shape, MaterialFile& mat, json& notes) {
	mat.alphaBlendMode = MaterialFile::AlphaBlendModeType::Unknown;
	mat.alphaTest = false;

	NiAlphaProperty* alphaProp = nif.GetAlphaProperty(shape);
	if (!alphaProp)
		return;

	mat.alphaTest = (alphaProp->flags & (1 << 9)) != 0;
	mat.alphaTestRef = static_cast<char>(alphaProp->threshold);
	if (alphaProp->flags & 1) {
		mat.alphaBlendMode = mat.ConvertAlphaBlendMode(1, (alphaProp->flags >> 1) & 0xF, (alphaProp->flags >> 5) & 0xF);
		if (mat.alphaBlendMode == MaterialFile::AlphaBlendModeType::None) {
			mat.alphaBlendMode = MaterialFile::AlphaBlendModeType::Standard;
			notes.push_back("The alpha property's blend functions have no material blend mode, standard blending was used.");
		}
	}
}

// A material from the shader of a Skyrim or Fallout 4 shape: a BGSM from a lighting shader, a BGEM from an effect
// shader. Fallout 4 NIFs keep a copy of their material's values in the shader.
MaterialFile MaterialFromShader(NifFile& nif, NiShape* shape, json& notes) {
	const NiVersion& version = nif.GetHeader().GetVersion();
	if (version.IsSF() || version.IsFO76())
		throw McpToolError("The shader of a " + GameName(version) + " shape doesn't have the material's values. Start from its material file with 'base'.");

	NiShader* shader = nif.GetShader(shape);
	if (!shader)
		throw McpToolError("The shape '" + shape->name.get() + "' has no shader.");

	const bool fo4 = version.IsFO4();
	auto texture = [&](uint32_t slot) {
		std::string path;
		nif.GetTextureSlot(shape, path, slot);
		return MaterialTexturePath(path);
	};

	if (auto* effect = dynamic_cast<BSEffectShaderProperty*>(shader)) {
		MaterialFile mat(MaterialFile::BGEM);
		const uint32_t f1 = effect->shaderFlags1;
		const uint32_t f2 = effect->shaderFlags2;

		mat.baseTexture = MaterialTexturePath(effect->sourceTexture.get());
		mat.grayscaleTexture = MaterialTexturePath(effect->greyscaleTexture.get());
		mat.fxEnvmapTexture = MaterialTexturePath(effect->envMapTexture.get());
		mat.fxNormalTexture = MaterialTexturePath(effect->normalTexture.get());
		mat.envmapMaskTexture = MaterialTexturePath(effect->envMaskTexture.get());

		mat.baseColor = Vector3(effect->baseColor.r, effect->baseColor.g, effect->baseColor.b);
		mat.alpha = effect->baseColor.a;
		mat.baseColorScale = effect->baseColorScale;
		mat.falloffStartAngle = effect->falloffStartAngle;
		mat.falloffStopAngle = effect->falloffStopAngle;
		mat.falloffStartOpacity = effect->falloffStartOpacity;
		mat.falloffStopOpacity = effect->falloffStopOpacity;
		mat.softDepth = effect->softFalloffDepth;
		mat.environmentMappingMaskScale = effect->envMapScale;
		mat.emittanceColor = Vector3(effect->emittanceColor.r, effect->emittanceColor.g, effect->emittanceColor.b);
		mat.uvOffset = effect->uvOffset;
		mat.uvScale = effect->uvScale;
		mat.tileU = (effect->textureClampMode & 2) != 0;
		mat.tileV = (effect->textureClampMode & 1) != 0;

		mat.grayscaleToPaletteColor = (f1 & SLSF1_GREYSCALETOPALETTE_COLOR) != 0;
		mat.grayscaleToPaletteAlpha = (f1 & SLSF1_GREYSCALETOPALETTE_ALPHA) != 0;
		mat.falloffEnabled = (f1 & SLSF1_USE_FALLOFF) != 0;
		mat.falloffColorEnabled = fo4 && (f1 & F4SF1_RGB_FALLOFF) != 0;
		mat.environmentMapping = (f1 & SLSF1_ENVIRONMENT_MAPPING) != 0;
		mat.decal = (f1 & SLSF1_DECAL) != 0;
		mat.softEnabled = (f1 & SLSF1_SOFT_EFFECT) != 0;
		mat.zBufferTest = (f1 & SLSF1_ZBUFFER_TEST) != 0;
		mat.zBufferWrite = (f2 & SLSF2_ZBUFFER_WRITE) != 0;
		mat.twoSided = (f2 & SLSF2_DOUBLE_SIDED) != 0;
		mat.bloodEnabled = (f2 & SLSF2_WEAPON_BLODD) != 0;
		mat.effectLightingEnabled = (f2 & SLSF2_EFFECT_LIGHTING) != 0;

		MaterialAlphaFromShape(nif, shape, mat, notes);
		return mat;
	}

	auto* lighting = dynamic_cast<BSLightingShaderProperty*>(shader);
	if (!lighting)
		throw McpToolError("The shape '" + shape->name.get() + "' has a " + shader->GetBlockName() + ", which no material is made from.");

	MaterialFile mat(MaterialFile::BGSM);
	const uint32_t f1 = lighting->shaderFlags1;
	const uint32_t f2 = lighting->shaderFlags2;
	const uint32_t type = lighting->GetShaderType();

	mat.diffuseTexture = texture(0);
	mat.normalTexture = texture(1);

	mat.specularEnabled = (f1 & SLSF1_SPECULAR) != 0;
	mat.specularColor = lighting->specularColor;
	mat.specularMult = lighting->specularStrength;
	mat.modelSpaceNormals = (f1 & SLSF1_MODEL_SPACE_NORMALS) != 0;
	mat.castShadows = (f1 & SLSF1_CAST_SHADOWS) != 0;
	mat.decal = (f1 & SLSF1_DECAL) != 0;
	mat.zBufferTest = (f1 & SLSF1_ZBUFFER_TEST) != 0;
	mat.environmentMapping = (f1 & SLSF1_ENVIRONMENT_MAPPING) != 0;
	mat.environmentMappingMaskScale = lighting->environmentMapScale;
	mat.zBufferWrite = (f2 & SLSF2_ZBUFFER_WRITE) != 0;
	mat.twoSided = (f2 & SLSF2_DOUBLE_SIDED) != 0;
	mat.glowMap = (f2 & SLSF2_GLOW_MAP) != 0 && (fo4 || type == BSLSP_GLOWMAP);
	mat.anisoLighting = (f2 & SLSF2_ANISOTROPIC_LIGHTING) != 0;
	mat.tree = (f2 & SLSF2_TREE_ANIM) != 0;
	mat.hairTintColor = lighting->hairTintColor;
	mat.alpha = lighting->alpha;
	mat.uvOffset = lighting->uvOffset;
	mat.uvScale = lighting->uvScale;
	mat.tileU = (lighting->textureClampMode & 2) != 0;
	mat.tileV = (lighting->textureClampMode & 1) != 0;

	const Color4 emissive = lighting->GetEmissiveColor();
	mat.emitEnabled = (f1 & SLSF1_OWN_EMIT) != 0 && (emissive.r > 0.0f || emissive.g > 0.0f || emissive.b > 0.0f);
	mat.emittanceColor = Vector3(emissive.r, emissive.g, emissive.b);
	mat.emittanceMult = lighting->GetEmissiveMultiple();

	// Both games' texture sets have glow in slot 2 and the environment cube map in slot 4
	if (mat.glowMap)
		mat.glowTexture = texture(2);
	if (mat.environmentMapping)
		mat.envmapTexture = texture(4);

	if (fo4) {
		// Fallout 4 NIFs keep the material's smoothness where Skyrim has the specular power
		mat.greyscaleTexture = texture(3);
		mat.smoothSpecTexture = texture(7);
		mat.smoothness = lighting->glossiness;
		mat.receiveShadows = true;
		mat.hair = (f1 & F4SF1_HAIR) != 0;
		mat.skinTint = (f1 & F4SF1_SKIN_TINT) != 0;
		mat.grayscaleToPaletteColor = (f1 & F4SF1_GREYSCALETOPALETTE_COLOR) != 0;
		mat.externalEmittance = (f1 & F4SF1_EXTERNAL_EMITTANCE) != 0;
		mat.skewSpecularAlpha = (f2 & F4SF2_SKEW_SPECULAR_ALPHA) != 0;
		mat.subsurfaceLightingRolloff = lighting->subsurfaceRolloff;
		mat.backLightPower = lighting->backlightPower;
		mat.grayscaleToPaletteScale = lighting->grayscaleToPaletteScale;
		mat.fresnelPower = lighting->fresnelPower;
		mat.wetnessControlSpecScale = lighting->wetnessSpecScale;
		mat.wetnessControlSpecPowerScale = lighting->wetnessSpecPower;
		mat.wetnessControlSpecMinvar = lighting->wetnessMinVar;
		mat.wetnessControlEnvMapScale = lighting->wetnessEnvmapScale;
		mat.wetnessControlFresnelPower = lighting->wetnessFresnelPower;
		mat.wetnessControlMetalness = lighting->wetnessMetalness;
		notes.push_back("Built from the Fallout 4 shader's copy of the material values and the NIF's texture paths. The game reads "
						"the material instead, so NIFs often have outdated values there (two-sided, tints, wetness): when the "
						"original material exists, start from it with 'base' and the dataFolders that have it.");
	}
	else {
		mat.smoothness = SmoothnessFromGlossiness(lighting->glossiness);
		mat.receiveShadows = (f1 & SLSF1_RECEIVE_SHADOWS) != 0;
		mat.hair = type == BSLSP_HAIRTINT;
		mat.skinTint = type == BSLSP_SKINTINT;
		mat.rimLighting = (f2 & SLSF2_RIM_LIGHTING) != 0;
		mat.rimPower = lighting->rimlightPower;
		mat.backLighting = (f2 & SLSF2_BACK_LIGHTING) != 0;
		mat.backLightPower = lighting->backlightPower;
		mat.subsurfaceLighting = (f2 & SLSF2_SOFT_LIGHTING) != 0;
		mat.subsurfaceLightingRolloff = lighting->softlighting;

		notes.push_back("Skyrim keeps specular in the normal map's alpha. Make a smoothness/specular map (_s: red is specular, "
						"e.g. from the normal map's alpha, green is smoothness) and set textures.smoothSpec; without one the "
						"shape has no specular. The normal map can lose its alpha (Fallout 4 uses BC5).");
		notes.push_back(ToUTF8(wxString::Format("smoothness %.2f came from SpecularPower %g. Fallout 4 multiplies it with the _s map's "
												"green, so raise it when that map is dark.",
												mat.smoothness,
												lighting->glossiness)));
		if (mat.environmentMapping)
			notes.push_back("Fallout 4 masks environment mapping with the _s map, not Skyrim's environment mask (slot 5).");
		notes.push_back("Texture paths were kept: copy the textures to the same paths under the Fallout 4 Data/Textures folder.");
	}

	MaterialAlphaFromShape(nif, shape, mat, notes);
	return mat;
}

// The path a shader names a material file by: from its Materials folder on, as the Shape Properties dialog sets it.
// Empty when the file isn't in a Materials folder.
std::string ShaderMaterialPath(const std::string& file) {
	std::string path = file;
	std::replace(path.begin(), path.end(), '/', '\\');
	std::string lower = path;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
	const size_t index = lower.rfind("\\materials\\");
	if (index == std::string::npos)
		return {};
	return path.substr(index + 1);
}

// A material file, or a material path as a shader names it found in dataFolders
std::string MaterialFileArg(const json& args, const char* name) {
	std::string path = McpArgs::String(args, name);
	std::string file = McpFilePath(path);
	if (!wxFileName::FileExists(wxString::FromUTF8(file)))
		file = FindMaterialFile(path, McpArgs::Strings(args, "dataFolders"));
	if (file.empty())
		throw McpToolError("There is no material file '" + path + "'.");
	return file;
}

McpToolResult WriteMaterial(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NifFile& nif = *project.GetWorkNif();
	const NiVersion& gameVersion = nif.GetHeader().GetVersion();

	const std::string file = McpFilePath(McpArgs::String(args, "file"));
	const wxString ext = wxFileName(wxString::FromUTF8(file)).GetExt().Lower();
	if (ext != "bgsm" && ext != "bgem")
		throw McpToolError("The file has to end in .bgsm (lighting material) or .bgem (effect material).");
	McpCheckOverwrite(file, args);

	if (McpArgs::Has(args, "base") && McpArgs::Has(args, "shape"))
		throw McpToolError("Pass either 'base' or 'shape' to start from, not both.");

	json notes = json::array();
	json source;
	MaterialFile mat(ext == "bgem" ? MaterialFile::BGEM : MaterialFile::BGSM);
	bool versionFromSource = false;

	if (McpArgs::Has(args, "base")) {
		std::string baseFile = MaterialFileArg(args, "base");
		mat = MaterialFile(baseFile);
		if (mat.Failed())
			throw McpToolError("'" + baseFile + "' couldn't be read as a BGSM or BGEM file.");
		source = {{"base", baseFile}};
		versionFromSource = true;
	}
	else if (McpArgs::Has(args, "shape")) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		if (project.GetShapeMaterialFile(shape, mat)) {
			source = {{"shape", shape->name.get()}, {"from", "material file"}, {"materialPath", nif.GetShader(shape)->name.get()}};
			versionFromSource = true;
		}
		else {
			mat = MaterialFromShader(nif, shape, notes);
			source = {{"shape", shape->name.get()}, {"from", "shader"}};
		}
	}

	const MaterialFile::Type type = ext == "bgem" ? MaterialFile::BGEM : MaterialFile::BGSM;
	if (mat.signature != type)
		throw McpToolError(std::string("The material to start from is a ") + (IsBGEM(mat) ? "BGEM" : "BGSM") + ", the file has to end in ." + (IsBGEM(mat) ? "bgem" : "bgsm") + ".");

	// Fallout 4 writes version 2, Fallout 76 20 to 22
	if (McpArgs::Has(args, "version")) {
		int version = McpArgs::Int(args, "version", 2);
		if (version < 1 || version > 22)
			throw McpToolError("Argument 'version' has to be from 1 to 22: 2 for Fallout 4, 20 to 22 for Fallout 76.");
		mat.version = static_cast<uint32_t>(version);
	}
	else if (!versionFromSource)
		mat.version = gameVersion.IsFO76() ? 22 : 2;

	if (const json* properties = McpArgs::Find(args, "properties"))
		SetMaterialProperties(mat, *properties, notes);

	// Checked before anything is written
	std::vector<NiShape*> assign;
	if (McpArgs::Has(args, "assignToShapes"))
		assign = mcp.Shapes(args, "assignToShapes", true);
	const std::string materialPath = ShaderMaterialPath(file);
	if (!assign.empty()) {
		if (!gameVersion.IsFO4() && !gameVersion.IsFO76())
			throw McpToolError("Only Fallout 4 and 76 shapes use material files; this project is " + GameName(gameVersion) + ".");
		if (materialPath.empty())
			throw McpToolError("Shapes name their material from the Materials folder on; write the file into a Materials folder (e.g. Data/Materials/MyOutfit/a.bgsm) to assign it.");
		for (NiShape* shape : assign) {
			NiShader* shader = nif.GetShader(shape);
			const bool effect = dynamic_cast<BSEffectShaderProperty*>(shader) != nullptr;
			if (!shader || (!effect && !dynamic_cast<BSLightingShaderProperty*>(shader)))
				throw McpToolError("The shape '" + shape->name.get() + "' has no lighting or effect shader to name the material.");
			if (effect != IsBGEM(mat))
				throw McpToolError("The shape '" + shape->name.get() + "' has " + (effect ? "an effect shader, which takes a BGEM." : "a lighting shader, which takes a BGSM."));
		}
	}

	wxFileName::Mkdir(wxFileName(wxString::FromUTF8(file)).GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	if (mat.Write(file) != 0)
		throw McpToolError("'" + file + "' couldn't be written.");

	// Read back, so the result shows what the file holds
	MaterialFile written(file);
	if (written.Failed())
		throw McpToolError("'" + file + "' was written but couldn't be read back.");

	McpToolResult result;
	json assigned = json::array();
	if (!assign.empty()) {
		// Only assigning changes the project, so the snapshot isn't taken for every call
		result.data["autoSnapshot"] = mcp.Snapshots().SaveAutomatic(mcp, "write_material");

		OutfitStudioFrame* frame = mcp.GetFrame();
		for (NiShape* shape : assign) {
			nif.GetShader(shape)->name.get() = materialPath;
			project.SetTextures(shape);
			frame->MeshFromProj(shape, true);
			assigned.push_back(shape->name.get());

			MaterialFile loaded;
			if (!project.GetShapeMaterialFile(shape, loaded))
				notes.push_back("The window shows '" + shape->name.get() + "' with its material once the file is in the game's Data folder (" + Config["GameDataPath"] + ").");
		}
		frame->SetPendingChanges();
		frame->glView->Render();
	}

	result.data["file"] = file;
	if (!materialPath.empty())
		result.data["materialPath"] = materialPath;
	if (!source.is_null())
		result.data["source"] = source;
	result.data["material"] = MaterialJson(written);
	if (!assigned.empty())
		result.data["assignedTo"] = assigned;
	if (!notes.empty())
		result.data["notes"] = notes;
	return result;
}

struct ForeignSet {
	SliderSet set;
	std::string nifPath;
	std::string shapeDataPath;
};

// A set of a slider set file of any BodySlide folder, with its NIF and the ShapeData folder its data comes from
ForeignSet LoadForeignSet(const std::string& file, const std::string& setName) {
	std::string path = McpFilePath(file);
	SliderSetFile setFile(path);
	if (setFile.fail())
		throw McpToolError("'" + path + "' couldn't be read as a slider set file.");

	std::vector<std::string> names;
	setFile.GetSetNames(names);
	std::string name = setName;
	if (name.empty()) {
		if (names.size() != 1)
			throw McpToolError("The file has " + std::to_string(names.size()) + " sets, pass one as 'set': " + json(names).dump());
		name = names.front();
	}

	ForeignSet result;
	if (setFile.GetSet(name, result.set))
		throw McpToolError("The file has no set '" + name + "'. It has: " + json(names).dump());

	// SliderSets and ShapeData are next to each other in a BodySlide folder
	wxFileName setPath(wxString::FromUTF8(path));
	setPath.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	wxFileName shapeData(setPath.GetPath(), "");
	shapeData.RemoveLastDir();
	shapeData.AppendDir("ShapeData");
	result.shapeDataPath = ToUTF8(shapeData.GetPath());
	result.set.SetBaseDataPath(result.shapeDataPath);
	result.nifPath = result.shapeDataPath + "/" + result.set.GetDefaultDataFolder() + "/" + result.set.GetInputFile();
	return result;
}

McpToolResult ImportFromGame(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();

	std::string file = McpFilePath(McpArgs::String(args, "file"));
	const bool isSet = wxFileName(wxString::FromUTF8(file)).GetExt().Lower() == "osp";

	std::unique_ptr<ForeignSet> foreign;
	std::string nifPath = file;
	if (isSet) {
		foreign = std::make_unique<ForeignSet>(LoadForeignSet(file, McpArgs::String(args, "set", "")));
		nifPath = foreign->nifPath;
	}

	NifFile nif;
	if (nif.Load(nifPath) != 0)
		throw McpToolError("'" + nifPath + "' couldn't be loaded as a NIF file.");

	std::vector<std::string> wanted = McpArgs::Strings(args, "shapes");
	const std::vector<std::string> skipped = McpArgs::Strings(args, "skipShapes");
	const std::string sliderMode = McpArgs::String(args, "sliders", "local");
	if (sliderMode != "local" && sliderMode != "all" && sliderMode != "zaps" && sliderMode != "none")
		McpArgs::Wrong("sliders", "\"local\", \"all\", \"zaps\" or \"none\"");

	std::vector<std::string> dataFolders = McpArgs::Strings(args, "dataFolders");

	std::vector<NiShape*> sourceShapes;
	for (NiShape* shape : nif.GetShapes()) {
		std::string name = shape->name.get();
		if (!wanted.empty() && std::find(wanted.begin(), wanted.end(), name) == wanted.end())
			continue;
		if (std::find(skipped.begin(), skipped.end(), name) != skipped.end())
			continue;
		sourceShapes.push_back(shape);
	}
	if (sourceShapes.empty())
		throw McpToolError("None of the shapes of '" + nifPath + "' were picked. It has: " + json(nif.GetShapeNames()).dump());

	// Slider data of the set, in the vertex order of its shapes
	DiffDataSets sourceDiffs;
	if (foreign && sliderMode != "none")
		foreign->set.LoadSetDiffData(sourceDiffs);

	if (!project.GetWorkAnim()->GetRefNif())
		project.GetWorkAnim()->SetRefNif(project.GetWorkNif());

	json shapesOut = json::array();
	json slidersAdded = json::array();
	std::set<std::string> sliderNames;

	for (NiShape* source : sourceShapes) {
		const std::string sourceName = source->name.get();
		std::string newName = sourceName;
		for (int n = 2; project.IsValidShape(newName); n++)
			newName = sourceName + "_" + std::to_string(n);

		std::vector<Vector3> verts;
		std::vector<Vector3> normals;
		std::vector<Vector2> uvs;
		std::vector<Color4> colors;
		std::vector<Triangle> tris;
		nif.GetVertsForShape(source, verts);
		nif.GetUvsForShape(source, uvs);
		nif.GetColorsForShape(source, colors);
		if (auto* n = nif.GetNormalsForShape(source))
			normals = *n;
		source->GetTriangles(tris);

		// Positions in global space: skinned shapes are stored in their skin space, others under their nodes
		MatTransform toGlobal;
		std::string space = "global";
		MatTransform globalToSkin;
		if (source->IsSkinned() && nif.CalcShapeTransformGlobalToSkin(source, globalToSkin)) {
			toGlobal = globalToSkin.InverseTransform();
			space = "skin";
		}
		else if (!source->IsSkinned()) {
			toGlobal = source->GetTransformToParent();
			for (NiNode* parent = nif.GetParentNode(source); parent; parent = nif.GetParentNode(parent))
				toGlobal = parent->GetTransformToParent().ComposeTransforms(toGlobal);
			space = "node";
		}

		for (auto& v : verts)
			v = toGlobal.ApplyTransform(v);
		for (auto& n : normals)
			n = toGlobal.ApplyTransformToDir(n);

		NiShape* shape = project.CreateNifShapeFromData(newName, &verts, &tris, uvs.size() == verts.size() ? &uvs : nullptr, normals.size() == verts.size() ? &normals : nullptr);
		if (!shape)
			throw McpToolError("The shape '" + sourceName + "' couldn't be created.");

		NifFile& work = *project.GetWorkNif();
		if (colors.size() == verts.size()) {
			shape->SetVertexColors(true);
			work.SetColorsForShape(shape, colors);
		}
		work.CalcTangentsForShape(shape);

		std::vector<std::string> sourceBones;
		nif.GetShapeBoneList(source, sourceBones);
		if (source->IsSkinned())
			project.CreateSkinning(shape);

		json shapeOut = {
			{"name", newName},
			{"source", sourceName},
			{"vertices", shape->GetNumVertices()},
			{"triangles", shape->GetNumTriangles()},
			{"sourceSpace", space},
			{"translationToGlobal", Vec(toGlobal.translation)},
			{"vertexColors", colors.size() == verts.size()},
			{"sourceBones", sourceBones.size()},
		};
		if (shape->GetNumVertices() < verts.size() || shape->GetNumTriangles() < tris.size())
			shapeOut["warning"] = "The target game's vertex or triangle limit was exceeded, the rest was dropped. Slider data was not imported.";

		// Shader: the source material file if there is one, else the source shader's textures
		NiShader* sourceShader = nif.GetShader(source);
		const std::string materialPath = sourceShader ? sourceShader->name.get() : std::string();
		std::string materialFile = FindMaterialFile(materialPath, dataFolders);
		if (!materialFile.empty()) {
			MaterialFile mat(materialFile);
			if (!mat.Failed()) {
				json mapped = ApplyMaterial(work, shape, mat);
				shapeOut["material"] = {{"file", materialFile}, {"source", MaterialJson(mat)}, {"target", mapped}};
			}
			else
				shapeOut["material"] = {{"file", materialFile}, {"error", "The material file couldn't be read."}};
		}
		else {
			json textures = json::object();
			for (uint32_t slot = 0; slot < 10; slot++) {
				std::string texture;
				if (nif.GetTextureSlot(source, texture, slot) && !texture.empty()) {
					work.SetTextureSlot(shape, texture, slot);
					textures[std::to_string(slot)] = texture;
				}
			}
			shapeOut["textures"] = textures;
			if (!materialPath.empty())
				shapeOut["material"] = {{"path", materialPath}, {"error", "Not found in dataFolders, only the shader's texture paths were copied."}};
		}
		project.SetTextures(shape);

		// Slider data, turned like the positions
		if (foreign && sliderMode != "none" && !shapeOut.contains("warning")) {
			SliderSet& set = foreign->set;
			for (size_t i = 0; i < set.size(); i++) {
				SliderData& slider = set[i];
				if (sliderMode == "zaps" && !slider.bZap)
					continue;

				for (auto& df : slider.dataFiles) {
					if (set.TargetToShape(df.targetName) != sourceName)
						continue;
					if (sliderMode != "all" && !df.bLocal && !slider.bZap)
						continue;

					TargetDataDiffs* diffs = sourceDiffs.GetDiffSet(df.dataName);
					if (!diffs)
						continue;

					TargetDataDiffs turned;
					for (auto& d : *diffs)
						if (d.first < shape->GetNumVertices())
							turned[d.first] = toGlobal.ApplyTransformToDiff(d.second);

					if (!project.ValidSlider(slider.name)) {
						project.AddEmptySlider(slider.name);
						size_t index = 0;
						project.SliderIndexFromName(slider.name, index);
						project.SetSliderZap(index, slider.bZap);
						project.SetSliderHidden(index, slider.bHidden);
						project.SetSliderInvert(index, slider.bInvert);
						project.SetSliderUV(index, slider.bUV);
						project.SetSliderDefault(index, static_cast<int>(slider.defSmallValue), false);
						project.SetSliderDefault(index, static_cast<int>(slider.defBigValue), true);
						if (!slider.zapToggles.empty()) {
							wxArrayString toggles;
							for (auto& t : slider.zapToggles)
								toggles.Add(wxString::FromUTF8(t));
							project.SetSliderZapToggles(index, toggles);
						}
						project.activeSet[slider.name].bClamp = slider.bClamp;
						slidersAdded.push_back(slider.name);
					}

					project.SetSliderFromDiff(slider.name, shape, turned);
					sliderNames.insert(slider.name);
				}
			}
		}

		shapesOut.push_back(shapeOut);
	}

	if (foreign) {
		if (project.mGamePath.empty())
			project.mGamePath = wxString::FromUTF8(foreign->set.GetOutputPath());
		if (project.mGameFile.empty())
			project.mGameFile = wxString::FromUTF8(foreign->set.GetOutputFile());
	}

	frame->RefreshGUIFromProj();
	frame->CreateSetSliders();
	frame->ApplySliders();
	frame->SetPendingChanges();

	McpToolResult result;
	result.data["sourceGame"] = GameName(nif.GetHeader().GetVersion());
	result.data["sourceFile"] = nifPath;
	result.data["shapes"] = shapesOut;
	result.data["slidersWithData"] = std::vector<std::string>(sliderNames.begin(), sliderNames.end());
	result.data["slidersAdded"] = slidersAdded;
	result.data["next"] =
		"Bone weights of another game's skeleton aren't kept: the shapes are skinned without weights. Load the target "
		"body (or a conversion reference from the source body), conform, then copy_bone_weights. Set partitions or "
		"segments for the target game, and check the shader.";
	return result;
}

} // namespace

void AddGameTools(OutfitStudioMcp& mcp) {
	McpTool importGame;
	importGame.name = "import_from_game";
	importGame.description =
		"Imports shapes of another game's NIF, or of a set of another game's BodySlide folder (.osp, with its "
		"ShapeData), as new shapes of the open project's game, e.g. Fallout 4 outfits into a Skyrim SE project. "
		"Geometry, UVs, normals and vertex colors are taken over in global space (skin space and node transforms are "
		"applied). With a set, the outfit's own slider data (morphs and zaps) comes along with the slider flags. "
		"Bone weights aren't kept, since the skeletons differ. The shader is set up from the source's material "
		"file (BGSM) when it's found in dataFolders: textures, specular, glossiness from smoothness, two-sided, "
		"alpha, glow, rim and back lighting; the result lists the source material and notes on what doesn't map.";
	importGame.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "NIF file, or slider set file (.osp) of another BodySlide folder (absolute or relative to this BodySlide folder)")},
			{"set", McpProp("string", "Set of the slider set file, needed when it has more than one")},
			{"shapes", McpStringList("Only these shapes. Default: all.")},
			{"skipShapes", McpStringList("Leave these shapes out, e.g. the source game's body")},
			{"sliders",
			 {{"type", "string"},
			  {"enum", {"local", "all", "zaps", "none"}},
			  {"description",
			   "Slider data to take over from a set: local (the outfit's own morphs and all zaps, default), all (also data "
			   "from the body's shared files), zaps or none"}}},
			{"dataFolders", McpStringList("Data folders of the source game or mod to find material files in, e.g. \"D:/Mods/MyOutfit\" that has Materials/...")},
		},
		{"file"});
	importGame.handler = [&mcp](const json& args) { return ImportFromGame(mcp, args); };
	mcp.AddTool(std::move(importGame));

	McpTool readMaterial;
	readMaterial.name = "read_material";
	readMaterial.description =
		"Reads a Fallout 4 or 76 material file (BGSM or BGEM) and returns all properties its type and version store, "
		"with texture paths in 'textures', named as write_material takes them.";
	readMaterial.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "Material file, or a material path as a shader names it (e.g. Materials\\\\Outfit\\\\a.bgsm) with dataFolders")},
			{"dataFolders", McpStringList("Data folders to find a material path in")},
		},
		{"path"});
	readMaterial.readOnly = true;
	readMaterial.handler = [](const json& args) {
		std::string file = MaterialFileArg(args, "path");
		MaterialFile mat(file);
		if (mat.Failed())
			throw McpToolError("'" + file + "' couldn't be read as a BGSM or BGEM file.");

		McpToolResult result;
		result.data = MaterialJson(mat);
		result.data["file"] = file;
		return result;
	};
	mcp.AddTool(std::move(readMaterial));

	McpTool writeMaterial;
	writeMaterial.name = "write_material";
	writeMaterial.description =
		"Writes a Fallout 4 or 76 material file: a BGSM (.bgsm, for lighting shaders) or BGEM (.bgem, for effect "
		"shaders). Starts from defaults, from another material file ('base'), or from a shape of the project "
		"('shape': its loaded material file, else its shader, e.g. a Skyrim shape for a conversion to Fallout 4). "
		"'properties' changes values on top, named as read_material returns them. The result shows the file as "
		"written and notes on what didn't carry over. 'assignToShapes' then names the material in those shapes' "
		"shaders (Fallout 4 and 76 projects; the file has to be in a Materials folder).";
	writeMaterial.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "Material file to write, ending in .bgsm or .bgem, e.g. \"D:/Mods/MyOutfit/Materials/MyOutfit/suit.bgsm\"")},
			{"base", McpProp("string", "Material file to start from, or a material path as a shader names it with dataFolders")},
			{"dataFolders", McpStringList("Data folders to find the base material path in")},
			{"shape", McpProp("string", "Shape of the project to start from")},
			{"version",
			 {{"type", "integer"},
			  {"description", "File version: 2 for Fallout 4, 20 to 22 for Fallout 76. Default: the base's, else 22 in a Fallout 76 project, else 2."}}},
			{"properties",
			 {{"type", "object"},
			  {"description",
			   "Property name to value, as read_material returns them: true/false, numbers, [r, g, b] colors from 0 to 1, "
			   "[u, v] for uvOffset and uvScale, alphaBlendMode (\"unknown\" for none as the game's files have it, "
			   "\"standard\", \"additive\", \"multiplicative\"), and \"textures\": {name: path relative to the "
			   "textures folder}, e.g. {\"diffuse\": \"MyOutfit/suit_d.dds\"}. BGSM textures: diffuse, normal, "
			   "smoothSpec, greyscale, glow, wrinkles, envmap, innerLayer, displacement (version 2), specular, "
			   "lighting, flow (Fallout 76). BGEM: base, grayscale, envmap, normal, envmapMask."}}},
			{"assignToShapes", McpStringList("Shapes whose shader then names this material")},
			{"overwrite", McpProp("boolean", "Replace an existing file")},
		},
		{"file"});
	writeMaterial.handler = [&mcp](const json& args) { return WriteMaterial(mcp, args); };
	mcp.AddTool(std::move(writeMaterial));
}
