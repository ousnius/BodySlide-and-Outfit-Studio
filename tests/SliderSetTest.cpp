#include "../src/components/SliderManager.h"
#include "../src/components/SliderSet.h"
#include "../src/components/SliderSetIndex.h"
#include "../src/components/SliderSetInheritance.h"
#include "../src/utils/StringStuff.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
// Project folder with a "SliderSets" folder of its own for each test
struct ProjectFolder {
	std::filesystem::path path;

	ProjectFolder() {
		std::random_device random;
		path = std::filesystem::temp_directory_path() / ("BSOSTests-" + std::to_string(random()));
		std::filesystem::create_directories(path / "SliderSets");
	}

	~ProjectFolder() {
		std::error_code error;
		std::filesystem::remove_all(path, error);
	}

	std::string SetFile(const std::string& fileName) const { return (path / "SliderSets" / fileName).u8string(); }
};

std::string DataFile(const std::string& osdName, const std::string& dataName) {
	return osdName + PathSepStr + dataName;
}

SliderSet MakeSet(const std::string& name, const std::string& dataFolder, const std::string& nif, const std::vector<std::string>& shapes) {
	SliderSet set;
	set.SetName(name);
	set.SetDataFolder(dataFolder);
	set.SetInputFile(nif);
	set.SetOutputPath("meshes");
	set.SetOutputFile(name);
	for (auto& shape : shapes)
		set.AddShapeTarget(shape, shape);
	return set;
}

SliderData& AddSlider(SliderSet& set, const std::string& sliderName) {
	return set[set.CreateSlider(sliderName)];
}

// Robe: shapes Robe and Cape, sliders Breasts (both shapes), Butt and ZapBelt (Robe)
SliderSet MakeBase() {
	SliderSet base = MakeSet("Robe", "Robe", "Robe.nif", {"Robe", "Cape"});
	base.SetProjectID("5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c");

	auto& breasts = AddSlider(base, "Breasts");
	breasts.AddDataFile("Robe", "RobeBreasts", DataFile("Robe.osd", "RobeBreasts"));
	breasts.AddDataFile("Cape", "CapeBreasts", DataFile("Robe.osd", "CapeBreasts"));

	auto& butt = AddSlider(base, "Butt");
	butt.AddDataFile("Robe", "RobeButt", DataFile("Robe.osd", "RobeButt"));

	auto& zapBelt = AddSlider(base, "ZapBelt");
	zapBelt.bZap = true;
	zapBelt.AddDataFile("Robe", "RobeZapBelt", DataFile("Robe.osd", "RobeZapBelt"));
	return base;
}

// Variant of Robe with a Hood shape of its own, a Flare slider of its own and without ZapBelt
SliderSet MakeVariant(const std::string& dataFolder = "Robe") {
	SliderSet variant = MakeSet("Robe Variant", dataFolder, "Robe Variant.nif", {"Robe", "Cape", "Hood"});
	variant.SetBaseProject("SliderSets" + std::string(PathSepStr) + "Robe.osp", "Robe", "5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c");

	auto& breasts = AddSlider(variant, "Breasts");
	breasts.bInherited = true;
	breasts.AddDataFile("Robe", "RobeBreasts", DataFile("Robe.osd", "RobeBreasts"), true, true);
	breasts.AddDataFile("Cape", "CapeBreasts", DataFile("Robe.osd", "CapeBreasts"), true, true);
	breasts.AddDataFile("Hood", "HoodBreasts", DataFile("Robe Variant.osd", "HoodBreasts"));

	auto& butt = AddSlider(variant, "Butt");
	butt.bInherited = true;
	butt.AddDataFile("Robe", "RobeButt", DataFile("Robe.osd", "RobeButt"), true, true);

	auto& flare = AddSlider(variant, "Flare");
	flare.AddDataFile("Robe", "RobeFlare", DataFile("Robe Variant.osd", "RobeFlare"));

	variant.AddRemovedSlider("ZapBelt");
	return variant;
}

void SaveSets(const std::string& fileName, std::vector<SliderSet> sets) {
	SliderSetFile file;
	file.New(fileName);
	for (auto& set : sets)
		file.UpdateSet(set);
	REQUIRE(file.Save());
}

SliderSet LoadSet(const std::string& fileName, const std::string& setName, bool resolveBase = true) {
	SliderSetFile file(fileName);
	REQUIRE_FALSE(file.fail());

	SliderSet set;
	REQUIRE(file.GetSet(setName, set, true, "", resolveBase) == 0);
	return set;
}

std::vector<std::string> SliderNames(const SliderSet& set) {
	std::vector<std::string> names;
	for (size_t i = 0; i < set.size(); i++)
		names.push_back(set[i].name);
	return names;
}

const DiffInfo* FindData(SliderSet& set, const std::string& sliderName, const std::string& target) {
	auto& slider = set[sliderName];
	for (auto& df : slider.dataFiles)
		if (df.targetName == target)
			return &df;
	return nullptr;
}
} // namespace

TEST_CASE("Version 2 project files keep base projects, add-ons and project IDs", "[SliderSet]") {
	ProjectFolder folder;

	SliderSet addOn = MakeSet("Robe Extra", "Robe", "Robe.nif", {"Robe"});
	addOn.SetAddOn(true);
	addOn.SetSliderDataFile("Robe Extra.osd");
	addOn.SetBaseProject("", "Robe", "");
	addOn.SetShapeVertexCount("Robe", 1234);
	AddSlider(addOn, "Extra").AddDataFile("Robe", "RobeExtra", DataFile("Robe Extra.osd", "RobeExtra"));

	SaveSets(folder.SetFile("Robe.osp"), {MakeBase(), MakeVariant(), addOn});

	SliderSetFile file(folder.SetFile("Robe.osp"));
	CHECK(file.GetVersion() == 2);
	CHECK(file.IsAddOn("Robe Extra"));
	CHECK_FALSE(file.IsAddOn("Robe Variant"));
	CHECK(file.FindSetByID("5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c") == "Robe");

	SliderSet variant = LoadSet(folder.SetFile("Robe.osp"), "Robe Variant", false);
	CHECK(variant.GetBaseProjectName() == "Robe");
	CHECK(variant.GetBaseProjectID() == "5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c");
	CHECK(variant.GetBaseProjectFile() == ToOSSlashes("SliderSets\\Robe.osp"));
	CHECK(variant.GetRemovedSliders() == std::vector<std::string>{"ZapBelt"});
	CHECK(variant["Breasts"].bInherited);
	CHECK_FALSE(variant["Flare"].bInherited);
	CHECK(FindData(variant, "Breasts", "Robe")->bInherited);
	CHECK_FALSE(FindData(variant, "Breasts", "Hood")->bInherited);

	SliderSet loadedAddOn = LoadSet(folder.SetFile("Robe.osp"), "Robe Extra", false);
	CHECK(loadedAddOn.IsAddOn());
	CHECK(loadedAddOn.GetSliderDataFile() == "Robe Extra.osd");
	CHECK(loadedAddOn.GetShapeVertexCount("Robe") == 1234);
}

TEST_CASE("Project files stay at version 1 without version 2 features", "[SliderSet]") {
	ProjectFolder folder;
	SliderSet plain = MakeSet("Plain", "Plain", "Plain.nif", {"Plain"});
	AddSlider(plain, "Breasts").AddDataFile("Plain", "PlainBreasts", DataFile("Plain.osd", "PlainBreasts"));
	SaveSets(folder.SetFile("Plain.osp"), {plain});

	CHECK(SliderSetFile(folder.SetFile("Plain.osp")).GetVersion() == 1);

	plain.SetProjectID(SliderSet::GenerateProjectID());
	SaveSets(folder.SetFile("Plain.osp"), {plain});
	CHECK(SliderSetFile(folder.SetFile("Plain.osp")).GetVersion() == 2);
}

TEST_CASE("Project IDs are random version 4 GUIDs", "[SliderSet]") {
	std::string id = SliderSet::GenerateProjectID();
	REQUIRE(id.size() == 36);
	CHECK(id[8] == '-');
	CHECK(id[14] == '4');
	CHECK(std::strchr("89ab", id[19]));
	CHECK(SliderSet::GenerateProjectID() != id);
}

TEST_CASE("Older versions read variants as they were saved and don't see add-ons", "[SliderSet]") {
	ProjectFolder folder;

	SliderSet addOn = MakeSet("Robe Extra", "Robe", "Robe.nif", {"Robe"});
	addOn.SetAddOn(true);
	addOn.SetBaseProject("", "Robe", "");
	AddSlider(addOn, "Extra").AddDataFile("Robe", "RobeExtra", DataFile("Robe Extra.osd", "RobeExtra"));
	SaveSets(folder.SetFile("Robe.osp"), {MakeBase(), MakeVariant(), addOn});

	// What versions before 2 index: only <SliderSet> elements
	XMLDocument doc;
	REQUIRE(doc.LoadFile(folder.SetFile("Robe.osp").c_str()) == XML_SUCCESS);
	std::vector<std::string> oldSetNames;
	for (auto element = doc.FirstChildElement("SliderSetInfo")->FirstChildElement("SliderSet"); element; element = element->NextSiblingElement("SliderSet"))
		oldSetNames.push_back(element->Attribute("name"));
	CHECK(oldSetNames == std::vector<std::string>{"Robe", "Robe Variant"});

	// Every slider of the variant is in the file, with data paths that resolve without the base project
	SliderSet variant = LoadSet(folder.SetFile("Robe.osp"), "Robe Variant", false);
	CHECK(SliderNames(variant) == std::vector<std::string>{"Breasts", "Butt", "Flare"});
	auto robeBreasts = FindData(variant, "Breasts", "Robe");
	REQUIRE(robeBreasts);
	CHECK(robeBreasts->bLocal);
	CHECK(robeBreasts->fileName == DataFile("Robe.osd", "RobeBreasts"));
}

TEST_CASE("Variants inherit the current sliders of their base project", "[SliderSet]") {
	ProjectFolder folder;
	SliderSet base = MakeBase();
	SaveSets(folder.SetFile("Robe.osp"), {base});
	SaveSets(folder.SetFile("Robe Variant.osp"), {MakeVariant()});

	SECTION("as saved") {
		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant.GetResolveStatus().baseFound);
		CHECK(variant.GetResolveStatus().warnings.empty());
		CHECK(SliderNames(variant) == std::vector<std::string>{"Breasts", "Butt", "Flare"});
		CHECK(variant["Breasts"].dataFiles.size() == 3);
		CHECK(FindData(variant, "Breasts", "Cape")->bInherited);
		CHECK_FALSE(FindData(variant, "Breasts", "Hood")->bInherited);
	}

	SECTION("with sliders and properties the base project got since") {
		auto& thighs = base[base.CreateSlider("Thighs")];
		thighs.AddDataFile("Robe", "RobeThighs", DataFile("Robe.osd", "RobeThighs"));
		base.DeleteSlider("Butt");
		base["Breasts"].bInvert = true;
		base["ZapBelt"].zapToggles = {"Breasts"};

		// Move Thighs before ZapBelt, after Breasts
		SliderSet reordered = MakeSet("Robe", "Robe", "Robe.nif", {"Robe", "Cape"});
		reordered.SetProjectID(base.GetProjectID());
		reordered.CopySlider(&base["Breasts"]);
		reordered.CopySlider(&base["Thighs"]);
		reordered.CopySlider(&base["ZapBelt"]);
		SaveSets(folder.SetFile("Robe.osp"), {reordered});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(SliderNames(variant) == std::vector<std::string>{"Breasts", "Thighs", "Flare"});
		CHECK(variant["Breasts"].bInvert);
		CHECK(variant["Thighs"].bInherited);
		CHECK(FindData(variant, "Thighs", "Robe")->bInherited);
		// Butt only had inherited data
		CHECK_FALSE(variant.GetResolveStatus().warnings.empty());
	}

	SECTION("keeping the properties and data the variant changed") {
		SliderSet changed = MakeVariant();
		changed["Breasts"].bInherited = false;
		changed["Breasts"].defBigValue = 50.0f;
		changed["Breasts"].dataFiles[0] = DiffInfo(true, "RobeBreasts", "Robe", DataFile("Robe Variant.osd", "RobeBreasts"));
		SaveSets(folder.SetFile("Robe Variant.osp"), {changed});

		base["Breasts"].defBigValue = 20.0f;
		SaveSets(folder.SetFile("Robe.osp"), {base});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant["Breasts"].defBigValue == 50.0f);
		CHECK_FALSE(FindData(variant, "Breasts", "Robe")->bInherited);
		CHECK(FindData(variant, "Breasts", "Robe")->fileName == DataFile("Robe Variant.osd", "RobeBreasts"));
		CHECK(FindData(variant, "Breasts", "Cape")->bInherited);
	}

	SECTION("without the shapes the variant doesn't have") {
		SliderSet noCape = MakeSet("Robe Variant", "Robe", "Robe Variant.nif", {"Robe"});
		noCape.SetBaseProject("", "Robe", "");
		auto& breasts = AddSlider(noCape, "Breasts");
		breasts.bInherited = true;
		breasts.AddDataFile("Robe", "RobeBreasts", DataFile("Robe.osd", "RobeBreasts"), true, true);
		SaveSets(folder.SetFile("Robe Variant.osp"), {noCape});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant["Breasts"].dataFiles.size() == 1);
		CHECK_FALSE(FindData(variant, "Breasts", "Cape"));
		// Base sliders it didn't have yet come in, the removed ZapBelt doesn't exist here
		CHECK(SliderNames(variant) == std::vector<std::string>{"Breasts", "Butt", "ZapBelt"});
	}

	SECTION("from a base project in another data folder") {
		SaveSets(folder.SetFile("Robe Variant.osp"), {MakeVariant("Robe Variant")});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		auto robeBreasts = FindData(variant, "Breasts", "Robe");
		CHECK_FALSE(robeBreasts->bLocal);
		CHECK(variant.GetShapeDataFolders("Robe") == std::vector<std::string>{"Robe Variant", "Robe"});
		CHECK(FindData(variant, "Breasts", "Hood")->bLocal);
		CHECK(variant.GetShapeDataFolders("Hood") == std::vector<std::string>{"Robe Variant"});
	}
}

TEST_CASE("Base projects are found by ID, file and name", "[SliderSet]") {
	ProjectFolder folder;

	SECTION("a renamed base project by its ID") {
		SliderSet base = MakeBase();
		base.SetName("Robe Renamed");
		base["Breasts"].bInvert = true;
		SaveSets(folder.SetFile("Robe.osp"), {base});
		SaveSets(folder.SetFile("Robe Variant.osp"), {MakeVariant()});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant.GetResolveStatus().baseFound);
		CHECK(variant["Breasts"].bInvert);
	}

	SECTION("a base project in another file than the hint by its name") {
		SliderSet base = MakeBase();
		base.SetProjectID("");
		base["Breasts"].bInvert = true;
		SaveSets(folder.SetFile("Other.osp"), {base});

		SliderSet stored = MakeVariant();
		stored.SetBaseProject("SliderSets" + std::string(PathSepStr) + "Missing.osp", "Robe", "");
		SaveSets(folder.SetFile("Robe Variant.osp"), {stored});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant.GetResolveStatus().baseFound);
		CHECK(variant["Breasts"].bInvert);
	}

	SECTION("a missing base project leaves the variant as it was saved") {
		SaveSets(folder.SetFile("Robe Variant.osp"), {MakeVariant()});

		SliderSet variant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		CHECK(variant.GetResolveStatus().baseMissing);
		CHECK_FALSE(variant.GetResolveStatus().warnings.empty());
		CHECK(SliderNames(variant) == std::vector<std::string>{"Breasts", "Butt", "Flare"});
		CHECK(FindData(variant, "Breasts", "Robe")->bInherited);
	}

	SECTION("base projects that name each other") {
		SliderSet first = MakeSet("First", "Loop", "First.nif", {"Loop"});
		first.SetBaseProject("", "Second", "");
		AddSlider(first, "A").AddDataFile("Loop", "LoopA", DataFile("First.osd", "LoopA"));

		SliderSet second = MakeSet("Second", "Loop", "Second.nif", {"Loop"});
		second.SetBaseProject("", "First", "");
		AddSlider(second, "B").AddDataFile("Loop", "LoopB", DataFile("Second.osd", "LoopB"));
		SaveSets(folder.SetFile("Loop.osp"), {first, second});

		SliderSet loaded = LoadSet(folder.SetFile("Loop.osp"), "First");
		CHECK_FALSE(loaded.GetResolveStatus().warnings.empty());
		CHECK(loaded.SliderExists("A"));
	}
}

TEST_CASE("Add-ons use the shapes of their base project", "[SliderSet]") {
	ProjectFolder folder;
	SaveSets(folder.SetFile("Robe.osp"), {MakeBase()});

	// Saved when the base project had another NIF file and only one shape
	SliderSet stored = MakeSet("Robe Extra", "Robe", "Old.nif", {"Robe"});
	stored.SetAddOn(true);
	stored.SetSliderDataFile("Robe Extra.osd");
	stored.SetBaseProject("", "Robe", "");
	stored.SetShapeVertexCount("Robe", 500);
	AddSlider(stored, "Extra").AddDataFile("Robe", "RobeExtra", DataFile("Robe Extra.osd", "RobeExtra"));
	SaveSets(folder.SetFile("Robe Extra.osp"), {stored});

	SliderSet addOn = LoadSet(folder.SetFile("Robe Extra.osp"), "Robe Extra");
	CHECK(addOn.IsAddOn());
	CHECK(addOn.GetInputFile() == "Robe.nif");
	CHECK(addOn.HasShape("Cape"));
	CHECK(addOn.GetShapeVertexCount("Robe") == 500);
	CHECK(SliderNames(addOn) == std::vector<std::string>{"Breasts", "Butt", "ZapBelt", "Extra"});
	CHECK(FindData(addOn, "Breasts", "Cape")->bInherited);
	CHECK_FALSE(FindData(addOn, "Extra", "Robe")->bInherited);
}

TEST_CASE("Elements of newer versions are written back", "[SliderSet]") {
	ProjectFolder folder;
	std::string fileName = folder.SetFile("Future.osp");

	XMLDocument doc;
	doc.Parse("<SliderSetInfo version=\"3\"><SliderSet name=\"Future\"><DataFolder>Future</DataFolder><SourceFile>Future.nif</SourceFile>"
			  "<OutputPath>meshes</OutputPath><OutputFile>Future</OutputFile><Shape target=\"Body\">Body</Shape>"
			  "<Slider name=\"Breasts\" invert=\"false\" small=\"0\" big=\"0\"><Data name=\"BodyBreasts\" target=\"Body\" local=\"true\">Future.osd\\BodyBreasts</Data></Slider>"
			  "<NormalsGeneration><Layer name=\"Base\"/></NormalsGeneration><Hologram strength=\"2\"><Part/></Hologram></SliderSet></SliderSetInfo>");
	REQUIRE(doc.SaveFile(fileName.c_str()) == XML_SUCCESS);

	SliderSet set = LoadSet(fileName, "Future");
	CHECK(set.GetResolveStatus().newerVersion);
	CHECK(set.GetUnknownElements().size() == 2);

	{
		SliderSetFile file(fileName);
		file.UpdateSet(set);
		REQUIRE(file.Save());
	}

	XMLDocument saved;
	REQUIRE(saved.LoadFile(fileName.c_str()) == XML_SUCCESS);
	auto setElement = saved.FirstChildElement("SliderSetInfo")->FirstChildElement("SliderSet");
	REQUIRE(setElement->FirstChildElement("Hologram"));
	CHECK(setElement->FirstChildElement("Hologram")->IntAttribute("strength") == 2);
	CHECK(setElement->FirstChildElement("Hologram")->FirstChildElement("Part"));
	CHECK(setElement->FirstChildElement("NormalsGeneration"));
	CHECK(saved.FirstChildElement("SliderSetInfo")->IntAttribute("version") == 3);
}

TEST_CASE("The save index only loads sets that mention the files", "[SliderSet]") {
	ProjectFolder folder;
	const std::string projectPath = folder.path.u8string();
	const std::string robeNif = projectPath + PathSepStr + "ShapeData" + PathSepStr + "Robe" + PathSepStr + "Robe.nif";
	const std::string robeOSD = projectPath + PathSepStr + "ShapeData" + PathSepStr + "Robe" + PathSepStr + "Robe.osd";

	// Another set in the same file that doesn't use Robe's files
	SliderSet boots = MakeSet("Boots", "Boots", "Boots.nif", {"Boots"});
	AddSlider(boots, "Heels").AddDataFile("Boots", "BootsHeels", DataFile("Boots.osd", "BootsHeels"));
	SaveSets(folder.SetFile("Robe.osp"), {MakeBase(), boots});

	// The names are compared without case
	SliderSet hood = MakeSet("Hood", "Robe", "Hood.nif", {"Hood"});
	AddSlider(hood, "Breasts").AddDataFile("Hood", "HoodBreasts", DataFile("ROBE.OSD", "HoodBreasts"));
	SaveSets(folder.SetFile("Hood.osp"), {hood});

	SaveSets(folder.SetFile("Gloves.osp"), {MakeSet("Gloves", "Gloves", "Gloves.nif", {"Gloves"})});

	SliderSetIndex full(projectPath);
	SliderSetIndex filtered(projectPath, {"Robe.nif", "Robe.osd"});

	std::vector<std::string> loaded;
	for (auto& entry : filtered.Entries())
		loaded.push_back(entry.setName);
	std::sort(loaded.begin(), loaded.end());
	CHECK(loaded == std::vector<std::string>{"Hood", "Robe"});
	CHECK(full.Entries().size() == 4);

	// Slider data is loaded from "ROBE.OSD" as the set names it, which is Robe.osd only where paths ignore case
#ifdef _WINDOWS
	const size_t hoodUsesRobeOSD = 1;
#else
	const size_t hoodUsesRobeOSD = 0;
#endif

	CHECK(filtered.DataUsedInFile(robeOSD, "", "") == full.DataUsedInFile(robeOSD, "", ""));
	CHECK(filtered.DataUsedInFile(robeOSD, "", "").count("HoodBreasts") == hoodUsesRobeOSD);
	CHECK(filtered.SetsUsingNif(robeNif, "", "").size() == full.SetsUsingNif(robeNif, "", "").size());

	// A name that's escaped in XML can't be looked for in the text, so every set is loaded
	SliderSetIndex escaped(projectPath, {"Robe & Cape.nif"});
	CHECK(escaped.Entries().size() == 4);
	CHECK(escaped.Covers({"Anything.osd"}));

	CHECK(filtered.Covers({"robe.NIF", "Robe.osd"}));
	CHECK_FALSE(filtered.Covers({"Robe.nif", "Hood.osd"}));

	auto dataSets = filtered.SetsUsingDataFile(robeOSD, folder.SetFile("Robe.osp"), "Robe");
	REQUIRE(dataSets.size() == hoodUsesRobeOSD);
	if (!dataSets.empty())
		CHECK(dataSets[0]->setName == "Hood");
}

TEST_CASE("The save index loads on a thread of its own", "[SliderSet]") {
	ProjectFolder folder;
	const std::string projectPath = folder.path.u8string();
	SaveSets(folder.SetFile("Robe.osp"), {MakeBase()});

	SliderSetIndexTask task(projectPath, {"Robe.nif", "Robe.osd"});
	for (int i = 0; i < 500 && !task.IsDone(); i++)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(task.IsDone());
	REQUIRE(task.Index().Entries().size() == 1);
	CHECK(task.Index().Entries()[0].setName == "Robe");

	// A cancelled task isn't done, unless it was before
	SliderSetIndexTask cancelled(projectPath, {"Robe.nif", "Robe.osd"});
	cancelled.Cancel();
	if (!cancelled.IsDone())
		CHECK(cancelled.Index().Entries().size() <= 1);
	task.Cancel();
	CHECK(task.IsDone());
}

TEST_CASE("Toggles keep their value and the sliders they set", "[SliderSet]") {
	ProjectFolder folder;
	SliderSet base = MakeBase();

	auto& pushUp = AddSlider(base, "PushUp");
	pushUp.bToggle = true;
	pushUp.toggleValue = 50.0f;
	pushUp.toggleSliders = {{"Breasts", 30.0f}, {"Butt", 100.0f}};
	pushUp.AddDataFile("Robe", "RobePushUp", DataFile("Robe.osd", "RobePushUp"));

	// Only sets other sliders, without data of its own
	auto& curvy = AddSlider(base, "Curvy");
	curvy.bToggle = true;
	curvy.toggleSliders = {{"Breasts", 80.0f}};

	// Without other sliders and data yet, as a new toggle that Outfit Studio saves
	auto& empty = AddSlider(base, "Empty");
	empty.bToggle = true;

	// A slider without data isn't kept
	AddSlider(base, "NoData");

	// A zap can't be a toggle
	base["ZapBelt"].bToggle = true;

	SaveSets(folder.SetFile("Robe.osp"), {base});
	SliderSet loaded = LoadSet(folder.SetFile("Robe.osp"), "Robe");

	CHECK(SliderNames(loaded) == std::vector<std::string>{"Breasts", "Butt", "ZapBelt", "PushUp", "Curvy", "Empty"});
	CHECK(loaded["Empty"].bToggle);
	CHECK(loaded["Empty"].toggleSliders.empty());
	CHECK(loaded["PushUp"].bToggle);
	CHECK(loaded["PushUp"].toggleValue == 50.0f);
	CHECK(loaded["PushUp"].toggleSliders == std::vector<ToggleTarget>{{"Breasts", 30.0f}, {"Butt", 100.0f}});
	CHECK(loaded["Curvy"].bToggle);
	CHECK(loaded["Curvy"].toggleValue == 100.0f);
	CHECK(loaded["Curvy"].dataFiles.empty());
	CHECK_FALSE(loaded["ZapBelt"].bToggle);
	CHECK_FALSE(loaded["Breasts"].bToggle);

	SECTION("without the sliders that are deleted") {
		loaded.DeleteSlider("Breasts");
		CHECK(loaded["PushUp"].toggleSliders == std::vector<ToggleTarget>{{"Butt", 100.0f}});
		CHECK(loaded["Curvy"].toggleSliders.empty());
	}

	SECTION("in variants, without the sliders they removed") {
		SliderSet variant = MakeVariant();
		variant.AddRemovedSlider("Butt");
		variant.DeleteSlider("Butt");
		SaveSets(folder.SetFile("Robe Variant.osp"), {variant});

		SliderSet loadedVariant = LoadSet(folder.SetFile("Robe Variant.osp"), "Robe Variant");
		REQUIRE(loadedVariant.SliderExists("PushUp"));
		REQUIRE(loadedVariant.SliderExists("Curvy"));
		REQUIRE(loadedVariant.SliderExists("Empty"));
		CHECK(loadedVariant["PushUp"].bInherited);
		CHECK(loadedVariant["PushUp"].toggleSliders == std::vector<ToggleTarget>{{"Breasts", 30.0f}});
		CHECK(loadedVariant["Curvy"].toggleSliders == std::vector<ToggleTarget>{{"Breasts", 80.0f}});
	}
}

TEST_CASE("Ticked toggles apply their value and hold the sliders and zaps they set", "[SliderSet]") {
	auto makeSlider = [](const std::string& name, float value) {
		Slider s;
		s.name = name;
		s.value = value;
		return s;
	};

	Slider pushUp = makeSlider("PushUp", 0.0f);
	pushUp.toggle = true;
	pushUp.toggleValue = 0.5f;
	pushUp.toggleSliders = {{"Breasts", 0.3f}, {"ZapBelt", 1.0f}, {"Curvy", 1.0f}, {"Clamp", 1.0f}, {"Missing", 1.0f}};

	// Holds ZapBelt off
	Slider curvy = makeSlider("Curvy", 0.0f);
	curvy.toggle = true;
	curvy.toggleSliders = {{"Breasts", 0.8f}, {"Butt", 0.6f}, {"ZapBelt", 0.0f}};

	Slider zapBelt = makeSlider("ZapBelt", 1.0f);
	zapBelt.zap = true;

	Slider clamp = makeSlider("Clamp", 0.0f);
	clamp.clamp = true;

	std::vector<Slider> sliders{makeSlider("Breasts", 0.2f), makeSlider("Butt", 0.4f), zapBelt, pushUp, curvy, clamp};
	std::vector<int> forcedBy;

	SECTION("unticked toggles are off and leave the other sliders alone") {
		CHECK(ToggledSliderValues(sliders, &forcedBy) == std::vector<float>{0.2f, 0.4f, 1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(forcedBy == std::vector<int>{-1, -1, -1, -1, -1, -1});
	}

	SECTION("a ticked toggle sets sliders and zaps, but not other toggles or clamps") {
		sliders[2].value = 0.0f;
		sliders[3].value = 1.0f;
		CHECK(ToggledSliderValues(sliders, &forcedBy) == std::vector<float>{0.3f, 0.4f, 1.0f, 0.5f, 0.0f, 0.0f});
		CHECK(forcedBy == std::vector<int>{3, -1, 3, -1, -1, -1});
	}

	SECTION("the last ticked toggle wins, and can hold a zap off") {
		sliders[3].value = 1.0f;
		sliders[4].value = 1.0f;
		CHECK(ToggledSliderValues(sliders, &forcedBy) == std::vector<float>{0.8f, 0.6f, 0.0f, 0.5f, 1.0f, 0.0f});
		CHECK(forcedBy == std::vector<int>{4, 4, 4, -1, -1, -1});
	}

	SECTION("toggles are ticked by any value of presets") {
		sliders[4].value = 0.25f;
		CHECK(ToggledSliderValues(sliders) == std::vector<float>{0.8f, 0.6f, 0.0f, 0.0f, 1.0f, 0.0f});
	}
}

TEST_CASE("Toggles of the slider manager are on or off", "[SliderSet]") {
	SliderSet set = MakeBase();
	auto& pushUp = AddSlider(set, "PushUp");
	pushUp.bToggle = true;
	pushUp.toggleValue = 50.0f;
	pushUp.toggleSliders = {{"Breasts", 30.0f}};
	pushUp.AddDataFile("Robe", "RobePushUp", DataFile("Robe.osd", "RobePushUp"));

	SliderManager manager;
	manager.AddSlidersInSet(set);
	manager.InitializeSliders();

	manager.SetSlider("PushUp", false, 0.7f);
	CHECK(manager.GetSlider("PushUp", false) == 1.0f);
	manager.SetSlider("PushUp", true, 0.0f);
	CHECK(manager.GetSlider("PushUp", true) == 0.0f);

	// Ticking a toggle changes both weights, even at the default value
	manager.SetChanged("PushUp", false);
	CHECK(manager.SliderHasChanged("PushUp", true));
	CHECK(manager.SliderHasChanged("PushUp", false));

	auto& slider = manager.slidersBig[3];
	REQUIRE(slider.name == "PushUp");
	CHECK(slider.toggle);
	CHECK(slider.toggleValue == 0.5f);
	CHECK(slider.toggleSliders == std::vector<std::pair<std::string, float>>{{"Breasts", 0.3f}});
	CHECK(ToggledSliderValues(manager.slidersBig)[0] == 0.3f);
}
