/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../program/OutfitStudio.h"
#include "../program/PartitionTypeChoices.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <limits>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

constexpr size_t MaxEntriesPerCall = 20000;

const json ShapesProp = McpStringList("Outfit shapes, the selected ones by default");
constexpr const char* SnapshotNote = " Like automation, this isn't in the undo history: take a snapshot_save first to go back.";

Mesh* ShapeMesh(OutfitStudioMcp& mcp, NiShape* shape) {
	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	if (!m)
		throw McpToolError("The shape '" + shape->name.get() + "' has no mesh in the view.");
	return m;
}

json Names(const std::vector<NiShape*>& shapes) {
	json names = json::array();
	for (NiShape* shape : shapes)
		names.push_back(shape->name.get());
	return names;
}

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

// The shapes of the project after a change, with their sizes
McpToolResult ShapeList(OutfitStudioMcp& mcp) {
	OutfitProject& project = mcp.Project();
	json shapes = json::array();
	for (auto& name : project.GetShapeOrder()) {
		NiShape* shape = project.GetWorkNif()->FindBlockByName<NiShape>(name);
		if (shape)
			shapes.push_back({{"name", name}, {"vertices", shape->GetNumVertices()}, {"triangles", shape->GetNumTriangles()}});
	}

	McpToolResult result;
	result.data["shapes"] = shapes;
	return result;
}

// How a shape differs from its mirror image across X, like the window's vertex asymmetry dialog finds it
struct ShapeSymmetry {
	Mesh* m = nullptr;
	// The vertices that take part: the unmasked ones, or all
	std::vector<bool> selVerts;
	SymmetricVertices symVerts;
	VertexAsymmetries asyms;
	VertexAsymmetryStats stats;
};

ShapeSymmetry FindSymmetry(OutfitStudioMcp& mcp, NiShape* shape, bool useMask) {
	ShapeSymmetry s;
	s.m = ShapeMesh(mcp, shape);
	s.selVerts.assign(s.m->nVerts, true);
	if (useMask && s.m->mask)
		for (int i = 0; i < s.m->nVerts; i++)
			s.selVerts[i] = s.m->mask[i] == 0.0f;

	OutfitProject& project = mcp.Project();
	project.MatchSymmetricVertices(shape, s.m->weldVerts, s.symVerts);
	project.FindVertexAsymmetries(shape, s.symVerts, s.m->weldVerts, s.asyms);
	CalcVertexAsymmetryStats(s.symVerts, s.asyms, s.selVerts, s.stats);
	return s;
}

// args[name] as true (all), false (none) or a list of names, as one flag per entry of names. Names that aren't
// among them go to outUnknown.
std::vector<bool> SymmetryChoice(const json& args, const char* name, const std::vector<std::string>& names, std::vector<std::string>& outUnknown) {
	std::vector<bool> chosen(names.size(), false);
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return chosen;
	if (value->is_boolean()) {
		chosen.assign(names.size(), value->get<bool>());
		return chosen;
	}
	if (!value->is_array())
		McpArgs::Wrong(name, "true, false or a list of names");

	for (auto& item : *value) {
		if (!item.is_string())
			McpArgs::Wrong(name, "true, false or a list of names");
		auto it = std::find(names.begin(), names.end(), item.get<std::string>());
		if (it == names.end())
			outUnknown.push_back(item.get<std::string>());
		else
			chosen[it - names.begin()] = true;
	}
	return chosen;
}

// The asymmetries of a shape: counts, mean differences, and the sliders and bones with the most, most first
json SymmetryReport(const ShapeSymmetry& s, size_t limit) {
	auto round = [](float v) { return std::round(v * 10000.0f) / 10000.0f; };
	auto worst = [&](size_t count, const std::vector<int>& counts, const std::vector<float>& avgs, const char* key, auto nameOf) {
		std::vector<size_t> order(count);
		for (size_t i = 0; i < count; i++)
			order[i] = i;
		std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return counts[a] > counts[b]; });
		json list = json::array();
		for (size_t i : order)
			if (counts[i] > 0 && list.size() < limit)
				list.push_back({{key, nameOf(i)}, {"vertices", counts[i]}, {"meanError", round(avgs[i])}});
		return list;
	};

	json sliders = worst(s.asyms.sliders.size(), s.stats.sliderCounts, s.stats.sliderAvgs, "slider", [&](size_t i) { return s.asyms.sliders[i].sliderName; });
	json bones = worst(s.asyms.bones.size(), s.stats.boneCounts, s.stats.boneAvgs, "bone", [&](size_t i) { return s.asyms.bones[i].boneName; });
	return {
		{"vertices", s.m->nVerts},
		{"considered", s.stats.unmaskedCount},
		{"unmatched", s.symVerts.unmatched.size()},
		{"positions", {{"vertices", s.stats.posCount}, {"meanError", round(s.stats.posAvg)}}},
		{"sliders", {{"vertices", s.stats.anySliderCount}, {"worst", sliders}}},
		{"bones", {{"vertices", s.stats.anyBoneCount}, {"worst", bones}}},
	};
}

std::string NewShapeName(OutfitStudioMcp& mcp, const json& args, const char* name) {
	std::string newName = McpArgs::String(args, name);
	if (newName.empty())
		McpArgs::Wrong(name, "a name");
	if (mcp.Project().IsValidShape(newName))
		throw McpToolError("There already is a shape '" + newName + "'.");
	return newName;
}

int VertexIndex(const std::string& key, int count, const std::string& shapeName) {
	int vertex = -1;
	try {
		size_t used = 0;
		vertex = std::stoi(key, &used);
		if (used != key.size())
			vertex = -1;
	}
	catch (const std::exception&) {
	}
	if (vertex < 0 || vertex >= count)
		throw McpToolError("'" + key + "' isn't a vertex of '" + shapeName + "'.");
	return vertex;
}

// {"index": [numbers]} with dims numbers each, or {"index": number} when dims is 1
std::map<int, std::vector<float>> VertexValues(const json& args, const char* name, int dims, int count, const std::string& shapeName) {
	std::map<int, std::vector<float>> values;
	const json* value = McpArgs::Find(args, name);
	if (!value)
		return values;
	if (!value->is_object())
		McpArgs::Wrong(name, "an object of vertex indices to values");

	for (auto& item : value->items()) {
		int vertex = VertexIndex(item.key(), count, shapeName);
		std::vector<float> v;
		if (dims == 1 && item.value().is_number())
			v.push_back(item.value().get<float>());
		else if (item.value().is_array() && item.value().size() == static_cast<size_t>(dims)) {
			for (auto& n : item.value()) {
				if (!n.is_number())
					McpArgs::Wrong(name, "an object of vertex indices to arrays of numbers");
				v.push_back(n.get<float>());
			}
		}
		else
			McpArgs::Wrong(name, dims == 1 ? "an object of vertex indices to numbers" : "an object of vertex indices to arrays of numbers");
		values[vertex] = std::move(v);
	}
	return values;
}

std::vector<int> VertexSelection(const json& args, int count) {
	std::vector<int> vertices;
	if (McpArgs::Has(args, "vertices")) {
		for (int v : McpArgs::Ints(args, "vertices"))
			if (v >= 0 && v < count)
				vertices.push_back(v);
		return vertices;
	}

	int start = std::max(0, McpArgs::Int(args, "start", 0));
	int number = McpArgs::Int(args, "count", count);
	for (int v = start; v < count && v - start < number; v++)
		vertices.push_back(v);
	return vertices;
}

McpToolResult GetShapeGeometry(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	NifFile* nif = project.GetWorkNif();

	std::vector<Vector3> verts;
	project.GetLiveVerts(shape, verts);

	McpToolResult result;
	json& data = result.data;
	data["vertices"] = verts.size();
	data["triangles"] = shape->GetNumTriangles();

	if (!verts.empty()) {
		Vector3 lo(FLT_MAX, FLT_MAX, FLT_MAX), hi(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (auto& v : verts) {
			lo.x = std::min(lo.x, v.x);
			lo.y = std::min(lo.y, v.y);
			lo.z = std::min(lo.z, v.z);
			hi.x = std::max(hi.x, v.x);
			hi.y = std::max(hi.y, v.y);
			hi.z = std::max(hi.z, v.z);
		}
		data["bounds"] = {{"min", Vec(lo)}, {"max", Vec(hi)}, {"center", Vec((lo + hi) * 0.5f)}, {"size", Vec(hi - lo)}};
	}

	std::vector<std::string> include = McpArgs::Strings(args, "include");
	auto wants = [&include](const char* what) { return std::find(include.begin(), include.end(), what) != include.end(); };
	for (auto& what : include)
		if (what != "positions" && what != "normals" && what != "uvs" && what != "colors" && what != "triangles")
			throw McpToolError("include takes \"positions\", \"normals\", \"uvs\", \"colors\" and \"triangles\".");
	if (include.empty())
		return result;

	std::vector<int> selection = VertexSelection(args, static_cast<int>(verts.size()));
	if (selection.size() > MaxEntriesPerCall)
		throw McpToolError("That's " + std::to_string(selection.size()) + " vertices, more than " + std::to_string(MaxEntriesPerCall)
						   + " per call. Pass vertices, or start and count.");

	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	std::vector<Vector2> uvs;
	if (wants("uvs"))
		nif->GetUvsForShape(shape, uvs);
	std::vector<Color4> colors;
	if (wants("colors"))
		nif->GetColorsForShape(shape, colors);

	json list = json::array();
	for (int v : selection) {
		json entry = {{"index", v}};
		if (wants("positions"))
			entry["position"] = Vec(verts[v]);
		if (wants("normals") && m && m->norms && v < m->nVerts)
			entry["normal"] = Vec(Mesh::TransformDirMeshToNif(m->norms[v]));
		if (wants("uvs") && v < static_cast<int>(uvs.size()))
			entry["uv"] = {uvs[v].u, uvs[v].v};
		if (wants("colors") && v < static_cast<int>(colors.size()))
			entry["color"] = {colors[v].r, colors[v].g, colors[v].b, colors[v].a};
		list.push_back(entry);
	}
	if (wants("positions") || wants("normals") || wants("uvs") || wants("colors"))
		data["vertexData"] = list;

	if (wants("triangles")) {
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);

		// With a vertex selection, only the triangles that use those vertices
		std::vector<bool> selected;
		if (McpArgs::Has(args, "vertices") || McpArgs::Has(args, "start") || McpArgs::Has(args, "count")) {
			selected.assign(verts.size(), false);
			for (int v : selection)
				selected[v] = true;
		}

		json triangles = json::array();
		for (size_t t = 0; t < tris.size(); t++) {
			const Triangle& tri = tris[t];
			if (!selected.empty() && !(selected[tri.p1] || selected[tri.p2] || selected[tri.p3]))
				continue;
			if (triangles.size() >= MaxEntriesPerCall) {
				data["trianglesTruncated"] = true;
				break;
			}
			triangles.push_back({t, tri.p1, tri.p2, tri.p3});
		}
		data["triangleList"] = triangles;
		data["triangleFormat"] = "[triangle index, vertex 1, vertex 2, vertex 3]";
	}

	return result;
}

// Positions, UVs, vertex colors and alpha of single vertices, each kind as one undo state
McpToolResult SetVertexData(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	Mesh* m = ShapeMesh(mcp, shape);
	const std::string name = shape->name.get();

	std::vector<Vector3> verts;
	project.GetLiveVerts(shape, verts);
	if (static_cast<int>(verts.size()) != m->nVerts)
		throw McpToolError("The shape '" + name + "' doesn't match its mesh in the view.");

	auto positions = VertexValues(args, "positions", 3, m->nVerts, name);
	auto uvs = VertexValues(args, "uvs", 2, m->nVerts, name);
	auto colors = VertexValues(args, "colors", 3, m->nVerts, name);
	auto alpha = VertexValues(args, "alpha", 1, m->nVerts, name);
	if (positions.empty() && uvs.empty() && colors.empty() && alpha.empty())
		throw McpToolError("Give positions, uvs, colors or alpha to change.");

	if (!positions.empty())
		mcp.CheckPositionEdit();
	if (!uvs.empty() && frame->editor.bEditSlider)
		throw McpToolError("UVs can't be changed while a slider is in edit mode. Leave it with set_slider_edit first.");
	if (!colors.empty() || !alpha.empty()) {
		auto shapeColors = project.GetWorkNif()->GetColorsForShape(shape);
		if (!shapeColors || shapeColors->empty() || !m->vcolors || !m->valpha)
			throw McpToolError("The shape '" + name + "' has no vertex colors.");
	}

	const bool relative = McpArgs::Bool(args, "relative", false);
	UndoHistory* history = frame->glView->GetUndoHistory();
	json changed;

	if (!positions.empty() || !uvs.empty()) {
		std::vector<Vector2> shapeUvs;
		if (!uvs.empty() && !project.GetWorkNif()->GetUvsForShape(shape, shapeUvs))
			throw McpToolError("The shape '" + name + "' has no UVs.");

		UndoStateProject* usp = history->PushState();
		usp->undoType = UndoType::VertexPosition;
		UndoStateShape uss;
		uss.shapeName = name;

		for (auto& p : positions) {
			Vector3 target(p.second[0], p.second[1], p.second[2]);
			if (relative)
				target += verts[p.first];
			uss.pointStartState[p.first] = Mesh::TransformPosNifToMesh(verts[p.first]);
			uss.pointEndState[p.first] = Mesh::TransformPosNifToMesh(target);
		}

		for (auto& uv : uvs) {
			if (uv.first >= static_cast<int>(shapeUvs.size()))
				continue;
			Vector2 target(uv.second[0], uv.second[1]);
			if (relative)
				target += shapeUvs[uv.first];
			uss.uvStartState[uv.first] = shapeUvs[uv.first];
			uss.uvEndState[uv.first] = target;
		}
		usp->usss.push_back(std::move(uss));

		if (frame->editor.bEditSlider) {
			usp->sliderName = frame->editor.activeSlider;
			float sliderscale = project.SliderValue(frame->editor.activeSlider);
			usp->sliderscale = sliderscale == 0.0f ? 1.0f : sliderscale;
		}

		frame->glView->ApplyUndoState(usp, false);
		changed["positions"] = positions.size();
		changed["uvs"] = uvs.size();
		changed["slider"] = usp->sliderName.empty() ? json(nullptr) : json(usp->sliderName);
	}

	// Applying a state can rebuild the meshes of the view
	if (!colors.empty()) {
		m = ShapeMesh(mcp, shape);
		UndoStateProject* usp = history->PushState();
		usp->undoType = UndoType::Color;
		UndoStateShape uss;
		uss.shapeName = name;
		for (auto& c : colors) {
			Vector3 target(c.second[0], c.second[1], c.second[2]);
			if (relative)
				target += m->vcolors[c.first];
			uss.pointStartState[c.first] = m->vcolors[c.first];
			uss.pointEndState[c.first] = Vector3(std::clamp(target.x, 0.0f, 1.0f), std::clamp(target.y, 0.0f, 1.0f), std::clamp(target.z, 0.0f, 1.0f));
		}
		usp->usss.push_back(std::move(uss));
		frame->glView->ApplyUndoState(usp, false);
		// Painting colors writes them to the shape when the stroke ends, the undo state alone doesn't
		frame->ActiveShapesUpdated(usp);
		changed["colors"] = colors.size();
	}

	if (!alpha.empty()) {
		m = ShapeMesh(mcp, shape);
		UndoStateProject* usp = history->PushState();
		usp->undoType = UndoType::Alpha;
		UndoStateShape uss;
		uss.shapeName = name;
		for (auto& a : alpha) {
			float target = a.second[0] + (relative ? m->valpha[a.first] : 0.0f);
			uss.pointStartState[a.first].x = m->valpha[a.first];
			uss.pointEndState[a.first].x = std::clamp(target, 0.0f, 1.0f);
		}
		usp->usss.push_back(std::move(uss));
		frame->glView->ApplyUndoState(usp, false);
		changed["alpha"] = alpha.size();
	}

	frame->UpdateUndoTools();
	frame->SetPendingChanges();

	McpToolResult result;
	result.data["changed"] = changed;
	return result;
}

// The vertex edit tools of the window: collapse a vertex, flip or split an edge
McpToolResult EditTopology(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	Mesh* m = ShapeMesh(mcp, shape);
	if (frame->editor.bEditSlider)
		throw McpToolError("The mesh can't be changed while a slider is in edit mode. Leave it with set_slider_edit first.");
	if (!m->bGotWeldVerts)
		m->CalcWeldVerts();

	const std::string op = McpArgs::String(args, "op");
	UndoStateShape uss;
	uss.shapeName = shape->name.get();

	auto edgeArg = [&]() {
		std::vector<int> edge = McpArgs::Ints(args, "edge");
		if (edge.size() != 2 || edge[0] == edge[1] || edge[0] < 0 || edge[1] < 0 || edge[0] >= m->nVerts || edge[1] >= m->nVerts)
			McpArgs::Wrong("edge", "two different vertices of the shape");

		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		Edge e(static_cast<uint16_t>(edge[0]), static_cast<uint16_t>(edge[1]));
		for (auto& t : tris)
			for (int i = 0; i < 3; i++)
				if (t.GetEdge(i).CompareIndices(e))
					return e;
		throw McpToolError("The vertices " + std::to_string(edge[0]) + " and " + std::to_string(edge[1]) + " don't share an edge.");
	};

	if (op == "collapse_vertex") {
		int vertex = McpArgs::Int(args, "vertex", -1);
		if (vertex < 0 || vertex >= m->nVerts)
			McpArgs::Wrong("vertex", "a vertex of the shape");

		std::vector<uint16_t> verts;
		m->GetWeldSet(vertex, verts);
		std::sort(verts.begin(), verts.end());
		if (!project.PrepareCollapseVertex(shape, uss, verts))
			throw McpToolError("The vertex has more than three connections, it can't be collapsed.");
	}
	else if (op == "flip_edge") {
		if (!project.PrepareFlipEdge(shape, uss, edgeArg()))
			throw McpToolError("The edge is on the surface boundary, only inner edges can be flipped.");
	}
	else if (op == "split_edge") {
		Edge edge = edgeArg();
		uint32_t maxTris = std::numeric_limits<uint16_t>::max();
		if (project.GetWorkNif()->GetHeader().GetVersion().IsFO4() || project.GetWorkNif()->GetHeader().GetVersion().IsFO76())
			maxTris = std::numeric_limits<uint32_t>::max();
		if (shape->GetNumVertices() > std::numeric_limits<uint16_t>::max() - 2u || shape->GetNumTriangles() > maxTris - 2u)
			throw McpToolError("The shape has reached the vertex or triangle limit.");

		std::vector<bool> pincs(shape->GetNumVertices(), false);
		pincs[edge.p1] = true;
		pincs[edge.p2] = true;
		if (!project.PrepareRefineMesh(shape, uss, pincs, m->weldVerts, McpArgs::Bool(args, "noCurveOffset", false)))
			throw McpToolError("The edge has triangles of the same orientation on both sides. Fix the orientations before splitting.");
	}
	else
		throw McpToolError("op has to be \"collapse_vertex\", \"flip_edge\" or \"split_edge\".");

	UndoStateProject* usp = frame->glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	frame->glView->ApplyUndoState(usp, false);
	frame->UpdateUndoTools();
	frame->SetPendingChanges();

	McpToolResult result;
	result.data["vertices"] = shape->GetNumVertices();
	result.data["triangles"] = shape->GetNumTriangles();
	return result;
}

constexpr const char* RefinePreviewOverlay = "refineAdaptiveEdges:";

// Vertices a refine pass added are excluded when all the old vertices they connect to are, so the exclusions of
// analysis_exclusions keep covering the same area
void GrowExclusions(OutfitStudioMcp& mcp, NiShape* shape, size_t oldCount) {
	auto& all = mcp.Exclusions();
	auto it = all.find(shape->name.get());
	if (it == all.end() || it->second.size() != oldCount || shape->GetNumVertices() <= oldCount)
		return;

	std::vector<bool>& excluded = it->second;
	const size_t count = shape->GetNumVertices();
	std::vector<int> oldNeighbors(count, 0), excludedNeighbors(count, 0);
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	std::set<std::pair<uint16_t, uint16_t>> seen;
	for (auto& tri : tris) {
		for (int i = 0; i < 3; i++) {
			Edge e = tri.GetEdge(i);
			if (e.p1 >= count || e.p2 >= count || !seen.emplace(std::min(e.p1, e.p2), std::max(e.p1, e.p2)).second)
				continue;
			for (auto [added, other] : {std::make_pair(e.p1, e.p2), std::make_pair(e.p2, e.p1)}) {
				if (added < oldCount || other >= oldCount)
					continue;
				oldNeighbors[added]++;
				excludedNeighbors[added] += excluded[other] ? 1 : 0;
			}
		}
	}

	excluded.resize(count, false);
	for (size_t v = oldCount; v < count; v++)
		excluded[v] = oldNeighbors[v] > 0 && excludedNeighbors[v] == oldNeighbors[v];
}

json RefineSelectionToJson(const AdaptiveRefineShape& found) {
	const MeshRefine::Selection& selection = found.selection;
	json entry = {
		{"edges", selection.edges.size()},
		{"byReason",
		 {{"length", selection.byLength},
		  {"curvature", selection.byCurvature},
		  {"clipping", selection.byClipping},
		  {"bodyDetail", selection.byBodyDetail},
		  {"conforming", selection.conforming}}},
		{"newVertices", found.newVertices},
		{"newTriangles", found.newTriangles},
	};
	if (selection.overLimit)
		entry["overLimit"] = selection.overLimit;
	if (selection.pokeThrough)
		entry["pokeThrough"] = selection.pokeThrough;
	return entry;
}

// Splits edges only where shapes lack detail, one undo state per pass
McpToolResult RefineAdaptive(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	auto shapes = mcp.Shapes(args, "shapes");
	if (frame->editor.bEditSlider)
		throw McpToolError("The mesh can't be changed while a slider is in edit mode. Leave it with set_slider_edit first.");

	AdaptiveRefineOptions options;
	MeshRefine::Options& select = options.select;
	select.maxEdgeLength = std::max(0.0f, McpArgs::Float(args, "maxEdgeLength", 0.0f));
	select.maxAngle = std::max(0.0f, McpArgs::Float(args, "maxAngle", 0.0f));
	select.creaseAngle = McpArgs::Float(args, "creaseAngle", select.creaseAngle);
	select.clipping = McpArgs::Bool(args, "clipping", false);
	select.clearance = McpArgs::Float(args, "clearance", select.clearance);
	select.bodyDetail = std::max(0.0f, McpArgs::Float(args, "bodyDetail", 0.0f));
	select.bodyDistance = std::max(0.0f, McpArgs::Float(args, "bodyDistance", select.bodyDistance));
	select.minEdgeLength = std::max(0.0f, McpArgs::Float(args, "minEdgeLength", select.minEdgeLength));
	select.conforming = McpArgs::Bool(args, "conforming", true);
	const int maxEdges = McpArgs::Int(args, "maxEdges", 0);
	if (maxEdges < 0)
		McpArgs::Wrong("maxEdges", "0 for no limit, or a number of edges");
	select.maxEdges = static_cast<size_t>(maxEdges);
	options.noCurveOffset = McpArgs::Bool(args, "noCurveOffset", false);
	if (select.maxEdgeLength == 0.0f && select.maxAngle == 0.0f && !select.clipping && select.bodyDetail == 0.0f)
		throw McpToolError("Give at least one of maxEdgeLength, maxAngle, clipping and bodyDetail.");

	const bool preview = McpArgs::Bool(args, "preview", false);
	const int passes = preview ? 1 : std::clamp(McpArgs::Int(args, "passes", 1), 1, 5);
	if (select.clipping || select.bodyDetail > 0.0f) {
		options.body = McpAgainstShape(mcp, args, shapes.front());
		options.bodyTriangles = mcp.SurfaceTriangles(options.body);
	}

	for (NiShape* shape : shapes)
		frame->glView->gls.DeleteOverlay(RefinePreviewOverlay + shape->name.get());

	UndoHistory& history = frame->editor.undoHistory;
	json passList = json::array();
	std::vector<AdaptiveRefineShape> lastFound;
	for (int pass = 0; pass < passes; pass++) {
		options.excluded.clear();
		for (NiShape* shape : shapes)
			if (auto* excluded = mcp.Excluded(shape))
				options.excluded[shape->name.get()] = *excluded;

		// Only pushed when it's applied: pushing drops the redo states, and the oldest one when the history is full
		auto prepared = std::make_unique<UndoStateProject>();
		std::vector<AdaptiveRefineShape> found;
		OpResult result;
		{
			// Measured with the case's slider values, the window's are back before the change is applied
			McpSliderValueScope sliders(project);
			if (McpArgs::Bool(args, "zeroOthers", true) && McpHasSliderValuesArg(args))
				sliders.Zero();
			sliders.Set(McpSliderValuesArg(mcp, args));
			result = mcp.Ops().PrepareAdaptiveRefine(shapes, options, *prepared, &found);
		}

		const bool apply = result.ok && !prepared->usss.empty() && !preview;
		if (!result.ok)
			throw McpToolError(result.error + (pass > 0 ? " The passes before it were applied." : ""));

		json shapeEntries = json::object();
		for (auto& f : found)
			shapeEntries[f.shapeName] = RefineSelectionToJson(f);
		passList.push_back({{"shapes", shapeEntries}});
		lastFound = std::move(found);
		if (!apply)
			break;

		std::map<std::string, size_t> oldCounts;
		for (NiShape* shape : shapes)
			oldCounts[shape->name.get()] = shape->GetNumVertices();

		UndoStateProject* usp = history.PushState(std::move(prepared));
		frame->glView->ApplyUndoState(usp, false);
		frame->UpdateUndoTools();
		frame->SetPendingChanges();

		for (NiShape* shape : shapes)
			GrowExclusions(mcp, shape, oldCounts[shape->name.get()]);
	}

	if (preview) {
		for (auto& f : lastFound) {
			Mesh* m = frame->glView->GetMesh(f.shapeName);
			if (m && !f.edges.empty())
				frame->glView->gls.AddVisEdges(m, f.edges, RefinePreviewOverlay + f.shapeName, Vector3(1.0f, 0.5f, 0.0f));
		}
		frame->glView->Render();
	}

	McpToolResult result = ShapeList(mcp);
	result.data["preview"] = preview;
	result.data["passes"] = passList;
	return result;
}

json ExtraDataList(NifFile* nif, NiAVObject* object) {
	json list = json::array();
	if (!object)
		return list;

	for (auto& ref : object->extraDataRefs) {
		auto extra = nif->GetHeader().GetBlock(ref);
		if (!extra)
			continue;

		json entry = {{"type", extra->GetBlockName()}, {"name", extra->name.get()}};
		if (auto s = dynamic_cast<NiStringExtraData*>(extra))
			entry["value"] = s->stringData.get();
		else if (auto i = dynamic_cast<NiIntegerExtraData*>(extra))
			entry["value"] = i->integerData;
		else if (auto f = dynamic_cast<NiFloatExtraData*>(extra))
			entry["value"] = f->floatData;
		else if (auto b = dynamic_cast<NiBooleanExtraData*>(extra))
			entry["value"] = b->booleanData;
		list.push_back(entry);
	}
	return list;
}

// The NiAVObject flags of shapes as the Creation Engine names them. Only hidden means the same in Oblivion and older,
// whose flags have 16 bits.
const std::pair<const char*, uint32_t> ShapeFlagBits[] = {
	{"hidden", 1u << 0},
	{"selectiveUpdate", 1u << 1},
	{"selectiveUpdateTransforms", 1u << 2},
	{"selectiveUpdateController", 1u << 3},
	{"selectiveUpdateRigid", 1u << 4},
	{"displayObject", 1u << 5},
	{"disableSorting", 1u << 6},
	{"selectiveUpdateTransformsOverride", 1u << 7},
	{"saveExternalGeometryData", 1u << 9},
	{"noDecals", 1u << 10},
	{"alwaysDraw", 1u << 11},
	{"meshLOD", 1u << 12},
	{"fixedBound", 1u << 13},
	{"topFadeNode", 1u << 14},
	{"ignoreFade", 1u << 15},
	{"noAnimSyncX", 1u << 16},
	{"noAnimSyncY", 1u << 17},
	{"noAnimSyncZ", 1u << 18},
	{"noAnimSyncS", 1u << 19},
	{"noDismember", 1u << 20},
	{"noDismemberValidity", 1u << 21},
	{"renderUse", 1u << 22},
	{"materialsApplied", 1u << 23},
	{"highDetail", 1u << 24},
	{"forceUpdate", 1u << 25},
	{"preProcessedNode", 1u << 26},
};

bool OldShapeFlags(NifFile* nif) {
	return nif->GetHeader().GetVersion().Stream() <= 26;
}

json ShapeFlags(NifFile* nif, NiShape* shape) {
	const bool old = OldShapeFlags(nif);
	json set = json::array();
	for (auto& bit : ShapeFlagBits)
		if ((shape->flags & bit.second) && (!old || bit.second == 1))
			set.push_back(bit.first);
	return {{"value", shape->flags}, {"hidden", (shape->flags & 1) != 0}, {"set", set}};
}

// Choices of the Transparency pane of the Shape Properties dialog, by their value in the alpha flags
const char* const AlphaBlendModes[] = {"ONE", "ZERO", "SRC_COLOR", "INV_SRC_COLOR", "DST_COLOR", "INV_DST_COLOR",
									   "SRC_ALPHA", "INV_SRC_ALPHA", "DST_ALPHA", "INV_DST_ALPHA", "SRC_ALPHA_SAT"};
const char* const AlphaTestFuncs[] = {"ALWAYS", "LESS", "EQUAL", "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL", "NEVER"};

template<size_t N>
json NameOf(const char* const (&names)[N], int value) {
	return value < static_cast<int>(N) ? json(names[value]) : json(value);
}

json AlphaProperty(NifFile* nif, NiShape* shape) {
	NiAlphaProperty* alpha = nif->GetAlphaProperty(shape);
	if (!alpha)
		return nullptr;

	NiShader* shader = nif->GetShader(shape);
	return {
		{"alphaBlend", (alpha->flags & 1) != 0},
		{"srcBlend", NameOf(AlphaBlendModes, (alpha->flags >> 1) & 0xF)},
		{"destBlend", NameOf(AlphaBlendModes, (alpha->flags >> 5) & 0xF)},
		{"alphaTest", (alpha->flags & (1 << 9)) != 0},
		{"testFunc", NameOf(AlphaTestFuncs, (alpha->flags >> 10) & 0x7)},
		{"threshold", alpha->threshold},
		{"noSorter", (alpha->flags & (1 << 13)) != 0},
		{"vertexAlpha", shader ? json(shader->HasVertexAlpha()) : json(nullptr)},
		{"flags", alpha->flags},
	};
}

std::string PartitionName(int slot) {
	std::string prefix = std::to_string(slot) + " ";
	for (const char* label : PartitionTypeChoices::Labels)
		if (std::string(label).rfind(prefix, 0) == 0)
			return std::string(label).substr(prefix.size());
	return "Unknown";
}

// Flag name to its word (1 or 2) and bit in Skyrim's lighting shader, for set_shader_properties
const std::map<std::string, std::pair<int, uint32_t>>& LightingFlagBits() {
	static const std::map<std::string, std::pair<int, uint32_t>> bits = {
		{"specular", {1, SLSF1_SPECULAR}},
		{"environmentMapping", {1, SLSF1_ENVIRONMENT_MAPPING}},
		{"emissive", {1, SLSF1_OWN_EMIT}},
		{"castShadows", {1, SLSF1_CAST_SHADOWS}},
		{"receiveShadows", {1, SLSF1_RECEIVE_SHADOWS}},
		{"modelSpaceNormals", {1, SLSF1_MODEL_SPACE_NORMALS}},
		{"vertexAlpha", {1, SLSF1_VERTEX_ALPHA}},
		{"doubleSided", {2, SLSF2_DOUBLE_SIDED}},
		{"vertexColors", {2, SLSF2_VERTEX_COLORS}},
		{"glowMap", {2, SLSF2_GLOW_MAP}},
		{"softLighting", {2, SLSF2_SOFT_LIGHTING}},
		{"rimLighting", {2, SLSF2_RIM_LIGHTING}},
		{"backLighting", {2, SLSF2_BACK_LIGHTING}},
	};
	return bits;
}

// The shader flags get_shape_properties shows
json ShaderFlags(NiShader* shader) {
	json flags = {
		{"doubleSided", shader->IsDoubleSided()},
		{"modelSpaceNormals", shader->IsModelSpace()},
		{"vertexColors", shader->HasVertexColors()},
		{"vertexAlpha", shader->HasVertexAlpha()},
		{"skinTinted", shader->IsSkinTinted()},
		{"environmentMapping", shader->HasEnvironmentMapping()},
		{"emissive", shader->IsEmissive()},
		{"specular", shader->HasSpecular()},
		{"glowMap", shader->HasGlowmap()},
		{"softLighting", shader->HasSoftlight()},
		{"rimLighting", shader->HasRimlight()},
		{"backLighting", shader->HasBacklight()},
	};
	if (auto* lighting = dynamic_cast<BSLightingShaderProperty*>(shader)) {
		flags["castShadows"] = (lighting->shaderFlags1 & SLSF1_CAST_SHADOWS) != 0;
		flags["receiveShadows"] = (lighting->shaderFlags1 & SLSF1_RECEIVE_SHADOWS) != 0;
	}
	return flags;
}

McpToolResult GetShapeProperties(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NifFile* nif = project.GetWorkNif();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));

	McpToolResult result;
	json& data = result.data;
	data["name"] = shape->name.get();
	data["blockType"] = shape->GetBlockName();
	data["vertices"] = shape->GetNumVertices();
	data["triangles"] = shape->GetNumTriangles();
	data["reference"] = project.IsBaseShape(shape);

	auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
	data["geometry"] = {
		{"skinned", shape->IsSkinned()},
		{"fullPrecision", bsTriShape ? json(bsTriShape->IsFullPrecision()) : json(nullptr)},
		{"subIndex", shape->HasType<BSSubIndexTriShape>()},
		{"dynamic", shape->HasType<BSDynamicTriShape>()},
		{"vertexColors", shape->HasVertexColors()},
		{"uvs", shape->HasUVs()},
		{"normals", shape->HasNormals()},
	};
	data["flags"] = ShapeFlags(nif, shape);

	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		// Older games keep colors and glossiness in a material property instead of the shader
		NiShader* values = shader;
		if (NiMaterialProperty* material = nif->GetMaterialProperty(shape))
			values = material;

		json shaderType = nullptr;
		if (dynamic_cast<BSLightingShaderProperty*>(shader))
			shaderType = "BSLighting:" + std::to_string(shader->GetShaderType());
		else if (dynamic_cast<BSShaderPPLightingProperty*>(shader))
			shaderType = "PPLighting:" + std::to_string(shader->GetShaderType());

		Color4 emissive = values->GetEmissiveColor();
		Vector2 uvOffset = shader->GetUVOffset();
		Vector2 uvScale = shader->GetUVScale();
		data["shader"] = {
			{"blockType", shader->GetBlockName()},
			{"name", shader->name.get()},
			{"properties",
			 {
				 {"ShaderType", shaderType},
				 {"SpecularColor", Vec(values->GetSpecularColor())},
				 {"SpecularStrength", shader->GetSpecularStrength()},
				 {"SpecularPower", values->GetGlossiness()},
				 {"EmissiveColor", {emissive.r, emissive.g, emissive.b, emissive.a}},
				 {"EmissiveMultiple", values->GetEmissiveMultiple()},
				 {"Alpha", values->GetAlpha()},
				 {"EnvMapScale", shader->GetEnvironmentMapScale()},
				 {"UVOffset", {uvOffset.u, uvOffset.v}},
				 {"UVScale", {uvScale.u, uvScale.v}},
			 }},
			{"flags", ShaderFlags(shader)},
		};

		// The values of set_shader_properties' LightingEffect1/2 (soft and rim light)
		if (auto* lighting = dynamic_cast<BSLightingShaderProperty*>(shader)) {
			if (nif->GetHeader().GetVersion().Stream() < 130) {
				json& properties = data["shader"]["properties"];
				properties["LightingEffect1"] = lighting->softlighting;
				properties["LightingEffect2"] = lighting->rimlightPower;
			}
		}
	}
	else
		data["shader"] = nullptr;
	data["alpha"] = AlphaProperty(nif, shape);

	json textures = json::array();
	auto paths = nif->GetTexturePathRefs(shape);
	for (size_t i = 0; i < paths.size(); i++)
		textures.push_back({{"index", i}, {"path", paths[i].get()}});
	data["textures"] = textures;

	data["extraData"] = ExtraDataList(nif, shape);
	data["rootExtraData"] = ExtraDataList(nif, nif->GetRootNode());

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitions;
	std::vector<int> triParts;
	NifSegmentationInfo segments;
	if (nif->GetShapePartitions(shape, partitions, triParts))
		data["partitions"] = partitions.size();
	else if (NifFile::GetShapeSegments(shape, segments, triParts))
		data["segments"] = segments.segs.size();
	return result;
}

// Triangles chosen by an assignment: listed, all, or those whose vertices are all (un)masked
std::vector<bool> TriangleSelection(OutfitStudioMcp& mcp, NiShape* shape, const json& spec) {
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	std::vector<bool> selected(tris.size(), false);

	if (McpArgs::Bool(spec, "all", false)) {
		selected.assign(tris.size(), true);
		return selected;
	}

	if (McpArgs::Has(spec, "triangles")) {
		for (int t : McpArgs::Ints(spec, "triangles")) {
			if (t < 0 || t >= static_cast<int>(tris.size()))
				throw McpToolError("Triangle " + std::to_string(t) + " doesn't exist on '" + shape->name.get() + "'.");
			selected[t] = true;
		}
		return selected;
	}

	const bool masked = McpArgs::Bool(spec, "masked", false);
	const bool unmasked = McpArgs::Bool(spec, "unmasked", false);
	if (masked == unmasked)
		throw McpToolError("Each assignment needs triangles, all: true, masked: true or unmasked: true.");

	Mesh* m = ShapeMesh(mcp, shape);
	auto isMasked = [m](int v) { return m->mask && v < m->nVerts && m->mask[v] > 0.5f; };
	for (size_t t = 0; t < tris.size(); t++) {
		bool all = isMasked(tris[t].p1) && isMasked(tris[t].p2) && isMasked(tris[t].p3);
		bool none = !isMasked(tris[t].p1) && !isMasked(tris[t].p2) && !isMasked(tris[t].p3);
		selected[t] = masked ? all : none;
	}
	return selected;
}

json AssignmentSchema(const char* target, const char* targetDescription) {
	json properties = {
		{target, McpProp("integer", targetDescription)},
		{"triangles", McpIntList("These triangles")},
		{"masked", McpProp("boolean", "The triangles whose vertices are all masked")},
		{"unmasked", McpProp("boolean", "The triangles whose vertices are all unmasked")},
		{"all", McpProp("boolean", "Every triangle")},
	};
	return {{"type", "array"},
			{"items", {{"type", "object"}, {"properties", properties}}},
			{"description", "Triangles to move, applied in order. Each names its target and the triangles."}};
}

McpToolResult GetPartitions(OutfitStudioMcp& mcp, const json& args) {
	NifFile* nif = mcp.Project().GetWorkNif();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	const bool withTriangles = McpArgs::Bool(args, "withTriangles", false);

	auto trianglesOf = [&](const std::vector<int>& triParts, int part) {
		json list = json::array();
		size_t count = 0;
		for (size_t t = 0; t < triParts.size(); t++) {
			if (triParts[t] != part)
				continue;
			count++;
			if (withTriangles && list.size() < MaxEntriesPerCall)
				list.push_back(t);
		}
		return std::make_pair(count, list);
	};

	McpToolResult result;
	json& data = result.data;

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitions;
	std::vector<int> triParts;
	if (nif->GetShapePartitions(shape, partitions, triParts)) {
		data["kind"] = "partitions";
		json list = json::array();
		for (size_t i = 0; i < partitions.size(); i++) {
			auto tris = trianglesOf(triParts, static_cast<int>(i));
			json entry = {{"index", i}, {"slot", partitions[i].partID}, {"name", PartitionName(partitions[i].partID)}, {"triangleCount", tris.first}};
			if (withTriangles)
				entry["triangles"] = tris.second;
			list.push_back(entry);
		}
		data["partitions"] = list;
		data["unassigned"] = trianglesOf(triParts, -1).first;
		return result;
	}

	NifSegmentationInfo segments;
	if (NifFile::GetShapeSegments(shape, segments, triParts)) {
		data["kind"] = "segments";
		data["ssfFile"] = segments.ssfFile;
		json list = json::array();
		for (size_t i = 0; i < segments.segs.size(); i++) {
			auto& seg = segments.segs[i];
			auto tris = trianglesOf(triParts, seg.partID);
			json entry = {{"index", i}, {"triangleCount", tris.first}};
			if (withTriangles)
				entry["triangles"] = tris.second;

			json subs = json::array();
			for (size_t j = 0; j < seg.subs.size(); j++) {
				auto& sub = seg.subs[j];
				auto subTris = trianglesOf(triParts, sub.partID);
				json subEntry = {{"index", j}, {"userSlotID", sub.userSlotID}, {"material", sub.material}, {"extraData", sub.extraData}, {"triangleCount", subTris.first}};
				if (withTriangles)
					subEntry["triangles"] = subTris.second;
				subs.push_back(subEntry);
			}
			entry["subsegments"] = subs;
			list.push_back(entry);
		}
		data["segments"] = list;
		data["unassigned"] = trianglesOf(triParts, -1).first;
		return result;
	}

	data["kind"] = nullptr;
	return result;
}

McpToolResult SetPartitions(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	NifFile* nif = mcp.Project().GetWorkNif();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	if (!shape->IsSkinned())
		throw McpToolError("Only skinned shapes have partitions.");

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitions;
	std::vector<int> triParts;
	if (!nif->GetShapePartitions(shape, partitions, triParts))
		triParts.assign(shape->GetNumTriangles(), -1);

	if (McpArgs::Has(args, "partitions")) {
		std::vector<int> slots = McpArgs::Ints(args, "partitions");
		if (slots.empty())
			McpArgs::Wrong("partitions", "a list of at least one slot");

		partitions.clear();
		for (int slot : slots) {
			BSDismemberSkinInstance::PartitionInfo info;
			info.flags = PF_EDITOR_VISIBLE;
			info.partID = static_cast<uint16_t>(slot);
			partitions.push_back(info);
		}

		// Triangles of partitions that are gone go to the first one, like deleting one in the window
		for (int& part : triParts)
			if (part < 0 || part >= static_cast<int>(partitions.size()))
				part = 0;
	}

	if (partitions.empty())
		throw McpToolError("The shape has no partitions yet. Pass partitions to create them.");

	if (const json* assign = McpArgs::Find(args, "assign")) {
		if (!assign->is_array())
			McpArgs::Wrong("assign", "an array of assignments");
		for (auto& spec : *assign) {
			int target = McpArgs::Int(spec, "partition", -1);
			if (target < 0 || target >= static_cast<int>(partitions.size()))
				throw McpToolError("Assignments need a partition index from 0 to " + std::to_string(partitions.size() - 1) + ".");
			std::vector<bool> selected = TriangleSelection(mcp, shape, spec);
			for (size_t t = 0; t < selected.size() && t < triParts.size(); t++)
				if (selected[t])
					triParts[t] = target;
		}
	}

	size_t unassigned = std::count(triParts.begin(), triParts.end(), -1);
	if (unassigned > 0)
		throw McpToolError(std::to_string(unassigned) + " triangles have no partition. Assign them, e.g. with all: true.");

	nif->SetShapePartitions(shape, partitions, triParts);
	frame->RefreshActivePartitionTree();
	frame->SetPendingChanges();
	return GetPartitions(mcp, {{"shape", shape->name.get()}});
}

McpToolResult SetSegments(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));

	NifSegmentationInfo segments;
	std::vector<int> triParts;
	if (!NifFile::GetShapeSegments(shape, segments, triParts))
		throw McpToolError("The shape has no segments. Make it a sub index shape first with set_geometry_properties subIndex: true.");

	// Triangles are kept by the position of their segment and subsegment
	std::map<int, std::pair<int, int>> oldPlace;
	for (size_t i = 0; i < segments.segs.size(); i++) {
		oldPlace[segments.segs[i].partID] = {static_cast<int>(i), -1};
		for (size_t j = 0; j < segments.segs[i].subs.size(); j++)
			oldPlace[segments.segs[i].subs[j].partID] = {static_cast<int>(i), static_cast<int>(j)};
	}

	if (const json* list = McpArgs::Find(args, "segments")) {
		if (!list->is_array() || list->empty())
			McpArgs::Wrong("segments", "a list of at least one segment");

		NifSegmentationInfo replaced;
		replaced.ssfFile = segments.ssfFile;
		for (auto& segJson : *list) {
			NifSegmentInfo seg;
			if (const json* subs = McpArgs::Find(segJson, "subsegments")) {
				if (!subs->is_array())
					McpArgs::Wrong("subsegments", "a list of subsegments");
				for (auto& subJson : *subs) {
					NifSubSegmentInfo sub;
					sub.userSlotID = static_cast<uint32_t>(McpArgs::Int(subJson, "userSlotID", 0));
					if (const json* material = McpArgs::Find(subJson, "material")) {
						if (!material->is_number_unsigned())
							McpArgs::Wrong("material", "a material hash, a whole number from 0");
						sub.material = material->get<uint32_t>();
					}
					if (const json* extra = McpArgs::Find(subJson, "extraData")) {
						if (!extra->is_array())
							McpArgs::Wrong("extraData", "a list of numbers");
						for (auto& v : *extra) {
							if (!v.is_number())
								McpArgs::Wrong("extraData", "a list of numbers");
							sub.extraData.push_back(v.get<float>());
						}
					}
					seg.subs.push_back(sub);
				}
			}
			replaced.segs.push_back(seg);
		}
		segments = std::move(replaced);
	}

	// Part IDs are numbered anew: each segment, then its subsegments
	int nextID = 0;
	for (auto& seg : segments.segs) {
		seg.partID = nextID++;
		for (auto& sub : seg.subs)
			sub.partID = nextID++;
	}

	auto partIDAt = [&segments](int segment, int subsegment) {
		if (segment < 0 || segment >= static_cast<int>(segments.segs.size()))
			return -1;
		auto& seg = segments.segs[segment];
		if (subsegment < 0)
			return seg.partID;
		if (subsegment >= static_cast<int>(seg.subs.size()))
			return -1;
		return seg.subs[subsegment].partID;
	};

	for (int& part : triParts) {
		auto it = oldPlace.find(part);
		int id = it != oldPlace.end() ? partIDAt(it->second.first, it->second.second) : -1;
		if (id < 0 && it != oldPlace.end())
			id = partIDAt(it->second.first, -1);
		part = id >= 0 ? id : partIDAt(0, -1);
	}

	if (const json* assign = McpArgs::Find(args, "assign")) {
		if (!assign->is_array())
			McpArgs::Wrong("assign", "an array of assignments");
		for (auto& spec : *assign) {
			int id = partIDAt(McpArgs::Int(spec, "segment", -1), McpArgs::Int(spec, "subsegment", -1));
			if (id < 0)
				throw McpToolError("An assignment names a segment or subsegment that doesn't exist.");
			std::vector<bool> selected = TriangleSelection(mcp, shape, spec);
			for (size_t t = 0; t < selected.size() && t < triParts.size(); t++)
				if (selected[t])
					triParts[t] = id;
		}
	}

	if (McpArgs::Has(args, "ssfFile"))
		segments.ssfFile = McpArgs::String(args, "ssfFile");

	NifFile::SetShapeSegments(shape, segments, triParts);
	frame->MeshFromProj(shape);
	frame->RefreshActiveSegmentTree();
	frame->SetPendingChanges();
	return GetPartitions(mcp, {{"shape", shape->name.get()}});
}

// A shader property value as SetShaderProperties takes it: a number, a list of up to four, or text
json ShaderPropertyEntry(const std::string& name, const json& value) {
	json entry = {{"name", name}};
	if (value.is_string())
		entry["stringValue"] = value.get<std::string>();
	else if (value.is_number())
		entry["value1"] = value.get<float>();
	else if (value.is_array() && !value.empty() && value.size() <= 4) {
		const char* keys[] = {"value1", "value2", "value3", "value4"};
		for (size_t i = 0; i < value.size(); i++) {
			if (!value[i].is_number())
				throw McpToolError("The shader property '" + name + "' has to be a number, a list of numbers or text.");
			entry[keys[i]] = value[i].get<float>();
		}
	}
	else
		throw McpToolError("The shader property '" + name + "' has to be a number, a list of numbers or text.");
	return entry;
}

McpToolResult SetShapeFlags(OutfitStudioMcp& mcp, const json& args) {
	NifFile* nif = mcp.Project().GetWorkNif();
	const bool old = OldShapeFlags(nif);
	const int bitCount = old ? 16 : 32;

	const json* value = McpArgs::Find(args, "value");
	if (value && (!value->is_number_unsigned() || value->get<uint64_t>() > (old ? 0xFFFFull : 0xFFFFFFFFull)))
		McpArgs::Wrong("value", old ? "a whole number from 0 to 65535" : "a whole number from 0 to 4294967295");

	uint32_t on = 0, off = 0;
	if (McpArgs::Has(args, "hidden"))
		(McpArgs::Bool(args, "hidden", false) ? on : off) |= 1u;

	if (const json* flags = McpArgs::Find(args, "flags")) {
		if (!flags->is_object())
			McpArgs::Wrong("flags", "an object of flag names to true or false");
		for (auto& item : flags->items()) {
			auto it = std::find_if(std::begin(ShapeFlagBits), std::end(ShapeFlagBits), [&](auto& bit) { return item.key() == bit.first; });
			if (it == std::end(ShapeFlagBits) || !item.value().is_boolean())
				throw McpToolError("'" + item.key() + "' isn't a shape flag this takes, or its value isn't true or false.");
			if (old && it->second != 1)
				throw McpToolError("Only hidden is named for Oblivion and older games. Use setBits and clearBits.");
			(item.value().get<bool>() ? on : off) |= it->second;
		}
	}

	for (auto [name, mask] : {std::make_pair("setBits", &on), std::make_pair("clearBits", &off)}) {
		for (int bit : McpArgs::Ints(args, name)) {
			if (bit < 0 || bit >= bitCount)
				throw McpToolError(std::string(name) + " takes bits from 0 to " + std::to_string(bitCount - 1) + ".");
			*mask |= 1u << bit;
		}
	}

	if (!value && on == 0 && off == 0)
		throw McpToolError("Give hidden, flags, setBits, clearBits or value.");
	if (on & off)
		throw McpToolError("A flag can't be turned on and off at once.");

	McpToolResult result;
	for (NiShape* shape : mcp.Shapes(args, "shapes", true)) {
		uint32_t flags = value ? value->get<uint32_t>() : shape->flags;
		shape->flags = (flags | on) & ~off;
		result.data["shapes"][shape->name.get()] = ShapeFlags(nif, shape);
	}
	mcp.GetFrame()->SetPendingChanges();
	return result;
}

// The alpha property of shapes like the Transparency pane of the Shape Properties dialog: added when missing, its
// flags and threshold set, or removed
McpToolResult SetAlphaProperty(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NifFile* nif = project.GetWorkNif();
	const bool remove = McpArgs::Bool(args, "remove", false);

	auto choice = [&args](const char* name, const auto& names) {
		std::string value = McpArgs::String(args, name);
		for (size_t i = 0; i < std::size(names); i++)
			if (value == names[i])
				return static_cast<uint16_t>(i);
		throw McpToolError(std::string(name) + " has to be one of " + json(std::vector<std::string>(std::begin(names), std::end(names))).dump() + ".");
	};

	// Bits of the flags to replace, and their new values
	uint16_t mask = 0, bits = 0;
	auto setField = [&](uint16_t fieldMask, uint16_t value) {
		mask |= fieldMask;
		bits = (bits & ~fieldMask) | (value & fieldMask);
	};
	if (McpArgs::Has(args, "alphaBlend"))
		setField(1, McpArgs::Bool(args, "alphaBlend", false) ? 1 : 0);
	if (McpArgs::Has(args, "srcBlend"))
		setField(0xF << 1, choice("srcBlend", AlphaBlendModes) << 1);
	if (McpArgs::Has(args, "destBlend"))
		setField(0xF << 5, choice("destBlend", AlphaBlendModes) << 5);
	if (McpArgs::Has(args, "alphaTest"))
		setField(1 << 9, McpArgs::Bool(args, "alphaTest", false) ? 1 << 9 : 0);
	if (McpArgs::Has(args, "testFunc"))
		setField(0x7 << 10, choice("testFunc", AlphaTestFuncs) << 10);
	if (McpArgs::Has(args, "noSorter"))
		setField(1 << 13, McpArgs::Bool(args, "noSorter", false) ? 1 << 13 : 0);

	const int threshold = McpArgs::Int(args, "threshold", -1);
	if (McpArgs::Has(args, "threshold") && (threshold < 0 || threshold > 255))
		McpArgs::Wrong("threshold", "a whole number from 0 to 255");
	const bool hasVertexAlpha = McpArgs::Has(args, "vertexAlpha");
	const bool vertexAlpha = McpArgs::Bool(args, "vertexAlpha", false);
	if (remove && (mask || threshold >= 0 || hasVertexAlpha))
		throw McpToolError("remove can't be combined with values to set.");

	std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes", true);
	if (!remove)
		for (NiShape* shape : shapes)
			if (!nif->GetAlphaProperty(shape) && !nif->GetShader(shape))
				throw McpToolError("'" + shape->name.get() + "' has no shader, which an alpha property needs. Add one with shape_shader first.");

	McpToolResult result;
	json notes = json::array();
	for (NiShape* shape : shapes) {
		if (remove)
			nif->RemoveAlphaProperty(shape);
		else {
			NiAlphaProperty* alpha = nif->GetAlphaProperty(shape);
			if (!alpha) {
				nif->AssignAlphaProperty(shape, std::make_unique<NiAlphaProperty>());
				alpha = nif->GetAlphaProperty(shape);
			}
			if (!alpha)
				throw McpToolError("'" + shape->name.get() + "' can't have an alpha property.");

			alpha->flags = (alpha->flags & ~mask) | bits;
			if (threshold >= 0)
				alpha->threshold = static_cast<uint8_t>(threshold);

			NiShader* shader = nif->GetShader(shape);
			if (hasVertexAlpha) {
				// Fallout 3 and New Vegas: vertex alpha only works with BSShaderNoLightingProperty
				if (!shader)
					notes.push_back("'" + shape->name.get() + "' has no shader for vertex alpha.");
				else if (shader->HasType<BSShaderLightingProperty>() && !shader->HasType<BSShaderNoLightingProperty>())
					notes.push_back("'" + shape->name.get() + "': vertex alpha only works with BSShaderNoLightingProperty, it was left alone.");
				else {
					bool hadVertexAlpha = shader->HasVertexAlpha();
					shader->SetVertexAlpha(vertexAlpha);
					if (vertexAlpha && !hadVertexAlpha) {
						shader->SetVertexColors(true);
						shape->SetVertexColors(true);
					}
				}
			}
		}

		project.SetTextures(shape);
		result.data["shapes"][shape->name.get()] = AlphaProperty(nif, shape);
	}

	frame->SetPendingChanges();
	frame->glView->Render();
	if (!notes.empty())
		result.data["notes"] = notes;
	return result;
}

// Adds the target game's default shader to shapes without one, or removes the shader with its texture set and alpha
// property, like the buttons of the Shape Properties dialog
McpToolResult ShapeShader(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NifFile* nif = project.GetWorkNif();
	const std::string op = McpArgs::String(args, "op");
	if (op != "add" && op != "remove")
		throw McpToolError("op has to be \"add\" or \"remove\".");

	json changed = json::array(), unchanged = json::array();
	for (NiShape* shape : mcp.Shapes(args, "shapes", true)) {
		bool done = false;
		if (op == "add")
			done = project.AddShader(shape);
		else if (nif->GetShader(shape)) {
			nif->DeleteShader(shape);
			done = true;
		}

		if (done)
			project.SetTextures(shape);
		(done ? changed : unchanged).push_back(shape->name.get());
	}

	if (!changed.empty()) {
		frame->SetPendingChanges();
		frame->glView->Render();
	}

	McpToolResult result;
	result.data["changed"] = changed;
	// Shapes that already had a shader, had none to remove, or can't have one
	result.data["unchanged"] = unchanged;
	return result;
}

// Clones a block of srcNif into nif with the blocks it refers to (a shader's texture set and controllers)
uint32_t CloneBlockInto(NifFile* nif, NifFile* srcNif, NiObject* src) {
	const uint32_t srcId = srcNif->GetBlockID(src);
	NiHeader& hdr = nif->GetHeader();
	auto clone = src->Clone();
	NiObject* block = clone.get();
	std::vector<NiStringRef*> strRefs;
	block->GetStringRefs(strRefs);
	for (auto* str : strRefs)
		str->SetIndex(hdr.AddOrFindStringId(str->get()));
	uint32_t id = hdr.AddBlock(std::move(clone));
	nif->CloneChildren(block, srcNif);

	// CloneChildren rebinds pointers to the children only: a controller's target still names the source block
	for (uint32_t i = id + 1; i < hdr.GetNumBlocks(); i++) {
		std::set<NiRef*> ptrs;
		hdr.GetBlock<NiObject>(i)->GetPtrs(ptrs);
		for (auto* ptr : ptrs)
			if (ptr->index == srcId)
				ptr->index = id;
	}
	return id;
}

// Deletes the block a shape refers to unless other blocks use it too, and clears the reference
void ReleaseBlock(NifFile* nif, NiRef& ref) {
	NiHeader& hdr = nif->GetHeader();
	if (ref.IsEmpty())
		return;
	if (hdr.GetBlockRefCount(ref.index, false) == 1) {
		if (auto* shader = hdr.GetBlock<NiShader>(ref)) {
			if (shader->HasTextureSet() && hdr.GetBlockRefCount(shader->TextureSetRef()->index, false) == 1)
				hdr.DeleteBlock(*shader->TextureSetRef());
			hdr.DeleteBlock(shader->controllerRef);
		}
		hdr.DeleteBlock(ref);
	}
	ref.Clear();
}

// Copies the shader with its texture set, only the texture paths, or the alpha property onto shapes, from shapes of
// a NIF file (a backup or the saved shape data of a set before it was rebuilt) or of the project
McpToolResult CopyShader(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	NifFile* nif = project.GetWorkNif();

	bool copyShader = false, copyTextures = false, copyAlpha = false;
	std::vector<std::string> parts = McpArgs::Strings(args, "copy");
	if (parts.empty())
		parts = {"shader", "alpha"};
	for (auto& part : parts) {
		if (part == "shader")
			copyShader = true;
		else if (part == "textures")
			copyTextures = true;
		else if (part == "alpha")
			copyAlpha = true;
		else
			McpArgs::Wrong("copy", "a list of \"shader\", \"textures\" and \"alpha\"");
	}
	// The shader comes with its texture set
	if (copyShader)
		copyTextures = false;

	// Older games keep shaders in the property list, which this doesn't handle
	const NiVersion& version = nif->GetHeader().GetVersion();
	if (!version.IsSK() && !version.IsSSE() && !version.IsFO4() && !version.IsFO76() && !version.IsSF())
		throw McpToolError("copy_shader takes Skyrim, Fallout 4 and newer. Use set_shader_properties and set_texture_paths for older games.");

	const std::string fromShape = McpArgs::String(args, "fromShape", "");
	NifFile fileNif;
	NifFile* srcNif = nif;
	std::string file;
	if (McpArgs::Has(args, "file")) {
		file = McpFilePath(McpArgs::String(args, "file"));
		if (fileNif.Load(std::filesystem::u8path(file)) != 0)
			throw McpToolError("'" + file + "' couldn't be loaded as a NIF file.");
		const NiVersion& from = fileNif.GetHeader().GetVersion();
		const NiVersion& to = nif->GetHeader().GetVersion();
		if (from.File() != to.File() || from.User() != to.User() || from.Stream() != to.Stream())
			throw McpToolError("'" + file + "' is made for another game than the project. Convert its materials instead (import_from_game).");
		srcNif = &fileNif;
	}
	else if (fromShape.empty())
		throw McpToolError("Give file, fromShape, or both.");

	// Sources are found and checked before anything changes
	std::vector<std::pair<NiShape*, NiShape*>> pairs;
	for (NiShape* shape : mcp.Shapes(args, "shapes", true)) {
		std::string name = fromShape.empty() ? shape->name.get() : fromShape;
		NiShape* src = srcNif->FindBlockByName<NiShape>(name);
		if (!src)
			throw McpToolError("There is no shape '" + name + "' " + (file.empty() ? "in the project." : "in '" + file + "'."));
		if (src == shape)
			throw McpToolError("'" + name + "' can't copy from itself.");
		if (!shape->ShaderPropertyRef() || !src->ShaderPropertyRef())
			throw McpToolError("'" + shape->name.get() + "' or its source can't have a shader.");
		if (copyTextures && !nif->GetShader(shape))
			throw McpToolError("'" + shape->name.get() + "' has no shader for the texture paths. Copy the shader instead.");
		pairs.emplace_back(shape, src);
	}

	McpToolResult result;
	json notes = json::array();
	for (auto& [shape, src] : pairs) {
		if (copyShader) {
			ReleaseBlock(nif, *shape->ShaderPropertyRef());
			if (NiShader* srcShader = srcNif->GetShader(src)) {
				shape->ShaderPropertyRef()->index = CloneBlockInto(nif, srcNif, srcShader);
				// Like CloneShape: model space normal maps don't use the vertex normals in Skyrim
				if ((version.IsSK() || version.IsSSE()) && nif->GetShader(shape)->IsModelSpace()) {
					shape->SetNormals(false);
					shape->SetTangents(false);
				}
			}
			else
				notes.push_back("'" + src->name.get() + "' has no shader, so '" + shape->name.get() + "' has none now.");
		}
		else if (copyTextures) {
			auto paths = srcNif->GetTexturePathRefs(src);
			for (size_t i = 0; i < paths.size(); i++) {
				std::string path = paths[i].get();
				nif->SetTextureSlot(shape, path, static_cast<uint32_t>(i));
			}
		}

		if (copyAlpha) {
			ReleaseBlock(nif, *shape->AlphaPropertyRef());
			if (NiAlphaProperty* srcAlpha = srcNif->GetAlphaProperty(src))
				shape->AlphaPropertyRef()->index = CloneBlockInto(nif, srcNif, srcAlpha);
		}

		project.SetTextures(shape);
		NiShader* shader = nif->GetShader(shape);
		result.data["shapes"][shape->name.get()] = {
			{"from", src->name.get()},
			{"shader", shader ? json(shader->GetBlockName()) : json(nullptr)},
			{"alpha", AlphaProperty(nif, shape)},
		};
	}

	frame->SetPendingChanges();
	frame->glView->Render();
	if (!notes.empty())
		result.data["notes"] = notes;
	return result;
}

} // namespace

void AddShapeTools(OutfitStudioMcp& mcp) {
	McpTool rename;
	rename.name = "rename_shape";
	rename.description = "Renames a shape. Its slider data and bone weights go with it.";
	rename.inputSchema = McpSchema({{"shape", McpProp("string", "Shape to rename")}, {"newName", McpProp("string", "New, unused name")}}, {"shape", "newName"});
	rename.handler = [&mcp](const json& args) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		std::string newName = NewShapeName(mcp, args, "newName");
		McpRunStep(mcp, {{"type", "RenameShape"}, {"OldName", shape->name.get()}, {"NewName", newName}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(rename));

	McpTool deleteShapes;
	deleteShapes.name = "delete_shapes";
	deleteShapes.description = std::string("Deletes shapes from the project, the reference included when named.") + SnapshotNote;
	deleteShapes.inputSchema = McpSchema({{"shapes", McpStringList("Shapes to delete")}}, {"shapes"});
	deleteShapes.handler = [&mcp](const json& args) {
		if (McpArgs::Strings(args, "shapes").empty())
			McpArgs::Wrong("shapes", "a list of shape names");
		auto shapes = mcp.Shapes(args, "shapes", true);
		McpRunStep(mcp, {{"type", "DeleteShape"}, {"TargetMeshes", Names(shapes)}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(deleteShapes));

	McpTool duplicate;
	duplicate.name = "duplicate_shape";
	duplicate.description = "Copies a shape under a new name, with its slider data and bone weights.";
	duplicate.inputSchema = McpSchema({{"shape", McpProp("string", "Shape to copy")}, {"newName", McpProp("string", "Name of the copy")}}, {"shape", "newName"});
	duplicate.handler = [&mcp](const json& args) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		std::string newName = NewShapeName(mcp, args, "newName");
		McpRunStep(mcp, {{"type", "DuplicateShape"}, {"TargetMeshes", {shape->name.get()}}, {"NewName", newName}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(duplicate));

	McpTool setReference;
	setReference.name = "set_reference_shape";
	setReference.description =
		"Makes a shape of the project the reference shape that conforming, weight copies and clipping fixes use, or "
		"with unset: true makes the reference an ordinary outfit shape.";
	setReference.inputSchema = McpSchema({
		{"shape", McpProp("string", "New reference shape")},
		{"unset", McpProp("boolean", "Turn the current reference into an outfit shape instead. Default false.")},
	});
	setReference.handler = [&mcp](const json& args) {
		if (McpArgs::Bool(args, "unset", false))
			McpRunStep(mcp, {{"type", "SetReferenceShape"}, {"UnsetReference", true}});
		else
			McpRunStep(mcp, {{"type", "SetReferenceShape"}, {"ShapeName", mcp.Shape(McpArgs::String(args, "shape"))->name.get()}});
		NiShape* reference = mcp.Project().GetBaseShape();
		McpToolResult result;
		result.data["reference"] = reference ? json(reference->name.get()) : json(nullptr);
		return result;
	};
	mcp.AddTool(std::move(setReference));

	McpTool deleteVerts;
	deleteVerts.name = "delete_vertices";
	deleteVerts.description = std::string(
								  "Deletes vertices with their triangles. Like Delete Vertices in the window, the mask protects: the unmasked "
								  "vertices go, unless masked: true deletes the masked ones. Mask the part to keep or remove with set_mask or "
								  "mask_ops first.")
							  + SnapshotNote;
	deleteVerts.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"masked", McpProp("boolean", "Delete the masked vertices instead of the unmasked ones. Default false.")},
		{"deleteEmptyShapes", McpProp("boolean", "Delete shapes that lose all of their triangles. Default true.")},
	});
	deleteVerts.handler = [&mcp](const json& args) {
		auto shapes = mcp.Shapes(args, "shapes");
		McpRunStep(mcp,
				   {{"type", "DeleteVertices"},
					{"TargetMeshes", Names(shapes)},
					{"DeleteMasked", McpArgs::Bool(args, "masked", false)},
					{"DeleteEmptyShapes", McpArgs::Bool(args, "deleteEmptyShapes", true)}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(deleteVerts));

	McpTool separate;
	separate.name = "separate_vertices";
	separate.description = std::string("Moves the masked vertices of a shape into a new shape.") + SnapshotNote;
	separate.inputSchema = McpSchema({{"shape", McpProp("string", "Shape to split")}, {"newName", McpProp("string", "Name of the new shape")}}, {"shape", "newName"});
	separate.handler = [&mcp](const json& args) {
		NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
		std::string newName = NewShapeName(mcp, args, "newName");
		McpRunStep(mcp, {{"type", "SeparateVertices"}, {"TargetMeshes", {shape->name.get()}}, {"NewName", newName}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(separate));

	McpTool merge;
	merge.name = "merge_shapes";
	merge.description = std::string("Copies the geometry of one shape into another, like Copy Geometry in the window.") + SnapshotNote;
	merge.inputSchema = McpSchema(
		{
			{"source", McpProp("string", "Shape whose geometry is copied")},
			{"target", McpProp("string", "Shape that receives it")},
			{"deleteSource", McpProp("boolean", "Delete the source shape after. Default false.")},
		},
		{"source", "target"});
	merge.handler = [&mcp](const json& args) {
		NiShape* source = mcp.Shape(McpArgs::String(args, "source"));
		NiShape* target = mcp.Shape(McpArgs::String(args, "target"));
		if (source == target)
			throw McpToolError("The source and target have to be different shapes.");
		McpRunStep(mcp,
				   {{"type", "MergeGeometry"},
					{"SourceShape", source->name.get()},
					{"TargetShape", target->name.get()},
					{"DeleteSource", McpArgs::Bool(args, "deleteSource", false)}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(merge));

	McpTool refine;
	refine.name = "refine_mesh";
	refine.description = std::string("Subdivides the unmasked triangles of shapes, adding vertices on the curve of the surface.") + SnapshotNote;
	refine.inputSchema = McpSchema({{"shapes", ShapesProp}});
	refine.handler = [&mcp](const json& args) {
		McpRunStep(mcp, {{"type", "RefineMesh"}, {"TargetMeshes", Names(mcp.Shapes(args, "shapes"))}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(refine));

	McpTool refineAdaptive;
	refineAdaptive.name = "refine_mesh_adaptive";
	refineAdaptive.description =
		"Adds vertices only where shapes lack detail, by splitting edges, to make them look rounder and smoother or to "
		"let them follow the body: across curves that look faceted (vertex normals turning by more than maxAngle, e.g. "
		"over breasts and butt), longer than maxEdgeLength, where the reference shows through or bulges into the outfit "
		"between its vertices (clipping), or much longer than the reference's edges under them (bodyDetail: the "
		"topologies don't match). Give at least one. New vertices sit on the curve of the surface and get averaged "
		"weights, slider data, UVs and colors. Fully masked vertices are left alone, so mask everything but the area. "
		"Everything is measured with the current slider values and pose, or sliderValues or a preset, e.g. where a "
		"large preset makes the breasts look faceted. preview: true only highlights the edges in the view. Each pass "
		"is one undo state.";
	json refineProperties = {
		{"shapes", ShapesProp},
		{"maxEdgeLength", McpProp("number", "Split edges longer than this, in NIF units. Off by default.")},
		{"maxAngle", McpProp("number", "Split edges whose vertex normals turn by more than this many degrees, to round off curves, e.g. 20. Off by default.")},
		{"creaseAngle", McpProp("number", "maxAngle leaves edges alone where the triangles meet at a sharper angle than this, to keep creases and hems sharp. Default 75.")},
		{"clipping", McpProp("boolean", "Split edges where the reference shows through between the vertices. Default false.")},
		{"clearance", McpProp("number", "With clipping: also split edges whose middle is closer to the reference than this, and closer than their ends. Default 0.")},
		{"bodyDetail", McpProp("number", "Split edges near the reference that are longer than this many times its edges there, e.g. 2. Off by default.")},
		{"bodyDistance", McpProp("number", "How close to the reference an edge has to be for bodyDetail. Default 1.")},
		{"minEdgeLength", McpProp("number", "Never split edges shorter than this. Default 0.05.")},
		{"maxEdges", McpProp("integer", "Split at most this many edges per shape and pass, the worst first (clipping goes first). Default no limit.")},
		{"conforming", McpProp("boolean", "Also split the longest edges of the triangles around, so no long thin triangles are left. Default true.")},
		{"noCurveOffset", McpProp("boolean", "Put the new vertices in the middle of the straight edges instead of on the curved surface. Default false.")},
		{"passes", McpProp("integer", "Measure and split again this many times, up to 5, e.g. to round curves further. For clipping, fix_clipping between passes instead: the new vertices stay where the body bulges, so another pass only splits more. Default 1.")},
		{"preview", McpProp("boolean", "Only find and highlight the edges, don't change anything. Default false.")},
		{"against", McpProp("string", "Measure clipping against this shape instead of the reference, e.g. a layer of the outfit underneath")},
		{"sliderValues", McpNumberMap("Slider values to measure clipping with, e.g. {\"BreastsNewSH\": 1}")},
		{"zeroOthers", McpProp("boolean", "With sliderValues or preset: set the other sliders to 0. Default true.")},
	};
	const json presetProperties = McpPresetValueProperties();
	for (auto& property : presetProperties.items())
		refineProperties[property.key()] = property.value();
	refineAdaptive.inputSchema = McpSchema(refineProperties);
	refineAdaptive.handler = [&mcp](const json& args) { return RefineAdaptive(mcp, args); };
	mcp.AddTool(std::move(refineAdaptive));

	McpTool mirror;
	mirror.name = "mirror_shapes";
	mirror.description = std::string("Mirrors whole shapes across the chosen axes, e.g. to make a left piece from a right one.") + SnapshotNote;
	mirror.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"axes", McpStringList("Axes to mirror across, \"x\", \"y\" and \"z\". Default [\"x\"].")},
		{"swapBones", McpProp("boolean", "Swap the weights of left and right bones, for X. Default false.")},
	});
	mirror.handler = [&mcp](const json& args) {
		std::vector<std::string> axes = McpArgs::Has(args, "axes") ? McpArgs::Strings(args, "axes") : std::vector<std::string>{"x"};
		for (auto& axis : axes)
			if (axis != "x" && axis != "y" && axis != "z")
				throw McpToolError("axes takes \"x\", \"y\" and \"z\".");
		auto has = [&axes](const char* axis) { return std::find(axes.begin(), axes.end(), axis) != axes.end(); };
		McpRunStep(mcp,
				   {{"type", "MirrorShape"},
					{"TargetMeshes", Names(mcp.Shapes(args, "shapes"))},
					{"MirrorX", has("x")},
					{"MirrorY", has("y")},
					{"MirrorZ", has("z")},
					{"SwapBonesX", McpArgs::Bool(args, "swapBones", false)}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(mirror));

	McpTool invertUVs;
	invertUVs.name = "invert_uvs";
	invertUVs.description = "Flips the UVs of shapes horizontally (u) and/or vertically (v).";
	invertUVs.inputSchema = McpSchema({{"shapes", ShapesProp}, {"u", McpProp("boolean", "Flip U. Default false.")}, {"v", McpProp("boolean", "Flip V. Default false.")}});
	invertUVs.handler = [&mcp](const json& args) {
		bool u = McpArgs::Bool(args, "u", false), v = McpArgs::Bool(args, "v", false);
		if (!u && !v)
			throw McpToolError("Pass u: true, v: true or both.");
		McpRunStep(mcp, {{"type", "InvertUVs"}, {"TargetMeshes", Names(mcp.Shapes(args, "shapes"))}, {"InvertU", u}, {"InvertV", v}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(invertUVs));

	McpTool normals;
	normals.name = "recalc_normals";
	normals.description = "Recalculates the normals of shapes, and changes the seam smoothing and normal lock settings the window has for them.";
	normals.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"force", McpProp("boolean", "Recalculate even when the normals are locked. Default true.")},
		{"smoothSeams", McpProp("boolean", "Smooth the normals across UV seams. Unchanged by default.")},
		{"seamAngle", McpProp("number", "Largest angle in degrees between faces whose seam is smoothed. Unchanged by default.")},
		{"lock", McpProp("boolean", "Lock the normals so later edits don't recalculate them. Unchanged by default.")},
	});
	normals.handler = [&mcp](const json& args) {
		auto triState = [&args](const char* name) { return McpArgs::Has(args, name) ? (McpArgs::Bool(args, name, false) ? 1 : 0) : -1; };
		McpRunStep(mcp,
				   {{"type", "RecalcNormals"},
					{"TargetMeshes", Names(mcp.Shapes(args, "shapes", true))},
					{"Force", McpArgs::Bool(args, "force", true)},
					{"SeamSmooth", triState("smoothSeams")},
					{"SeamAngle", McpArgs::Float(args, "seamAngle", -1.0f)},
					{"LockNormals", triState("lock")}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(normals));

	McpTool symmetrize;
	symmetrize.name = "symmetrize_vertices";
	symmetrize.description =
		"Makes shapes symmetric across X, like Symmetrize Vertices in the window: the mirror vertices of each pair get "
		"consistent positions, slider data and bone weights (the mirror bone's on the other side), for the unmasked "
		"vertices. analyze_symmetry shows what is asymmetric first. Slider data and weights can be limited to some "
		"sliders and bones; for a bone pair like the breasts, name both sides. Weights are normalized afterwards with the "
		"bones that aren't locked in the Bones tab. One undo step. The result has what was asymmetric before.";
	json namesOrAll = {{"type", {"boolean", "array"}}, {"items", {{"type", "string"}}}};
	json sliderChoice = namesOrAll, boneChoice = namesOrAll;
	sliderChoice["description"] = "Mirror the slider data: true for every slider, or the names of some. Default false.";
	boneChoice["description"] = "Mirror the bone weights: true for every bone, or the names of some. Default false.";
	symmetrize.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"positions", McpProp("boolean", "Mirror the positions. Default true.")},
		{"unmatched", McpProp("boolean", "Also move vertices without an exact partner to the nearest one. Default false.")},
		{"sliders", sliderChoice},
		{"bones", boneChoice},
		{"useMask", McpProp("boolean", "Leave masked vertices alone. Default true.")},
	});
	symmetrize.handler = [&mcp](const json& args) {
		OutfitStudioFrame* frame = mcp.GetFrame();
		OutfitProject& project = mcp.Project();
		auto shapes = mcp.Shapes(args, "shapes");
		const bool useMask = McpArgs::Bool(args, "useMask", true);

		std::vector<std::string> normBones, notNormBones;
		frame->GetNormalizeBones(&normBones, &notNormBones);

		UndoStateProject* usp = frame->glView->GetUndoHistory()->PushState();
		usp->undoType = UndoType::Mesh;

		json before = json::object();
		std::set<std::string> unknownSliders, unknownBones;
		for (auto* shape : shapes) {
			ShapeSymmetry s = FindSymmetry(mcp, shape, useMask);

			std::vector<std::string> sliderNames, boneNames, unknown;
			for (auto& slider : s.asyms.sliders)
				sliderNames.push_back(slider.sliderName);
			for (auto& bone : s.asyms.bones)
				boneNames.push_back(bone.boneName);

			VertexAsymmetryTasks tasks;
			tasks.doPos = McpArgs::Bool(args, "positions", true);
			tasks.doUnmatched = McpArgs::Bool(args, "unmatched", false);
			tasks.doSliders = SymmetryChoice(args, "sliders", sliderNames, unknown);
			unknownSliders.insert(unknown.begin(), unknown.end());
			unknown.clear();
			tasks.doBones = SymmetryChoice(args, "bones", boneNames, unknown);
			unknownBones.insert(unknown.begin(), unknown.end());

			usp->usss.emplace_back();
			UndoStateShape& uss = usp->usss.back();
			uss.shapeName = shape->name.get();
			project.PrepareSymmetrizeVertices(shape, uss, s.symVerts, s.asyms, tasks, s.m->weldVerts, s.selVerts, normBones, notNormBones);

			before[shape->name.get()] = SymmetryReport(s, 10);
		}

		frame->glView->ApplyUndoState(usp, false);
		frame->UpdateUndoTools();
		frame->SetPendingChanges();

		McpToolResult result;
		result.data["before"] = before;
		// Slider and bone names that had nothing asymmetric on any shape, or don't exist
		if (!unknownSliders.empty())
			result.data["slidersWithoutAsymmetry"] = unknownSliders;
		if (!unknownBones.empty())
			result.data["bonesWithoutAsymmetry"] = unknownBones;
		return result;
	};
	mcp.AddTool(std::move(symmetrize));

	McpTool symmetry;
	symmetry.name = "analyze_symmetry";
	symmetry.description =
		"How shapes differ from their mirror image across X, like the window's vertex asymmetry dialog shows it: vertices "
		"without a mirror partner, mirror pairs whose positions differ, and the sliders and bones whose data or weights "
		"differ between the sides, with how many vertices and the mean difference, most first. Only unmasked vertices "
		"count unless useMask is false. symmetrize_vertices fixes them; mask_ops asymmetric masks everything else.";
	symmetry.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"useMask", McpProp("boolean", "Only the unmasked vertices. Default true.")},
		{"limit", McpProp("integer", "How many sliders and bones to list. Default 25.")},
	});
	symmetry.readOnly = true;
	symmetry.handler = [&mcp](const json& args) {
		const bool useMask = McpArgs::Bool(args, "useMask", true);
		const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 0, 1000));
		McpToolResult result;
		for (auto* shape : mcp.Shapes(args, "shapes"))
			result.data["shapes"][shape->name.get()] = SymmetryReport(FindSymmetry(mcp, shape, useMask), limit);
		return result;
	};
	mcp.AddTool(std::move(symmetry));

	McpTool transforms;
	transforms.name = "shape_transforms";
	transforms.description =
		"Bakes transforms into the geometry. apply: the node transform of each shape goes into its vertices. reset: "
		"like Reset Transforms in the window, clears the root, shape and skin transforms of all shapes so only the "
		"bones place them.";
	transforms.inputSchema = McpSchema({
		{"op", {{"type", "string"}, {"enum", {"apply", "reset"}}}},
		{"shapes", McpStringList("apply: shapes, the selected ones by default")},
	}, {"op"});
	transforms.handler = [&mcp](const json& args) {
		std::string op = McpArgs::String(args, "op");
		if (op == "apply")
			McpRunStep(mcp, {{"type", "ApplyTransforms"}, {"TargetMeshes", Names(mcp.Shapes(args, "shapes", true))}});
		else if (op == "reset")
			McpRunStep(mcp, {{"type", "ResetTransforms"}});
		else
			throw McpToolError("op has to be \"apply\" or \"reset\".");
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(transforms));

	McpTool baseShape;
	baseShape.name = "set_base_shape";
	baseShape.description =
		"Makes the current slider result the new base geometry of all shapes, like Set Base Shape in the window, and "
		"zeroes the sliders. Slider data stays relative to the new base.";
	baseShape.handler = [&mcp](const json&) {
		McpRunStep(mcp, {{"type", "SetBaseShape"}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(baseShape));

	McpTool convRef;
	convRef.name = "make_conversion_reference";
	convRef.description =
		"Turns the current slider values into one conversion slider, like Make Conversion Reference in the window: "
		"the reference takes the slider result as its new base, the new slider undoes it, and every other slider is "
		"deleted. Clears the undo history.";
	convRef.inputSchema = McpSchema({{"sliderName", McpProp("string", "Name of the conversion slider")}}, {"sliderName"});
	convRef.handler = [&mcp](const json& args) {
		McpRunStep(mcp, {{"type", "MakeConversionRef"}, {"SliderName", McpArgs::String(args, "sliderName")}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(convRef));

	McpTool unusedNodes;
	unusedNodes.name = "remove_unused_nodes";
	unusedNodes.description = "Deletes the nodes of the NIF that no shape uses, like Delete Unreferenced Nodes in the window.";
	unusedNodes.handler = [&mcp](const json&) {
		McpRunStep(mcp, {{"type", "RemoveUnusedNodes"}});
		return ShapeList(mcp);
	};
	mcp.AddTool(std::move(unusedNodes));

	McpTool geometry;
	geometry.name = "get_shape_geometry";
	geometry.description =
		"Returns the bounds of a shape (min, max, center, size in NIF units, as shown with the current sliders and "
		"pose), and with include the positions, normals, UVs, vertex colors or triangles of chosen vertices. "
		"+X is the character's right, +Y her front, +Z up.";
	geometry.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"include",
			 {{"type", "array"},
			  {"items", {{"type", "string"}, {"enum", {"positions", "normals", "uvs", "colors", "triangles"}}}},
			  {"description", "Per vertex data to return. Default none, only the bounds."}}},
			{"vertices", McpIntList("Only these vertices, and with triangles only the triangles that use them")},
			{"start", McpProp("integer", "First vertex of a range. Default 0.")},
			{"count", McpProp("integer", "Number of vertices of the range. At most 20000 vertices per call.")},
		},
		{"shape"});
	geometry.readOnly = true;
	geometry.handler = [&mcp](const json& args) { return GetShapeGeometry(mcp, args); };
	mcp.AddTool(std::move(geometry));

	McpTool vertexData;
	vertexData.name = "set_vertex_data";
	vertexData.description =
		"Sets the positions (NIF units), UVs, vertex colors (0 to 1) or vertex alpha of single vertices, like the "
		"Move Vertex tool, UV editor and color brushes. Each kind is one undo state. Positions change the base shape "
		"only with all sliders at 0; in slider edit mode they go into the slider. The mask isn't used.";
	vertexData.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"positions", {{"type", "object"}, {"description", "Vertex index to [x, y, z]"}}},
			{"uvs", {{"type", "object"}, {"description", "Vertex index to [u, v]"}}},
			{"colors", {{"type", "object"}, {"description", "Vertex index to [r, g, b]"}}},
			{"alpha", {{"type", "object"}, {"description", "Vertex index to alpha"}}},
			{"relative", McpProp("boolean", "The values are offsets to add instead of new values. Default false.")},
		},
		{"shape"});
	vertexData.handler = [&mcp](const json& args) { return SetVertexData(mcp, args); };
	mcp.AddTool(std::move(vertexData));

	McpTool topology;
	topology.name = "edit_topology";
	topology.description =
		"The vertex edit tools of the window: collapse_vertex removes a vertex with at most three connections, "
		"flip_edge turns the inner edge between two triangles, split_edge adds a vertex in the middle of an edge. "
		"get_shape_geometry with include [\"triangles\"] finds vertices and edges. One undo state.";
	topology.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"op", {{"type", "string"}, {"enum", {"collapse_vertex", "flip_edge", "split_edge"}}}},
			{"vertex", McpProp("integer", "collapse_vertex: the vertex")},
			{"edge", McpIntList("flip_edge and split_edge: the two vertices of the edge")},
			{"noCurveOffset", McpProp("boolean", "split_edge: put the new vertex on the straight edge instead of the curved surface. Default false.")},
		},
		{"shape", "op"});
	topology.handler = [&mcp](const json& args) { return EditTopology(mcp, args); };
	mcp.AddTool(std::move(topology));

	McpTool properties;
	properties.name = "get_shape_properties";
	properties.description =
		"Returns what the Shape Properties dialog shows: geometry type and flags, shader with its properties (named "
		"as set_shader_properties takes them), texture paths by slot, extra data of the shape and the root node, and "
		"the number of partitions or segments.";
	properties.inputSchema = McpSchema({{"shape", McpProp("string", "Shape name")}}, {"shape"});
	properties.readOnly = true;
	properties.handler = [&mcp](const json& args) { return GetShapeProperties(mcp, args); };
	mcp.AddTool(std::move(properties));

	McpTool shaderProps;
	shaderProps.name = "set_shader_properties";
	shaderProps.description =
		"Changes shader properties of shapes. Properties: ShaderType (\"BSLighting:<type>\" or \"PPLighting:<type>\"), "
		"SpecularColor [r, g, b], SpecularStrength, SpecularPower, EmissiveColor [r, g, b, a], EmissiveMultiple, "
		"Alpha, EnvMapScale, EyeCubemapScale, UVOffset [u, v], UVScale [u, v], LightingEffect1, LightingEffect2, "
		"SkinTintColor [r, g, b], HairTintColor [r, g, b], RefractionStrength. Properties the shader doesn't have "
		"are skipped with a log entry. flags turns shader flags of Skyrim's lighting shader on or off, e.g. "
		"{\"doubleSided\": true} for a cape seen from both sides, {\"specular\": true} for the normal map's specular "
		"mask, {\"emissive\": false}. Flags that already have the value are left alone on any shader, so the shader "
		"of get_shape_properties can be passed back as it is. To copy a whole shader from another file, use copy_shader.";
	shaderProps.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"properties", {{"type", "object"}, {"description", "Property name to a number, a list of numbers or text"}}},
		{"flags",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "boolean"}}},
		  {"description", "Flag name to on or off: specular, environmentMapping, emissive (own emit), castShadows, receiveShadows, "
						  "modelSpaceNormals, vertexAlpha, doubleSided, vertexColors, glowMap, softLighting, rimLighting, backLighting. "
						  "skinTinted is taken when it doesn't change (it's the shader type)."}}},
	});
	shaderProps.handler = [&mcp](const json& args) {
		const json* props = McpArgs::Find(args, "properties");
		const json* flags = McpArgs::Find(args, "flags");
		if ((!props || !props->is_object() || props->empty()) && (!flags || !flags->is_object() || flags->empty()))
			McpArgs::Wrong("properties", "an object of property names to values, or flags an object of flag names to true or false");
		std::vector<NiShape*> shapes = mcp.Shapes(args, "shapes", true);
		json names = Names(shapes);

		if (flags && flags->is_object()) {
			const auto& flagBits = LightingFlagBits();
			for (auto& item : flags->items())
				if ((!flagBits.count(item.key()) && item.key() != "skinTinted") || !item.value().is_boolean())
					throw McpToolError("'" + item.key() + "' isn't a shader flag this takes, or its value isn't true or false.");

			// Flags that already have the value are left alone, so what get_shape_properties shows can be passed back
			// as it is, for any shader. All shapes are checked before any changes.
			OutfitProject& project = mcp.Project();
			std::vector<std::pair<BSLightingShaderProperty*, NiShape*>> changing;
			for (NiShape* shape : shapes) {
				NiShader* shader = project.GetWorkNif()->GetShader(shape);
				json current = shader ? ShaderFlags(shader) : json::object();
				bool changes = false;
				for (auto& item : flags->items()) {
					if (current.contains(item.key()) && current[item.key()] == item.value())
						continue;
					if (item.key() == "skinTinted")
						throw McpToolError("skinTinted comes from the shader type: set ShaderType to \"BSLighting:5\" (skin tint) or another type instead.");
					changes = true;
				}
				if (!changes)
					continue;
				auto* lighting = dynamic_cast<BSLightingShaderProperty*>(shader);
				if (!lighting)
					throw McpToolError("'" + shape->name.get() + "' has no lighting shader (BSLightingShaderProperty) for these flags.");
				changing.emplace_back(lighting, shape);
			}

			for (auto& [shader, shape] : changing) {
				for (auto& item : flags->items()) {
					if (item.key() == "skinTinted")
						continue;
					const auto& bit = flagBits.at(item.key());
					uint32_t& word = bit.first == 1 ? shader->shaderFlags1 : shader->shaderFlags2;
					word = item.value().get<bool>() ? (word | bit.second) : (word & ~bit.second);
				}
				project.SetTextures(shape);
			}
			if (!changing.empty()) {
				mcp.GetFrame()->SetPendingChanges(true);
				mcp.GetFrame()->glView->Render();
			}
		}

		if (props && props->is_object() && !props->empty()) {
			json list = json::array();
			for (auto& item : props->items())
				// get_shape_properties shows ShaderType null for shaders without one
				if (!item.value().is_null())
					list.push_back(ShaderPropertyEntry(item.key(), item.value()));
			if (!list.empty())
				McpRunStep(mcp, {{"type", "SetShaderProperties"}, {"TargetMeshes", names}, {"ShaderProperties", list}});
		}
		return GetShapeProperties(mcp, {{"shape", names.front()}});
	};
	mcp.AddTool(std::move(shaderProps));

	McpTool texturePaths;
	texturePaths.name = "set_texture_paths";
	texturePaths.description =
		"Sets texture paths of shapes by slot index (0 diffuse, 1 normal, 2 glow or skin, 3 height or parallax, 4 "
		"environment, 5 environment mask, 6 subsurface or inner layer, 7 back light or specular). Paths are relative "
		"to the game's Data folder, e.g. textures\\armor\\outfit_d.dds.";
	texturePaths.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"textures", {{"type", "object"}, {"additionalProperties", {{"type", "string"}}}, {"description", "Slot index to path, e.g. {\"0\": \"textures\\\\a_d.dds\"}"}}},
		},
		{"textures"});
	texturePaths.handler = [&mcp](const json& args) {
		const json* textures = McpArgs::Find(args, "textures");
		if (!textures || !textures->is_object() || textures->empty())
			McpArgs::Wrong("textures", "an object of slot indices to paths");
		json list = json::array();
		for (auto& item : textures->items()) {
			if (!item.value().is_string())
				McpArgs::Wrong("textures", "an object of slot indices to paths");
			int slot = -1;
			try {
				slot = std::stoi(item.key());
			}
			catch (const std::exception&) {
			}
			if (slot < 0 || std::to_string(slot) != item.key())
				throw McpToolError("'" + item.key() + "' isn't a texture slot index.");
			list.push_back({{"index", slot}, {"path", item.value().get<std::string>()}});
		}
		json names = Names(mcp.Shapes(args, "shapes", true));
		McpRunStep(mcp, {{"type", "SetTexturePaths"}, {"TargetMeshes", names}, {"TexturePaths", list}});
		return GetShapeProperties(mcp, {{"shape", names.front()}});
	};
	mcp.AddTool(std::move(texturePaths));

	McpTool geometryProps;
	geometryProps.name = "set_geometry_properties";
	geometryProps.description =
		"Changes the geometry type of shapes like the Geometry tab of the Shape Properties dialog: skinned (false "
		"removes the skinning), full precision vertices (Fallout 4 and 76), sub index shape (Fallout 4 and 76 "
		"segments) and dynamic shape (Skyrim SE head parts). Properties a shape can't have are skipped with a log entry.";
	geometryProps.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"skinned", McpProp("boolean", "Skinned to bones")},
		{"fullPrecision", McpProp("boolean", "Full precision vertices")},
		{"subIndex", McpProp("boolean", "BSSubIndexTriShape with segments")},
		{"dynamic", McpProp("boolean", "BSDynamicTriShape")},
	});
	geometryProps.handler = [&mcp](const json& args) {
		json list = json::array();
		const std::pair<const char*, const char*> properties[] = {{"skinned", "Skinned"}, {"fullPrecision", "FullPrecision"}, {"subIndex", "SubIndex"}, {"dynamic", "Dynamic"}};
		for (auto& name : properties)
			if (McpArgs::Has(args, name.first))
				list.push_back({{"name", name.second}, {"enabled", McpArgs::Bool(args, name.first, false)}});
		if (list.empty())
			throw McpToolError("Give at least one of skinned, fullPrecision, subIndex and dynamic.");
		// The step can replace the shape blocks, so only the names are kept
		json names = Names(mcp.Shapes(args, "shapes", true));
		McpRunStep(mcp, {{"type", "SetGeometryProperties"}, {"TargetMeshes", names}, {"GeometryProperties", list}});
		return GetShapeProperties(mcp, {{"shape", names.front()}});
	};
	mcp.AddTool(std::move(geometryProps));

	McpTool shapeFlags;
	shapeFlags.name = "set_shape_flags";
	shapeFlags.description =
		"Turns flags of shapes (NiTriShape, BSTriShape and others, in every game) on or off, e.g. hidden: true for a "
		"shape the game shouldn't draw. flags takes the names the Creation Engine uses for Fallout 3 and newer; "
		"setBits and clearBits take bit numbers, and value replaces all flags before them. Oblivion and older have 16 "
		"bits and only hidden is named. The window still draws hidden shapes. get_shape_properties shows the flags.";
	json flagNames = json::array();
	for (auto& bit : ShapeFlagBits)
		flagNames.push_back(bit.first);
	shapeFlags.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to change, the selected ones by default")},
		{"hidden", McpProp("boolean", "The hidden flag (bit 0)")},
		{"flags",
		 {{"type", "object"},
		  {"additionalProperties", {{"type", "boolean"}}},
		  {"description", "Flag name to on or off, e.g. {\"noDecals\": true}. Names: " + flagNames.dump()}}},
		{"setBits", McpIntList("Bit numbers to turn on")},
		{"clearBits", McpIntList("Bit numbers to turn off")},
		{"value", McpProp("integer", "Replaces all flags with this number first, e.g. 524302 (the default of skinned shapes)")},
	});
	shapeFlags.handler = [&mcp](const json& args) { return SetShapeFlags(mcp, args); };
	mcp.AddTool(std::move(shapeFlags));

	McpTool shader;
	shader.name = "shape_shader";
	shader.description =
		"Adds or removes the shader of shapes, like the Add and Remove buttons of the Shape Properties dialog. add gives "
		"shapes without one the target game's default shader with an empty texture set (BSLightingShaderProperty for "
		"Skyrim and Fallout 4, BSShaderPPLightingProperty with a material for Fallout 3 and New Vegas, a material for "
		"Oblivion). remove deletes the shader with its texture set and the alpha property. Change it after with "
		"set_shader_properties and set_texture_paths.";
	shader.inputSchema = McpSchema({
		{"op", {{"type", "string"}, {"enum", {"add", "remove"}}}},
		{"shapes", McpStringList("Shapes to change, the selected ones by default")},
	}, {"op"});
	shader.handler = [&mcp](const json& args) { return ShapeShader(mcp, args); };
	mcp.AddTool(std::move(shader));

	McpTool copyShader;
	copyShader.name = "copy_shader";
	copyShader.description =
		"Copies the shader (with its texture set and every property and flag), only the texture paths, or the alpha "
		"property onto shapes from shapes of a NIF file or of the project, exactly as they are there. Use it to keep a "
		"set's look when it's rebuilt from its source: the saved shape data or a backup of it has the shaders, gloss, "
		"environment maps and transparency set up since. By default each shape takes the shape of the same name in "
		"file; fromShape names one shape for all. The file has to be for the same game.";
	copyShader.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to change, the selected ones by default")},
		{"file", McpProp("string", "NIF file to copy from, absolute or relative to the BodySlide folder. Without it, fromShape is a shape of the project.")},
		{"fromShape", McpProp("string", "Shape to copy from. Default: the shape of the same name in file.")},
		{"copy",
		 {{"type", "array"},
		  {"items", {{"type", "string"}, {"enum", {"shader", "textures", "alpha"}}}},
		  {"description", "What to copy: shader (with its textures), textures (only the paths, keeping the shader) and alpha (removed when the source has none). Default [\"shader\", \"alpha\"]."}}},
	});
	copyShader.handler = [&mcp](const json& args) { return CopyShader(mcp, args); };
	mcp.AddTool(std::move(copyShader));

	json blendModes = {{"type", "string"}, {"enum", std::vector<std::string>(std::begin(AlphaBlendModes), std::end(AlphaBlendModes))}};
	json testFuncs = {{"type", "string"}, {"enum", std::vector<std::string>(std::begin(AlphaTestFuncs), std::end(AlphaTestFuncs))}};
	McpTool alpha;
	alpha.name = "set_alpha_property";
	alpha.description =
		"Sets the transparency of shapes like the Transparency pane of the Shape Properties dialog: adds an alpha "
		"property (NiAlphaProperty) when a shape has none and sets the given values, or with remove: true deletes it. "
		"A new one tests alpha (GREATER than threshold 128) without blending. alphaTest cuts pixels off by the "
		"diffuse alpha, e.g. for hair, lace or fringes; alphaBlend blends them, e.g. SRC_ALPHA with INV_SRC_ALPHA for "
		"glass or sheer cloth. Values not given stay.";
	alpha.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to change, the selected ones by default")},
		{"remove", McpProp("boolean", "Remove the alpha property. Default false.")},
		{"alphaTest", McpProp("boolean", "Alpha testing")},
		{"testFunc", testFuncs},
		{"threshold", McpProp("integer", "Alpha test threshold, 0 to 255")},
		{"alphaBlend", McpProp("boolean", "Alpha blending")},
		{"srcBlend", blendModes},
		{"destBlend", blendModes},
		{"noSorter", McpProp("boolean", "Don't sort the blended triangles")},
		{"vertexAlpha", McpProp("boolean", "The shader's vertex alpha flag (turns vertex colors on with it). Fallout 3 and New Vegas: only with BSShaderNoLightingProperty.")},
	});
	alpha.handler = [&mcp](const json& args) { return SetAlphaProperty(mcp, args); };
	mcp.AddTool(std::move(alpha));

	McpTool extraData;
	extraData.name = "set_extra_data";
	extraData.description =
		"Adds or changes an extra data block of shapes, or of the root node when no shapes are given (e.g. BODYTRI "
		"for body morphs, or HDT physics XML paths). With delete: true removes the blocks of that name instead.";
	extraData.inputSchema = McpSchema(
		{
			{"shapes", McpStringList("Shapes to change. Default: the root node.")},
			{"name", McpProp("string", "Name of the extra data, e.g. \"BODYTRI\"")},
			{"type",
			 {{"type", "string"},
			  {"enum", {"NiStringExtraData", "NiIntegerExtraData", "NiFloatExtraData", "NiBooleanExtraData"}},
			  {"description", "Block type. Default NiStringExtraData."}}},
			{"value", McpProp("string", "Value as text, e.g. a path, \"12\", \"0.5\" or \"true\"")},
			{"delete", McpProp("boolean", "Remove the extra data of this name. Default false.")},
		},
		{"name"});
	extraData.handler = [&mcp](const json& args) {
		json step = {{"Name", McpArgs::String(args, "name")}};
		if (McpArgs::Has(args, "shapes"))
			step["TargetMeshes"] = Names(mcp.Shapes(args, "shapes", true));
		if (McpArgs::Bool(args, "delete", false))
			step["type"] = "DeleteExtraData";
		else {
			step["type"] = "SetExtraData";
			step["BlockType"] = McpArgs::String(args, "type", "NiStringExtraData");
			step["Value"] = McpArgs::String(args, "value");
		}
		McpRunStep(mcp, step);

		NifFile* nif = mcp.Project().GetWorkNif();
		McpToolResult result;
		result.data["rootExtraData"] = ExtraDataList(nif, nif->GetRootNode());
		if (McpArgs::Has(args, "shapes")) {
			json shapes = json::object();
			for (NiShape* shape : mcp.Shapes(args, "shapes", true))
				shapes[shape->name.get()] = ExtraDataList(nif, shape);
			result.data["shapeExtraData"] = shapes;
		}
		return result;
	};
	mcp.AddTool(std::move(extraData));

	McpTool getPartitions;
	getPartitions.name = "get_partitions";
	getPartitions.description =
		"Returns the partitions (Skyrim and older: body slots per triangle) or segments (Fallout 4 and 76) of a shape "
		"with their triangle counts, and the triangles of each with withTriangles.";
	getPartitions.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"withTriangles", McpProp("boolean", "List the triangles of each partition or segment. Default false.")},
		},
		{"shape"});
	getPartitions.readOnly = true;
	getPartitions.handler = [&mcp](const json& args) { return GetPartitions(mcp, args); };
	mcp.AddTool(std::move(getPartitions));

	McpTool setPartitions;
	setPartitions.name = "set_partitions";
	setPartitions.description =
		"Edits the partitions of a skinned shape (Skyrim and older), like the Partitions tab: partitions replaces the "
		"list with these body slots (e.g. [32, 38] for body and calves; triangles of removed partitions go to the "
		"first), and assign moves triangles to a partition, e.g. the masked ones. Every triangle needs a partition.";
	setPartitions.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"partitions", McpIntList("Body slot of each partition, in order, e.g. 32 for body")},
			{"assign", AssignmentSchema("partition", "Index of the partition in the list")},
		},
		{"shape"});
	setPartitions.handler = [&mcp](const json& args) { return SetPartitions(mcp, args); };
	mcp.AddTool(std::move(setPartitions));

	McpTool setSegments;
	setSegments.name = "set_segments";
	setSegments.description =
		"Edits the segments of a sub index shape (Fallout 4 and 76), like the Segments tab: segments replaces the "
		"list (triangles keep the segment and subsegment at the same position, or go to the first segment), assign "
		"moves triangles to a segment or subsegment, and ssfFile sets the segment file path.";
	json subsegmentSchema = {
		{"type", "object"},
		{"properties",
		 {{"userSlotID", McpProp("integer", "Body part slot, e.g. 30")},
		  {"material", McpProp("integer", "Material hash")},
		  {"extraData", {{"type", "array"}, {"items", {{"type", "number"}}}}}}},
	};
	json assignSegments = AssignmentSchema("segment", "Index of the segment");
	assignSegments["items"]["properties"]["subsegment"] = McpProp("integer", "Index of the subsegment in the segment, none for the segment itself");
	setSegments.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape name")},
			{"segments",
			 {{"type", "array"},
			  {"items", {{"type", "object"}, {"properties", {{"subsegments", {{"type", "array"}, {"items", subsegmentSchema}}}}}}},
			  {"description", "Segments in order, each with its subsegments"}}},
			{"assign", assignSegments},
			{"ssfFile", McpProp("string", "Path of the segment (.ssf) file")},
		},
		{"shape"});
	setSegments.handler = [&mcp](const json& args) { return SetSegments(mcp, args); };
	mcp.AddTool(std::move(setSegments));

	McpTool copyPartitions;
	copyPartitions.name = "copy_partitions";
	copyPartitions.description = "Copies the partitions or segments of the reference shape to outfit shapes by the nearest reference triangle.";
	copyPartitions.inputSchema = McpSchema({{"shapes", ShapesProp}});
	copyPartitions.handler = [&mcp](const json& args) {
		json names = Names(mcp.Shapes(args, "shapes"));
		McpRunStep(mcp, {{"type", "CopySegPart"}, {"TargetMeshes", names}});
		return GetPartitions(mcp, {{"shape", names.front()}});
	};
	mcp.AddTool(std::move(copyPartitions));
}
