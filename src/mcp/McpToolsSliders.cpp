/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "McpSnapshots.h"
#include "../program/AutomationRunner.h"
#include "../components/SliderPresets.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/ProjectUtil.h"
#include "../program/OutfitStudio.h"

#include <wx/filename.h>

#include <algorithm>
#include <cmath>
#include <set>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace nifly;

namespace {

// Deltas of more vertices than this are only returned as statistics
constexpr size_t MaxVerticesPerCall = 20000;

json VectorToJson(const Vector3& v) {
	return json::array({v.x, v.y, v.z});
}

Vector3 VectorFromJson(const json& value, const std::string& what) {
	if (!value.is_array() || value.size() != 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number())
		throw McpToolError(what + " has to be an array of three numbers.");
	return Vector3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
}

void CheckSlider(OutfitProject& project, const std::string& name) {
	if (!project.ValidSlider(name))
		throw McpToolError("There is no slider '" + name + "'. list_sliders lists them.");
}

json ListSliders(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	std::string filter = McpArgs::String(args, "filter", "");
	std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	auto shapes = project.GetWorkNif()->GetShapes();

	json list = json::array();
	for (size_t i = 0; i < project.SliderCount(); i++) {
		std::string name = project.GetSliderName(i);
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (!filter.empty() && lower.find(filter) == std::string::npos)
			continue;

		json withData = json::array();
		SliderData& data = project.activeSet[i];
		for (auto* shape : shapes) {
			TargetDataDiffs* diff = project.GetDiffSet(data, shape);
			if (diff && !diff->empty())
				withData.push_back(shape->name.get());
		}

		json entry = {{"name", name}, {"value", project.SliderValue(i)}, {"shapesWithData", withData}};
		if (project.SliderZap(i))
			entry["zap"] = true;
		if (project.SliderUV(i))
			entry["uv"] = true;
		if (project.SliderClamp(i))
			entry["clamp"] = true;
		if (project.SliderInvert(i))
			entry["invert"] = true;
		if (project.SliderHidden(i))
			entry["hidden"] = true;
		entry["defaultSmall"] = project.SliderDefault(i, false);
		entry["defaultBig"] = project.SliderDefault(i, true);
		list.push_back(entry);
	}

	return list;
}

McpToolResult GetDeltas(OutfitStudioMcp& mcp, const json& args) {
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	std::string slider = McpArgs::String(args, "slider");

	std::vector<Vector3> deltas;
	mcp.Apply(mcp.Ops().GetSliderDeltas(shape, slider, deltas));

	size_t moved = 0;
	double sum = 0.0;
	float largest = 0.0f;
	int largestVertex = -1;
	for (size_t v = 0; v < deltas.size(); v++) {
		float length = deltas[v].length();
		if (length <= 1e-6f)
			continue;
		moved++;
		sum += length;
		if (length > largest) {
			largest = length;
			largestVertex = static_cast<int>(v);
		}
	}

	McpToolResult result;
	result.data["vertexCount"] = deltas.size();
	result.data["moved"] = moved;
	result.data["meanLength"] = moved ? sum / moved : 0.0;
	result.data["maxLength"] = largest;
	result.data["maxVertex"] = largestVertex;

	// Requested vertices, or all moved ones when asked for
	std::vector<int> vertices = McpArgs::Ints(args, "vertices");
	const bool all = McpArgs::Bool(args, "allMoved", false);
	if (!vertices.empty() || all) {
		if (all)
			for (size_t v = 0; v < deltas.size(); v++)
				if (deltas[v].length() > 1e-6f)
					vertices.push_back(static_cast<int>(v));

		if (vertices.size() > MaxVerticesPerCall)
			throw McpToolError("That's " + std::to_string(vertices.size()) + " vertices, ask for at most " + std::to_string(MaxVerticesPerCall)
							   + " per call with 'vertices'.");

		json list = json::object();
		for (int v : vertices) {
			if (v < 0 || static_cast<size_t>(v) >= deltas.size())
				throw McpToolError("Vertex " + std::to_string(v) + " doesn't exist.");
			list[std::to_string(v)] = VectorToJson(deltas[v]);
		}
		result.data["deltas"] = list;
	}

	return result;
}

// Absolute path of a file given absolute or relative to the BodySlide folder
std::string ProjectFile(const std::string& path) {
	return AutomationRunner::MakeAbsoluteToProject(wxString::FromUTF8(path)).ToUTF8().data();
}

// The direction from a bone to the child it reaches, e.g. from the thigh to the knee, in global space
bool ReachDirection(AnimBone* bone, Vector3& outDirection) {
	AnimBone* reach = nullptr;
	float longest = 0.0f;
	for (AnimBone* child : bone->children) {
		float length = child->xformToParent.translation.length();
		if (length > longest) {
			longest = length;
			reach = child;
		}
	}

	if (!reach || longest <= 1e-4f)
		return false;

	outDirection = reach->xformPoseToGlobal.translation - bone->xformPoseToGlobal.translation;
	outDirection.Normalize();
	return true;
}

// Where a bone is and which way it points in the skeleton's global space, with the pose
json BoneTransform(AnimBone* bone) {
	json entry = {{"position", VectorToJson(bone->xformPoseToGlobal.translation)}};

	Vector3 direction;
	if (ReachDirection(bone, direction))
		entry["direction"] = VectorToJson(direction);
	return entry;
}

// Turns a bone about the skeleton's global axes by a rotation vector in radians
void TurnBoneGlobal(AnimBone* bone, const Vector3& rotation) {
	// The bone's frame before its own pose rotation: parent pose, then the bone's rest transform
	Matrix3 frameRotation = bone->xformToParent.rotation;
	if (bone->parent)
		frameRotation = bone->parent->xformPoseToGlobal.rotation * frameRotation;

	// RotVecToMat matrices turn the opposite way round in this composition, so the rotation is negated to turn
	// right-handed about the global axis
	Matrix3 local = frameRotation.Transpose() * RotVecToMat(rotation * -1.0f) * frameRotation * RotVecToMat(bone->poseRotVec);
	bone->poseRotVec = RotMatToVec(local);
	bone->UpdatePoseTransform();
}

// The smallest turn that points a bone in a global direction
Vector3 AimRotation(AnimBone* bone, Vector3 target) {
	Vector3 current;
	if (!ReachDirection(bone, current))
		throw McpToolError("The bone '" + bone->boneName + "' has no child to aim with.");

	target.Normalize();
	const float cosine = std::clamp(current.dot(target), -1.0f, 1.0f);
	Vector3 axis = current.cross(target);
	if (axis.length() < 1e-6f) {
		if (cosine > 0.0f)
			return Vector3();
		// Opposite: any axis across the bone does
		axis = std::abs(current.x) < 0.9f ? current.cross(Vector3(1.0f, 0.0f, 0.0f)) : current.cross(Vector3(0.0f, 1.0f, 0.0f));
	}

	axis.Normalize();
	return axis * std::acos(cosine);
}

// The bones of the current pose that differ from the unposed skeleton, as set_preview_pose takes them.
// With withTransforms, also the global position and direction of the bones.
McpToolResult GetPose(bool posed, const json& args = json::object()) {
	McpToolResult result;
	result.data["posed"] = posed;

	if (McpArgs::Bool(args, "withTransforms", false)) {
		std::vector<std::string> names = McpArgs::Strings(args, "bones");
		if (names.empty()) {
			AnimSkeleton::getInstance().GetBoneNames(names);
			names.erase(std::remove_if(names.begin(), names.end(), [](const std::string& n) { return n.rfind("NPC ", 0) != 0; }), names.end());
		}

		json transforms = json::object();
		for (auto& name : names) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
			if (!bone)
				throw McpToolError("There is no bone '" + name + "' in the skeleton.");
			transforms[name] = BoneTransform(bone);
		}
		result.data["transforms"] = transforms;
	}

	if (!posed) {
		result.data["bones"] = json::object();
		return result;
	}

	PoseData current;
	PoseDataCollection::CaptureCurrentPose("current", false, current);

	json bones = json::object();
	for (auto& bone : current.boneData) {
		const bool rotated = !bone.rotation.IsZero(true);
		const bool moved = !bone.translation.IsZero(true);
		const bool scaled = std::abs(bone.scale - 1.0f) > 1e-5f;
		if (!rotated && !moved && !scaled)
			continue;

		json entry = json::object();
		if (rotated)
			entry["rotation"] = VectorToJson(bone.rotation * (1.0f / DEG2RAD));
		if (moved)
			entry["translation"] = VectorToJson(bone.translation);
		if (scaled)
			entry["scale"] = bone.scale;
		bones[bone.name] = entry;
	}

	result.data["bones"] = bones;
	return result;
}

const json ShapesProp = McpStringList("Outfit shapes, the selected ones by default");

} // namespace

PresetCollection McpLoadPresets() {
	PresetCollection presets;
	std::vector<std::string> groups;
	presets.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", "", groups, true);
	return presets;
}

std::map<std::string, float> McpSliderValuesArg(OutfitStudioMcp& mcp, const json& args) {
	std::map<std::string, float> values;
	const std::string preset = McpArgs::String(args, "preset", "");
	if (!preset.empty()) {
		const std::string weight = McpArgs::String(args, "presetWeight", "high");
		if (weight != "low" && weight != "high")
			McpArgs::Wrong("presetWeight", "low or high");

		PresetCollection presets = McpLoadPresets();
		std::vector<std::string> installed;
		presets.GetPresetNames(installed);
		if (std::find(installed.begin(), installed.end(), preset) == installed.end())
			throw McpToolError("There is no preset '" + preset + "'. list_presets lists them.");

		// Only what the preset sets, the others stay at 0 like with sliderValues
		for (auto& value : mcp.Ops().GetPresetValues(presets, preset, weight == "high"))
			if (value.second != 0.0f)
				values[value.first] = value.second;
	}
	for (auto& value : McpArgs::FloatMap(args, "sliderValues"))
		values[value.first] = value.second;
	return values;
}

bool McpHasSliderValuesArg(const json& args) {
	return McpArgs::Has(args, "sliderValues") || !McpArgs::String(args, "preset", "").empty();
}

json McpPresetValueProperties() {
	return {
		{"preset", McpProp("string", "Instead of or under sliderValues: the values of this body preset (list_presets)")},
		{"presetWeight", {{"type", "string"}, {"enum", {"low", "high"}}, {"description", "With preset: the low or high body. Default high."}}},
	};
}

std::vector<std::pair<std::string, std::map<std::string, float>>> McpPresetCases(OutfitStudioMcp& mcp,
																			 const json& args,
																			 const char* name,
																			 const char* filterName,
																			 const char* weightName) {
	std::vector<std::string> names = McpArgs::Strings(args, name);
	std::string filter = McpArgs::String(args, filterName, "");
	if (names.empty() && filter.empty())
		return {};

	PresetCollection presets = McpLoadPresets();
	std::vector<std::string> installed;
	presets.GetPresetNames(installed);

	if (!filter.empty()) {
		wxString lowerFilter = wxString::FromUTF8(filter).Lower();
		for (auto& preset : installed)
			if (wxString::FromUTF8(preset).Lower().Contains(lowerFilter) && std::find(names.begin(), names.end(), preset) == names.end())
				names.push_back(preset);
		if (names.empty())
			throw McpToolError("No installed preset has '" + filter + "' in its name. list_presets lists them.");
	}

	for (auto& preset : names)
		if (std::find(installed.begin(), installed.end(), preset) == installed.end())
			throw McpToolError("There is no preset '" + preset + "'. list_presets lists them.");

	std::vector<std::string> weights = McpArgs::Strings(args, weightName);
	if (weights.empty())
		weights = {"low", "high"};
	for (auto& weight : weights)
		if (weight != "low" && weight != "high")
			McpArgs::Wrong(weightName, "a list of \"low\" and \"high\"");

	ProjectOperations ops = mcp.Ops();
	std::vector<std::pair<std::string, std::map<std::string, float>>> cases;
	for (auto& preset : names)
		for (auto& weight : weights)
			cases.emplace_back(preset + " (" + weight + ")", ops.GetPresetValues(presets, preset, weight == "high"));
	return cases;
}

void AddSliderTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool list;
	list.name = "list_sliders";
	list.description =
		"Lists the sliders with their current value, flags (zap, uv, clamp, invert, hidden), BodySlide defaults and the "
		"shapes that have morph data for each.";
	list.inputSchema = McpSchema({{"filter", McpProp("string", "Only sliders whose name contains this text, ignoring case")}});
	list.readOnly = true;
	list.handler = [&mcp](const json& args) {
		McpToolResult result;
		result.data["sliders"] = ListSliders(mcp, args);
		return result;
	};
	mcp.AddTool(std::move(list));

	McpTool setValues;
	setValues.name = "set_slider_values";
	setValues.description = "Sets slider values in the window, 0 to 1 (100% in the window), to see the outfit with them. Other sliders keep their values.";
	setValues.inputSchema = McpSchema({{"values", McpNumberMap("Slider names to values, e.g. {\"Breasts\": 1, \"Waist\": 0.5}")}}, {"values"});
	setValues.handler = [&mcp](const json& args) {
		mcp.Apply(mcp.Ops().SetSliderValues(McpArgs::FloatMap(args, "values")));
		return McpToolResult();
	};
	mcp.AddTool(std::move(setValues));

	McpTool zero;
	zero.name = "zero_sliders";
	zero.description = "Sets all sliders to 0, apart from clamp sliders.";
	zero.handler = [&mcp, frame](const json&) {
		frame->ApplyRefresh(mcp.Ops().ZeroSliders());
		return McpToolResult();
	};
	mcp.AddTool(std::move(zero));

	McpTool listPresets;
	listPresets.name = "list_presets";
	listPresets.description = "Lists the body presets installed in the SliderPresets folder, with their groups. apply_preset and quality_sweep take them.";
	listPresets.inputSchema = McpSchema({{"filter", McpProp("string", "Only presets whose name contains this text, ignoring case")}});
	listPresets.readOnly = true;
	listPresets.handler = [](const json& args) {
		PresetCollection presets = McpLoadPresets();
		std::vector<std::string> names;
		presets.GetPresetNames(names);

		wxString filter = wxString::FromUTF8(McpArgs::String(args, "filter", "")).Lower();
		json list = json::array();
		for (auto& name : names) {
			if (!filter.empty() && !wxString::FromUTF8(name).Lower().Contains(filter))
				continue;

			std::vector<std::string> groups;
			presets.GetPresetGroups(name, groups);
			list.push_back({{"name", name}, {"groups", groups}});
		}

		McpToolResult result;
		result.data["presets"] = list;
		return result;
	};
	mcp.AddTool(std::move(listPresets));

	McpTool applyPreset;
	applyPreset.name = "apply_preset";
	applyPreset.description =
		"Sets the sliders to a body preset of list_presets, like Load Preset in the window: the low (weight 0) or high "
		"(weight 100) body. Sliders the preset doesn't have take their default, zap sliders it doesn't set are turned off. "
		"Returns the values other than 0, which analyze_clipping, fix_clipping (combination mode) and render_view (with "
		"zeroOthers) take as sliderValues. The sliders stay set afterwards: zero_sliders puts them back.";
	applyPreset.inputSchema = McpSchema(
		{
			{"preset", McpProp("string", "Preset name")},
			{"weight", {{"type", "string"}, {"enum", {"low", "high"}}, {"description", "Default high"}}},
			{"returnValues", McpProp("boolean", "Return the values. Default true; false returns only how many sliders were set, for when only the window (render_view, analyze_clipping with zeroOthers false) needs them.")},
		},
		{"preset"});
	applyPreset.handler = [&mcp](const json& args) {
		std::string name = McpArgs::String(args, "preset");
		std::string weight = McpArgs::String(args, "weight", "high");
		if (weight != "low" && weight != "high")
			McpArgs::Wrong("weight", "low or high");

		PresetCollection presets = McpLoadPresets();
		std::vector<std::string> installed;
		presets.GetPresetNames(installed);
		if (std::find(installed.begin(), installed.end(), name) == installed.end())
			throw McpToolError("There is no preset '" + name + "'. list_presets lists them.");

		ProjectOperations ops = mcp.Ops();
		auto values = ops.GetPresetValues(presets, name, weight == "high");
		mcp.Apply(ops.SetSliderValues(values));

		// The values other than 0, which the analysis tools take as sliderValues with the others at 0
		json set = json::object();
		for (auto& value : values)
			if (value.second != 0.0f)
				set[value.first] = value.second;

		McpToolResult result;
		if (McpArgs::Bool(args, "returnValues", true))
			result.data["values"] = set;
		else
			result.data["slidersSet"] = set.size();
		return result;
	};
	mcp.AddTool(std::move(applyPreset));

	McpTool getDeltas;
	getDeltas.name = "get_slider_deltas";
	getDeltas.description =
		"Statistics of the morph of a slider on a shape (moved vertices, mean and largest movement), and the "
		"deltas of chosen vertices in NIF units as [x, y, z].";
	getDeltas.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"slider", McpProp("string", "Slider name")},
			{"vertices", McpIntList("Vertex indices to return the deltas of")},
			{"allMoved", McpProp("boolean", "Return the deltas of all moved vertices. Default false.")},
		},
		{"shape", "slider"});
	getDeltas.readOnly = true;
	getDeltas.handler = [&mcp](const json& args) { return GetDeltas(mcp, args); };
	mcp.AddTool(std::move(getDeltas));

	McpTool setDeltas;
	setDeltas.name = "set_slider_deltas";
	setDeltas.description = "Replaces or adds to the morph deltas of vertices of a shape for a slider, in NIF units.";
	setDeltas.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"slider", McpProp("string", "Slider name")},
			{"deltas",
			 {{"type", "object"},
			  {"additionalProperties", {{"type", "array"}, {"items", {{"type", "number"}}}, {"minItems", 3}, {"maxItems", 3}}},
			  {"description", "Vertex index to [x, y, z], e.g. {\"12\": [0, 0.5, 0]}"}}},
			{"add", McpProp("boolean", "Add to the current deltas instead of replacing them. Default false.")},
		},
		{"shape", "slider", "deltas"});
	setDeltas.handler = [&mcp](const json& args) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		const json& list = args.at("deltas");
		if (!list.is_object())
			McpArgs::Wrong("deltas", "an object of vertex indices to [x, y, z]");

		std::unordered_map<uint16_t, Vector3> deltas;
		for (auto& item : list.items()) {
			int vertex = 0;
			try {
				vertex = std::stoi(item.key());
			}
			catch (const std::exception&) {
				throw McpToolError("'" + item.key() + "' isn't a vertex index.");
			}
			if (vertex < 0 || vertex > 65535)
				throw McpToolError("Vertex " + item.key() + " doesn't exist.");
			deltas[static_cast<uint16_t>(vertex)] = VectorFromJson(item.value(), "The delta of vertex " + item.key());
		}

		mcp.Apply(mcp.Ops().SetSliderDeltas(shape, McpArgs::String(args, "slider"), deltas, McpArgs::Bool(args, "add", false)));
		McpToolResult result;
		result.data["changed"] = deltas.size();
		return result;
	};
	mcp.AddTool(std::move(setDeltas));

	McpTool scale;
	scale.name = "scale_slider_deltas";
	scale.description = "Multiplies the morph of a slider on shapes by a factor, e.g. 0.8 to make it weaker. Masked vertices keep their deltas.";
	scale.inputSchema = McpSchema(
		{{"shapes", ShapesProp}, {"slider", McpProp("string", "Slider name")}, {"factor", McpProp("number", "Factor for the deltas")}},
		{"slider", "factor"});
	scale.handler = [&mcp](const json& args) {
		const float factor = static_cast<float>(McpArgs::Number(args, "factor"));
		std::string slider = McpArgs::String(args, "slider");
		for (auto* shape : mcp.Shapes(args, "shapes"))
			mcp.Apply(mcp.Ops().ScaleSliderDeltas(shape, slider, factor));
		return McpToolResult();
	};
	mcp.AddTool(std::move(scale));

	McpTool smooth;
	smooth.name = "smooth_slider_deltas";
	smooth.description = "Smooths the morph of a slider on shapes, which evens out bumps a conform left. Masked vertices keep their deltas.";
	smooth.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"slider", McpProp("string", "Slider name")},
			{"iterations", McpProp("integer", "Smoothing passes. Default 2.")},
			{"strength", McpProp("number", "Strength of each pass, 0 to 1. Default 0.5.")},
		},
		{"slider"});
	smooth.handler = [&mcp](const json& args) {
		std::string slider = McpArgs::String(args, "slider");
		const int iterations = std::clamp(McpArgs::Int(args, "iterations", 2), 1, 100);
		const float strength = std::clamp(McpArgs::Float(args, "strength", 0.5f), 0.0f, 1.0f);
		for (auto* shape : mcp.Shapes(args, "shapes"))
			mcp.Apply(mcp.Ops().SmoothSliderDeltas(shape, slider, iterations, strength));
		return McpToolResult();
	};
	mcp.AddTool(std::move(smooth));

	McpTool edit;
	edit.name = "set_slider_edit";
	edit.description =
		"Enters slider edit mode for a slider, so brush strokes change its morph instead of the base shape, or leaves "
		"it without a slider.";
	edit.inputSchema = McpSchema({{"slider", McpProp("string", "Slider to edit, leave out to stop editing")}});
	edit.handler = [&mcp, frame](const json& args) {
		if (McpArgs::Has(args, "slider")) {
			std::string slider = McpArgs::String(args, "slider");
			CheckSlider(mcp.Project(), slider);
			if (frame->editor.bEditSlider)
				frame->ExitSliderEdit();
			frame->EnterSliderEdit(slider);
		}
		else if (frame->editor.bEditSlider)
			frame->ExitSliderEdit();

		McpToolResult result;
		result.data["editingSlider"] = frame->editor.bEditSlider ? json(frame->editor.activeSlider) : json(nullptr);
		return result;
	};
	mcp.AddTool(std::move(edit));
}

void AddConversionTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool conform;
	conform.name = "conform_sliders";
	conform.description =
		"Conforms the sliders of outfit shapes to the reference shape: each outfit vertex takes the morphs of the "
		"nearby reference vertices. Masked vertices are left alone. Run it after loading the reference. Sliders the "
		"reference has no data for keep the shapes' data and are listed in noReferenceData.";
	conform.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"sliders", McpStringList("Sliders to conform, all sliders that aren't zap or UV sliders by default")},
		{"proximityRadius", McpProp("number", "Search radius for reference vertices. Default 10.")},
		{"maxResults", McpProp("integer", "Most reference vertices to blend. Default 10.")},
		{"noSqueeze", McpProp("boolean", "Don't move vertices inwards. Default false.")},
		{"solidMode", McpProp("boolean", "Move connected vertices together, for rigid parts. Default false.")},
		{"axisX", McpProp("boolean", "Use the X part of the morphs. Default true.")},
		{"axisY", McpProp("boolean", "Use the Y part of the morphs. Default true.")},
		{"axisZ", McpProp("boolean", "Use the Z part of the morphs. Default true.")},
		{"smooth", McpProp("boolean", "Smooth the results. Default false.")},
		{"smoothIterations", McpProp("integer", "Default 2.")},
		{"smoothStrength", McpProp("number", "Default 0.5.")},
		{"fixClipping", McpProp("boolean", "Push the morphs of whole shapes out of the reference after conforming. Default false; prefer fix_clipping on a masked area afterwards, as this can distort meshes.")},
		{"fixClippingStrength", McpProp("number", "Default 0.1.")},
		{"clearMissing",
		 McpProp("boolean",
				 "Clear the data of the conformed sliders that the reference has no data for (noReferenceData in the result) "
				 "from the unmasked vertices, e.g. to drop a slider of another reference from part of a shape. Default false, "
				 "which leaves their data as it is.")},
	});
	conform.handler = [&mcp](const json& args) {
		ConformOptions options;
		options.sliderNames = McpArgs::Strings(args, "sliders");
		for (auto& slider : options.sliderNames)
			CheckSlider(mcp.Project(), slider);
		options.proximityRadius = McpArgs::Float(args, "proximityRadius", options.proximityRadius);
		options.maxResults = McpArgs::Int(args, "maxResults", options.maxResults);
		options.noSqueeze = McpArgs::Bool(args, "noSqueeze", options.noSqueeze);
		options.solidMode = McpArgs::Bool(args, "solidMode", options.solidMode);
		options.axisX = McpArgs::Bool(args, "axisX", options.axisX);
		options.axisY = McpArgs::Bool(args, "axisY", options.axisY);
		options.axisZ = McpArgs::Bool(args, "axisZ", options.axisZ);
		options.smoothResultDeltas = McpArgs::Bool(args, "smooth", options.smoothResultDeltas);
		options.smoothIterations = McpArgs::Int(args, "smoothIterations", options.smoothIterations);
		options.smoothStrength = McpArgs::Float(args, "smoothStrength", options.smoothStrength);
		options.fixClipping = McpArgs::Bool(args, "fixClipping", options.fixClipping);
		options.fixClippingStrength = McpArgs::Float(args, "fixClippingStrength", 0.1f);

		auto shapes = mcp.Shapes(args, "shapes");
		OutfitProject& project = mcp.Project();

		// Sliders the reference has no data for are left as they are, which is easy to miss
		std::vector<std::string> noData;
		if (NiShape* reference = project.GetBaseShape()) {
			for (size_t i = 0; i < project.SliderCount(); i++) {
				if (project.SliderZap(i) || project.SliderUV(i))
					continue;
				const std::string name = project.GetSliderName(i);
				if (!options.sliderNames.empty() && std::find(options.sliderNames.begin(), options.sliderNames.end(), name) == options.sliderNames.end())
					continue;
				TargetDataDiffs* refDiff = project.GetDiffSet(project.activeSet[i], reference);
				if (!refDiff || refDiff->empty())
					noData.push_back(name);
			}
		}

		json names = json::array();
		for (auto* shape : shapes)
			names.push_back(shape->name.get());

		mcp.Apply(mcp.Ops().ConformSliders(shapes, options));

		McpToolResult result;
		result.data["conformed"] = names;
		if (!noData.empty()) {
			result.data["noReferenceData"] = noData;
			// Clearing them makes the shapes follow the reference there too: not at all
			if (McpArgs::Bool(args, "clearMissing", false)) {
				McpRunStep(mcp, {{"type", "ClearSliderData"}, {"TargetMeshes", names}, {"SliderNames", noData}, {"UseMask", true}});
				result.data["cleared"] = noData;
			}
		}
		return result;
	};
	mcp.AddTool(std::move(conform));

	McpTool weights;
	weights.name = "copy_bone_weights";
	weights.description =
		"Copies bone weights from the reference shape to outfit shapes, blending nearby reference vertices. Masked "
		"vertices keep their weights. Bones that references mark as not to copy (named like \"Don't copy this\", e.g. the "
		"placeholder of the UBE curtain references) are skipped, and so are excludeBones. The copied vertices are cleaned to "
		"the game's bone limit and normalized in the same step. One undo takes it all back, bones it added included. The "
		"result lists the bones added and skipped and the weight problems left per shape.";
	weights.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"bones", McpStringList("Bones to copy, all bones of the reference by default")},
		{"excludeBones", McpStringList("Bones of the reference not to copy")},
		{"proximityRadius", McpProp("number", "Search radius for reference vertices. Default 10.")},
		{"maxResults", McpProp("integer", "Most reference vertices to blend. Default 10.")},
		{"copySkinTransform", McpProp("boolean", "Give the shapes the skin transform of the reference. Default false.")},
		{"transformGeometry", McpProp("boolean", "With copySkinTransform, move the geometry along so it stays in place. Default false.")},
		{"cleanup", McpProp("boolean", "Limit the copied vertices to the game's bones per vertex (4, Starfield 8) and normalize them. Default true.")},
		{"removeEmptyBones", McpProp("boolean", "Remove the bones the copy added to a shape that ended up without weights on it, e.g. the leg bones of a top. Default true.")},
	});
	weights.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		AnimInfo* anim = project.GetWorkAnim();
		NiShape* reference = project.GetBaseShape();
		if (!reference)
			throw McpToolError("No reference shape is loaded.");

		BoneWeightCopyOptions options;
		options.selectedBones = McpArgs::Strings(args, "bones");
		options.proximityRadius = McpArgs::Float(args, "proximityRadius", options.proximityRadius);
		options.maxResults = McpArgs::Int(args, "maxResults", options.maxResults);
		options.copySkinTransform = McpArgs::Bool(args, "copySkinTransform", false);
		options.transformGeometry = McpArgs::Bool(args, "transformGeometry", false);

		// Placeholder bones of references and excluded bones stay behind
		std::vector<std::string> exclude = McpArgs::Strings(args, "excludeBones");
		std::vector<std::string> referenceBones = anim->shapeBones[reference->name.get()];
		std::vector<std::string> skipped;
		for (auto& bone : referenceBones) {
			wxString lower = wxString::FromUTF8(bone).Lower();
			lower.Replace("'", "");
			const bool placeholder = lower.Contains("dont copy") || lower.Contains("do not copy");
			if (placeholder || std::find(exclude.begin(), exclude.end(), bone) != exclude.end())
				skipped.push_back(bone);
		}
		if (!skipped.empty()) {
			std::vector<std::string>& selected = options.selectedBones;
			if (selected.empty())
				selected = referenceBones;
			selected.erase(std::remove_if(selected.begin(), selected.end(), [&](const std::string& b) { return std::find(skipped.begin(), skipped.end(), b) != skipped.end(); }),
						   selected.end());
			if (selected.empty())
				throw McpToolError("No bones are left to copy after skipping " + json(skipped).dump() + ".");
		}

		auto shapes = mcp.Shapes(args, "shapes");
		std::map<std::string, std::vector<std::string>> bonesBefore;
		for (auto* shape : shapes)
			bonesBefore[shape->name.get()] = anim->shapeBones[shape->name.get()];

		// Recorded like the window does, so it can be undone
		UndoHistory& history = frame->editor.undoHistory;
		UndoStateProject* usp = history.PushState();
		OpResult result = mcp.Ops().CopyBoneWeights(shapes, options, *usp);
		if (!result.ok || usp->usss.empty()) {
			history.PopState();
			usp = nullptr;
		}

		// Cleanup of the copied vertices, folded into the same undo step: drop the smallest weights over the
		// game's limit and normalize
		const auto game = static_cast<TargetGame>(Config.GetIntValue("TargetGame"));
		const size_t limit = game == SF ? 8 : 4;
		size_t cleaned = 0;
		if (usp && result.ok && McpArgs::Bool(args, "cleanup", true)) {
			for (auto& uss : usp->usss) {
				std::set<uint16_t> vertices;
				for (auto& bw : uss.boneWeights)
					for (auto& w : bw.weights)
						vertices.insert(w.first);

				std::vector<std::string> bones = anim->shapeBones[uss.shapeName];
				for (uint16_t v : vertices) {
					std::vector<std::pair<float, std::string>> list;
					for (auto& bone : bones) {
						auto weightsPtr = anim->GetWeightsPtr(uss.shapeName, bone);
						if (!weightsPtr)
							continue;
						auto it = weightsPtr->find(v);
						if (it != weightsPtr->end() && it->second > 0.0f)
							list.emplace_back(it->second, bone);
					}
					if (list.empty())
						continue;

					std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
					std::map<std::string, float> goal;
					float sum = 0.0f;
					for (size_t i = 0; i < list.size() && i < limit; i++)
						sum += list[i].first;
					for (size_t i = 0; i < list.size(); i++)
						goal[list[i].second] = i < limit && sum > 0.0f ? list[i].first / sum : 0.0f;

					bool changed = false;
					for (auto& item : list) {
						const float target = goal[item.second];
						if (std::fabs(target - item.first) < 1e-6f)
							continue;
						changed = true;

						auto bw = std::find_if(uss.boneWeights.begin(), uss.boneWeights.end(), [&](const UndoStateBoneWeights& b) { return b.boneName == item.second; });
						if (bw == uss.boneWeights.end()) {
							UndoStateBoneWeights added;
							added.boneName = item.second;
							auto& before = bonesBefore[uss.shapeName];
							added.addedBone = std::find(before.begin(), before.end(), item.second) == before.end();
							uss.boneWeights.push_back(std::move(added));
							bw = std::prev(uss.boneWeights.end());
						}
						auto entry = bw->weights.find(v);
						if (entry == bw->weights.end())
							bw->weights[v] = {item.first, target};
						else
							entry->second.endVal = target;
					}
					cleaned += changed ? 1 : 0;
				}
			}
			if (cleaned)
				result.refresh |= mcp.Ops().ApplyShapeEdits(*usp, false);
		}

		// Bones the copy brought to a shape that ended up without weights there, e.g. the leg bones of a top, only
		// clutter it. Undo and redo leave them out too. The cleanup above already drops those it emptied.
		size_t emptyRemoved = 0;
		if (usp && result.ok && McpArgs::Bool(args, "removeEmptyBones", true)) {
			for (auto* shape : shapes) {
				const std::string name = shape->name.get();
				const std::vector<std::string> bones = anim->shapeBones[name];
				for (auto& bone : bones) {
					if (std::find(bonesBefore[name].begin(), bonesBefore[name].end(), bone) != bonesBefore[name].end())
						continue;
					auto weightsPtr = anim->GetWeightsPtr(name, bone);
					if (weightsPtr && !weightsPtr->empty())
						continue;
					anim->RemoveShapeBone(name, bone);
					result.refresh |= RefreshFlags::PendingChanges;
				}
			}
		}
		// Bones the copy had for a shape that aren't on it now, whether the cleanup or this left them out
		if (usp) {
			for (auto& uss : usp->usss) {
				std::set<std::string> counted;
				const auto& now = anim->shapeBones[uss.shapeName];
				for (auto& bw : uss.boneWeights)
					if (bw.addedBone && std::find(now.begin(), now.end(), bw.boneName) == now.end() && counted.insert(bw.boneName).second)
						emptyRemoved++;
			}
		}

		frame->UpdateUndoTools();
		mcp.Apply(result);
		frame->RefreshGUIWeightColors();

		McpToolResult tool;
		for (auto* shape : shapes) {
			const std::string name = shape->name.get();
			json added = json::array();
			for (auto& bone : anim->shapeBones[name])
				if (std::find(bonesBefore[name].begin(), bonesBefore[name].end(), bone) == bonesBefore[name].end())
					added.push_back(bone);

			MeshAnalysis::WeightStats stats;
			mcp.Ops().AnalyzeWeights(shape, stats, static_cast<int>(limit));
			tool.data["shapes"][name] = {
				{"bonesAdded", added},
				{"unweighted", stats.unweighted},
				{"overInfluenceLimit", stats.overInfluenceLimit},
				{"badSum", stats.badSum},
			};
			// Vertices with no reference vertex within the radius lose their weights, e.g. the hem of a long skirt
			if (stats.unweighted)
				tool.data["shapes"][name]["hint"] = "Vertices farther than proximityRadius from the reference are left without weights. Undo and copy again with a larger proximityRadius (a long dress needed 25).";
		}
		if (!skipped.empty())
			tool.data["skippedBones"] = skipped;
		tool.data["cleanedVertices"] = cleaned;
		if (emptyRemoved)
			tool.data["emptyBonesRemoved"] = emptyRemoved;
		return tool;
	};
	mcp.AddTool(std::move(weights));

	McpTool clipping;
	clipping.name = "fix_clipping";
	clipping.description =
		"Pushes outfit shapes out of the reference shape where they go into it. Mode \"shapes\" fixes the base "
		"geometry with the current slider values, \"sliders\" fixes the morph of each slider on its own. Masked "
		"vertices are left alone and partly masked ones take part of the fix. The fix can distort meshes: start with a low "
		"strength and only fix the affected area, by masking everything else with a smooth border (set_mask on the clipping "
		"vertices, mask_ops grow, mask_ops smooth, then mask_ops invert), instead of fixing whole shapes. Check the result "
		"with quality_sweep or analyze_clipping and raise the strength step by step. Mode \"combination\" fixes clipping "
		"that only a combination of sliders has (a body preset, several big sliders), which no slider has on its own: "
		"the push is shared among the sliders of the combination (or the ones in 'sliders') by how far each moves the "
		"vertex. Vertices of analysis_exclusions are always left alone. auto_fix_clipping runs the whole masked loop.";
	clipping.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"mode", {{"type", "string"}, {"enum", {"shapes", "sliders", "combination"}}, {"description", "Default shapes"}}},
		{"strength", McpProp("number", "How far to push out, 0 to 1. Default 0.1; raise it in small steps.")},
		{"sliders", McpStringList("Sliders mode: only these sliders. Combination mode: the sliders that take the fix, default all of the combination.")},
		{"sliderValues", McpNumberMap("Combination mode: the combination, e.g. the values of apply_preset. The other sliders are at 0.")},
	});
	const json presetProperties = McpPresetValueProperties();
	for (auto& property : presetProperties.items())
		clipping.inputSchema["properties"][property.key()] = property.value();
	clipping.handler = [&mcp](const json& args) {
		std::string mode = McpArgs::String(args, "mode", "shapes");
		if (mode != "shapes" && mode != "sliders" && mode != "combination")
			McpArgs::Wrong("mode", "shapes, sliders or combination");

		auto sliders = McpArgs::Strings(args, "sliders");
		for (auto& slider : sliders)
			CheckSlider(mcp.Project(), slider);

		auto shapes = mcp.Shapes(args, "shapes");
		const float strength = McpArgs::Float(args, "strength", 0.1f);
		McpExclusionMaskScope protect(mcp, shapes);

		McpToolResult result;
		if (mode == "combination") {
			auto values = McpSliderValuesArg(mcp, args);
			if (values.empty())
				McpArgs::Wrong("sliderValues", "the slider values of the combination, or a preset");
			size_t changed = 0;
			mcp.Apply(mcp.Ops().FixClippingCombination(shapes, values, sliders, strength, &changed));
			result.data["changedVertices"] = changed;
		}
		else
			mcp.Apply(mcp.Ops().FixClipping(shapes, mode == "sliders" ? ClippingFixMode::Sliders : ClippingFixMode::Shapes, strength, sliders));
		return result;
	};
	mcp.AddTool(std::move(clipping));

	McpTool zaps;
	zaps.name = "transfer_zaps";
	zaps.description =
		"Gives a shape the zaps of a shape in a set of another slider set file, by closest surface point, e.g. the BodyZap "
		"of the original project's body for the new body of a conversion. Zap sliders this project doesn't have are "
		"created with the source's settings; the shape's data of the zaps is replaced. load_reference already does this "
		"for the old reference when it's replaced.";
	zaps.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "Slider set file with the source shape (absolute or relative to the BodySlide folder)")},
			{"set", McpProp("string", "Set in the file")},
			{"shape", McpProp("string", "Source shape, by shape or target name, e.g. the body of the original project")},
			{"sliders", McpStringList("Zap sliders to transfer. Default all zaps with data on the source shape.")},
			{"target", McpProp("string", "Shape here to zap. Default the reference shape.")},
			{"maxDistance", McpProp("number", "How far to look for the source surface. Default 10.")},
		},
		{"file", "set", "shape"});
	zaps.handler = [&mcp](const json& args) {
		std::string file = AutomationRunner::MakeAbsoluteToProject(wxString::FromUTF8(McpArgs::String(args, "file"))).ToUTF8().data();
		if (!wxFileName::FileExists(wxString::FromUTF8(file)))
			throw McpToolError("There is no file '" + file + "'.");

		NiShape* target = McpArgs::Has(args, "target") ? mcp.Shape(McpArgs::String(args, "target")) : mcp.Project().GetBaseShape();
		if (!target)
			throw McpToolError("No reference shape is loaded, name the shape to zap in 'target'.");

		ProjectOperations ops = mcp.Ops();
		ZapSource source;
		mcp.Apply(ops.CaptureZapsFromProject(file, McpArgs::String(args, "set"), McpArgs::String(args, "shape"), McpArgs::Strings(args, "sliders"), source));
		if (source.zaps.empty())
			throw McpToolError("The source shape has no zap data" + std::string(McpArgs::Has(args, "sliders") ? " for those sliders." : "."));

		std::vector<ZapTransfer> transfers;
		mcp.Apply(ops.ApplyZaps(source, target, McpArgs::Float(args, "maxDistance", 10.0f), transfers));

		json list = json::array();
		for (auto& t : transfers) {
			json entry = {{"slider", t.slider}, {"sourceZapped", t.sourceZapped}, {"zapped", t.zapped}};
			if (t.created)
				entry["created"] = true;
			list.push_back(entry);
		}

		McpToolResult result;
		result.data["target"] = target->name.get();
		result.data["sourceVertices"] = source.verts.size();
		result.data["targetVertices"] = target->GetNumVertices();
		result.data["zaps"] = list;
		return result;
	};
	mcp.AddTool(std::move(zaps));

	McpTool listPoses;
	listPoses.name = "list_poses";
	listPoses.description = "Lists the poses set_preview_pose can show.";
	listPoses.readOnly = true;
	listPoses.handler = [frame](const json&) {
		json names = json::array();
		for (auto& pose : frame->poseDataCollection.poseData)
			names.push_back(pose.name);

		McpToolResult result;
		result.data["poses"] = names;
		return result;
	};
	mcp.AddTool(std::move(listPoses));

	McpTool pose;
	pose.name = "set_preview_pose";
	pose.description =
		"Poses the skeleton to look at the outfit in a pose, e.g. to check clipping when moving. The shapes stay "
		"unposed in the project. Start from a pose of list_poses, a pose file, the current pose (keepCurrent) or "
		"the unposed skeleton, and change single bones with 'bones'. Bones that 'bones' leaves out keep the start "
		"pose. get_pose shows the result, save_pose saves it.";
	pose.inputSchema = McpSchema({
		{"pose", McpProp("string", "Start from this pose of list_poses")},
		{"file", McpProp("string", "Start from a .hkx, .json, .yaml or .yml pose file (absolute or relative to the BodySlide folder)")},
		{"keepCurrent", McpProp("boolean", "Without pose or file: start from the current pose instead of the unposed skeleton. Default false.")},
		{"bones",
		 {{"type", "object"},
		  {"additionalProperties",
		   {{"type", "object"},
			{"properties", {{"rotation", {{"type", "array"}, {"items", {{"type", "number"}}}}}, {"translation", {{"type", "array"}, {"items", {{"type", "number"}}}}}, {"scale", {{"type", "number"}}}}}}},
		  {"description", "Bone name to {rotation, translation: [x, y, z], scale} relative to the bone's unposed transform. "
						  "The rotation is the rotation axis scaled by the angle in degrees, e.g. [0, 0, 45] turns 45 degrees around the bone's Z axis. "
						  "Fields that are left out keep the start pose."}}},
		{"worldRotation",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "array"}, {"items", {{"type", "number"}}}}},
		  {"description", "Bone name to a turn about the skeleton's global axes, applied after 'bones' and parents first: the rotation "
						  "axis scaled by the angle in degrees. Global axes: +X is the character's right, +Y her front, +Z up. Easier than "
						  "'bones' for posing by eye, as each bone's own axes point in different directions."}}},
		{"aim",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "array"}, {"items", {{"type", "number"}}}}},
		  {"description", "Bone name to a global direction [x, y, z] to point the bone in, towards its next joint (e.g. the "
						  "upper arm towards the elbow), with the smallest turn. Applied with worldRotation, parents first. "
						  "get_pose with withTransforms shows the directions bones point in now."}}},
	});
	pose.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		ProjectOperations ops = mcp.Ops();

		PoseData base;
		bool hasBase = false;
		if (McpArgs::Has(args, "pose")) {
			std::string name = McpArgs::String(args, "pose");
			auto& poses = frame->poseDataCollection.poseData;
			auto it = std::find_if(poses.begin(), poses.end(), [&](const PoseData& p) { return p.name == name; });
			if (it == poses.end())
				throw McpToolError("There is no pose '" + name + "'. list_poses lists them.");
			base = *it;
			hasBase = true;
		}
		else if (McpArgs::Has(args, "file")) {
			std::string file = ProjectFile(McpArgs::String(args, "file"));
			if (!wxFileName::FileExists(wxString::FromUTF8(file)))
				throw McpToolError("There is no file '" + file + "'.");

			std::string skeletonHkx;
			if (PoseDataCollection::GetPoseFileFormat(file) == PoseFileFormat::Hkx) {
				wxString error;
				if (!PoseDataCollection::FindReferenceSkeletonHkx(skeletonHkx, error))
					throw McpToolError(error.ToUTF8().data());
			}

			std::string error;
			if (!PoseDataCollection::LoadPoseFile(file, base, skeletonHkx, &error))
				throw McpToolError(error.empty() ? "The pose file couldn't be loaded." : error);
			hasBase = true;
		}

		if (hasBase) {
			ops.ResetPose();
			base.ApplyToSkeleton();
		}
		else if (!McpArgs::Bool(args, "keepCurrent", false) || !project.bPose)
			ops.ResetPose();

		// The start pose relative to the unposed skeleton, with the bones changed on top
		PoseData data;
		PoseDataCollection::CaptureCurrentPose("MCP", false, data);

		if (const json* bones = McpArgs::Find(args, "bones")) {
			if (!bones->is_object())
				McpArgs::Wrong("bones", "an object of bone names to transforms");

			for (auto& item : bones->items()) {
				auto bone = std::find_if(data.boneData.begin(), data.boneData.end(), [&](const PoseBoneData& b) { return b.name == item.key(); });
				if (bone == data.boneData.end())
					throw McpToolError("There is no bone '" + item.key() + "' in the skeleton. list_shapes with withBones lists the bones of the shapes.");

				const json& t = item.value();
				if (!t.is_object())
					throw McpToolError("The transform of '" + item.key() + "' has to be an object.");

				// A rotation vector (axis times angle) in radians, given in degrees
				if (t.contains("rotation"))
					bone->rotation = VectorFromJson(t["rotation"], "The rotation of " + item.key()) * DEG2RAD;
				if (t.contains("translation"))
					bone->translation = VectorFromJson(t["translation"], "The translation of " + item.key());
				if (t.contains("scale") && t["scale"].is_number())
					bone->scale = t["scale"].get<float>();
			}
		}

		data.ApplyToSkeleton();

		// Turns about the skeleton's global axes and aims, parents first, so a child turns from where its
		// parent put it
		struct Turn {
			AnimBone* bone = nullptr;
			Vector3 value;
			bool aim = false;
		};
		std::vector<Turn> turns;

		auto collect = [&](const char* name, bool aim) {
			const json* list = McpArgs::Find(args, name);
			if (!list)
				return;
			if (!list->is_object())
				McpArgs::Wrong(name, "an object of bone names to [x, y, z]");

			for (auto& item : list->items()) {
				AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(item.key());
				if (!bone)
					throw McpToolError("There is no bone '" + item.key() + "' in the skeleton.");
				Vector3 value = VectorFromJson(item.value(), std::string(name) + " of " + item.key());
				turns.push_back({bone, aim ? value : value * DEG2RAD, aim});
			}
		};
		collect("worldRotation", false);
		collect("aim", true);

		auto depth = [](AnimBone* bone) {
			int d = 0;
			for (AnimBone* p = bone->parent; p; p = p->parent)
				d++;
			return d;
		};
		std::stable_sort(turns.begin(), turns.end(), [&](const Turn& a, const Turn& b) { return depth(a.bone) < depth(b.bone); });

		for (auto& turn : turns)
			TurnBoneGlobal(turn.bone, turn.aim ? AimRotation(turn.bone, turn.value) : turn.value);

		PoseData posed;
		PoseDataCollection::CaptureCurrentPose("MCP", false, posed);
		frame->ApplyRefresh(ops.SetPreviewPose(posed));
		return GetPose(true);
	};
	mcp.AddTool(std::move(pose));

	McpTool getPose;
	getPose.name = "get_pose";
	getPose.description =
		"Returns the bones of the current preview pose that differ from the unposed skeleton, in the form "
		"set_preview_pose takes them: rotation as axis times angle in degrees, translation and scale. With "
		"withTransforms, also where bones are in the skeleton's global space (+X the character's right, +Y her front, "
		"+Z up) and which way they point towards their next joint.";
	getPose.inputSchema = McpSchema({
		{"withTransforms", McpProp("boolean", "Also return global positions and directions. Default false.")},
		{"bones", McpStringList("With withTransforms: only these bones, all NPC bones by default")},
	});
	getPose.readOnly = true;
	getPose.handler = [&mcp](const json& args) { return GetPose(mcp.Project().bPose, args); };
	mcp.AddTool(std::move(getPose));

	McpTool savePose;
	savePose.name = "save_pose";
	savePose.description =
		"Saves the current preview pose. With 'name' it goes to Outfit Studio's pose list (the PoseData folder). "
		"With 'file' it's written as a pose file chosen by the extension: .hkx (a one-frame Havok animation for "
		"Skyrim LE, SE, VR or Fallout 4, which needs the game's skeleton .hkx next to the reference skeleton), "
		".json (SAF/SAM) or .yaml/.yml (SAM). Existing poses and files are only replaced with overwrite: true.";
	savePose.inputSchema = McpSchema({
		{"name", McpProp("string", "Name in the pose list")},
		{"file", McpProp("string", "Pose file to write (absolute or relative to the BodySlide folder)")},
		{"overwrite", McpProp("boolean", "Replace a pose or file that exists. Default false.")},
	});
	savePose.handler = [&mcp, frame](const json& args) {
		if (!mcp.Project().bPose)
			throw McpToolError("No preview pose is shown. Set one with set_preview_pose first.");

		const bool overwrite = McpArgs::Bool(args, "overwrite", false);
		const bool hasName = McpArgs::Has(args, "name");
		const bool hasFile = McpArgs::Has(args, "file");
		if (!hasName && !hasFile)
			throw McpToolError("Give a name for the pose list, a file, or both.");

		McpToolResult result;
		if (hasName) {
			std::string name = McpArgs::String(args, "name");
			if (name.empty())
				throw McpToolError("The pose name is empty.");

			auto& poses = frame->poseDataCollection.poseData;
			auto existing = std::find_if(poses.begin(), poses.end(), [&](const PoseData& p) { return p.name == name; });
			if (existing != poses.end() && !overwrite)
				throw McpToolError("There already is a pose '" + name + "'. Pass overwrite: true to replace it.");

			std::string error = frame->SaveCurrentPose(name);
			if (!error.empty())
				throw McpToolError(error);
			result.data["name"] = name;
		}

		if (hasFile) {
			std::string file = ProjectFile(McpArgs::String(args, "file"));
			if (wxFileName::FileExists(wxString::FromUTF8(file)) && !overwrite)
				throw McpToolError("The file '" + file + "' exists. Pass overwrite: true to replace it.");

			std::string error = frame->SaveCurrentPoseFile(file);
			if (!error.empty())
				throw McpToolError(error);
			result.data["file"] = file;
		}

		return result;
	};
	mcp.AddTool(std::move(savePose));

	McpTool clearPose;
	clearPose.name = "clear_preview_pose";
	clearPose.description = "Shows the shapes unposed again.";
	clearPose.handler = [&mcp, frame](const json&) {
		ProjectOperations ops = mcp.Ops();
		ops.ResetPose();
		frame->ApplyRefresh(ops.ClearPreviewPose());
		return McpToolResult();
	};
	mcp.AddTool(std::move(clearPose));
}

void AddHistoryTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	auto undoRedo = [frame](bool undo) {
		if (!frame->glView)
			throw McpToolError("The view isn't available.");
		if (frame->glView->GetSegmentMode())
			throw McpToolError("Undo isn't available while editing segments.");

		// The window's first undo of a step in another mode (slider edit, slider value) only switches to that
		// mode, so the user sees what the next press undoes. An agent wants the step undone in one call.
		UndoHistory& history = frame->editor.undoHistory;
		auto step = [&]() { return undo ? history.GetCurState() : history.GetNextState(); };
		UndoStateProject* target = step();

		bool done = undo ? frame->glView->UndoStroke() : frame->glView->RedoStroke();
		if (done && target && step() == target)
			done = undo ? frame->glView->UndoStroke() : frame->glView->RedoStroke();

		frame->UpdateUndoTools();
		if (!done)
			throw McpToolError(undo ? "There is nothing to undo." : "There is nothing to redo.");
		frame->SetPendingChanges();

		McpToolResult result;
		result.data["editingSlider"] = frame->editor.bEditSlider ? json(frame->editor.activeSlider) : json(nullptr);
		return result;
	};

	McpTool undo;
	undo.name = "undo";
	undo.description =
		"Undoes the last brush stroke, weight copy or other step of the window's undo history. A step of a slider's morph "
		"enters edit mode for that slider, like undo in the window; editingSlider says which.";
	undo.handler = [undoRedo](const json&) { return undoRedo(true); };
	mcp.AddTool(std::move(undo));

	McpTool redo;
	redo.name = "redo";
	redo.description = "Redoes the last undone step.";
	redo.handler = [undoRedo](const json&) { return undoRedo(false); };
	mcp.AddTool(std::move(redo));

	McpTool save;
	save.name = "snapshot_save";
	save.description =
		"Keeps a copy of the whole project (shapes, sliders, weights, reference, masks) under a name, to go back to "
		"with snapshot_restore after trying something, or to compare with snapshot_diff. Up to 10 are kept while the "
		"window is open; a name that exists is replaced. Tools the window can't undo also take automatic snapshots "
		"(auto-1, auto-2, ...), the last 3 are kept.";
	save.inputSchema = McpSchema({{"name", McpProp("string", "Name of the snapshot")}}, {"name"});
	save.handler = [&mcp](const json& args) {
		mcp.Snapshots().Save(mcp, McpArgs::String(args, "name"));

		McpToolResult result;
		result.data["snapshots"] = mcp.Snapshots().Names();
		return result;
	};
	mcp.AddTool(std::move(save));

	McpTool restore;
	restore.name = "snapshot_restore";
	restore.description =
		"Brings the project back to a snapshot, a named one or an automatic one (auto-N). The undo history is cleared, "
		"the snapshot is kept, and an automatic snapshot of the state before is taken.";
	restore.inputSchema = McpSchema({{"name", McpProp("string", "Name of the snapshot")}}, {"name"});
	restore.handler = [&mcp, frame](const json& args) {
		std::string name = McpArgs::String(args, "name");
		McpSnapshotStore& store = mcp.Snapshots();
		store.Track(&mcp.Project());
		if (!store.Find(name))
			throw McpToolError("There is no snapshot '" + name + "'. Snapshots: " + store.Names().dump());

		// The state before, to go back to; restoring clears the undo history
		std::string before = store.SaveAutomatic(mcp, "snapshot_restore", name);

		frame->RestoreEditSnapshot(*store.Find(name)->snapshot);
		frame->SetPendingChanges();

		McpToolResult result;
		result.data["autoSnapshot"] = before;
		return result;
	};
	mcp.AddTool(std::move(restore));

	McpTool listSnapshots;
	listSnapshots.name = "snapshot_list";
	listSnapshots.description = "Lists the snapshots, and the automatic ones with the tool each was taken before.";
	listSnapshots.readOnly = true;
	listSnapshots.handler = [&mcp, frame](const json&) {
		McpSnapshotStore& store = mcp.Snapshots();
		store.Track(frame->project);
		McpToolResult result;
		result.data["snapshots"] = store.Names();
		result.data["automatic"] = store.Automatic();
		return result;
	};
	mcp.AddTool(std::move(listSnapshots));

	McpTool deleteSnapshot;
	deleteSnapshot.name = "snapshot_delete";
	deleteSnapshot.description = "Deletes a snapshot.";
	deleteSnapshot.inputSchema = McpSchema({{"name", McpProp("string", "Name of the snapshot")}}, {"name"});
	deleteSnapshot.handler = [&mcp](const json& args) {
		std::string name = McpArgs::String(args, "name");
		if (!mcp.Snapshots().Remove(name))
			throw McpToolError("There is no snapshot '" + name + "'.");

		McpToolResult result;
		result.data["snapshots"] = mcp.Snapshots().Names();
		return result;
	};
	mcp.AddTool(std::move(deleteSnapshot));

	McpTool diff;
	diff.name = "snapshot_diff";
	diff.description =
		"What changed since a snapshot, to check an edit did what it should and nothing else: shapes and sliders added "
		"or removed, the reference, and per shape the vertex and triangle counts, moved vertices and the largest move, "
		"changed UVs, the masked vertex count, bones added, removed or reweighted, and the sliders whose morph changed "
		"with their moved vertices and largest delta before and after. Compares with the project now, or with another "
		"snapshot.";
	diff.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "Snapshot to compare from, e.g. auto-3 or one of snapshot_save")},
			{"to", McpProp("string", "Snapshot to compare to. Default the project now.")},
			{"limit", McpProp("integer", "Most changed sliders to list per shape. Default 25.")},
		},
		{"name"});
	diff.outputSchema = {
		{"type", "object"},
		{"properties",
		 {{"changed", {{"type", "boolean"}}},
		  {"shapesAdded", {{"type", "array"}, {"items", {{"type", "string"}}}}},
		  {"shapesRemoved", {{"type", "array"}, {"items", {{"type", "string"}}}}},
		  {"slidersAdded", {{"type", "array"}, {"items", {{"type", "string"}}}}},
		  {"slidersRemoved", {{"type", "array"}, {"items", {{"type", "string"}}}}},
		  {"shapes", {{"type", "object"}, {"additionalProperties", {{"type", "object"}}}}}}},
	};
	diff.readOnly = true;
	diff.handler = [&mcp](const json& args) {
		McpSnapshotStore& store = mcp.Snapshots();
		store.Track(&mcp.Project());

		auto find = [&store](const std::string& name) {
			McpSnapshotStore::Entry* entry = store.Find(name);
			if (!entry)
				throw McpToolError("There is no snapshot '" + name + "'. Snapshots: " + store.Names().dump());
			return entry;
		};

		std::string from = McpArgs::String(args, "name");
		const McpProjectDigest& before = find(from)->digest;
		McpProjectDigest now;
		const McpProjectDigest* after = &now;
		if (McpArgs::Has(args, "to"))
			after = &find(McpArgs::String(args, "to"))->digest;
		else
			now = McpDigest(mcp);

		const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 1, 1000));
		McpToolResult result;
		result.data = McpDigestDiff(before, *after, limit);
		result.data["from"] = from;
		result.data["to"] = McpArgs::String(args, "to", "now");
		return result;
	};
	mcp.AddTool(std::move(diff));
}
