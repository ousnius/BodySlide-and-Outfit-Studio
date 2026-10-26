/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/ClippingFixer.h"
#include "../components/MaskOps.h"
#include "../components/StarfieldSpace.h"
#include "../program/OutfitStudio.h"
#include "../utils/ConfigurationManager.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <set>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace nifly;

namespace {

constexpr float SeamTolerance = 0.001f;

Mesh* ShapeMesh(OutfitStudioMcp& mcp, NiShape* shape) {
	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	if (!m)
		throw McpToolError("The shape '" + shape->name.get() + "' has no mesh in the view.");
	return m;
}

json VectorToJson(const Vector3& v) {
	return json::array({v.x, v.y, v.z});
}

std::vector<Vector3> RestVerts(OutfitProject& project, NiShape* shape) {
	std::vector<Vector3> verts;
	project.GetWorkNif()->GetVertsForShape(shape, verts);
	return verts;
}

std::vector<Triangle> Triangles(NiShape* shape) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	return tris;
}

void CheckMorphSlider(OutfitProject& project, const std::string& slider) {
	size_t index = 0;
	if (!project.SliderIndexFromName(slider, index))
		throw McpToolError("There is no slider '" + slider + "'.");
	if (project.SliderZap(index) || project.SliderUV(index))
		throw McpToolError("'" + slider + "' is a zap or UV slider, it has no morph.");
}

// Morph sliders with data on any of the shapes, or the sliders named in args[name]
std::vector<std::string> MorphSliders(OutfitProject& project, const json& args, const char* name, const std::vector<NiShape*>& shapes) {
	if (McpArgs::Has(args, name)) {
		std::vector<std::string> sliders = McpArgs::Strings(args, name);
		for (auto& slider : sliders)
			CheckMorphSlider(project, slider);
		return sliders;
	}

	std::vector<std::string> sliders;
	for (size_t i = 0; i < project.SliderCount(); i++) {
		if (project.SliderZap(i) || project.SliderUV(i))
			continue;
		for (auto* shape : shapes) {
			TargetDataDiffs* diff = project.GetDiffSet(project.activeSet[i], shape);
			if (diff && !diff->empty()) {
				sliders.push_back(project.GetSliderName(i));
				break;
			}
		}
	}
	return sliders;
}

// Neighbors of each vertex through triangles and through vertices at the same place
std::vector<std::vector<int>> VertexNeighbors(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris) {
	std::vector<std::set<int>> sets(verts.size());
	for (auto& t : tris) {
		const uint16_t p[3] = {t.p1, t.p2, t.p3};
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				if (i != j && p[i] < verts.size() && p[j] < verts.size())
					sets[p[i]].insert(p[j]);
	}

	for (auto& group : MeshAnalysis::CoincidentGroups(verts, SeamTolerance))
		for (int a : group)
			for (int b : group)
				if (a != b)
					sets[a].insert(b);

	std::vector<std::vector<int>> neighbors(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		neighbors[v].assign(sets[v].begin(), sets[v].end());
	return neighbors;
}

// The selection grown by rings of neighbors
void GrowSelection(std::vector<bool>& selected, const std::vector<std::vector<int>>& neighbors, int rings) {
	for (int r = 0; r < rings; r++) {
		std::vector<bool> grown = selected;
		for (size_t v = 0; v < selected.size(); v++)
			if (selected[v])
				for (int n : neighbors[v])
					grown[n] = true;
		selected.swap(grown);
	}
}

// Vertices at the same place on several shapes, or along split seams of one shape
struct SeamVertex {
	std::string shape;
	uint16_t vertex = 0;
	bool reference = false;
};

std::vector<std::vector<SeamVertex>> SeamGroups(OutfitProject& project, const std::vector<NiShape*>& shapes, float tolerance) {
	std::vector<Vector3> points;
	std::vector<SeamVertex> owners;
	for (auto* shape : shapes) {
		const bool reference = project.IsBaseShape(shape);
		auto verts = RestVerts(project, shape);
		for (size_t v = 0; v < verts.size(); v++) {
			points.push_back(verts[v]);
			owners.push_back({shape->name.get(), static_cast<uint16_t>(v), reference});
		}
	}

	std::vector<std::vector<SeamVertex>> groups;
	for (auto& group : MeshAnalysis::CoincidentGroups(points, tolerance)) {
		std::vector<SeamVertex> members;
		bool hasOutfit = false;
		for (int i : group) {
			members.push_back(owners[i]);
			hasOutfit = hasOutfit || !owners[i].reference;
		}
		// The reference's own seams are left alone
		if (hasOutfit)
			groups.push_back(std::move(members));
	}
	return groups;
}

// Slider deltas of each shape, by shape name
std::map<std::string, std::vector<Vector3>> ShapeDeltas(OutfitStudioMcp& mcp, const std::vector<NiShape*>& shapes, const std::string& slider) {
	std::map<std::string, std::vector<Vector3>> deltas;
	ProjectOperations ops = mcp.Ops();
	for (auto* shape : shapes)
		mcp.Apply(ops.GetSliderDeltas(shape, slider, deltas[shape->name.get()]));
	return deltas;
}

float WeightOf(AnimInfo* anim, const std::string& shape, const std::string& bone, uint16_t v) {
	auto weights = anim->GetWeightsPtr(shape, bone);
	if (!weights)
		return 0.0f;
	auto it = weights->find(v);
	return it != weights->end() ? it->second : 0.0f;
}

std::vector<std::string> ShapeBones(AnimInfo* anim, const std::string& shape) {
	auto it = anim->shapeBones.find(shape);
	return it != anim->shapeBones.end() ? it->second : std::vector<std::string>();
}

MeshAnalysis::BoneWeightMap WeightMap(AnimInfo* anim, const std::string& shape) {
	MeshAnalysis::BoneWeightMap map;
	for (auto& bone : ShapeBones(anim, shape))
		map[bone] = anim->GetWeightsPtr(shape, bone);
	return map;
}

// Records new weights of a shape as one undo step and applies them, like the weight brush. newWeights holds the
// new weight of vertices by bone, 0 removes it. Bones the shape doesn't have yet are added to it. Returns the
// number of changed vertex weights.
size_t ApplyWeights(OutfitStudioMcp& mcp, NiShape* shape, const std::map<std::string, std::unordered_map<uint16_t, float>>& newWeights) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	const std::string shapeName = shape->name.get();

	UndoStateShape uss;
	uss.shapeName = shapeName;
	size_t changed = 0;
	for (auto& bone : newWeights) {
		UndoStateBoneWeights bw;
		bw.boneName = bone.first;
		bw.addedBone = anim->GetShapeBoneIndex(shapeName, bone.first) < 0;
		for (auto& w : bone.second) {
			float current = WeightOf(anim, shapeName, bone.first, w.first);
			if (std::fabs(current - w.second) < 1e-6f)
				continue;
			bw.weights[w.first] = {current, w.second};
		}
		if (bw.weights.empty())
			continue;
		changed += bw.weights.size();
		uss.boneWeights.push_back(std::move(bw));
	}

	if (changed == 0)
		return 0;

	project.CreateSkinning(shape);
	for (auto& bw : uss.boneWeights)
		if (anim->GetShapeBoneIndex(shapeName, bw.boneName) < 0)
			anim->AddShapeBone(shapeName, bw.boneName);

	UndoStateProject* usp = frame->editor.undoHistory.PushState();
	usp->undoType = UndoType::Weight;
	usp->usss.push_back(std::move(uss));

	frame->ApplyRefresh(mcp.Ops().ApplyShapeEdits(*usp, false));
	frame->UpdateUndoTools();
	frame->UpdateAnimationGUI();
	frame->RefreshGUIWeightColors();
	frame->SetPendingChanges();
	return changed;
}

// The name of the bone on the other side of the body, or the name itself when there is none
std::string MirrorBoneName(const std::string& name) {
	static const std::vector<std::pair<std::string, std::string>> pairs = {
		{" L ", " R "}, {"[L", "[R"}, {"Left", "Right"}, {"left", "right"}, {"_L", "_R"}, {".L", ".R"}, {"L_", "R_"},
	};

	std::string mirrored;
	for (size_t i = 0; i < name.size();) {
		bool replaced = false;
		for (auto& pair : pairs) {
			for (int side = 0; side < 2 && !replaced; side++) {
				const std::string& from = side == 0 ? pair.first : pair.second;
				const std::string& to = side == 0 ? pair.second : pair.first;
				if (name.compare(i, from.size(), from) == 0) {
					mirrored += to;
					i += from.size();
					replaced = true;
				}
			}
			if (replaced)
				break;
		}
		if (!replaced)
			mirrored += name[i++];
	}

	if (mirrored != name && AnimSkeleton::getInstance().GetBonePtr(mirrored))
		return mirrored;
	return name;
}

// A pair of a vertex and its partner of a seam that drift apart with a slider
struct SeamOpening {
	float gap = 0.0f;
	const std::vector<SeamVertex>* group = nullptr;
};

json SeamGroupToJson(const std::vector<SeamVertex>& group) {
	json members = json::array();
	for (auto& m : group)
		members.push_back({{"shape", m.shape}, {"vertex", m.vertex}});
	return members;
}

// Largest distance between the deltas of the members of a seam group
float GroupOpening(const std::vector<SeamVertex>& group, std::map<std::string, std::vector<Vector3>>& deltas) {
	float largest = 0.0f;
	for (size_t a = 0; a < group.size(); a++) {
		for (size_t b = a + 1; b < group.size(); b++) {
			auto& da = deltas[group[a].shape];
			auto& db = deltas[group[b].shape];
			if (group[a].vertex < da.size() && group[b].vertex < db.size())
				largest = std::max(largest, da[group[a].vertex].DistanceTo(db[group[b].vertex]));
		}
	}
	return largest;
}

// Half the summed difference of the bone weights of the members of a seam group, the largest of any two members
float GroupWeightMismatch(AnimInfo* anim, const std::vector<SeamVertex>& group) {
	float largest = 0.0f;
	for (size_t a = 0; a < group.size(); a++) {
		for (size_t b = a + 1; b < group.size(); b++) {
			std::set<std::string> bones;
			for (auto& bone : ShapeBones(anim, group[a].shape))
				bones.insert(bone);
			for (auto& bone : ShapeBones(anim, group[b].shape))
				bones.insert(bone);

			float difference = 0.0f;
			for (auto& bone : bones)
				difference += std::fabs(WeightOf(anim, group[a].shape, bone, group[a].vertex) - WeightOf(anim, group[b].shape, bone, group[b].vertex));
			largest = std::max(largest, difference * 0.5f);
		}
	}
	return largest;
}

// The shapes of a seam check: the given or selected outfit shapes, and the reference unless withReference is false
std::vector<NiShape*> SeamShapes(OutfitStudioMcp& mcp, const json& args) {
	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes");
	NiShape* reference = mcp.Project().GetBaseShape();
	if (reference && McpArgs::Bool(args, "withReference", true))
		shapes.push_back(reference);
	return shapes;
}

McpToolResult ListMeshParts(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	const int minVertices = std::max(1, McpArgs::Int(args, "minVertices", 1));
	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 50), 1, 1000));

	auto verts = RestVerts(project, shape);
	auto tris = Triangles(shape);
	int count = 0;
	auto parts = MeshAnalysis::ConnectedParts(verts, tris, McpArgs::Float(args, "weldDistance", SeamTolerance), nullptr, &count);

	Mesh* m = project.GetHost().GetShapeMesh(shape->name.get());
	AnimInfo* anim = project.GetWorkAnim();
	auto bones = ShapeBones(anim, shape->name.get());

	struct Part {
		size_t vertices = 0;
		size_t triangles = 0;
		size_t masked = 0;
		Vector3 lo, hi, sum;
		int first = -1;
		std::map<std::string, float> boneSums;
	};
	std::vector<Part> list(count);

	for (size_t v = 0; v < verts.size(); v++) {
		Part& p = list[parts[v]];
		if (p.vertices == 0) {
			p.lo = p.hi = verts[v];
			p.first = static_cast<int>(v);
		}
		p.vertices++;
		p.sum += verts[v];
		p.lo = Vector3(std::min(p.lo.x, verts[v].x), std::min(p.lo.y, verts[v].y), std::min(p.lo.z, verts[v].z));
		p.hi = Vector3(std::max(p.hi.x, verts[v].x), std::max(p.hi.y, verts[v].y), std::max(p.hi.z, verts[v].z));
		if (m && m->mask && v < static_cast<size_t>(m->nVerts) && m->mask[v] > 0.0f)
			p.masked++;
		for (auto& bone : bones)
			p.boneSums[bone] += WeightOf(anim, shape->name.get(), bone, static_cast<uint16_t>(v));
	}
	for (auto& t : tris)
		if (t.p1 < parts.size())
			list[parts[t.p1]].triangles++;

	json result = json::array();
	size_t shown = 0;
	for (int i = 0; i < count && shown < limit; i++) {
		const Part& p = list[i];
		if (p.vertices < static_cast<size_t>(minVertices))
			continue;

		json entry = {
			{"part", i},
			{"vertices", p.vertices},
			{"triangles", p.triangles},
			{"center", VectorToJson(p.sum / static_cast<float>(p.vertices))},
			{"min", VectorToJson(p.lo)},
			{"max", VectorToJson(p.hi)},
			{"firstVertex", p.first},
		};
		if (p.masked)
			entry["masked"] = p.masked;

		auto top = std::max_element(p.boneSums.begin(), p.boneSums.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
		if (top != p.boneSums.end() && top->second > 0.0f)
			entry["mainBone"] = top->first;

		result.push_back(entry);
		shown++;
	}

	McpToolResult tool;
	tool.data["parts"] = count;
	tool.data["list"] = result;
	return tool;
}

McpToolResult RigidifySlider(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	auto shapes = mcp.Shapes(args, "shapes");

	// All morph sliders that move the shapes, unless they are named
	std::vector<std::string> sliders = MorphSliders(project, args, "sliders", shapes);
	if (sliders.empty())
		throw McpToolError("No sliders to rigidify: none are named in 'sliders', and none have data on the shapes.");

	RigidifyOptions options;
	options.allowScale = McpArgs::Bool(args, "allowScale", false);
	options.perPart = McpArgs::Bool(args, "perPart", true);
	options.weldDistance = McpArgs::Float(args, "weldDistance", options.weldDistance);
	options.clearance = McpArgs::Float(args, "clearance", 0.02f);
	options.maxPartVertices = std::max(0, McpArgs::Int(args, "maxPartVertices", 0));
	options.maxPartLength = std::max(0.0f, McpArgs::Float(args, "maxPartLength", 0.0f));
	size_t bending = 0;
	options.outBendingParts = &bending;

	UndoHistory& history = frame->editor.undoHistory;
	json changed = json::object();
	size_t parts = 0;
	for (auto& slider : sliders) {
		UndoStateProject* usp = history.PushState();
		size_t sliderParts = 0;
		bending = 0;
		OpResult result = mcp.Ops().RigidifySlider(shapes, slider, options, *usp, &sliderParts);
		size_t vertices = 0;
		for (auto& uss : usp->usss)
			vertices += uss.restDiffs.size();
		if (!result.ok || usp->usss.empty())
			history.PopState();

		frame->UpdateUndoTools();
		mcp.Apply(result);
		if (vertices)
			changed[slider] = vertices;
		parts = sliderParts;
	}

	McpToolResult result;
	result.data["parts"] = parts;
	if (options.maxPartVertices > 0 || options.maxPartLength > 0.0f)
		result.data["bendingParts"] = bending;
	result.data["slidersChecked"] = sliders.size();
	// Only the sliders that moved the parts, the others had no morph there
	result.data["changedVertices"] = changed;
	return result;
}

McpToolResult AnalyzeSeams(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	auto shapes = SeamShapes(mcp, args);
	const float tolerance = McpArgs::Float(args, "tolerance", SeamTolerance);
	const float opening = McpArgs::Float(args, "opening", 0.01f);
	const float weightTolerance = McpArgs::Float(args, "weightTolerance", 0.05f);
	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 1, 1000));

	auto groups = SeamGroups(project, shapes, tolerance);

	// Which shapes meet where
	std::map<std::string, size_t> joins;
	for (auto& group : groups) {
		std::set<std::string> names;
		for (auto& m : group)
			names.insert(m.shape);
		std::string key;
		for (auto& name : names)
			key += (key.empty() ? "" : " + ") + name;
		joins[key] += 1;
	}

	std::vector<NiShape*> outfitShapes;
	for (auto* shape : shapes)
		if (!project.IsBaseShape(shape))
			outfitShapes.push_back(shape);

	struct Row {
		std::string slider;
		size_t opened = 0;
		float largest = 0.0f;
		const std::vector<SeamVertex>* worst = nullptr;
	};
	std::vector<Row> rows;
	for (auto& slider : MorphSliders(project, args, "sliders", shapes)) {
		auto deltas = ShapeDeltas(mcp, shapes, slider);
		Row row;
		row.slider = slider;
		for (auto& group : groups) {
			float gap = GroupOpening(group, deltas);
			if (gap > opening)
				row.opened++;
			if (gap > row.largest) {
				row.largest = gap;
				row.worst = &group;
			}
		}
		if (row.opened)
			rows.push_back(row);
	}
	std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.largest > b.largest; });

	json sliderList = json::array();
	for (size_t i = 0; i < rows.size() && i < limit; i++) {
		json entry = {{"slider", rows[i].slider}, {"openSeams", rows[i].opened}, {"largestGap", rows[i].largest}};
		if (rows[i].worst)
			entry["worst"] = SeamGroupToJson(*rows[i].worst);
		sliderList.push_back(entry);
	}

	// Seams whose sides are weighted differently tear open when the body moves
	AnimInfo* anim = project.GetWorkAnim();
	size_t weightMismatch = 0;
	float largestMismatch = 0.0f;
	const std::vector<SeamVertex>* worstWeights = nullptr;
	for (auto& group : groups) {
		float mismatch = GroupWeightMismatch(anim, group);
		if (mismatch > weightTolerance)
			weightMismatch++;
		if (mismatch > largestMismatch) {
			largestMismatch = mismatch;
			worstWeights = &group;
		}
	}

	McpToolResult result;
	result.data["seams"] = groups.size();
	result.data["joins"] = joins;
	result.data["sliders"] = sliderList;
	if (rows.size() > limit)
		result.data["moreSliders"] = rows.size() - limit;
	json weights = {{"mismatched", weightMismatch}, {"largest", largestMismatch}};
	if (worstWeights && weightMismatch)
		weights["worst"] = SeamGroupToJson(*worstWeights);
	result.data["weights"] = weights;
	return result;
}

McpToolResult WeldSeams(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	auto shapes = SeamShapes(mcp, args);
	const float tolerance = McpArgs::Float(args, "tolerance", SeamTolerance);
	const bool useMask = McpArgs::Bool(args, "useMask", true);
	const bool doSliders = McpArgs::Bool(args, "sliderData", true);
	const bool doWeights = McpArgs::Bool(args, "weights", false);

	auto groups = SeamGroups(project, shapes, tolerance);

	// How much each outfit vertex may change, by the mask
	std::map<std::string, std::vector<float>> influence;
	for (auto* shape : shapes) {
		if (project.IsBaseShape(shape))
			continue;
		std::vector<float> f;
		if (useMask)
			f = MaskOps::FixWeights(project.GetHost().GetShapeMesh(shape->name.get()));
		if (f.size() != shape->GetNumVertices())
			f.assign(shape->GetNumVertices(), 1.0f);
		influence[shape->name.get()] = f;
	}

	McpToolResult result;
	result.data["seams"] = groups.size();

	if (doSliders) {
		UndoHistory& history = frame->editor.undoHistory;
		json changedSliders = json::object();
		for (auto& slider : MorphSliders(project, args, "sliders", shapes)) {
			auto deltas = ShapeDeltas(mcp, shapes, slider);

			std::map<std::string, std::unordered_map<uint16_t, Vector3>> changes;
			for (auto& group : groups) {
				// The reference's morph where the outfit meets it, otherwise the mean of the sides
				const SeamVertex* reference = nullptr;
				for (auto& m : group)
					if (m.reference)
						reference = &m;

				Vector3 target;
				if (reference)
					target = deltas[reference->shape][reference->vertex];
				else {
					for (auto& m : group)
						target += deltas[m.shape][m.vertex];
					target /= static_cast<float>(group.size());
				}

				for (auto& m : group) {
					if (m.reference)
						continue;
					Vector3 change = (target - deltas[m.shape][m.vertex]) * influence[m.shape][m.vertex];
					if (change.length() > 1e-6f)
						changes[m.shape][m.vertex] = change;
				}
			}

			if (changes.empty())
				continue;

			UndoStateProject* usp = history.PushState();
			OpResult combined;
			size_t vertices = 0;
			for (auto& change : changes) {
				OpResult r = mcp.Ops().ChangeSliderDeltas(mcp.Shape(change.first), slider, change.second, *usp);
				combined.refresh |= r.refresh;
				if (!r.ok) {
					combined.ok = false;
					combined.error = r.error;
					break;
				}
				vertices += change.second.size();
			}
			if (usp->usss.empty())
				history.PopState();
			frame->UpdateUndoTools();
			mcp.Apply(combined);
			changedSliders[slider] = vertices;
		}
		result.data["sliders"] = changedSliders;
	}

	if (doWeights) {
		AnimInfo* anim = project.GetWorkAnim();
		std::map<std::string, std::map<std::string, std::unordered_map<uint16_t, float>>> newWeights;
		for (auto& group : groups) {
			// The reference's weights where the outfit meets it, otherwise the mean of the sides
			std::map<std::string, float> target;
			const SeamVertex* reference = nullptr;
			for (auto& m : group)
				if (m.reference)
					reference = &m;

			if (reference) {
				for (auto& bone : ShapeBones(anim, reference->shape)) {
					float w = WeightOf(anim, reference->shape, bone, reference->vertex);
					if (w > 0.0f)
						target[bone] = w;
				}
			}
			else {
				for (auto& m : group)
					for (auto& bone : ShapeBones(anim, m.shape))
						target[bone] += WeightOf(anim, m.shape, bone, m.vertex) / static_cast<float>(group.size());
			}

			float sum = 0.0f;
			for (auto& t : target)
				sum += t.second;
			if (sum <= 0.0f)
				continue;

			for (auto& m : group) {
				if (m.reference)
					continue;
				const float f = influence[m.shape][m.vertex];
				if (f <= 0.0f)
					continue;

				std::set<std::string> bones;
				for (auto& bone : ShapeBones(anim, m.shape))
					bones.insert(bone);
				for (auto& t : target)
					bones.insert(t.first);

				for (auto& bone : bones) {
					float current = WeightOf(anim, m.shape, bone, m.vertex);
					auto t = target.find(bone);
					float goal = t != target.end() ? t->second / sum : 0.0f;
					newWeights[m.shape][bone][m.vertex] = current + (goal - current) * f;
				}
			}
		}

		json changedWeights = json::object();
		for (auto& shapeWeights : newWeights)
			changedWeights[shapeWeights.first] = ApplyWeights(mcp, mcp.Shape(shapeWeights.first), shapeWeights.second);
		result.data["weights"] = changedWeights;
	}

	return result;
}

McpToolResult AnalyzeFit(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	auto shapes = mcp.Shapes(args, "shapes");
	const float value = McpArgs::Float(args, "value", 1.0f);
	const float maxDistance = McpArgs::Float(args, "maxDistance", 2.0f);
	const float threshold = McpArgs::Float(args, "threshold", 0.3f);
	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 1, 1000));

	std::vector<NiShape*> withReference = shapes;
	if (project.GetBaseShape())
		withReference.push_back(project.GetBaseShape());
	auto sliders = MorphSliders(project, args, "sliders", withReference);

	struct Row {
		json entry;
		double score = 0.0;
	};
	std::vector<Row> rows;

	McpSliderValueScope values(project);
	for (auto* shape : shapes) {
		NiShape* against = McpAgainstShape(mcp, args, shape);
		values.Zero();
		auto before = McpSignedDistances(mcp, shape, against);

		for (auto& slider : sliders) {
			values.Zero();
			values.Set({{slider, value}});
			auto after = McpSignedDistances(mcp, shape, against);

			Row row;
			json gap = McpGapStats(before, after, maxDistance, threshold, &row.score);
			if (row.score <= 0.0)
				continue;
			row.entry = {{"shape", shape->name.get()}, {"slider", slider}, {"gap", gap}};
			rows.push_back(std::move(row));
		}
	}

	std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.score > b.score; });

	json list = json::array();
	for (size_t i = 0; i < rows.size() && i < limit; i++)
		list.push_back(rows[i].entry);

	McpToolResult result;
	result.data["value"] = value;
	result.data["sliders"] = sliders.size();
	result.data["worst"] = list;
	if (rows.size() > limit)
		result.data["more"] = rows.size() - limit;
	return result;
}

McpToolResult CleanWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	const float pruneBelow = McpArgs::Float(args, "pruneBelow", 0.0f);
	const int maxInfluences = McpArgs::Int(args, "maxInfluences", 0);
	const bool normalize = McpArgs::Bool(args, "normalize", true);
	const bool useMask = McpArgs::Bool(args, "useMask", true);

	McpToolResult result;
	for (auto* shape : mcp.Shapes(args, "shapes")) {
		const std::string shapeName = shape->name.get();
		auto bones = ShapeBones(anim, shapeName);
		std::vector<float> influence;
		if (useMask)
			influence = MaskOps::FixWeights(project.GetHost().GetShapeMesh(shapeName));

		std::map<std::string, std::unordered_map<uint16_t, float>> newWeights;
		size_t pruned = 0, limited = 0, normalized = 0;
		for (uint16_t v = 0; v < shape->GetNumVertices(); v++) {
			if (!influence.empty() && v < influence.size() && influence[v] <= 0.0f)
				continue;

			std::vector<std::pair<float, std::string>> weights;
			for (auto& bone : bones) {
				float w = WeightOf(anim, shapeName, bone, v);
				if (w > 0.0f)
					weights.emplace_back(w, bone);
			}
			if (weights.empty())
				continue;

			std::vector<std::pair<float, std::string>> kept;
			for (auto& w : weights) {
				if (w.first < pruneBelow)
					pruned++;
				else
					kept.push_back(w);
			}
			// Keep the largest one when all are below the limit, a vertex shouldn't lose all its weights
			if (kept.empty()) {
				kept.push_back(*std::max_element(weights.begin(), weights.end()));
				pruned--;
			}

			std::sort(kept.begin(), kept.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
			if (maxInfluences > 0 && kept.size() > static_cast<size_t>(maxInfluences)) {
				limited++;
				kept.resize(maxInfluences);
			}

			float sum = 0.0f;
			for (auto& w : kept)
				sum += w.first;
			const bool rescale = normalize && sum > 0.0f && std::fabs(sum - 1.0f) > 1e-4f;
			if (rescale)
				normalized++;

			std::map<std::string, float> final;
			for (auto& w : kept)
				final[w.second] = rescale ? w.first / sum : w.first;

			for (auto& w : weights) {
				auto it = final.find(w.second);
				float goal = it != final.end() ? it->second : 0.0f;
				if (std::fabs(goal - w.first) > 1e-6f)
					newWeights[w.second][v] = goal;
			}
		}

		size_t changed = ApplyWeights(mcp, shape, newWeights);
		result.data["shapes"][shapeName] = {{"changedWeights", changed}, {"pruned", pruned}, {"limited", limited}, {"normalized", normalized}};
	}
	return result;
}

McpToolResult MirrorWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	std::string from = McpArgs::String(args, "from");
	if (from != "left" && from != "right")
		McpArgs::Wrong("from", "\"left\" or \"right\"");
	// +X is the character's right
	const float sourceSign = from == "right" ? 1.0f : -1.0f;
	const float center = McpArgs::Float(args, "centerTolerance", 0.01f);
	const float maxDistance = McpArgs::Float(args, "maxDistance", 1.0f);
	const bool useMask = McpArgs::Bool(args, "useMask", true);

	McpToolResult result;
	for (auto* shape : mcp.Shapes(args, "shapes")) {
		const std::string shapeName = shape->name.get();
		auto verts = RestVerts(project, shape);
		auto tris = Triangles(shape);
		auto bones = ShapeBones(anim, shapeName);
		std::vector<float> influence;
		if (useMask)
			influence = MaskOps::FixWeights(project.GetHost().GetShapeMesh(shapeName));

		// Only triangles on the source side give weights
		std::vector<Triangle> sourceTris;
		for (auto& t : tris)
			if (verts[t.p1].x * sourceSign >= -center && verts[t.p2].x * sourceSign >= -center && verts[t.p3].x * sourceSign >= -center)
				sourceTris.push_back(t);

		std::vector<uint16_t> targets;
		std::vector<Vector3> mirrored;
		for (size_t v = 0; v < verts.size(); v++) {
			if (verts[v].x * sourceSign >= -center)
				continue;
			if (!influence.empty() && influence[v] <= 0.0f)
				continue;
			targets.push_back(static_cast<uint16_t>(v));
			mirrored.emplace_back(-verts[v].x, verts[v].y, verts[v].z);
		}

		auto closest = MeshAnalysis::ClosestSurfacePoints(verts, sourceTris, mirrored, maxDistance);

		std::map<std::string, std::unordered_map<uint16_t, float>> newWeights;
		size_t mirroredCount = 0, missed = 0;
		for (size_t i = 0; i < targets.size(); i++) {
			const uint16_t v = targets[i];
			const MeshAnalysis::SurfacePoint& sp = closest[i];
			if (sp.triangle < 0) {
				missed++;
				continue;
			}

			const Triangle& t = sourceTris[sp.triangle];
			std::map<std::string, float> goal;
			float sum = 0.0f;
			for (auto& bone : bones) {
				float w = WeightOf(anim, shapeName, bone, t.p1) * sp.u + WeightOf(anim, shapeName, bone, t.p2) * sp.v + WeightOf(anim, shapeName, bone, t.p3) * sp.w;
				if (w <= 0.0f)
					continue;
				goal[MirrorBoneName(bone)] += w;
				sum += w;
			}
			if (sum <= 0.0f) {
				missed++;
				continue;
			}

			const float f = influence.empty() ? 1.0f : influence[v];
			std::set<std::string> all(bones.begin(), bones.end());
			for (auto& g : goal)
				all.insert(g.first);
			for (auto& bone : all) {
				float current = WeightOf(anim, shapeName, bone, v);
				auto it = goal.find(bone);
				float target = it != goal.end() ? it->second / sum : 0.0f;
				newWeights[bone][v] = current + (target - current) * f;
			}
			mirroredCount++;
		}

		size_t changed = ApplyWeights(mcp, shape, newWeights);
		json entry = {{"mirroredVertices", mirroredCount}, {"changedWeights", changed}};
		if (missed)
			entry["noSourceFound"] = missed;
		result.data["shapes"][shapeName] = entry;
	}
	return result;
}

// Weights of a shape as one row per vertex and one column per bone
MeshAnalysis::DenseWeights DenseShapeWeights(AnimInfo* anim, const std::string& shape, const std::vector<std::string>& bones, size_t vertexCount) {
	MeshAnalysis::DenseWeights dense(vertexCount, std::vector<float>(bones.size(), 0.0f));
	for (size_t b = 0; b < bones.size(); b++)
		if (auto weights = anim->GetWeightsPtr(shape, bones[b]))
			for (auto& w : *weights)
				if (w.first < vertexCount)
					dense[w.first][b] = w.second;
	return dense;
}

// The new weights ApplyWeights takes to go from one set of dense weights to another
std::map<std::string, std::unordered_map<uint16_t, float>> DenseWeightChanges(const MeshAnalysis::DenseWeights& before,
																			   const MeshAnalysis::DenseWeights& after,
																			   const std::vector<std::string>& bones) {
	std::map<std::string, std::unordered_map<uint16_t, float>> changes;
	for (size_t v = 0; v < before.size() && v < after.size(); v++)
		for (size_t b = 0; b < bones.size(); b++)
			if (std::fabs(after[v][b] - before[v][b]) > 1e-6f)
				changes[bones[b]][static_cast<uint16_t>(v)] = after[v][b];
	return changes;
}

std::vector<Vector3> MorphedVerts(OutfitProject& project, NiShape* shape) {
	std::vector<Vector3> verts;
	project.GetMorphedVerts(shape, verts);
	return verts;
}

// Poses of args["poses"], or all poses of list_poses
std::vector<std::string> PoseNames(OutfitStudioMcp& mcp, const json& args) {
	std::vector<std::string> poses = McpArgs::Strings(args, "poses");
	if (poses.empty())
		for (auto& pose : mcp.GetFrame()->poseDataCollection.poseData)
			poses.push_back(pose.name);
	if (poses.empty())
		throw McpToolError("There are no poses to check. list_poses lists them.");
	return poses;
}

// Skins the vertices of a shape in poses with any weights, to try weights out without changing the shape, the way
// ApplySkinningToVerts does it with the shape's own. The bone transforms of each pose are taken once.
class PoseSkinner {
public:
	PoseSkinner(OutfitStudioMcp& mcp, NiShape* shape, const std::vector<std::string>& bones, const std::vector<std::string>& poses) {
		OutfitProject& project = mcp.Project();
		AnimInfo* anim = project.GetWorkAnim();
		const bool isSF = project.GetWorkNif()->GetHeader().GetVersion().IsSF();
		const std::string shapeName = shape->name.get();
		const MatTransform globalToSkin = anim->GetTransformGlobalToShape(shape);

		McpPoseScope scope(mcp);
		for (auto& pose : poses) {
			scope.Set(pose);
			std::vector<MatTransform> list(bones.size());
			std::vector<bool> valid(bones.size(), false);
			for (size_t b = 0; b < bones.size(); b++) {
				AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(bones[b]);
				MatTransform skinToBone;
				if (!bone || !anim->GetXFormSkinToBone(shapeName, bones[b], skinToBone))
					continue;
				valid[b] = true;
				// The unposed skeleton leaves the vertices where they are
				if (pose == "none")
					continue;
				list[b] = globalToSkin.ComposeTransforms(bone->xformPoseToGlobal.ComposeTransforms(skinToBone));
				if (isSF)
					list[b].translation *= sfHavokScale;
			}
			transforms.push_back(std::move(list));
			validBones.push_back(std::move(valid));
		}
	}

	std::vector<Vector3> Posed(size_t pose, const std::vector<Vector3>& verts, const MeshAnalysis::DenseWeights& weights) const {
		std::vector<Vector3> posed(verts.size());
		for (size_t v = 0; v < verts.size(); v++) {
			Vector3 p;
			float total = 0.0f;
			if (v < weights.size()) {
				for (size_t b = 0; b < weights[v].size() && b < transforms[pose].size(); b++) {
					const float w = weights[v][b];
					if (w <= 0.0f || !validBones[pose][b])
						continue;
					p += transforms[pose][b].ApplyTransform(verts[v]) * w;
					total += w;
				}
			}
			posed[v] = total < EPSILON ? verts[v] : p / total;
		}
		return posed;
	}

private:
	std::vector<std::vector<MatTransform>> transforms; // By pose and bone
	std::vector<std::vector<bool>> validBones;
};

// Strain of a shape in each pose with the given weights, from the unposed vertices
std::vector<MeshAnalysis::StrainStats> PoseStrain(const PoseSkinner& skinner,
												  size_t poseCount,
												  const std::vector<Vector3>& verts,
												  const std::vector<Triangle>& tris,
												  const MeshAnalysis::DenseWeights& weights,
												  float threshold) {
	std::vector<MeshAnalysis::StrainStats> stats;
	for (size_t i = 0; i < poseCount; i++)
		stats.push_back(MeshAnalysis::AnalyzeStrain(verts, skinner.Posed(i, verts, weights), tris, threshold));
	return stats;
}

json StrainToJson(const MeshAnalysis::StrainStats& stats) {
	return {{"stretchedEdges", stats.stretched}, {"maxStrain", stats.maxStrain}, {"meanStrain", stats.meanStrain}};
}

int DefaultInfluenceLimit() {
	return static_cast<TargetGame>(Config.GetIntValue("TargetGame")) == SF ? 8 : 4;
}

// Smoothing passes that spread weights over about radius (NIF units) on a mesh with the mean edge length. Weights
// spread like heat: the reach grows with the square root of the passes.
int SmoothIterationsForRadius(float radius, float meanEdge, float strength) {
	if (meanEdge <= 0.0f || strength <= 0.0f)
		return 1;
	const float rings = radius / meanEdge;
	return std::clamp(static_cast<int>(std::ceil(rings * rings / strength)), 1, 5000);
}

float MeanEdgeLength(const std::vector<Vector3>& verts, const std::vector<std::vector<int>>& neighbors) {
	double sum = 0.0;
	size_t count = 0;
	for (size_t v = 0; v < verts.size() && v < neighbors.size(); v++)
		for (int n : neighbors[v]) {
			const float length = verts[v].DistanceTo(verts[n]);
			// Copies of a vertex along split seams aren't edges
			if (length > SeamTolerance) {
				sum += length;
				count++;
			}
		}
	return count ? static_cast<float>(sum / count) : 0.0f;
}

McpToolResult AnalyzePoses(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	auto shapes = mcp.Shapes(args, "shapes");
	auto poses = PoseNames(mcp, args);
	const float threshold = McpArgs::Float(args, "threshold", 0.5f);
	const size_t worst = static_cast<size_t>(std::clamp(McpArgs::Int(args, "worstVertices", 5), 0, 1000));

	McpSliderValueScope sliders(project);
	if (McpHasSliderValuesArg(args)) {
		sliders.Zero();
		sliders.Set(McpSliderValuesArg(mcp, args));
	}

	// The body's own strain in each pose, for what's natural there: skin stretches over joints too
	std::vector<MeshAnalysis::StrainStats> referenceStats;
	NiShape* reference = project.GetBaseShape();
	if (reference && McpArgs::Bool(args, "withReference", true) && project.GetWorkAnim()->shapeSkinning.count(reference->name.get())) {
		auto bones = ShapeBones(anim, reference->name.get());
		auto verts = MorphedVerts(project, reference);
		PoseSkinner skinner(mcp, reference, bones, poses);
		referenceStats = PoseStrain(skinner, poses.size(), verts, Triangles(reference), DenseShapeWeights(anim, reference->name.get(), bones, verts.size()), threshold);
	}

	McpToolResult result;
	for (auto* shape : shapes) {
		const std::string shapeName = shape->name.get();
		auto bones = ShapeBones(anim, shapeName);
		auto verts = MorphedVerts(project, shape);
		PoseSkinner skinner(mcp, shape, bones, poses);
		auto stats = PoseStrain(skinner, poses.size(), verts, Triangles(shape), DenseShapeWeights(anim, shapeName, bones, verts.size()), threshold);

		std::vector<size_t> order(poses.size());
		for (size_t i = 0; i < order.size(); i++)
			order[i] = i;
		std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return stats[a].maxStrain > stats[b].maxStrain; });

		json list = json::array();
		for (size_t i : order) {
			json entry = StrainToJson(stats[i]);
			entry["pose"] = poses[i];

			std::vector<std::pair<float, int>> sorted;
			for (size_t v = 0; v < stats[i].vertexStrain.size(); v++)
				if (stats[i].vertexStrain[v] > threshold)
					sorted.emplace_back(stats[i].vertexStrain[v], static_cast<int>(v));
			std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
			if (worst > 0) {
				json vertices = json::array();
				for (size_t k = 0; k < sorted.size() && k < worst; k++)
					vertices.push_back({{"vertex", sorted[k].second}, {"strain", sorted[k].first}});
				entry["worstVertices"] = vertices;
			}
			entry["stretchedVertices"] = sorted.size();
			if (i < referenceStats.size())
				entry["reference"] = StrainToJson(referenceStats[i]);
			list.push_back(entry);
		}
		result.data["shapes"][shapeName] = {{"edges", stats.empty() ? 0 : stats.front().edges}, {"poses", list}};
	}
	result.data["threshold"] = threshold;
	return result;
}

McpToolResult SmoothShapeWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	const float strength = std::clamp(McpArgs::Float(args, "strength", 0.5f), 0.01f, 1.0f);
	const int maxInfluences = McpArgs::Int(args, "maxInfluences", DefaultInfluenceLimit());
	const float pruneBelow = McpArgs::Float(args, "pruneBelow", 0.01f);
	const bool useMask = McpArgs::Bool(args, "useMask", true);
	std::vector<std::string> onlyBones = McpArgs::Strings(args, "bones");

	McpToolResult result;
	for (auto* shape : mcp.Shapes(args, "shapes")) {
		const std::string shapeName = shape->name.get();
		auto bones = ShapeBones(anim, shapeName);
		if (bones.empty())
			throw McpToolError("'" + shapeName + "' has no bone weights to smooth.");
		auto verts = RestVerts(project, shape);
		auto neighbors = VertexNeighbors(verts, Triangles(shape));

		std::vector<bool> boneFilter;
		if (!onlyBones.empty()) {
			boneFilter.assign(bones.size(), false);
			for (auto& name : onlyBones) {
				auto it = std::find(bones.begin(), bones.end(), name);
				if (it == bones.end())
					throw McpToolError("'" + shapeName + "' has no bone '" + name + "'.");
				boneFilter[it - bones.begin()] = true;
			}
		}

		int iterations = std::clamp(McpArgs::Int(args, "iterations", 10), 1, 5000);
		const float meanEdge = MeanEdgeLength(verts, neighbors);
		if (McpArgs::Has(args, "radius"))
			iterations = SmoothIterationsForRadius(McpArgs::Float(args, "radius", 2.0f), meanEdge, strength);

		std::vector<float> influence;
		if (useMask)
			influence = MaskOps::FixWeights(ShapeMesh(mcp, shape));
		if (influence.size() != verts.size())
			influence.assign(verts.size(), 1.0f);

		auto before = DenseShapeWeights(anim, shapeName, bones, verts.size());
		auto after = MeshAnalysis::SmoothWeights(before, neighbors, influence, iterations, strength, boneFilter.empty() ? nullptr : &boneFilter);
		for (size_t v = 0; v < after.size(); v++)
			if (influence[v] > 0.0f)
				MeshAnalysis::LimitWeights(after[v], maxInfluences, pruneBelow);

		auto jumpStats = [&](const MeshAnalysis::DenseWeights& weights) {
			auto jump = MeshAnalysis::WeightJump(weights, neighbors);
			float maxJump = 0.0f;
			double sum = 0.0;
			size_t count = 0;
			for (size_t v = 0; v < jump.size(); v++) {
				if (influence[v] <= 0.0f)
					continue;
				maxJump = std::max(maxJump, jump[v]);
				sum += jump[v];
				count++;
			}
			return json{{"max", maxJump}, {"mean", count ? sum / count : 0.0}};
		};

		auto changes = DenseWeightChanges(before, after, bones);
		std::set<uint16_t> changedVertices;
		for (auto& bone : changes)
			for (auto& w : bone.second)
				changedVertices.insert(w.first);

		size_t changed = ApplyWeights(mcp, shape, changes);
		result.data["shapes"][shapeName] = {
			{"iterations", iterations},
			{"meanEdgeLength", meanEdge},
			{"changedVertices", changedVertices.size()},
			{"changedWeights", changed},
			{"weightJumpBefore", jumpStats(before)},
			{"weightJumpAfter", jumpStats(after)},
		};
	}
	return result;
}

McpToolResult ReassignWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	std::vector<std::string> from = McpArgs::Strings(args, "from");
	if (from.empty())
		McpArgs::Wrong("from", "a list of bone names");
	const bool toParent = McpArgs::Bool(args, "toParent", false);
	std::string to = McpArgs::String(args, "to", "");
	if (to.empty() == !toParent)
		throw McpToolError("Give either 'to' or toParent: true.");
	const bool useMask = McpArgs::Bool(args, "useMask", true);

	// Where each bone's weights go
	std::map<std::string, std::string> targets;
	for (auto& bone : from) {
		if (!toParent) {
			if (bone == to)
				throw McpToolError("'" + bone + "' can't be moved onto itself.");
			targets[bone] = to;
			continue;
		}
		AnimBone* animBone = AnimSkeleton::getInstance().GetBonePtr(bone);
		if (!animBone || !animBone->parent)
			throw McpToolError("The skeleton has no parent for '" + bone + "'.");
		targets[bone] = animBone->parent->boneName;
	}

	McpToolResult result;
	for (auto* shape : mcp.Shapes(args, "shapes")) {
		const std::string shapeName = shape->name.get();
		std::vector<float> influence;
		if (useMask)
			influence = MaskOps::FixWeights(ShapeMesh(mcp, shape));

		std::map<std::string, std::unordered_map<uint16_t, float>> newWeights;
		std::set<uint16_t> moved;
		for (auto& target : targets) {
			auto weights = anim->GetWeightsPtr(shapeName, target.first);
			if (!weights)
				continue;
			for (auto& w : *weights) {
				const float f = influence.empty() || w.first >= influence.size() ? 1.0f : influence[w.first];
				if (f <= 0.0f || w.second <= 0.0f)
					continue;
				const float amount = w.second * f;
				// Weights already moved onto this bone, or what it has
				auto& goal = newWeights[target.second];
				auto existing = goal.find(w.first);
				const float current = existing != goal.end() ? existing->second : WeightOf(anim, shapeName, target.second, w.first);
				goal[w.first] = current + amount;

				auto& source = newWeights[target.first];
				auto sourceExisting = source.find(w.first);
				const float sourceCurrent = sourceExisting != source.end() ? sourceExisting->second : w.second;
				source[w.first] = sourceCurrent - amount;
				moved.insert(w.first);
			}
		}

		size_t changed = ApplyWeights(mcp, shape, newWeights);
		json entry = {{"movedVertices", moved.size()}, {"changedWeights", changed}};
		json empty = json::array();
		for (auto& target : targets) {
			auto weights = anim->GetWeightsPtr(shapeName, target.first);
			bool any = false;
			if (weights)
				for (auto& w : *weights)
					any = any || w.second > 0.0f;
			if (anim->GetShapeBoneIndex(shapeName, target.first) >= 0 && !any)
				empty.push_back(target.first);
		}
		if (!empty.empty())
			entry["emptyBones"] = empty;
		result.data["shapes"][shapeName] = entry;
	}
	result.data["moves"] = targets;
	return result;
}

McpToolResult AutoFixWeights(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	AnimInfo* anim = project.GetWorkAnim();
	auto shapes = mcp.Shapes(args, "shapes");
	auto poses = PoseNames(mcp, args);
	const float threshold = McpArgs::Float(args, "threshold", 0.5f);
	const float strength = std::clamp(McpArgs::Float(args, "strength", 0.5f), 0.01f, 1.0f);
	const float startRadius = std::max(0.01f, McpArgs::Float(args, "radius", 2.0f));
	const float radiusStep = std::max(0.1f, McpArgs::Float(args, "radiusStep", 2.0f));
	const float maxRadius = std::max(startRadius, McpArgs::Float(args, "maxRadius", 12.0f));
	const std::vector<std::string> onlyBones = McpArgs::Strings(args, "bones");
	const int grow = std::clamp(McpArgs::Int(args, "grow", 3), 0, 50);
	const int feather = std::clamp(McpArgs::Int(args, "feather", 4), 0, 50);
	const float keepClose = McpArgs::Float(args, "keepClose", 0.0f);
	const float maxPoseIncrease = std::max(0.0f, McpArgs::Float(args, "maxPoseIncrease", 0.25f));
	const int maxInfluences = McpArgs::Int(args, "maxInfluences", DefaultInfluenceLimit());
	const float pruneBelow = McpArgs::Float(args, "pruneBelow", 0.01f);
	const bool useMask = McpArgs::Bool(args, "useMask", true);

	McpSliderValueScope sliders(project);
	sliders.Zero();
	NiShape* reference = project.GetBaseShape();

	McpToolResult result;
	for (auto* shape : shapes) {
		const std::string shapeName = shape->name.get();
		auto bones = ShapeBones(anim, shapeName);
		if (bones.empty())
			throw McpToolError("'" + shapeName + "' has no bone weights.");
		auto verts = MorphedVerts(project, shape);
		auto tris = Triangles(shape);
		auto neighbors = VertexNeighbors(verts, tris);
		const float meanEdge = MeanEdgeLength(verts, neighbors);

		// What may change: the user's mask, and nothing that sits on the body, whose weights should stay the body's
		std::vector<float> allowed;
		if (useMask)
			allowed = MaskOps::FixWeights(ShapeMesh(mcp, shape));
		if (allowed.size() != verts.size())
			allowed.assign(verts.size(), 1.0f);
		size_t keptClose = 0;
		if (reference && keepClose > 0.0f) {
			McpPoseScope unposed(mcp);
			unposed.Set("none");
			auto distances = McpSignedDistances(mcp, shape, reference);
			for (size_t v = 0; v < verts.size() && v < distances.size(); v++)
				if (distances[v] != FLT_MAX && distances[v] < keepClose && allowed[v] > 0.0f) {
					allowed[v] = 0.0f;
					keptClose++;
				}
		}

		PoseSkinner skinner(mcp, shape, bones, poses);
		auto start = DenseShapeWeights(anim, shapeName, bones, verts.size());
		auto current = start;

		// Bones to blend: the given ones, or each step the bones whose weights differ most across the stretched edges,
		// e.g. the two thighs a skirt is split between. Blending only those keeps the others (spine, pelvis) as they are.
		std::vector<bool> givenBones;
		if (!onlyBones.empty()) {
			givenBones.assign(bones.size(), false);
			for (auto& name : onlyBones) {
				auto it = std::find(bones.begin(), bones.end(), name);
				if (it == bones.end())
					throw McpToolError("'" + shapeName + "' has no bone '" + name + "'.");
				givenBones[it - bones.begin()] = true;
			}
		}
		auto pickBones = [&](const std::vector<bool>& strained) {
			if (!givenBones.empty())
				return givenBones;
			std::vector<double> difference(bones.size(), 0.0);
			double total = 0.0;
			for (size_t v = 0; v < verts.size(); v++) {
				if (!strained[v])
					continue;
				for (int n : neighbors[v]) {
					if (!strained[n])
						continue;
					for (size_t b = 0; b < bones.size(); b++) {
						const double d = std::fabs(current[v][b] - current[n][b]);
						difference[b] += d;
						total += d;
					}
				}
			}
			std::vector<bool> picked(bones.size(), false);
			for (size_t b = 0; b < bones.size(); b++)
				picked[b] = total > 0.0 && difference[b] >= total * 0.1;
			return picked;
		};
		auto boneNames = [&](const std::vector<bool>& picked) {
			json list = json::array();
			for (size_t b = 0; b < bones.size(); b++)
				if (picked[b])
					list.push_back(bones[b]);
			return list;
		};

		// The body stretches over its joints too: a pose only needs fixing where the shape stretches more than the
		// body does in it
		std::vector<float> poseThreshold(poses.size(), threshold);
		json referencePeaks = json::object();
		if (reference && McpArgs::Bool(args, "relativeToReference", true) && anim->shapeSkinning.count(reference->name.get())) {
			auto referenceBones = ShapeBones(anim, reference->name.get());
			auto referenceVerts = MorphedVerts(project, reference);
			PoseSkinner referenceSkinner(mcp, reference, referenceBones, poses);
			auto referenceStats = PoseStrain(referenceSkinner, poses.size(), referenceVerts, Triangles(reference),
											 DenseShapeWeights(anim, reference->name.get(), referenceBones, referenceVerts.size()), threshold);
			for (size_t i = 0; i < poses.size(); i++) {
				poseThreshold[i] = std::max(threshold, referenceStats[i].maxStrain);
				referencePeaks[poses[i]] = referenceStats[i].maxStrain;
			}
		}

		struct Measure {
			std::vector<MeshAnalysis::StrainStats> stats;
			size_t stretched = 0;
			float maxStrain = 0.0f;
		};
		auto measure = [&](const MeshAnalysis::DenseWeights& weights) {
			Measure m;
			m.stats = PoseStrain(skinner, poses.size(), verts, tris, weights, threshold);
			for (auto& s : m.stats) {
				m.stretched += s.stretched;
				m.maxStrain = std::max(m.maxStrain, s.maxStrain);
			}
			return m;
		};

		const Measure before = measure(current);
		Measure now = before;
		json steps = json::array();
		std::string stopped;
		float radius = startRadius;
		for (; radius <= maxRadius + 1e-4f; radius += radiusStep) {
			// The vertices stretched more than the body in any pose, grown by rings and with a soft border, are what's
			// smoothed
			std::vector<bool> selected(verts.size(), false);
			size_t found = 0;
			for (size_t i = 0; i < poses.size(); i++)
				for (size_t v = 0; v < now.stats[i].vertexStrain.size(); v++)
					if (now.stats[i].vertexStrain[v] > poseThreshold[i] && allowed[v] > 0.0f && !selected[v]) {
						selected[v] = true;
						found++;
					}
			if (found == 0) {
				stopped = "nothing stretched more than the body";
				break;
			}
			const std::vector<bool> picked = pickBones(selected);
			GrowSelection(selected, neighbors, grow);

			std::vector<float> influence(verts.size(), 0.0f);
			for (size_t v = 0; v < verts.size(); v++)
				influence[v] = selected[v] ? 1.0f : 0.0f;
			for (int i = 0; i < feather; i++) {
				std::vector<float> next = influence;
				for (size_t v = 0; v < verts.size(); v++) {
					if (selected[v] || neighbors[v].empty())
						continue;
					float sum = 0.0f;
					for (int n : neighbors[v])
						sum += influence[n];
					next[v] = influence[v] + (sum / neighbors[v].size() - influence[v]) * 0.5f;
				}
				influence.swap(next);
			}
			for (size_t v = 0; v < verts.size(); v++)
				influence[v] *= allowed[v];

			const int iterations = SmoothIterationsForRadius(radius, meanEdge, strength);
			auto candidate = MeshAnalysis::SmoothWeights(current, neighbors, influence, iterations, strength, &picked);
			for (size_t v = 0; v < candidate.size(); v++)
				if (influence[v] > 0.0f)
					MeshAnalysis::LimitWeights(candidate[v], maxInfluences, pruneBelow);

			Measure after = measure(candidate);
			json step = {
				{"radius", radius},
				{"iterations", iterations},
				{"bones", boneNames(picked)},
				{"smoothedVertices", std::count(selected.begin(), selected.end(), true)},
				{"stretchedEdges", after.stretched},
				{"maxStrain", after.maxStrain},
			};

			// Better is lower peaks over the poses together, without any pose's peak getting worse. Spreading a tear
			// over more edges is the point, so the stretched edges may grow.
			double peaksBefore = 0.0, peaksAfter = 0.0;
			bool poseWorse = false;
			json worse = json::object();
			for (size_t i = 0; i < poses.size(); i++) {
				peaksBefore += now.stats[i].maxStrain;
				peaksAfter += after.stats[i].maxStrain;
				// A pose may get a little worse for the others to get better, but not past the body's strain by much
				const float limit = std::max(poseThreshold[i], now.stats[i].maxStrain) * (1.0f + maxPoseIncrease);
				if (after.stats[i].maxStrain > limit) {
					poseWorse = true;
					worse[poses[i]] = {now.stats[i].maxStrain, after.stats[i].maxStrain};
				}
			}
			if (!worse.empty())
				step["worsePoses"] = worse;
			// A small radius can be worse where a larger one helps: half blending two bones that turn apart pulls the
			// vertices together, while a wider gradient shares the turn over many rings. So a step that doesn't help
			// is taken back and the next radius tried from the same weights.
			if (poseWorse || peaksAfter >= peaksBefore * 0.99) {
				step["reverted"] = true;
				steps.push_back(step);
				continue;
			}
			steps.push_back(step);
			current = std::move(candidate);
			now = std::move(after);
		}
		if (stopped.empty())
			stopped = "maxRadius";

		size_t changed = ApplyWeights(mcp, shape, DenseWeightChanges(start, current, bones));

		json perPose = json::array();
		for (size_t i = 0; i < poses.size(); i++) {
			json entry = {{"pose", poses[i]}, {"before", StrainToJson(before.stats[i])}, {"after", StrainToJson(now.stats[i])}};
			if (referencePeaks.contains(poses[i]))
				entry["referenceMaxStrain"] = referencePeaks[poses[i]];
			perPose.push_back(entry);
		}

		result.data["shapes"][shapeName] = {
			{"before", {{"stretchedEdges", before.stretched}, {"maxStrain", before.maxStrain}}},
			{"after", {{"stretchedEdges", now.stretched}, {"maxStrain", now.maxStrain}}},
			{"changedWeights", changed},
			{"keptClose", keptClose},
			{"steps", steps},
			{"stopped", stopped},
			{"poses", perPose},
		};
	}
	result.data["threshold"] = threshold;
	return result;
}

struct Findings {
	json errors = json::array();
	json warnings = json::array();
	json info = json::array();

	void Add(json& list, const std::string& shape, const std::string& check, const std::string& message, json extra = json::object()) {
		json entry = {{"check", check}, {"message", message}};
		if (!shape.empty())
			entry["shape"] = shape;
		for (auto& item : extra.items())
			entry[item.key()] = item.value();
		list.push_back(entry);
	}
};

json Sample(const std::vector<int>& items, size_t count = 20) {
	json list = json::array();
	for (size_t i = 0; i < items.size() && i < count; i++)
		list.push_back(items[i]);
	return list;
}

McpToolResult ValidateProject(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NifFile* nif = project.GetWorkNif();
	NiShape* reference = project.GetBaseShape();
	const auto game = static_cast<TargetGame>(Config.GetIntValue("TargetGame"));
	const int influenceLimit = McpArgs::Int(args, "influenceLimit", game == SF ? 8 : 4);
	const bool usesPartitions = game == SKYRIM || game == SKYRIMSE || game == SKYRIMVR;
	const bool usesSegments = game == FO4 || game == FO4VR || game == FO76;

	std::vector<NiShape*> shapes;
	if (McpArgs::Has(args, "shapes"))
		shapes = mcp.Shapes(args, "shapes");
	else
		for (auto* shape : nif->GetShapes())
			if (!project.IsBaseShape(shape))
				shapes.push_back(shape);

	Findings f;

	// Shape names have to be unique, BodySlide finds shapes and their data by name
	std::map<std::string, int> names;
	for (auto* shape : nif->GetShapes())
		names[shape->name.get()]++;
	for (auto& n : names)
		if (n.second > 1)
			f.Add(f.errors, n.first, "duplicateName", "There are " + std::to_string(n.second) + " shapes with this name. Rename them so each is unique.");

	// Whether the body is saved with the outfit, see save_project copyReference
	if (reference) {
		if (project.mCopyRef)
			f.Add(f.info, reference->name.get(), "referenceShape",
				  "The reference shape is saved and built with the outfit (copyReference). Its zap sliders hide the body under the outfit. "
				  "Save with copyReference: false if the outfit shouldn't include the body.");
		else
			f.Add(f.info, reference->name.get(), "referenceShape",
				  "The reference shape isn't saved with the outfit (copyReference false). If the original project included the body, save with copyReference: true.");
	}

	for (auto* shape : shapes) {
		const std::string name = shape->name.get();
		std::vector<Vector3> verts = RestVerts(project, shape);
		std::vector<Triangle> tris = Triangles(shape);
		std::vector<Vector2> uvs;
		nif->GetUvsForShape(shape, uvs);

		if (verts.size() > 65535)
			f.Add(f.errors, name, "vertexLimit", "The shape has " + std::to_string(verts.size()) + " vertices, more than the 65535 a shape can have.");
		if (tris.empty())
			f.Add(f.errors, name, "noTriangles", "The shape has no triangles.");

		// Broken numbers make the whole shape disappear or stretch in game
		std::vector<int> badPositions, badUVs;
		for (size_t v = 0; v < verts.size(); v++)
			if (!std::isfinite(verts[v].x) || !std::isfinite(verts[v].y) || !std::isfinite(verts[v].z))
				badPositions.push_back(static_cast<int>(v));
		for (size_t v = 0; v < uvs.size(); v++)
			if (!std::isfinite(uvs[v].u) || !std::isfinite(uvs[v].v))
				badUVs.push_back(static_cast<int>(v));
		if (!badPositions.empty())
			f.Add(f.errors, name, "invalidPositions", "Vertices with invalid positions (NaN or infinite).", {{"count", badPositions.size()}, {"vertices", Sample(badPositions)}});
		if (!badUVs.empty())
			f.Add(f.errors, name, "invalidUVs", "Vertices with invalid UVs (NaN or infinite).", {{"count", badUVs.size()}, {"vertices", Sample(badUVs)}});

		std::vector<int> degenerate;
		std::vector<bool> used(verts.size(), false);
		for (size_t t = 0; t < tris.size(); t++) {
			const Triangle& tri = tris[t];
			if (tri.p1 >= verts.size() || tri.p2 >= verts.size() || tri.p3 >= verts.size()) {
				degenerate.push_back(static_cast<int>(t));
				continue;
			}
			used[tri.p1] = used[tri.p2] = used[tri.p3] = true;
			Vector3 n = (verts[tri.p2] - verts[tri.p1]).cross(verts[tri.p3] - verts[tri.p1]);
			if (tri.p1 == tri.p2 || tri.p2 == tri.p3 || tri.p1 == tri.p3 || n.length() < 1e-10f)
				degenerate.push_back(static_cast<int>(t));
		}
		if (!degenerate.empty())
			f.Add(f.warnings, name, "degenerateTriangles", "Triangles without area. They can shade wrong; edit_topology collapse_vertex or delete_vertices removes them.",
				  {{"count", degenerate.size()}, {"triangles", Sample(degenerate)}});

		std::vector<int> unused;
		for (size_t v = 0; v < used.size(); v++)
			if (!used[v])
				unused.push_back(static_cast<int>(v));
		if (!unused.empty())
			f.Add(f.info, name, "unusedVertices", "Vertices that are in no triangle.", {{"count", unused.size()}, {"vertices", Sample(unused)}});

		NiShader* shader = nif->GetShader(shape);
		auto* bsShader = dynamic_cast<BSShaderProperty*>(shader);
		if (!shader)
			f.Add(f.warnings, name, "noShader", "The shape has no shader, it may not show in game.");

		if (shape->IsSkinned()) {
			MeshAnalysis::WeightStats stats;
			mcp.Apply(mcp.Ops().AnalyzeWeights(shape, stats, influenceLimit));
			if (stats.unweighted)
				f.Add(f.errors, name, "unweighted", "Vertices without bone weights stay behind when the body moves. copy_bone_weights or set_weights fixes them.",
					  {{"count", stats.unweighted}});
			if (stats.overInfluenceLimit)
				f.Add(f.warnings, name, "influenceLimit",
					  "Vertices weighted to more than " + std::to_string(influenceLimit) + " bones; the game drops the extra ones. clean_weights with maxInfluences fixes them.",
					  {{"count", stats.overInfluenceLimit}});
			if (stats.badSum)
				f.Add(f.warnings, name, "weightSum", "Vertices whose weights don't add up to 1. clean_weights normalizes them.", {{"count", stats.badSum}, {"largestError", stats.maxSumError}});

			if (bsShader && game != SF && !bsShader->IsSkinned())
				f.Add(f.warnings, name, "shaderNotSkinned", "The shape is skinned, but its shader doesn't have the Skinned flag.");

			if (usesPartitions) {
				NiVector<BSDismemberSkinInstance::PartitionInfo> partitions;
				std::vector<int> triParts;
				if (!nif->GetShapePartitions(shape, partitions, triParts) || partitions.empty())
					f.Add(f.errors, name, "noPartitions", "The skinned shape has no partitions. copy_partitions or set_partitions adds them.");
				else {
					std::vector<int> unassigned;
					for (size_t t = 0; t < triParts.size(); t++)
						if (triParts[t] < 0)
							unassigned.push_back(static_cast<int>(t));
					if (!unassigned.empty())
						f.Add(f.errors, name, "unpartitioned", "Triangles in no partition.", {{"count", unassigned.size()}, {"triangles", Sample(unassigned)}});
				}
			}
			else if (usesSegments) {
				NifSegmentationInfo segments;
				std::vector<int> triParts;
				if (!NifFile::GetShapeSegments(shape, segments, triParts) || segments.segs.empty())
					f.Add(f.warnings, name, "noSegments", "The skinned shape has no segments. copy_partitions or set_segments adds them.");
				else {
					std::vector<int> unassigned;
					for (size_t t = 0; t < triParts.size(); t++)
						if (triParts[t] < 0)
							unassigned.push_back(static_cast<int>(t));
					if (!unassigned.empty())
						f.Add(f.errors, name, "unsegmented", "Triangles in no segment.", {{"count", unassigned.size()}, {"triangles", Sample(unassigned)}});
				}
			}

			if (!shape->GetTransformToParent().IsNearlyEqualTo(MatTransform()))
				f.Add(f.info, name, "shapeTransform", "The skinned shape has its own transform. shape_transforms apply bakes it into the vertices.");

			// SMP adds the bones a simulated shape is weighted to that its XML doesn't declare with the default bone
			// template. With a mass above 0 there, they're dynamic and fall, e.g. body bones a weight copy added.
			std::vector<std::string> missingXmls;
			for (auto& file : project.ReadPhysicsXmls(name, &missingXmls)) {
				const PhysicsXmlFile::Shape* simulated = file.second.FindShape(name);
				if (!simulated || simulated->implicitMass <= 0.0f)
					continue;

				std::vector<std::string> dynamic;
				for (auto& bone : project.GetWorkAnim()->shapeBones[name]) {
					auto weights = project.GetWorkAnim()->GetWeightsPtr(name, bone);
					if (weights && !weights->empty() && !simulated->Declares(bone))
						dynamic.push_back(bone);
				}
				if (!dynamic.empty())
					f.Add(f.warnings, name, "undeclaredPhysicsBones",
						  "The SMP physics XML simulates this shape but doesn't declare these bones, so SMP makes them dynamic with its default "
						  "bone template (mass above 0) and they fall in game. Unless the original had them, remove their weights "
						  "(reassign_weights, delete_bones) or copy weights again with addBones false.",
						  {{"xml", file.first}, {"bones", dynamic}, {"defaultMass", simulated->implicitMass}});
			}
			if (!missingXmls.empty())
				f.Add(f.info, name, "physicsXmlNotFound", "The shape links SMP physics XML files that weren't found, so its physics bones weren't checked.", {{"files", missingXmls}});
		}
		else if (reference && reference->IsSkinned()) {
			f.Add(f.warnings, name, "notSkinned", "The shape isn't skinned, so it won't move with the body.");
		}

		// Textures, where the shader takes them from the NIF
		MaterialFile materialFile;
		if (bsShader && game != SF && !project.GetShapeMaterialFile(shape, materialFile)) {
			auto textures = nif->GetTexturePathRefs(shape);
			if (!textures.empty() && textures[0].get().empty())
				f.Add(f.warnings, name, "noDiffuse", "The shape has no diffuse texture (slot 0).");
		}

		// Sliders that move the reference under this shape but not the shape. Sliders that only move the body
		// elsewhere (the hips for a top) don't count.
		if (reference) {
			auto refVerts = RestVerts(project, reference);
			auto refTris = Triangles(reference);
			std::set<uint16_t> under;
			for (auto& sp : MeshAnalysis::ClosestSurfacePoints(refVerts, refTris, verts, 3.0f)) {
				if (sp.triangle < 0)
					continue;
				const Triangle& t = refTris[sp.triangle];
				under.insert(t.p1);
				under.insert(t.p2);
				under.insert(t.p3);
			}

			std::vector<std::string> missing;
			for (size_t i = 0; i < project.SliderCount(); i++) {
				if (project.SliderZap(i) || project.SliderUV(i))
					continue;
				TargetDataDiffs* diff = project.GetDiffSet(project.activeSet[i], shape);
				if (diff && !diff->empty())
					continue;

				std::vector<Vector3> refDeltas(refVerts.size());
				project.GetSliderDiff(reference, project.GetSliderName(i), refDeltas);
				bool movesUnder = false;
				for (uint16_t v : under)
					if (v < refDeltas.size() && refDeltas[v].length() > 0.01f) {
						movesUnder = true;
						break;
					}
				if (movesUnder)
					missing.push_back(project.GetSliderName(i));
			}
			if (!missing.empty()) {
				json list = json::array();
				for (size_t i = 0; i < missing.size() && i < 30; i++)
					list.push_back(missing[i]);
				f.Add(f.warnings, name, "missingMorphs", "Sliders that move the reference under this shape but not the shape. conform_sliders adds them, or zero them out on purpose for rigid parts.",
					  {{"count", missing.size()}, {"sliders", list}});
			}

			// Measured like analyze_clipping: with the body poking through, without what zaps hide or exclusions
			McpSliderValueScope values(project);
			values.Zero();
			MeshAnalysis::PenetrationStats clipping;
			clipping.signedDistances = McpSignedDistances(mcp, shape, reference);
			const size_t excluded = McpApplyExclusions(mcp, shape, clipping, 0.1f);
			if (clipping.penetrating) {
				json details = {{"count", clipping.penetrating}, {"maxDepth", clipping.maxDepth}};
				if (excluded)
					details["excluded"] = excluded;
				size_t hidden = 0;
				mcp.SurfaceTriangles(reference, &hidden);
				if (hidden)
					details["hiddenByZaps"] = mcp.ActiveAnalysisZaps();
				f.Add(f.info, name, "clipping", "Vertices inside the reference with all sliders at 0.", details);
			}
		}
	}

	// Zap sliders that zap nothing, e.g. a body zap whose data stayed on a reference that was replaced
	std::vector<std::string> emptyZaps;
	for (size_t i = 0; i < project.SliderCount(); i++) {
		if (!project.SliderZap(i))
			continue;
		bool hasData = false;
		for (auto* shape : nif->GetShapes()) {
			std::vector<bool> zapped = project.GetZappedVertices(shape, project.GetSliderName(i));
			if (std::find(zapped.begin(), zapped.end(), true) != zapped.end()) {
				hasData = true;
				break;
			}
		}
		if (!hasData)
			emptyZaps.push_back(project.GetSliderName(i));
	}
	if (!emptyZaps.empty())
		f.Add(f.warnings, "", "emptyZaps",
			  "Zap sliders that zap nothing. If they hid the body of the original project, transfer_zaps gets them from its body.",
			  {{"sliders", emptyZaps}});

	BadBonesReport bad = project.FindBadBones();
	if (bad.found)
		f.Add(f.warnings, "", "badBones", "Bones whose transforms don't match the skeleton. fix_bad_bones fixes them.");

	if (frame->HasPendingChanges())
		f.Add(f.info, "", "unsaved", "The project has changes that aren't saved.");

	McpToolResult result;
	result.data["ok"] = f.errors.empty();
	result.data["errors"] = f.errors;
	result.data["warnings"] = f.warnings;
	result.data["info"] = f.info;
	result.data["shapes"] = shapes.size();
	return result;
}

// For each vertex of a mesh, whether it lies within distance of an open edge (a hole like the neck, wrists or
// ankles of a body). Vertices at the same place are welded first, so split UV seams don't count as edges.
std::vector<bool> NearOpenEdges(const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, float distance) {
	std::vector<int> rep(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		rep[v] = static_cast<int>(v);
	for (auto& group : MeshAnalysis::CoincidentGroups(verts, SeamTolerance))
		for (int v : group)
			rep[v] = group.front();

	std::map<std::pair<int, int>, int> edges;
	for (auto& t : tris) {
		const uint16_t p[3] = {t.p1, t.p2, t.p3};
		if (p[0] >= verts.size() || p[1] >= verts.size() || p[2] >= verts.size())
			continue;
		for (int i = 0; i < 3; i++) {
			int a = rep[p[i]], b = rep[p[(i + 1) % 3]];
			if (a != b)
				edges[{std::min(a, b), std::max(a, b)}]++;
		}
	}

	std::set<int> boundary;
	for (auto& e : edges) {
		if (e.second == 1) {
			boundary.insert(e.first.first);
			boundary.insert(e.first.second);
		}
	}

	std::vector<bool> closeToEdge(verts.size(), false);
	if (boundary.empty())
		return closeToEdge;

	for (size_t v = 0; v < verts.size(); v++)
		for (int b : boundary)
			if (verts[v].DistanceTo(verts[b]) <= distance) {
				closeToEdge[v] = true;
				break;
			}
	return closeToEdge;
}

McpToolResult AnalysisExclusions(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	const std::string op = McpArgs::String(args, "op");
	const bool replace = McpArgs::Bool(args, "replace", false);
	const int grow = std::clamp(McpArgs::Int(args, "grow", op == "deep_at_rest" ? 2 : 0), 0, 50);
	auto& all = mcp.Exclusions();

	McpToolResult result;
	if (op == "zaps") {
		// Which zaps hide the reference where clipping is measured: the given ones, or BodySlide's defaults again
		if (McpArgs::Has(args, "zaps")) {
			std::vector<std::string> zaps = McpArgs::Strings(args, "zaps");
			for (auto& zap : zaps) {
				size_t index = 0;
				if (!project.SliderIndexFromName(zap, index) || !project.SliderZap(index))
					throw McpToolError("'" + zap + "' isn't a zap slider.");
			}
			mcp.AnalysisZaps() = zaps;
		}
		else
			mcp.AnalysisZaps().reset();

		result.data["zaps"] = mcp.ActiveAnalysisZaps();
		if (NiShape* reference = project.GetBaseShape()) {
			size_t hidden = 0;
			mcp.SurfaceTriangles(reference, &hidden);
			result.data["hiddenTriangles"] = hidden;
		}
		return result;
	}

	for (auto* shape : mcp.Shapes(args, "shapes")) {
		const std::string name = shape->name.get();
		const size_t count = shape->GetNumVertices();

		if (op == "clear") {
			all.erase(name);
			result.data["shapes"][name] = {{"excluded", 0}};
			continue;
		}

		if (op == "show") {
			// Puts the exclusions into the mask to look at them with render_view display.mask
			const std::vector<bool>* excluded = mcp.Excluded(shape);
			Mesh* m = ShapeMesh(mcp, shape);
			frame->glView->ApplyMaskChanges({m}, [&](Mesh& mesh) {
				std::unordered_map<int, float> changes;
				for (int v = 0; v < mesh.nVerts; v++)
					changes[v] = excluded && v < static_cast<int>(excluded->size()) && (*excluded)[v] ? 1.0f : 0.0f;
				return changes;
			});
			frame->glView->Render();
			result.data["shapes"][name] = {{"excluded", excluded ? std::count(excluded->begin(), excluded->end(), true) : 0}};
			continue;
		}

		std::vector<bool> selected(count, false);
		if (op == "from_mask") {
			Mesh* m = ShapeMesh(mcp, shape);
			for (int v = 0; v < m->nVerts && v < static_cast<int>(count); v++)
				selected[v] = m->mask && m->mask[v] >= 0.5f;
		}
		else if (op == "vertices") {
			for (int v : McpArgs::Ints(args, "vertices")) {
				if (v < 0 || v >= static_cast<int>(count))
					throw McpToolError("Vertex " + std::to_string(v) + " doesn't exist on '" + name + "'.");
				selected[v] = true;
			}
		}
		else if (op == "deep_at_rest") {
			// Inside the body by design: deeper than real clipping usually goes, with all sliders at 0
			NiShape* against = McpAgainstShape(mcp, args, shape);
			const float minDepth = McpArgs::Float(args, "minDepth", 1.0f);
			McpSliderValueScope sliders(project);
			sliders.Zero();
			std::vector<Vector3> verts, surface;
			project.GetMorphedVerts(shape, verts);
			project.GetMorphedVerts(against, surface);
			auto distances = ClippingFixer::SignedDistancesToBody(surface, Triangles(against), verts, 5.0f);
			for (size_t v = 0; v < count && v < distances.size(); v++)
				selected[v] = distances[v] != FLT_MAX && distances[v] < -minDepth;
		}
		else if (op == "reference_edges") {
			// Where the body ends (neck, wrists, ankles), inside and outside can't be told apart
			NiShape* against = McpAgainstShape(mcp, args, shape);
			auto surface = RestVerts(project, against);
			auto surfaceTris = Triangles(against);
			auto nearEdge = NearOpenEdges(surface, surfaceTris, McpArgs::Float(args, "edgeDistance", 1.5f));
			auto closest = MeshAnalysis::ClosestSurfacePoints(surface, surfaceTris, RestVerts(project, shape), 5.0f);
			for (size_t v = 0; v < count && v < closest.size(); v++) {
				if (closest[v].triangle < 0)
					continue;
				const Triangle& t = surfaceTris[closest[v].triangle];
				selected[v] = nearEdge[t.p1] || nearEdge[t.p2] || nearEdge[t.p3];
			}
		}
		else
			McpArgs::Wrong("op", "from_mask, vertices, deep_at_rest, reference_edges, show or clear");

		if (grow > 0)
			GrowSelection(selected, VertexNeighbors(RestVerts(project, shape), Triangles(shape)), grow);

		const size_t found = static_cast<size_t>(std::count(selected.begin(), selected.end(), true));
		const std::vector<bool>* existing = mcp.Excluded(shape);
		if (!replace && existing)
			for (size_t v = 0; v < count; v++)
				selected[v] = selected[v] || (*existing)[v];

		const size_t total = static_cast<size_t>(std::count(selected.begin(), selected.end(), true));
		if (total == 0)
			all.erase(name);
		else
			all[name] = std::move(selected);
		result.data["shapes"][name] = {{"found", found}, {"excluded", total}, {"vertices", count}};
	}
	return result;
}

const json ShapesProp = McpStringList("Outfit shapes, the selected ones by default");

} // namespace

McpSliderValueScope::McpSliderValueScope(OutfitProject& inProject)
	: project(inProject) {
	for (size_t i = 0; i < project.SliderCount(); i++)
		saved.push_back(project.SliderValue(i));
}

McpSliderValueScope::~McpSliderValueScope() {
	for (size_t i = 0; i < saved.size() && i < project.SliderCount(); i++)
		project.SliderValue(i) = saved[i];
}

void McpSliderValueScope::Zero() {
	for (size_t i = 0; i < project.SliderCount(); i++)
		project.SliderValue(i) = 0.0f;
}

void McpSliderValueScope::Set(const std::map<std::string, float>& values) {
	for (auto& value : values) {
		if (!project.ValidSlider(value.first))
			throw McpToolError("There is no slider '" + value.first + "'.");
		project.SliderValue(value.first) = value.second;
	}
}

McpPoseScope::McpPoseScope(OutfitStudioMcp& inMcp)
	: mcp(inMcp)
	, saved(std::make_unique<PoseData>()) {
	wasPosed = mcp.Project().bPose;
	PoseDataCollection::CaptureCurrentPose("MCP", false, *saved);
}

McpPoseScope::~McpPoseScope() {
	Restore();
}

void McpPoseScope::Restore() {
	OutfitStudioFrame* frame = mcp.GetFrame();
	if (!frame->project)
		return;

	ProjectOperations(*frame->project, frame->editor).ResetPose();
	saved->ApplyToSkeleton();
	frame->project->bPose = wasPosed;
}

void McpPoseScope::Set(const std::string& pose) {
	OutfitProject& project = mcp.Project();
	if (pose == "none") {
		project.bPose = false;
		return;
	}

	auto& poses = mcp.GetFrame()->poseDataCollection.poseData;
	auto it = std::find_if(poses.begin(), poses.end(), [&](const PoseData& p) { return p.name == pose; });
	if (it == poses.end())
		throw McpToolError("There is no pose '" + pose + "'. list_poses lists them.");

	mcp.Ops().ResetPose();
	it->ApplyToSkeleton();
	project.bPose = true;
}

NiShape* McpAgainstShape(OutfitStudioMcp& mcp, const json& args, NiShape* shape) {
	NiShape* against = nullptr;
	if (McpArgs::Has(args, "against"))
		against = mcp.Shape(McpArgs::String(args, "against"));
	else
		against = mcp.Project().GetBaseShape();

	if (!against)
		throw McpToolError("No reference shape is loaded. Load one, or name the shape to measure against in 'against'.");
	if (against == shape)
		throw McpToolError("'" + shape->name.get() + "' can't be measured against itself.");
	return against;
}

std::vector<float> McpSignedDistances(OutfitStudioMcp& mcp, NiShape* shape, NiShape* against, float searchRadius) {
	OutfitProject& project = mcp.Project();
	std::vector<Vector3> verts, surfaceVerts;
	project.GetLiveVerts(shape, verts);
	project.GetLiveVerts(against, surfaceVerts);
	auto distances = ClippingFixer::SignedDistancesToBody(surfaceVerts, Triangles(against), verts, searchRadius);
	McpDropHiddenSurface(mcp, against, surfaceVerts, verts, distances);
	McpMergePokeThrough(mcp, shape, verts, against, surfaceVerts, distances);

	// Excluded vertices count as having no surface nearby, like parts far from the body
	if (auto* excluded = mcp.Excluded(shape))
		for (size_t v = 0; v < distances.size() && v < excluded->size(); v++)
			if ((*excluded)[v])
				distances[v] = FLT_MAX;
	return distances;
}

void McpDropHiddenSurface(OutfitStudioMcp& mcp, NiShape* against, const std::vector<Vector3>& surfaceVerts, const std::vector<Vector3>& verts, std::vector<float>& distances) {
	std::vector<Triangle> tris = Triangles(against);
	std::vector<bool> hidden = mcp.HiddenSurfaceTriangles(against, tris);
	if (std::find(hidden.begin(), hidden.end(), true) == hidden.end())
		return;

	auto closest = MeshAnalysis::ClosestSurfacePoints(surfaceVerts, tris, verts, 5.0f);
	for (size_t v = 0; v < closest.size() && v < distances.size(); v++)
		if (closest[v].triangle >= 0 && hidden[closest[v].triangle])
			distances[v] = FLT_MAX;
}

McpPokeStats McpMergePokeThrough(OutfitStudioMcp& mcp,
								 NiShape* shape,
								 const std::vector<Vector3>& verts,
								 NiShape* against,
								 const std::vector<Vector3>& surfaceVerts,
								 std::vector<float>& distances) {
	McpPokeStats stats;
	std::vector<Triangle> tris = Triangles(shape);
	// Pokes deeper than this are the outfit's own parts inside the body, which the vertices show already
	constexpr float maxPokeDepth = 1.0f;
	auto pokes = ClippingFixer::BodyPokeThrough(surfaceVerts, mcp.SurfaceTriangles(against), verts, tris, maxPokeDepth);

	const std::vector<bool>* excluded = mcp.Excluded(shape);
	for (auto& poke : pokes) {
		if (poke.triangle < 0 || static_cast<size_t>(poke.triangle) >= tris.size())
			continue;

		const Triangle& tri = tris[poke.triangle];
		bool skip = false;
		for (uint16_t v : {tri.p1, tri.p2, tri.p3})
			skip = skip || v >= distances.size() || (excluded && v < excluded->size() && (*excluded)[v]);
		if (skip)
			continue;

		stats.bodyVertices++;
		stats.maxDepth = std::max(stats.maxDepth, poke.depth);
		for (uint16_t v : {tri.p1, tri.p2, tri.p3})
			distances[v] = std::min(distances[v], -poke.depth);
	}
	return stats;
}

json McpMaskSummary(const Mesh& m) {
	auto masked = MaskOps::GetMasked(m);
	size_t full = 0;
	for (auto& v : masked)
		if (v.second >= 1.0f)
			full++;
	// masked counts any mask value, so after feathering it looks like nearly everything is masked
	const size_t count = static_cast<size_t>(m.nVerts);
	return {{"vertices", count}, {"masked", masked.size()}, {"fullyMasked", full}, {"partlyMasked", masked.size() - full}, {"unmasked", count - masked.size()}};
}

size_t McpApplyExclusions(OutfitStudioMcp& mcp, NiShape* shape, MeshAnalysis::PenetrationStats& stats, float nearDistance) {
	const std::vector<bool>* excluded = mcp.Excluded(shape);

	size_t count = 0;
	for (size_t v = 0; excluded && v < stats.signedDistances.size() && v < excluded->size(); v++) {
		if ((*excluded)[v]) {
			stats.signedDistances[v] = FLT_MAX;
			count++;
		}
	}

	stats.penetrating = 0;
	stats.nearSurface = 0;
	stats.maxDepth = 0.0f;
	double depthSum = 0.0;
	for (float d : stats.signedDistances) {
		if (d == FLT_MAX)
			continue;
		if (d < 0.0f) {
			stats.penetrating++;
			stats.maxDepth = std::max(stats.maxDepth, -d);
			depthSum += -d;
		}
		else if (d < nearDistance)
			stats.nearSurface++;
	}
	stats.meanDepth = stats.penetrating ? static_cast<float>(depthSum / stats.penetrating) : 0.0f;
	return count;
}

McpExclusionMaskScope::McpExclusionMaskScope(OutfitStudioMcp& inMcp, const std::vector<NiShape*>& shapes)
	: mcp(inMcp) {
	for (auto* shape : shapes) {
		const std::vector<bool>* excluded = mcp.Excluded(shape);
		Mesh* m = excluded ? mcp.Project().GetHost().GetShapeMesh(shape->name.get()) : nullptr;
		if (!m || !m->mask)
			continue;

		std::unordered_map<int, float> old;
		for (int v = 0; v < m->nVerts && v < static_cast<int>(excluded->size()); v++) {
			if ((*excluded)[v]) {
				old[v] = m->mask[v];
				m->mask[v] = 1.0f;
			}
		}
		saved.emplace_back(shape->name.get(), std::move(old));
	}
}

McpExclusionMaskScope::~McpExclusionMaskScope() {
	OutfitStudioFrame* frame = mcp.GetFrame();
	if (!frame->project)
		return;

	for (auto& s : saved) {
		Mesh* m = frame->project->GetHost().GetShapeMesh(s.first);
		if (!m || !m->mask)
			continue;
		for (auto& v : s.second)
			if (v.first < m->nVerts)
				m->mask[v.first] = v.second;
	}
}

json McpGapStats(const std::vector<float>& before, const std::vector<float>& after, float maxDistance, float threshold, double* outScore) {
	size_t compared = 0, tightened = 0, loosened = 0, newlyInside = 0;
	float maxTighten = 0.0f, maxLoosen = 0.0f;
	double sum = 0.0;
	for (size_t v = 0; v < before.size() && v < after.size(); v++) {
		if (before[v] == FLT_MAX || after[v] == FLT_MAX || before[v] > maxDistance)
			continue;
		compared++;
		const float change = after[v] - before[v];
		sum += std::fabs(change);
		if (change < -threshold)
			tightened++;
		else if (change > threshold)
			loosened++;
		maxTighten = std::max(maxTighten, -change);
		maxLoosen = std::max(maxLoosen, change);
		if (before[v] >= 0.0f && after[v] < 0.0f)
			newlyInside++;
	}

	if (outScore)
		*outScore = static_cast<double>(tightened + loosened) + std::max(maxTighten - threshold, 0.0f) * 10.0 + std::max(maxLoosen - threshold, 0.0f) * 5.0;

	return {
		{"compared", compared},
		{"tightened", tightened},
		{"loosened", loosened},
		{"maxTighten", maxTighten},
		{"maxLoosen", maxLoosen},
		{"meanChange", compared ? sum / compared : 0.0},
		{"newlyInside", newlyInside},
	};
}

const std::vector<std::string>& McpMetricNames() {
	static const std::vector<std::string> names = {"clipping", "roughness", "flipped", "weight_deviation", "gap_change", "seam_gap", "pose_stretch", "weight_jump"};
	return names;
}

json McpMetricProperties() {
	json properties = {
		{"metric",
		 {{"type", "string"},
		  {"enum", McpMetricNames()},
		  {"description",
		   "clipping: depth inside the reference (or 'against') in NIF units, negative outside. roughness: how much a vertex's "
		   "slider delta differs from its neighbors'. flipped: 1 on triangles that turn over at the slider value. "
		   "weight_deviation: 0 to 1, how different the bone weights are from the body under the vertex. gap_change: how much "
		   "the distance to the body changes with the slider, positive looser, negative tighter. seam_gap: how far the vertex "
		   "drifts with the slider from vertices at the same place (split seams, other shapes, the reference). pose_stretch: "
		   "how much the edges at the vertex stretch or shrink in the pose, 1 for double or half the length (weights that "
		   "change abruptly tear there). weight_jump: 0 to 1, the largest difference of the vertex's weights to a "
		   "neighbor's."}}},
		{"slider", McpProp("string", "roughness, flipped, gap_change, seam_gap: the slider")},
		{"pose", McpProp("string", "pose_stretch: the pose of list_poses")},
		{"value", McpProp("number", "flipped, gap_change, seam_gap: slider value to measure at. Default 1.")},
		{"sliderValues", McpNumberMap("clipping: slider values to measure with, the others at 0. Default the current values.")},
		{"against", McpProp("string", "clipping, gap_change, weight_deviation: the shape to measure against, e.g. an inner layer of the outfit. Default the reference.")},
		{"maxDistance", McpProp("number", "gap_change: only vertices this close to the surface at 0 count. weight_deviation: how far to look for the body. Default 2.")},
	};
	properties.update(McpPresetValueProperties());
	return properties;
}

std::vector<float> McpVertexMetric(OutfitStudioMcp& mcp, NiShape* shape, const json& args, float* outThreshold) {
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();
	const std::string metric = McpArgs::String(args, "metric");
	const float value = McpArgs::Float(args, "value", 1.0f);
	const size_t count = shape->GetNumVertices();

	auto sliderArg = [&]() {
		std::string slider = McpArgs::String(args, "slider");
		CheckMorphSlider(project, slider);
		return slider;
	};

	std::vector<float> values(count, 0.0f);
	float threshold = 0.0f;

	if (metric == "clipping") {
		NiShape* against = McpAgainstShape(mcp, args, shape);
		McpSliderValueScope sliders(project);
		if (McpHasSliderValuesArg(args)) {
			sliders.Zero();
			sliders.Set(McpSliderValuesArg(mcp, args));
		}
		auto distances = McpSignedDistances(mcp, shape, against);
		for (size_t v = 0; v < count && v < distances.size(); v++)
			values[v] = distances[v] == FLT_MAX ? -FLT_MAX : -distances[v];
		threshold = 0.0f;
	}
	else if (metric == "roughness") {
		std::vector<Vector3> deltas;
		mcp.Apply(ops.GetSliderDeltas(shape, sliderArg(), deltas));
		values = MeshAnalysis::VertexRoughness(Triangles(shape), deltas);
		threshold = 0.05f;
	}
	else if (metric == "flipped") {
		std::vector<Vector3> deltas;
		mcp.Apply(ops.GetSliderDeltas(shape, sliderArg(), deltas));
		auto before = RestVerts(project, shape);
		auto after = before;
		for (size_t v = 0; v < after.size() && v < deltas.size(); v++)
			after[v] += deltas[v] * value;
		auto tris = Triangles(shape);
		for (size_t t : MeshAnalysis::FlippedTriangles(before, after, tris)) {
			values[tris[t].p1] = 1.0f;
			values[tris[t].p2] = 1.0f;
			values[tris[t].p3] = 1.0f;
		}
		threshold = 0.5f;
	}
	else if (metric == "weight_deviation") {
		NiShape* against = McpAgainstShape(mcp, args, shape);
		AnimInfo* anim = project.GetWorkAnim();
		auto closest = MeshAnalysis::ClosestSurfacePoints(RestVerts(project, against), Triangles(against), RestVerts(project, shape), McpArgs::Float(args, "maxDistance", 2.0f));
		auto deviation = MeshAnalysis::WeightDeviation(closest, Triangles(against), WeightMap(anim, against->name.get()), WeightMap(anim, shape->name.get()));
		for (size_t v = 0; v < count && v < deviation.size(); v++)
			values[v] = std::max(deviation[v], 0.0f);
		threshold = 0.25f;
	}
	else if (metric == "gap_change") {
		NiShape* against = McpAgainstShape(mcp, args, shape);
		std::string slider = sliderArg();
		const float maxDistance = McpArgs::Float(args, "maxDistance", 2.0f);

		McpSliderValueScope sliders(project);
		sliders.Zero();
		auto before = McpSignedDistances(mcp, shape, against);
		sliders.Set({{slider, value}});
		auto after = McpSignedDistances(mcp, shape, against);
		for (size_t v = 0; v < count && v < before.size() && v < after.size(); v++)
			if (before[v] != FLT_MAX && after[v] != FLT_MAX && before[v] <= maxDistance)
				values[v] = after[v] - before[v];
		threshold = 0.3f;
	}
	else if (metric == "seam_gap") {
		std::string slider = sliderArg();
		std::vector<NiShape*> shapes = project.GetWorkNif()->GetShapes();
		auto groups = SeamGroups(project, shapes, SeamTolerance);
		auto deltas = ShapeDeltas(mcp, shapes, slider);
		const std::string name = shape->name.get();
		for (auto& group : groups) {
			for (auto& m : group) {
				if (m.shape != name)
					continue;
				for (auto& other : group) {
					if (&other == &m)
						continue;
					float gap = deltas[name][m.vertex].DistanceTo(deltas[other.shape][other.vertex]) * std::fabs(value);
					values[m.vertex] = std::max(values[m.vertex], gap);
				}
			}
		}
		threshold = 0.01f;
	}
	else if (metric == "pose_stretch") {
		const std::string pose = McpArgs::String(args, "pose");
		McpSliderValueScope sliders(project);
		if (McpHasSliderValuesArg(args)) {
			sliders.Zero();
			sliders.Set(McpSliderValuesArg(mcp, args));
		}
		AnimInfo* anim = project.GetWorkAnim();
		auto bones = ShapeBones(anim, shape->name.get());
		auto verts = MorphedVerts(project, shape);
		PoseSkinner skinner(mcp, shape, bones, {pose});
		auto weights = DenseShapeWeights(anim, shape->name.get(), bones, verts.size());
		auto stats = MeshAnalysis::AnalyzeStrain(verts, skinner.Posed(0, verts, weights), Triangles(shape));
		for (size_t v = 0; v < count && v < stats.vertexStrain.size(); v++)
			values[v] = stats.vertexStrain[v];
		threshold = 0.5f;
	}
	else if (metric == "weight_jump") {
		AnimInfo* anim = project.GetWorkAnim();
		auto verts = RestVerts(project, shape);
		auto jump = MeshAnalysis::WeightJump(DenseShapeWeights(anim, shape->name.get(), ShapeBones(anim, shape->name.get()), verts.size()),
											 VertexNeighbors(verts, Triangles(shape)));
		for (size_t v = 0; v < count && v < jump.size(); v++)
			values[v] = jump[v];
		threshold = 0.5f;
	}
	else
		McpArgs::Wrong("metric", "one of the listed metrics");

	if (outThreshold)
		*outThreshold = threshold;
	return values;
}

// Mask from a measurement, for mask_ops from_analysis
McpToolResult McpMaskFromMetric(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	const std::string target = McpArgs::String(args, "target", "fix");
	if (target != "fix" && target != "protect")
		McpArgs::Wrong("target", "\"fix\" or \"protect\"");
	const int grow = std::clamp(McpArgs::Int(args, "grow", 1), 0, 50);
	const int feather = std::clamp(McpArgs::Int(args, "feather", 0), 0, 50);

	McpToolResult result;
	for (auto* shape : mcp.Shapes(args, "shapes")) {
		float threshold = 0.0f;
		std::vector<float> values = McpVertexMetric(mcp, shape, args, &threshold);
		threshold = McpArgs::Float(args, "threshold", threshold);
		const bool signedMetric = McpArgs::String(args, "metric") == "gap_change";

		std::vector<bool> found(values.size(), false);
		size_t foundCount = 0;
		for (size_t v = 0; v < values.size(); v++) {
			float measured = signedMetric ? std::fabs(values[v]) : values[v];
			if (measured > threshold) {
				found[v] = true;
				foundCount++;
			}
		}

		// Nothing to fix leaves the mask alone, a fully masked shape would only block the next step
		if (foundCount == 0) {
			json summary = McpMaskSummary(*ShapeMesh(mcp, shape));
			summary["found"] = 0;
			summary["threshold"] = threshold;
			summary["unchanged"] = true;
			result.data["shapes"][shape->name.get()] = summary;
			continue;
		}

		auto verts = RestVerts(project, shape);
		GrowSelection(found, VertexNeighbors(verts, Triangles(shape)), grow);

		// "fix" leaves only what was found unmasked, so fixes change nothing else; "protect" masks it. Excluded
		// vertices (analysis_exclusions) always stay masked.
		const std::vector<bool>* excluded = mcp.Excluded(shape);
		auto isExcluded = [excluded](int v) { return excluded && v < static_cast<int>(excluded->size()) && (*excluded)[v]; };

		Mesh* m = ShapeMesh(mcp, shape);
		frame->glView->ApplyMaskChanges({m}, [&](Mesh& mesh) {
			std::unordered_map<int, float> changes;
			for (int v = 0; v < mesh.nVerts && v < static_cast<int>(found.size()); v++)
				changes[v] = (found[v] == (target == "protect") || isExcluded(v)) ? 1.0f : 0.0f;
			return changes;
		});
		for (int i = 0; i < feather; i++)
			frame->glView->ApplyMaskChanges({m}, [](Mesh& mesh) { return MaskOps::SmoothChanges(mesh, 0.5f); });
		// The feather only spreads outwards: what was selected keeps its value, so a fix takes full effect there
		// instead of fading out over the very vertices it's for
		if (feather > 0)
			frame->glView->ApplyMaskChanges({m}, [&](Mesh& mesh) {
				std::unordered_map<int, float> changes;
				for (int v = 0; v < mesh.nVerts && v < static_cast<int>(found.size()); v++) {
					if (isExcluded(v))
						changes[v] = 1.0f;
					else if (found[v])
						changes[v] = target == "protect" ? 1.0f : 0.0f;
				}
				return changes;
			});

		size_t selected = 0;
		for (bool b : found)
			selected += b ? 1 : 0;

		json summary = McpMaskSummary(*ShapeMesh(mcp, shape));
		summary["found"] = foundCount;
		summary["selected"] = selected;
		summary["threshold"] = threshold;
		if (excluded)
			summary["excluded"] = std::count(excluded->begin(), excluded->end(), true);
		result.data["shapes"][shape->name.get()] = summary;
	}
	frame->glView->Render();
	return result;
}

void AddQualityTools(OutfitStudioMcp& mcp) {
	McpTool parts;
	parts.name = "list_mesh_parts";
	parts.description =
		"Lists the connected pieces of a shape (buttons, buckles, straps, plates, the main cloth), largest first, with their "
		"size, bounds, center, main bone and how much of them is masked. Copies of a vertex along split seams count as "
		"joined. mask_ops with op \"parts\" masks parts by their number, rigidify_slider keeps them from bending.";
	parts.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"minVertices", McpProp("integer", "Leave out smaller parts. Default 1.")},
			{"limit", McpProp("integer", "Most parts to list. Default 50.")},
			{"weldDistance", McpProp("number", "Vertices closer than this count as joined. Default 0.001.")},
		},
		{"shape"});
	parts.readOnly = true;
	parts.handler = [&mcp](const json& args) { return ListMeshParts(mcp, args); };
	mcp.AddTool(std::move(parts));

	McpTool rigidify;
	rigidify.name = "rigidify_slider";
	rigidify.description =
		"Makes the unmasked parts of shapes follow sliders as a whole, without bending: each connected part of the unmasked "
		"vertices gets the move and turn (and with allowScale an even scale) that best fits its morph. Use it for buckles, "
		"gems, buttons, armor plates and other rigid pieces a conform warped: mask everything else first (mask_ops parts or "
		"invert). Partly masked vertices blend between the morph and the rigid motion. A rigid part can't follow a valley or "
		"curve the slider makes, so each part is moved out as a whole where it would sink into the reference (clearance). "
		"One undo step per slider; the window has it as Slider > Rigidify Parts.";
	rigidify.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"sliders", McpStringList("Sliders to rigidify. Default all morph sliders with data on the shapes.")},
			{"allowScale", McpProp("boolean", "Let parts grow or shrink evenly with the morph. Default false.")},
			{"perPart", McpProp("boolean", "Fit each connected part on its own. With false all unmasked vertices of a shape move as one. Default true.")},
			{"weldDistance", McpProp("number", "Vertices closer than this count as joined. Default 0.001.")},
			{"clearance", McpProp("number", "Keep each part at least this far outside the reference with the slider at 1, or as far as it is with all sliders at 0 when that's less. Negative to leave the parts where the fit puts them. Default 0.02.")},
			{"maxPartVertices", McpProp("integer", "Parts with more vertices keep bending with the morph, e.g. the cloth or long wires next to small clasps and zip teeth. Default 0, no limit.")},
			{"maxPartLength", McpProp("number", "Parts longer than this along their longest side keep bending, e.g. 5 for wires and chains while rings and studs stay rigid. bendingParts in the result counts them. Default 0, no limit.")},
		});
	rigidify.handler = [&mcp](const json& args) { return RigidifySlider(mcp, args); };
	mcp.AddTool(std::move(rigidify));

	McpTool exclusions;
	exclusions.name = "analysis_exclusions";
	exclusions.description =
		"Leaves vertices out of clipping measurements and protects them from clipping fixes, for parts that are inside the "
		"body by design and would otherwise dominate every count: end caps of a sleeveless suit hidden under the sleeves, "
		"cuffs past the body's open wrists or ankles, inner layers. analyze_clipping, quality_sweep, show_heatmap clipping, "
		"the clipping and gap_change metrics and auto_fix_clipping skip them; mask_ops from_analysis keeps them masked and "
		"fix_clipping leaves them alone. Ops: deep_at_rest (deeper than minDepth into the reference with sliders at 0), "
		"reference_edges (where the reference has an open edge: neck, wrists, ankles), from_mask (the masked vertices), "
		"vertices, show (puts them into the mask to look at with render_view display.mask) and clear. New vertices are "
		"added to the existing ones unless replace is true. They last until another project is opened. "
		"The reference is also measured the way BodySlide builds the outfit: what the zap sliders BodySlide turns on by "
		"default remove (a body zapped under a bodysuit) can't show through, so it doesn't count. Op zaps sets the zap "
		"sliders that hide it instead, e.g. [\"BodyZapKeepArms\"] for that variant or [] for the whole body, and without "
		"zaps goes back to the defaults; analyze_clipping reports hiddenByZaps.";
	exclusions.inputSchema = McpSchema(
		{
			{"op", {{"type", "string"}, {"enum", {"deep_at_rest", "reference_edges", "from_mask", "vertices", "show", "clear", "zaps"}}}},
			{"zaps", McpStringList("zaps: the zap sliders that hide parts of the reference while measuring. Leave out for the ones BodySlide turns on by default.")},
			{"shapes", ShapesProp},
			{"vertices", McpIntList("vertices: the vertex indices")},
			{"minDepth", McpProp("number", "deep_at_rest: depth inside the reference that counts as by design. Default 1.")},
			{"edgeDistance", McpProp("number", "reference_edges: how far from the open edge. Default 1.5.")},
			{"against", McpProp("string", "deep_at_rest, reference_edges: measure against this shape instead of the reference")},
			{"grow", McpProp("integer", "Rings of neighbors to add. Default 2 for deep_at_rest, else 0.")},
			{"replace", McpProp("boolean", "Replace the shape's exclusions instead of adding to them. Default false.")},
		},
		{"op"});
	exclusions.handler = [&mcp](const json& args) { return AnalysisExclusions(mcp, args); };
	mcp.AddTool(std::move(exclusions));

	McpTool seams;
	seams.name = "analyze_seams";
	seams.description =
		"Finds seams: vertices at the same place, along split UV or normal seams of a shape, where outfit shapes meet, and "
		"where the outfit meets the reference (neck, wrists). Reports which shapes meet, the sliders that pull seams apart "
		"(a gap or crack in game) with the worst seam, and seams whose sides are weighted differently (they tear open when "
		"the body moves). weld_seams closes them.";
	seams.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"withReference", McpProp("boolean", "Include seams with the reference shape. Default true.")},
		{"sliders", McpStringList("Sliders to check. Default all morph sliders with data on the shapes.")},
		{"tolerance", McpProp("number", "Distance at which vertices count as the same place. Default 0.001.")},
		{"opening", McpProp("number", "Gap a slider has to open at full value to count. Default 0.01.")},
		{"weightTolerance", McpProp("number", "Weight difference (0 to 1) that counts as mismatched. Default 0.05.")},
		{"limit", McpProp("integer", "How many sliders to return. Default 25.")},
	});
	seams.readOnly = true;
	seams.handler = [&mcp](const json& args) { return AnalyzeSeams(mcp, args); };
	mcp.AddTool(std::move(seams));

	McpTool weld;
	weld.name = "weld_seams";
	weld.description =
		"Closes seams that sliders pull apart: the vertices at each seam get the same morph, the reference's where the outfit "
		"meets it, otherwise the mean of the sides. With weights: true they also get the same weights. Masked vertices "
		"keep theirs. One undo step per slider and per shape's weights.";
	weld.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"withReference", McpProp("boolean", "Weld to the reference shape where the outfit meets it. Default true.")},
		{"sliders", McpStringList("Sliders to weld. Default all morph sliders with data on the shapes.")},
		{"sliderData", McpProp("boolean", "Weld the slider morphs. Default true.")},
		{"weights", McpProp("boolean", "Also weld the bone weights. Default false.")},
		{"tolerance", McpProp("number", "Distance at which vertices count as the same place. Default 0.001.")},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	weld.handler = [&mcp](const json& args) { return WeldSeams(mcp, args); };
	mcp.AddTool(std::move(weld));

	McpTool fit;
	fit.name = "analyze_fit";
	fit.description =
		"Checks that sliders keep the outfit's distance to the body: for each slider on its own, how much the gap between "
		"the outfit and the reference (or 'against') changes at vertices close to it. A good conform keeps the gap; "
		"tightened vertices hug or squash into the body, loosened ones drift away. Clipping checks only see the gap going "
		"below zero. Returns the worst sliders first. mask_ops from_analysis with metric gap_change masks the vertices.";
	fit.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"sliders", McpStringList("Sliders to check. Default all morph sliders with data on the shapes or the reference.")},
		{"against", McpProp("string", "Shape to measure the gap to. Default the reference.")},
		{"value", McpProp("number", "Slider value. Default 1.")},
		{"maxDistance", McpProp("number", "Only vertices this close to the body at 0 count. Default 2.")},
		{"threshold", McpProp("number", "Change of the gap that counts as tightened or loosened. Default 0.3.")},
		{"limit", McpProp("integer", "How many to return. Default 25.")},
	});
	fit.readOnly = true;
	fit.handler = [&mcp](const json& args) { return AnalyzeFit(mcp, args); };
	mcp.AddTool(std::move(fit));

	McpTool clean;
	clean.name = "clean_weights";
	clean.description =
		"Cleans up the bone weights of shapes: drops weights below pruneBelow, keeps only the maxInfluences largest bones "
		"of each vertex (the game uses 4) and makes each vertex's weights add up to 1. Masked vertices are left alone. One "
		"undo step per shape.";
	clean.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"pruneBelow", McpProp("number", "Drop weights smaller than this, e.g. 0.01. Default 0, none.")},
		{"maxInfluences", McpProp("integer", "Keep at most this many bones per vertex. Default 0, no limit.")},
		{"normalize", McpProp("boolean", "Make the weights of each vertex add up to 1. Default true.")},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	clean.handler = [&mcp](const json& args) { return CleanWeights(mcp, args); };
	mcp.AddTool(std::move(clean));

	McpTool mirror;
	mirror.name = "mirror_weights";
	mirror.description =
		"Copies the bone weights of one side of shapes to the other, with left and right bones swapped (\"NPC L Calf [LClf]\" "
		"to \"NPC R Calf [RClf]\"). Each vertex takes the weights of the surface at its mirror position, so the sides don't "
		"need matching vertices. +X is the character's right. Masked vertices are left alone. One undo step per shape.";
	mirror.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"from", {{"type", "string"}, {"enum", {"left", "right"}}, {"description", "The side whose weights are copied"}}},
			{"centerTolerance", McpProp("number", "Vertices this close to the middle are left alone. Default 0.01.")},
			{"maxDistance", McpProp("number", "How far from the mirror position to look for the other side's surface. Default 1.")},
			{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
		},
		{"from"});
	mirror.handler = [&mcp](const json& args) { return MirrorWeights(mcp, args); };
	mcp.AddTool(std::move(mirror));

	McpTool smooth;
	smooth.name = "smooth_weights";
	smooth.description =
		"Blends the bone weights of shapes over the mesh, like the weight smoothing brush: each pass moves a vertex's weights "
		"towards the mean of its neighbors', so hard borders between bones become gradients that bend instead of tearing. "
		"Use it on hanging parts (skirt panels, capes, sleeves) whose weight copy split them between two bones. Masked "
		"vertices keep their weights and anchor the blend, partly masked ones change partly; mask what sits on the body. "
		"radius sets how far weights spread in NIF units, instead of iterations. The result is limited to maxInfluences "
		"bones per vertex and normalized. One undo step per shape.";
	smooth.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"radius", McpProp("number", "How far weights spread, in NIF units. Sets the passes from the mesh's mean edge length instead of iterations.")},
		{"iterations", McpProp("integer", "Smoothing passes. Default 10.")},
		{"strength", McpProp("number", "How far each pass moves the weights, 0 to 1. Default 0.5.")},
		{"bones", McpStringList("Only blend these bones; each vertex keeps its total weight on them, the other bones stay. Default all.")},
		{"maxInfluences", McpProp("integer", "Most bones per vertex afterwards. Default 4 (8 for Starfield).")},
		{"pruneBelow", McpProp("number", "Drop weights below this afterwards. Default 0.01.")},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	smooth.handler = [&mcp](const json& args) { return SmoothShapeWeights(mcp, args); };
	mcp.AddTool(std::move(smooth));

	McpTool reassign;
	reassign.name = "reassign_weights";
	reassign.description =
		"Moves the weights of bones onto another bone ('to') or each onto its parent in the skeleton (toParent), e.g. the "
		"calf weights of a long skirt onto the thighs so its hem doesn't fold with the knees, or a helper bone the game "
		"doesn't have onto a standard one. Masked vertices keep their weights, partly masked ones move part. Lists the "
		"bones left without weights (emptyBones), which delete_bones removes. One undo step per shape.";
	reassign.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"from", McpStringList("Bones whose weights move")},
			{"to", McpProp("string", "Bone that gets them")},
			{"toParent", McpProp("boolean", "Move each bone's weights to its parent instead. Default false.")},
			{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
		},
		{"from"});
	reassign.handler = [&mcp](const json& args) { return ReassignWeights(mcp, args); };
	mcp.AddTool(std::move(reassign));

	McpTool poses;
	poses.name = "analyze_poses";
	poses.description =
		"Tests the weights of shapes in poses: how much the mesh stretches or shrinks in each pose (edge strain: 0 when an "
		"edge only moves or turns, 1 for double or half the length), the stretched edges and vertices and the worst ones. "
		"Weights that change abruptly between neighbors tear the mesh apart (a skirt split between the thighs), which shows "
		"as a high maximum strain. The body's own strain in each pose is listed as reference: skin stretches over joints "
		"too. Worst poses first. Measured from the unposed shapes, with the current slider values or sliderValues. "
		"mask_ops from_analysis and show_heatmap take metric pose_stretch with a pose; smooth_weights and auto_fix_weights fix it.";
	poses.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"poses", McpStringList("Poses of list_poses to test. Default all.")},
		{"threshold", McpProp("number", "Strain that counts as stretched. Default 0.5.")},
		{"worstVertices", McpProp("integer", "How many of the most stretched vertices to return per pose. Default 5.")},
		{"withReference", McpProp("boolean", "Also measure the reference shape. Default true.")},
		{"sliderValues", McpNumberMap("Slider values to measure with, the others at 0. Default the current values.")},
	});
	const json presetProperties = McpPresetValueProperties();
	for (auto& property : presetProperties.items())
		poses.inputSchema["properties"][property.key()] = property.value();
	poses.readOnly = true;
	poses.handler = [&mcp](const json& args) { return AnalyzePoses(mcp, args); };
	mcp.AddTool(std::move(poses));

	McpTool autoWeights;
	autoWeights.name = "auto_fix_weights";
	autoWeights.description =
		"Smooths weights where shapes tear or stretch in poses, step by step: finds the vertices whose edges strain more "
		"in any of the poses than the reference's do in it (the body stretches over its joints too), blends the bones whose "
		"weights jump across those edges (e.g. the two thighs a skirt panel is split between) there, grown by rings and "
		"with a soft border, over a radius "
		"that grows each step (a step that doesn't help is taken back and the next radius tried), and keeps a step only when it lowers the peak strain of the poses together without raising "
		"any pose's: a tear spread over more edges is what it's for, so stretchedEdges may grow. Masked vertices keep their "
		"weights: mask what sits tight on the body (mask_ops parts), whose weights should stay the body's. keepClose also "
		"keeps vertices that close to the reference, but hanging parts touch the legs at rest, so it's off by default. "
		"Tries the weights out without changing the shape, then applies the result as one undo step per shape. Reports each "
		"pose before and after. Check with analyze_poses and render_view variants with poses.";
	autoWeights.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"poses", McpStringList("Poses of list_poses to fix in. Default all.")},
		{"threshold", McpProp("number", "Strain that counts as stretched. Default 0.5.")},
		{"relativeToReference", McpProp("boolean", "Only fix where a pose stretches the shape more than it stretches the reference (the body's peak strain in that pose, at least threshold). Default true.")},
		{"bones", McpStringList("Only blend these bones. Default each step the bones whose weights differ most across the stretched edges.")},
		{"radius", McpProp("number", "Smoothing radius of the first step, NIF units. Default 2.")},
		{"radiusStep", McpProp("number", "How much the radius grows per step. Default 2.")},
		{"maxRadius", McpProp("number", "Largest radius to try. Default 12.")},
		{"strength", McpProp("number", "Strength of each smoothing pass. Default 0.5.")},
		{"grow", McpProp("integer", "Rings of neighbors around the stretched vertices that are smoothed too. Default 3.")},
		{"feather", McpProp("integer", "Passes that soften the border of the smoothed area. Default 4.")},
		{"keepClose", McpProp("number", "Vertices closer to the reference than this (or inside it) keep their weights. Default 0, off.")},
		{"maxPoseIncrease", McpProp("number", "How much one pose's peak strain may rise for the others to get better, relative to its peak or the threshold, whichever is higher. Default 0.25.")},
		{"maxInfluences", McpProp("integer", "Most bones per vertex. Default 4 (8 for Starfield).")},
		{"pruneBelow", McpProp("number", "Drop weights below this. Default 0.01.")},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	autoWeights.handler = [&mcp](const json& args) { return AutoFixWeights(mcp, args); };
	mcp.AddTool(std::move(autoWeights));

	McpTool validate;
	validate.name = "validate_project";
	validate.description =
		"Checks the project for what breaks in game rather than in the view, before saving: duplicate shape names, the "
		"reference still in the project, vertex limits, invalid positions or UVs, triangles without area, unweighted "
		"vertices, too many bones per vertex, weights not adding up to 1, shaders without the Skinned flag, triangles "
		"without a partition or segment, missing diffuse textures, sliders the reference has but a shape doesn't follow, "
		"clipping at rest and bad bones. Returns errors, warnings and info; ok is true without errors.";
	validate.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to check. Default all outfit shapes.")},
		{"influenceLimit", McpProp("integer", "Most bones per vertex the game takes. Default 4 (8 for Starfield).")},
	});
	validate.outputSchema = {
		{"type", "object"},
		{"properties",
		 {{"ok", {{"type", "boolean"}}},
		  {"errors", {{"type", "array"}, {"items", {{"type", "object"}}}}},
		  {"warnings", {{"type", "array"}, {"items", {{"type", "object"}}}}},
		  {"info", {{"type", "array"}, {"items", {{"type", "object"}}}}},
		  {"shapes", {{"type", "integer"}}}}},
	};
	validate.readOnly = true;
	validate.handler = [&mcp](const json& args) { return ValidateProject(mcp, args); };
	mcp.AddTool(std::move(validate));
}
