#include "../src/files/CSMaterialFile.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace {

std::filesystem::path MaterialData(const char* fileName) {
	return std::filesystem::u8path(BSOS_TEST_DATA_DIR) / "materials" / "csm" / fileName;
}

bool ReadText(const std::string& text, CSMaterial& material, std::string* error = nullptr) {
	std::istringstream input(text);
	return material.Read(input, error);
}

// A set of material files by their data folder relative path, read the way the game data would be
CSMaterialResolver::Loader MapLoader(const std::map<std::string, std::string>& files) {
	return [files](const std::string& relPath) -> std::unique_ptr<std::istream> {
		auto file = files.find(relPath);
		if (file == files.end())
			return nullptr;

		return std::make_unique<std::istringstream>(file->second);
	};
}

} // namespace

TEST_CASE("The example of the format reads with every key", "[CSMaterial]") {
	std::ifstream input(MaterialData("Example.csm"));
	REQUIRE(input);

	CSMaterial material;
	std::string error;
	REQUIRE(material.Read(input, &error));
	CHECK(error.empty());
	CHECK(material.Validate());

	CHECK(material.version == 1);
	CHECK(material.schema == "<TBD>");
	CHECK(material.rootMaterialPath == "");
	CHECK(material.diffuseTexture == "pbr/example/material.dds");
	CHECK(material.rmaosTexture == "pbr/example/material_rmaos.dds");
	CHECK(material.features0Texture == "pbr/example/material_s.dds");
	CHECK(material.features1Texture == "pbr/example/material_f.dds");
	CHECK(material.tileU == true);
	CHECK(material.castShadows == true);
	CHECK(material.decal == false);
	CHECK(material.pbrEnabled == true);
	CHECK(material.IsPBR());
	CHECK(*material.pbrSpecularLevel == Catch::Approx(0.04));
	CHECK(*material.pbrSubsurfaceOpacity == Catch::Approx(0.3));
	CHECK(*material.pbrFuzzRoughness == Catch::Approx(-1.0));
	CHECK(*material.pbrGlintMicrofacetRoughness == Catch::Approx(0.015));
	CHECK(material.pbrHairMarschner == false);

	REQUIRE(material.pbrSubsurfaceColor);
	CHECK(CSMaterial::FormatColor(*material.pbrSubsurfaceColor) == "#e68066");
	CHECK(material.pbrSubsurfaceColor->x == Catch::Approx(0xe6 / 255.0f));

	// Every key of the format is in the example
	for (const auto& key : CSMaterial::Keys()) {
		INFO(key.name);
		CHECK(material.HasKey(key.name));
	}
}

TEST_CASE("Keys a material leaves out stay unset", "[CSMaterial]") {
	CSMaterial material;
	REQUIRE(ReadText(R"({"Version": 1, "PBRRoughnessScale": 0.5})", material));

	CHECK(*material.pbrRoughnessScale == Catch::Approx(0.5));
	CHECK_FALSE(material.pbrEnabled);
	CHECK_FALSE(material.diffuseTexture);
	CHECK_FALSE(material.twoSided);
	CHECK_FALSE(material.IsPBR());
	CHECK(material.GetKey("PBRSpecularLevel").is_null());
}

TEST_CASE("Files that aren't version 1 materials are refused", "[CSMaterial]") {
	const char* files[] = {
		R"({})",
		R"([])",
		R"({"Version": 2})",
		R"({"Version": "1"})",
		R"({"Version": 1.5})",
		R"({"Version": 1, "Unknown": true})",
		R"({"Version": 1, "TwoSided": 1})",
		R"({"Version": 1, "Alpha": "1"})",
		R"({"Version": 1, "Alpha": 129})",
		R"({"Version": 1, "PBRSpecularLevel": -0.1})",
		R"({"Version": 1, "PBRGlintMicrofacetRoughness": 0.5})",
		R"({"Version": 1, "EmittanceColor": "#fff"})",
		R"({"Version": 1, "EmittanceColor": "ffffff"})",
		R"({"Version": 1, "EmittanceColor": "#gggggg"})",
		R"({"Version": 1, "DiffuseTexture": "a.png"})",
		R"({"Version": 1, "RootMaterialPath": "root.bgsm"})",
		R"({"Version": 1, "DiffuseTexture": null})",
		R"({"Version": 1, "$schema": 1})",
		R"({"Version": 1, "TwoSided": true,})",
		"{\"Version\": 1 // comment\n}",
		"not json",
	};

	for (const char* text : files) {
		INFO(text);
		CSMaterial material;
		std::string error;
		CHECK_FALSE(ReadText(text, material, &error));
		CHECK_FALSE(error.empty());
	}

	CSMaterial material;
	CHECK(ReadText(R"({"Version": 1.0, "DiffuseTexture": "", "RootMaterialPath": "", "Alpha": 128, "PBRFuzzRoughness": -5})", material));
	CHECK(ReadText(R"({"Version": 1, "DiffuseTexture": "A\\B.DDS", "EmittanceColor": "#A0b0C0"})", material));

	// Saved by an editor that puts a byte order mark in front
	CHECK(ReadText("\xEF\xBB\xBF{\"Version\": 1, \"TwoSided\": true}", material));
	CHECK(material.twoSided == true);

	// A file that fails part way leaves nothing of what was read before
	CHECK_FALSE(ReadText(R"({"Version": 1, "Alpha": 0.5, "TwoSided": "yes"})", material));
	CHECK(material == CSMaterial());
}

TEST_CASE("The rules between keys of the format are checked", "[CSMaterial]") {
	const char* invalid[] = {
		R"({"Version": 1, "PBRHairMarschner": true, "PBRSubsurfaceEnabled": true})",
		R"({"Version": 1, "PBRHairMarschner": true, "PBRTwoLayerCoatNormal": true})",
		R"({"Version": 1, "PBRHairMarschner": true, "PBRGlintEnabled": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": true, "PBRThinSurfaceEnabled": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": true, "PBRFuzzEnabled": true})",
		R"({"Version": 1, "PBRFuzzEnabled": true, "PBRGlintEnabled": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": false, "PBRTwoLayerColoredCoat": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": false, "PBRTwoLayerInterlayerParallax": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": false, "PBRTwoLayerCoatNormal": true})",
		R"({"Version": 1, "PBREnabled": true, "RmaosTexture": ""})",
		R"({"Version": 1, "PBRTwoLayerEnabled": true, "PBRTwoLayerInterlayerParallax": true, "DisplacementTexture": ""})",
		R"({"Version": 1, "PBRTwoLayerEnabled": true, "PBRTwoLayerCoatNormal": true, "Features1Texture": ""})",
	};

	for (const char* text : invalid) {
		INFO(text);
		CSMaterial material;
		REQUIRE(ReadText(text, material));
		CHECK_FALSE(material.Validate());
	}

	// Left out, the coat and the textures take no part in a single file
	const char* valid[] = {
		R"({"Version": 1, "PBRTwoLayerColoredCoat": true})",
		R"({"Version": 1, "PBREnabled": true})",
		R"({"Version": 1, "PBRHairMarschner": false, "PBRSubsurfaceEnabled": true, "PBRFuzzEnabled": true})",
		R"({"Version": 1, "PBRTwoLayerEnabled": true, "PBRTwoLayerColoredCoat": true, "PBRTwoLayerCoatNormal": true, "Features1Texture": "c.dds"})",
	};

	for (const char* text : valid) {
		INFO(text);
		CSMaterial material;
		REQUIRE(ReadText(text, material));
		CHECK(material.Validate());
	}
}

TEST_CASE("Resolved against the texture set, PBR needs its maps and features left out are off", "[CSMaterial]") {
	CSMaterial material;
	REQUIRE(ReadText(R"({"Version": 1, "PBREnabled": true, "RmaosTexture": "a_rmaos.dds"})", material));

	std::vector<std::string> slots = {"a.dds", "a_n.dds"};
	material.ApplyTextures(slots);
	REQUIRE(slots.size() == 8);
	CHECK(slots[5] == "a_rmaos.dds");
	CHECK(material.ValidateResolved(slots));

	slots[1].clear();
	CHECK_FALSE(material.ValidateResolved(slots));

	CSMaterial coat;
	REQUIRE(ReadText(R"({"Version": 1, "PBRTwoLayerColoredCoat": true})", coat));
	CHECK(coat.Validate());
	CHECK_FALSE(coat.ValidateResolved(slots));
}

TEST_CASE("Textures a material sets go into their texture set slots", "[CSMaterial]") {
	CSMaterial material;
	REQUIRE(ReadText(R"({"Version": 1, "DiffuseTexture": "d.dds", "NormalTexture": "", "GlowTexture": "g.dds", "DisplacementTexture": "p.dds",
		"RmaosTexture": "r.dds", "Features0Texture": "f0.dds", "Features1Texture": "f1.dds"})", material));

	std::vector<std::string> slots = {"nif_d.dds", "nif_n.dds", "", "", "nif_e.dds", "nif_m.dds", "", "", "", ""};
	material.ApplyTextures(slots);
	CHECK(slots == std::vector<std::string>{"d.dds", "", "g.dds", "p.dds", "nif_e.dds", "r.dds", "f1.dds", "f0.dds", "", ""});
}

TEST_CASE("Materials write back to the same material", "[CSMaterial]") {
	std::ifstream input(MaterialData("Example.csm"));
	CSMaterial material;
	REQUIRE(material.Read(input));

	std::ostringstream output;
	REQUIRE(material.Write(output));

	CSMaterial again;
	REQUIRE(ReadText(output.str(), again));
	CHECK(again == material);

	// Numbers come out the way they went in, and the keys in the order of the format
	const std::string text = output.str();
	CHECK(text.find("\"PBRSpecularLevel\": 0.04,") != std::string::npos);
	CHECK(text.find("\"PBRSubsurfaceColor\": \"#e68066\"") != std::string::npos);
	CHECK(text.find("\"$schema\"") < text.find("\"Version\""));
	CHECK(text.find("\"Version\"") < text.find("\"RootMaterialPath\""));
	CHECK(text.find("\"DiffuseTexture\"") < text.find("\"TileU\""));
	CHECK(text.find("\"PBRGlintDensityRandomization\"") < text.find("\"PBRHairMarschner\""));

	// Only what is set is written
	CSMaterial partial;
	REQUIRE(ReadText(R"({"Version": 1, "PBREnabled": true})", partial));
	auto json = partial.ToJson();
	CHECK(json.size() == 2);
	CHECK(json["PBREnabled"] == true);
}

TEST_CASE("Keys are set and cleared one at a time", "[CSMaterial]") {
	CSMaterial material;
	std::string error;

	CHECK(material.SetKey("PBRSpecularLevel", 0.08, &error));
	CHECK(material.GetKey("PBRSpecularLevel") == 0.08);
	CHECK(material.SetKey("PBRSpecularLevel", nullptr));
	CHECK_FALSE(material.HasKey("PBRSpecularLevel"));

	CHECK_FALSE(material.SetKey("PBRSpecularLevel", 2.0, &error));
	CHECK(error.find("PBRSpecularLevel") != std::string::npos);
	CHECK_FALSE(material.HasKey("PBRSpecularLevel"));

	CHECK_FALSE(material.SetKey("NotAKey", true, &error));
	CHECK(material.SetKey("PBRFuzzColor", "#102030"));
	CHECK(material.GetKey("PBRFuzzColor") == "#102030");
}

TEST_CASE("Material paths are normalized to the materials folder", "[CSMaterial]") {
	CHECK(CSMaterialResolver::IsMaterialPath("Materials\\PBR\\Iron.CSM"));
	CHECK_FALSE(CSMaterialResolver::IsMaterialPath("Materials\\Iron.bgsm"));
	CHECK_FALSE(CSMaterialResolver::IsMaterialPath(""));

	CHECK(CSMaterialResolver::NormalizePath("Materials\\PBR\\Iron.csm") == "materials/PBR/Iron.csm");
	CHECK(CSMaterialResolver::NormalizePath("C:\\Data\\materials\\pbr\\iron.csm") == "materials/pbr/iron.csm");
	CHECK(CSMaterialResolver::NormalizePath("pbr/iron.csm") == "materials/pbr/iron.csm");
}

TEST_CASE("A material builds on its chain of root materials", "[CSMaterial]") {
	auto load = MapLoader({
		{"materials/base.csm", R"({"Version": 1, "PBREnabled": true, "PBRSpecularLevel": 0.04, "PBRRoughnessScale": 0.8, "TwoSided": true})"},
		{"materials/metal/iron.csm", R"({"Version": 1, "RootMaterialPath": "Materials\\base.csm", "PBRRoughnessScale": 0.5, "RmaosTexture": "iron_rmaos.dds"})"},
		{"materials/metal/rusty.csm", R"({"Version": 1, "RootMaterialPath": "metal/iron.csm", "TwoSided": false})"},
	});

	CSMaterial material;
	std::string error;
	std::vector<std::string> chain;
	REQUIRE(CSMaterialResolver::Resolve("Materials\\metal\\rusty.csm", load, material, &error, &chain));
	CHECK(error.empty());
	CHECK(chain == std::vector<std::string>{"materials/metal/rusty.csm", "materials/metal/iron.csm", "materials/base.csm"});

	CHECK(material.twoSided == false);
	CHECK(*material.pbrRoughnessScale == Catch::Approx(0.5));
	CHECK(*material.pbrSpecularLevel == Catch::Approx(0.04));
	CHECK(material.rmaosTexture == "iron_rmaos.dds");
	CHECK(material.IsPBR());
	CHECK(material.rootMaterialPath == "metal/iron.csm");
}

TEST_CASE("A shape's material goes over its texture set only when it is valid with it", "[CSMaterial]") {
	auto load = MapLoader({
		{"materials/pbr.csm", R"({"Version": 1, "PBREnabled": true, "RmaosTexture": "a_rmaos.dds"})"},
		{"materials/plain.csm", R"({"Version": 1, "DiffuseTexture": "b.dds", "TwoSided": true})"},
	});

	const std::vector<std::string> nifTextures = {"a.dds", "a_n.dds", "", "", "", "a_m.dds", "", "", "", ""};

	std::vector<std::string> textures = nifTextures;
	auto material = CSMaterialResolver::ResolveForShape("Materials\\pbr.csm", load, textures);
	REQUIRE(material);
	CHECK(material->IsPBR());
	CHECK(textures.size() == nifTextures.size());
	CHECK(textures[5] == "a_rmaos.dds");
	CHECK(textures[0] == "a.dds");

	textures = nifTextures;
	material = CSMaterialResolver::ResolveForShape("plain.csm", load, textures);
	REQUIRE(material);
	CHECK_FALSE(material->IsPBR());
	CHECK(textures[0] == "b.dds");

	// PBR without a normal map is a material the game ignores, and so the texture set stays the NIF's
	textures = nifTextures;
	textures[1].clear();
	std::string error;
	CHECK_FALSE(CSMaterialResolver::ResolveForShape("pbr.csm", load, textures, &error));
	CHECK(error.find("NormalTexture") != std::string::npos);
	CHECK(textures[5] == "a_m.dds");
}

TEST_CASE("A broken chain of root materials makes the material invalid", "[CSMaterial]") {
	CSMaterial material;
	std::string error;

	SECTION("Missing material") {
		CHECK_FALSE(CSMaterialResolver::Resolve("materials/none.csm", MapLoader({}), material, &error));
	}

	SECTION("Missing root") {
		auto load = MapLoader({{"materials/a.csm", R"({"Version": 1, "RootMaterialPath": "b.csm"})"}});
		CHECK_FALSE(CSMaterialResolver::Resolve("materials/a.csm", load, material, &error));
		CHECK(error.find("materials/b.csm") != std::string::npos);
	}

	SECTION("Invalid root") {
		auto load = MapLoader({
			{"materials/a.csm", R"({"Version": 1, "RootMaterialPath": "b.csm"})"},
			{"materials/b.csm", R"({"Version": 2})"},
		});
		CHECK_FALSE(CSMaterialResolver::Resolve("materials/a.csm", load, material, &error));
	}

	SECTION("Loop") {
		auto load = MapLoader({
			{"materials/a.csm", R"({"Version": 1, "RootMaterialPath": "B.csm"})"},
			{"materials/B.csm", R"({"Version": 1, "RootMaterialPath": "a.csm"})"},
		});
		CHECK_FALSE(CSMaterialResolver::Resolve("materials/a.csm", load, material, &error));
		CHECK(error.find("loop") != std::string::npos);
	}

	SECTION("Rules broken only once merged") {
		auto load = MapLoader({
			{"materials/a.csm", R"({"Version": 1, "RootMaterialPath": "b.csm", "PBRGlintEnabled": true})"},
			{"materials/b.csm", R"({"Version": 1, "PBRFuzzEnabled": true})"},
		});
		CHECK_FALSE(CSMaterialResolver::Resolve("materials/a.csm", load, material, &error));
	}

	CHECK_FALSE(error.empty());
	CHECK(material == CSMaterial());
}
