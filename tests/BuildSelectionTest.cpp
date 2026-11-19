#include "components/BuildSelection.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace {
struct TempFile {
	std::filesystem::path path;

	TempFile() {
		std::random_device random;
		path = std::filesystem::temp_directory_path() / ("BSOSTests-BuildSelection-" + std::to_string(random()) + ".xml");
	}

	~TempFile() {
		std::error_code ec;
		std::filesystem::remove(path, ec);
	}

	std::string str() const { return path.u8string(); }
};

BuildRecord Record(const std::string& path, const std::string& folder, const std::string& set, const std::string& preset) {
	BuildRecord record;
	record.path = path;
	record.dataFolder = folder;
	record.set = set;
	record.preset = preset;
	record.time = "2026-11-16 12:00:00";
	return record;
}

void Load(const std::string& fileName, BuildSelection& buildSel) {
	BuildSelectionFile file(fileName);
	REQUIRE_FALSE(file.fail());
	file.Get(buildSel);
}
} // namespace

TEST_CASE("Build records are saved per output path and data folder", "[BuildSelection]") {
	TempFile temp;
	std::filesystem::path dataA = std::filesystem::temp_directory_path() / "BSOSTests-DataA";
	std::filesystem::path dataB = std::filesystem::temp_directory_path() / "BSOSTests-DataB";
	const std::string folderA = dataA.u8string();
	const std::string folderB = dataB.u8string();

	{
		BuildSelectionFile file;
		file.New(temp.str());
		file.SetBuildRecords({Record("meshes\\armor\\a", folderA, "Outfit A", "Preset 1"),
							  Record("meshes\\armor\\a", folderB, "Outfit B", "Preset 2"),
							  Record("meshes\\armor\\b", folderA, "Outfit C", "Preset 1")});
		REQUIRE(file.Save());
	}

	BuildSelection buildSel;
	Load(temp.str(), buildSel);

	// The trailing separator doesn't matter
	const BuildRecord* record = buildSel.GetBuildRecord("meshes\\armor\\a", folderA + "/");
	REQUIRE(record);
	CHECK(record->set == "Outfit A");
	CHECK(record->preset == "Preset 1");
	CHECK(record->time == "2026-11-16 12:00:00");

	record = buildSel.GetBuildRecord("meshes\\armor\\a", folderB);
	REQUIRE(record);
	CHECK(record->set == "Outfit B");

	CHECK(buildSel.GetBuildRecord("meshes\\armor\\b", folderB) == nullptr);
	CHECK(buildSel.GetBuildRecord("meshes\\armor\\c", folderA) == nullptr);

#ifdef _WINDOWS
	// Windows paths compare without case
	std::string upperA = folderA;
	for (auto& c : upperA)
		c = (char)toupper((unsigned char)c);
	CHECK(buildSel.GetBuildRecord("meshes\\armor\\b", upperA) != nullptr);
#endif
}

TEST_CASE("Build records are replaced and removed in one data folder only", "[BuildSelection]") {
	TempFile temp;
	const std::string folderA = (std::filesystem::temp_directory_path() / "BSOSTests-DataA").u8string();
	const std::string folderB = (std::filesystem::temp_directory_path() / "BSOSTests-DataB").u8string();

	{
		BuildSelectionFile file;
		file.New(temp.str());
		file.SetBuildRecords({Record("meshes\\a", folderA, "Outfit A", "Preset 1"), Record("meshes\\a", folderB, "Outfit A", "Preset 1")});
		REQUIRE(file.Save());
	}

	{
		BuildSelectionFile file(temp.str());
		REQUIRE_FALSE(file.fail());
		file.SetBuildRecords({Record("meshes\\a", folderA + "/", "Outfit B", "Preset 2")});
		file.RemoveBuildRecords({"meshes\\a", "meshes\\missing"}, folderB);
		REQUIRE(file.Save());
	}

	BuildSelection buildSel;
	Load(temp.str(), buildSel);

	const BuildRecord* record = buildSel.GetBuildRecord("meshes\\a", folderA);
	REQUIRE(record);
	CHECK(record->set == "Outfit B");
	CHECK(record->preset == "Preset 2");
	CHECK(buildSel.GetBuildRecord("meshes\\a", folderB) == nullptr);

	// Replacing kept a single element for the folder
	tinyxml2::XMLDocument doc;
	REQUIRE(doc.LoadFile(temp.str().c_str()) == tinyxml2::XML_SUCCESS);
	int count = 0;
	for (auto elem = doc.FirstChildElement("BuildSelection")->FirstChildElement("Built"); elem; elem = elem->NextSiblingElement("Built"))
		count++;
	CHECK(count == 1);
}

TEST_CASE("Build records leave output and zap choices alone", "[BuildSelection]") {
	TempFile temp;
	const std::string folder = (std::filesystem::temp_directory_path() / "BSOSTests-DataA").u8string();

	{
		BuildSelectionFile file;
		file.New(temp.str());

		BuildSelection buildSel;
		buildSel.SetOutputChoice("meshes\\a", "Outfit A");
		buildSel.SetZapChoice("Outfit A", "Zap", true);
		file.UpdateOutputChoices(buildSel);
		file.UpdateZapChoices(buildSel);
		file.SetBuildRecords({Record("meshes\\a", folder, "Outfit B", "Preset")});
		file.RemoveBuildRecords({"meshes\\a"}, folder);
		REQUIRE(file.Save());
	}

	BuildSelection buildSel;
	Load(temp.str(), buildSel);
	CHECK(buildSel.GetOutputChoice("meshes\\a") == "Outfit A");
	CHECK(buildSel.GetZapChoice("Outfit A", "Zap"));
	CHECK(buildSel.GetBuildRecord("meshes\\a", folder) == nullptr);
}

TEST_CASE("Build records keep only values that differ from the preset", "[BuildSelection]") {
	BuildRecord record;
	CHECK_FALSE(record.IsModified());

	// Same whole percent as the preset
	record.AddPresetChange("Breasts", false, false, 0.501f, 0.5f);
	record.AddPresetChange("Breasts", true, false, 0.3f, 0.3f);
	CHECK_FALSE(record.IsModified());

	record.AddPresetChange("Breasts", false, false, 0.49f, 0.5f);
	record.AddPresetChange("Butt", true, false, 0.0f, 0.25f);
	REQUIRE(record.sliders.size() == 2);
	CHECK(record.sliders[0].name == "Breasts");
	CHECK_FALSE(record.sliders[0].isSmall);
	CHECK(record.sliders[0].value == 49);
	CHECK(record.sliders[1].name == "Butt");
	CHECK(record.sliders[1].isSmall);
	CHECK(record.sliders[1].value == 0);

	// Zaps are on or off, and only have the high value
	record.AddPresetChange("Stockings", false, true, 0.01f, 1.0f);
	record.AddPresetChange("Stockings", true, true, 0.0f, 1.0f);
	CHECK(record.zaps.empty());

	record.AddPresetChange("Stockings", false, true, 1.0f, 0.0f);
	record.AddPresetChange("Gloves", false, true, 0.0f, 1.0f);
	REQUIRE(record.zaps.size() == 2);
	CHECK(record.zaps[0].name == "Stockings");
	CHECK(record.zaps[0].zapped);
	CHECK(record.zaps[1].name == "Gloves");
	CHECK_FALSE(record.zaps[1].zapped);
	CHECK(record.IsModified());
}

TEST_CASE("Build records save their slider and zap changes", "[BuildSelection]") {
	TempFile temp;
	const std::string folder = (std::filesystem::temp_directory_path() / "BSOSTests-DataA").u8string();

	BuildRecord record = Record("meshes\\a", folder, "Outfit A", "Preset");
	record.time = "2026-11-16T21:15:50+01:00";
	record.sliders.push_back({"Breasts", false, 49});
	record.sliders.push_back({"Breasts", true, 30});
	record.zaps.push_back({"Stockings", true});
	record.zaps.push_back({"Gloves", false});

	{
		BuildSelectionFile file;
		file.New(temp.str());
		file.SetBuildRecords({record});
		REQUIRE(file.Save());
	}

	BuildSelection buildSel;
	Load(temp.str(), buildSel);
	const BuildRecord* loaded = buildSel.GetBuildRecord("meshes\\a", folder);
	REQUIRE(loaded);
	CHECK(loaded->time == "2026-11-16T21:15:50+01:00");
	REQUIRE(loaded->sliders.size() == 2);
	CHECK(loaded->sliders[0].name == "Breasts");
	CHECK_FALSE(loaded->sliders[0].isSmall);
	CHECK(loaded->sliders[0].value == 49);
	CHECK(loaded->sliders[1].isSmall);
	CHECK(loaded->sliders[1].value == 30);
	REQUIRE(loaded->zaps.size() == 2);
	CHECK(loaded->zaps[0].name == "Stockings");
	CHECK(loaded->zaps[0].zapped);
	CHECK(loaded->zaps[1].name == "Gloves");
	CHECK_FALSE(loaded->zaps[1].zapped);

	// A rebuild without changes clears them
	{
		BuildSelectionFile file(temp.str());
		REQUIRE_FALSE(file.fail());
		file.SetBuildRecords({Record("meshes\\a", folder, "Outfit A", "Preset")});
		REQUIRE(file.Save());
	}

	BuildSelection rebuilt;
	Load(temp.str(), rebuilt);
	loaded = rebuilt.GetBuildRecord("meshes\\a", folder);
	REQUIRE(loaded);
	CHECK_FALSE(loaded->IsModified());
}
