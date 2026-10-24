/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/Anim.h"
#include "../components/PoseMath.h"
#include "../program/OutfitStudio.h"

#include <algorithm>
#include <cmath>

using nlohmann::json;
using namespace nifly;

namespace {

json Names(const std::vector<NiShape*>& shapes) {
	json names = json::array();
	for (NiShape* shape : shapes)
		names.push_back(shape->name.get());
	return names;
}

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

Vector3 VectorArg(const json& args, const char* name) {
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return Vector3();
	if (!value->is_array() || value->size() != 3 || !(*value)[0].is_number() || !(*value)[1].is_number() || !(*value)[2].is_number())
		McpArgs::Wrong(name, "an array of three numbers");
	return Vector3((*value)[0].get<float>(), (*value)[1].get<float>(), (*value)[2].get<float>());
}

json BoneJson(const AnimBone& bone) {
	return {
		{"name", bone.boneName},
		{"parent", bone.parent ? json(bone.parent->boneName) : json(nullptr)},
		{"custom", !bone.isStandardBone},
		{"inProject", bone.refCount > 0},
		{"translation", Vec(bone.xformToParent.translation)},
		{"rotation", Vec(RotMatToVec(bone.xformToParent.rotation))},
		{"globalPosition", Vec(bone.xformToGlobal.translation)},
	};
}

// What a bone does in posing: its role and range, length and unposed direction, and the twist bones of its segment
json BonePoseInfo(const AnimBone& bone, bool main, const PoseMath::TwistContext& context, float units) {
	json info = {{"main", main}, {"shortName", PoseMath::ShortBoneName(bone.boneName)}};

	PoseMath::BoneRole role = PoseMath::GetBoneRole(bone.boneName);
	if (!role.role.empty()) {
		info["role"] = role.role;
		info["range"] = role.limits;
	}

	if (AnimBone* child = PoseMath::ReachChild(&bone)) {
		Vector3 direction = child->xformToGlobal.translation - bone.xformToGlobal.translation;
		info["length"] = direction.length() * units;
		direction.Normalize();
		info["unposedDirection"] = Vec(direction);
		info["reaches"] = child->boneName;
	}

	if (main) {
		json twistBones = json::array();
		for (auto& share : PoseMath::SegmentTwistBones(&bone, context))
			twistBones.push_back({{"bone", share.bone->boneName}, {"fraction", std::round(share.fraction * 100.0f) / 100.0f}});
		if (!twistBones.empty())
			info["twistBones"] = twistBones;
	}
	return info;
}

McpToolResult BoneResult(const std::string& name) {
	McpToolResult result;
	AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
	result.data["bone"] = bone ? BoneJson(*bone) : json(nullptr);
	return result;
}

// Custom bones are added and edited with a transform to the parent bone
json CustomBoneStep(const char* type, const json& args) {
	std::string name = McpArgs::String(args, "name");
	if (name.empty())
		McpArgs::Wrong("name", "a bone name");
	Vector3 translation = VectorArg(args, "translation");
	Vector3 rotation = VectorArg(args, "rotation");
	return {
		{"type", type},
		{"BoneName", name},
		{"ParentBone", McpArgs::String(args, "parent", "")},
		{"TransX", translation.x},
		{"TransY", translation.y},
		{"TransZ", translation.z},
		{"RotX", rotation.x},
		{"RotY", rotation.y},
		{"RotZ", rotation.z},
	};
}

const json TranslationProp = McpProp("array", "Position [x, y, z] relative to the parent bone. Default [0, 0, 0].");
const json RotationProp = McpProp("array", "Rotation to the parent as a rotation vector [x, y, z]: the axis times the angle in radians, as the dialog takes it. Default [0, 0, 0].");

} // namespace

void AddBoneTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool skeleton;
	skeleton.name = "list_skeleton_bones";
	skeleton.description =
		"Lists the bones of the reference skeleton and the custom bones, with parent, transform to the parent, "
		"global position and whether the project uses them. add_bones takes bones from this list. For posing, "
		"mainOnly lists only the body's main joints and withPoseInfo adds each bone's role and natural range, length, "
		"unposed direction and the twist bones that share its twist, with how far along the limb they are. "
		"translation and globalPosition are in the skeleton's units (meters for Starfield, see units), length in mesh units.";
	skeleton.inputSchema = McpSchema({
		{"filter", McpProp("string", "Only bones whose name contains this text, ignoring case")},
		{"inProject", McpProp("boolean", "Only the bones the project uses. Default false.")},
		{"mainOnly", McpProp("boolean", "Only the main bones: pelvis, spine, neck, head, clavicles, arms, hands, legs and feet. Default false.")},
		{"withPoseInfo", McpProp("boolean", "Add role, range of motion, length, unposed direction and twist bones. Default false.")},
	});
	skeleton.readOnly = true;
	skeleton.handler = [&mcp](const json& args) {
		std::string filter = McpArgs::String(args, "filter", "");
		std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
		const bool inProject = McpArgs::Bool(args, "inProject", false);
		const bool mainOnly = McpArgs::Bool(args, "mainOnly", false);
		const bool withPoseInfo = McpArgs::Bool(args, "withPoseInfo", false);

		AnimSkeleton& skel = AnimSkeleton::getInstance();
		std::vector<std::string> names;
		skel.GetBoneNames(names);

		std::vector<AnimBone*> all;
		for (auto& name : names)
			if (AnimBone* bone = skel.GetBonePtr(name))
				all.push_back(bone);
		const std::vector<AnimBone*> mainBones = PoseMath::MainBones(all);
		PoseMath::TwistContext context;
		if (withPoseInfo)
			context = McpTwistContext(mcp);
		const float units = McpBoneUnits(mcp);

		json bones = json::array();
		for (auto& name : names) {
			AnimBone* bone = skel.GetBonePtr(name);
			if (!bone || (inProject && bone->refCount <= 0))
				continue;
			std::string lower = name;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			if (!filter.empty() && lower.find(filter) == std::string::npos)
				continue;
			const bool main = std::find(mainBones.begin(), mainBones.end(), bone) != mainBones.end();
			if (mainOnly && !main)
				continue;

			json entry = BoneJson(*bone);
			if (withPoseInfo)
				entry.update(BonePoseInfo(*bone, main, context, units));
			bones.push_back(entry);
		}

		McpToolResult result;
		result.data["bones"] = bones;
		result.data["units"] = {{"meshUnitsPerSkeletonUnit", units}};
		return result;
	};
	mcp.AddTool(std::move(skeleton));

	McpTool addBones;
	addBones.name = "add_bones";
	addBones.description = "Adds bones of the reference skeleton to the project, like Add Bone in the window. Weights for them come from copy_bone_weights, set_weights or the weight brush.";
	addBones.inputSchema = McpSchema({{"bones", McpStringList("Bone names from list_skeleton_bones")}}, {"bones"});
	addBones.handler = [&mcp, frame](const json& args) {
		std::vector<std::string> bones = McpArgs::Strings(args, "bones");
		if (bones.empty())
			McpArgs::Wrong("bones", "a list of bone names");
		for (auto& bone : bones)
			if (!AnimSkeleton::getInstance().GetBonePtr(bone))
				throw McpToolError("The skeleton has no bone '" + bone + "'. list_skeleton_bones lists them.");
		McpRunStep(mcp, {{"type", "AddBone"}, {"BoneNames", bones}});
		frame->UpdateAnimationGUI();

		McpToolResult result;
		json list = json::array();
		for (auto& bone : bones)
			list.push_back(BoneJson(*AnimSkeleton::getInstance().GetBonePtr(bone)));
		result.data["bones"] = list;
		return result;
	};
	mcp.AddTool(std::move(addBones));

	McpTool addCustom;
	addCustom.name = "add_custom_bone";
	addCustom.description = "Adds a bone that isn't in the reference skeleton, e.g. for physics, like Add Custom Bone in the window.";
	addCustom.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "New bone name")},
			{"parent", McpProp("string", "Parent bone. Default: none, the root.")},
			{"translation", TranslationProp},
			{"rotation", RotationProp},
		},
		{"name"});
	addCustom.handler = [&mcp, frame](const json& args) {
		std::string name = McpArgs::String(args, "name");
		if (AnimSkeleton::getInstance().GetBonePtr(name))
			throw McpToolError("There already is a bone '" + name + "'. Use edit_bone to change a custom bone.");
		std::string parent = McpArgs::String(args, "parent", "");
		if (!parent.empty() && !AnimSkeleton::getInstance().GetBonePtr(parent))
			throw McpToolError("There is no parent bone '" + parent + "'.");
		McpRunStep(mcp, CustomBoneStep("AddCustomBone", args));
		frame->UpdateAnimationGUI();
		return BoneResult(name);
	};
	mcp.AddTool(std::move(addCustom));

	McpTool editBone;
	editBone.name = "edit_bone";
	editBone.description = "Changes the parent and transform of a custom bone, like Edit Bone in the window. Bones of the reference skeleton can't be edited.";
	editBone.inputSchema = McpSchema(
		{
			{"name", McpProp("string", "Custom bone")},
			{"parent", McpProp("string", "Parent bone. Default: none, the root.")},
			{"translation", TranslationProp},
			{"rotation", RotationProp},
		},
		{"name"});
	editBone.handler = [&mcp, frame](const json& args) {
		std::string name = McpArgs::String(args, "name");
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
		if (!bone)
			throw McpToolError("There is no bone '" + name + "'.");
		if (bone->isStandardBone)
			throw McpToolError("'" + name + "' is a bone of the reference skeleton, only custom bones can be edited.");
		McpRunStep(mcp, CustomBoneStep("EditBone", args));
		frame->UpdateAnimationGUI();
		return BoneResult(name);
	};
	mcp.AddTool(std::move(editBone));

	McpTool deleteBones;
	deleteBones.name = "delete_bones";
	deleteBones.description =
		"Deletes bones and their weights from the whole project, or with shapes only removes their weights from those "
		"shapes, like Delete Bone and Delete Bone From Selected in the window.";
	deleteBones.inputSchema = McpSchema(
		{
			{"bones", McpStringList("Bone names")},
			{"shapes", McpStringList("Only remove the bones from these shapes")},
		},
		{"bones"});
	deleteBones.handler = [&mcp, frame](const json& args) {
		std::vector<std::string> bones = McpArgs::Strings(args, "bones");
		if (bones.empty())
			McpArgs::Wrong("bones", "a list of bone names");

		json step = {{"type", "DeleteBones"}, {"BoneNames", bones}, {"FromProject", !McpArgs::Has(args, "shapes")}};
		if (McpArgs::Has(args, "shapes"))
			step["TargetMeshes"] = Names(mcp.Shapes(args, "shapes", true));
		McpRunStep(mcp, step);
		frame->UpdateAnimationGUI();

		McpToolResult result;
		result.data["deleted"] = bones;
		return result;
	};
	mcp.AddTool(std::move(deleteBones));

	McpTool boneTransforms;
	boneTransforms.name = "set_bone_transforms";
	boneTransforms.description =
		"Recalculates bone transforms of shapes, like Set Bone Skin and Set Bone Node in the window: skin_from_node "
		"makes the skin transform match the bone's node, node_from_skin moves a custom bone's node to match the skin.";
	boneTransforms.inputSchema = McpSchema(
		{
			{"shapes", McpStringList("Shapes, the selected ones by default")},
			{"bones", McpStringList("Bone names")},
			{"mode", {{"type", "string"}, {"enum", {"skin_from_node", "node_from_skin"}}, {"description", "Default skin_from_node"}}},
		},
		{"bones"});
	boneTransforms.handler = [&mcp, frame](const json& args) {
		std::vector<std::string> bones = McpArgs::Strings(args, "bones");
		if (bones.empty())
			McpArgs::Wrong("bones", "a list of bone names");
		std::string mode = McpArgs::String(args, "mode", "skin_from_node");
		if (mode != "skin_from_node" && mode != "node_from_skin")
			McpArgs::Wrong("mode", "skin_from_node or node_from_skin");

		json names = Names(mcp.Shapes(args, "shapes", true));
		McpRunStep(mcp, {{"type", "SetBoneTransform"}, {"TargetMeshes", names}, {"BoneNames", bones}, {"Mode", mode == "skin_from_node" ? 0 : 1}});
		frame->UpdateAnimationGUI();

		McpToolResult result;
		result.data["shapes"] = names;
		return result;
	};
	mcp.AddTool(std::move(boneTransforms));

	McpTool transfer;
	transfer.name = "transfer_weights";
	transfer.description =
		"Copies bone weights from the reference to shapes with the same vertex count by vertex index, like Transfer "
		"Selected Weights in the window. Unlike copy_bone_weights there is no search for the nearest surface, so "
		"it's for shapes made from the reference mesh. Masked vertices keep their weights.";
	transfer.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes, the selected outfit shapes by default")},
		{"bones", McpStringList("Bones to transfer. Default: every bone of the reference.")},
		{"useMask", McpProp("boolean", "Keep the weights of masked vertices, blended by the mask value. Default true.")},
	});
	transfer.handler = [&mcp](const json& args) {
		if (!mcp.Project().GetBaseShape())
			throw McpToolError("There is no reference shape to transfer weights from.");
		json names = Names(mcp.Shapes(args, "shapes"));
		McpRunStep(mcp,
				   {{"type", "TransferWeights"},
					{"TargetMeshes", names},
					{"BoneNames", McpArgs::Strings(args, "bones")},
					{"UseMask", McpArgs::Bool(args, "useMask", true)}});

		McpToolResult result;
		result.data["shapes"] = names;
		return result;
	};
	mcp.AddTool(std::move(transfer));

	McpTool removeSkinning;
	removeSkinning.name = "remove_skinning";
	removeSkinning.description = "Removes the skinning and all bone weights of shapes, so they no longer follow the bones.";
	removeSkinning.inputSchema = McpSchema({{"shapes", McpStringList("Shapes, the selected ones by default")}});
	removeSkinning.handler = [&mcp, frame](const json& args) {
		json names = Names(mcp.Shapes(args, "shapes", true));
		McpRunStep(mcp, {{"type", "RemoveSkinning"}, {"TargetMeshes", names}});
		frame->UpdateAnimationGUI();

		McpToolResult result;
		result.data["shapes"] = names;
		return result;
	};
	mcp.AddTool(std::move(removeSkinning));

	McpTool poseToMesh;
	poseToMesh.name = "apply_pose_to_mesh";
	poseToMesh.description =
		"Bakes a pose into the geometry of all shapes, so they keep it without the preview pose. Without pose it bakes "
		"the current preview pose like the Pose to Mesh button, as one undo state. With pose it bakes a pose of the pose "
		"list, like the Apply Pose automation step, which isn't in the undo history.";
	poseToMesh.inputSchema = McpSchema({{"pose", McpProp("string", "Name of a pose from list_poses. Default: the current preview pose.")}});
	poseToMesh.handler = [&mcp, frame](const json& args) {
		OutfitProject& project = mcp.Project();
		if (McpArgs::Has(args, "pose"))
			McpRunStep(mcp, {{"type", "ApplyPose"}, {"PoseName", McpArgs::String(args, "pose")}});
		else {
			if (!project.bPose)
				throw McpToolError("No preview pose is on. Set one with set_preview_pose, or name a pose.");

			wxCommandEvent event(wxEVT_BUTTON, XRCID("poseToMesh"));
			frame->GetEventHandler()->ProcessEvent(event);
			frame->UpdateUndoTools();
		}

		McpToolResult result;
		result.data["poseActive"] = project.bPose;
		return result;
	};
	mcp.AddTool(std::move(poseToMesh));

	McpTool badBones;
	badBones.name = "fix_bad_bones";
	badBones.description =
		"Checks the bones of the project for transforms that don't match the skeleton, like Check Bad Bones in the "
		"window, and fixes them. The log lists what was found.";
	badBones.handler = [&mcp, frame](const json&) {
		BadBonesReport report = mcp.Project().FindBadBones();

		McpToolResult result;
		json shapes = json::object();
		for (auto& shape : report.shapeBBs) {
			json bones = json::array();
			for (auto& bone : shape.second.badStandard)
				bones.push_back(bone.first);
			for (auto& bone : shape.second.badCustom)
				bones.push_back(bone.first);
			if (!bones.empty())
				shapes[shape.first] = bones;
		}
		result.data["badBones"] = shapes;

		if (report.found) {
			McpRunStep(mcp, {{"type", "FixBadBones"}});
			frame->UpdateAnimationGUI();
		}
		result.data["fixed"] = report.found;
		return result;
	};
	mcp.AddTool(std::move(badBones));
}
