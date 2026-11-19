#include "project/ProjectPacker.h"
#include "utils/ConfigurationManager.h"
#include "utils/PlatformUtil.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

namespace {
// A project folder with two projects (a robe and a variant of it) and an output folder, set as the project path
struct PackFolder {
	std::filesystem::path root;
	std::string previousProjectPath;

	PackFolder() {
		std::random_device random;
		root = std::filesystem::temp_directory_path() / ("BSOSTests-Pack-" + std::to_string(random()));
		std::filesystem::create_directories(root / "Projects" / "SliderSets");
		std::filesystem::create_directories(root / "Projects" / "ShapeData" / "Robe");
		std::filesystem::create_directories(root / "Out");
		Write("Projects/ShapeData/Robe/Robe.nif", "nif");
		Write("Projects/ShapeData/Robe/Robe.osd", "osd");
		Write("Projects/ShapeData/Robe/Variant.osd", "variant osd");
		Write("Group.xml", "<SliderGroups/>");

		previousProjectPath = Config["ProjectPath"];
		Config.SetValue("ProjectPath", (root / "Projects").u8string());
	}

	~PackFolder() {
		Config.SetValue("ProjectPath", previousProjectPath);
		std::error_code error;
		std::filesystem::remove_all(root, error);
	}

	void Write(const std::string& file, const std::string& text) const { std::ofstream(root / file, std::ios::binary) << text; }

	static std::string Read(const std::filesystem::path& file) {
		std::ifstream in(file, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(in), {});
	}
};

SliderSet MakeSet(const std::string& name, const std::string& osd) {
	SliderSet set;
	set.SetName(name);
	set.SetDataFolder("Robe");
	set.SetInputFile("Robe.nif");
	set.SetOutputPath("meshes");
	set.SetOutputFile(name);
	set.AddShapeTarget("Robe", "Robe");
	set[set.CreateSlider("Breasts")].AddDataFile("Robe", "RobeBreasts", osd + PathSepStr + "RobeBreasts");
	return set;
}

std::map<std::string, SliderSet> Projects() {
	std::map<std::string, SliderSet> projects;
	projects["Robe"] = MakeSet("Robe", "Robe.osd");

	SliderSet variant = MakeSet("Robe Variant", "Variant.osd");
	variant.SetBaseProject("SliderSets" + std::string(PathSepStr) + "Robe.osp", "Robe", "robe-id");
	projects["Robe Variant"] = variant;

	SliderSet orphan = MakeSet("Robe Orphan", "Variant.osd");
	orphan.SetBaseProject("SliderSets" + std::string(PathSepStr) + "Other.osp", "Other Robe", "other-id");
	projects["Robe Orphan"] = orphan;
	return projects;
}

const std::string Sep = PathSepStr;
const std::string BaseDir = "CalienteTools" + Sep + "BodySlide";
} // namespace

TEST_CASE("Packing projects to a folder copies their files and merges the projects", "[ProjectPacker]") {
	PackFolder folder;
	auto projects = Projects();

	FolderPackTarget target(wxString::FromUTF8((folder.root / "Out").u8string()));
	PackResult result = PackProjects(projects, {"Robe", "Robe Variant"}, "Merged.osp", wxString::FromUTF8((folder.root / "Group.xml").u8string()), BaseDir, target);
	REQUIRE(result.error.empty());
	CHECK(result.missingBaseProjects.empty());

	auto out = folder.root / "Out" / "CalienteTools" / "BodySlide";
	CHECK(PackFolder::Read(out / "ShapeData" / "Robe" / "Robe.nif") == "nif");
	CHECK(PackFolder::Read(out / "ShapeData" / "Robe" / "Robe.osd") == "osd");
	CHECK(PackFolder::Read(out / "ShapeData" / "Robe" / "Variant.osd") == "variant osd");
	CHECK(PackFolder::Read(out / "SliderGroups" / "Group.xml") == "<SliderGroups/>");

	// The variant's base project is packed too, so it refers to the merged file
	SliderSetFile merged((out / "SliderSets" / "Merged.osp").u8string());
	REQUIRE_FALSE(merged.fail());
	std::vector<std::string> names;
	merged.GetSetNamesUnsorted(names, false);
	CHECK(std::set<std::string>(names.begin(), names.end()) == std::set<std::string>{"Robe", "Robe Variant"});

	SliderSet variant;
	REQUIRE(merged.GetSet("Robe Variant", variant) == 0);
	CHECK(variant.GetBaseProjectFile() == "SliderSets" + Sep + "Merged.osp");
}

TEST_CASE("Packing projects to an archive writes the same files", "[ProjectPacker]") {
	PackFolder folder;
	auto projects = Projects();
	auto zipFile = folder.root / "Out" / "Pack.zip";

	PackResult result;
	{
		ZipPackTarget target(wxString::FromUTF8(zipFile.u8string()));
		result = PackProjects(projects, {"Robe", "Robe Orphan"}, "Merged.osp", wxEmptyString, BaseDir, target);
	}
	REQUIRE(result.error.empty());
	// The orphan's base project isn't packed and has to be installed
	CHECK(result.missingBaseProjects == std::set<std::string>{"Other Robe"});

	wxFFileInputStream in(wxString::FromUTF8(zipFile.u8string()));
	REQUIRE(in.IsOk());
	wxZipInputStream zip(in);
	std::set<std::string> entries;
	while (std::unique_ptr<wxZipEntry> entry{zip.GetNextEntry()})
		entries.insert(std::string{entry->GetName(wxPATH_UNIX).ToUTF8()});

	CHECK(entries
		  == std::set<std::string>{"CalienteTools/BodySlide/ShapeData/Robe/Robe.nif",
								   "CalienteTools/BodySlide/ShapeData/Robe/Robe.osd",
								   "CalienteTools/BodySlide/ShapeData/Robe/Variant.osd",
								   "CalienteTools/BodySlide/SliderSets/Merged.osp"});
}

TEST_CASE("Packing stops at a missing input file", "[ProjectPacker]") {
	PackFolder folder;
	auto projects = Projects();
	projects["Robe"].SetInputFile("Missing.nif");

	FolderPackTarget target(wxString::FromUTF8((folder.root / "Out").u8string()));
	PackResult result = PackProjects(projects, {"Robe"}, "Merged.osp", wxEmptyString, BaseDir, target);
	CHECK(result.error.Contains("Missing.nif"));
	CHECK_FALSE(std::filesystem::exists(folder.root / "Out" / "CalienteTools" / "BodySlide" / "SliderSets" / "Merged.osp"));
}
