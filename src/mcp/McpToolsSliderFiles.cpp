/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/SliderPresets.h"
#include "../program/OutfitStudio.h"
#include "../utils/ProjectUtil.h"

#include <wx/filename.h>

#include <algorithm>
#include <cmath>
#include <regex>

using nlohmann::json;
using namespace nifly;

namespace {

json Names(const std::vector<NiShape*>& shapes) {
	json names = json::array();
	for (NiShape* shape : shapes)
		names.push_back(shape->name.get());
	return names;
}

size_t SliderIndex(OutfitProject& project, const std::string& name) {
	size_t index = 0;
	if (!project.SliderIndexFromName(name, index))
		throw McpToolError("There is no slider '" + name + "'. list_sliders lists them.");
	return index;
}

std::vector<std::string> ExistingSliders(OutfitProject& project, const json& args, const char* name) {
	std::vector<std::string> sliders = McpArgs::Strings(args, name);
	if (sliders.empty())
		McpArgs::Wrong(name, "a list of slider names");
	for (auto& slider : sliders)
		SliderIndex(project, slider);
	return sliders;
}

void CheckNewSlider(OutfitProject& project, const std::string& name) {
	if (name.empty())
		throw McpToolError("The slider needs a name.");
	if (project.ValidSlider(name))
		throw McpToolError("There already is a slider '" + name + "'.");
}

json SliderProperties(OutfitProject& project, size_t index) {
	json toggles = json::array();
	for (auto& toggle : project.SliderZapToggles(index))
		toggles.push_back(toggle.ToUTF8().data());

	return {
		{"name", project.GetSliderName(index)},
		{"zap", project.SliderZap(index)},
		{"zapToggles", toggles},
		{"invert", project.SliderInvert(index)},
		{"hidden", project.SliderHidden(index)},
		{"uv", project.SliderUV(index)},
		{"defaultSmall", project.SliderDefault(index, false)},
		{"defaultBig", project.SliderDefault(index, true)},
	};
}

McpToolResult SliderNames(OutfitProject& project) {
	std::vector<std::string> sliders;
	project.GetSliderList(sliders);
	McpToolResult result;
	result.data["sliders"] = sliders;
	return result;
}

// The sliders a tool worked on under key, with how many sliders the project has now
McpToolResult SlidersChanged(OutfitProject& project, const char* key, const json& sliders) {
	McpToolResult result;
	result.data[key] = sliders;
	result.data["sliderCount"] = project.SliderCount();
	return result;
}

McpToolResult SetSliderProperties(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	std::string name = McpArgs::String(args, "slider");
	size_t index = SliderIndex(project, name);

	auto percent = [&args](const char* arg) {
		int value = McpArgs::Int(args, arg, -1);
		if (value < 0 || value > 100)
			McpArgs::Wrong(arg, "a whole number from 0 to 100");
		return value;
	};

	if (McpArgs::Has(args, "zap"))
		project.SetSliderZap(index, McpArgs::Bool(args, "zap", false));
	if (McpArgs::Has(args, "zapped")) {
		int value = McpArgs::Bool(args, "zapped", false) ? 100 : 0;
		project.SetSliderDefault(index, value, false);
		project.SetSliderDefault(index, value, true);
	}
	if (McpArgs::Has(args, "zapToggles")) {
		wxArrayString toggles;
		for (auto& toggle : McpArgs::Strings(args, "zapToggles")) {
			SliderIndex(project, toggle);
			toggles.Add(wxString::FromUTF8(toggle));
		}
		project.SetSliderZapToggles(index, toggles);
	}
	if (McpArgs::Has(args, "invert"))
		project.SetSliderInvert(index, McpArgs::Bool(args, "invert", false));
	if (McpArgs::Has(args, "hidden"))
		project.SetSliderHidden(index, McpArgs::Bool(args, "hidden", false));
	if (McpArgs::Has(args, "uv"))
		project.SetSliderUV(index, McpArgs::Bool(args, "uv", false));
	if (McpArgs::Has(args, "defaultSmall"))
		project.SetSliderDefault(index, percent("defaultSmall"), false);
	if (McpArgs::Has(args, "defaultBig"))
		project.SetSliderDefault(index, percent("defaultBig"), true);

	RefreshFlags refresh = RefreshFlags::PendingChanges;
	if (McpArgs::Has(args, "newName")) {
		std::string newName = McpArgs::String(args, "newName");
		if (newName != name) {
			CheckNewSlider(project, newName);
			// The slider panels are named after their slider, so they are made anew
			if (frame->editor.bEditSlider)
				frame->ExitSliderEdit();
			project.SetSliderName(index, newName);
			refresh |= RefreshFlags::SliderSet;
		}
	}

	frame->ApplyRefresh(refresh);

	McpToolResult result;
	result.data["slider"] = SliderProperties(project, index);
	return result;
}

McpToolResult ExportSliderData(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	std::string path = McpFilePath(McpArgs::String(args, "path"));
	auto shapes = mcp.Shapes(args, "shapes", true);
	const bool folder = McpArgs::Bool(args, "folder", false);

	std::string format = McpArgs::String(args, "format", "");
	if (format.empty())
		format = wxFileName(wxString::FromUTF8(path)).GetExt().Lower().ToUTF8().data();
	if (format != "nif" && format != "bsd" && format != "obj" && format != "dat")
		throw McpToolError("The format has to be nif, bsd, obj or dat (Starfield morph.dat). Give it with format or the file extension.");

	std::vector<std::string> sliders;
	if (format != "dat") {
		sliders = McpArgs::Strings(args, "sliders");
		if (sliders.empty() && frame->editor.bEditSlider)
			sliders.push_back(frame->editor.activeSlider);
		if (sliders.empty())
			throw McpToolError("Name the sliders to export, or put a slider in edit mode.");
		for (auto& slider : sliders)
			SliderIndex(project, slider);
	}

	const size_t files = format == "dat" ? shapes.size() : shapes.size() * sliders.size();
	if (files > 1 && !folder)
		throw McpToolError("That's " + std::to_string(files) + " files. Pass folder: true with a folder as path, or one shape and slider.");
	if (folder && !wxFileName::DirExists(wxString::FromUTF8(path)) && !wxFileName::Mkdir(wxString::FromUTF8(path), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
		throw McpToolError("The folder '" + path + "' couldn't be made.");

	// One file per shape and slider, named like the window names them
	auto fileFor = [&](NiShape* shape, const std::string& slider) {
		if (!folder)
			return path;
		std::string name = format == "dat" ? shape->name.get() + "_morph" : shape->name.get() + "_" + slider;
		return path + "/" + name + "." + format;
	};

	json written = json::array();
	json failed = json::array();
	for (NiShape* shape : shapes) {
		std::vector<std::string> shapeSliders = format == "dat" ? std::vector<std::string>{""} : sliders;
		for (auto& slider : shapeSliders) {
			std::string file = fileFor(shape, slider);
			McpCheckOverwrite(file, args);

			bool ok = true;
			if (format == "nif")
				ok = project.SaveSliderNIF(slider, shape, file) == 0;
			else if (format == "bsd")
				ok = project.SaveSliderBSD(slider, shape, file) == 0;
			else if (format == "obj")
				ok = project.SaveSliderOBJ(slider, shape, file, McpArgs::Bool(args, "onlyChanged", false)) == 0;
			else
				ok = project.WriteSFMorphs(shape, file);
			(ok ? written : failed).push_back(file);
		}
	}

	// Writing slider meshes changes the shown state, like in the window
	frame->ApplySliders();

	McpToolResult result;
	result.data["written"] = written;
	if (!failed.empty()) {
		result.data["failed"] = failed;
		result.isError = written.empty();
	}
	return result;
}

McpToolResult SavePreset(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	std::string name = McpArgs::String(args, "name");
	if (name.empty())
		McpArgs::Wrong("name", "a preset name");

	std::string file = McpArgs::String(args, "file");
	if (wxFileName(wxString::FromUTF8(file)).GetPath().empty())
		file = ProjectUtil::GetProjectPath() + "/SliderPresets/" + file;
	else
		file = McpFilePath(file);
	if (wxFileName(wxString::FromUTF8(file)).GetExt().empty())
		file += ".xml";

	// Like the window: the current values of the visible sliders that differ from their default
	PresetCollection presets;
	json saved = json::object();
	std::vector<std::string> sliders;
	project.GetSliderList(sliders);
	for (auto& slider : sliders) {
		size_t index = SliderIndex(project, slider);
		if (project.SliderZap(index) || project.SliderHidden(index))
			continue;

		float value = project.SliderValue(index);
		if (project.SliderInvert(index))
			value = 1.0f - value;
		if (std::fabs(project.SliderDefault(index, true) / 100.0f - value) < 0.001f)
			continue;

		presets.SetSliderPreset(name, slider, value, -10000.0f);
		saved[slider] = value;
	}

	if (saved.empty())
		throw McpToolError("All sliders have their default value, so there's nothing to save. Set them with set_slider_values first.");

	if (!McpArgs::Bool(args, "overwrite", false) && wxFileName::FileExists(wxString::FromUTF8(file))) {
		PresetCollection existing;
		std::vector<std::string> noGroups, names;
		existing.LoadPresetFile(file, "", noGroups, true, &names);
		if (std::find(names.begin(), names.end(), name) != names.end())
			throw McpToolError("The file already has a preset '" + name + "'. Pass overwrite: true to replace it.");
	}

	std::vector<std::string> groups = McpArgs::Strings(args, "groups");
	int error = presets.SavePreset(file, name, "OutfitStudioFrame", groups);
	if (error)
		throw McpToolError("Saving the preset failed with error " + std::to_string(error) + ".");

	McpToolResult result;
	result.data["file"] = file;
	result.data["preset"] = name;
	result.data["values"] = saved;
	return result;
}

McpToolResult PhysicsData(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	std::string op = McpArgs::String(args, "op");

	if (op == "import") {
		std::string path = McpFilePath(McpArgs::String(args, "path"));
		if (!wxFileName::FileExists(wxString::FromUTF8(path)))
			throw McpToolError("There is no file '" + path + "'.");
		auto block = std::make_unique<BSClothExtraData>();
		if (!block->FromHKX(path))
			throw McpToolError("'" + path + "' couldn't be read as physics data.");
		project.AddClothData(path, std::move(block));
		frame->SetPendingChanges();
	}
	else if (op == "export") {
		std::string source = McpArgs::String(args, "source");
		auto& blocks = project.GetClothData();
		auto it = blocks.find(source);
		if (it == blocks.end())
			throw McpToolError("There is no physics data from '" + source + "'. op list shows the sources.");
		std::string path = McpFilePath(McpArgs::String(args, "path"));
		McpCheckOverwrite(path, args);
		if (!it->second->ToHKX(path))
			throw McpToolError("Writing '" + path + "' failed.");
	}
	else if (op != "list")
		McpArgs::Wrong("op", "list, import or export");

	McpToolResult result;
	json sources = json::array();
	for (auto& block : project.GetClothData())
		sources.push_back(block.first);
	result.data["sources"] = sources;
	return result;
}

} // namespace

void AddSliderFileTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool newSlider;
	newSlider.name = "new_slider";
	newSlider.description =
		"Adds a slider. empty: a slider without data, to fill with set_slider_edit and editing tools or "
		"set_slider_deltas. zap: a zap slider that removes the unmasked vertices of the shapes when it's on, e.g. of the "
		"reference to hide the body under the outfit (mask_ops covered). "
		"combined: a slider that moves the shapes the way the current slider values do together.";
	newSlider.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "New slider name")},
			{"kind", {{"type", "string"}, {"enum", {"empty", "zap", "combined"}}, {"description", "Default empty"}}},
			{"shapes", McpStringList("zap: shapes, the reference included, the selected shapes by default")},
		},
		{"name"});
	newSlider.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		std::string name = McpArgs::String(args, "name");
		CheckNewSlider(project, name);

		std::string kind = McpArgs::String(args, "kind", "empty");
		if (kind == "empty")
			frame->NewSlider(name, true);
		else if (kind == "zap")
			McpRunStep(mcp, {{"type", "NewZapSlider"}, {"TargetMeshes", Names(mcp.Shapes(args, "shapes", true))}, {"SliderName", name}});
		else if (kind == "combined")
			McpRunStep(mcp, {{"type", "NewCombinedSlider"}, {"SliderName", name}});
		else
			McpArgs::Wrong("kind", "empty, zap or combined");

		McpToolResult result;
		result.data["slider"] = SliderProperties(project, SliderIndex(project, name));
		return result;
	};
	mcp.AddTool(std::move(newSlider));

	McpTool clone;
	clone.name = "clone_slider";
	clone.description = "Copies a slider with its data for all shapes under a new name.";
	clone.inputSchema = McpSchema({{"source", McpProp("string", "Slider to copy")}, {"name", McpProp("string", "Name of the copy")}}, {"source", "name"});
	clone.handler = [&mcp](const json& args) {
		OutfitProject& project = mcp.Project();
		std::string source = McpArgs::String(args, "source");
		SliderIndex(project, source);
		std::string name = McpArgs::String(args, "name");
		CheckNewSlider(project, name);
		McpRunStep(mcp, {{"type", "CloneSlider"}, {"SourceSlider", source}, {"SliderName", name}});

		McpToolResult result;
		result.data["slider"] = SliderProperties(project, SliderIndex(project, name));
		return result;
	};
	mcp.AddTool(std::move(clone));

	McpTool negate;
	negate.name = "negate_sliders";
	negate.description = "Reverses the data of sliders on shapes, so they move the other way.";
	negate.inputSchema = McpSchema({{"sliders", McpStringList("Slider names")}, {"shapes", McpStringList("Shapes, the selected outfit shapes by default")}}, {"sliders"});
	negate.handler = [&mcp](const json& args) {
		auto sliders = ExistingSliders(mcp.Project(), args, "sliders");
		auto shapes = mcp.Shapes(args, "shapes");
		McpRunStep(mcp, {{"type", "NegateSlider"}, {"TargetMeshes", Names(shapes)}, {"SliderNames", sliders}});
		return SlidersChanged(mcp.Project(), "negated", sliders);
	};
	mcp.AddTool(std::move(negate));

	McpTool clear;
	clear.name = "clear_slider_data";
	clear.description =
		"Removes the data of sliders from shapes, like Clear Slider Data in the window: with useMask each vertex's data is "
		"multiplied by its mask value, so unmasked vertices lose it, masked ones keep it and a soft mask border keeps a share "
		"(to blend two conversions). The sliders stay. Not in the undo history: take a snapshot_save first to go back.";
	clear.inputSchema = McpSchema({
		{"sliders", McpStringList("Slider names. Default: every slider.")},
		{"shapes", McpStringList("Shapes, the selected outfit shapes by default")},
		{"useMask", McpProp("boolean", "Scale the data by the mask: masked vertices keep it, unmasked lose it. Default true.")},
		{"allowReference", McpProp("boolean", "Also change the reference shape when it is named in shapes. Default false, which refuses it.")},
	});
	clear.handler = [&mcp](const json& args) {
		std::vector<std::string> sliders = McpArgs::Strings(args, "sliders");
		for (auto& slider : sliders)
			SliderIndex(mcp.Project(), slider);
		// Steps can replace the shapes, so only their names are kept
		json shapes = Names(mcp.Shapes(args, "shapes", McpArgs::Bool(args, "allowReference", false)));
		McpRunStep(mcp, {{"type", "ClearSliderData"}, {"TargetMeshes", shapes}, {"SliderNames", sliders}, {"UseMask", McpArgs::Bool(args, "useMask", true)}});
		McpToolResult result = SlidersChanged(mcp.Project(), "cleared", sliders.empty() ? json("all") : json(sliders));
		result.data["shapes"] = shapes;
		return result;
	};
	mcp.AddTool(std::move(clear));

	McpTool deleteSliders;
	deleteSliders.name = "delete_sliders";
	deleteSliders.description =
		"Deletes sliders and their data from the project. notOnReference deletes the morph sliders the reference has no data "
		"for, e.g. the helper sliders a curtain reference left after loading the body again with mergeSliders. Not in the "
		"undo history: take a snapshot_save first to go back.";
	deleteSliders.inputSchema = McpSchema({
		{"sliders", McpStringList("Slider names, or regular expressions with regex")},
		{"regex", McpProp("boolean", "The names are regular expressions. Default false.")},
		{"notOnReference", McpProp("boolean", "Also delete every slider that isn't a zap and has no data on the reference shape. Default false.")},
	});
	deleteSliders.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		const bool regex = McpArgs::Bool(args, "regex", false);
		const bool notOnReference = McpArgs::Bool(args, "notOnReference", false);
		std::vector<std::string> sliders = regex || (notOnReference && McpArgs::Strings(args, "sliders").empty())
			? McpArgs::Strings(args, "sliders")
			: ExistingSliders(project, args, "sliders");
		if (notOnReference) {
			NiShape* reference = project.GetBaseShape();
			if (!reference)
				throw McpToolError("notOnReference needs a reference shape.");
			for (size_t i = 0; i < project.SliderCount(); i++) {
				if (project.SliderZap(i))
					continue;
				TargetDataDiffs* diff = project.GetDiffSet(project.activeSet[i], reference);
				const std::string name = project.GetSliderName(i);
				if ((!diff || diff->empty()) && std::find(sliders.begin(), sliders.end(), name) == sliders.end())
					sliders.push_back(regex ? "^" + std::regex_replace(name, std::regex(R"([.^$|()\[\]{}*+?\\])"), R"(\$&)") + "$" : name);
			}
		}
		if (sliders.empty())
			McpArgs::Wrong("sliders", "a list of slider names, or notOnReference");
		if (frame->editor.bEditSlider)
			frame->ExitSliderEdit();
		std::vector<std::string> before;
		mcp.Project().GetSliderList(before);
		McpRunStep(mcp, {{"type", "DeleteSlider"}, {"SliderName", sliders}, {"Regex", regex}});

		std::vector<std::string> after;
		mcp.Project().GetSliderList(after);
		json deleted = json::array();
		for (auto& name : before)
			if (std::find(after.begin(), after.end(), name) == after.end())
				deleted.push_back(name);
		return SlidersChanged(mcp.Project(), "deleted", deleted);
	};
	mcp.AddTool(std::move(deleteSliders));

	McpTool properties;
	properties.name = "set_slider_properties";
	properties.description =
		"Changes a slider like the Slider Properties dialog: name, zap, zap toggles, invert, hidden, UV and the default "
		"values BodySlide starts at for the small and big body (0 to 100). Only what's given changes.";
	properties.inputSchema = McpSchema(
		{
			{"slider", McpProp("string", "Slider name")},
			{"newName", McpProp("string", "Rename the slider")},
			{"zap", McpProp("boolean", "A zap slider, which removes vertices instead of moving them")},
			{"zapped", McpProp("boolean", "zap: on by default in BodySlide")},
			{"zapToggles", McpStringList("zap: other zap sliders this one turns off")},
			{"invert", McpProp("boolean", "BodySlide shows the slider reversed")},
			{"hidden", McpProp("boolean", "BodySlide hides the slider")},
			{"uv", McpProp("boolean", "The slider changes UVs instead of positions")},
			{"defaultSmall", McpProp("integer", "Default value for the small body, 0 to 100")},
			{"defaultBig", McpProp("integer", "Default value for the big body, 0 to 100")},
		},
		{"slider"});
	properties.handler = [&mcp](const json& args) { return SetSliderProperties(mcp, args); };
	mcp.AddTool(std::move(properties));

	McpTool importData;
	importData.name = "import_slider_data";
	importData.description =
		"Imports slider data. An OSD or TRI file fills the sliders it has data for. A NIF, OBJ, FBX or BSD file with "
		"the same vertex order fills one slider for the matching shapes: the first of sliders, or one named after "
		"the file. fromFolder reads every such file of a folder, NIF/OBJ/FBX/BSD named \"Shape#Slider.ext\". "
		"Missing sliders are added unless mergeOnly.";
	importData.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "File or folder (absolute or relative to the BodySlide folder)")},
			{"fromFolder", McpProp("boolean", "path is a folder. Default false.")},
			{"sliders", McpStringList("Only these sliders, or the slider a mesh file fills")},
			{"mergeOnly", McpProp("boolean", "Only fill sliders the project already has. Default false.")},
		},
		{"path"});
	importData.handler = [&mcp](const json& args) {
		std::string path = McpFilePath(McpArgs::String(args, "path"));
		const bool fromFolder = McpArgs::Bool(args, "fromFolder", false);
		if (fromFolder ? !wxFileName::DirExists(wxString::FromUTF8(path)) : !wxFileName::FileExists(wxString::FromUTF8(path)))
			throw McpToolError("There is no " + std::string(fromFolder ? "folder" : "file") + " '" + path + "'.");
		McpRunStep(mcp,
				   {{"type", "ImportSliderData"},
					{"SliderDataFile", path},
					{"FromFolder", fromFolder},
					{"MergeSliders", McpArgs::Bool(args, "mergeOnly", false)},
					{"SliderNames", McpArgs::Strings(args, "sliders")}});
		return SliderNames(mcp.Project());
	};
	mcp.AddTool(std::move(importData));

	McpTool exportData;
	exportData.name = "export_slider_data";
	exportData.description =
		"Writes slider data of shapes, like the Export Slider Data menu: nif (the shape as moved by the slider), obj, "
		"bsd (the slider's deltas) or dat (Starfield morph.dat with all sliders of a shape). For several files pass "
		"folder: true and a folder as path; they're named \"<shape>_<slider>.<ext>\". OSD and TRI files of all "
		"sliders come from export_file.";
	exportData.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "File, or folder with folder: true (absolute or relative to the BodySlide folder)")},
			{"format", {{"type", "string"}, {"enum", {"nif", "obj", "bsd", "dat"}}, {"description", "Default: the extension of path"}}},
			{"sliders", McpStringList("Sliders to export. Default: the slider in edit mode.")},
			{"shapes", McpStringList("Shapes, the selected ones by default")},
			{"folder", McpProp("boolean", "path is a folder for one file per shape and slider. Default false.")},
			{"onlyChanged", McpProp("boolean", "obj: only the vertices the slider moves. Default false.")},
			{"overwrite", McpProp("boolean", "Replace files that exist. Default false, which refuses.")},
		},
		{"path"});
	exportData.handler = [&mcp](const json& args) { return ExportSliderData(mcp, args); };
	mcp.AddTool(std::move(exportData));

	McpTool savePreset;
	savePreset.name = "save_preset";
	savePreset.description =
		"Saves the current slider values as a BodySlide preset, like Save Preset in the window: visible sliders that "
		"aren't at their default.";
	savePreset.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "Preset name")},
			{"file", McpProp("string", "Preset file, a name in the SliderPresets folder (e.g. \"MyPresets.xml\") or a path")},
			{"groups", McpStringList("Slider groups the preset applies to, e.g. [\"CBBE\", \"3BA\"]")},
			{"overwrite", McpProp("boolean", "Replace a preset of the same name in the file. Default false.")},
		},
		{"name", "file"});
	savePreset.handler = [&mcp](const json& args) { return SavePreset(mcp, args); };
	mcp.AddTool(std::move(savePreset));

	McpTool physics;
	physics.name = "physics_data";
	physics.description =
		"Lists, imports or exports the cloth physics data (HKX) of the project, like Import and Export Physics Data "
		"in the window. Data is listed by the file it came from.";
	physics.inputSchema = McpSchema(
		{
			{"op", {{"type", "string"}, {"enum", {"list", "import", "export"}}}},
			{"path", McpProp("string", "import: HKX file to read. export: HKX file to write.")},
			{"source", McpProp("string", "export: the data source as op list shows it")},
			{"overwrite", McpProp("boolean", "export: replace a file that exists. Default false.")},
		},
		{"op"});
	physics.handler = [&mcp](const json& args) { return PhysicsData(mcp, args); };
	mcp.AddTool(std::move(physics));
}
