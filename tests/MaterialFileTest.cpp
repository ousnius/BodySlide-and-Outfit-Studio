#include "../src/files/MaterialFile.h"
#include "../src/utils/StringStuff.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

namespace {

std::string ReadBytes(const std::filesystem::path& path) {
	std::ifstream input(path, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string WriteBytes(MaterialFile& material) {
	std::ostringstream output(std::ios::binary);
	material.Write(output);
	return output.str();
}

MaterialFile ReadMaterial(const std::string& bytes) {
	std::istringstream input(bytes, std::ios::binary);
	return MaterialFile(input);
}

std::filesystem::path MaterialData(const char* fileName) {
	return std::filesystem::u8path(BSOS_TEST_DATA_DIR) / "materials" / fileName;
}

} // namespace

TEST_CASE("Material files are written back byte for byte", "[MaterialFile]") {
	for (const char* fileName : {"Default_v2.bgsm", "DefaultEffect_v2.bgem", "Default_v21.bgsm", "DefaultEffect_v21.bgem"}) {
		INFO(fileName);
		const std::string bytes = ReadBytes(MaterialData(fileName));
		REQUIRE(!bytes.empty());

		MaterialFile material = ReadMaterial(bytes);
		REQUIRE(!material.Failed());
		REQUIRE(WriteBytes(material) == bytes);
	}
}

TEST_CASE("Material files read their values", "[MaterialFile]") {
	MaterialFile bgsm = ReadMaterial(ReadBytes(MaterialData("Default_v2.bgsm")));
	REQUIRE(bgsm.signature == MaterialFile::BGSM);
	REQUIRE(bgsm.version == 2);
	// Paths come without their terminating null, with the system's slashes
	REQUIRE(bgsm.diffuseTexture == ToOSSlashes("Shared/FlatGray01_d.dds"));
	REQUIRE(bgsm.normalTexture == ToOSSlashes("Shared/FlatFlat_n.dds"));
	REQUIRE(bgsm.rootMaterialPath.empty());

	MaterialFile bgem = ReadMaterial(ReadBytes(MaterialData("DefaultEffect_v2.bgem")));
	REQUIRE(bgem.signature == MaterialFile::BGEM);
	REQUIRE(bgem.version == 2);
	REQUIRE(bgem.baseTexture == ToOSSlashes("Shared/FlatGray01_d.dds"));
	REQUIRE(bgem.baseColorScale == 1.0f);
	REQUIRE(bgem.softDepth == 100.0f);

	MaterialFile bgem21 = ReadMaterial(ReadBytes(MaterialData("DefaultEffect_v21.bgem")));
	REQUIRE(bgem21.version == 21);
	REQUIRE(!bgem21.glassEnabled);
	REQUIRE(bgem21.softDepth == 100.0f);
}

TEST_CASE("Material files write strings like the game's files", "[MaterialFile]") {
	MaterialFile material(MaterialFile::BGSM);
	material.diffuseTexture = "Armor\\Suit\\suit_d.dds";
	const std::string bytes = WriteBytes(material);

	// Length with the null, forward slashes
	const std::string expected = std::string("\x16\0\0\0", 4) + "Armor/Suit/suit_d.dds" + std::string(1, '\0');
	REQUIRE(bytes.find(expected) != std::string::npos);
	// Empty strings are just the null: the normal map follows
	REQUIRE(bytes.compare(bytes.find(expected) + expected.size(), 5, std::string("\x01\0\0\0\0", 5)) == 0);

	MaterialFile read = ReadMaterial(bytes);
	REQUIRE(!read.Failed());
	REQUIRE(read.diffuseTexture == ToOSSlashes("Armor/Suit/suit_d.dds"));
	REQUIRE(read.normalTexture.empty());
}

TEST_CASE("Material files keep values of every version", "[MaterialFile]") {
	SECTION("Fallout 4 BGSM") {
		MaterialFile material(MaterialFile::BGSM);
		REQUIRE(material.version == 2);
		material.rimLighting = true;
		material.rimPower = 3.5f;
		material.emitEnabled = true;
		material.emittanceColor = nifly::Vector3(0.25f, 0.5f, 0.75f);
		material.smoothness = 0.4f;
		material.rootMaterialPath = "template/OutfitTemplate_Wet.bgsm";

		MaterialFile read = ReadMaterial(WriteBytes(material));
		REQUIRE(!read.Failed());
		REQUIRE(read.rimLighting);
		REQUIRE(read.rimPower == 3.5f);
		REQUIRE(read.emittanceColor == nifly::Vector3(0.25f, 0.5f, 0.75f));
		REQUIRE(read.smoothness == 0.4f);
		REQUIRE(read.rootMaterialPath == ToOSSlashes("template/OutfitTemplate_Wet.bgsm"));
	}

	SECTION("Fallout 76 BGEM with glass") {
		MaterialFile material(MaterialFile::BGEM);
		material.version = 22;
		material.baseColorScale = 2.5f;
		material.glassEnabled = true;
		material.glassFresnelColor = nifly::Vector3(0.5f, 0.6f, 0.7f);
		material.glassBlurScaleBase = 0.01f;
		material.glassBlurScaleFactor = 0.5f;
		material.glassRefractionScaleBase = 0.1f;
		material.glassDirtOverlayTexture = "Glass/dirt.dds";
		material.effectPbrSpecular = true;

		const std::string bytes = WriteBytes(material);
		MaterialFile read = ReadMaterial(bytes);
		REQUIRE(!read.Failed());
		REQUIRE(read.baseColorScale == 2.5f);
		REQUIRE(read.glassEnabled);
		REQUIRE(read.glassFresnelColor == nifly::Vector3(0.5f, 0.6f, 0.7f));
		REQUIRE(read.glassBlurScaleBase == 0.01f);
		REQUIRE(read.glassBlurScaleFactor == 0.5f);
		REQUIRE(read.glassRefractionScaleBase == 0.1f);
		REQUIRE(read.glassDirtOverlayTexture == ToOSSlashes("Glass/dirt.dds"));
		REQUIRE(read.effectPbrSpecular);
		REQUIRE(WriteBytes(read) == bytes);
	}
}

TEST_CASE("Truncated material files fail", "[MaterialFile]") {
	const std::string bytes = ReadBytes(MaterialData("DefaultEffect_v2.bgem"));
	REQUIRE(ReadMaterial(bytes.substr(0, bytes.size() - 4)).Failed());
	REQUIRE(ReadMaterial("BGXX").Failed());
}

// A folder of extracted game materials, e.g. all of "Fallout4 - Materials.ba2"
TEST_CASE("Game material files are written back byte for byte", "[MaterialFile]") {
	const char* folder = std::getenv("BSOS_MATERIALS_DIR");
	if (!folder)
		SKIP("BSOS_MATERIALS_DIR is not set");

	size_t count = 0;
	for (auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::u8path(folder))) {
		std::string ext = entry.path().extension().string();
		for (auto& c : ext)
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (ext != ".bgsm" && ext != ".bgem")
			continue;

		INFO(entry.path().string());
		const std::string bytes = ReadBytes(entry.path());
		MaterialFile material = ReadMaterial(bytes);
		REQUIRE(!material.Failed());
		REQUIRE(WriteBytes(material) == bytes);
		count++;
	}
	REQUIRE(count > 0);
}
