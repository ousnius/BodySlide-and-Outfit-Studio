#include "../src/components/AddOnSliders.h"
#include "../src/components/SliderSet.h"
#include "../src/components/SliderSetIndex.h"
#include "../src/utils/StringStuff.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

using nifly::NifFile;
using nifly::NiShape;
using nifly::Triangle;
using nifly::Vector2;
using nifly::Vector3;

namespace {
// Project folder with "SliderSets" and "ShapeData" folders of its own for each test
struct ProjectFolder {
	std::filesystem::path path;

	ProjectFolder() {
		std::random_device random;
		path = std::filesystem::temp_directory_path() / ("BSOSTests-" + std::to_string(random()));
		std::filesystem::create_directories(path / "SliderSets");
		std::filesystem::create_directories(path / "ShapeData");
	}

	~ProjectFolder() {
		std::error_code error;
		std::filesystem::remove_all(path, error);
	}

	std::string Path() const { return path.u8string(); }
	std::string SetFile(const std::string& fileName) const { return (path / "SliderSets" / fileName).u8string(); }
	std::string RefTemplateFile(const std::string& fileName) const {
		std::filesystem::create_directories(path / "RefTemplates");
		return (path / "RefTemplates" / fileName).u8string();
	}
	std::string ShapeDataPath() const { return (path / "ShapeData").u8string(); }
	std::filesystem::path DataFolder(const std::string& folder) const {
		auto dataFolder = path / "ShapeData" / folder;
		std::filesystem::create_directories(dataFolder);
		return dataFolder;
	}
};

std::string DataFile(const std::string& osdName, const std::string& dataName) {
	return osdName + PathSepStr + dataName;
}

SliderSet MakeSet(const std::string& name, const std::string& dataFolder, const std::string& nif, const std::vector<std::string>& shapes, const std::string& outputFile = "") {
	SliderSet set;
	set.SetName(name);
	set.SetDataFolder(dataFolder);
	set.SetInputFile(nif);
	set.SetOutputPath("meshes");
	set.SetOutputFile(outputFile.empty() ? name : outputFile);
	for (auto& shape : shapes)
		set.AddShapeTarget(shape, shape);
	return set;
}

SliderData& AddSlider(SliderSet& set, const std::string& sliderName) {
	return set[set.CreateSlider(sliderName)];
}

// An add-on of 'base' that writes the same file from the same NIF
SliderSet MakeAddOn(const std::string& name, const SliderSet& base, const std::string& baseID, const std::string& outputFile = "") {
	std::vector<std::string> shapes;
	for (auto shape = base.ShapesBegin(); shape != base.ShapesEnd(); ++shape)
		shapes.push_back(shape->first);

	SliderSet addOn = MakeSet(name, base.GetDefaultDataFolder(), base.GetInputFile(), shapes, outputFile.empty() ? base.GetName() : outputFile);
	addOn.SetAddOn(true);
	addOn.SetSliderDataFile(name + ".osd");
	addOn.SetBaseProject("", base.GetName(), baseID);
	addOn.SetProjectID(SliderSet::GenerateProjectID());
	return addOn;
}

void SaveSets(const std::string& fileName, std::vector<SliderSet> sets) {
	SliderSetFile file;
	file.New(fileName);
	for (auto& set : sets)
		file.UpdateSet(set);
	REQUIRE(file.Save());
}

SliderSet LoadSet(const std::string& fileName, const std::string& setName, const std::string& baseDataPath = "") {
	SliderSetFile file(fileName);
	REQUIRE_FALSE(file.fail());

	SliderSet set;
	REQUIRE(file.GetSet(setName, set) == 0);
	if (!baseDataPath.empty())
		set.SetBaseDataPath(baseDataPath);
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

// Robe: shapes Robe and Cape, sliders Breasts and Butt
SliderSet MakeRobe() {
	SliderSet base = MakeSet("Robe", "Robe", "Robe.nif", {"Robe", "Cape"});
	base.SetProjectID("5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c");
	AddSlider(base, "Breasts").AddDataFile("Robe", "RobeBreasts", DataFile("Robe.osd", "RobeBreasts"));
	AddSlider(base, "Butt").AddDataFile("Robe", "RobeButt", DataFile("Robe.osd", "RobeButt"));
	return base;
}

ProjectCatalog LoadCatalog(const ProjectFolder& folder) {
	ProjectCatalog catalog;
	catalog.Load(folder.Path());
	return catalog;
}

// Flat grid in the XY plane at height z, (n + 1) x (n + 1) vertices spanning 0 to 10
std::string WriteGridNif(const std::filesystem::path& dir, const std::string& fileName, const std::string& shapeName, int n, float z) {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	for (int y = 0; y <= n; y++) {
		for (int x = 0; x <= n; x++) {
			verts.emplace_back(x * 10.0f / n, y * 10.0f / n, z);
			uvs.emplace_back(static_cast<float>(x) / n, static_cast<float>(y) / n);
		}
	}

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			uint16_t i = static_cast<uint16_t>(y * (n + 1) + x);
			tris.emplace_back(i, static_cast<uint16_t>(i + 1), static_cast<uint16_t>(i + n + 1));
			tris.emplace_back(static_cast<uint16_t>(i + 1), static_cast<uint16_t>(i + n + 2), static_cast<uint16_t>(i + n + 1));
		}
	}

	NifFile nif;
	nif.Create(nifly::NiVersion::getSSE());
	nif.CreateShapeFromData(shapeName, &verts, &tris, &uvs);

	auto path = dir / fileName;
	REQUIRE(nif.Save(path) == 0);
	return path.u8string();
}

// Writes data sets of the same target to an .osd file
void WriteOSD(const std::filesystem::path& file, const std::string& target, const std::map<std::string, TargetDataDiffs>& data) {
	DiffDataSets sets;
	std::map<std::string, std::string> names;
	for (auto& d : data) {
		sets.LoadSet(d.first, target, d.second);
		names[d.first] = target;
	}
	REQUIRE(sets.SaveData({{file.u8string(), names}}));
}

TargetDataDiffs Lift(size_t vertexCount, float z) {
	TargetDataDiffs diffs;
	for (uint16_t i = 0; i < vertexCount; i++)
		diffs[i] = Vector3(0.0f, 0.0f, z);
	return diffs;
}

bool LoadNif(const std::string& fileName, NifFile& nif) {
	std::fstream file(std::filesystem::u8path(fileName), std::ios::in | std::ios::binary);
	return nif.Load(file) == 0;
}
} // namespace

TEST_CASE("Add-ons that build the base project's output stack into it", "[AddOnSliders]") {
	ProjectFolder folder;
	SliderSet robe = MakeRobe();

	// New slider, and new data for a slider of the base project
	SliderSet extra = MakeAddOn("Robe Extra", robe, robe.GetProjectID());
	AddSlider(extra, "Extra").AddDataFile("Robe", "RobeExtra", DataFile("Robe Extra.osd", "RobeExtra"));
	AddSlider(extra, "Butt").AddDataFile("Robe", "RobeButtExtra", DataFile("Robe Extra.osd", "RobeButtExtra"));
	extra.AddRemovedSlider("Breasts");

	// Builds a file of its own
	SliderSet elsewhere = MakeAddOn("Robe Elsewhere", robe, robe.GetProjectID(), "RobeElsewhere");
	AddSlider(elsewhere, "Elsewhere").AddDataFile("Robe", "RobeElsewhere", DataFile("Robe Elsewhere.osd", "RobeElsewhere"));

	// An add-on of the add-on
	SliderSet more = MakeAddOn("Robe More", extra, extra.GetProjectID(), "Robe");
	AddSlider(more, "More").AddDataFile("Cape", "CapeMore", DataFile("Robe More.osd", "CapeMore"));

	SaveSets(folder.SetFile("Robe.osp"), {robe});
	SaveSets(folder.SetFile("Robe Extra.osp"), {extra, elsewhere, more});

	ProjectCatalog catalog = LoadCatalog(folder);
	CHECK(catalog.IsStackedAddOn("Robe Extra"));
	CHECK(catalog.IsStackedAddOn("robe more"));
	CHECK_FALSE(catalog.IsStackedAddOn("Robe Elsewhere"));
	CHECK_FALSE(catalog.IsStackedAddOn("Robe"));
	CHECK(catalog.StackRoot("Robe More") == "Robe");
	CHECK(catalog.StackRoot("Robe Elsewhere") == "Robe Elsewhere");

	SliderSet set = LoadSet(folder.SetFile("Robe.osp"), "Robe");
	std::vector<std::string> warnings;
	auto stacked = StackAddOns(set, catalog, warnings);

	REQUIRE(stacked.size() == 2);
	CHECK(stacked[0].setName == "Robe Extra");
	CHECK(stacked[1].setName == "Robe More");

	// Sliders that an add-on leaves out stay
	CHECK(SliderNames(set) == std::vector<std::string>{"Breasts", "Butt", "Extra", "More"});
	CHECK_FALSE(warnings.empty());

	CHECK(set["Extra"].stackedFrom == "Robe Extra");
	CHECK(set["More"].stackedFrom == "Robe More");
	CHECK(set["Butt"].stackedFrom.empty());
	CHECK(FindData(set, "Butt", "Robe")->dataName == "RobeButtExtra");
	CHECK(FindData(set, "More", "Cape")->fileName == DataFile("Robe More.osd", "CapeMore"));

	// A copy of the project file that kept the project ID isn't the project the add-ons are of
	CHECK(catalog.AddOnsOf("Robe", robe.GetProjectID(), true).size() == 2);
	CHECK(catalog.AddOnsOf("Robe Renamed", robe.GetProjectID(), true).size() == 2);
	SliderSet copy = MakeRobe();
	copy.SetName("Robe Copy");
	SaveSets(folder.SetFile("Robe Copy.osp"), {copy});
	ProjectCatalog withCopy = LoadCatalog(folder);
	CHECK(withCopy.AddOnsOf("Robe Copy", robe.GetProjectID(), true).empty());

	// All add-ons of a body add sliders to its shapes, even ones that build elsewhere
	SliderSet body = LoadSet(folder.SetFile("Robe.osp"), "Robe");
	StackAddOns(body, catalog, warnings, "", "", false);
	CHECK(body.SliderExists("Elsewhere"));
}

TEST_CASE("Sliders of add-ons that aren't installed are left out", "[AddOnSliders]") {
	ProjectFolder folder;
	SliderSet robe = MakeRobe();
	SliderSet extra = MakeAddOn("Robe Extra", robe, robe.GetProjectID());
	SaveSets(folder.SetFile("Robe.osp"), {robe, extra});

	SliderSet outfit = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	auto& installed = AddSlider(outfit, "Extra");
	installed.addOnName = "Robe Extra";
	installed.addOnID = extra.GetProjectID();
	installed.AddDataFile("Dress", "DressExtra", DataFile("Dress.osd", "DressExtra"));

	auto& missing = AddSlider(outfit, "Other");
	missing.addOnName = "Other Add-on";
	missing.AddDataFile("Dress", "DressOther", DataFile("Dress.osd", "DressOther"));

	auto& zap = AddSlider(outfit, "ZapSleeves");
	zap.bZap = true;
	zap.zapToggles = {"Other", "Extra"};
	zap.AddDataFile("Dress", "DressZapSleeves", DataFile("Dress.osd", "DressZapSleeves"));

	// The add-on is kept in the project file
	SaveSets(folder.SetFile("Dress.osp"), {outfit});
	SliderSet loaded = LoadSet(folder.SetFile("Dress.osp"), "Dress");
	CHECK(loaded["Extra"].addOnName == "Robe Extra");
	CHECK(loaded["Extra"].addOnID == extra.GetProjectID());
	CHECK(loaded["Other"].addOnName == "Other Add-on");

	ProjectCatalog catalog = LoadCatalog(folder);
	auto missingAddOns = RemoveUninstalledAddOnSliders(loaded, catalog);
	CHECK(missingAddOns == std::vector<std::string>{"Other Add-on"});
	CHECK(SliderNames(loaded) == std::vector<std::string>{"Extra", "ZapSleeves"});
	CHECK(loaded["ZapSleeves"].zapToggles == std::vector<std::string>{"Extra"});

	// An add-on that was renamed is still found by its ID
	SliderSet renamed = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	auto& byID = AddSlider(renamed, "Extra");
	byID.addOnName = "Old Name";
	byID.addOnID = extra.GetProjectID();
	byID.AddDataFile("Dress", "DressExtra", DataFile("Dress.osd", "DressExtra"));
	CHECK(RemoveUninstalledAddOnSliders(renamed, catalog).empty());
}

TEST_CASE("Generated slider data is loaded but never saved", "[AddOnSliders]") {
	SliderSet set = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	auto& own = AddSlider(set, "Own");
	own.AddDataFile("Dress", "DressOwn", DataFile("Dress.osd", "DressOwn"));

	auto& conformed = AddSlider(set, "Conformed");
	conformed.AddDataFile("Dress", "DressConformed", "", false);
	conformed.dataFiles.back().bGenerated = true;
	conformed.bAutoConformed = true;

	auto generated = std::make_shared<GeneratedSliderData>();
	generated->data["DressConformed"] = {"Dress", Lift(4, 2.0f)};
	set.SetGeneratedData(generated);

	DiffDataSets diffs;
	set.LoadSetDiffData(diffs);
	TargetDataDiffs* loaded = diffs.GetDiffSet("DressConformed");
	REQUIRE(loaded);
	CHECK(loaded->size() == 4);
	CHECK(loaded->at(3).z == Catch::Approx(2.0f));

	// Copies of the set share the data
	SliderSet copy = set;
	DiffDataSets copyDiffs;
	copy.LoadSetDiffData(copyDiffs);
	CHECK(copyDiffs.GetDiffSet("DressConformed"));

	XMLDocument doc;
	XMLElement* root = doc.NewElement("SliderSetInfo");
	doc.InsertEndChild(root);
	XMLElement* element = root->InsertNewChildElement("SliderSet");
	set.WriteSliderSet(element);

	std::vector<std::string> written;
	for (XMLElement* slider = element->FirstChildElement("Slider"); slider; slider = slider->NextSiblingElement("Slider"))
		written.push_back(slider->Attribute("name"));
	CHECK(written == std::vector<std::string>{"Own"});
}

TEST_CASE("Variants get the new sliders of their base project's add-ons", "[AddOnSliders]") {
	ProjectFolder folder;
	SliderSet robe = MakeRobe();
	SliderSet extra = MakeAddOn("Robe Extra", robe, robe.GetProjectID());
	AddSlider(extra, "Extra").AddDataFile("Robe", "RobeExtra", DataFile("Robe Extra.osd", "RobeExtra"));
	AddSlider(extra, "Butt").AddDataFile("Robe", "RobeButtExtra", DataFile("Robe Extra.osd", "RobeButtExtra"));
	extra.SetShapeVertexCount("Robe", 121);

	SliderSet variant = MakeSet("Robe Long", "Robe Long", "Robe Long.nif", {"Robe", "Cape"}, "Robe");
	variant.SetBaseProject("", "Robe", robe.GetProjectID());
	AddSlider(variant, "Butt").AddDataFile("Robe", "RobeButtLong", DataFile("Robe Long.osd", "RobeButtLong"));

	SaveSets(folder.SetFile("Robe.osp"), {robe, extra, variant});
	ProjectCatalog catalog = LoadCatalog(folder);

	SliderSet set = LoadSet(folder.SetFile("Robe.osp"), "Robe Long", folder.ShapeDataPath());
	std::vector<std::string> warnings;
	SetStackResult stack = StackSetAddOns(set, catalog, warnings);
	CHECK(stack.addOns.empty());
	REQUIRE(stack.baseAddOns.size() == 1);
	CHECK(stack.baseAddOns[0].vertexCounts.at("Robe") == 121);

	// The variant keeps its own data of sliders it has
	CHECK(set["Extra"].stackedFrom == "Robe Extra");
	CHECK(FindData(set, "Butt", "Robe")->dataName == "RobeButtLong");

	// The add-on's local data is in the base project's data folder
	const DiffInfo* extraData = FindData(set, "Extra", "Robe");
	REQUIRE(extraData);
	CHECK_FALSE(extraData->bLocal);
	auto folders = set.GetTargetDataFolders("Robe");
	CHECK(std::find(folders.begin(), folders.end(), "Robe") != folders.end());

	// A shape with the vertices the add-on was made for keeps the data, others leave it out without conforming
	NifFile sameNif;
	LoadNif(WriteGridNif(folder.DataFolder("Robe Long"), "Same.nif", "Robe", 10, 0.0f), sameNif);
	SliderSet same = set;
	FitBaseAddOnsToVariant(same, sameNif, nullptr, stack.baseAddOns, catalog, folder.ShapeDataPath(), LoadNif, false, warnings);
	CHECK(FindData(same, "Extra", "Robe"));

	NifFile editedNif;
	LoadNif(WriteGridNif(folder.DataFolder("Robe Long"), "Edited.nif", "Robe", 8, 0.0f), editedNif);
	SliderSet edited = set;
	FitBaseAddOnsToVariant(edited, editedNif, nullptr, stack.baseAddOns, catalog, folder.ShapeDataPath(), LoadNif, false, warnings);
	CHECK_FALSE(edited.SliderExists("Extra"));
}

TEST_CASE("The body of an outfit is found by its reference info or its data folders", "[AddOnSliders]") {
	ProjectFolder folder;
	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	body.SetProjectID(SliderSet::GenerateProjectID());
	AddSlider(body, "Breasts").AddDataFile("BodyShape", "BodyShapeBreasts", DataFile("Body.osd", "BodyShapeBreasts"));
	SliderSet bodyExtra = MakeAddOn("Body Extra", body, body.GetProjectID());
	AddSlider(bodyExtra, "Extra").AddDataFile("BodyShape", "BodyShapeExtra", DataFile("Body Extra.osd", "BodyShapeExtra"));
	SaveSets(folder.SetFile("Body.osp"), {body, bodyExtra});

	SliderSet byReference = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	byReference.SetReferenceInfo("SliderSets" + std::string(PathSepStr) + "Body.osp", "Body", "BodyShape");

	SliderSet byFolder = MakeSet("Top", "Top", "Top.nif", {"Top", "BodyShape"});
	byFolder.SetTargetDataFolders("BodyShape", {"Body"});

	SliderSet unrelated = MakeSet("Hat", "Hat", "Hat.nif", {"Hat"});
	SaveSets(folder.SetFile("Outfits.osp"), {byReference, byFolder, unrelated});

	ProjectCatalog catalog = LoadCatalog(folder);

	ConformBodyRef ref = FindConformBody(byReference, catalog, folder.Path());
	REQUIRE(ref.Found());
	CHECK(ref.setName == "Body");
	CHECK(ref.shapeName == "BodyShape");
	CHECK(ref.fromReferenceInfo);

	// Outfits made from an add-on of the body still conform the sliders of all its add-ons
	SliderSet byAddOn = byReference;
	byAddOn.SetReferenceInfo("SliderSets" + std::string(PathSepStr) + "Body.osp", "Body Extra", "BodyShape");
	CHECK(FindConformBody(byAddOn, catalog, folder.Path()).setName == "Body");

	ref = FindConformBody(byFolder, catalog, folder.Path());
	REQUIRE(ref.Found());
	CHECK(ref.setName == "Body");
	CHECK(ref.shapeName == "BodyShape");
	CHECK_FALSE(ref.fromReferenceInfo);

	CHECK_FALSE(FindConformBody(unrelated, catalog, folder.Path()).Found());

	// Not for the body itself or its add-ons
	SliderSet bodySet = LoadSet(folder.SetFile("Body.osp"), "Body");
	CHECK_FALSE(FindConformBody(bodySet, catalog, folder.Path()).Found());
	SliderSet addOnSet = LoadSet(folder.SetFile("Body.osp"), "Body Extra");
	CHECK_FALSE(FindConformBody(addOnSet, catalog, folder.Path()).Found());
}

TEST_CASE("Add-on sliders of the body are conformed to outfits that don't have them", "[AddOnSliders]") {
	ProjectFolder folder;
	auto bodyFolder = folder.DataFolder("Body");
	auto dressFolder = folder.DataFolder("Dress");

	WriteGridNif(bodyFolder, "Body.nif", "BodyShape", 10, 0.0f);
	WriteOSD(bodyFolder / "Body.osd", "BodyShape", {{"BodyShapeBreasts", Lift(121, 1.0f)}});
	WriteOSD(bodyFolder / "Body Extra.osd", "BodyShape", {{"BodyShapeExtra", Lift(121, 2.0f)}, {"BodyShapeHandmade", Lift(121, 3.0f)}});
	std::string dressNif = WriteGridNif(dressFolder, "Dress.nif", "Dress", 6, 0.5f);

	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	body.SetProjectID(SliderSet::GenerateProjectID());
	AddSlider(body, "Breasts").AddDataFile("BodyShape", "BodyShapeBreasts", DataFile("Body.osd", "BodyShapeBreasts"));

	SliderSet bodyExtra = MakeAddOn("Body Extra", body, body.GetProjectID());
	auto& extraSlider = AddSlider(bodyExtra, "Extra");
	extraSlider.defBigValue = 50.0f;
	extraSlider.AddDataFile("BodyShape", "BodyShapeExtra", DataFile("Body Extra.osd", "BodyShapeExtra"));
	AddSlider(bodyExtra, "Handmade").AddDataFile("BodyShape", "BodyShapeHandmade", DataFile("Body Extra.osd", "BodyShapeHandmade"));
	SaveSets(folder.SetFile("Body.osp"), {body, bodyExtra});

	// The outfit already has a handcrafted version of one add-on slider
	SliderSet dress = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	dress.SetReferenceInfo("SliderSets" + std::string(PathSepStr) + "Body.osp", "Body", "BodyShape");
	AddSlider(dress, "Breasts").AddDataFile("Dress", "DressBreasts", DataFile("Dress.osd", "DressBreasts"));
	auto& handmade = AddSlider(dress, "Handmade");
	handmade.addOnName = "Body Extra";
	handmade.AddDataFile("Dress", "DressHandmade", DataFile("Dress.osd", "DressHandmade"));
	dress.SetBaseDataPath(folder.ShapeDataPath());

	ProjectCatalog catalog = LoadCatalog(folder);
	ConformBodyRef ref = FindConformBody(dress, catalog, folder.Path());
	REQUIRE(ref.Found());

	ConformBody conformBody;
	std::vector<std::string> warnings;
	REQUIRE(conformBody.Load(ref, catalog, folder.ShapeDataPath(), LoadNif, warnings));
	CHECK(conformBody.addOnNames == std::vector<std::string>{"Body Extra"});
	REQUIRE(conformBody.sliders.size() == 2);

	NifFile nif;
	REQUIRE(LoadNif(dressNif, nif));
	AnimInfo anim;
	LoadConformAnim(anim, nif);

	auto conformed = ConformAddOnSliders(dress, nif, anim, conformBody);
	CHECK(conformed == std::vector<std::string>{"Body Extra"});
	CHECK(SliderNames(dress) == std::vector<std::string>{"Breasts", "Handmade", "Extra"});

	SliderData& extra = dress["Extra"];
	CHECK(extra.bAutoConformed);
	CHECK(extra.stackedFrom == "Body Extra");
	CHECK(extra.defBigValue == Catch::Approx(50.0f));
	CHECK_FALSE(dress["Handmade"].bAutoConformed);

	// Conformed like Outfit Studio does: the dress follows the body's lift
	DiffDataSets diffs;
	dress.LoadSetDiffData(diffs);
	TargetDataDiffs* extraDiffs = diffs.GetDiffSet(extra.dataFiles.at(0).dataName);
	REQUIRE(extraDiffs);
	CHECK(extraDiffs->size() == nif.FindBlockByName<NiShape>("Dress")->GetNumVertices());
	for (auto& diff : *extraDiffs)
		CHECK(diff.second.z == Catch::Approx(2.0f).margin(1e-4));
}

TEST_CASE("Add-on sliders kept off outfits stay on the body but aren't conformed", "[AddOnSliders]") {
	ProjectFolder folder;
	auto bodyFolder = folder.DataFolder("Body");
	auto dressFolder = folder.DataFolder("Dress");

	WriteGridNif(bodyFolder, "Body.nif", "BodyShape", 10, 0.0f);
	WriteOSD(bodyFolder / "Body Extra.osd", "BodyShape", {{"BodyShapeExtra", Lift(121, 2.0f)}, {"BodyShapeBodyOnly", Lift(121, 3.0f)}});
	std::string dressNif = WriteGridNif(dressFolder, "Dress.nif", "Dress", 6, 0.5f);

	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	body.SetProjectID(SliderSet::GenerateProjectID());

	SliderSet bodyExtra = MakeAddOn("Body Extra", body, body.GetProjectID());
	AddSlider(bodyExtra, "Extra").AddDataFile("BodyShape", "BodyShapeExtra", DataFile("Body Extra.osd", "BodyShapeExtra"));
	auto& bodyOnly = AddSlider(bodyExtra, "Body Only");
	bodyOnly.bNoAutoConform = true;
	bodyOnly.AddDataFile("BodyShape", "BodyShapeBodyOnly", DataFile("Body Extra.osd", "BodyShapeBodyOnly"));
	SaveSets(folder.SetFile("Body.osp"), {body, bodyExtra});

	CHECK(LoadSet(folder.SetFile("Body.osp"), "Body Extra")["Body Only"].bNoAutoConform);
	CHECK_FALSE(LoadSet(folder.SetFile("Body.osp"), "Body Extra")["Extra"].bNoAutoConform);

	ProjectCatalog catalog = LoadCatalog(folder);

	// The body still has the slider with its data
	SliderSet stackedBody = LoadSet(folder.SetFile("Body.osp"), "Body", folder.ShapeDataPath());
	std::vector<std::string> warnings;
	StackSetAddOns(stackedBody, catalog, warnings, true, folder.SetFile("Body.osp"));
	REQUIRE(stackedBody.SliderExists("Body Only"));
	CHECK(stackedBody["Body Only"].bNoAutoConform);
	CHECK(FindData(stackedBody, "Body Only", "BodyShape"));

	SliderSet dress = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	dress.SetReferenceInfo("SliderSets" + std::string(PathSepStr) + "Body.osp", "Body", "BodyShape");
	dress.SetBaseDataPath(folder.ShapeDataPath());

	ConformBodyRef ref = FindConformBody(dress, catalog, folder.Path());
	REQUIRE(ref.Found());

	ConformBody conformBody;
	REQUIRE(conformBody.Load(ref, catalog, folder.ShapeDataPath(), LoadNif, warnings));
	REQUIRE(conformBody.sliders.size() == 1);
	CHECK(conformBody.sliders[0].slider.name == "Extra");

	NifFile nif;
	REQUIRE(LoadNif(dressNif, nif));
	AnimInfo anim;
	LoadConformAnim(anim, nif);

	CHECK(ConformAddOnSliders(dress, nif, anim, conformBody) == std::vector<std::string>{"Body Extra"});
	CHECK(SliderNames(dress) == std::vector<std::string>{"Extra"});
}

TEST_CASE("Add-ons save and load all their base projects", "[AddOnSliders]") {
	ProjectFolder folder;
	const std::string refBody = "RefTemplates" + std::string(PathSepStr) + "Body.osp";
	const std::string refOther = "RefTemplates" + std::string(PathSepStr) + "Body Ref.osp";

	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	SliderSet addOn = MakeAddOn("Body Extra", body, "");
	CHECK(addOn.AddOtherBaseProject({refBody, "Body", ""}));
	CHECK(addOn.AddOtherBaseProject({refOther, "Body Ref", "5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c"}));

	// The first base project and entries already listed aren't added again, in any case and with either slash
	CHECK_FALSE(addOn.AddOtherBaseProject({"", "Body", ""}));
	CHECK_FALSE(addOn.AddOtherBaseProject({"RefTemplates/Body.osp", "body", ""}));
	CHECK(addOn.GetOtherBaseProjects().size() == 2);
	SaveSets(folder.SetFile("Body.osp"), {body, addOn});

	SliderSetFile file(folder.SetFile("Body.osp"));
	CHECK(file.GetSetBaseProjectName("Body Extra") == "Body");
	auto saved = file.GetSetOtherBaseProjects("Body Extra");
	REQUIRE(saved.size() == 2);
	CHECK(saved[0].name == "Body");
	CHECK(saved[0].file == refBody);
	CHECK(saved[1].name == "Body Ref");
	CHECK(saved[1].id == "5a9b3c1e-2f4d-4e6a-8b7c-0d1e2f3a4b5c");

	// Loading with the base project merged in and saving again keeps them
	SliderSet loaded = LoadSet(folder.SetFile("Body.osp"), "Body Extra");
	CHECK(loaded.GetBaseProjectName() == "Body");
	REQUIRE(loaded.GetOtherBaseProjects().size() == 2);
	CHECK(loaded.GetOtherBaseProjects()[1].file == refOther);

	SaveSets(folder.SetFile("Again.osp"), {loaded});
	SliderSetFile again(folder.SetFile("Again.osp"));
	CHECK(again.GetSetOtherBaseProjects("Body Extra").size() == 2);

	loaded.ClearBaseProject();
	CHECK(loaded.GetOtherBaseProjects().empty());
}

TEST_CASE("Add-ons fit exact copies of their base project in other files", "[AddOnSliders]") {
	ProjectFolder folder;
	auto bodyFolder = folder.DataFolder("Body");
	folder.DataFolder("Dress");

	WriteGridNif(bodyFolder, "Body.nif", "BodyShape", 10, 0.0f);
	WriteOSD(bodyFolder / "Body.osd", "BodyShape", {{"BodyShapeBreasts", Lift(121, 1.0f)}});
	WriteOSD(bodyFolder / "Body Extra.osd", "BodyShape", {{"BodyShapeExtra", Lift(121, 2.0f)}});
	std::string dressNif = WriteGridNif(folder.DataFolder("Dress"), "Dress.nif", "Dress", 6, 0.5f);

	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	AddSlider(body, "Breasts").AddDataFile("BodyShape", "BodyShapeBreasts", DataFile("Body.osd", "BodyShapeBreasts"));
	SliderSet bodyExtra = MakeAddOn("Body Extra", body, "");
	AddSlider(bodyExtra, "Extra").AddDataFile("BodyShape", "BodyShapeExtra", DataFile("Body Extra.osd", "BodyShapeExtra"));
	SaveSets(folder.SetFile("Body.osp"), {body, bodyExtra});

	// A copy of the body in RefTemplates, which the add-on doesn't list, and a set of its name with other shapes
	SaveSets(folder.RefTemplateFile("Body.osp"), {body});
	SliderSet otherBody = MakeSet("Body", "Other", "Other.nif", {"OtherShape"});
	SaveSets(folder.RefTemplateFile("Other.osp"), {otherBody});

	ProjectCatalog catalog = LoadCatalog(folder);

	SliderSet copy = LoadSet(folder.RefTemplateFile("Body.osp"), "Body", folder.ShapeDataPath());
	auto matched = catalog.AddOnsOfSet(copy, folder.RefTemplateFile("Body.osp"), false);
	REQUIRE(matched.size() == 1);
	CHECK(matched[0].entry->setName == "Body Extra");
	CHECK_FALSE(matched[0].otherBase);

	SliderSet other = LoadSet(folder.RefTemplateFile("Other.osp"), "Body", folder.ShapeDataPath());
	CHECK(catalog.AddOnsOfSet(other, folder.RefTemplateFile("Other.osp"), false).empty());

	// Outfits made with the copy conform the add-on's sliders from it
	SliderSet dress = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	dress.SetReferenceInfo("RefTemplates" + std::string(PathSepStr) + "Body.osp", "Body", "BodyShape");
	dress.SetBaseDataPath(folder.ShapeDataPath());

	ConformBodyRef ref = FindConformBody(dress, catalog, folder.Path());
	REQUIRE(ref.Found());
	CHECK(SliderSetIndex::SameFile(ref.fileName, folder.RefTemplateFile("Body.osp")));
	CHECK(ref.setName == "Body");
	CHECK(ref.fromReferenceInfo);

	ConformBody conformBody;
	std::vector<std::string> warnings;
	REQUIRE(conformBody.Load(ref, catalog, folder.ShapeDataPath(), LoadNif, warnings));
	CHECK(conformBody.addOnNames == std::vector<std::string>{"Body Extra"});

	NifFile nif;
	REQUIRE(LoadNif(dressNif, nif));
	AnimInfo anim;
	LoadConformAnim(anim, nif);
	CHECK(ConformAddOnSliders(dress, nif, anim, conformBody) == std::vector<std::string>{"Body Extra"});
	CHECK(dress.SliderExists("Extra"));

	// Not for a set of the body's name with other shapes
	SliderSet hat = MakeSet("Hat", "Hat", "Hat.nif", {"Hat"});
	hat.SetReferenceInfo("RefTemplates" + std::string(PathSepStr) + "Other.osp", "Body", "OtherShape");
	CHECK_FALSE(FindConformBody(hat, catalog, folder.Path()).Found());
}

TEST_CASE("Add-ons fit other projects of the body that they list as base projects", "[AddOnSliders]") {
	ProjectFolder folder;
	auto bodyFolder = folder.DataFolder("Body");
	auto refFolder = folder.DataFolder("BodyRef");

	WriteGridNif(bodyFolder, "Body.nif", "BodyShape", 10, 0.0f);
	WriteOSD(bodyFolder / "Body.osd", "BodyShape", {{"BodyShapeBreasts", Lift(121, 1.0f)}});
	WriteOSD(bodyFolder / "Body Extra.osd", "BodyShape", {{"BodyShapeExtra", Lift(121, 2.0f)}, {"BodyShapeBreastsMore", Lift(121, 5.0f)}});
	std::string refNifFile = WriteGridNif(refFolder, "BodyRef.nif", "RefShape", 8, 0.0f);
	WriteOSD(refFolder / "BodyRef.osd", "RefShape", {{"RefShapeBreasts", Lift(81, 1.0f)}});
	std::string dressNif = WriteGridNif(folder.DataFolder("Dress"), "Dress.nif", "Dress", 6, 0.5f);

	SliderSet body = MakeSet("Body", "Body", "Body.nif", {"BodyShape"});
	body.SetProjectID(SliderSet::GenerateProjectID());
	AddSlider(body, "Breasts").AddDataFile("BodyShape", "BodyShapeBreasts", DataFile("Body.osd", "BodyShapeBreasts"));

	// The add-on adds a slider and changes one of the body
	const std::string refFile = "RefTemplates" + std::string(PathSepStr) + "Body Ref.osp";
	SliderSet bodyExtra = MakeAddOn("Body Extra", body, body.GetProjectID());
	bodyExtra.AddOtherBaseProject({refFile, "Body Ref", ""});
	AddSlider(bodyExtra, "Extra").AddDataFile("BodyShape", "BodyShapeExtra", DataFile("Body Extra.osd", "BodyShapeExtra"));
	AddSlider(bodyExtra, "Breasts").AddDataFile("BodyShape", "BodyShapeBreastsMore", DataFile("Body Extra.osd", "BodyShapeBreastsMore"));
	SaveSets(folder.SetFile("Body.osp"), {body, bodyExtra});

	SliderSet bodyRef = MakeSet("Body Ref", "BodyRef", "BodyRef.nif", {"RefShape"});
	AddSlider(bodyRef, "Breasts").AddDataFile("RefShape", "RefShapeBreasts", DataFile("BodyRef.osd", "RefShapeBreasts"));
	SaveSets(folder.RefTemplateFile("Body Ref.osp"), {bodyRef});

	ProjectCatalog catalog = LoadCatalog(folder);

	// BodySlide still builds the add-on with its first base project only
	CHECK(catalog.IsStackedAddOn("Body Extra"));
	CHECK(catalog.AddOnsOf("Body", body.GetProjectID(), true).size() == 1);

	SliderSet refSet = LoadSet(folder.RefTemplateFile("Body Ref.osp"), "Body Ref", folder.ShapeDataPath());
	CHECK(catalog.AddOnsOfSet(refSet, folder.RefTemplateFile("Body Ref.osp"), true).empty());
	auto matched = catalog.AddOnsOfSet(refSet, folder.RefTemplateFile("Body Ref.osp"), false);
	REQUIRE(matched.size() == 1);
	CHECK(matched[0].otherBase);

	// BodySlide adds the sets one by one: the relative file of the other base project is in the project path
	ProjectCatalog added;
	added.SetProjectPath(folder.Path());
	SliderSetFile bodyFile(folder.SetFile("Body.osp"));
	added.Add(bodyFile, "Body");
	added.Add(bodyFile, "Body Extra");
	added.Finish();
	CHECK(added.AddOnsOfSet(refSet, folder.RefTemplateFile("Body Ref.osp"), false).size() == 1);

	// Only its new sliders, which are fitted to the reference's shape
	std::vector<std::string> warnings;
	SetStackResult stack = StackSetAddOns(refSet, catalog, warnings, true, folder.RefTemplateFile("Body Ref.osp"));
	CHECK(stack.addOns.empty());
	REQUIRE(stack.baseAddOns.size() == 1);
	CHECK(stack.baseAddOns[0].otherBase);
	CHECK(SliderNames(refSet) == std::vector<std::string>{"Breasts", "Extra"});
	CHECK(refSet["Breasts"].TargetDataName("RefShape") == "RefShapeBreasts");

	NifFile refNif;
	REQUIRE(LoadNif(refNifFile, refNif));
	AnimInfo refAnim;
	LoadConformAnim(refAnim, refNif);
	FitBaseAddOnsToVariant(refSet, refNif, &refAnim, stack.baseAddOns, catalog, folder.ShapeDataPath(), LoadNif, true, warnings);

	SliderData& extra = refSet["Extra"];
	REQUIRE(extra.dataFiles.size() == 1);
	CHECK(extra.dataFiles[0].targetName == "RefShape");
	CHECK(extra.dataFiles[0].bGenerated);
	CHECK(extra.bAutoConformed);

	DiffDataSets diffs;
	refSet.LoadSetDiffData(diffs, "RefShape");
	TargetDataDiffs* extraDiffs = diffs.GetDiffSet(extra.dataFiles[0].dataName);
	REQUIRE(extraDiffs);
	CHECK(extraDiffs->size() == 81);
	for (auto& diff : *extraDiffs)
		CHECK(diff.second.z == Catch::Approx(2.0f).margin(1e-4));

	// An outfit made with the reference conforms the add-on's sliders from it
	SliderSet dress = MakeSet("Dress", "Dress", "Dress.nif", {"Dress"});
	dress.SetReferenceInfo(refFile, "Body Ref", "RefShape");
	dress.SetBaseDataPath(folder.ShapeDataPath());

	ConformBodyRef ref = FindConformBody(dress, catalog, folder.Path());
	REQUIRE(ref.Found());
	CHECK(ref.setName == "Body Ref");

	ConformBody conformBody;
	REQUIRE(conformBody.Load(ref, catalog, folder.ShapeDataPath(), LoadNif, warnings));
	REQUIRE(conformBody.sliders.size() == 1);
	CHECK(conformBody.sliders[0].slider.name == "Extra");

	NifFile nif;
	REQUIRE(LoadNif(dressNif, nif));
	AnimInfo anim;
	LoadConformAnim(anim, nif);
	CHECK(ConformAddOnSliders(dress, nif, anim, conformBody) == std::vector<std::string>{"Body Extra"});

	DiffDataSets dressDiffs;
	dress.LoadSetDiffData(dressDiffs);
	TargetDataDiffs* dressExtra = dressDiffs.GetDiffSet(dress["Extra"].dataFiles.at(0).dataName);
	REQUIRE(dressExtra);
	for (auto& diff : *dressExtra)
		CHECK(diff.second.z == Catch::Approx(2.0f).margin(1e-4));
}
