/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "McpSnapshots.h"
#include "../program/AutomationRunner.h"
#include "../components/ClippingFixer.h"
#include "../components/PoseMath.h"
#include "../components/SliderPresets.h"
#include "../components/StarfieldSpace.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/ProjectUtil.h"
#include "../program/OutfitStudio.h"

#include <wx/filename.h>

#include <algorithm>
#include <cfloat>
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

double Degrees(float radians) {
	return std::round(radians / DEG2RAD * 10.0f) / 10.0;
}

std::vector<AnimBone*> SkeletonBones() {
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	std::vector<std::string> names;
	skeleton.GetBoneNames(names);

	std::vector<AnimBone*> bones;
	for (auto& name : names)
		if (AnimBone* bone = skeleton.GetBonePtr(name))
			bones.push_back(bone);
	return bones;
}

AnimBone* SkeletonBone(const std::string& name) {
	AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
	if (!bone)
		throw McpToolError("There is no bone '" + name + "' in the skeleton. list_skeleton_bones lists them.");
	return bone;
}

int BoneDepth(const AnimBone* bone) {
	int depth = 0;
	for (AnimBone* p = bone->parent; p; p = p->parent)
		depth++;
	return depth;
}

bool HasAxis(const AnimBone* bone) {
	// A bone with no joint below, like the head, turns about the direction of its parent
	Vector3 axis;
	return PoseMath::BoneAxis(bone, axis);
}

// A position given in mesh units, in the skeleton's units
Vector3 ToBoneUnits(const Vector3& v, float units) {
	return v / units;
}

// Where a bone is (in mesh units) and which way it points in the skeleton's global space, with the pose, and how its
// joint is twisted and bent
json BoneTransform(AnimBone* bone, const PoseMath::TwistContext& context, float units) {
	json entry = {{"position", VectorToJson(bone->xformPoseToGlobal.translation * units)}};

	Vector3 direction;
	if (HasAxis(bone) && PoseMath::BoneAxis(bone, direction)) {
		entry["direction"] = VectorToJson(direction);
		if (PoseMath::JointParent(bone))
			entry["bend"] = Degrees(PoseMath::BendAngle(bone));
	}
	entry["twist"] = Degrees(PoseMath::SegmentTwist(bone, context));
	if (PoseMath::GetBoneRole(bone->boneName).role.find("hinge") != std::string::npos && PoseMath::BendsBackwards(bone))
		entry["bendsBackwards"] = true;
	return entry;
}

bool IsLimbRole(const std::string& role) {
	for (const char* limb : {"upper arm", "forearm", "hand", "thigh", "calf"})
		if (role.rfind(limb, 0) == 0)
			return true;
	return false;
}

// Joints of the pose that deform the mesh badly: twisted limbs, hinges bent backwards or folded flat
json PoseWarnings(const PoseMath::TwistContext& context) {
	json warnings = json::array();
	for (AnimBone* bone : PoseMath::MainBones(context.bones)) {
		if (bone->poseRotVec.IsZero(true))
			continue;

		// A twist spread over the twist bones of the limb deforms well up to a point, one on the bone alone doesn't
		const std::string role = PoseMath::GetBoneRole(bone->boneName).role;
		const double twist = Degrees(PoseMath::SegmentTwist(bone, context));
		const bool spread = !PoseMath::SegmentTwistBones(bone, context).empty() && PoseMath::TwistSpreadError(bone, context) < 10.0f * DEG2RAD;
		if (IsLimbRole(role) && std::abs(twist) > (spread ? 150.0 : 60.0))
			warnings.push_back(bone->boneName + " is twisted " + wxString::Format("%g", twist).ToStdString() + " degrees about its own length"
							   + (spread ? ", more than the skin can follow." : " without its twist bones, which wrings the skin.")
							   + " Set it with twist (spread over the twist bones), aim with a twist, or solve the limb with reach and another pole.");

		if (role.find("hinge") == std::string::npos)
			continue;
		if (PoseMath::BendsBackwards(bone))
			warnings.push_back(bone->boneName + " bends backwards, against the way the joint bends. Bend it the other way or solve the limb with reach and a pole.");
		const double bend = Degrees(PoseMath::BendAngle(bone));
		if (bend > 150.0)
			warnings.push_back(bone->boneName + " is folded " + wxString::Format("%g", bend).ToStdString() + " degrees, more than the joint can.");
	}
	return warnings;
}

// The body's mass center over the feet, with the current pose: the reference shape, or all skinned shapes when the
// body is part of the outfit
json Balance(OutfitStudioMcp& mcp) {
	OutfitProject& project = mcp.Project();
	std::vector<NiShape*> bodies;
	for (auto* shape : project.GetWorkNif()->GetShapes())
		if (project.IsBaseShape(shape))
			bodies.push_back(shape);
	const bool reference = !bodies.empty();
	if (!reference)
		for (auto* shape : project.GetWorkNif()->GetShapes())
			if (shape->IsSkinned())
				bodies.push_back(shape);

	Vector3 center;
	size_t count = 0;
	for (NiShape* body : bodies) {
		std::vector<Vector3> verts = McpGlobalLiveVerts(mcp, body);
		for (auto& v : verts)
			center += v;
		count += verts.size();
	}
	if (count == 0)
		throw McpToolError("balance has no skinned shape to weigh.");
	center /= static_cast<float>(count);

	const float units = McpBoneUnits(mcp);
	std::vector<Vector3> feet;
	for (AnimBone* bone : PoseMath::MainBones(SkeletonBones()))
		if (PoseMath::GetBoneRole(bone->boneName).role.rfind("foot", 0) == 0)
			feet.push_back(bone->xformPoseToGlobal.translation * units);

	json balance = {{"centerOfMass", VectorToJson(center)}};
	if (feet.empty())
		return balance;

	Vector3 support;
	float spread = 0.0f;
	for (auto& foot : feet)
		support += foot;
	support /= static_cast<float>(feet.size());
	for (auto& foot : feet)
		spread = std::max(spread, std::hypot(foot.x - support.x, foot.y - support.y));

	const float offsetX = center.x - support.x;
	const float offsetY = center.y - support.y;
	balance["feetCenter"] = VectorToJson(support);
	balance["offset"] = json::array({offsetX, offsetY});
	// Standing needs the mass over the feet, give or take a foot's length
	balance["balanced"] = std::hypot(offsetX, offsetY) <= spread + 5.0f;
	balance["note"] = "offset is the mass center minus the middle of the feet, in x (her right) and y (her front).";
	return balance;
}

// How close shapes come to each other with the current pose, e.g. a hand to the head
json Contacts(OutfitStudioMcp& mcp, const json& list) {
	if (!list.is_array())
		McpArgs::Wrong("contacts", "a list of {from, to}");

	json contacts = json::array();
	for (auto& item : list) {
		NiShape* from = mcp.Shape(McpArgs::String(item, "from"));
		NiShape* to = mcp.Shape(McpArgs::String(item, "to"));

		// In global space: a head's vertices are in its own space, a hand's in the skeleton's
		std::vector<Triangle> triangles;
		to->GetTriangles(triangles);
		std::vector<float> distances = ClippingFixer::SignedDistancesToBody(McpGlobalLiveVerts(mcp, to), triangles, McpGlobalLiveVerts(mcp, from), 20.0f);

		float closest = FLT_MAX;
		int closestVertex = -1;
		size_t penetrating = 0;
		size_t touching = 0;
		for (size_t v = 0; v < distances.size(); v++) {
			const float d = distances[v];
			if (d == FLT_MAX)
				continue;
			if (d < closest) {
				closest = d;
				closestVertex = static_cast<int>(v);
			}
			if (d < 0.0f)
				penetrating++;
			else if (d < 0.5f)
				touching++;
		}

		json entry = {{"from", from->name.get()}, {"to", to->name.get()}, {"penetrating", penetrating}, {"touching", touching}};
		if (closestVertex >= 0) {
			entry["closestDistance"] = std::round(closest * 100.0f) / 100.0f;
			entry["closestVertex"] = closestVertex;
		}
		else
			entry["closestDistance"] = nullptr;
		contacts.push_back(entry);
	}
	return contacts;
}

// The bones of the current pose that differ from the unposed skeleton, as set_preview_pose takes them.
// With withTransforms, also the global position and direction of the bones.
McpToolResult GetPose(OutfitStudioMcp& mcp, bool posed, const json& args = json::object()) {
	McpToolResult result;
	result.data["posed"] = posed;

	if (McpArgs::Bool(args, "withTransforms", false)) {
		const PoseMath::TwistContext context = McpTwistContext(mcp);
		std::vector<std::string> names = McpArgs::Strings(args, "bones");
		if (names.empty()) {
			// Skyrim's NPC bones, or the main bones of skeletons that don't name them so
			const bool npc = std::any_of(context.bones.begin(), context.bones.end(), [](const AnimBone* b) { return b->boneName.rfind("NPC ", 0) == 0; });
			if (McpArgs::Bool(args, "mainBones", false) || !npc) {
				for (AnimBone* bone : PoseMath::MainBones(context.bones))
					names.push_back(bone->boneName);
			}
			else {
				for (AnimBone* bone : context.bones)
					if (bone->boneName.rfind("NPC ", 0) == 0)
						names.push_back(bone->boneName);
			}
		}

		const float units = McpBoneUnits(mcp);
		json transforms = json::object();
		for (auto& name : names)
			transforms[name] = BoneTransform(SkeletonBone(name), context, units);
		result.data["transforms"] = transforms;
	}

	if (McpArgs::Bool(args, "balance", false))
		result.data["balance"] = Balance(mcp);
	if (const json* contacts = McpArgs::Find(args, "contacts"))
		result.data["contacts"] = Contacts(mcp, *contacts);

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

std::vector<Vector3> McpGlobalLiveVerts(OutfitStudioMcp& mcp, NiShape* shape) {
	OutfitProject& project = mcp.Project();
	std::vector<Vector3> verts;
	project.GetLiveVerts(shape, verts);

	const MatTransform toGlobal = project.GetWorkAnim()->GetTransformGlobalToShape(shape).InverseTransform();
	if (!toGlobal.IsNearlyEqualTo(MatTransform()))
		for (auto& v : verts)
			v = toGlobal.ApplyTransform(v);
	return verts;
}

float McpBoneUnits(OutfitStudioMcp& mcp) {
	return mcp.Project().GetWorkNif()->GetHeader().GetVersion().IsSF() ? sfHavokScale : 1.0f;
}

PoseMath::TwistContext McpTwistContext(OutfitStudioMcp& mcp) {
	PoseMath::TwistContext context;
	context.bones = SkeletonBones();

	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	NifFile* nif = project.GetWorkNif();
	const float units = McpBoneUnits(mcp);

	// The unposed vertices of each shape in the skeleton's space and units, read once
	std::map<std::string, std::vector<Vector3>> shapeVerts;
	auto vertsOf = [&](NiShape* shape) -> const std::vector<Vector3>& {
		auto found = shapeVerts.find(shape->name.get());
		if (found != shapeVerts.end())
			return found->second;

		std::vector<Vector3> verts;
		nif->GetVertsForShape(shape, verts);
		auto skin = anim->shapeSkinning.find(shape->name.get());
		const MatTransform toGlobal = skin != anim->shapeSkinning.end() ? skin->second.xformGlobalToSkin.InverseTransform() : MatTransform();
		for (auto& v : verts)
			v = toGlobal.ApplyTransform(v) / units;
		return shapeVerts.emplace(shape->name.get(), std::move(verts)).first->second;
	};

	for (AnimBone* bone : context.bones) {
		std::string lower = bone->boneName;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (lower.find("twist") == std::string::npos)
			continue;

		Vector3 sum;
		double total = 0.0;
		for (NiShape* shape : nif->GetShapes()) {
			auto* weights = anim->GetWeightsPtr(shape->name.get(), bone->boneName);
			if (!weights || weights->empty())
				continue;
			const std::vector<Vector3>& verts = vertsOf(shape);
			for (auto& weight : *weights) {
				if (weight.first < verts.size()) {
					sum += verts[weight.first] * weight.second;
					total += weight.second;
				}
			}
		}

		if (total > 1e-6)
			context.skinCenters[bone] = bone->xformToGlobal.InverseTransform().ApplyTransform(sum / static_cast<float>(total));
	}
	return context;
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
	smooth.description =
		"Smooths the morph of sliders on shapes, which evens out bumps a conform left, or keeps a strap straight between "
		"parts that move apart (mask its ends first). Without slider or sliders, every morph slider with data on the "
		"shapes is smoothed. Masked vertices keep their deltas.";
	smooth.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"slider", McpProp("string", "Slider name")},
		{"sliders", McpStringList("Several sliders. Leave out both for all morph sliders with data on the shapes.")},
		{"iterations", McpProp("integer", "Smoothing passes, at most 100. Default 2.")},
		{"strength", McpProp("number", "Strength of each pass, 0 to 1. Default 0.5.")},
	});
	smooth.handler = [&mcp](const json& args) {
		OutfitProject& project = mcp.Project();
		const int iterations = std::clamp(McpArgs::Int(args, "iterations", 2), 1, 100);
		const float strength = std::clamp(McpArgs::Float(args, "strength", 0.5f), 0.0f, 1.0f);
		auto shapes = mcp.Shapes(args, "shapes");

		std::vector<std::string> sliders = McpArgs::Strings(args, "sliders");
		if (McpArgs::Has(args, "slider"))
			sliders.push_back(McpArgs::String(args, "slider"));
		const bool all = sliders.empty();
		if (all)
			for (size_t i = 0; i < project.SliderCount(); i++)
				if (!project.SliderZap(i) && !project.SliderUV(i))
					sliders.push_back(project.GetSliderName(i));

		McpToolResult result;
		json smoothed = json::object();
		for (auto& slider : sliders) {
			CheckSlider(project, slider);
			for (auto* shape : shapes) {
				// All sliders: only where the slider has data, the others have no morph to smooth
				TargetDataDiffs* diff = project.GetDiffSet(project.activeSet[slider], shape);
				if (all && (!diff || diff->empty()))
					continue;
				mcp.Apply(mcp.Ops().SmoothSliderDeltas(shape, slider, iterations, strength));
				smoothed[shape->name.get()].push_back(slider);
			}
		}
		result.data["smoothed"] = smoothed;
		return result;
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
		"placeholder of curtain references) are skipped, and so are excludeBones. By default, bones the reference doesn't "
		"have (physics chains, bones only the outfit has) keep their weights, and parts weighted to one bone stay rigid. A shape "
		"that a linked SMP physics XML simulates only gets new bones the XML declares: SMP makes every other bone "
		"dynamic when the XML raised the default mass. The copied vertices are cleaned to the game's bone limit and "
		"normalized in the same step. One undo takes it all back, bones it added included. The result lists per shape "
		"the bones added, kept and not added, the rigid parts and the weight problems left.";
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
		{"keepOtherBones",
		 McpProp("boolean",
				 "Bones of the shapes that the reference doesn't have keep their weights, and the copied bones share the rest of "
				 "each vertex. Excluded reference bones don't count as kept. "
				 "Default true. false lets the copy replace them, like the window does, e.g. for weights of an old body's bones "
				 "that the new one doesn't have (keptBones in the result lists them).")},
		{"addBones",
		 McpProp("boolean",
				 "Add copied bones that a shape doesn't have yet. Default true. false only copies the bones each shape has, for an "
				 "outfit that is weighted already: no genital, head or forearm weights on a top.")},
		{"keepRigidParts",
		 McpProp("boolean", "Connected parts weighted at least 0.99 to one copied bone on every vertex (buckles, plates, props) keep their weights. Default true.")},
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

		options.keepOtherBones = McpArgs::Bool(args, "keepOtherBones", true);
		options.keepRigidParts = McpArgs::Bool(args, "keepRigidParts", true);
		const bool addBones = McpArgs::Bool(args, "addBones", true);

		// A shape that an SMP physics XML simulates takes every bone it's weighted to into the physics, and the ones
		// the XML doesn't declare get its default bone template: dynamic, if the XML raised its mass. So these shapes
		// only get new bones the XML declares.
		std::map<std::string, std::vector<const PhysicsXmlFile::Shape*>> physicsShapes;
		std::vector<std::pair<std::string, PhysicsXmlFile>> physicsFiles;
		std::vector<std::string> physicsMissing;
		for (auto* shape : shapes) {
			for (auto& file : project.ReadPhysicsXmls(shape->name.get(), &physicsMissing))
				physicsFiles.push_back(std::move(file));
		}
		for (auto* shape : shapes)
			for (auto& file : physicsFiles)
				if (const PhysicsXmlFile::Shape* simulated = file.second.FindShape(shape->name.get()))
					physicsShapes[shape->name.get()].push_back(simulated);

		std::map<std::string, std::vector<std::string>> notAddedForPhysics;
		options.mayAddBone = [&](const std::string& shapeName, const std::string& bone) {
			if (!addBones)
				return false;
			auto simulated = physicsShapes.find(shapeName);
			if (simulated == physicsShapes.end())
				return true;
			for (auto* s : simulated->second)
				if (s->Declares(bone))
					return true;
			notAddedForPhysics[shapeName].push_back(bone);
			return false;
		};

		// Recorded like the window does, so it can be undone
		UndoHistory& history = frame->editor.undoHistory;
		UndoStateProject* usp = history.PushState();
		std::map<std::string, BoneWeightCopyReport> reports;
		OpResult result = mcp.Ops().CopyBoneWeights(shapes, options, *usp, &reports);
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
				const std::vector<std::string>& kept = reports[uss.shapeName].keptBones;
				auto isKept = [&kept](const std::string& bone) { return std::find(kept.begin(), kept.end(), bone) != kept.end(); };
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

					// Kept bones (physics chains) come first and keep their weights, even small ones; the copied
					// bones fill the rest of the vertex
					std::sort(list.begin(), list.end(), [&](const auto& a, const auto& b) {
						const bool keptA = isKept(a.second);
						const bool keptB = isKept(b.second);
						return keptA != keptB ? keptA : a.first > b.first;
					});
					std::map<std::string, float> goal;
					float keptSum = 0.0f;
					float copiedSum = 0.0f;
					for (size_t i = 0; i < list.size() && i < limit; i++)
						(isKept(list[i].second) ? keptSum : copiedSum) += list[i].first;
					const float keptShare = std::min(keptSum, 1.0f);
					const float keptFactor = keptSum <= 0.0f ? 0.0f : (copiedSum > 0.0f ? keptShare : 1.0f) / keptSum;
					const float copiedFactor = copiedSum <= 0.0f ? 0.0f : (1.0f - keptShare) / copiedSum;
					for (size_t i = 0; i < list.size(); i++)
						goal[list[i].second] = i < limit ? list[i].first * (isKept(list[i].second) ? keptFactor : copiedFactor) : 0.0f;

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

			auto report = reports.find(name);
			if (report == reports.end())
				continue;
			json& out = tool.data["shapes"][name];
			if (!report->second.keptBones.empty())
				out["keptBones"] = report->second.keptBones;
			if (!report->second.notAdded.empty())
				out["bonesNotAdded"] = report->second.notAdded;
			auto physics = notAddedForPhysics.find(name);
			if (physics != notAddedForPhysics.end()) {
				out["bonesNotDeclaredByPhysics"] = physics->second;
				out["physicsHint"] =
					"The shape is simulated by an SMP physics XML that doesn't declare these bones. SMP would add them with its default "
					"bone template, which makes them fall when its mass is above 0. add_bones before the copy if they're needed anyway.";
			}
			if (!report->second.rigidParts.empty()) {
				json parts = json::array();
				for (auto& part : report->second.rigidParts)
					parts.push_back({{"vertices", part.first}, {"bone", part.second}});
				out["rigidPartsKept"] = parts;
			}
		}
		if (!skipped.empty())
			tool.data["skippedBones"] = skipped;
		if (!physicsMissing.empty()) {
			std::sort(physicsMissing.begin(), physicsMissing.end());
			physicsMissing.erase(std::unique(physicsMissing.begin(), physicsMissing.end()), physicsMissing.end());
			tool.data["physicsXmlNotFound"] = physicsMissing;
		}
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
		"the unposed skeleton, and change bones on top. Global axes: +X is the character's right, +Y her front, +Z up. "
		"For posing by eye, prefer tilt, aim, twist and reach, which say which way in the character's terms, over "
		"'bones' and worldRotation. Changes apply parents first: bones, then shift, worldRotation, aim, tilt and twist "
		"per bone, then reach, then plant. Bones left out keep the start pose. The result has the pose (as get_pose), "
		"the reach and plant results, and warnings about twisted, backwards or folded joints. get_pose shows more, "
		"save_pose saves it.";
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
		{"shift",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "array"}, {"items", {{"type", "number"}}}}},
		  {"description", "Bone name to a move [x, y, z] in mesh units (as list_shapes bounds) with its children, e.g. {\"NPC COM [COM ]\": [3, 0, -1]} "
						  "moves the body 3 units to her right and 1 down. With plant, the feet stay and the legs bend to follow."}}},
		{"worldRotation",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "array"}, {"items", {{"type", "number"}}}}},
		  {"description", "Bone name to a turn about the skeleton's global axes: the rotation axis scaled by the angle in degrees, "
						  "right-handed. [0, 10, 0] turns about her front axis and tips the top of an upright bone 10 degrees "
						  "towards her right; [10, 0, 0] tips it back; [0, 0, 10] turns her to her left. tilt says the same in plainer terms."}}},
		{"aim",
		 {{"type", "object"},
		  {"additionalProperties",
		   {{"anyOf",
			 {{{"type", "array"}, {"items", {{"type", "number"}}}},
			  {{"type", "object"},
			   {"properties",
				{{"direction", {{"type", "array"}, {"items", {{"type", "number"}}}}},
				 {"twist", McpProp("number", "Twist about the bone's length afterwards, as twist sets it")}}}}}}}},
		  {"description", "Bone name to a global direction [x, y, z] to point the bone in, towards its next joint (e.g. the upper "
						  "arm towards the elbow), with the smallest turn from where it points now. Aiming a bone that was turned "
						  "before keeps that twist: give {direction, twist: 0} to set the twist too, or use reach for a whole limb."}}},
		{"tilt",
		 {{"type", "object"},
		  {"additionalProperties",
		   {{"type", "object"},
			{"properties", {{"right", {{"type", "number"}}}, {"front", {{"type", "number"}}}, {"up", {{"type", "number"}}}}}}},
		  {"description", "Bone name to {right, front, up} in degrees: turns the bone so its end (the next joint) moves towards "
						  "her right (+X), her front (+Y) or up (+Z); negative values move it to her left, back or down. Applied in "
						  "that order from where the bone points, e.g. {\"NPC Spine1 [Spn1]\": {\"right\": -10}} leans the chest "
						  "to her left and {\"NPC Head [Head]\": {\"front\": 15}} nods the head forward. A bone that points along "
						  "an axis can't tilt towards it."}}},
		{"twist",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "number"}}},
		  {"description", "Bone name to its twist about its own length in degrees, right-handed about the direction it points, "
						  "compared to its unposed twist (get_pose withTransforms shows it). The twist bones along the limb take a "
						  "share by how far along they are, so the skin turns gradually: twisting the hand spreads over the forearm "
						  "twist bones, the upper arm over its own. 0 removes the twist."}}},
		{"reach",
		 {{"type", "object"},
		  {"additionalProperties",
		   {{"type", "object"},
			{"properties",
			 {{"target",
			   {{"description", "Global position [x, y, z] in mesh units, a bone name, or {bone, offset: [x, y, z]} for a point next to a bone"}}},
			  {"pole", {{"type", "array"}, {"items", {{"type", "number"}}}, {"description", "Global direction the middle joint (elbow, knee) points to"}}}}}}},
		  {"description", "Two-bone IK: end bone (hand or foot) to {target, pole}. Turns the upper and lower bone of the limb so the "
						  "end bone's joint reaches the target, e.g. {\"NPC L Hand [LHnd]\": {\"target\": {\"bone\": \"NPC Head [Head]\", "
						  "\"offset\": [-6, -8, 4]}, \"pole\": [-1, 0, 0]}} puts the left hand behind the head with the elbow out to "
						  "her left. Both bones are solved from their unposed rotation, rolled so the joint bends the way it does "
						  "naturally, so no twist is left. Without a pole the joint points the way the unposed limb bends. The "
						  "result says whether the target was in reach and how far it missed. The end bone isn't turned: aim or "
						  "tilt it afterwards in the same call."}}},
		{"plant", McpStringList("End bones (feet, or hands on something) to keep where the start pose has them, with their "
								"orientation: after everything else, their limbs are solved with reach. Use with shift or pelvis "
								"turns, so the hips move and the legs follow.")},
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

		// End bones to keep where the start pose has them, with the way their middle joint points
		struct Plant {
			AnimBone* bone = nullptr;
			Vector3 position;
			Vector3 pole;
			PoseMath::BoneOrientation orientation;
		};
		std::vector<Plant> plants;
		for (auto& name : McpArgs::Strings(args, "plant")) {
			AnimBone* bone = SkeletonBone(name);
			AnimBone* upper = nullptr;
			AnimBone* lower = nullptr;
			if (!PoseMath::LimbChain(bone, upper, lower))
				throw McpToolError("'" + name + "' can't be planted: it isn't the end of a limb, like a foot below its calf and thigh.");

			Plant plant;
			plant.bone = bone;
			plant.position = bone->xformPoseToGlobal.translation;
			plant.orientation = PoseMath::CaptureOrientation(bone);
			const Vector3 start = upper->xformPoseToGlobal.translation;
			Vector3 line = plant.position - start;
			line.Normalize();
			plant.pole = lower->xformPoseToGlobal.translation - start;
			plant.pole -= line * plant.pole.dot(line);
			if (plant.pole.length() < 1e-3f)
				plant.pole = Vector3(0.0f, 1.0f, 0.0f);
			plants.push_back(plant);
		}

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

		// Changes in global terms, parents first so a child changes from where its parent put it
		enum class Kind { Shift, Turn, Aim, Tilt, Twist };
		struct Change {
			AnimBone* bone = nullptr;
			Kind kind = Kind::Turn;
			Vector3 value;
			float twist = 0.0f;
			bool hasTwist = false;
		};
		std::vector<Change> changes;

		auto objectArg = [&](const char* name) -> const json* {
			const json* list = McpArgs::Find(args, name);
			if (list && !list->is_object())
				McpArgs::Wrong(name, "an object of bone names to values");
			return list;
		};

		const float units = McpBoneUnits(mcp);
		if (const json* list = objectArg("shift"))
			for (auto& item : list->items())
				changes.push_back({SkeletonBone(item.key()), Kind::Shift, ToBoneUnits(VectorFromJson(item.value(), "shift of " + item.key()), units)});
		if (const json* list = objectArg("worldRotation"))
			for (auto& item : list->items())
				changes.push_back({SkeletonBone(item.key()), Kind::Turn, VectorFromJson(item.value(), "worldRotation of " + item.key()) * DEG2RAD});
		if (const json* list = objectArg("aim")) {
			for (auto& item : list->items()) {
				Change change{SkeletonBone(item.key()), Kind::Aim};
				if (item.value().is_object()) {
					if (!item.value().contains("direction"))
						throw McpToolError("aim of " + item.key() + " needs a direction.");
					change.value = VectorFromJson(item.value()["direction"], "aim direction of " + item.key());
					if (item.value().contains("twist") && item.value()["twist"].is_number()) {
						change.twist = item.value()["twist"].get<float>() * DEG2RAD;
						change.hasTwist = true;
					}
				}
				else
					change.value = VectorFromJson(item.value(), "aim of " + item.key());
				if (change.value.length() < 1e-6f)
					throw McpToolError("aim of " + item.key() + " has no direction.");
				changes.push_back(change);
			}
		}
		if (const json* list = objectArg("tilt")) {
			for (auto& item : list->items()) {
				if (!item.value().is_object())
					throw McpToolError("tilt of " + item.key() + " has to be {right, front, up}.");
				Vector3 degrees(McpArgs::Float(item.value(), "right", 0.0f), McpArgs::Float(item.value(), "front", 0.0f), McpArgs::Float(item.value(), "up", 0.0f));
				changes.push_back({SkeletonBone(item.key()), Kind::Tilt, degrees * DEG2RAD});
			}
		}
		if (const json* list = objectArg("twist")) {
			for (auto& item : list->items()) {
				if (!item.value().is_number())
					throw McpToolError("twist of " + item.key() + " has to be a number of degrees.");
				Change change{SkeletonBone(item.key()), Kind::Twist};
				change.twist = item.value().get<float>() * DEG2RAD;
				changes.push_back(change);
			}
		}

		std::stable_sort(changes.begin(), changes.end(), [](const Change& a, const Change& b) { return BoneDepth(a.bone) < BoneDepth(b.bone); });

		const PoseMath::TwistContext twistContext = McpTwistContext(mcp);
		for (auto& change : changes) {
			AnimBone* bone = change.bone;
			switch (change.kind) {
				case Kind::Shift: PoseMath::ShiftBoneGlobal(bone, change.value); break;
				case Kind::Turn: PoseMath::TurnBoneGlobal(bone, change.value); break;
				case Kind::Aim:
					if (!HasAxis(bone))
						throw McpToolError("The bone '" + bone->boneName + "' reaches no joint to aim with.");
					PoseMath::TurnBoneGlobal(bone, PoseMath::AimRotation(bone, change.value));
					if (change.hasTwist)
						PoseMath::SetTwistSpread(bone, change.twist, twistContext);
					break;
				case Kind::Tilt: {
					const Vector3 axes[] = {Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)};
					const char* names[] = {"right", "front", "up"};
					const float angles[] = {change.value.x, change.value.y, change.value.z};
					for (int i = 0; i < 3; i++) {
						if (std::abs(angles[i]) < 1e-6f)
							continue;
						Vector3 direction;
						if (!HasAxis(bone) || !PoseMath::BoneAxis(bone, direction))
							throw McpToolError("The bone '" + bone->boneName + "' reaches no joint to tilt with.");
						Vector3 turnAxis = direction.cross(axes[i]);
						if (turnAxis.length() < 0.05f)
							throw McpToolError("'" + bone->boneName + "' points almost straight along the " + names[i]
											   + " axis, so it can't tilt towards it. Tilt it towards another axis, or use aim.");
						turnAxis.Normalize();
						PoseMath::TurnBoneGlobal(bone, turnAxis * angles[i]);
					}
					break;
				}
				case Kind::Twist: PoseMath::SetTwistSpread(bone, change.twist, twistContext); break;
			}
		}

		// Limbs to a target, parents first
		json reachResults = json::object();
		if (const json* list = objectArg("reach")) {
			struct Reach {
				AnimBone* bone = nullptr;
				json spec;
			};
			std::vector<Reach> reaches;
			for (auto& item : list->items()) {
				AnimBone* bone = SkeletonBone(item.key());
				AnimBone* upper = nullptr;
				AnimBone* lower = nullptr;
				if (!PoseMath::LimbChain(bone, upper, lower))
					throw McpToolError("'" + item.key() + "' can't reach: it isn't the end of a limb, like a hand below its forearm and upper arm.");
				if (!item.value().is_object() || !item.value().contains("target"))
					throw McpToolError("reach of " + item.key() + " needs a target.");
				reaches.push_back({bone, item.value()});
			}
			std::stable_sort(reaches.begin(), reaches.end(), [](const Reach& a, const Reach& b) { return BoneDepth(a.bone) < BoneDepth(b.bone); });

			for (auto& reach : reaches) {
				const json& target = reach.spec["target"];
				Vector3 position;
				if (target.is_string())
					position = SkeletonBone(target.get<std::string>())->xformPoseToGlobal.translation;
				else if (target.is_object()) {
					position = SkeletonBone(McpArgs::String(target, "bone"))->xformPoseToGlobal.translation;
					if (target.contains("offset"))
						position += ToBoneUnits(VectorFromJson(target["offset"], "reach offset of " + reach.bone->boneName), units);
				}
				else
					position = ToBoneUnits(VectorFromJson(target, "reach target of " + reach.bone->boneName), units);

				Vector3 pole;
				const bool hasPole = reach.spec.contains("pole");
				if (hasPole)
					pole = VectorFromJson(reach.spec["pole"], "reach pole of " + reach.bone->boneName);

				auto solved = PoseMath::ReachTwoBone(reach.bone, position, hasPole ? &pole : nullptr);
				// The roll that turns the joint into the pole's plane goes over the upper bone's twist bones
				if (solved.upper)
					PoseMath::SetTwistSpread(solved.upper, PoseMath::SegmentTwist(solved.upper, twistContext), twistContext);
				reachResults[reach.bone->boneName] = {
					{"reachable", solved.reachable},
					{"missed", std::round(solved.missed * units * 100.0f) / 100.0f},
					{"target", VectorToJson(position * units)},
					{"middleJoint", VectorToJson(solved.midPosition * units)},
				};
				if (!solved.reachable)
					reachResults[reach.bone->boneName]["note"] = "Out of reach: the limb points at the target as far as it goes.";
			}
		}

		json planted = json::object();
		for (auto& plant : plants) {
			auto solved = PoseMath::ReachTwoBone(plant.bone, plant.position, &plant.pole);
			if (solved.upper)
				PoseMath::SetTwistSpread(solved.upper, PoseMath::SegmentTwist(solved.upper, twistContext), twistContext);
			PoseMath::RestoreOrientation(plant.bone, plant.orientation);
			planted[plant.bone->boneName] = {{"reachable", solved.reachable}, {"missed", std::round(solved.missed * units * 100.0f) / 100.0f}};
			if (!solved.reachable)
				planted[plant.bone->boneName]["note"] = "The leg can't reach the planted spot any more: move the body less far.";
		}

		PoseData posed;
		PoseDataCollection::CaptureCurrentPose("MCP", false, posed);
		frame->ApplyRefresh(ops.SetPreviewPose(posed));

		McpToolResult result = GetPose(mcp, true);
		if (!reachResults.empty())
			result.data["reach"] = reachResults;
		if (!planted.empty())
			result.data["planted"] = planted;
		json warnings = PoseWarnings(twistContext);
		if (!warnings.empty())
			result.data["warnings"] = warnings;
		return result;
	};
	mcp.AddTool(std::move(pose));

	McpTool getPose;
	getPose.name = "get_pose";
	getPose.description =
		"Returns the bones of the current preview pose that differ from the unposed skeleton, in the form "
		"set_preview_pose takes them: rotation as axis times angle in degrees, translation and scale. With "
		"withTransforms, also where bones are in the skeleton's global space (+X the character's right, +Y her front, "
		"+Z up), which way they point towards their next joint, their twist about their own length and how far "
		"their joint bends, in degrees, with bendsBackwards for a hinge bent the wrong way. balance weighs the "
		"reference body over the feet; contacts measures how close shapes come, e.g. a hand to the head.";
	getPose.inputSchema = McpSchema({
		{"withTransforms", McpProp("boolean", "Also return global positions, directions, twist and bend. Default false.")},
		{"bones", McpStringList("With withTransforms: only these bones, all NPC bones by default")},
		{"mainBones", McpProp("boolean", "With withTransforms and no bones: only the main bones (spine, neck, head, arms, legs). Default false.")},
		{"balance", McpProp("boolean", "Where the reference body's mass center is compared to the middle of the feet, and whether she'd stand. Default false.")},
		{"contacts",
		 {{"type", "array"},
		  {"items", {{"type", "object"}, {"properties", {{"from", McpProp("string", "Shape")}, {"to", McpProp("string", "Shape")}}}}},
		  {"description", "Pairs of shapes to measure with the pose: closest distance of from's vertices to to's surface "
						  "(negative inside), how many vertices go in (penetrating) and how many are within 0.5 (touching)."}}},
	});
	getPose.readOnly = true;
	getPose.handler = [&mcp](const json& args) { return GetPose(mcp, mcp.Project().bPose, args); };
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
