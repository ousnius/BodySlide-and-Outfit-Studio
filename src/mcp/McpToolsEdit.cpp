/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/MaskOps.h"
#include "../program/BrushStrokeController.h"
#include "../program/OutfitStudio.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

constexpr size_t MaxVerticesPerCall = 20000;

Mesh* ShapeMesh(OutfitStudioMcp& mcp, NiShape* shape) {
	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	if (!m)
		throw McpToolError("The shape '" + shape->name.get() + "' has no mesh in the view.");
	return m;
}

int VertexIndex(const std::string& key, const Mesh* m) {
	int vertex = -1;
	try {
		vertex = std::stoi(key);
	}
	catch (const std::exception&) {
		throw McpToolError("'" + key + "' isn't a vertex index.");
	}
	if (vertex < 0 || vertex >= m->nVerts)
		throw McpToolError("Vertex " + key + " doesn't exist on '" + m->shapeName + "'.");
	return vertex;
}

// Vertex values given as {"index": value} or as a list of indices that all get the same value
std::unordered_map<int, float> VertexValues(const json& args, const char* name, const Mesh* m, float listValue) {
	std::unordered_map<int, float> values;
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return values;

	if (value->is_array()) {
		for (auto& item : *value) {
			if (!item.is_number_integer())
				McpArgs::Wrong(name, "a list of vertex indices or an object of vertex indices to values");
			values[VertexIndex(std::to_string(item.get<int>()), m)] = listValue;
		}
	}
	else if (value->is_object()) {
		for (auto& item : value->items()) {
			if (!item.value().is_number())
				McpArgs::Wrong(name, "a list of vertex indices or an object of vertex indices to values");
			values[VertexIndex(item.key(), m)] = item.value().get<float>();
		}
	}
	else
		McpArgs::Wrong(name, "a list of vertex indices or an object of vertex indices to values");

	return values;
}

json MaskSummary(const Mesh& m) {
	return McpMaskSummary(m);
}

// Mask summaries by shape name. Steps can rebuild the meshes of the view, so they are looked up again.
McpToolResult MaskSummaries(OutfitStudioMcp& mcp, const json& names) {
	McpToolResult result;
	json shapes = json::object();
	for (auto& name : names)
		shapes[name.get<std::string>()] = MaskSummary(*ShapeMesh(mcp, mcp.Shape(name.get<std::string>())));
	result.data["shapes"] = shapes;
	return result;
}

// Brushes the brush_stroke tool can use, by name
std::unique_ptr<TweakBrush> MakeBrush(const std::string& name) {
	if (name == "inflate")
		return std::make_unique<TB_Inflate>();
	if (name == "deflate")
		return std::make_unique<TB_Deflate>();
	if (name == "smooth")
		return std::make_unique<TB_Smooth>();
	if (name == "undiff")
		return std::make_unique<TB_Undiff>();
	if (name == "mask")
		return std::make_unique<TB_Mask>();
	if (name == "unmask") {
		auto brush = std::make_unique<TB_Unmask>();
		brush->setStrength(-TB_Mask().getStrength());
		return brush;
	}
	if (name == "smooth_mask")
		return std::make_unique<TB_SmoothMask>();
	if (name == "weight")
		return std::make_unique<TB_Weight>();
	if (name == "unweight") {
		auto brush = std::make_unique<TB_Unweight>();
		brush->setStrength(-TB_Weight().getStrength());
		return brush;
	}
	if (name == "smooth_weight") {
		auto brush = std::make_unique<TB_SmoothWeight>();
		brush->setStrength(TB_Weight().getStrength() * 15.0f);
		return brush;
	}
	if (name == "color")
		return std::make_unique<TB_Color>();
	if (name == "uncolor") {
		auto brush = std::make_unique<TB_Uncolor>();
		brush->setStrength(-TB_Color().getStrength());
		return brush;
	}
	if (name == "alpha")
		return std::make_unique<TB_Alpha>();
	if (name == "unalpha") {
		auto brush = std::make_unique<TB_Unalpha>();
		brush->setStrength(-TB_Alpha().getStrength());
		return brush;
	}

	throw McpToolError("Unknown brush '" + name + "'.");
}

const std::vector<std::string> BrushNames = {"inflate", "deflate", "smooth", "undiff", "mask", "unmask", "smooth_mask", "weight", "unweight",
											 "smooth_weight", "color", "uncolor", "alpha", "unalpha"};

// Points the weight brushes at their bones, like the window does for the active bone
template<typename Brush>
void SetupWeightBrush(TweakBrush* brush, const WeightBrushBones& bones, AnimInfo* animInfo, bool normalize) {
	if (auto weightBrush = dynamic_cast<Brush*>(brush)) {
		bones.ApplyTo(*weightBrush, animInfo);
		weightBrush->bNormalizeWeights = normalize;
	}
}

McpToolResult BrushStroke(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();

	std::string brushName = McpArgs::String(args, "brush");
	std::unique_ptr<TweakBrush> brush = MakeBrush(brushName);
	const bool isWeight = brush->Type() == TweakBrush::BrushType::Weight;

	// Like the window, brushes that move vertices follow the slider rules
	const TweakBrush::BrushType type = brush->Type();
	if (type != TweakBrush::BrushType::Mask && type != TweakBrush::BrushType::Weight && type != TweakBrush::BrushType::Color && type != TweakBrush::BrushType::Alpha)
		mcp.CheckPositionEdit();

	auto shapes = mcp.Shapes(args, "shapes");
	std::vector<Mesh*> meshes;
	for (auto* shape : shapes)
		meshes.push_back(ShapeMesh(mcp, shape));

	std::string pathShape = McpArgs::String(args, "shape", shapes.front()->name.get());
	auto pathMesh = std::find_if(meshes.begin(), meshes.end(), [&](Mesh* m) { return m->shapeName == pathShape; });
	if (pathMesh == meshes.end())
		throw McpToolError("The path shape '" + pathShape + "' has to be one of the shapes the stroke works on.");

	// Where the brush goes: vertices of the path shape, or points at the nearest vertex of the shapes
	std::vector<TweakPickInfo> picks;
	std::vector<int> path = McpArgs::Ints(args, "path");
	for (int v : path) {
		if (v < 0 || v >= (*pathMesh)->nVerts)
			throw McpToolError("Vertex " + std::to_string(v) + " doesn't exist on '" + pathShape + "'.");
		picks.push_back(BrushStrokeController::PickAtVertex(*pathMesh, v));
	}

	if (const json* points = McpArgs::Find(args, "points")) {
		if (!path.empty())
			throw McpToolError("Give either path or points, not both.");
		if (!points->is_array())
			McpArgs::Wrong("points", "an array of [x, y, z] positions");

		std::vector<std::vector<Vector3>> liveVerts(shapes.size());
		for (size_t s = 0; s < shapes.size(); s++)
			project.GetLiveVerts(shapes[s], liveVerts[s]);

		for (auto& point : *points) {
			if (!point.is_array() || point.size() != 3 || !point[0].is_number() || !point[1].is_number() || !point[2].is_number())
				McpArgs::Wrong("points", "an array of [x, y, z] positions");
			Vector3 p(point[0].get<float>(), point[1].get<float>(), point[2].get<float>());

			size_t bestShape = 0;
			int bestVertex = -1;
			float bestDist = FLT_MAX;
			for (size_t s = 0; s < shapes.size(); s++) {
				for (int v = 0; v < meshes[s]->nVerts && v < static_cast<int>(liveVerts[s].size()); v++) {
					float d = p.DistanceSquaredTo(liveVerts[s][v]);
					if (d < bestDist) {
						bestDist = d;
						bestShape = s;
						bestVertex = v;
					}
				}
			}
			if (bestVertex < 0)
				throw McpToolError("The shapes have no vertices to paint.");
			picks.push_back(BrushStrokeController::PickAtVertex(meshes[bestShape], bestVertex));
		}
	}

	if (picks.empty())
		throw McpToolError("Give the path as vertices (path) or positions (points), at least one.");

	// Dabs in between, like dragging the brush from one point to the next
	if (McpArgs::Has(args, "step")) {
		const float step = Mesh::TransformDistNifToMesh(McpArgs::Float(args, "step", 0.5f));
		if (step <= 0.0f)
			McpArgs::Wrong("step", "a distance above 0");

		std::vector<TweakPickInfo> dense{picks.front()};
		for (size_t i = 1; i < picks.size(); i++) {
			const TweakPickInfo& a = picks[i - 1];
			const TweakPickInfo& b = picks[i];
			const int count = static_cast<int>(std::ceil(a.origin.DistanceTo(b.origin) / step));
			for (int n = 1; n < count; n++) {
				const float t = static_cast<float>(n) / count;
				TweakPickInfo pick = a;
				pick.origin = a.origin + (b.origin - a.origin) * t;
				pick.normal = a.normal + (b.normal - a.normal) * t;
				pick.normal.Normalize();
				pick.view = pick.normal;
				dense.push_back(pick);
			}
			dense.push_back(b);
		}
		picks = std::move(dense);
	}

	const int passes = std::max(1, McpArgs::Int(args, "passes", 1));

	brush->setRadius(Mesh::TransformDistNifToMesh(McpArgs::Float(args, "radius", 4.5f)));
	if (McpArgs::Has(args, "strength")) {
		float strength = std::abs(McpArgs::Float(args, "strength", 0.0f));
		const bool erases = brushName == "unmask" || brushName == "unweight" || brushName == "uncolor" || brushName == "unalpha";
		brush->setStrength(erases ? -strength : strength);
	}
	if (McpArgs::Has(args, "focus"))
		brush->setFocus(McpArgs::Float(args, "focus", 1.0f));
	if (McpArgs::Has(args, "spacing"))
		brush->setSpacing(McpArgs::Float(args, "spacing", 0.015f));
	brush->setMirror(McpArgs::Bool(args, "mirror", false));
	brush->setConnected(McpArgs::Bool(args, "connectedOnly", false));

	if (auto colorBrush = dynamic_cast<TB_Color*>(brush.get())) {
		if (const json* color = McpArgs::Find(args, "color")) {
			if (!color->is_array() || color->size() != 3)
				McpArgs::Wrong("color", "[r, g, b] from 0 to 1");
			colorBrush->color = Vector3((*color)[0].get<float>(), (*color)[1].get<float>(), (*color)[2].get<float>());
		}
	}

	BrushStrokeController controller(project, frame->editor);

	// Weight brushes paint the active bone, which is set for the stroke
	std::string previousBone = frame->editor.activeBone;
	struct BoneRestore {
		EditorState& editor;
		std::string bone;
		~BoneRestore() { editor.activeBone = bone; }
	} boneRestore{frame->editor, previousBone};

	if (isWeight) {
		std::string bone = McpArgs::String(args, "bone");
		if (!AnimSkeleton::getInstance().GetBonePtr(bone))
			throw McpToolError("There is no bone '" + bone + "' in the skeleton.");

		frame->editor.activeBone = bone;
		controller.PrepareWeightStroke(shapes);

		WeightBrushBones bones = mcp.Ops().GetWeightBrushBones(McpArgs::String(args, "mirrorBone", ""));
		const bool normalize = McpArgs::Bool(args, "normalize", true);
		AnimInfo* animInfo = project.GetWorkAnim();
		SetupWeightBrush<TB_Weight>(brush.get(), bones, animInfo, normalize);
		SetupWeightBrush<TB_Unweight>(brush.get(), bones, animInfo, normalize);
		SetupWeightBrush<TB_SmoothWeight>(brush.get(), bones, animInfo, normalize);
	}

	controller.Begin(meshes, brush.get(), picks.front());
	for (int pass = 0; pass < passes; pass++)
		for (size_t i = pass == 0 ? 1 : 0; i < picks.size(); i++)
			frame->ApplyRefresh(controller.Update(picks[i]));
	frame->ApplyRefresh(controller.End());

	frame->UpdateUndoTools();
	if (isWeight)
		frame->RefreshGUIWeightColors();
	frame->glView->Render();

	McpToolResult result;
	result.data["dabs"] = picks.size() * passes;
	return result;
}

McpToolResult SmoothShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	mcp.CheckPositionEdit();

	json names = json::array();
	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	for (auto* shape : shapes) {
		ShapeMesh(mcp, shape);
		names.push_back(shape->name.get());
	}

	json followNames = json::array();
	std::vector<NiShape*> followers;
	for (auto& name : McpArgs::Strings(args, "followShapes")) {
		NiShape* shape = mcp.Shape(name);
		ShapeMesh(mcp, shape);
		followers.push_back(shape);
		followNames.push_back(name);
	}

	auto liveVerts = [&](const std::vector<NiShape*>& list) {
		std::vector<std::vector<Vector3>> verts(list.size());
		for (size_t i = 0; i < list.size(); i++)
			project.GetLiveVerts(list[i], verts[i]);
		return verts;
	};
	auto before = liveVerts(shapes);
	auto followBefore = liveVerts(followers);

	McpRunStep(mcp,
			   {{"type", "SmoothVertices"},
				{"TargetMeshes", names},
				{"Iterations", McpArgs::Int(args, "iterations", 10)},
				{"Strength", McpArgs::Float(args, "strength", 0.1f)},
				{"FollowShapes", followNames},
				{"FollowRadius", McpArgs::Float(args, "followRadius", 1.5f)},
				{"KeepBorders", McpArgs::Bool(args, "keepBorders", true)}});
	frame->UpdateUndoTools();

	// How far the vertices moved, per shape
	auto summary = [&](const std::vector<NiShape*>& list, const std::vector<std::vector<Vector3>>& old) {
		json out = json::object();
		auto now = liveVerts(list);
		for (size_t i = 0; i < list.size(); i++) {
			size_t moved = 0;
			float largest = 0.0f;
			double sum = 0.0;
			for (size_t v = 0; v < now[i].size() && v < old[i].size(); v++) {
				float d = now[i][v].DistanceTo(old[i][v]);
				if (d < 1.0e-4f)
					continue;
				moved++;
				sum += d;
				largest = std::max(largest, d);
			}
			out[list[i]->name.get()] = {{"moved", moved}, {"maxMove", largest}, {"meanMove", moved ? sum / moved : 0.0}};
		}
		return out;
	};

	McpToolResult result;
	result.data["shapes"] = summary(shapes, before);
	if (!followers.empty())
		result.data["followShapes"] = summary(followers, followBefore);
	return result;
}

McpToolResult GetWeights(OutfitStudioMcp& mcp, const json& args) {
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	std::string bone = McpArgs::String(args, "bone");
	AnimInfo* anim = mcp.Project().GetWorkAnim();

	std::unordered_map<uint16_t, float> weights;
	if (auto ptr = anim->GetWeightsPtr(shape->name.get(), bone))
		weights = *ptr;

	size_t weighted = 0;
	double sum = 0.0;
	float largest = 0.0f;
	for (auto& w : weights) {
		if (w.second <= 0.0f)
			continue;
		weighted++;
		sum += w.second;
		largest = std::max(largest, w.second);
	}

	McpToolResult result;
	result.data["vertices"] = shape->GetNumVertices();
	result.data["weighted"] = weighted;
	result.data["meanWeight"] = weighted ? sum / weighted : 0.0;
	result.data["maxWeight"] = largest;

	std::vector<int> vertices = McpArgs::Ints(args, "vertices");
	if (McpArgs::Bool(args, "all", false))
		for (auto& w : weights)
			if (w.second > 0.0f)
				vertices.push_back(w.first);

	if (!vertices.empty()) {
		if (vertices.size() > MaxVerticesPerCall)
			throw McpToolError("That's " + std::to_string(vertices.size()) + " vertices, ask for at most " + std::to_string(MaxVerticesPerCall)
							   + " per call with 'vertices'.");

		json list = json::object();
		for (int v : vertices) {
			if (v < 0 || static_cast<size_t>(v) >= shape->GetNumVertices())
				throw McpToolError("Vertex " + std::to_string(v) + " doesn't exist.");
			auto it = weights.find(static_cast<uint16_t>(v));
			list[std::to_string(v)] = it != weights.end() ? it->second : 0.0f;
		}
		result.data["weights"] = list;
	}

	return result;
}

McpToolResult SetWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();

	auto shapes = mcp.Shapes(args, "shapes");
	if (shapes.size() != 1)
		throw McpToolError("Give exactly one shape in 'shapes'.");
	NiShape* shape = shapes.front();
	const std::string shapeName = shape->name.get();

	std::string bone = McpArgs::String(args, "bone");
	if (!AnimSkeleton::getInstance().GetBonePtr(bone))
		throw McpToolError("There is no bone '" + bone + "' in the skeleton.");

	const json* list = McpArgs::Find(args, "weights");
	if (!list || !list->is_object())
		McpArgs::Wrong("weights", "an object of vertex indices to weights from 0 to 1");

	std::unordered_map<uint16_t, float> newWeights;
	for (auto& item : list->items()) {
		int vertex = -1;
		try {
			vertex = std::stoi(item.key());
		}
		catch (const std::exception&) {
			throw McpToolError("'" + item.key() + "' isn't a vertex index.");
		}
		if (vertex < 0 || static_cast<size_t>(vertex) >= shape->GetNumVertices())
			throw McpToolError("Vertex " + item.key() + " doesn't exist.");
		if (!item.value().is_number())
			McpArgs::Wrong("weights", "an object of vertex indices to weights from 0 to 1");
		newWeights[static_cast<uint16_t>(vertex)] = std::clamp(item.value().get<float>(), 0.0f, 1.0f);
	}

	// Like a weight brush, a bone the shape doesn't have yet is added to it
	project.CreateSkinning(shape);
	if (anim->GetShapeBoneIndex(shapeName, bone) < 0)
		project.AddBoneRef(bone);

	const bool normalize = McpArgs::Bool(args, "normalize", true);
	auto weightOf = [anim, &shapeName](const std::string& boneName, uint16_t v) {
		auto weights = anim->GetWeightsPtr(shapeName, boneName);
		if (!weights)
			return 0.0f;
		auto it = weights->find(v);
		return it != weights->end() ? it->second : 0.0f;
	};

	UndoHistory& history = frame->editor.undoHistory;
	UndoStateProject* usp = history.PushState();
	usp->undoType = UndoType::Weight;
	usp->usss.emplace_back();
	UndoStateShape& uss = usp->usss.back();
	uss.shapeName = shapeName;

	std::map<std::string, UndoStateBoneWeights> byBone;
	auto record = [&](const std::string& boneName, uint16_t v, float endValue) {
		UndoStateBoneWeights& bw = byBone[boneName];
		bw.boneName = boneName;
		bw.weights[v] = {weightOf(boneName, v), endValue};
	};

	size_t unnormalized = 0;
	for (auto& w : newWeights) {
		record(bone, w.first, w.second);
		if (!normalize)
			continue;

		// The other bones share what's left, in their current proportions
		float others = 0.0f;
		for (auto& other : anim->shapeBones[shapeName])
			if (other != bone)
				others += weightOf(other, w.first);

		if (others <= 0.0f) {
			if (w.second < 1.0f)
				unnormalized++;
			continue;
		}

		const float factor = (1.0f - w.second) / others;
		for (auto& other : anim->shapeBones[shapeName]) {
			if (other == bone)
				continue;
			float current = weightOf(other, w.first);
			if (current > 0.0f)
				record(other, w.first, current * factor);
		}
	}

	for (auto& bw : byBone)
		uss.boneWeights.push_back(bw.second);

	frame->ApplyRefresh(mcp.Ops().ApplyShapeEdits(*usp, false));
	frame->UpdateUndoTools();
	frame->RefreshGUIWeightColors();
	frame->SetPendingChanges();

	McpToolResult result;
	result.data["changed"] = newWeights.size();
	if (unnormalized)
		result.data["unnormalized"] = unnormalized;
	return result;
}

const json ShapesProp = McpStringList("Outfit shapes, the selected ones by default");

// A vector argument given as [x, y, z], or as one number for all three axes when "uniform" is allowed
Vector3 VectorArg(const json& args, const char* name, const Vector3& def, bool uniform) {
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return def;
	if (uniform && value->is_number()) {
		float v = value->get<float>();
		return Vector3(v, v, v);
	}
	if (!value->is_array() || value->size() != 3 || !(*value)[0].is_number() || !(*value)[1].is_number() || !(*value)[2].is_number())
		McpArgs::Wrong(name, uniform ? "a number or an array of three numbers" : "an array of three numbers");
	return Vector3((*value)[0].get<float>(), (*value)[1].get<float>(), (*value)[2].get<float>());
}

// Moves, rotates, scales and inflates shapes the way the Move, Rotate, Scale and Inflate dialogs do:
// masked vertices stay, the change is one undo state, and in slider edit mode it goes into the slider
McpToolResult TransformShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	if (shapes.empty())
		throw McpToolError("No shapes are given or selected.");
	mcp.CheckPositionEdit();

	const Vector3 move = VectorArg(args, "move", Vector3(), false);
	const Vector3 rotate = VectorArg(args, "rotate", Vector3(), false);
	const Vector3 scale = VectorArg(args, "scale", Vector3(1.0f, 1.0f, 1.0f), true);
	const Vector3 inflate = VectorArg(args, "inflate", Vector3(), true);
	if (scale.x < 0.01f || scale.y < 0.01f || scale.z < 0.01f)
		throw McpToolError("scale has to be at least 0.01 on every axis.");

	bool mirror[3] = {false, false, false};
	for (auto& axis : McpArgs::Has(args, "mirrorMove") ? McpArgs::Strings(args, "mirrorMove") : std::vector<std::string>()) {
		if (axis == "x")
			mirror[0] = true;
		else if (axis == "y")
			mirror[1] = true;
		else if (axis == "z")
			mirror[2] = true;
		else
			throw McpToolError("mirrorMove takes the axes \"x\", \"y\" and \"z\".");
	}

	const bool useMask = McpArgs::Bool(args, "useMask", true);

	struct ShapeData {
		NiShape* shape;
		Mesh* mesh;
		std::vector<Vector3> verts;
	};
	std::vector<ShapeData> data;
	for (NiShape* shape : shapes) {
		ShapeData d{shape, ShapeMesh(mcp, shape), {}};
		project.GetLiveVerts(shape, d.verts);
		if (d.verts.size() != static_cast<size_t>(d.mesh->nVerts))
			throw McpToolError("The shape '" + shape->name.get() + "' doesn't match its mesh in the view.");
		data.push_back(std::move(d));
	}

	auto weightOf = [useMask](const Mesh* m, size_t i) { return useMask && m->mask ? 1.0f - m->mask[i] : 1.0f; };

	// Pivot of rotation and scale: the origin, the middle of the unmasked vertices, or a point
	Vector3 origin;
	const json* originArg = McpArgs::Find(args, "origin");
	if (originArg && originArg->is_string()) {
		std::string mode = originArg->get<std::string>();
		if (mode == "center") {
			double sum = 0.0;
			Vector3 total;
			for (auto& d : data)
				for (size_t i = 0; i < d.verts.size(); i++) {
					float w = weightOf(d.mesh, i);
					if (w > 0.0f) {
						total += d.verts[i] * w;
						sum += w;
					}
				}
			if (sum > 0.0)
				origin = total / static_cast<float>(sum);
		}
		else if (mode != "zero") {
			throw McpToolError("origin has to be \"zero\", \"center\" or an array of three numbers.");
		}
	}
	else if (originArg) {
		origin = VectorArg(args, "origin", Vector3(), false);
	}

	Matrix4 xform;
	xform.PushTranslate(origin);
	xform.PushRotate(rotate.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
	xform.PushRotate(rotate.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	xform.PushRotate(rotate.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
	xform.PushScale(scale.x, scale.y, scale.z);
	xform.PushTranslate(origin * -1.0f);

	UndoHistory* history = frame->glView->GetUndoHistory();
	UndoStateProject* usp = history->PushState();
	usp->undoType = UndoType::VertexPosition;

	size_t changed = 0;
	for (auto& d : data) {
		Mesh* m = d.mesh;
		if (!inflate.IsZero() && !m->bGotWeldVerts)
			m->CalcWeldVerts();

		UndoStateShape uss;
		uss.shapeName = d.shape->name.get();

		for (size_t i = 0; i < d.verts.size(); i++) {
			const Vector3& pos = d.verts[i];
			Vector3 moveDiff = move;
			if (mirror[0] && pos.x < 0.0f)
				moveDiff.x = -moveDiff.x;
			if (mirror[1] && pos.y < 0.0f)
				moveDiff.y = -moveDiff.y;
			if (mirror[2] && pos.z < 0.0f)
				moveDiff.z = -moveDiff.z;

			Vector3 diff = xform * pos - pos + moveDiff;
			if (!inflate.IsZero() && m->norms)
				diff += Mesh::TransformDirMeshToNif(m->norms[i]).ComponentMultiply(inflate);

			diff *= weightOf(m, i);
			if (diff.IsZero(true))
				continue;

			uss.pointStartState[i] = Mesh::TransformPosNifToMesh(pos);
			uss.pointEndState[i] = Mesh::TransformPosNifToMesh(pos + diff);
		}

		// Welded vertices move together like in the Inflate dialog, so seams stay closed
		if (!inflate.IsZero()) {
			for (size_t i = 0; i < d.verts.size(); i++) {
				auto it = uss.pointEndState.find(i);
				if (it == uss.pointEndState.end())
					continue;
				m->DoForEachWeldedVertex(i, [&](int p) {
					auto weld = uss.pointEndState.find(p);
					if (weld != uss.pointEndState.end())
						weld->second = it->second;
				});
			}
		}

		changed += uss.pointEndState.size();
		usp->usss.push_back(std::move(uss));
	}

	if (changed == 0) {
		history->PopState();
		throw McpToolError("Nothing changed. Either no transform was given, or every vertex is masked.");
	}

	if (frame->editor.bEditSlider) {
		usp->sliderName = frame->editor.activeSlider;
		float sliderscale = project.SliderValue(frame->editor.activeSlider);
		usp->sliderscale = sliderscale == 0.0f ? 1.0f : sliderscale;
	}

	frame->glView->ApplyUndoState(usp, false);
	frame->UpdateUndoTools();

	McpToolResult result;
	result.data["changedVertices"] = changed;
	result.data["origin"] = {origin.x, origin.y, origin.z};
	result.data["slider"] = usp->sliderName.empty() ? json(nullptr) : json(usp->sliderName);
	return result;
}

} // namespace

void AddEditTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool getMask;
	getMask.name = "get_mask";
	getMask.description =
		"Returns the masked vertices of a shape with their mask values (1 is fully masked). Masked vertices are "
		"protected from conforming, weight copies, clipping fixes, smoothing and brushes.";
	getMask.inputSchema = McpSchema({{"shape", McpProp("string", "Shape name")}}, {"shape"});
	getMask.readOnly = true;
	getMask.handler = [&mcp](const json& args) {
		Mesh* m = ShapeMesh(mcp, mcp.Shape(McpArgs::String(args, "shape")));
		auto masked = MaskOps::GetMasked(*m);
		if (masked.size() > MaxVerticesPerCall)
			throw McpToolError("More than " + std::to_string(MaxVerticesPerCall) + " vertices are masked.");

		json values = json::object();
		for (auto& v : masked)
			values[std::to_string(v.first)] = v.second;

		McpToolResult result;
		result.data = MaskSummary(*m);
		result.data["mask"] = values;
		return result;
	};
	mcp.AddTool(std::move(getMask));

	McpTool setMask;
	setMask.name = "set_mask";
	setMask.description =
		"Masks vertices of a shape. By default the given vertices replace the mask, with replace: false they are "
		"added. More ways to mask (by bone weights, by slider, asymmetric vertices, from a file) are automation steps "
		"for run_steps: MaskBoneWeighted, MaskWeighted, MaskSliderAffected, MaskAsymmetric, LoadMask. Can be undone "
		"when mask history is turned on in the settings.";
	setMask.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"vertices", {{"description", "Vertex indices to mask fully, or an object of vertex indices to mask values from 0 to 1"}}},
			{"replace", McpProp("boolean", "Clear the rest of the mask. Default true.")},
		},
		{"shape", "vertices"});
	setMask.handler = [&mcp, frame](const json& args) {
		Mesh* m = ShapeMesh(mcp, mcp.Shape(McpArgs::String(args, "shape")));
		auto values = VertexValues(args, "vertices", m, 1.0f);
		const bool replace = McpArgs::Bool(args, "replace", true);

		frame->glView->ApplyMaskChanges({m}, [&](Mesh& mesh) {
			std::unordered_map<int, float> changes;
			if (replace)
				changes = MaskOps::ClearChanges(mesh);
			for (auto& v : values)
				changes[v.first] = std::clamp(v.second, 0.0f, 1.0f);
			return changes;
		});
		frame->glView->Render();

		McpToolResult result;
		result.data = MaskSummary(*m);
		return result;
	};
	mcp.AddTool(std::move(setMask));

	McpTool maskOps;
	maskOps.name = "mask_ops";
	maskOps.description =
		"Clears, inverts, grows, shrinks or smooths the mask of shapes. Smoothing turns a hard mask border into a gradient, "
		"so fixes on the unmasked area blend into their surroundings. These can be undone when mask history is turned on in "
		"the settings. weighted, bone_weighted and slider_affected are the masking commands of the window: they replace the "
		"mask with the vertices that have any bone weight, are weighted to the given bones or are moved by the slider. "
		"asymmetric adds everything but the parts that differ from their mirror image across X to the mask, so edits go to "
		"the asymmetric parts; clear the mask first to only keep those. connected grows the mask over the whole connected "
		"pieces it touches, parts masks the pieces of list_mesh_parts by number. from_analysis masks by a measurement "
		"(metric): with target \"fix\" (the default) only the bad vertices, grown by 'grow' rings, stay unmasked, so a "
		"clipping fix, smoothing, brush or rigidify changes nothing else; with \"protect\" they are masked instead. When "
		"nothing is found, the mask stays as it is.";
	json maskProperties = {
		{"shapes", McpStringList("Shapes, the selected ones by default. The reference can be included, except for from_analysis.")},
		{"op",
		 {{"type", "string"},
		  {"enum", {"clear", "invert", "grow", "shrink", "smooth", "weighted", "bone_weighted", "slider_affected", "asymmetric", "connected", "parts", "box", "from_analysis"}}}},
		{"box",
		 {{"type", "object"},
		  {"properties", {{"min", {{"type", "array"}, {"items", {{"type", "number"}}}}}, {"max", {{"type", "array"}, {"items", {{"type", "number"}}}}}}},
		  {"description", "box: masks the vertices inside, as the shapes are shown now (list_shapes bounds, NIF units). Leave out min or max for no limit that way."}}},
		{"keepOthers", McpProp("boolean", "box: leave the mask of the vertices outside as it is instead of unmasking them. Default false.")},
		{"steps", McpProp("integer", "Grow or shrink: how many rings of vertices. Smooth: how many passes, each widening the gradient by about a ring. Default 1.")},
		{"strength", McpProp("number", "Smooth: how far each pass moves values towards their neighbors, 0 to 1. Default 0.5.")},
		{"bones", McpStringList("bone_weighted: the bones")},
		{"slider", McpProp("string", "slider_affected and the slider metrics of from_analysis: the slider")},
		{"asymmetry",
		 {{"type", "object"},
		  {"description", "asymmetric: what counts as asymmetric. Defaults: triangles false, positions true, sliders false, bones false, unmatched false."},
		  {"properties",
		   {{"triangles", McpProp("boolean", "Find asymmetric triangles instead of vertices")},
			{"positions", McpProp("boolean", "Vertices whose position differs from their partner's")},
			{"sliders", McpProp("boolean", "Vertices whose slider data differs")},
			{"bones", McpProp("boolean", "Vertices whose bone weights differ")},
			{"unmatched", McpProp("boolean", "Vertices without a partner")}}}}},
		{"parts", McpIntList("parts: numbers of the parts of list_mesh_parts to mask, the rest is unmasked")},
		{"threshold", McpProp("number", "from_analysis: value above which a vertex counts as bad. Defaults: clipping 0, roughness 0.05, flipped 0.5, weight_deviation 0.25, gap_change 0.3 (either way), seam_gap 0.01, pose_stretch 0.5, weight_jump 0.5.")},
		{"target", {{"type", "string"}, {"enum", {"fix", "protect"}}, {"description", "from_analysis: leave only the bad vertices unmasked (fix, default) or mask them (protect)"}}},
		{"grow", McpProp("integer", "from_analysis: rings of neighbors added to the bad vertices. Default 1.")},
		{"feather", McpProp("integer", "from_analysis: smoothing passes on the mask border, so fixes blend in. Default 0.")},
	};
	const json metricProperties = McpMetricProperties();
	for (auto& property : metricProperties.items())
		if (!maskProperties.contains(property.key()))
			maskProperties[property.key()] = property.value();
	maskOps.inputSchema = McpSchema(maskProperties, {"op"});
	maskOps.handler = [&mcp, frame](const json& args) {
		std::string op = McpArgs::String(args, "op");
		if (op == "from_analysis")
			return McpMaskFromMetric(mcp, args);

		std::vector<Mesh*> meshes;
		json names = json::array();
		for (auto* shape : mcp.Shapes(args, "shapes", true)) {
			meshes.push_back(ShapeMesh(mcp, shape));
			names.push_back(shape->name.get());
		}

		json step = {{"TargetMeshes", names}};
		if (op == "weighted")
			step["type"] = "MaskWeighted";
		else if (op == "bone_weighted") {
			std::vector<std::string> bones = McpArgs::Strings(args, "bones");
			if (bones.empty())
				McpArgs::Wrong("bones", "a list of bone names");
			step["type"] = "MaskBoneWeighted";
			step["BoneNames"] = bones;
		}
		else if (op == "slider_affected") {
			std::string slider = McpArgs::String(args, "slider");
			if (!mcp.Project().ValidSlider(slider))
				throw McpToolError("There is no slider '" + slider + "'.");
			step["type"] = "MaskSliderAffected";
			step["SliderName"] = slider;
		}
		else if (op == "asymmetric") {
			json asym = McpArgs::Has(args, "asymmetry") ? args.at("asymmetry") : json::object();
			if (!asym.is_object())
				McpArgs::Wrong("asymmetry", "an object");
			step["type"] = "MaskAsymmetric";
			step["Mode"] = McpArgs::Bool(asym, "triangles", false) ? 0 : 1;
			step["Positions"] = McpArgs::Bool(asym, "positions", true);
			step["Sliders"] = McpArgs::Bool(asym, "sliders", false);
			step["Bones"] = McpArgs::Bool(asym, "bones", false);
			step["Unmatched"] = McpArgs::Bool(asym, "unmatched", false);
		}

		if (step.contains("type")) {
			McpRunStep(mcp, step);
			frame->glView->Render();
			return MaskSummaries(mcp, names);
		}

		int steps = 1;
		std::function<std::unordered_map<int, float>(Mesh&)> changes;
		if (op == "clear")
			changes = [](Mesh& m) { return MaskOps::ClearChanges(m); };
		else if (op == "invert")
			changes = [](Mesh& m) { return MaskOps::InvertChanges(m); };
		else if (op == "grow" || op == "shrink") {
			const bool grow = op == "grow";
			steps = std::clamp(McpArgs::Int(args, "steps", 1), 1, 100);
			changes = [grow](Mesh& m) { return MaskOps::GrowShrinkChanges(m, grow); };
		}
		else if (op == "smooth") {
			steps = std::clamp(McpArgs::Int(args, "steps", 1), 1, 100);
			const float strength = std::clamp(McpArgs::Float(args, "strength", 0.5f), 0.0f, 1.0f);
			changes = [strength](Mesh& m) { return MaskOps::SmoothChanges(m, strength); };
		}
		else if (op == "connected" || op == "parts") {
			std::vector<int> wanted = McpArgs::Ints(args, "parts");
			if (op == "parts" && wanted.empty())
				McpArgs::Wrong("parts", "a list of part numbers from list_mesh_parts");

			OutfitProject& project = mcp.Project();
			changes = [&project, op, wanted](Mesh& m) {
				NiShape* shape = project.GetWorkNif()->FindBlockByName<NiShape>(m.shapeName);
				std::unordered_map<int, float> result;
				if (!shape)
					return result;

				std::vector<Vector3> verts;
				project.GetWorkNif()->GetVertsForShape(shape, verts);
				std::vector<Triangle> tris;
				shape->GetTriangles(tris);
				auto parts = MeshAnalysis::ConnectedParts(verts, tris, 0.001f);

				std::set<int> chosen(wanted.begin(), wanted.end());
				if (op == "connected")
					for (int v = 0; v < m.nVerts && v < static_cast<int>(parts.size()); v++)
						if (m.mask && m.mask[v] > 0.0f)
							chosen.insert(parts[v]);

				for (int v = 0; v < m.nVerts && v < static_cast<int>(parts.size()); v++) {
					if (chosen.count(parts[v]))
						result[v] = 1.0f;
					else if (op == "parts")
						result[v] = 0.0f;
				}
				return result;
			};
		}
		else if (op == "box") {
			// Vertices inside a box, as the shapes are shown now: masked, the others unmasked unless keepOthers
			const json* box = McpArgs::Find(args, "box");
			if (!box || !box->is_object())
				McpArgs::Wrong("box", "an object with min and max as [x, y, z]");
			const Vector3 lo = VectorArg(*box, "min", Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX), false);
			const Vector3 hi = VectorArg(*box, "max", Vector3(FLT_MAX, FLT_MAX, FLT_MAX), false);
			const bool keepOthers = McpArgs::Bool(args, "keepOthers", false);

			OutfitProject& project = mcp.Project();
			changes = [&project, lo, hi, keepOthers](Mesh& m) {
				std::unordered_map<int, float> result;
				NiShape* shape = project.GetWorkNif()->FindBlockByName<NiShape>(m.shapeName);
				if (!shape)
					return result;

				std::vector<Vector3> verts;
				project.GetLiveVerts(shape, verts);
				for (int v = 0; v < m.nVerts && v < static_cast<int>(verts.size()); v++) {
					const Vector3& p = verts[v];
					const bool inside = p.x >= lo.x && p.x <= hi.x && p.y >= lo.y && p.y <= hi.y && p.z >= lo.z && p.z <= hi.z;
					if (inside)
						result[v] = 1.0f;
					else if (!keepOthers)
						result[v] = 0.0f;
				}
				return result;
			};
		}
		else
			McpArgs::Wrong("op", "one of the listed mask operations");

		for (int i = 0; i < steps; i++)
			frame->glView->ApplyMaskChanges(meshes, changes);
		frame->glView->Render();
		return MaskSummaries(mcp, names);
	};
	mcp.AddTool(std::move(maskOps));

	McpTool maskFile;
	maskFile.name = "mask_file";
	maskFile.description =
		"Saves the masks of shapes under a name in a mask file, or loads a named mask from one onto the shapes, "
		"like the Save Mask and Load Mask automation steps.";
	maskFile.inputSchema = McpSchema(
		{
			{"op", {{"type", "string"}, {"enum", {"save", "load"}}}},
			{"file", McpProp("string", "Mask file (absolute or relative to the BodySlide folder)")},
			{"name", McpProp("string", "Name of the mask in the file")},
			{"shapes", McpStringList("Shapes, the selected ones by default. The reference can be included.")},
			{"merge", McpProp("boolean", "save: keep the other masks of an existing file. Default true.")},
		},
		{"op", "file", "name"});
	maskFile.handler = [&mcp, frame](const json& args) {
		std::string op = McpArgs::String(args, "op");
		json names = json::array();
		for (auto* shape : mcp.Shapes(args, "shapes", true)) {
			ShapeMesh(mcp, shape);
			names.push_back(shape->name.get());
		}

		std::string file = McpFilePath(McpArgs::String(args, "file"));
		std::string name = McpArgs::String(args, "name");
		if (op == "save")
			McpRunStep(mcp, {{"type", "SaveMask"}, {"TargetMeshes", names}, {"MaskFile", file}, {"MaskName", name}, {"Merge", McpArgs::Bool(args, "merge", true)}});
		else if (op == "load")
			McpRunStep(mcp, {{"type", "LoadMask"}, {"TargetMeshes", names}, {"MaskFile", file}, {"MaskName", name}});
		else
			McpArgs::Wrong("op", "save or load");
		frame->glView->Render();

		McpToolResult result = MaskSummaries(mcp, names);
		result.data["file"] = file;
		return result;
	};
	mcp.AddTool(std::move(maskFile));

	McpTool stroke;
	stroke.name = "brush_stroke";
	stroke.description =
		"Paints one brush stroke along a path of vertices, like dragging a brush in the window, and records it for "
		"undo. Geometry brushes (inflate, deflate, smooth, undiff) change the base shape, or the morph of the slider "
		"being edited (set_slider_edit). Mask brushes change the mask, weight brushes the weights of 'bone', color and "
		"alpha brushes the vertex colors. Masked vertices are protected from everything but mask brushes.";
	stroke.inputSchema = McpSchema(
		{
			{"brush", {{"type", "string"}, {"enum", BrushNames}}},
			{"shapes", McpStringList("Shapes the brush works on, the selected outfit shapes by default")},
			{"shape", McpProp("string", "Shape of the path vertices, the first of shapes by default")},
			{"path", McpIntList("Vertices the brush passes over, in order. One vertex is a single dab.")},
			{"points",
			 {{"type", "array"},
			  {"items", {{"type", "array"}, {"items", {{"type", "number"}}}}},
			  {"description",
			   "Instead of path: positions [x, y, z] (NIF units, as the shapes are shown now) the brush passes over, each at the nearest "
			   "vertex of the shapes. The same points then work on every outfit fitted to one body."}}},
			{"step", McpProp("number", "Adds dabs between the path points this far apart (NIF units), like dragging the brush. Default: one dab per point.")},
			{"passes", McpProp("integer", "Goes over the path this many times in the one stroke. Default 1.")},
			{"radius", McpProp("number", "Brush radius in NIF units. Default 4.5.")},
			{"strength", McpProp("number", "Brush strength as the brush settings in the window show it")},
			{"focus", McpProp("number", "How much stronger the center is than the edge, as in the window")},
			{"spacing", McpProp("number", "Distance between dabs along the path, as in the window")},
			{"mirror", McpProp("boolean", "Also paint mirrored on X. Default false.")},
			{"connectedOnly", McpProp("boolean", "Only affect vertices connected to the path. Default false.")},
			{"bone", McpProp("string", "Weight brushes: the bone to paint")},
			{"mirrorBone", McpProp("string", "Weight brushes with mirror: the bone of the other side")},
			{"normalize", McpProp("boolean", "Weight brushes: keep the weights of each vertex summing to 1. Default true.")},
			{"color", {{"type", "array"}, {"items", {{"type", "number"}}}, {"description", "Color brush: [r, g, b] from 0 to 1"}}},
		},
		{"brush"});
	stroke.handler = [&mcp](const json& args) { return BrushStroke(mcp, args); };
	mcp.AddTool(std::move(stroke));

	McpTool smooth;
	smooth.name = "smooth_shapes";
	smooth.description =
		"Smooths all unmasked vertices of shapes like the smooth brush, without a path: the base shape, or the morph of "
		"the slider being edited (set_slider_edit). The mask scales the change, so mask what to keep, then mask_ops "
		"smooth for a soft border. followShapes (trim, zippers, straps lying on the shapes) move as far as the surface "
		"under them moved across it, so they stay on it. keepBorders holds open edges in place. One undo step, and one "
		"more for the follow shapes. Returns how far the vertices moved. The SmoothVertices step does the same in automation.";
	smooth.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to smooth, the selected outfit shapes by default")},
		{"iterations", McpProp("integer", "How many times it smooths. Default 10.")},
		{"strength", McpProp("number", "Strength of each time, like the smooth brush strength. Default 0.1.")},
		{"followShapes", McpStringList("Shapes that move with the smoothed ones")},
		{"followRadius", McpProp("number", "How far (NIF units) a follow shape vertex may be from the smoothed shapes to move with them. Default 1.5.")},
		{"keepBorders", McpProp("boolean", "Keep open mesh edges in place: smoothing pulls them in, opening gaps to zippers and trim attached there. Default true.")},
	});
	smooth.handler = [&mcp](const json& args) { return SmoothShapes(mcp, args); };
	mcp.AddTool(std::move(smooth));

	McpTool listBones;
	listBones.name = "list_bones";
	listBones.description = "Lists the bones of a shape with the number of vertices weighted to each and their largest weight.";
	listBones.inputSchema = McpSchema({{"shape", McpProp("string", "Shape name")}}, {"shape"});
	listBones.readOnly = true;
	listBones.handler = [&mcp](const json& args) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		AnimInfo* anim = mcp.Project().GetWorkAnim();

		json bones = json::array();
		auto it = anim->shapeBones.find(shape->name.get());
		if (it != anim->shapeBones.end()) {
			for (auto& bone : it->second) {
				size_t weighted = 0;
				float largest = 0.0f;
				if (auto weights = anim->GetWeightsPtr(shape->name.get(), bone)) {
					for (auto& w : *weights) {
						if (w.second > 0.0f)
							weighted++;
						largest = std::max(largest, w.second);
					}
				}
				bones.push_back({{"name", bone}, {"weighted", weighted}, {"maxWeight", largest}});
			}
		}

		McpToolResult result;
		result.data["bones"] = bones;
		return result;
	};
	mcp.AddTool(std::move(listBones));

	McpTool getWeights;
	getWeights.name = "get_weights";
	getWeights.description = "Statistics of the weights of a bone on a shape, and the weights of chosen or all weighted vertices.";
	getWeights.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"bone", McpProp("string", "Bone name")},
			{"vertices", McpIntList("Vertex indices to return the weights of")},
			{"all", McpProp("boolean", "Return the weights of all weighted vertices. Default false.")},
		},
		{"shape", "bone"});
	getWeights.readOnly = true;
	getWeights.handler = [&mcp](const json& args) { return GetWeights(mcp, args); };
	mcp.AddTool(std::move(getWeights));

	McpTool setWeights;
	setWeights.name = "set_weights";
	setWeights.description =
		"Sets the weights of a bone for vertices of one shape. With normalize (the default), the vertices' other "
		"bones are scaled so each vertex sums to 1. Can be undone.";
	setWeights.inputSchema = McpSchema(
		{
			{"shapes", McpStringList("The one shape to change, the selected one by default")},
			{"bone", McpProp("string", "Bone name")},
			{"weights", {{"type", "object"}, {"additionalProperties", {{"type", "number"}}}, {"description", "Vertex index to weight from 0 to 1"}}},
			{"normalize", McpProp("boolean", "Scale the other bones of the vertices to make up the rest. Default true.")},
		},
		{"bone", "weights"});
	setWeights.handler = [&mcp](const json& args) { return SetWeights(mcp, args); };
	mcp.AddTool(std::move(setWeights));

	McpTool transform;
	transform.name = "transform_shapes";
	transform.description =
		"Moves, rotates, scales and inflates shapes like the Move, Rotate, Scale and Inflate Shape dialogs, in NIF "
		"units and degrees. Vertices are changed directly, as one undo step: scale, then rotate (X, Y, Z) about "
		"'origin', then move, then inflate along the vertex normals. Masked vertices stay (partly masked ones move "
		"partly), so mask to transform only a part. The base shape changes only with all sliders at 0. In slider edit "
		"mode (set_slider_edit, slider not at 0) the change goes into that slider's morph instead. The shape's own NIF "
		"transform isn't touched: shape_transforms apply bakes that into the vertices.";
	transform.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to transform. Default: the selected outfit shapes.")},
		{"move", McpProp("array", "Offset [x, y, z]. +X is the character's right, +Y her front, +Z up.")},
		{"mirrorMove",
		 McpStringList("Axes (\"x\", \"y\", \"z\") on which vertices on the negative side move the opposite way, e.g. [\"x\"] to move both sides outwards")},
		{"rotate", McpProp("array", "Rotation [x, y, z] in degrees about the X, Y and Z axes through 'origin'")},
		{"scale", json{{"type", {"number", "array"}}, {"description", "Factor, one number or [x, y, z]. At least 0.01."}}},
		{"inflate", json{{"type", {"number", "array"}}, {"description", "Distance along the vertex normals, one number or [x, y, z] per axis. Negative deflates."}}},
		{"origin",
		 json{{"type", {"string", "array"}},
			  {"description", "Pivot of rotate and scale: \"zero\" (default, the NIF origin), \"center\" (middle of the unmasked vertices) or [x, y, z]"}}},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	transform.handler = [&mcp](const json& args) { return TransformShapes(mcp, args); };
	mcp.AddTool(std::move(transform));
}
