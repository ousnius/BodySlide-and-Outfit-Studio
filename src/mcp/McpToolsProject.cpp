/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/AutomationJson.h"
#include "../program/AutomationRunner.h"
#include "../program/OutfitStudio.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/GameUtil.h"
#include "../utils/ProjectUtil.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/utils.h>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace nifly;

namespace {

std::string ToUTF8(const wxString& text) {
	return text.ToUTF8().data();
}

// Path relative to the BodySlide folder, the way the tools take it back
std::string RelativeToProject(const wxString& path) {
	wxFileName fileName(path);
	fileName.MakeRelativeTo(wxString::FromUTF8(ProjectUtil::GetProjectPath()));
	return ToUTF8(fileName.GetFullPath(wxPATH_UNIX));
}

// A slider set file given as a path, or by its name in the SliderSets folder
std::string SliderSetFilePath(const std::string& file) {
	std::string path = McpFilePath(file);
	if (!wxFileName::FileExists(wxString::FromUTF8(path))) {
		std::string inSliderSets = ProjectUtil::GetProjectPath() + "/SliderSets/" + file;
		if (wxFileName::FileExists(wxString::FromUTF8(inSliderSets)))
			return inSliderSets;
		throw McpToolError("There is no slider set file '" + file + "'. list_slider_sets lists them.");
	}
	return path;
}

std::vector<std::string> SetNames(const std::string& path) {
	SliderSetFile file(path);
	if (file.fail())
		throw McpToolError("'" + path + "' couldn't be read as a slider set file.");

	std::vector<std::string> names;
	file.GetSetNames(names);
	return names;
}

// Unsaved changes are only thrown away when the agent says so
void CheckPendingChanges(OutfitStudioFrame* frame, const json& args) {
	if (!frame->HasPendingChanges())
		return;

	if (!McpArgs::Bool(args, "discardChanges", false))
		throw McpToolError("The project has unsaved changes. Save it with save_project first, or pass discardChanges: true to drop them.");

	frame->SetPendingChanges(false);
}

// The objects of an OBJ file with their material and its texture maps from the .mtl files it names
json ObjObjects(const std::string& path) {
	std::ifstream file(std::filesystem::u8path(path));
	std::vector<std::string> mtlFiles;
	json objects = json::array();
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string cmd, value;
		iss >> cmd >> value;
		if (cmd == "o" || cmd == "g")
			objects.push_back({{"name", value}});
		else if (cmd == "usemtl" && !objects.empty() && !objects.back().contains("material"))
			objects.back()["material"] = value;
		else if (cmd == "mtllib") {
			// File names can have spaces
			std::string rest;
			if (std::getline(iss, rest))
				value += rest;
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
				value.pop_back();
			if (!value.empty())
				mtlFiles.push_back(value);
		}
	}

	// Map statements to their texture files, relative paths made absolute to the .mtl
	std::map<std::string, json> materials;
	const std::filesystem::path folder = std::filesystem::u8path(path).parent_path();
	for (auto& mtlFile : mtlFiles) {
		std::ifstream mtl(folder / std::filesystem::u8path(mtlFile));
		std::string current;
		while (std::getline(mtl, line)) {
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			std::istringstream iss(line);
			std::string cmd;
			iss >> cmd;
			if (cmd == "newmtl")
				iss >> current;
			else if (cmd.rfind("map_", 0) == 0 && !current.empty()) {
				// The file is the last token, after options like "-bm 1.0"
				std::string token, texture;
				while (iss >> token)
					texture = token;
				if (!texture.empty())
					materials[current][cmd] = (folder / std::filesystem::u8path(texture)).lexically_normal().u8string();
			}
		}
	}

	for (auto& object : objects) {
		auto mat = materials.find(object.value("material", std::string()));
		if (mat != materials.end())
			object["textures"] = mat->second;
	}
	return objects;
}

const json DiscardChangesProp = McpProp("boolean", "Drop unsaved changes of the open project. Default false, which refuses when there are any.");

McpToolResult GetStatus(OutfitStudioFrame* frame) {
	OutfitProject* project = frame->project;

	int targetGame = Config.GetIntValue("TargetGame");
	McpToolResult result;
	json& data = result.data;
	data["targetGame"] = targetGame >= 0 && targetGame < static_cast<int>(GameUtil::TargetGames.size()) ? ToUTF8(GameUtil::TargetGames[targetGame])
																										 : std::to_string(targetGame);
	data["busy"] = nullptr;

	if (!project) {
		data["project"] = nullptr;
		return result;
	}

	NiShape* baseShape = project->GetBaseShape();

	data["project"] = {
		// The name save_project uses. The slider set's own name changes to the reference's when one is loaded.
		{"displayName", ToUTF8(project->mOutfitName)},
		{"sliderSetFile", ToUTF8(project->mFileName)},
		{"copyReference", project->mCopyRef},
		{"shapeDataFolder", ToUTF8(project->mDataDir)},
		{"shapeDataFile", ToUTF8(project->mBaseFile)},
		{"outputDataPath", ToUTF8(project->mGamePath)},
		{"outputFileName", ToUTF8(project->mGameFile)},
		{"pendingChanges", frame->HasPendingChanges()},
	};

	data["reference"] = baseShape ? json{
										{"shape", baseShape->name.get()},
										{"projectFile", project->GetReferenceProjectFile()},
										{"projectName", project->GetReferenceProjectName()},
									}
								  : json(nullptr);

	json shapes = json::array();
	for (auto& name : project->GetShapeOrder())
		if (!baseShape || name != baseShape->name.get())
			shapes.push_back(name);

	data["outfitShapes"] = shapes;
	data["selectedShapes"] = frame->GetSelectedShapeNames();
	data["sliderCount"] = project->SliderCount();
	data["editingSlider"] = frame->editor.bEditSlider ? json(frame->editor.activeSlider) : json(nullptr);
	return result;
}

// Escapes a name for a regular expression that matches it exactly
std::string RegexLiteral(const std::string& text) {
	static const std::string special = R"(.^$|()[]{}*+?\)";
	std::string escaped;
	for (char c : text) {
		if (special.find(c) != std::string::npos)
			escaped += '\\';
		escaped += c;
	}
	return escaped;
}

// The errors and warnings of BodySlide's last run in its log
json BodySlideLogProblems(const std::string& logPath) {
	std::ifstream file(std::filesystem::u8path(logPath));
	std::vector<std::string> run;
	std::string line;
	while (std::getline(file, line)) {
		if (line.rfind("[#Log", 0) == 0)
			run.clear();
		run.push_back(line);
	}

	json errors = json::array();
	json warnings = json::array();
	for (auto& entry : run) {
		// "[time][level]\tfile(line): text", level 1 is an error and 2 a warning
		const size_t level = entry.find("][");
		if (level == std::string::npos || level + 3 >= entry.size())
			continue;
		const size_t text = entry.find(": ", level);
		const std::string message = text == std::string::npos ? entry : entry.substr(text + 2);
		if (entry[level + 2] == '1')
			errors.push_back(message);
		else if (entry[level + 2] == '2')
			warnings.push_back(message);
	}
	return {{"errors", errors}, {"warnings", warnings}};
}

McpToolResult BuildOutfits(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject* project = frame->project;

	wxFileName bodySlide(wxString::FromUTF8(Config["AppDir"]), "BodySlide");
#ifdef _WIN32
	bodySlide.SetExt("exe");
#endif
	if (!bodySlide.FileExists())
		throw McpToolError("BodySlide isn't at '" + ToUTF8(bodySlide.GetFullPath()) + "', next to Outfit Studio.");

	std::vector<std::string> outfits = McpArgs::Strings(args, "outfits");
	const std::string filter = McpArgs::String(args, "filter", "");
	const std::vector<std::string> groups = McpArgs::Strings(args, "groups");
	if (!outfits.empty() && !filter.empty())
		throw McpToolError("Give outfits or filter, not both.");
	if (outfits.empty() && filter.empty() && groups.empty()) {
		if (!project || project->mOutfitName.empty())
			throw McpToolError("No project is open. Name the outfits to build in 'outfits'.");
		outfits.push_back(ToUTF8(project->mOutfitName));
	}

	// Sets that don't exist are named here, BodySlide would only say that its filter matched nothing
	json missing = json::array();
	if (!outfits.empty()) {
		std::set<std::string, case_insensitive_compare> known;
		wxArrayString setFiles;
		wxString folder = wxString::FromUTF8(ProjectUtil::GetProjectPath() + "/SliderSets");
		if (wxDir::Exists(folder))
			wxDir::GetAllFiles(folder, &setFiles, "*.osp");
		for (auto& file : setFiles) {
			SliderSetFile setFile(ToUTF8(file));
			if (setFile.fail())
				continue;
			std::vector<std::string> names;
			setFile.GetSetNames(names);
			known.insert(names.begin(), names.end());
		}

		std::vector<std::string> existing;
		for (auto& name : outfits) {
			if (known.count(name))
				existing.push_back(name);
			else
				missing.push_back(name);
		}
		if (existing.empty())
			throw McpToolError("There is no slider set named " + missing.dump() + ". list_slider_sets lists them.");
		outfits = existing;
	}

	// Never into the game: the build is for looking at, and the game's files stay as the user built them
	wxString target = wxString::FromUTF8(McpArgs::String(args, "targetDir", ""));
	if (target.empty())
		target = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "BodySlide MCP Build";
	else
		target = wxString::FromUTF8(McpFilePath(ToUTF8(target)));
	wxFileName targetDir = wxFileName::DirName(target);
	targetDir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	for (const char* key : {"GameDataPath", "OutputDataPath"}) {
		if (Config[key].empty())
			continue;
		wxFileName gameDir = wxFileName::DirName(wxString::FromUTF8(Config[key]));
		gameDir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
		const wxString game = gameDir.GetPathWithSep().Lower();
		if (targetDir.GetPathWithSep().Lower().StartsWith(game))
			throw McpToolError("targetDir is in the game's data folder. Build into another folder; BodySlide itself builds into the game.");
	}
	if (!targetDir.DirExists() && !targetDir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
		throw McpToolError("The folder '" + ToUTF8(targetDir.GetPath()) + "' couldn't be made.");

	std::vector<wxString> argv = {bodySlide.GetFullPath(), "--quiet", "--targetdir", targetDir.GetPath()};
	if (!outfits.empty()) {
		// As one exact regular expression, since names can contain the separators of --build
		std::string pattern;
		for (auto& name : outfits)
			pattern += (pattern.empty() ? "" : "|") + RegexLiteral(name);
		argv.insert(argv.end(), {"--filter", wxString::FromUTF8("^(" + pattern + ")$"), "--regexfilter"});
	}
	else if (!filter.empty()) {
		argv.insert(argv.end(), {"--filter", wxString::FromUTF8(filter)});
		if (McpArgs::Bool(args, "regex", false))
			argv.push_back("--regexfilter");
	}
	if (!groups.empty()) {
		std::string joined;
		for (auto& group : groups)
			joined += (joined.empty() ? "" : ",") + group;
		argv.insert(argv.end(), {"--groupbuild", wxString::FromUTF8(joined)});
	}
	const std::string preset = McpArgs::String(args, "preset", "");
	if (!preset.empty())
		argv.insert(argv.end(), {"--preset", wxString::FromUTF8(preset)});
	if (McpArgs::Bool(args, "tri", false))
		argv.push_back("--trimorphs");

	std::vector<const wxChar*> argvPtrs;
	for (auto& arg : argv)
		argvPtrs.push_back(arg.wc_str());
	argvPtrs.push_back(nullptr);

	// The files there before, so only what this build wrote is reported
	const std::filesystem::path root(targetDir.GetPath().ToStdWstring());
	auto listFiles = [&root]() {
		std::map<std::filesystem::path, std::filesystem::file_time_type> list;
		std::error_code error;
		for (auto it = std::filesystem::recursive_directory_iterator(root, error); !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error))
			if (it->is_regular_file())
				list[it->path()] = it->last_write_time();
		return list;
	};
	const auto before = listFiles();

	// The window is disabled until BodySlide is done, so nobody changes the project in the meantime
	const long code = wxExecute(argvPtrs.data(), wxEXEC_SYNC);
	if (code == -1)
		throw McpToolError("BodySlide couldn't be started.");

	// What the build wrote, with the shapes of each NIF to compare with the project
	json files = json::array();
	for (auto& file : listFiles()) {
		auto old = before.find(file.first);
		if (old != before.end() && old->second == file.second)
			continue;
		const std::filesystem::path& path = file.first;
		json entry = {{"path", ToUTF8(wxString(path.wstring()))}};
		std::string ext = path.extension().u8string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		NifFile nif;
		if (ext == ".nif" && nif.Load(path) == 0) {
			json shapes = json::array();
			for (auto* shape : nif.GetShapes())
				shapes.push_back({{"name", shape->name.get()}, {"vertices", shape->GetNumVertices()}, {"triangles", shape->GetNumTriangles()}});
			entry["shapes"] = shapes;
		}
		files.push_back(entry);
	}

	McpToolResult result;
	result.data["targetDir"] = ToUTF8(targetDir.GetPath());
	result.data["files"] = files;
	json problems = BodySlideLogProblems(Config["AppDir"] + "/Log_BS.txt");
	result.data["errors"] = problems["errors"];
	if (!missing.empty())
		result.data["missing"] = missing;
	if (!problems["warnings"].empty())
		result.data["warnings"] = problems["warnings"];
	if (files.empty())
		result.data["note"] = "Nothing was built. errors says why, e.g. an outfit name that doesn't exist.";
	if (project && frame->HasPendingChanges())
		result.data["unsavedChanges"] = "The open project has unsaved changes, which BodySlide doesn't see: it builds the saved slider set.";
	return result;
}

} // namespace

void AddProjectTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool status;
	status.name = "get_status";
	status.description =
		"Overview of the open project: its name (displayName) and where it saves to, whether the reference is saved "
		"with it, the reference shape, the "
		"outfit shapes in order, selected shapes, slider count, the slider being edited and whether there are "
		"unsaved changes. Call it first, and again after anything that failed or timed out.";
	status.readOnly = true;
	status.handler = [frame](const json&) { return GetStatus(frame); };
	mcp.AddTool(std::move(status));

	McpTool listSets;
	listSets.name = "list_slider_sets";
	listSets.description =
		"Lists slider set files (.osp) in the SliderSets folder with the sets (projects) in each. A slider set "
		"file can hold several sets. Use filter to search, as there can be thousands.";
	listSets.inputSchema = McpSchema({
		{"file", McpProp("string", "Only list the sets of this file (absolute, relative to the BodySlide folder, or a name in SliderSets)")},
		{"filter", McpProp("string", "Only sets or files whose name contains this text, ignoring case")},
	});
	listSets.readOnly = true;
	listSets.handler = [](const json& args) {
		wxString filter = wxString::FromUTF8(McpArgs::String(args, "filter", "")).Lower();

		wxArrayString files;
		if (McpArgs::Has(args, "file"))
			files.Add(wxString::FromUTF8(SliderSetFilePath(McpArgs::String(args, "file"))));
		else {
			wxString folder = wxString::FromUTF8(ProjectUtil::GetProjectPath() + "/SliderSets");
			if (wxDir::Exists(folder))
				wxDir::GetAllFiles(folder, &files, "*.osp");
		}

		json list = json::array();
		for (auto& file : files) {
			const bool fileMatches = filter.empty() || wxFileName(file).GetName().Lower().Contains(filter);

			json sets = json::array();
			SliderSetFile setFile(ToUTF8(file));
			if (setFile.fail())
				continue;

			std::vector<std::string> names;
			setFile.GetSetNames(names);
			for (auto& name : names)
				if (fileMatches || wxString::FromUTF8(name).Lower().Contains(filter))
					sets.push_back(name);

			if (!sets.empty())
				list.push_back({{"file", RelativeToProject(file)}, {"sets", sets}});
		}

		McpToolResult result;
		result.data["files"] = list;
		return result;
	};
	mcp.AddTool(std::move(listSets));

	McpTool listTemplates;
	listTemplates.name = "list_reference_templates";
	listTemplates.description = "Lists the reference templates, which name a body shape to convert outfits to, e.g. \"CBBE 3BA\". load_reference takes them by name.";
	listTemplates.readOnly = true;
	listTemplates.handler = [frame](const json&) {
		frame->UpdateReferenceTemplates();

		json list = json::array();
		for (auto& tmpl : frame->GetRefTemplates())
			list.push_back({{"name", tmpl.GetName()}, {"file", tmpl.GetSource()}, {"set", tmpl.GetSetName()}, {"shape", tmpl.GetShape()}});

		McpToolResult result;
		result.data["templates"] = list;
		return result;
	};
	mcp.AddTool(std::move(listTemplates));

	McpTool newProject;
	newProject.name = "new_project";
	newProject.description = "Closes the open project and starts an empty one. Load a reference and import or add outfit shapes after.";
	newProject.inputSchema = McpSchema({{"discardChanges", DiscardChangesProp}});
	newProject.handler = [&mcp, frame](const json& args) {
		CheckPendingChanges(frame, args);
		McpRunStep(mcp, {{"type", "ClearProject"}});
		frame->SetPendingChanges(false);
		return GetStatus(frame);
	};
	mcp.AddTool(std::move(newProject));

	McpTool loadProject;
	loadProject.name = "load_project";
	loadProject.description =
		"Opens a set of a slider set file as the project, replacing the open one. Its reference shape, if it has "
		"one, is loaded too.";
	loadProject.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "Slider set file (absolute, relative to the BodySlide folder, or a name in SliderSets)")},
			{"set", McpProp("string", "Set to open, needed when the file has more than one")},
			{"discardChanges", DiscardChangesProp},
		},
		{"file"});
	loadProject.handler = [frame](const json& args) {
		std::string path = SliderSetFilePath(McpArgs::String(args, "file"));
		std::string set = McpArgs::String(args, "set", "");

		// Shape data is read from this BodySlide folder's ShapeData, so a set of another folder would load empty
		wxFileName setFile(wxString::FromUTF8(path));
		setFile.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_LONG);
		wxFileName sliderSets(wxString::FromUTF8(ProjectUtil::GetProjectPath() + "/SliderSets/"));
		sliderSets.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_LONG);
		wxString setFolder = setFile.GetPath().Lower() + wxFileName::GetPathSeparator();
		if (!setFolder.StartsWith(sliderSets.GetPath().Lower() + wxFileName::GetPathSeparator()))
			throw McpToolError("Only slider set files of this BodySlide folder's SliderSets can be opened. To score against a project of "
							   "another folder, use compare_to_project.");

		std::vector<std::string> sets = SetNames(path);
		if (set.empty() && sets.size() > 1)
			throw McpToolError("The file has " + std::to_string(sets.size()) + " sets, pass one as 'set': " + json(sets).dump());
		if (!set.empty() && std::find(sets.begin(), sets.end(), set) == sets.end())
			throw McpToolError("The file has no set '" + set + "'. It has: " + json(sets).dump());

		CheckPendingChanges(frame, args);
		if (!frame->LoadProject(path, set))
			throw McpToolError("Loading the project failed, see 'messages' and 'log'.");

		return GetStatus(frame);
	};
	mcp.AddTool(std::move(loadProject));

	McpTool loadReference;
	loadReference.name = "load_reference";
	loadReference.description =
		"Loads the reference shape: the body the outfit is converted to and conformed against, with its sliders. "
		"Give a template name from list_reference_templates, or a slider set file with set and shape, or a NIF file "
		"with shape. By default the outfit's sliders are replaced by the reference's, keeping zap sliders, and zaps of "
		"the old reference are moved onto the new one (zapsTransferred in the result).";
	loadReference.inputSchema = McpSchema({
		{"template", McpProp("string", "Name of a reference template")},
		{"file", McpProp("string", "Slider set file or NIF file (absolute or relative to the BodySlide folder)")},
		{"set", McpProp("string", "Set of the slider set file")},
		{"shape", McpProp("string", "Reference shape in the set or NIF")},
		{"mergeSliders", McpProp("boolean", "Keep the outfit's sliders and add the reference's data to them. Default false.")},
		{"mergeZaps", McpProp("boolean", "Keep the outfit's zap sliders. Default true.")},
		{"appendNewSliders", McpProp("boolean", "Add the reference's sliders that the project doesn't have. Default true; false only fills in data for sliders the project already has.")},
		{"transferZaps",
		 McpProp("boolean",
				 "With mergeZaps: give the new reference the zaps the old one had (e.g. BodyZap hiding the body under a "
				 "bodysuit), by closest surface point. Default true.")},
		{"zapDistance", McpProp("number", "transferZaps: how far to look for the old reference's surface. Default 10.")},
	});
	loadReference.handler = [&mcp, frame](const json& args) {
		json step = {
			{"type", "LoadReference"},
			{"LoadAll", false},
			{"MergeSliders", McpArgs::Bool(args, "mergeSliders", false)},
			{"MergeZaps", McpArgs::Bool(args, "mergeZaps", true)},
			{"AppendNewSliders", McpArgs::Bool(args, "appendNewSliders", true)},
		};

		if (McpArgs::Has(args, "template")) {
			std::string name = McpArgs::String(args, "template");
			frame->UpdateReferenceTemplates();
			const auto& templates = frame->GetRefTemplates();
			auto tmpl = std::find_if(templates.begin(), templates.end(), [&](const RefTemplate& t) { return t.GetName() == name; });
			if (tmpl == templates.end())
				throw McpToolError("There is no reference template '" + name + "'. list_reference_templates lists them.");

			step["SourceFile"] = tmpl->GetSource();
			step["Set"] = tmpl->GetSetName();
			step["Shape"] = tmpl->GetShape();
		}
		else {
			step["SourceFile"] = McpArgs::String(args, "file");
			step["Set"] = McpArgs::String(args, "set", "");
			step["Shape"] = McpArgs::String(args, "shape");
		}

		// Kept zap sliders lose their data on the old reference, so the new one gets the same zaps by proximity
		ZapSource zaps;
		const bool transferZaps = McpArgs::Bool(args, "mergeZaps", true) && McpArgs::Bool(args, "transferZaps", true);
		if (transferZaps && mcp.Project().GetBaseShape())
			zaps = ProjectOperations::CaptureZaps(mcp.Project(), mcp.Project().GetBaseShape());

		McpRunStep(mcp, step);

		json transferred = json::array();
		NiShape* newBase = mcp.Project().GetBaseShape();
		if (newBase && !zaps.zaps.empty()) {
			// Only zaps that are still there and that the new reference has no data for
			for (auto it = zaps.zaps.begin(); it != zaps.zaps.end();) {
				std::vector<bool> current = mcp.Project().ValidSlider(it->first) ? mcp.Project().GetZappedVertices(newBase, it->first) : std::vector<bool>();
				const bool keep = mcp.Project().ValidSlider(it->first) && std::find(current.begin(), current.end(), true) == current.end();
				it = keep ? std::next(it) : zaps.zaps.erase(it);
			}

			std::vector<ZapTransfer> transfers;
			mcp.Apply(mcp.Ops().ApplyZaps(zaps, newBase, McpArgs::Float(args, "zapDistance", 10.0f), transfers));
			for (auto& t : transfers)
				transferred.push_back({{"slider", t.slider}, {"from", zaps.shapeName}, {"sourceZapped", t.sourceZapped}, {"zapped", t.zapped}});
		}

		McpToolResult result = GetStatus(frame);
		if (!transferred.empty())
			result.data["zapsTransferred"] = transferred;
		return result;
	};
	mcp.AddTool(std::move(loadReference));

	McpTool addProject;
	addProject.name = "add_project";
	addProject.description = "Adds the shapes and sliders of a set of a slider set file to the open project, e.g. to combine outfit parts.";
	addProject.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "Slider set file (absolute, relative to the BodySlide folder, or a name in SliderSets)")},
			{"set", McpProp("string", "Set to add")},
			{"appendNewSliders", McpProp("boolean", "Also add sliders the project doesn't have yet. Default true.")},
		},
		{"file", "set"});
	addProject.handler = [&mcp, frame](const json& args) {
		McpRunStep(mcp,
				{{"type", "AddProject"},
				 {"SourceFile", SliderSetFilePath(McpArgs::String(args, "file"))},
				 {"Set", McpArgs::String(args, "set")},
				 {"AppendNewSliders", McpArgs::Bool(args, "appendNewSliders", true)}});
		return GetStatus(frame);
	};
	mcp.AddTool(std::move(addProject));

	McpTool importFile;
	importFile.name = "import_file";
	importFile.description =
		"Imports a NIF, OBJ, FBX or head TRI file into the project as outfit shapes, with default import options, or "
		"with fromFolder every such file of a folder. TRI files next to a NIF add their morphs as sliders, and a head "
		"TRI file becomes a shape with its morphs as sliders. Slider data of existing shapes comes from "
		"import_slider_data. OBJ files from other programs (Blender, sculpting tools) take upAxis, scale, objects and "
		"skipObjects; each of their objects becomes a shape, and the result lists the objects with their material and "
		"its texture files from the .mtl. OBJ units are imported times 10, like Outfit Studio's OBJ export divides "
		"them; a figure made at real-world height in meters comes in at about a tenth of the game's size.";
	importFile.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "File or folder to import (absolute or relative to the BodySlide folder)")},
			{"fromFolder", McpProp("boolean", "path is a folder. Default false.")},
			{"triSliders", McpProp("boolean", "Load the morphs of TRI files as sliders. Default true.")},
			{"upAxis",
			 {{"type", "string"},
			  {"enum", {"z", "y"}},
			  {"description", "OBJ: the file's up axis. \"y\" for files that are Y up and face +Z (Blender's default OBJ export): they are turned to Z up, facing +Y like the games. Default \"z\"."}}},
			{"scale", McpProp("number", "OBJ: scale on import, on top of the factor 10. Default 1.")},
			{"objects", McpStringList("OBJ: only import these objects")},
			{"skipObjects", McpStringList("OBJ: don't import these objects")},
		},
		{"path"});
	importFile.handler = [&mcp, frame](const json& args) {
		std::string path = McpFilePath(McpArgs::String(args, "path"));
		const bool fromFolder = McpArgs::Bool(args, "fromFolder", false);
		if (fromFolder ? !wxFileName::DirExists(wxString::FromUTF8(path)) : !wxFileName::FileExists(wxString::FromUTF8(path)))
			throw McpToolError("There is no " + std::string(fromFolder ? "folder" : "file") + " '" + path + "'.");

		ObjImportOptions objOptions;
		objOptions.YUp = McpArgs::String(args, "upAxis", "z") == "y";
		objOptions.Scale = McpArgs::Float(args, "scale", 1.0f);
		if (objOptions.Scale <= 0.0f)
			throw McpToolError("Argument 'scale' has to be above 0.");

		const bool isObj = !fromFolder && wxFileName(wxString::FromUTF8(path)).GetExt().Lower() == "obj";
		json objects;
		if (isObj) {
			objects = ObjObjects(path);
			std::vector<std::string> only = McpArgs::Strings(args, "objects");
			std::vector<std::string> skip = McpArgs::Strings(args, "skipObjects");
			if (!only.empty() || !skip.empty()) {
				objOptions.ImportAll = false;
				std::set<std::string> known;
				for (auto& object : objects)
					known.insert(object["name"].get<std::string>());
				for (auto& name : only)
					if (!known.count(name))
						throw McpToolError("The OBJ file has no object '" + name + "'.");
				for (auto& name : known)
					if ((only.empty() || std::find(only.begin(), only.end(), name) != only.end()) && std::find(skip.begin(), skip.end(), name) == skip.end())
						objOptions.ImportShapes.insert(name);
				if (objOptions.ImportShapes.empty())
					throw McpToolError("objects and skipObjects leave no object to import.");
			}
		}

		McpRunStep(mcp, {{"type", "ImportFile"}, {"FilePath", path}, {"FromFolder", fromFolder}, {"TRISliders", McpArgs::Bool(args, "triSliders", true)}}, &objOptions);

		McpToolResult result = GetStatus(frame);
		if (isObj) {
			for (auto& object : objects)
				object["imported"] = objOptions.ImportAll || objOptions.ImportShapes.count(object["name"].get<std::string>()) > 0;
			result.data["objObjects"] = objects;
		}
		return result;
	};
	mcp.AddTool(std::move(importFile));

	McpTool saveProject;
	saveProject.name = "save_project";
	saveProject.description =
		"Saves the project as a set of a slider set file, with its shape data, for BodySlide to build. Arguments "
		"that are left out keep the project's current values, which get_status shows; a new project needs at least "
		"sliderSetFile, displayName and outputFileName. A set that already exists in the file, including the one "
		"the project was loaded from, is only replaced with overwrite: true.";
	saveProject.inputSchema = McpSchema({
		{"displayName", McpProp("string", "Name of the set as BodySlide lists it")},
		{"sliderSetFile", McpProp("string", "Slider set file, relative to the SliderSets folder, e.g. \"MyOutfit.osp\"")},
		{"shapeDataFolder", McpProp("string", "Folder of the shape data, relative to the ShapeData folder")},
		{"shapeDataFile", McpProp("string", "File name of the shape data NIF, e.g. \"MyOutfit.nif\"")},
		{"outputDataPath", McpProp("string", "Game path BodySlide builds the meshes to, e.g. \"meshes\\\\armor\\\\myoutfit\"")},
		{"outputFileName", McpProp("string", "File name of the built meshes without extension")},
		{"genWeights", McpProp("boolean", "Build _0 and _1 weight meshes (Skyrim)")},
		{"copyReference",
		 McpProp("boolean",
				 "Save the reference shape (the body) with the outfit. Default what the project had (get_status "
				 "copyReference): true when the loaded set had the body, which outfits with body zaps or a visible body "
				 "need. Conversions should keep it like the original did.")},
		{"overwrite", McpProp("boolean", "Replace a set of the same name in the slider set file. Default false, which refuses.")},
	});
	saveProject.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		if (!project.GetWorkNif()->IsValid())
			throw McpToolError("There are no shapes to save.");

		SaveProjectOptions options;
		options.displayName = McpArgs::String(args, "displayName", ToUTF8(project.mOutfitName));
		options.sliderSetFile = McpArgs::String(args, "sliderSetFile", ToUTF8(project.mFileName));
		options.shapeDataFolder = McpArgs::String(args, "shapeDataFolder", ToUTF8(project.mDataDir));
		options.shapeDataFile = McpArgs::String(args, "shapeDataFile", ToUTF8(project.mBaseFile));
		options.outputDataPath = McpArgs::String(args, "outputDataPath", ToUTF8(project.mGamePath));
		options.outputFileName = McpArgs::String(args, "outputFileName", ToUTF8(project.mGameFile));
		options.genWeights = McpArgs::Bool(args, "genWeights", project.mGenWeights);
		options.copyReference = McpArgs::Bool(args, "copyReference", project.mCopyRef);
		options.sfMorphPath = ToUTF8(project.mSFMorphPath);
		options.sfMorphTargetShape = ToUTF8(project.mSFMorphTargetShape);

		if (options.displayName.empty())
			throw McpToolError("The project has no name yet, pass displayName.");
		if (options.shapeDataFolder.empty())
			options.shapeDataFolder = options.displayName;
		if (options.shapeDataFile.empty())
			options.shapeDataFile = options.displayName + ".nif";

		// Replacing a set loses what was there, so the agent has to ask for it
		wxFileName setFile(wxString::FromUTF8(options.sliderSetFile));
		if (setFile.IsRelative())
			setFile.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath() + "/SliderSets"));
		if (setFile.FileExists() && !McpArgs::Bool(args, "overwrite", false)) {
			std::vector<std::string> sets = SetNames(ToUTF8(setFile.GetFullPath()));
			if (std::find(sets.begin(), sets.end(), options.displayName) != sets.end())
				throw McpToolError("The set '" + options.displayName + "' already exists in '" + ToUTF8(setFile.GetFullPath())
								   + "'. Pass overwrite: true to replace it, or save under another displayName or sliderSetFile.");
		}

		// Normals come from the meshes like when saving in the window
		std::vector<Mesh*> meshes;
		for (auto* shape : project.GetWorkNif()->GetShapes())
			if (!project.IsBaseShape(shape))
				if (Mesh* m = project.GetHost().GetShapeMesh(shape->name.get()))
					meshes.push_back(m);
		project.UpdateNifNormals(project.GetWorkNif(), meshes);

		mcp.Apply(mcp.Ops().SaveProject(options));
		frame->SetPendingChanges(false);

		McpToolResult result = GetStatus(frame);
		std::vector<std::string> unweighted;
		if (project.HasUnweighted(&unweighted))
			result.data["unweightedShapes"] = unweighted;
		return result;
	};
	mcp.AddTool(std::move(saveProject));

	McpTool exportFile;
	exportFile.name = "export_file";
	exportFile.description =
		"Exports the outfit shapes, or the given shapes, to a NIF, OBJ, FBX, OSD (slider data) or TRI (morphs) "
		"file, chosen by the extension. NIF export of all shapes writes the current slider and pose state of the "
		"meshes. headTri writes the head TRI file of one shape. Slider data of single sliders comes from "
		"export_slider_data. An existing file is only replaced with overwrite: true.";
	exportFile.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "File to write (absolute or relative to the BodySlide folder)")},
			{"withReference", McpProp("boolean", "Include the reference shape (NIF, OBJ, FBX). Default false.")},
			{"global", McpProp("boolean", "OBJ and FBX: positions in global space instead of each shape's own. Default false.")},
			{"shapes", McpStringList("Only these shapes, like Export Selected Shapes. Default: all outfit shapes.")},
			{"headTri", McpProp("boolean", "TRI: write the head morphs of the one shape in shapes. Default false.")},
			{"overwrite", McpProp("boolean", "Replace a file that exists. Default false, which refuses.")},
		},
		{"path"});
	exportFile.handler = [&mcp](const json& args) {
		std::string path = McpFilePath(McpArgs::String(args, "path"));
		McpCheckOverwrite(path, args);

		ExportOptions options;
		options.withReference = McpArgs::Bool(args, "withReference", false);
		options.global = McpArgs::Bool(args, "global", false);
		for (auto& name : McpArgs::Strings(args, "shapes"))
			options.shapes.push_back(mcp.Shape(name)->name.get());
		options.headTri = McpArgs::Bool(args, "headTri", false);
		mcp.Apply(mcp.Ops().ExportFile(path, options));

		McpToolResult result;
		result.data["path"] = path;
		return result;
	};
	mcp.AddTool(std::move(exportFile));

	McpTool build;
	build.name = "build_outfits";
	build.description =
		"Builds outfits with BodySlide the way the user would, into a scratch folder instead of the game: zaps applied, "
		"the low and high weight bodies, the preset's slider values, and TRI files with tri. It builds the saved slider "
		"sets, so save_project first. Returns the files written with the shapes and vertex counts of each NIF, and "
		"BodySlide's errors and warnings. import_file a built NIF into a new project to look at it. Without outfits, "
		"filter or groups it builds the open project's set.";
	build.inputSchema = McpSchema({
		{"outfits", McpStringList("Slider set names to build. Default: the open project's.")},
		{"filter", McpProp("string", "Builds every set whose name contains this text, like BodySlide's outfit filter")},
		{"regex", McpProp("boolean", "filter is a regular expression. Default false.")},
		{"groups", McpStringList("Builds the sets of these groups, narrowed by filter if given")},
		{"preset", McpProp("string", "Preset name, or a preset XML file optionally followed by '?' and a preset name. Default: BodySlide's last preset.")},
		{"tri", McpProp("boolean", "Write TRI morph files too. Default false.")},
		{"targetDir", McpProp("string", "Folder to build into, never the game's data folder. Default: 'BodySlide MCP Build' in the temporary folder.")},
	});
	build.handler = [&mcp](const json& args) { return BuildOutfits(mcp, args); };
	mcp.AddTool(std::move(build));
}
