/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/MeshAnalysis.h"
#include "../components/PoseData.h"
#include "../components/PoseFit.h"
#include "../components/PoseMath.h"
#include "../components/SurfaceFit.h"
#include "../program/OutfitStudio.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

float Round(float v, float step = 0.01f) {
	return std::round(v / step) * step;
}

// Vertices of a shape as the sliders morph it, unposed, in the skeleton's global space and units
std::vector<Vector3> GlobalVerts(OutfitProject& project, NiShape* shape, float units) {
	std::vector<Vector3> verts;
	project.GetMorphedVerts(shape, verts);
	const MatTransform toGlobal = project.GetWorkAnim()->GetTransformShapeToGlobal(shape);
	const bool transformed = !toGlobal.IsNearlyEqualTo(MatTransform());
	for (auto& v : verts) {
		if (transformed)
			v = toGlobal.ApplyTransform(v);
		v /= units;
	}
	return verts;
}

// A skinned shape of the project as a template of the body for fitting the skeleton
PoseFit::Template MakeTemplate(OutfitProject& project, NiShape* shape, float units) {
	if (!shape->IsSkinned())
		throw McpToolError("The template '" + shape->name.get() + "' isn't skinned. Templates have to be weighted to the skeleton, like the reference body.");

	PoseFit::Template t;
	t.verts = GlobalVerts(project, shape, units);
	shape->GetTriangles(t.tris);
	t.influences.resize(t.verts.size());

	AnimInfo* anim = project.GetWorkAnim();
	auto skin = anim->shapeSkinning.find(shape->name.get());
	if (skin == anim->shapeSkinning.end())
		throw McpToolError("The template '" + shape->name.get() + "' has no bone weights.");

	size_t weighted = 0;
	for (auto& boneName : skin->second.boneNames) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName.first);
		auto weights = skin->second.boneWeights.find(boneName.second);
		if (!bone || weights == skin->second.boneWeights.end())
			continue;
		for (auto& w : weights->second.weights)
			if (w.first < t.influences.size() && w.second > 0.0f) {
				t.influences[w.first].push_back({bone, w.second});
				weighted++;
			}
	}
	if (weighted == 0)
		throw McpToolError("The template '" + shape->name.get() + "' has no bone weights.");
	return t;
}

std::vector<NiShape*> NamedShapes(OutfitStudioMcp& mcp, const json& args, const char* name, bool required) {
	std::vector<NiShape*> shapes;
	for (auto& shapeName : McpArgs::Strings(args, name))
		shapes.push_back(mcp.Shape(shapeName));
	if (shapes.empty() && required)
		throw McpToolError(std::string("Name the shapes in '") + name + "'.");
	return shapes;
}

// The templates named in args["templates"], or the reference
std::vector<NiShape*> TemplateShapes(OutfitStudioMcp& mcp, const json& args) {
	std::vector<NiShape*> shapes = NamedShapes(mcp, args, "templates", false);
	if (shapes.empty()) {
		NiShape* base = mcp.Project().GetBaseShape();
		if (!base)
			throw McpToolError("No reference is loaded. Load the body with load_reference, or name skinned shapes in 'templates'.");
		shapes.push_back(base);
	}
	return shapes;
}

// Shapes merged into one surface; triangles index one vertex list, so it has to stay within their 16-bit range
void MergeSurface(const std::vector<std::vector<Vector3>>& vertsOf, const std::vector<std::vector<Triangle>>& trisOf, std::vector<Vector3>& outVerts, std::vector<Triangle>& outTris) {
	size_t total = 0;
	for (auto& v : vertsOf)
		total += v.size();
	if (total > 65535)
		throw McpToolError("The shapes have " + std::to_string(total) + " vertices together, more than one surface can have (65535). Use fewer or smaller shapes.");

	outVerts.clear();
	outTris.clear();
	for (size_t s = 0; s < vertsOf.size(); s++) {
		const uint16_t offset = static_cast<uint16_t>(outVerts.size());
		outVerts.insert(outVerts.end(), vertsOf[s].begin(), vertsOf[s].end());
		for (auto& t : trisOf[s])
			outTris.emplace_back(static_cast<uint16_t>(t.p1 + offset), static_cast<uint16_t>(t.p2 + offset), static_cast<uint16_t>(t.p3 + offset));
	}
}

// The gap the masked vertices next to the unmasked ones have to the surface, their median: fitting the unmasked
// area at it brings it level with its surroundings. Rings is how far into the mask they are looked for.
std::optional<float> BorderGap(const std::vector<Vector3>& verts,
							   const std::vector<Triangle>& tris,
							   const std::vector<float>& lock,
							   const std::vector<Vector3>& surfaceVerts,
							   const std::vector<Triangle>& surfaceTris,
							   int rings,
							   float searchRadius,
							   size_t* outCount) {
	if (lock.size() != verts.size())
		return std::nullopt;

	std::vector<std::vector<int>> neighbors(verts.size());
	for (auto& t : tris)
		for (auto [a, b] : {std::pair<int, int>{t.p1, t.p2}, {t.p2, t.p3}, {t.p3, t.p1}})
			if (a < static_cast<int>(verts.size()) && b < static_cast<int>(verts.size())) {
				neighbors[a].push_back(b);
				neighbors[b].push_back(a);
			}

	// Rings out from the unmasked vertices into the masked ones
	std::vector<int> ring(verts.size(), -1);
	std::vector<int> front;
	for (size_t v = 0; v < verts.size(); v++)
		if (lock[v] < 0.5f) {
			ring[v] = 0;
			front.push_back(static_cast<int>(v));
		}
	if (front.empty() || front.size() == verts.size())
		return std::nullopt;

	std::vector<Vector3> border;
	for (int r = 1; r <= rings && !front.empty(); r++) {
		std::vector<int> next;
		for (int v : front)
			for (int n : neighbors[v])
				if (ring[n] < 0) {
					ring[n] = r;
					next.push_back(n);
					border.push_back(verts[n]);
				}
		front = std::move(next);
	}

	auto stats = MeshAnalysis::AnalyzePenetration(border, surfaceVerts, surfaceTris, 0.1f, searchRadius);
	std::vector<float> gaps;
	for (float d : stats.signedDistances)
		if (d != FLT_MAX)
			gaps.push_back(d);
	if (outCount)
		*outCount = gaps.size();
	if (gaps.empty())
		return std::nullopt;

	std::nth_element(gaps.begin(), gaps.begin() + gaps.size() / 2, gaps.end());
	return gaps[gaps.size() / 2];
}

std::vector<AnimBone*> SkeletonBoneList() {
	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	std::vector<std::string> names;
	skeleton.GetBoneNames(names);
	std::vector<AnimBone*> bones;
	for (auto& name : names)
		if (AnimBone* bone = skeleton.GetBonePtr(name))
			bones.push_back(bone);
	return bones;
}

AnimBone* BoneByName(const std::string& name) {
	AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name);
	if (!bone)
		throw McpToolError("There is no bone '" + name + "' in the skeleton. list_skeleton_bones lists them.");
	return bone;
}

bool IsFinger(const AnimBone* bone) {
	return bone->boneName.find("Finger") != std::string::npos || bone->boneName.find("finger") != std::string::npos;
}

// The bones to fit by default: the main bones that move template skin, the fingers with fingers: true, and the bone
// above them all (COM in Skyrim), so the body as a whole can move
std::vector<AnimBone*> DefaultFitBones(const std::vector<PoseFit::Template>& templates, bool fingers) {
	std::set<const AnimBone*> skinned;
	for (auto& t : templates)
		for (auto& vertex : t.influences)
			for (auto& influence : vertex)
				skinned.insert(influence.bone);

	// A bone counts when it or one of its helper bones below it (twist, breast) moves skin
	auto movesSkin = [&](const AnimBone* bone) {
		for (const AnimBone* s : skinned)
			for (const AnimBone* b = s; b; b = b->parent) {
				if (b == bone)
					return true;
				if (b != s && PoseMath::IsMainBone(b->boneName))
					break;
			}
		return false;
	};

	std::vector<AnimBone*> all = SkeletonBoneList();
	std::vector<AnimBone*> bones;
	for (AnimBone* bone : PoseMath::MainBones(all))
		if (movesSkin(bone))
			bones.push_back(bone);
	if (fingers)
		for (AnimBone* bone : all)
			if (IsFinger(bone) && skinned.count(bone))
				bones.push_back(bone);
	if (bones.empty())
		return bones;

	// The lowest common ancestor of all of them
	AnimBone* top = bones.front();
	auto isAbove = [](const AnimBone* a, const AnimBone* b) {
		for (const AnimBone* p = b; p; p = p->parent)
			if (p == a)
				return true;
		return false;
	};
	while (top && !std::all_of(bones.begin(), bones.end(), [&](const AnimBone* b) { return isAbove(top, b); }))
		top = top->parent;
	if (top && std::find(bones.begin(), bones.end(), top) == bones.end())
		bones.insert(bones.begin(), top);
	return bones;
}

json PoseSummary(const PoseFit::Result& fit, float units) {
	json bones = json::array();
	for (auto& b : fit.bones) {
		json entry = {{"bone", b.bone->boneName}, {"matches", b.matches}, {"distance", Round(b.distance * units)}, {"turned", Round(b.turnedDegrees, 0.1f)}};
		if (std::fabs(b.scale - 1.0f) > 1e-3f)
			entry["scale"] = Round(b.scale, 0.001f);
		bones.push_back(entry);
	}
	return bones;
}

McpToolResult FitPoseToShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();
	const float units = McpBoneUnits(mcp);

	std::vector<NiShape*> templateShapes = TemplateShapes(mcp, args);
	std::vector<NiShape*> targetShapes = NamedShapes(mcp, args, "targets", true);
	for (NiShape* target : targetShapes)
		if (std::find(templateShapes.begin(), templateShapes.end(), target) != templateShapes.end())
			throw McpToolError("'" + target->name.get() + "' is both a template and a target.");

	if (!McpArgs::Bool(args, "keepCurrent", true) || !project.bPose)
		ops.ResetPose();

	std::vector<PoseFit::Template> templates;
	for (NiShape* shape : templateShapes)
		templates.push_back(MakeTemplate(project, shape, units));

	std::vector<std::vector<Vector3>> targetVerts;
	std::vector<std::vector<Triangle>> targetTris;
	for (NiShape* shape : targetShapes) {
		targetVerts.push_back(GlobalVerts(project, shape, units));
		targetTris.emplace_back();
		shape->GetTriangles(targetTris.back());
	}
	std::vector<Vector3> surfaceVerts;
	std::vector<Triangle> surfaceTris;
	MergeSurface(targetVerts, targetTris, surfaceVerts, surfaceTris);

	PoseFit::Options options;
	if (McpArgs::Has(args, "bones")) {
		for (auto& name : McpArgs::Strings(args, "bones"))
			options.bones.push_back(BoneByName(name));
	}
	else
		options.bones = DefaultFitBones(templates, McpArgs::Bool(args, "fingers", false));
	if (options.bones.empty())
		throw McpToolError("No bones to fit: the templates move no main bones of the skeleton.");

	options.allowScale = McpArgs::Bool(args, "allowScale", true);
	options.moveRoot = McpArgs::Bool(args, "moveRoot", true);
	options.scaleBody = McpArgs::Bool(args, "scaleBody", false);
	options.iterations = std::clamp(McpArgs::Int(args, "iterations", options.iterations), 1, 200);
	options.maxDistance = McpArgs::Float(args, "maxDistance", options.maxDistance * units) / units;
	options.trim = std::clamp(McpArgs::Float(args, "trim", options.trim), 0.0f, 0.9f);
	options.maxTurnDegrees = std::clamp(McpArgs::Float(args, "maxTurn", options.maxTurnDegrees), 1.0f, 90.0f);
	options.minScale = McpArgs::Float(args, "minScale", options.minScale);
	options.maxScale = McpArgs::Float(args, "maxScale", options.maxScale);
	if (options.maxDistance <= 0.0f || options.minScale <= 0.0f || options.maxScale < options.minScale)
		throw McpToolError("maxDistance has to be above 0, and minScale above 0 and not over maxScale.");

	PoseFit::Result fit = PoseFit::Fit(templates, surfaceVerts, surfaceTris, options);

	PoseData posed;
	PoseDataCollection::CaptureCurrentPose("MCP", false, posed);
	frame->ApplyRefresh(ops.SetPreviewPose(posed));

	McpToolResult result;
	result.data["distanceBefore"] = Round(fit.distanceBefore * units);
	result.data["distanceAfter"] = Round(fit.distanceAfter * units);
	result.data["matchedVertices"] = fit.matchesAfter;
	result.data["bones"] = PoseSummary(fit, units);
	result.data["note"] =
		"The fitted pose is the preview pose now. Check it with render_view (skeleton: true, the templates and targets shown) from the front and the side, "
		"correct bones with set_preview_pose keepCurrent (its warnings show joints bent too far), then unpose_shapes.";
	return result;
}

McpToolResult ScaleToBody(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();

	if (project.bPose)
		throw McpToolError("A preview pose is on. Clear it with clear_preview_pose first: the scale is measured on the unposed body.");
	if (frame->editor.bEditSlider)
		throw McpToolError("A slider is in edit mode. Leave it with set_slider_edit first.");
	mcp.CheckPositionEdit();

	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	if (shapes.empty())
		throw McpToolError("No shapes are given or selected.");
	std::vector<NiShape*> targetShapes = NamedShapes(mcp, args, "targets", false);
	if (targetShapes.empty())
		targetShapes = shapes;
	std::vector<NiShape*> templateShapes = TemplateShapes(mcp, args);
	for (NiShape* shape : shapes)
		if (std::find(templateShapes.begin(), templateShapes.end(), shape) != templateShapes.end())
			throw McpToolError("'" + shape->name.get() + "' is a template; only the figure's shapes are scaled.");

	std::vector<PoseFit::Template> templates;
	for (NiShape* shape : templateShapes)
		templates.push_back(MakeTemplate(project, shape, 1.0f));

	std::vector<std::vector<Vector3>> targetVerts;
	std::vector<std::vector<Triangle>> targetTris;
	for (NiShape* shape : targetShapes) {
		targetVerts.push_back(GlobalVerts(project, shape, 1.0f));
		targetTris.emplace_back();
		shape->GetTriangles(targetTris.back());
	}
	std::vector<Vector3> surfaceVerts;
	std::vector<Triangle> surfaceTris;
	MergeSurface(targetVerts, targetTris, surfaceVerts, surfaceTris);

	PoseFit::ScaleEstimate estimate = PoseFit::EstimateScale(templates, surfaceVerts, surfaceTris);
	const bool estimated = !McpArgs::Has(args, "scale");
	const float scale = estimated ? estimate.scale : McpArgs::Float(args, "scale", 1.0f);
	if (scale <= 0.0f)
		throw McpToolError("scale has to be above 0.");
	const bool moveToFeet = McpArgs::Bool(args, "moveToFeet", true);
	const Vector3 destination = moveToFeet ? estimate.templateFloor : estimate.figureFloor;

	UndoHistory* history = frame->glView->GetUndoHistory();
	UndoStateProject* usp = history->PushState();
	usp->undoType = UndoType::VertexPosition;
	size_t changed = 0;
	for (NiShape* shape : shapes) {
		std::vector<Vector3> global = GlobalVerts(project, shape, 1.0f);
		std::vector<Vector3> current;
		project.GetWorkNif()->GetVertsForShape(shape, current);
		const MatTransform toShape = project.GetWorkAnim()->GetTransformGlobalToShape(shape);
		const bool transformed = !toShape.IsNearlyEqualTo(MatTransform());

		UndoStateShape uss;
		uss.shapeName = shape->name.get();
		for (size_t i = 0; i < global.size() && i < current.size(); i++) {
			Vector3 p = destination + (global[i] - estimate.figureFloor) * scale;
			if (transformed)
				p = toShape.ApplyTransform(p);
			Vector3 diff = p - current[i];
			if (!diff.IsZero(true))
				uss.restDiffs[static_cast<int>(i)] = diff;
		}
		changed += uss.restDiffs.size();
		if (!uss.restDiffs.empty())
			usp->usss.push_back(std::move(uss));
	}
	if (changed == 0) {
		history->PopState();
		throw McpToolError("Nothing moved: the shapes already have the size and place.");
	}
	frame->glView->ApplyUndoState(usp, false);
	frame->UpdateUndoTools();

	McpToolResult result;
	result.data["scale"] = Round(scale, 0.0001f);
	result.data["estimated"] = estimated;
	if (estimated) {
		result.data["meanDistance"] = Round(estimate.distance);
		result.data["method"] = estimate.byLegs ? "leg length" : "best match";
		if (estimate.byLegs) {
			result.data["figureCrotchHeight"] = Round(estimate.figureCrotch);
			result.data["templateCrotchHeight"] = Round(estimate.templateCrotch);
		}
		if (!estimate.byLegs)
			result.data["note"] = "The legs couldn't be measured (no gap between them), so the size is the best match, which a figure of other "
								  "proportions can throw off. Check it with render_view from the front and the side, and give scale to correct it.";
	}
	result.data["figureFeet"] = {Round(estimate.figureFloor.x), Round(estimate.figureFloor.y), Round(estimate.figureFloor.z)};
	result.data["templateFeet"] = {Round(estimate.templateFloor.x), Round(estimate.templateFloor.y), Round(estimate.templateFloor.z)};
	result.data["changedVertices"] = changed;
	return result;
}

// Maps each bone of a weight set to its parent while it's one of the excluded bones or below one
void ApplyBoneExclusions(std::vector<std::vector<PoseFit::Template::Influence>>& weights, const std::vector<AnimBone*>& excluded) {
	if (excluded.empty())
		return;
	auto replacement = [&](AnimBone* bone) {
		AnimBone* topExcluded = nullptr;
		for (AnimBone* b = bone; b; b = b->parent)
			if (std::find(excluded.begin(), excluded.end(), b) != excluded.end())
				topExcluded = b;
		return topExcluded ? topExcluded->parent : bone;
	};
	for (auto& vertex : weights) {
		std::map<AnimBone*, float> merged;
		for (auto& influence : vertex)
			if (AnimBone* bone = replacement(influence.bone))
				merged[bone] += influence.weight;
		vertex.clear();
		for (auto& m : merged)
			vertex.push_back({m.first, m.second});
	}
}

McpToolResult UnposeShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();
	const float units = McpBoneUnits(mcp);

	if (!project.bPose)
		throw McpToolError("No preview pose is on. Fit the skeleton to the shapes with fit_pose_to_shapes (or pose it with set_preview_pose) first.");
	if (frame->editor.bEditSlider)
		throw McpToolError("A slider is in edit mode. Leave it with set_slider_edit first: unposing changes the base shapes.");
	mcp.CheckPositionEdit();

	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	std::vector<NiShape*> templateShapes = TemplateShapes(mcp, args);
	for (NiShape* shape : shapes)
		if (std::find(templateShapes.begin(), templateShapes.end(), shape) != templateShapes.end())
			throw McpToolError("'" + shape->name.get() + "' is a template. Only shapes that are where the posed templates are get unposed.");

	std::vector<PoseFit::Template> templates;
	for (NiShape* shape : templateShapes)
		templates.push_back(MakeTemplate(project, shape, units));

	std::vector<AnimBone*> excluded;
	for (auto& name : McpArgs::Strings(args, "excludeBones"))
		excluded.push_back(BoneByName(name));

	const float searchRadius = McpArgs::Float(args, "searchRadius", 50.0f) / units;
	const int smoothIterations = std::clamp(McpArgs::Int(args, "smoothIterations", 4), 0, 100);
	const bool rigidParts = McpArgs::Bool(args, "rigidParts", false);
	const int maxPartVertices = std::max(1, McpArgs::Int(args, "maxPartVertices", 300));

	UndoHistory* history = frame->glView->GetUndoHistory();
	UndoStateProject* usp = history->PushState();
	usp->undoType = UndoType::VertexPosition;

	json shapesResult = json::object();
	size_t changed = 0;
	for (NiShape* shape : shapes) {
		std::vector<Vector3> posedPoints = GlobalVerts(project, shape, units);
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);

		auto weights = PoseFit::WeightsFromTemplates(templates, posedPoints, searchRadius);
		size_t unweighted = static_cast<size_t>(std::count_if(weights.begin(), weights.end(), [](const auto& w) { return w.empty(); }));
		ApplyBoneExclusions(weights, excluded);
		PoseFit::SmoothWeights(weights, posedPoints, tris, smoothIterations, 0.5f);

		// Small connected parts (chain links, buckles, nails) get one set of weights, so they move as a whole
		size_t rigidCount = 0;
		if (rigidParts) {
			int partCount = 0;
			std::vector<int> parts = MeshAnalysis::ConnectedParts(posedPoints, tris, 0.001f, nullptr, &partCount);
			std::vector<std::vector<int>> members(partCount);
			for (size_t i = 0; i < parts.size(); i++)
				if (parts[i] >= 0)
					members[parts[i]].push_back(static_cast<int>(i));
			for (auto& part : members) {
				if (part.empty() || static_cast<int>(part.size()) > maxPartVertices)
					continue;
				std::map<AnimBone*, float> mean;
				for (int i : part)
					for (auto& influence : weights[i])
						mean[influence.bone] += influence.weight / static_cast<float>(part.size());
				std::vector<PoseFit::Template::Influence> shared;
				for (auto& m : mean)
					shared.push_back({m.first, m.second});
				for (int i : part)
					weights[i] = shared;
				rigidCount++;
			}
		}

		std::vector<Vector3> rest = PoseFit::Unpose(posedPoints, weights);

		// Back to the shape's own space and units
		const MatTransform toShape = project.GetWorkAnim()->GetTransformGlobalToShape(shape);
		const bool transformed = !toShape.IsNearlyEqualTo(MatTransform());
		std::vector<Vector3> current;
		project.GetWorkNif()->GetVertsForShape(shape, current);

		UndoStateShape uss;
		uss.shapeName = shape->name.get();
		double moveSum = 0.0;
		float maxMove = 0.0f;
		for (size_t i = 0; i < rest.size() && i < current.size(); i++) {
			Vector3 p = rest[i] * units;
			if (transformed)
				p = toShape.ApplyTransform(p);
			Vector3 diff = p - current[i];
			if (diff.IsZero(true))
				continue;
			uss.restDiffs[static_cast<uint16_t>(i)] = diff;
			moveSum += diff.length();
			maxMove = std::max(maxMove, diff.length());
		}

		json entry = {{"movedVertices", uss.restDiffs.size()}, {"meanMove", uss.restDiffs.empty() ? 0.0f : Round(static_cast<float>(moveSum / uss.restDiffs.size()))}, {"maxMove", Round(maxMove)}};
		MeshAnalysis::StrainStats strain = MeshAnalysis::AnalyzeStrain(posedPoints, rest, tris, 0.25f);
		entry["maxStrain"] = Round(strain.maxStrain);
		entry["strainedEdges"] = strain.stretched;
		if (unweighted)
			entry["notNearTemplates"] = unweighted;
		if (rigidParts)
			entry["rigidParts"] = rigidCount;
		shapesResult[shape->name.get()] = entry;
		changed += uss.restDiffs.size();
		if (!uss.restDiffs.empty())
			usp->usss.push_back(std::move(uss));
	}

	if (changed == 0) {
		history->PopState();
		throw McpToolError("Nothing moved: the pose doesn't move the shapes' closest template skin.");
	}

	frame->glView->ApplyUndoState(usp, false);
	frame->UpdateUndoTools();

	if (McpArgs::Bool(args, "clearPose", true)) {
		ops.ResetPose();
		frame->ApplyRefresh(ops.ClearPreviewPose());
	}

	McpToolResult result;
	result.data["shapes"] = shapesResult;
	result.data["poseCleared"] = !project.bPose;
	return result;
}

McpToolResult FitToBody(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();

	if (project.bPose)
		throw McpToolError("A preview pose is on. Clear it with clear_preview_pose first: the fit works on the unposed shapes.");

	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	if (shapes.empty())
		throw McpToolError("No shapes are given or selected.");

	std::vector<NiShape*> against = NamedShapes(mcp, args, "against", false);
	if (against.empty()) {
		if (!project.GetBaseShape())
			throw McpToolError("No reference is loaded. Load one with load_reference, or name the body's shapes in 'against'.");
		against.push_back(project.GetBaseShape());
	}
	for (NiShape* shape : shapes)
		if (std::find(against.begin(), against.end(), shape) != against.end())
			throw McpToolError("'" + shape->name.get() + "' can't be fitted to itself.");

	const std::string sliderName = McpArgs::String(args, "asSlider", "");
	if (sliderName.empty()) {
		if (frame->editor.bEditSlider)
			throw McpToolError("A slider is in edit mode. Leave it with set_slider_edit first, or pass asSlider to write the fit into a slider.");
		mcp.CheckPositionEdit();
	}
	for (NiShape* shape : shapes)
		if (!sliderName.empty() && project.IsBaseShape(shape))
			throw McpToolError("The reference can't take slider data this way. Fit it as an outfit shape (set_reference_shape another shape first), then make it the reference again.");

	// The body as the sliders show it now
	std::vector<std::vector<Vector3>> bodyVerts;
	std::vector<std::vector<Triangle>> bodyTris;
	for (NiShape* shape : against) {
		bodyVerts.emplace_back();
		project.GetMorphedVerts(shape, bodyVerts.back());
		bodyTris.push_back(mcp.SurfaceTriangles(shape));
	}
	std::vector<Vector3> surfaceVerts;
	std::vector<Triangle> surfaceTris;
	MergeSurface(bodyVerts, bodyTris, surfaceVerts, surfaceTris);

	std::vector<NiShape*> gapFrom = NamedShapes(mcp, args, "gapFrom", false);
	std::vector<Vector3> gapVerts;
	std::vector<Triangle> gapTris;
	if (!gapFrom.empty()) {
		std::vector<std::vector<Vector3>> v;
		std::vector<std::vector<Triangle>> t;
		for (NiShape* shape : gapFrom) {
			v.emplace_back();
			project.GetMorphedVerts(shape, v.back());
			t.emplace_back();
			shape->GetTriangles(t.back());
		}
		MergeSurface(v, t, gapVerts, gapTris);
	}

	const std::string gapMode = McpArgs::String(args, "gapMode", "constant");
	if (gapMode != "constant" && gapMode != "keep" && gapMode != "fromShape")
		McpArgs::Wrong("gapMode", "\"constant\", \"keep\" or \"fromShape\"");
	if (gapMode == "fromShape" && gapFrom.empty())
		throw McpToolError("gapMode fromShape measures the gap to the shapes in 'gapFrom'. Name them.");

	SurfaceFit::Options options;
	options.gap = McpArgs::Float(args, "gap", 0.3f);
	options.maxDistance = McpArgs::Float(args, "maxDistance", 10.0f);
	options.minNormalAgreement = std::clamp(McpArgs::Float(args, "minNormalAgreement", 0.0f), -1.0f, 1.0f);
	options.symmetric = McpArgs::Bool(args, "symmetric", false);
	options.stiffness = std::max(McpArgs::Float(args, "stiffness", 1.0f), 1e-3f);
	options.stages = std::clamp(McpArgs::Int(args, "stages", options.stages), 1, 10);
	options.iterations = std::clamp(McpArgs::Int(args, "iterations", options.iterations), 1, 20);
	const float minGap = McpArgs::Float(args, "minGap", 0.05f);
	options.minGap = minGap;
	options.reliefSmoothing = std::clamp(McpArgs::Int(args, "relief", gapMode == "constant" ? 10 : 0), 0, 100);
	options.clippingPasses = std::clamp(McpArgs::Int(args, "clippingPasses", options.clippingPasses), 0, 20);
	options.maxStrain = std::max(McpArgs::Float(args, "maxStrain", options.maxStrain), 0.0f);
	const bool useMask = McpArgs::Bool(args, "useMask", true);
	const bool rigidParts = McpArgs::Bool(args, "rigidParts", false);
	const int maxPartVertices = std::max(1, McpArgs::Int(args, "maxPartVertices", 300));
	options.rigidScale = McpArgs::Bool(args, "rigidScale", false);
	const bool autoGap = McpArgs::Bool(args, "autoGap", false);
	const int autoGapRings = std::clamp(McpArgs::Int(args, "autoGapRings", 3), 1, 50);
	if (autoGap && gapMode != "constant")
		throw McpToolError("autoGap measures the constant gap; use it with gapMode constant.");
	if (autoGap && !useMask)
		throw McpToolError("autoGap measures the gap at the border of the mask; it needs useMask.");

	UndoHistory* history = frame->glView->GetUndoHistory();
	UndoStateProject* usp = history->PushState();
	usp->undoType = UndoType::VertexPosition;

	if (!sliderName.empty() && !project.ValidSlider(sliderName))
		frame->NewSlider(sliderName, true);

	ProjectOperations ops = mcp.Ops();
	OpResult opResult;
	json shapesResult = json::object();
	for (NiShape* shape : shapes) {
		std::vector<Vector3> verts;
		project.GetWorkNif()->GetVertsForShape(shape, verts);
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);

		SurfaceFit::Options shapeOptions = options;
		Mesh* m = project.GetHost().GetShapeMesh(shape->name.get());
		if (useMask && m && m->mask && m->nVerts == static_cast<int>(verts.size()))
			shapeOptions.lock.assign(m->mask.get(), m->mask.get() + m->nVerts);

		size_t borderVertices = 0;
		if (autoGap) {
			auto gap = BorderGap(verts, tris, shapeOptions.lock, surfaceVerts, surfaceTris, autoGapRings, options.maxDistance, &borderVertices);
			if (!gap) {
				history->PopState();
				throw McpToolError("autoGap found no masked vertices near the body next to the unmasked ones of '" + shape->name.get() +
								   "'. Mask the shape outside the area to fit first.");
			}
			shapeOptions.gap = std::max(*gap, minGap);
		}

		// keep: the fit measures the gaps itself, the way it pulls
		shapeOptions.keepGaps = gapMode == "keep";
		if (gapMode == "fromShape") {
			// The gap each vertex has now to the shapes named in gapFrom
			auto stats = MeshAnalysis::AnalyzePenetration(verts, gapVerts, gapTris, 0.1f, options.maxDistance);
			shapeOptions.targetGaps.resize(verts.size());
			for (size_t i = 0; i < verts.size(); i++) {
				float d = stats.signedDistances[i];
				shapeOptions.targetGaps[i] = d == FLT_MAX ? options.gap : std::max(d, minGap);
			}
		}

		if (rigidParts) {
			int partCount = 0;
			std::vector<int> parts = MeshAnalysis::ConnectedParts(verts, tris, 0.001f, nullptr, &partCount);
			std::vector<int> sizes(partCount, 0);
			for (int p : parts)
				if (p >= 0)
					sizes[p]++;
			for (auto& p : parts)
				if (p >= 0 && sizes[p] > maxPartVertices)
					p = -1;
			shapeOptions.rigidParts = parts;
		}

		SurfaceFit::Result fit = SurfaceFit::Fit(verts, tris, surfaceVerts, surfaceTris, shapeOptions);

		std::unordered_map<uint16_t, Vector3> diffs;
		for (size_t i = 0; i < verts.size(); i++) {
			Vector3 diff = fit.positions[i] - verts[i];
			if (!diff.IsZero(true))
				diffs[static_cast<uint16_t>(i)] = diff;
		}

		if (!sliderName.empty()) {
			OpResult r = ops.ChangeSliderDeltas(shape, sliderName, diffs, *usp);
			if (!r.ok) {
				history->PopState();
				throw McpToolError(r.error);
			}
			opResult.refresh = opResult.refresh | r.refresh;
		}
		else if (!diffs.empty()) {
			UndoStateShape uss;
			uss.shapeName = shape->name.get();
			for (auto& diff : diffs)
				uss.restDiffs[diff.first] = diff.second;
			usp->usss.push_back(std::move(uss));
		}

		shapesResult[shape->name.get()] = {
			{"movedVertices", diffs.size()},
			{"meanMove", Round(fit.meanMove)},
			{"maxMove", Round(fit.maxMove)},
			{"pulled", fit.pulled},
			{"followed", fit.followed},
			{"meanGapError", Round(fit.meanGapError)},
			{"maxGapError", Round(fit.maxGapError)},
			{"insideBefore", fit.insideBefore},
			{"insideAfter", fit.insideAfter},
			{"bodyPokingThroughBefore", fit.pokeBefore},
			{"bodyPokingThroughAfter", fit.pokeAfter},
			{"flippedTriangles", fit.flipped},
			{"maxStrain", Round(fit.maxStrain)},
			{"meanStrain", Round(fit.meanStrain, 0.001f)},
			{"strainedEdges", fit.strainedEdges},
			{"keptForm", fit.relaxed},
		};
		if (autoGap)
			shapesResult[shape->name.get()]["autoGap"] = {{"gap", Round(shapeOptions.gap)}, {"borderVertices", borderVertices}};
	}

	if (usp->usss.empty()) {
		history->PopState();
		throw McpToolError("Nothing moved: the shapes are farther than maxDistance from the body, or fully masked.");
	}

	if (sliderName.empty())
		frame->glView->ApplyUndoState(usp, false);
	else
		mcp.Apply(opResult);
	frame->UpdateUndoTools();

	McpToolResult result;
	result.data["shapes"] = shapesResult;
	if (!sliderName.empty()) {
		result.data["slider"] = sliderName;
		result.data["note"] = "The fit is in the slider. Check it at 1 (set_slider_values, render_view, analyze_slider), then bake it with set_base_shape or conform other shapes to it.";
	}
	return result;
}

} // namespace

void AddFittingTools(OutfitStudioMcp& mcp) {
	const json shapesProp = McpStringList("Shapes, the selected outfit shapes by default");

	McpTool scaleTool;
	scaleTool.name = "scale_to_body";
	scaleTool.description =
		"Scales a figure's shapes evenly about their feet to the size of the body, and moves their feet onto the body's feet: the "
		"first step for a model from another program, before fit_pose_to_shapes. The size is found from how well the figure's "
		"targets (by default the shapes themselves; best its skin body and leg parts like boots) match the legs and torso of the "
		"unposed templates both ways, so arms in another pose don't count. Give the templates feet (\"CBBE Feet\" with "
		"add_project) so both have feet to stand on. scale sets the size instead of finding it. One undo step.";
	scaleTool.inputSchema = McpSchema({
		{"shapes", McpStringList("All shapes of the figure to scale and move, the selected outfit shapes by default")},
		{"targets", McpStringList("Shapes of the figure to measure by. Default: the shapes.")},
		{"templates", McpStringList("Skinned shapes of the body. Default: the reference.")},
		{"scale", McpProp("number", "Scale to use instead of finding it")},
		{"moveToFeet", McpProp("boolean", "Move the figure's feet onto the body's. Default true.")},
	});
	scaleTool.handler = [&mcp](const json& args) { return ScaleToBody(mcp, args); };
	mcp.AddTool(std::move(scaleTool));

	McpTool fitPose;
	fitPose.name = "fit_pose_to_shapes";
	fitPose.description =
		"Fits the skeleton to a figure that has no skeleton, like a model from another program in an A-pose or with other proportions: "
		"poses the bones (and with allowScale scales them) so the skinned templates (the reference body by default, plus hands or "
		"feet added with add_project) are where the target shapes are. Scale the figure to the body with scale_to_body first. "
		"Then this goes in two phases: the whole body is moved and turned to the figure by its torso (moveRoot), then the joints "
		"are fitted, each within its natural range, and last the bones' lengths are corrected a little (allowScale). Targets are the figure's body or close-fitting parts, e.g. its "
		"skin body, gloves and boots, not loose parts like a cape or hat. Matches go both ways, with the worst left out (trim), so "
		"parts only one side has don't pull. Starts from the current preview pose (keepCurrent): for limbs that are far off, aim "
		"them roughly with set_preview_pose first. The result is the preview pose, with per bone how far its skin is from the "
		"figure; unpose_shapes then takes the figure's shapes to the skeleton's rest pose.";
	fitPose.inputSchema = McpSchema(
		{
			{"targets", McpStringList("Shapes of the figure to fit the skeleton to")},
			{"templates", McpStringList("Skinned shapes that stand for the body. Default: the reference.")},
			{"bones", McpStringList("Bones to fit. Default: the main bones that move the templates and the bone above them all.")},
			{"fingers", McpProp("boolean", "Also fit the finger bones, when the templates have hands. Default false.")},
			{"allowScale", McpProp("boolean", "After the pose, let bones grow or shrink a little with their children (minScale to maxScale), for other proportions. Default true.")},
			{"moveRoot", McpProp("boolean", "First move and turn the body as a whole to the figure by its torso, with the topmost bone. Default true.")},
			{"scaleBody", McpProp("boolean", "With moveRoot: also scale the whole skeleton to the figure. Default false: scale the figure with scale_to_body first, which is more reliable.")},
			{"keepCurrent", McpProp("boolean", "Start from the current preview pose. Default true; false starts unposed.")},
			{"iterations", McpProp("integer", "Default 20.")},
			{"maxDistance", McpProp("number", "Points farther apart than this aren't matched. Default 12.")},
			{"trim", McpProp("number", "Share of each bone's worst matches left out, 0 to 0.9. Default 0.2.")},
			{"maxTurn", McpProp("number", "Most degrees a bone turns per iteration. Default 15.")},
			{"minScale", McpProp("number", "Smallest scale of a bone against its parent. Default 0.8.")},
			{"maxScale", McpProp("number", "Largest scale of a bone against its parent. Default 1.25.")},
		},
		{"targets"});
	fitPose.handler = [&mcp](const json& args) { return FitPoseToShapes(mcp, args); };
	mcp.AddTool(std::move(fitPose));

	McpTool unpose;
	unpose.name = "unpose_shapes";
	unpose.description =
		"Takes shapes that are where the preview pose puts the templates back to the skeleton's rest pose, e.g. after "
		"fit_pose_to_shapes: each vertex gets the bone weights of the closest point of the posed templates (smoothed over the "
		"mesh), and the inverse of its blended pose transform, so it ends up where its skin is unposed. Arms in an A-pose come "
		"down to the game's rest pose and other proportions change to the skeleton's. The shapes keep no weights from this: "
		"copy_bone_weights afterwards. excludeBones hands bones' weights to their parent, e.g. the upper arms for a cape that "
		"shouldn't follow the arms. rigidParts moves small connected parts (chain links, buckles) as a whole. One undo step; "
		"clears the preview pose afterwards.";
	unpose.inputSchema = McpSchema({
		{"shapes", shapesProp},
		{"templates", McpStringList("Skinned shapes whose posed skin gives the weights. Default: the reference.")},
		{"excludeBones", McpStringList("Bones whose weights (and those of the bones below them) go to their parent")},
		{"searchRadius", McpProp("number", "How far to look for template skin. Default 50.")},
		{"smoothIterations", McpProp("integer", "Smoothing passes of the weights. Default 4.")},
		{"rigidParts", McpProp("boolean", "Move connected parts of up to maxPartVertices vertices as a whole. Default false.")},
		{"maxPartVertices", McpProp("integer", "Default 300.")},
		{"clearPose", McpProp("boolean", "Clear the preview pose afterwards. Default true.")},
	});
	unpose.handler = [&mcp](const json& args) { return UnposeShapes(mcp, args); };
	mcp.AddTool(std::move(unpose));

	McpTool fit;
	fit.name = "fit_to_body";
	fit.description =
		"Fits shapes onto the body without losing their own form: a non-rigid fit whose displacement is kept smooth, so the "
		"shapes as a whole move onto the body while folds, seams, rims and buckles keep their shape. For outfits made for "
		"another body: close-fitting parts (boots, gloves, a corset) to a constant gap, or keeping the gap they have to the "
		"old body (gapMode fromShape with that body in gapFrom). symmetric also pulls the shape onto body parts it doesn't "
		"reach yet, which morphs one body onto another of another topology; as a slider (asSlider) on the old body, other "
		"shapes then conform to it like to a conversion reference. Faces pointing away from the body (the inside of a thick "
		"garment) aren't pulled and keep their offset; vertices farther than maxDistance follow their neighbors. Masked "
		"vertices stay. Clipping left at the end is pushed out (clippingPasses). For loose parts like a cape, gapMode keep only "
		"pushes out what clips and leaves the rest. The result has, per shape, how close the pulled vertices got to their gap, "
		"the vertices inside the body and the body vertices poking through before and after, flipped triangles and the edge "
		"strain (how much edges got longer or shorter: over 0.25 is visible stretching). One undo step. Raise stiffness when "
		"details bend or strain is high, lower it when the fit stays loose.";
	fit.inputSchema = McpSchema({
		{"shapes", shapesProp},
		{"against", McpStringList("Shapes of the body. Default: the reference. Measured with the current slider values.")},
		{"gap", McpProp("number", "Distance outside the body to fit to (constant), and the default elsewhere. Default 0.3.")},
		{"autoGap",
		 McpProp("boolean",
				 "gapMode constant: fit at the gap the masked vertices right next to the unmasked ones have (their median), so an area "
				 "fitted on its own, like the cloth between the breasts onto a filled cleavage, comes in level with its surroundings. "
				 "Each shape's gap is in the result. Default false.")},
		{"autoGapRings", McpProp("integer", "autoGap: how many rings of masked vertices next to the unmasked ones are measured. Default 3.")},
		{"gapMode",
		 {{"type", "string"},
		  {"enum", {"constant", "keep", "fromShape"}},
		  {"description", "constant: gap everywhere. keep: the gap each vertex has to the body now, at least minGap (only pulls clipping vertices out and smooths). fromShape: the gap each vertex has to the shapes in gapFrom, e.g. the outfit's own old body. Default constant."}}},
		{"gapFrom", McpStringList("gapMode fromShape: the shapes to measure the gap to")},
		{"minGap", McpProp("number", "Smallest gap any vertex is fitted to. Default 0.05.")},
		{"relief",
		 McpProp("integer",
				 "Smoothing passes that find the shape's relief (seam ridges, rims, straps on top), which is added to the gap so it isn't "
				 "pressed flat. Larger finds broader relief. Default 10 with gapMode constant, else 0 (the measured gaps have it).")},
		{"maxDistance", McpProp("number", "Vertices farther from the body aren't pulled. Default 10.")},
		{"minNormalAgreement", McpProp("number", "Least cosine between a vertex normal and the body's to pull it, -1 to 1. Default 0; -1 for meshes with flipped normals.")},
		{"symmetric", McpProp("boolean", "Also pull the shape onto body points it doesn't reach yet. For morphing a body onto another. Default false.")},
		{"stiffness", McpProp("number", "How stiff the form stays. Default 1.")},
		{"stages", McpProp("integer", "Stiff-to-loose stages. Default 4.")},
		{"iterations", McpProp("integer", "Correspondence updates per stage. Default 3.")},
		{"asSlider", McpProp("string", "Write the fit into this slider (made if missing) instead of the base shape")},
		{"maxStrain",
		 McpProp("number",
				 "Where the fit would stretch or squash edges by more than this (0.3 is 30%), the pull gives way and the shape keeps its "
				 "own form there; keptForm in the result counts those vertices. Default 0.3; 0 for no limit.")},
		{"clippingPasses",
		 McpProp("integer",
				 "Passes at the end that push out what still clips, gently: vertices inside the body go out to minGap, and where the "
				 "body pokes through between the shape's vertices (a dense body under a coarse corset) the shape bulges out over it. "
				 "Default 4; 0 for none.")},
		{"rigidParts", McpProp("boolean", "Move connected parts of up to maxPartVertices vertices as a whole. Default false.")},
		{"maxPartVertices", McpProp("integer", "Default 300.")},
		{"rigidScale",
		 McpProp("boolean",
				 "Rigid parts may also grow or shrink evenly. With a maxPartVertices as large as the shape, a stiff piece like a "
				 "collar, choker or bracelet then moves and scales as a whole instead of bending onto the body. Default false.")},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	fit.handler = [&mcp](const json& args) { return FitToBody(mcp, args); };
	mcp.AddTool(std::move(fit));
}
