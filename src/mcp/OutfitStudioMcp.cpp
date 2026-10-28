/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "McpSnapshots.h"
#include "../program/OutfitStudio.h"

#include <wx/log.h>

#include <algorithm>
#include <cstdlib>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

constexpr const char* ServerName = "outfit-studio";
// Version of the tool interface, not of the program
constexpr const char* InterfaceVersion = "1.0.0";

constexpr const char* Instructions =
	"Controls the open Outfit Studio window: its project (outfit shapes, sliders, bone weights) and "
	"the reference body it is converted to. Changes show in the window and can be undone there. "
	"Start with get_status. Tools refuse to run while the user has a dialog open or is painting. "
	"File paths can be absolute or relative to the BodySlide folder. Tools that change the project in ways the "
	"window can't undo take an automatic snapshot first (autoSnapshot in their result); snapshot_restore goes back "
	"to it and snapshot_diff shows what changed. Before saving, validate_project checks for problems in game. "
	"Before a task, read its guide with get_guide: basics (read first), body-conversion, conform-sliders, "
	"fix-clipping, bone-weights, cross-game, external-models and posing.";

// Tools that change the project in ways the window's undo can't take back. An automatic snapshot is taken
// before each, so an agent can always go back.
const std::set<std::string> AutoSnapshotTools = {
	"run_steps", "run_automation_script", "load_reference", "add_project", "import_file", "load_from_archive",
	"conform_sliders", "fix_clipping", "set_slider_deltas", "scale_slider_deltas", "smooth_slider_deltas",
	"rename_shape", "delete_shapes", "duplicate_shape", "set_reference_shape", "delete_vertices",
	"separate_vertices", "merge_shapes", "refine_mesh", "mirror_shapes", "invert_uvs", "recalc_normals",
	"symmetrize_vertices", "shape_transforms", "set_base_shape", "make_conversion_reference",
	"remove_unused_nodes", "set_shader_properties", "set_texture_paths", "set_geometry_properties",
	"set_shape_flags", "shape_shader", "copy_shader", "set_alpha_property", "set_extra_data", "set_partitions", "set_segments",
	"copy_partitions", "add_bones", "add_custom_bone",
	"edit_bone", "delete_bones", "set_bone_transforms", "transfer_weights", "remove_skinning",
	"apply_pose_to_mesh", "fix_bad_bones", "new_slider", "clone_slider", "negate_sliders", "clear_slider_data",
	"delete_sliders", "set_slider_properties", "import_slider_data", "physics_data", "transfer_zaps", "auto_fix_clipping", "import_from_game",
	"fit_to_body", "make_contact_morph",
};

// Tools of AutoSnapshotTools whose preview: true only measures, which needs no snapshot
const std::set<std::string> ReadOnlyPreviewTools = {"make_contact_morph"};

// Tools that replace the project or overwrite files, which a snapshot doesn't bring back
const std::set<std::string> DestructiveTools = {
	"new_project", "load_project", "save_project", "export_file", "export_slider_data", "save_preset",
	"save_pose", "mask_file", "snapshot_delete", "snapshot_restore", "write_material", "convert_textures",
	"build_outfits",
};

const char* LevelName(HostMessageLevel level) {
	switch (level) {
		case HostMessageLevel::Warning: return "warning";
		case HostMessageLevel::Error: return "error";
		default: return "info";
	}
}

// Collects the warnings and errors logged during a tool call, and passes everything on
class LogCapture : public wxLog {
public:
	json entries = json::array();

protected:
	void DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo&) override {
		if (level == wxLOG_Error || level == wxLOG_FatalError)
			entries.push_back({{"level", "error"}, {"text", msg.ToUTF8().data()}});
		else if (level == wxLOG_Warning)
			entries.push_back({{"level", "warning"}, {"text", msg.ToUTF8().data()}});
	}
};

} // namespace

OutfitStudioMcp::OutfitStudioMcp(OutfitStudioFrame* inFrame)
	: frame(inFrame)
	, protocol(registry, ServerName, InterfaceVersion, Instructions)
	, server(protocol)
	, snapshots(std::make_unique<McpSnapshotStore>()) {
	AddProjectTools(*this);
	AddAutomationTools(*this);
	AddViewTools(*this);
	AddSliderTools(*this);
	AddConversionTools(*this);
	AddEditTools(*this);
	AddHistoryTools(*this);
	AddAnalysisTools(*this);
	AddArchiveTools(*this);
	AddShapeTools(*this);
	AddBoneTools(*this);
	AddSliderFileTools(*this);
	AddQualityTools(*this);
	AddMeasureTools(*this);
	AddFittingTools(*this);
	AddTextureTools(*this);
	AddGameTools(*this);
	AddGuideTools(*this);
}

OutfitStudioMcp::~OutfitStudioMcp() {
	Stop();
}

bool OutfitStudioMcp::Start(int port, const std::string& token, std::string& outError) {
	*stopping = false;
	protocol.SetToken(token);
	if (!server.Start(port, outError))
		return false;

	wxLogMessage("MCP server listening at %s.", server.GetUrl());
	return true;
}

void OutfitStudioMcp::Stop() {
	if (!server.IsRunning())
		return;

	// Calls waiting for the main thread are let through and give up right away
	*stopping = true;
	server.Stop([this]() { frame->GetDispatcher().ProcessPending(); });
	wxLogMessage("MCP server stopped.");
}

void OutfitStudioMcp::AddTool(McpTool tool, std::chrono::milliseconds timeout) {
	const bool autoSnapshot = !tool.readOnly && AutoSnapshotTools.count(tool.name) != 0;
	if (autoSnapshot || DestructiveTools.count(tool.name))
		tool.destructive = true;

	tool.progressHandler = OnMainThread(std::move(tool.handler), tool.name, timeout, autoSnapshot);
	tool.handler = nullptr;
	registry.Add(std::move(tool));
}

OutfitStudioMcp::ProgressHandler OutfitStudioMcp::OnMainThread(Handler handler, const std::string& toolName, std::chrono::milliseconds timeout, bool autoSnapshot) {
	OutfitStudioFrame* owner = frame;
	auto stoppingFlag = stopping;

	return [this, owner, stoppingFlag, handler, toolName, timeout, autoSnapshot](const json& args, const McpProgress& progress) {
		// Owned by the call, which can still run after this side gave up waiting
		auto result = std::make_shared<McpToolResult>();

		auto run = [this, owner, stoppingFlag, handler, toolName, args, result, autoSnapshot, progress]() {
			if (*stoppingFlag) {
				*result = McpToolResult::Error("The MCP server is stopping.");
				return;
			}

			std::string busy = owner->GetBusyReason();
			if (!busy.empty()) {
				*result = McpToolResult::Error("Outfit Studio is busy: " + busy + " Try again later.");
				return;
			}

			wxLogMessage("MCP: %s", toolName);

			// The window's progress bar goes to the client, in steps of at least a percent. Operations only report
			// progress while a progress bar runs, so the call runs with one, like the window's commands.
			struct ProgressReset {
				OutfitStudioFrame* frame;
				bool started = false;
				~ProgressReset() {
					if (started && !frame->progressStack.empty())
						frame->EndProgress("", true);
					frame->progressListener = nullptr;
				}
			} progressReset{owner};
			if (progress) {
				auto last = std::make_shared<int>(-1000);
				owner->progressListener = [progress, last](int value, const wxString& message) {
					if (std::abs(value - *last) < 100 && value < 10000)
						return;
					*last = value;
					progress(value / 100.0, 100.0, message.ToUTF8().data());
				};
				if (owner->progressStack.empty()) {
					owner->StartProgress(wxString::FromUTF8(toolName));
					progressReset.started = true;
				}
			}

			// The chain deletes the capture when it goes
			auto capture = new LogCapture();
			json logged;
			{
				wxLogChain chain(capture);
				UnattendedScope scope(owner->projectHost);

				// A snapshot to go back to, the window can't undo what the tool does. A preview changes nothing.
				std::string snapshotName;
				const bool preview = ReadOnlyPreviewTools.count(toolName) && args.is_object() && args.contains("preview") && args["preview"].is_boolean() &&
									 args["preview"].get<bool>();
				if (autoSnapshot && !preview && owner->project) {
					try {
						snapshotName = snapshots->SaveAutomatic(*this, toolName);
					}
					catch (const std::exception& error) {
						wxLogWarning("MCP: no automatic snapshot before %s: %s", toolName, error.what());
					}
				}

				try {
					*result = handler(args);
				}
				catch (const McpToolError& error) {
					*result = McpToolResult::Error(error.what());
				}
				catch (const json::exception& error) {
					*result = McpToolResult::Error(std::string("Invalid arguments: ") + error.what());
				}
				catch (const std::exception& error) {
					*result = McpToolResult::Error(error.what());
				}

				if (!snapshotName.empty() && result->data.is_object())
					result->data["autoSnapshot"] = snapshotName;

				auto messages = scope.Messages();
				if (!messages.empty() && result->data.is_object()) {
					json list = json::array();
					for (auto& message : messages)
						list.push_back({{"level", LevelName(message.level)}, {"title", message.title}, {"text", message.text}});
					result->data["messages"] = list;
				}

				logged = std::move(capture->entries);
			}

			if (!logged.empty() && result->data.is_object())
				result->data["log"] = logged;
		};

		switch (owner->GetDispatcher().Run(run, timeout)) {
			case MainThreadDispatcher::Result::Done: return *result;
			case MainThreadDispatcher::Result::TimedOut:
				return McpToolResult::Error("The call timed out. It may still finish, check with get_status before trying again.");
			default: return McpToolResult::Error("Outfit Studio is closing.");
		}
	};
}

OutfitProject& OutfitStudioMcp::Project() {
	if (!frame->project)
		throw McpToolError("No project is open.");
	return *frame->project;
}

ProjectOperations OutfitStudioMcp::Ops() {
	return ProjectOperations(Project(), frame->editor);
}

NiShape* OutfitStudioMcp::Shape(const std::string& name) {
	NiShape* shape = Project().GetWorkNif()->FindBlockByName<NiShape>(name);
	if (!shape)
		throw McpToolError("There is no shape '" + name + "'. get_status lists the shapes.");
	return shape;
}

std::vector<NiShape*> OutfitStudioMcp::Shapes(const json& args, const char* name, bool allowReference) {
	std::vector<std::string> names = McpArgs::Strings(args, name);
	const bool fromSelection = names.empty();
	if (fromSelection)
		names = frame->GetSelectedShapeNames();

	std::vector<NiShape*> shapes;
	for (auto& shapeName : names) {
		NiShape* shape = Shape(shapeName);
		if (!allowReference && Project().IsBaseShape(shape)) {
			// A selected reference is left out, a named one is a mistake
			if (fromSelection)
				continue;
			throw McpToolError("'" + shapeName + "' is the reference shape, which this tool doesn't change.");
		}
		shapes.push_back(shape);
	}

	if (shapes.empty())
		throw McpToolError(std::string("No shapes given in '") + name + "' and no outfit shapes are selected.");
	return shapes;
}

std::map<std::string, std::vector<bool>>& OutfitStudioMcp::Exclusions() {
	// Exclusions belong to the project they were made for
	const uint64_t id = Project().GetInstanceId();
	if (id != exclusionsProject) {
		exclusions.clear();
		analysisZaps.reset();
		exclusionsProject = id;
	}
	return exclusions;
}

std::optional<std::vector<std::string>>& OutfitStudioMcp::AnalysisZaps() {
	Exclusions();
	return analysisZaps;
}

std::vector<std::string> OutfitStudioMcp::ActiveAnalysisZaps() {
	if (AnalysisZaps())
		return *analysisZaps;

	// BodySlide starts with these zaps on
	OutfitProject& project = Project();
	std::vector<std::string> zaps;
	for (size_t i = 0; i < project.SliderCount(); i++)
		if (project.SliderZap(i) && (project.activeSet[i].defSmallValue > 0.0f || project.activeSet[i].defBigValue > 0.0f))
			zaps.push_back(project.GetSliderName(i));
	return zaps;
}

std::vector<bool> OutfitStudioMcp::HiddenSurfaceTriangles(NiShape* shape, const std::vector<Triangle>& tris) {
	std::vector<bool> hiddenVerts;
	for (auto& zap : ActiveAnalysisZaps()) {
		std::vector<bool> zapped = Project().GetZappedVertices(shape, zap);
		if (hiddenVerts.size() < zapped.size())
			hiddenVerts.resize(zapped.size(), false);
		for (size_t v = 0; v < zapped.size(); v++)
			if (zapped[v])
				hiddenVerts[v] = true;
	}

	// A zap removes every triangle that uses a zapped vertex
	std::vector<bool> hidden(tris.size(), false);
	if (hiddenVerts.empty())
		return hidden;
	auto isHidden = [&](uint16_t v) { return v < hiddenVerts.size() && hiddenVerts[v]; };
	for (size_t t = 0; t < tris.size(); t++)
		hidden[t] = isHidden(tris[t].p1) || isHidden(tris[t].p2) || isHidden(tris[t].p3);
	return hidden;
}

std::vector<Triangle> OutfitStudioMcp::SurfaceTriangles(NiShape* shape, size_t* outHidden) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	std::vector<bool> hidden = HiddenSurfaceTriangles(shape, tris);

	std::vector<Triangle> visible;
	for (size_t t = 0; t < tris.size(); t++)
		if (!hidden[t])
			visible.push_back(tris[t]);
	if (outHidden)
		*outHidden = tris.size() - visible.size();
	return visible;
}

const std::vector<bool>* OutfitStudioMcp::Excluded(NiShape* shape) {
	if (!shape)
		return nullptr;
	auto& all = Exclusions();
	auto it = all.find(shape->name.get());
	if (it == all.end() || it->second.size() != shape->GetNumVertices())
		return nullptr;
	return &it->second;
}

void OutfitStudioMcp::CheckPositionEdit() {
	OutfitProject& project = Project();
	if (frame->editor.bEditSlider) {
		if (project.SliderValue(frame->editor.activeSlider) == 0.0f)
			throw McpToolError("The slider '" + frame->editor.activeSlider + "' in edit mode is at 0, so the change wouldn't show. Set it to 1 with set_slider_values first.");
	}
	else if (!project.AllSlidersZero())
		throw McpToolError("The base shape can only be changed with all sliders at 0, or the slider shapes would be baked into it. "
						   "Call zero_sliders first, or set_slider_edit to change a slider instead.");
}

void OutfitStudioMcp::Apply(const OpResult& result) {
	frame->ApplyRefresh(result.refresh);
	if (!result.ok)
		throw McpToolError(result.error);
}
