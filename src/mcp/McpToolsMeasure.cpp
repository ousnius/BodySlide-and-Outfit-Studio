/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/MeshAnalysis.h"
#include "../program/OutfitStudio.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>

using nlohmann::json;
using namespace nifly;

namespace {

float Round(float v, float step = 0.01f) {
	return std::round(v / step) * step;
}

json VectorJson(const Vector3& v) {
	return json::array({Round(v.x, 0.001f), Round(v.y, 0.001f), Round(v.z, 0.001f)});
}

Vector3 VectorArg(const json& value, const char* name) {
	if (!value.is_array() || value.size() != 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number())
		McpArgs::Wrong(name, "[x, y, z]");
	return Vector3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
}

// A box of the shape's rest positions, with all sliders at 0, so its vertices are the same in every case
struct Region {
	std::string name = "all";
	bool everything = true;
	Vector3 low;
	Vector3 high;
	bool mirrorX = false;

	bool Contains(const Vector3& p) const {
		if (everything)
			return true;
		auto inside = [this](const Vector3& q) {
			return q.x >= low.x && q.x <= high.x && q.y >= low.y && q.y <= high.y && q.z >= low.z && q.z <= high.z;
		};
		return inside(p) || (mirrorX && inside(Vector3(-p.x, p.y, p.z)));
	}
};

std::vector<Region> Regions(const json& args) {
	std::vector<Region> regions;
	const json* list = McpArgs::Find(args, "regions");
	if (!list || list->empty())
		return {Region()};
	if (!list->is_array())
		McpArgs::Wrong("regions", "a list of {name, min, max, mirrorX}");

	for (auto& item : *list) {
		if (!item.is_object() || !item.contains("min") || !item.contains("max"))
			McpArgs::Wrong("regions", "a list of {name, min: [x, y, z], max: [x, y, z], mirrorX}");
		Region region;
		region.everything = false;
		region.name = item.value("name", "region " + std::to_string(regions.size()));
		region.low = VectorArg(item["min"], "regions min");
		region.high = VectorArg(item["max"], "regions max");
		region.mirrorX = item.value("mirrorX", false);
		regions.push_back(region);
	}
	return regions;
}

// Which vertices of a shape take part: all, or only its masked or unmasked ones
std::vector<bool> MaskFilter(OutfitStudioMcp& mcp, NiShape* shape, size_t count, const std::string& which) {
	std::vector<bool> use(count, true);
	if (which == "all")
		return use;
	if (which != "masked" && which != "unmasked")
		McpArgs::Wrong("vertices", "\"all\", \"masked\" or \"unmasked\"");

	Mesh* m = mcp.Project().GetHost().GetShapeMesh(shape->name.get());
	const bool hasMask = m && m->mask && m->nVerts == static_cast<int>(count);
	for (size_t v = 0; v < count; v++) {
		const bool masked = hasMask && m->mask[v] > 0.5f;
		use[v] = which == "masked" ? masked : !masked;
	}
	return use;
}

float Percentile(std::vector<float>& sorted, float share) {
	if (sorted.empty())
		return 0.0f;
	size_t i = std::min(sorted.size() - 1, static_cast<size_t>(share * static_cast<float>(sorted.size() - 1) + 0.5f));
	return sorted[i];
}

// Distribution of signed values: gaps to a surface, or their change
json Distribution(std::vector<float> values, float over, double* outMedian = nullptr, double* outInside = nullptr) {
	if (values.empty())
		return {{"vertices", 0}};

	std::sort(values.begin(), values.end());
	double sum = std::accumulate(values.begin(), values.end(), 0.0);
	size_t inside = std::count_if(values.begin(), values.end(), [](float d) { return d < 0.0f; });
	size_t beyond = std::count_if(values.begin(), values.end(), [over](float d) { return d > over; });
	const double n = static_cast<double>(values.size());
	if (outMedian)
		*outMedian = Percentile(values, 0.5f);
	if (outInside)
		*outInside = inside / n;
	return {
		{"vertices", values.size()},
		{"min", Round(values.front())},
		{"p5", Round(Percentile(values, 0.05f))},
		{"median", Round(Percentile(values, 0.5f))},
		{"mean", Round(static_cast<float>(sum / n))},
		{"p95", Round(Percentile(values, 0.95f))},
		{"max", Round(values.back())},
		{"inside", Round(static_cast<float>(inside / n), 0.001f)},
		{"over", Round(static_cast<float>(beyond / n), 0.001f)},
	};
}

// Slider values of a case and its name
struct Case {
	std::string name;
	std::map<std::string, float> values;
	bool keepCurrent = false;
};

// The cases of a measurement: args["cases"], the presets of args["presets"] and args["presetFilter"], else the slider
// values of sliderValues or preset, else the current values
std::vector<Case> Cases(OutfitStudioMcp& mcp, const json& args) {
	std::vector<Case> cases;
	if (const json* list = McpArgs::Find(args, "cases")) {
		if (!list->is_array())
			McpArgs::Wrong("cases", "a list of {name, sliderValues, preset, presetWeight}");
		for (auto& item : *list) {
			if (!item.is_object())
				McpArgs::Wrong("cases", "a list of {name, sliderValues, preset, presetWeight}");
			Case c;
			c.name = item.value("name", item.value("preset", "case " + std::to_string(cases.size())));
			c.values = McpSliderValuesArg(mcp, item);
			cases.push_back(c);
		}
	}

	for (auto& preset : McpPresetCases(mcp, args, "presets", "presetFilter", "presetWeights")) {
		Case c;
		c.name = preset.first;
		for (auto& value : preset.second)
			if (value.second != 0.0f)
				c.values[value.first] = value.second;
		cases.push_back(c);
	}

	if (cases.empty()) {
		Case c;
		if (McpHasSliderValuesArg(args)) {
			c.name = McpArgs::String(args, "preset", "slider values");
			c.values = McpSliderValuesArg(mcp, args);
		}
		else {
			c.name = "current";
			c.keepCurrent = true;
		}
		cases.push_back(c);
	}
	return cases;
}

// Sets the sliders of a case for as long as it lives, with the other sliders at 0 unless zeroOthers is false
class CaseScope {
public:
	CaseScope(OutfitProject& project, const Case& c, bool zeroOthers)
		: scope(project) {
		if (c.keepCurrent)
			return;
		if (zeroOthers)
			scope.Zero();
		scope.Set(c.values);
	}

private:
	McpSliderValueScope scope;
};

std::vector<Vector3> LiveVerts(OutfitProject& project, NiShape* shape) {
	std::vector<Vector3> verts;
	project.GetLiveVerts(shape, verts);
	return verts;
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

std::vector<float> SignedGaps(const std::vector<Vector3>& verts, const std::vector<Vector3>& surfaceVerts, const std::vector<Triangle>& surfaceTris, float radius) {
	return MeshAnalysis::AnalyzePenetration(verts, surfaceVerts, surfaceTris, 0.1f, radius).signedDistances;
}

// Median gap per step along an axis, e.g. down the middle of a cleavage
json Profile(const std::vector<float>& gaps, const std::vector<Vector3>& rest, const std::vector<size_t>& vertices, int axis, float step) {
	std::map<int, std::vector<float>> bins;
	for (size_t v : vertices) {
		float at = axis == 0 ? rest[v].x : axis == 1 ? rest[v].y : rest[v].z;
		bins[static_cast<int>(std::floor(at / step))].push_back(gaps[v]);
	}

	json profile = json::array();
	for (auto& bin : bins) {
		std::vector<float> sorted = bin.second;
		std::sort(sorted.begin(), sorted.end());
		profile.push_back({{"at", Round((bin.first + 0.5f) * step)}, {"median", Round(Percentile(sorted, 0.5f))}, {"vertices", sorted.size()}});
	}
	return profile;
}

int AxisArg(const json& args, const char* name, int def) {
	std::string axis = McpArgs::String(args, name, std::string(1, "xyz"[def]));
	if (axis == "x")
		return 0;
	if (axis == "y")
		return 1;
	if (axis == "z")
		return 2;
	McpArgs::Wrong(name, "\"x\", \"y\" or \"z\"");
}

McpToolResult MeasureGap(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	auto shapes = mcp.Shapes(args, "shapes", McpArgs::Has(args, "against"));
	auto regions = Regions(args);
	auto cases = Cases(mcp, args);
	const bool zeroOthers = McpArgs::Bool(args, "zeroOthers", true);
	const float radius = McpArgs::Float(args, "maxDistance", 5.0f);
	const float over = McpArgs::Float(args, "overDistance", 1.0f);
	const float targetGap = McpArgs::Float(args, "targetGap", 0.2f);
	const std::string which = McpArgs::String(args, "vertices", "all");

	const json* profileArg = McpArgs::Find(args, "profile");
	int profileAxis = 2;
	float profileStep = 1.0f;
	if (profileArg) {
		if (!profileArg->is_object())
			McpArgs::Wrong("profile", "{axis, step}");
		profileAxis = AxisArg(*profileArg, "axis", 2);
		profileStep = std::max(0.01f, McpArgs::Float(*profileArg, "step", 1.0f));
	}

	// Which vertices each region has, from the rest positions
	struct ShapeRegions {
		NiShape* shape = nullptr;
		NiShape* against = nullptr;
		std::vector<Vector3> rest;
		std::vector<std::vector<size_t>> vertices;
	};
	std::vector<ShapeRegions> measured;
	for (auto* shape : shapes) {
		ShapeRegions sr;
		sr.shape = shape;
		sr.against = McpAgainstShape(mcp, args, shape);
		sr.rest = RestVerts(project, shape);
		std::vector<bool> use = MaskFilter(mcp, shape, sr.rest.size(), which);
		for (auto& region : regions) {
			sr.vertices.emplace_back();
			for (size_t v = 0; v < sr.rest.size(); v++)
				if (use[v] && region.Contains(sr.rest[v]))
					sr.vertices.back().push_back(v);
		}
		measured.push_back(std::move(sr));
	}

	json caseList = json::array();
	std::vector<std::pair<double, std::string>> ranking;
	for (auto& c : cases) {
		CaseScope scope(project, c, zeroOthers);
		json shapesJson = json::object();
		double score = 0.0;
		size_t scored = 0;
		for (auto& sr : measured) {
			auto verts = LiveVerts(project, sr.shape);
			auto gaps = SignedGaps(verts, LiveVerts(project, sr.against), mcp.SurfaceTriangles(sr.against), radius);

			json regionsJson = json::object();
			for (size_t r = 0; r < regions.size(); r++) {
				std::vector<float> values;
				std::vector<size_t> measuredVertices;
				for (size_t v : sr.vertices[r])
					if (v < gaps.size() && gaps[v] != FLT_MAX) {
						values.push_back(gaps[v]);
						measuredVertices.push_back(v);
					}

				double median = 0.0, inside = 0.0;
				json stats = Distribution(values, over, &median, &inside);
				if (!values.empty()) {
					score += std::fabs(median - targetGap) + inside;
					scored++;
				}
				if (profileArg && !values.empty())
					stats["profile"] = Profile(gaps, sr.rest, measuredVertices, profileAxis, profileStep);
				regionsJson[regions[r].name] = stats;
			}
			shapesJson[sr.shape->name.get()] = regionsJson;
		}

		json entry = {{"case", c.name}, {"shapes", shapesJson}};
		if (!c.keepCurrent)
			entry["sliderValues"] = c.values;
		if (scored) {
			entry["score"] = Round(static_cast<float>(score / scored), 0.001f);
			ranking.emplace_back(score / scored, c.name);
		}
		caseList.push_back(entry);
	}

	McpToolResult result;
	result.data["cases"] = caseList;
	if (cases.size() > 1) {
		std::sort(ranking.begin(), ranking.end());
		json rank = json::array();
		for (auto& r : ranking)
			rank.push_back({{"case", r.second}, {"score", Round(static_cast<float>(r.first), 0.001f)}});
		result.data["ranking"] = rank;
	}
	return result;
}

// The arguments of the source side of compare_shapes, by the names the slider value helpers take
json SourceValueArgs(const json& args) {
	json source = json::object();
	if (McpArgs::Has(args, "sourceSliderValues"))
		source["sliderValues"] = args["sourceSliderValues"];
	if (McpArgs::Has(args, "sourcePreset"))
		source["preset"] = args["sourcePreset"];
	if (McpArgs::Has(args, "sourcePresetWeight"))
		source["presetWeight"] = args["sourcePresetWeight"];
	return source;
}

Case SideCase(OutfitStudioMcp& mcp, const json& values) {
	Case c;
	if (McpHasSliderValuesArg(values))
		c.values = McpSliderValuesArg(mcp, values);
	else
		c.keepCurrent = true;
	return c;
}

// Rotation angle of a rotation matrix, in degrees
float RotationDegrees(const double rotation[3][3]) {
	double trace = rotation[0][0] + rotation[1][1] + rotation[2][2];
	double cosine = std::clamp((trace - 1.0) / 2.0, -1.0, 1.0);
	return static_cast<float>(std::acos(cosine) * 180.0 / 3.14159265358979323846);
}

McpToolResult CompareShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	NiShape* source = mcp.Shape(McpArgs::String(args, "source"));
	if (shape == source)
		throw McpToolError("Name another shape in 'source', e.g. the outfit from the source file imported next to it, or a duplicate from before a fit.");

	auto regions = Regions(args);
	const bool zeroOthers = McpArgs::Bool(args, "zeroOthers", true);
	const float radius = McpArgs::Float(args, "maxDistance", 5.0f);
	const float threshold = McpArgs::Float(args, "threshold", 0.3f);
	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 15), 0, 1000));
	const bool allowScale = McpArgs::Bool(args, "allowScale", false);
	const int minPartVertices = std::max(1, McpArgs::Int(args, "minPartVertices", 3));

	NiShape* body = nullptr;
	if (McpArgs::Has(args, "body"))
		body = mcp.Shape(McpArgs::String(args, "body"));
	else
		body = project.GetBaseShape();
	NiShape* sourceBody = McpArgs::Has(args, "sourceBody") ? mcp.Shape(McpArgs::String(args, "sourceBody")) : body;
	NiShape* anchor = McpArgs::Has(args, "anchorTo") ? mcp.Shape(McpArgs::String(args, "anchorTo")) : nullptr;
	NiShape* sourceAnchor = McpArgs::Has(args, "sourceAnchorTo") ? mcp.Shape(McpArgs::String(args, "sourceAnchorTo")) : anchor;
	if (McpArgs::Has(args, "sourceAnchorTo") && !anchor)
		throw McpToolError("sourceAnchorTo goes with anchorTo: name the shape the parts are anchored to now.");

	Case sourceCase = SideCase(mcp, SourceValueArgs(args));
	Case currentCase = SideCase(mcp, args);

	// The source side first, at its slider values
	std::vector<Vector3> sourceVerts, sourceBodyVerts, sourceAnchorVerts;
	{
		CaseScope scope(project, sourceCase, zeroOthers);
		sourceVerts = LiveVerts(project, source);
		if (sourceBody)
			sourceBodyVerts = LiveVerts(project, sourceBody);
		if (sourceAnchor)
			sourceAnchorVerts = LiveVerts(project, sourceAnchor);
	}
	std::vector<Vector3> verts, bodyVerts, anchorVerts;
	{
		CaseScope scope(project, currentCase, zeroOthers);
		verts = LiveVerts(project, shape);
		if (body)
			bodyVerts = LiveVerts(project, body);
		if (anchor)
			anchorVerts = LiveVerts(project, anchor);
	}

	if (verts.size() != sourceVerts.size())
		throw McpToolError("'" + shape->name.get() + "' has " + std::to_string(verts.size()) + " vertices and '" + source->name.get() + "' " +
						   std::to_string(sourceVerts.size()) + ". Compare the same mesh, e.g. import the source file next to the outfit.");
	if (anchor && anchorVerts.size() != sourceAnchorVerts.size())
		throw McpToolError("'" + anchor->name.get() + "' and '" + sourceAnchor->name.get() + "' need the same vertex count for anchorTo.");

	std::vector<Vector3> rest = RestVerts(project, shape);
	std::vector<Triangle> tris = Triangles(shape);
	std::vector<bool> use = MaskFilter(mcp, shape, verts.size(), McpArgs::String(args, "vertices", "all"));

	// How far each vertex is from where it is in the source, and how its gap to the body changed
	std::vector<float> moved(verts.size());
	for (size_t v = 0; v < verts.size(); v++)
		moved[v] = (verts[v] - sourceVerts[v]).length();

	std::vector<float> gapChange;
	if (body && sourceBody) {
		auto gaps = SignedGaps(verts, bodyVerts, mcp.SurfaceTriangles(body), radius);
		auto sourceGaps = SignedGaps(sourceVerts, sourceBodyVerts, mcp.SurfaceTriangles(sourceBody), radius);
		gapChange.assign(verts.size(), FLT_MAX);
		for (size_t v = 0; v < verts.size(); v++)
			if (gaps[v] != FLT_MAX && sourceGaps[v] != FLT_MAX)
				gapChange[v] = gaps[v] - sourceGaps[v];
	}

	json regionsJson = json::object();
	for (auto& region : regions) {
		std::vector<float> moves, changes;
		size_t tighter = 0, looser = 0;
		for (size_t v = 0; v < verts.size(); v++) {
			if (!use[v] || !region.Contains(rest[v]))
				continue;
			moves.push_back(moved[v]);
			if (!gapChange.empty() && gapChange[v] != FLT_MAX) {
				changes.push_back(gapChange[v]);
				tighter += gapChange[v] < -threshold ? 1 : 0;
				looser += gapChange[v] > threshold ? 1 : 0;
			}
		}

		json entry = json::object();
		json move = Distribution(moves, threshold);
		move.erase("inside");
		move.erase("min");
		entry["moved"] = move;
		if (!changes.empty()) {
			json change = Distribution(changes, threshold);
			change.erase("inside");
			change.erase("over");
			change["tighter"] = tighter;
			change["looser"] = looser;
			entry["gapChange"] = change;
		}
		regionsJson[region.name] = entry;
	}

	McpToolResult result;
	result.data["regions"] = regionsJson;

	// Each part: how it moved as a whole, how much it bent, and with anchorTo the move that gives it its offset to the
	// anchor from the source back
	if (McpArgs::Bool(args, "parts", true) && limit > 0) {
		// Numbered like list_mesh_parts, so mask_ops parts takes them
		int partCount = 0;
		std::vector<int> parts = MeshAnalysis::ConnectedParts(rest, tris, McpArgs::Float(args, "weldDistance", 0.001f), nullptr, &partCount);
		std::vector<std::vector<size_t>> partVertices(partCount);
		for (size_t v = 0; v < parts.size(); v++)
			if (parts[v] >= 0 && use[v])
				partVertices[parts[v]].push_back(v);

		const float contactDistance = McpArgs::Float(args, "contactDistance", 1.0f);
		std::vector<Triangle> anchorTris;
		if (anchor)
			anchorTris = Triangles(sourceAnchor);

		struct PartRow {
			int part;
			float bend;
			json entry;
		};
		std::vector<PartRow> rows;
		for (int p = 0; p < partCount; p++) {
			auto& pv = partVertices[p];
			if (static_cast<int>(pv.size()) < minPartVertices)
				continue;

			std::vector<Vector3> from, to;
			for (size_t v : pv) {
				from.push_back(sourceVerts[v]);
				to.push_back(verts[v]);
			}
			MeshAnalysis::SimilarityTransform fit = MeshAnalysis::BestFitTransform(from, to, nullptr, allowScale);
			double squared = 0.0;
			float worst = 0.0f;
			Vector3 fromCenter, toCenter;
			for (size_t i = 0; i < from.size(); i++) {
				float residual = (fit.Apply(from[i]) - to[i]).length();
				squared += residual * residual;
				worst = std::max(worst, residual);
				fromCenter += from[i];
				toCenter += to[i];
			}
			fromCenter /= static_cast<float>(from.size());
			toCenter /= static_cast<float>(from.size());
			const float bend = static_cast<float>(std::sqrt(squared / from.size()));

			json entry = {
				{"part", p},
				{"vertices", pv.size()},
				{"move", VectorJson(toCenter - fromCenter)},
				{"turn", Round(RotationDegrees(fit.rotation), 0.1f)},
				{"bend", Round(bend, 0.001f)},
				{"maxBend", Round(worst, 0.001f)},
			};
			if (allowScale)
				entry["scale"] = Round(static_cast<float>(fit.scale), 0.001f);

			if (anchor) {
				auto closest = MeshAnalysis::ClosestSurfacePoints(sourceAnchorVerts, anchorTris, from, contactDistance);
				Vector3 sum;
				std::vector<Vector3> needs;
				for (size_t i = 0; i < closest.size(); i++) {
					auto& sp = closest[i];
					if (sp.triangle < 0)
						continue;
					const Triangle& t = anchorTris[sp.triangle];
					auto at = [&](const std::vector<Vector3>& a) { return a[t.p1] * sp.u + a[t.p2] * sp.v + a[t.p3] * sp.w; };
					// The offset to the anchor's surface point it had in the source, against the one it has now
					Vector3 need = (from[i] - at(sourceAnchorVerts)) - (to[i] - at(anchorVerts));
					needs.push_back(need);
					sum += need;
				}
				if (!needs.empty()) {
					Vector3 mean = sum / static_cast<float>(needs.size());
					double spread = 0.0;
					for (auto& n : needs)
						spread += (n - mean).length2();
					entry["anchor"] = {
						{"contactVertices", needs.size()},
						{"restoreMove", VectorJson(mean)},
						{"spread", Round(static_cast<float>(std::sqrt(spread / needs.size())), 0.001f)},
					};
				}
			}
			rows.push_back({p, bend, entry});
		}

		std::sort(rows.begin(), rows.end(), [](const PartRow& a, const PartRow& b) { return a.bend > b.bend; });
		json list = json::array();
		for (size_t i = 0; i < rows.size() && i < limit; i++)
			list.push_back(rows[i].entry);
		result.data["parts"] = list;
		result.data["partCount"] = rows.size();
		if (rows.size() > limit)
			result.data["moreParts"] = rows.size() - limit;
	}
	return result;
}

// Main direction of points: the eigenvector of the largest eigenvalue of their covariance
Vector3 MainAxis(const std::vector<Vector3>& points, const Vector3& center) {
	double c[3][3] = {};
	for (auto& p : points) {
		const double d[3] = {p.x - center.x, p.y - center.y, p.z - center.z};
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				c[i][j] += d[i] * d[j];
	}

	double axis[3] = {1.0, 1.0, 1.0};
	for (int iteration = 0; iteration < 50; iteration++) {
		double next[3] = {};
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				next[i] += c[i][j] * axis[j];
		double length = std::sqrt(next[0] * next[0] + next[1] * next[1] + next[2] * next[2]);
		if (length < 1e-12)
			break;
		for (int i = 0; i < 3; i++)
			axis[i] = next[i] / length;
	}
	return Vector3(static_cast<float>(axis[0]), static_cast<float>(axis[1]), static_cast<float>(axis[2]));
}

McpToolResult AnalyzeStrap(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	auto cases = Cases(mcp, args);
	const bool zeroOthers = McpArgs::Bool(args, "zeroOthers", true);
	const float endLength = std::max(0.01f, McpArgs::Float(args, "endLength", 0.5f));
	const int sections = std::clamp(McpArgs::Int(args, "sections", 10), 3, 100);
	const float radius = McpArgs::Float(args, "maxDistance", 5.0f);

	// The strap's vertices: a part of list_mesh_parts, the unmasked ones, or the whole shape
	std::vector<Vector3> rest = RestVerts(project, shape);
	std::vector<size_t> strap;
	if (McpArgs::Has(args, "part")) {
		const int part = McpArgs::Int(args, "part", 0);
		int partCount = 0;
		std::vector<int> parts = MeshAnalysis::ConnectedParts(rest, Triangles(shape), McpArgs::Float(args, "weldDistance", 0.001f), nullptr, &partCount);
		if (part < 0 || part >= partCount)
			throw McpToolError("'" + shape->name.get() + "' has parts 0 to " + std::to_string(partCount - 1) + ". list_mesh_parts lists them.");
		for (size_t v = 0; v < parts.size(); v++)
			if (parts[v] == part)
				strap.push_back(v);
	}
	else {
		std::vector<bool> use = MaskFilter(mcp, shape, rest.size(), McpArgs::String(args, "vertices", "unmasked"));
		for (size_t v = 0; v < use.size(); v++)
			if (use[v])
				strap.push_back(v);
	}
	if (strap.size() < 3)
		throw McpToolError("The strap has fewer than 3 vertices. Name its part, or mask everything else.");

	std::vector<NiShape*> joins;
	if (McpArgs::Has(args, "joins")) {
		for (auto& name : McpArgs::Strings(args, "joins"))
			joins.push_back(mcp.Shape(name));
	}
	else {
		for (auto* other : project.GetWorkNif()->GetShapes())
			if (other != shape && !project.IsBaseShape(other))
				joins.push_back(other);
	}

	json caseList = json::array();
	for (auto& c : cases) {
		CaseScope scope(project, c, zeroOthers);
		std::vector<Vector3> all = LiveVerts(project, shape);
		std::vector<Vector3> points;
		Vector3 center;
		for (size_t v : strap) {
			points.push_back(all[v]);
			center += all[v];
		}
		center /= static_cast<float>(points.size());

		// Along its main axis: the length, the centre of each section and the ends
		Vector3 axis = MainAxis(points, center);
		std::vector<float> along(points.size());
		float low = FLT_MAX, high = -FLT_MAX;
		for (size_t i = 0; i < points.size(); i++) {
			along[i] = (points[i] - center).dot(axis);
			low = std::min(low, along[i]);
			high = std::max(high, along[i]);
		}
		const float length = high - low;

		std::vector<Vector3> sectionSum(sections);
		std::vector<int> sectionCount(sections, 0);
		for (size_t i = 0; i < points.size(); i++) {
			int s = length > 0.0f ? std::min(sections - 1, static_cast<int>((along[i] - low) / length * sections)) : 0;
			sectionSum[s] += points[i];
			sectionCount[s]++;
		}
		std::vector<std::pair<float, Vector3>> centers;
		for (int s = 0; s < sections; s++)
			if (sectionCount[s])
				centers.emplace_back((s + 0.5f) / sections, sectionSum[s] / static_cast<float>(sectionCount[s]));

		// Bow: how far the section centres are off the straight line between the end sections
		json entry = {{"case", c.name}, {"length", Round(length)}};
		if (centers.size() >= 3) {
			Vector3 a = centers.front().second, b = centers.back().second;
			Vector3 chord = b - a;
			float chordLength = chord.length();
			float bow = 0.0f, bowAt = 0.0f;
			Vector3 bowOffset;
			for (size_t i = 1; i + 1 < centers.size(); i++) {
				Vector3 p = centers[i].second - a;
				float t = chordLength > 0.0f ? p.dot(chord) / (chordLength * chordLength) : 0.0f;
				Vector3 offset = p - chord * t;
				if (offset.length() > bow) {
					bow = offset.length();
					bowAt = centers[i].first;
					bowOffset = offset;
				}
			}
			entry["bow"] = Round(bow, 0.001f);
			entry["bowAt"] = Round(bowAt);
			entry["bowOffset"] = VectorJson(bowOffset);
		}

		// The gap of each end to the shapes it joins, the closest of them
		json ends = json::array();
		for (int side = 0; side < 2; side++) {
			std::vector<Vector3> end;
			for (size_t i = 0; i < points.size(); i++)
				if (side == 0 ? along[i] <= low + endLength : along[i] >= high - endLength)
					end.push_back(points[i]);

			json endEntry = {{"vertices", end.size()}, {"position", VectorJson(std::accumulate(end.begin(), end.end(), Vector3()) / static_cast<float>(std::max<size_t>(1, end.size())))}};
			float best = FLT_MAX;
			std::string bestShape;
			for (auto* other : joins) {
				auto closest = MeshAnalysis::ClosestSurfacePoints(LiveVerts(project, other), Triangles(other), end, radius);
				for (auto& sp : closest)
					if (sp.triangle >= 0 && sp.distance < best) {
						best = sp.distance;
						bestShape = other->name.get();
					}
			}
			if (!bestShape.empty()) {
				endEntry["gap"] = Round(best, 0.001f);
				endEntry["to"] = bestShape;
			}
			ends.push_back(endEntry);
		}
		entry["ends"] = ends;
		if (!c.keepCurrent)
			entry["sliderValues"] = c.values;
		caseList.push_back(entry);
	}

	McpToolResult result;
	result.data["strapVertices"] = strap.size();
	result.data["cases"] = caseList;
	return result;
}

json RegionsProp() {
	return {
		{"type", "array"},
		{"items",
		 {{"type", "object"},
		  {"properties",
		   {{"name", {{"type", "string"}}},
			{"min", {{"type", "array"}, {"items", {{"type", "number"}}}}},
			{"max", {{"type", "array"}, {"items", {{"type", "number"}}}}},
			{"mirrorX", {{"type", "boolean"}}}}}}},
		{"description",
		 "Boxes of the shape at rest (all sliders at 0) to measure on their own, e.g. [{\"name\": \"front\", \"min\": [-7, 3, -31], \"max\": [7, 20, -16]}, "
		 "{\"name\": \"sides\", \"min\": [7, -3, -31], \"max\": [14, 20, -16], \"mirrorX\": true}]. mirrorX also takes the box's mirror image on the "
		 "other side. Default the whole shape."},
	};
}

json CasesProp() {
	return {
		{"type", "array"},
		{"items",
		 {{"type", "object"},
		  {"properties",
		   {{"name", {{"type", "string"}}},
			{"sliderValues", {{"type", "object"}, {"additionalProperties", {{"type", "number"}}}}},
			{"preset", {{"type", "string"}}},
			{"presetWeight", {{"type", "string"}, {"enum", {"low", "high"}}}}}}}},
		{"description", "Slider values to measure at, each case on its own, e.g. [{\"name\": \"small\", \"sliderValues\": {\"Breasts\": -0.5}}, {\"preset\": \"CBBE Curvy\"}]"},
	};
}

} // namespace

void AddMeasureTools(OutfitStudioMcp& mcp) {
	const json presetProperties = McpPresetValueProperties();
	const json verticesProp = {
		{"type", "string"},
		{"enum", {"all", "masked", "unmasked"}},
		{"description", "Which vertices count: all (default), only the masked or only the unmasked ones"},
	};

	McpTool gap;
	gap.name = "measure_gap";
	gap.description =
		"How far shapes are from the reference (or 'against'), per region: the signed gap of their vertices (negative inside) "
		"as min, 5th percentile, median, mean, 95th percentile and max, and the share inside and further than overDistance. "
		"Regions are boxes of the shapes at rest, so they hold the same vertices in every case. Measured at the current slider "
		"values, at sliderValues or a preset, or for several cases at once: cases, presets or presetFilter. With more than one "
		"case, ranking sorts them by how close they bring the median gaps to targetGap without clipping (score: the mean over "
		"shapes and regions of |median - targetGap| + share inside), e.g. to find the body preset an outfit was made for, or "
		"the slider value a variant fits at. profile gives the median gap step by step along an axis, e.g. down the middle "
		"of a cleavage. A shape can be measured against another (the reference against a filled-cleavage shape, a layer "
		"against the one under it). Slider values are put back afterwards.";
	gap.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to measure. Default the selected outfit shapes; with 'against' the reference can be one.")},
		{"against", McpProp("string", "Shape to measure the gap to. Default the reference.")},
		{"regions", RegionsProp()},
		{"vertices", verticesProp},
		{"sliderValues", McpNumberMap("Slider values to measure at, the others at 0 unless zeroOthers is false")},
		{"cases", CasesProp()},
		{"presets", McpStringList("Body presets to measure at, by name, each a case")},
		{"presetFilter", McpProp("string", "Also every preset whose name contains this text, ignoring case")},
		{"presetWeights", {{"type", "array"}, {"items", {{"type", "string"}, {"enum", {"low", "high"}}}}, {"description", "Weights of the presets. Default both."}}},
		{"zeroOthers", McpProp("boolean", "Set the sliders a case doesn't give to 0. Default true.")},
		{"targetGap", McpProp("number", "The gap the ranking looks for. Default 0.2.")},
		{"overDistance", McpProp("number", "Gap that counts as loose, for the 'over' share. Default 1.")},
		{"maxDistance", McpProp("number", "Vertices farther from the surface aren't counted (hanging parts). Default 5.")},
		{"profile",
		 {{"type", "object"},
		  {"properties", {{"axis", {{"type", "string"}, {"enum", {"x", "y", "z"}}}}, {"step", {{"type", "number"}}}}},
		  {"description", "Median gap per step along an axis in each region, e.g. {\"axis\": \"z\", \"step\": 1}"}}},
	});
	for (auto& property : presetProperties.items())
		gap.inputSchema["properties"][property.key()] = property.value();
	gap.readOnly = true;
	gap.handler = [&mcp](const json& args) { return MeasureGap(mcp, args); };
	mcp.AddTool(std::move(gap));

	McpTool compare;
	compare.name = "compare_shapes";
	compare.description =
		"Compares a shape with another of the same vertex count, its source: the outfit from the source file imported next "
		"to it, a duplicate from before a fit, or the same mesh from the other game's conversion. Per region it reports how "
		"far the vertices moved and how their gap to the body changed (gap to 'body' now minus gap to 'sourceBody' in the "
		"source, so the source can be measured on another body, like the old body of a cross-game conversion). Per connected "
		"part, worst first: how it moved and turned as a whole and how much it bent (what's left after the best rigid fit), "
		"which finds trim, wires and gems that a conversion warped. With anchorTo, each part also gets the move that gives it "
		"back the offset it had to that shape in the source (restoreMove; spread says how much its vertices disagree): a "
		"piercing to the body where it goes through, a strap's ends to the cups they join. Apply it with mask_ops parts and "
		"transform_shapes move. The two sides can be at different slider values: sourceSliderValues or sourcePreset for the "
		"source (e.g. the body preset the source was made on), sliderValues or preset for the shape. Slider values are put back afterwards.";
	compare.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "The shape to check")},
			{"source", McpProp("string", "The shape to compare it with, with the same vertices")},
			{"body", McpProp("string", "Shape the gap of 'shape' is measured to. Default the reference.")},
			{"sourceBody", McpProp("string", "Shape the gap of 'source' is measured to. Default 'body'.")},
			{"anchorTo", McpProp("string", "Per part: the move that restores the part's offset to this shape from the source")},
			{"sourceAnchorTo", McpProp("string", "The anchor shape on the source side, with the same vertices as anchorTo. Default anchorTo.")},
			{"contactDistance", McpProp("number", "anchorTo: part vertices this close to the anchor in the source count. Default 1.")},
			{"regions", RegionsProp()},
			{"vertices", verticesProp},
			{"sliderValues", McpNumberMap("Slider values for the shape's side")},
			{"sourceSliderValues", McpNumberMap("Slider values for the source's side")},
			{"sourcePreset", McpProp("string", "Body preset for the source's side")},
			{"sourcePresetWeight", {{"type", "string"}, {"enum", {"low", "high"}}, {"description", "Default high."}}},
			{"zeroOthers", McpProp("boolean", "Set the sliders a side doesn't give to 0. Default true.")},
			{"threshold", McpProp("number", "Gap change that counts as tighter or looser, and move that counts as 'over'. Default 0.3.")},
			{"parts", McpProp("boolean", "Report the connected parts. Default true.")},
			{"allowScale", McpProp("boolean", "Parts may also grow or shrink evenly in the rigid fit. Default false.")},
			{"minPartVertices", McpProp("integer", "Leave out smaller parts. Default 3.")},
			{"weldDistance", McpProp("number", "Vertices closer than this count as joined. Default 0.001.")},
			{"limit", McpProp("integer", "How many parts to return. Default 15.")},
			{"maxDistance", McpProp("number", "Vertices farther from the body don't count for the gap. Default 5.")},
		},
		{"shape", "source"});
	for (auto& property : presetProperties.items())
		compare.inputSchema["properties"][property.key()] = property.value();
	compare.readOnly = true;
	compare.handler = [&mcp](const json& args) { return CompareShapes(mcp, args); };
	mcp.AddTool(std::move(compare));

	McpTool strap;
	strap.name = "analyze_strap";
	strap.description =
		"Measures a strap, chain or bar: its length along its main direction, how far it bows (the largest distance of its "
		"section centres from the straight line between its ends, where along it from 0 to 1, and the offset there) and the "
		"gap of each end to the closest of the shapes it joins. The strap is a part of list_mesh_parts, or the unmasked "
		"vertices of the shape. Measured at the current slider values, or per case (cases, presets, sliderValues), e.g. to "
		"check that a bikini's centre bar stays straight and on the cups with every preset. Slider values are put back afterwards.";
	strap.inputSchema = McpSchema(
		{
			{"shape", McpProp("string", "Shape with the strap")},
			{"part", McpProp("integer", "The strap's part number of list_mesh_parts. Default the unmasked vertices.")},
			{"vertices", verticesProp},
			{"joins", McpStringList("Shapes the ends join. Default the other outfit shapes.")},
			{"endLength", McpProp("number", "How far from each end along the strap counts as its end. Default 0.5.")},
			{"sections", McpProp("integer", "Sections along the strap for the bow. Default 10.")},
			{"sliderValues", McpNumberMap("Slider values to measure at")},
			{"cases", CasesProp()},
			{"presets", McpStringList("Body presets to measure at, each a case")},
			{"presetFilter", McpProp("string", "Also every preset whose name contains this text")},
			{"presetWeights", {{"type", "array"}, {"items", {{"type", "string"}, {"enum", {"low", "high"}}}}, {"description", "Default both."}}},
			{"zeroOthers", McpProp("boolean", "Set the sliders a case doesn't give to 0. Default true.")},
			{"weldDistance", McpProp("number", "With part: vertices closer than this count as joined. Default 0.001.")},
			{"maxDistance", McpProp("number", "Ends farther from the joined shapes have no gap. Default 5.")},
		},
		{"shape"});
	for (auto& property : presetProperties.items())
		strap.inputSchema["properties"][property.key()] = property.value();
	strap.readOnly = true;
	strap.handler = [&mcp](const json& args) { return AnalyzeStrap(mcp, args); };
	mcp.AddTool(std::move(strap));
}
