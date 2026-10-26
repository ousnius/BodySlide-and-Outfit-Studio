/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/MaskOps.h"
#include "../program/AutomationRunner.h"
#include "../program/OutfitStudio.h"

#include <wx/filename.h>

#include <algorithm>
#include <cfloat>
#include <optional>
#include <set>
#include <sstream>

using nlohmann::json;
using namespace nifly;

namespace {

json MorphToJson(const MeshAnalysis::MorphStats& stats) {
	return {
		{"moved", stats.movedCount},
		{"meanDelta", stats.meanDelta},
		{"maxDelta", stats.maxDelta},
		{"roughness", stats.roughness},
		{"normalizedRoughness", stats.normalizedRoughness},
		{"maxBoundaryRoughness", stats.maxBoundaryRoughness},
	};
}

// The deepest vertices of a penetration analysis, deepest first
json DeepestVertices(const MeshAnalysis::PenetrationStats& stats, size_t count) {
	std::vector<std::pair<float, int>> inside;
	for (size_t v = 0; v < stats.signedDistances.size(); v++)
		if (stats.signedDistances[v] < 0.0f)
			inside.emplace_back(stats.signedDistances[v], static_cast<int>(v));

	std::sort(inside.begin(), inside.end());
	if (inside.size() > count)
		inside.resize(count);

	json list = json::array();
	for (auto& v : inside)
		list.push_back({{"vertex", v.second}, {"depth", -v.first}});
	return list;
}

json PenetrationToJson(const MeshAnalysis::PenetrationStats& stats) {
	return {
		{"penetrating", stats.penetrating},
		{"nearSurface", stats.nearSurface},
		{"maxDepth", stats.maxDepth},
		{"meanDepth", stats.meanDepth},
	};
}

// Clipping of a shape into another shape, the reference by default, as both are shown now. The body poking out
// between the shape's vertices counts for the vertices of the triangles it goes through, outPokes receives how
// much. Vertices of analysis_exclusions are left out, outExcluded receives how many.
MeshAnalysis::PenetrationStats Clipping(OutfitStudioMcp& mcp, NiShape* shape, NiShape* against, float nearDistance, size_t* outExcluded = nullptr, McpPokeStats* outPokes = nullptr) {
	OutfitProject& project = mcp.Project();
	std::vector<Vector3> verts, surfaceVerts;
	project.GetLiveVerts(shape, verts);
	project.GetLiveVerts(against, surfaceVerts);
	std::vector<Triangle> tris;
	against->GetTriangles(tris);
	MeshAnalysis::PenetrationStats stats = MeshAnalysis::AnalyzePenetration(verts, surfaceVerts, tris, nearDistance);
	McpDropHiddenSurface(mcp, against, surfaceVerts, verts, stats.signedDistances);
	McpPokeStats pokes = McpMergePokeThrough(mcp, shape, verts, against, surfaceVerts, stats.signedDistances);
	if (outPokes)
		*outPokes = pokes;
	size_t excluded = McpApplyExclusions(mcp, shape, stats, nearDistance);
	if (outExcluded)
		*outExcluded = excluded;
	return stats;
}

// Sliders whose name says they go both ways, like "BreastsPositionWidth n|p": BodySlide shows them from -100 to 100
bool Bidirectional(const std::string& slider) {
	return slider.find("n|p") != std::string::npos || slider.find("p|n") != std::string::npos;
}

std::string ValueText(float value) {
	std::ostringstream text;
	text << value;
	return text.str();
}

McpToolResult AnalyzeClipping(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	auto shapes = mcp.Shapes(args, "shapes");
	const float nearDistance = McpArgs::Float(args, "nearDistance", 0.1f);
	const size_t worst = static_cast<size_t>(std::clamp(McpArgs::Int(args, "worstVertices", 10), 0, 1000));

	McpSliderValueScope sliders(project);
	if (McpArgs::Bool(args, "zeroOthers", true) && McpHasSliderValuesArg(args))
		sliders.Zero();
	sliders.Set(McpSliderValuesArg(mcp, args));

	json result = json::object();
	for (auto* shape : shapes) {
		size_t excluded = 0;
		McpPokeStats pokes;
		MeshAnalysis::PenetrationStats stats = Clipping(mcp, shape, McpAgainstShape(mcp, args, shape), nearDistance, &excluded, &pokes);

		json entry = PenetrationToJson(stats);
		entry["vertices"] = stats.signedDistances.size();
		if (excluded)
			entry["excluded"] = excluded;
		if (pokes.bodyVertices)
			entry["pokeThrough"] = {{"bodyVertices", pokes.bodyVertices}, {"maxDepth", pokes.maxDepth}};
		size_t hidden = 0;
		mcp.SurfaceTriangles(McpAgainstShape(mcp, args, shape), &hidden);
		if (hidden)
			entry["hiddenByZaps"] = {{"zaps", mcp.ActiveAnalysisZaps()}, {"triangles", hidden}};
		if (worst)
			entry["deepest"] = DeepestVertices(stats, worst);
		result[shape->name.get()] = entry;
	}

	McpToolResult tool;
	tool.data["shapes"] = result;
	return tool;
}

// Everything the sweep measures for one shape with one set of slider values
struct SweepCase {
	std::string name;
	std::map<std::string, float> values;
	std::string slider; // The slider measured on its own, empty for combinations
	float sliderValue = 1.0f;
	bool rest = false; // The shapes with all sliders at 0, fixed in the base geometry
};

// Presets set sliders beyond 0 to 1 (e.g. BreastsZero -1.5, BreastUnderCurve 2), which the single sliders at 1
// don't cover: each slider gets a case on its own at the most extreme values the presets use, unless singles
// has it already
void AddPresetExtremeCases(OutfitProject& project,
						   const std::vector<std::pair<std::string, std::map<std::string, float>>>& presets,
						   std::set<std::pair<std::string, float>>& singles,
						   std::vector<SweepCase>& cases) {
	std::map<std::string, std::pair<float, float>> extremes;
	for (auto& preset : presets) {
		for (auto& v : preset.second) {
			size_t index = 0;
			if (!project.SliderIndexFromName(v.first, index) || project.SliderZap(index) || project.SliderUV(index))
				continue;
			auto it = extremes.try_emplace(v.first, v.second, v.second).first;
			it->second.first = std::min(it->second.first, v.second);
			it->second.second = std::max(it->second.second, v.second);
		}
	}
	for (auto& e : extremes) {
		for (float extreme : {e.second.first, e.second.second}) {
			if ((extreme >= 0.0f && extreme <= 1.0f) || singles.count({e.first, extreme}))
				continue;
			singles.insert({e.first, extreme});
			cases.push_back({e.first + " = " + ValueText(extreme) + " (preset range)", {{e.first, extreme}}, e.first, extreme});
		}
	}
}

McpToolResult QualitySweep(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();

	NiShape* baseShape = project.GetBaseShape();
	if (!baseShape)
		throw McpToolError("No reference shape is loaded, the sweep measures clipping against it.");

	auto shapes = mcp.Shapes(args, "shapes");
	const float value = McpArgs::Float(args, "value", 1.0f);
	const float nearDistance = McpArgs::Float(args, "nearDistance", 0.1f);
	const float gapDistance = McpArgs::Float(args, "gapDistance", 2.0f);
	const float gapThreshold = McpArgs::Float(args, "gapThreshold", 0.3f);
	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 1, 1000));

	std::map<std::string, NiShape*> against;
	for (auto* shape : shapes)
		against[shape->name.get()] = McpAgainstShape(mcp, args, shape);

	// Each case is measured in each pose, by default only as the shapes are shown now
	std::vector<std::string> poses = McpArgs::Strings(args, "poses");
	const bool posed = !poses.empty();
	if (!posed)
		poses.push_back("");

	// Sliders on their own: the given ones, or all morph sliders with data on the shapes or the reference
	std::vector<std::string> sliderNames = McpArgs::Strings(args, "sliders");
	if (sliderNames.empty() && !McpArgs::Has(args, "sliders")) {
		for (size_t i = 0; i < project.SliderCount(); i++) {
			if (project.SliderZap(i) || project.SliderUV(i))
				continue;

			SliderData& data = project.activeSet[i];
			bool hasData = false;
			for (auto* shape : shapes) {
				TargetDataDiffs* diff = project.GetDiffSet(data, shape);
				hasData = hasData || (diff && !diff->empty());
			}
			TargetDataDiffs* refDiff = project.GetDiffSet(data, baseShape);
			if (hasData || (refDiff && !refDiff->empty()))
				sliderNames.push_back(project.GetSliderName(i));
		}
	}

	// Sliders that go both ways are also checked at the negative value, the way BodySlide lets users set them
	const bool negative = McpArgs::Bool(args, "negativeValues", true);
	std::vector<SweepCase> cases;
	std::set<std::pair<std::string, float>> singles;
	for (auto& name : sliderNames) {
		if (!project.ValidSlider(name))
			throw McpToolError("There is no slider '" + name + "'.");
		cases.push_back({name, {{name, value}}, name, value});
		singles.insert({name, value});
		if (negative && Bidirectional(name)) {
			cases.push_back({name + " = " + ValueText(-value), {{name, -value}}, name, -value});
			singles.insert({name, -value});
		}
	}

	auto presets = McpPresetCases(mcp, args, "presets", "presetFilter", "presetWeights");
	for (auto& preset : presets)
		cases.push_back({preset.first, preset.second, "", 0.0f});

	if (McpArgs::Bool(args, "presetExtremes", true))
		AddPresetExtremeCases(project, presets, singles, cases);

	if (const json* combinations = McpArgs::Find(args, "combinations")) {
		if (!combinations->is_array())
			McpArgs::Wrong("combinations", "an array of {name, values}");
		for (size_t i = 0; i < combinations->size(); i++) {
			const json& combination = (*combinations)[i];
			SweepCase c;
			c.name = combination.value("name", "combination " + std::to_string(i));
			c.values = McpArgs::FloatMap(combination, "values");
			if (c.values.empty())
				throw McpToolError("Combination '" + c.name + "' has no values.");
			cases.push_back(c);
		}
	}

	McpSliderValueScope sliders(project);
	// The pose is only touched when poses are checked
	std::optional<McpPoseScope> poseScope;
	if (posed)
		poseScope.emplace(mcp);

	std::map<std::string, std::vector<Triangle>> shapeTris;
	std::map<std::string, std::vector<Vector3>> shapeVerts;
	for (auto* shape : shapes) {
		shape->GetTriangles(shapeTris[shape->name.get()]);
		project.GetWorkNif()->GetVertsForShape(shape, shapeVerts[shape->name.get()]);
	}

	struct Row {
		json entry;
		double score = 0.0;
	};
	std::vector<Row> rows;
	json baselines = json::object();

	// Morph quality doesn't depend on the pose: measured once per case and shape, and given to every pose's row
	// so their scores compare
	struct MorphResult {
		json entry; // "morph" or "missingMorph"
		double score = 0.0;
	};
	std::map<std::pair<std::string, std::string>, MorphResult> morphResults;

	for (auto& pose : poses) {
		if (posed)
			poseScope->Set(pose);

		// Clipping with all sliders at zero, which each case is compared with
		sliders.Zero();
		std::map<std::string, MeshAnalysis::PenetrationStats> baseline;
		std::map<std::string, size_t> excluded;
		for (auto* shape : shapes)
			baseline[shape->name.get()] = Clipping(mcp, shape, against[shape->name.get()], nearDistance, &excluded[shape->name.get()]);

		json baselineJson = json::object();
		for (auto& b : baseline) {
			baselineJson[b.first] = PenetrationToJson(b.second);
			if (excluded[b.first])
				baselineJson[b.first]["excluded"] = excluded[b.first];
		}
		baselines[posed ? pose : "current"] = baselineJson;

		for (auto& c : cases) {
			sliders.Zero();
			sliders.Set(c.values);

			for (auto* shape : shapes) {
				const std::string shapeName = shape->name.get();

				MeshAnalysis::PenetrationStats clipping = Clipping(mcp, shape, against[shapeName], nearDistance);
				const MeshAnalysis::PenetrationStats& base = baseline[shapeName];

				// Only what the case adds to the clipping that's already there unmorphed
				size_t newlyInside = 0;
				for (size_t v = 0; v < clipping.signedDistances.size() && v < base.signedDistances.size(); v++)
					if (clipping.signedDistances[v] < 0.0f && !(base.signedDistances[v] < 0.0f))
						newlyInside++;

				Row row;
				row.entry = {{"case", posed ? c.name + " @ " + pose : c.name}, {"shape", shapeName}};
				if (posed)
					row.entry["pose"] = pose;
				json clip = PenetrationToJson(clipping);
				clip["newlyPenetrating"] = newlyInside;
				row.entry["clipping"] = clip;
				row.score = static_cast<double>(newlyInside) + clipping.maxDepth * 10.0;

				// How the gap to the body changes where the outfit is close to it
				double gapScore = 0.0;
				json gap = McpGapStats(base.signedDistances, clipping.signedDistances, gapDistance, gapThreshold, &gapScore);
				if (gapScore > 0.0) {
					row.entry["gap"] = gap;
					row.score += gapScore * 0.5;
				}

				// Morph quality of the slider on its own at the case's value
				if (!c.slider.empty()) {
					auto key = std::make_pair(c.name, shapeName);
					auto cached = morphResults.find(key);
					if (cached == morphResults.end()) {
						MorphResult morphResult;
						std::vector<Vector3> deltas;
						if (ops.GetSliderDeltas(shape, c.slider, deltas).ok) {
							auto morph = MeshAnalysis::AnalyzeMorph(shapeTris[shapeName], deltas);
							if (morph.movedCount == 0) {
								// The reference moves with this slider but the shape has no morph for it
								TargetDataDiffs* refDiff = project.GetDiffSet(project.activeSet[c.slider], baseShape);
								if (refDiff && !refDiff->empty())
									morphResult.entry = {{"missingMorph", true}};
							}
							else {
								json morphJson = MorphToJson(morph);
								std::vector<Vector3> after = shapeVerts[shapeName];
								for (size_t v = 0; v < after.size() && v < deltas.size(); v++)
									after[v] += deltas[v] * c.sliderValue;
								size_t flipped = MeshAnalysis::CountFlippedTriangles(shapeVerts[shapeName], after, shapeTris[shapeName]);
								morphJson["flippedTriangles"] = flipped;
								morphResult.entry = {{"morph", morphJson}};
								morphResult.score = morph.roughness * 20.0 + morph.maxBoundaryRoughness * 5.0 + flipped * 2.0;
							}
						}
						cached = morphResults.emplace(key, std::move(morphResult)).first;
					}

					if (!cached->second.entry.is_null())
						row.entry.update(cached->second.entry);
					row.score += cached->second.score;
				}

				row.entry["score"] = row.score;
				rows.push_back(std::move(row));
			}
		}
	}

	std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.score > b.score; });

	json list = json::array();
	for (size_t i = 0; i < rows.size() && i < limit; i++)
		list.push_back(rows[i].entry);

	McpToolResult result;
	result.data["value"] = value;
	result.data["cases"] = cases.size() * poses.size();
	result.data["baseline"] = posed ? baselines : baselines["current"];
	result.data["worst"] = list;
	if (rows.size() > limit)
		result.data["more"] = rows.size() - limit;
	return result;
}

// The masked, gentle clipping fix loop of the playbook for each case that needs it: mask the clipping area with a
// soft border, fix at a low strength, measure, and raise the strength while it helps. A step that adds too many
// folds or too much roughness is taken back.
McpToolResult AutoFixClipping(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	ProjectOperations ops = mcp.Ops();
	OutfitStudioFrame* frame = mcp.GetFrame();

	NiShape* baseShape = project.GetBaseShape();
	if (!baseShape)
		throw McpToolError("No reference shape is loaded.");

	auto shapes = mcp.Shapes(args, "shapes");
	const size_t minNew = static_cast<size_t>(std::max(0, McpArgs::Int(args, "minNew", 5)));
	const float startStrength = std::clamp(McpArgs::Float(args, "strength", 0.1f), 0.01f, 1.0f);
	const float maxStrength = std::clamp(McpArgs::Float(args, "maxStrength", 0.3f), startStrength, 1.0f);
	const float stepStrength = std::clamp(McpArgs::Float(args, "strengthStep", 0.05f), 0.01f, 1.0f);
	const int grow = std::clamp(McpArgs::Int(args, "grow", 2), 0, 50);
	const int feather = std::clamp(McpArgs::Int(args, "feather", 4), 0, 50);
	const size_t maxFlipIncrease = static_cast<size_t>(std::max(0, McpArgs::Int(args, "maxFlippedIncrease", 5)));
	const float maxFlipRatio = std::max(0.0f, McpArgs::Float(args, "maxFlippedRatio", 0.15f));
	const float maxFlipPerMoved = std::max(0.0f, McpArgs::Float(args, "maxFlippedPerMoved", 0.01f));
	const float maxRoughIncrease = std::max(0.0f, McpArgs::Float(args, "maxRoughnessIncrease", 0.1f));
	const float minRoughAllowance = std::max(0.0f, McpArgs::Float(args, "minRoughnessAllowance", 0.005f));
	const float nearDistance = 0.1f;

	// Cases: single sliders at 1 (and at -1 when they go both ways), then combinations and presets, which are fixed
	// in combination mode, like single sliders at a value other than 1
	std::vector<SweepCase> cases;
	std::set<std::pair<std::string, float>> singles;
	const bool negative = McpArgs::Bool(args, "negativeValues", true);
	auto addSingle = [&](const std::string& name) {
		cases.push_back({name, {{name, 1.0f}}, name, 1.0f});
		singles.insert({name, 1.0f});
		if (negative && Bidirectional(name)) {
			cases.push_back({name + " = -1", {{name, -1.0f}}, name, -1.0f});
			singles.insert({name, -1.0f});
		}
	};
	if (McpArgs::Has(args, "sliders")) {
		for (auto& name : McpArgs::Strings(args, "sliders")) {
			size_t index = 0;
			if (!project.SliderIndexFromName(name, index) || project.SliderZap(index) || project.SliderUV(index))
				throw McpToolError("'" + name + "' isn't a morph slider.");
			addSingle(name);
		}
	}
	else if (!McpArgs::Has(args, "combinations") && !McpArgs::Has(args, "presets") && !McpArgs::Has(args, "presetFilter")) {
		for (size_t i = 0; i < project.SliderCount(); i++) {
			if (project.SliderZap(i) || project.SliderUV(i))
				continue;
			bool hasData = false;
			for (auto* shape : shapes) {
				TargetDataDiffs* diff = project.GetDiffSet(project.activeSet[i], shape);
				hasData = hasData || (diff && !diff->empty());
			}
			if (hasData)
				addSingle(project.GetSliderName(i));
		}
	}
	if (const json* combinations = McpArgs::Find(args, "combinations")) {
		if (!combinations->is_array())
			McpArgs::Wrong("combinations", "an array of {name, values}");
		for (size_t i = 0; i < combinations->size(); i++) {
			SweepCase c;
			c.name = (*combinations)[i].value("name", "combination " + std::to_string(i));
			c.values = McpArgs::FloatMap((*combinations)[i], "values");
			if (c.values.empty())
				throw McpToolError("Combination '" + c.name + "' has no values.");
			cases.push_back(c);
		}
	}
	auto presets = McpPresetCases(mcp, args, "presets", "presetFilter", "presetWeights");
	// The sliders on their own at the extreme values the presets set go first: the presets build on them
	std::vector<SweepCase> extremes;
	if (McpArgs::Bool(args, "presetExtremes", true))
		AddPresetExtremeCases(project, presets, singles, extremes);
	cases.insert(cases.end(), extremes.begin(), extremes.end());
	for (auto& preset : presets)
		cases.push_back({preset.first, preset.second, "", 0.0f});

	// Clipping that's there with all sliders at 0 goes first: it changes the base the other cases are measured from
	if (McpArgs::Bool(args, "rest", false)) {
		SweepCase rest;
		rest.name = "rest (sliders at 0)";
		rest.rest = true;
		rest.sliderValue = 0.0f;
		cases.insert(cases.begin(), rest);
	}

	// The user's mask is put back afterwards
	std::map<std::string, std::unordered_map<uint16_t, float>> savedMasks;
	for (auto* shape : shapes)
		if (Mesh* m = project.GetHost().GetShapeMesh(shape->name.get()))
			savedMasks[shape->name.get()] = MaskOps::GetMasked(*m);

	McpSliderValueScope sliders(project);

	struct Measure {
		size_t newlyPenetrating = 0;
		float maxDepth = 0.0f;
		size_t flipped = 0;
		float roughness = 0.0f;
	};

	// Sliders a case changes: the slider, or the morph sliders of the combination
	auto caseSliders = [&](const SweepCase& c) {
		std::vector<std::string> list;
		if (!c.slider.empty())
			list.push_back(c.slider);
		else
			for (auto& v : c.values) {
				size_t index = 0;
				if (v.second != 0.0f && project.SliderIndexFromName(v.first, index) && !project.SliderZap(index) && !project.SliderUV(index))
					list.push_back(v.first);
			}
		return list;
	};

	json results = json::array();
	size_t fixedCases = 0;
	for (auto* shape : shapes) {
		const std::string shapeName = shape->name.get();
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		std::vector<Vector3> rest;
		project.GetWorkNif()->GetVertsForShape(shape, rest);
		// The base geometry before the rest case changes it, to count the folds that adds
		const std::vector<Vector3> original = rest;

		sliders.Zero();
		MeshAnalysis::PenetrationStats baseline = Clipping(mcp, shape, baseShape, nearDistance);

		auto measure = [&](const SweepCase& c) {
			Measure m;
			sliders.Zero();
			sliders.Set(c.values);
			MeshAnalysis::PenetrationStats stats = Clipping(mcp, shape, baseShape, nearDistance);
			m.maxDepth = stats.maxDepth;
			if (c.rest) {
				// Everything inside counts, and the folds are those the base geometry got since the start
				m.newlyPenetrating = stats.penetrating;
				std::vector<Vector3> now;
				project.GetWorkNif()->GetVertsForShape(shape, now);
				m.flipped = MeshAnalysis::CountFlippedTriangles(original, now, tris);
				return m;
			}
			for (size_t v = 0; v < stats.signedDistances.size() && v < baseline.signedDistances.size(); v++)
				if (stats.signedDistances[v] < 0.0f && !(baseline.signedDistances[v] < 0.0f))
					m.newlyPenetrating++;

			// Folds and roughness of the case's morph: the slider, or all sliders of the combination at their values
			std::vector<Vector3> combined(rest.size());
			for (auto& slider : caseSliders(c)) {
				std::vector<Vector3> deltas;
				ops.GetSliderDeltas(shape, slider, deltas);
				const float value = c.values.at(slider);
				for (size_t v = 0; v < combined.size() && v < deltas.size(); v++)
					combined[v] += deltas[v] * value;
			}
			std::vector<Vector3> after = rest;
			for (size_t v = 0; v < after.size(); v++)
				after[v] += combined[v];
			m.flipped = MeshAnalysis::CountFlippedTriangles(rest, after, tris);
			m.roughness = MeshAnalysis::AnalyzeMorph(tris, combined).roughness;
			return m;
		};
		auto measureJson = [](const Measure& m) {
			return json{{"newlyPenetrating", m.newlyPenetrating}, {"maxDepth", m.maxDepth}, {"flippedTriangles", m.flipped}, {"roughness", m.roughness}};
		};

		for (auto& c : cases) {
			Measure before = measure(c);
			if (before.newlyPenetrating < minNew)
				continue;

			json entry = {{"case", c.name}, {"shape", shapeName}, {"before", measureJson(before)}};
			Measure current = before;
			float used = 0.0f;
			bool reverted = false;
			int steps = 0;

			// What the case starts from, to count the vertices the fix has moved so far
			std::map<std::string, std::vector<Vector3>> startDeltas;
			for (auto& slider : caseSliders(c))
				ops.GetSliderDeltas(shape, slider, startDeltas[slider]);
			std::vector<Vector3> startBase;
			if (c.rest)
				project.GetWorkNif()->GetVertsForShape(shape, startBase);
			auto movedSinceStart = [&]() {
				std::vector<bool> moved(rest.size(), false);
				auto compare = [&](const std::vector<Vector3>& from, const std::vector<Vector3>& to) {
					for (size_t v = 0; v < from.size() && v < to.size() && v < moved.size(); v++)
						if ((to[v] - from[v]).length() > 1e-4f)
							moved[v] = true;
				};
				for (auto& start : startDeltas) {
					std::vector<Vector3> now;
					ops.GetSliderDeltas(shape, start.first, now);
					compare(start.second, now);
				}
				if (c.rest) {
					std::vector<Vector3> now;
					project.GetWorkNif()->GetVertsForShape(shape, now);
					compare(startBase, now);
				}
				return static_cast<size_t>(std::count(moved.begin(), moved.end(), true));
			};

			for (float strength = startStrength; strength <= maxStrength + 1e-4f && current.newlyPenetrating > 0; strength += stepStrength) {
				// Deltas or base geometry before this step, to take it back
				std::map<std::string, std::vector<Vector3>> previous;
				for (auto& slider : caseSliders(c))
					ops.GetSliderDeltas(shape, slider, previous[slider]);
				std::vector<Vector3> previousBase;
				if (c.rest)
					project.GetWorkNif()->GetVertsForShape(shape, previousBase);

				json maskArgs = {{"op", "from_analysis"}, {"shapes", json::array({shapeName})}, {"metric", "clipping"}, {"sliderValues", c.values}, {"grow", grow}, {"feather", feather}};
				sliders.Zero();
				McpMaskFromMetric(mcp, maskArgs);
				{
					McpExclusionMaskScope protect(mcp, {shape});
					if (c.rest)
						mcp.Apply(ops.FixClipping({shape}, ClippingFixMode::Shapes, strength));
					else if (!c.slider.empty() && c.sliderValue == 1.0f)
						mcp.Apply(ops.FixClipping({shape}, ClippingFixMode::Sliders, strength, {c.slider}));
					else
						mcp.Apply(ops.FixClippingCombination({shape}, c.values, {}, strength));
				}
				steps++;

				Measure after = measure(c);
				// A big combination starts with hundreds of slivers, so the allowance grows with them, and a fix that
				// moves a large area may turn a few slivers in it
				const size_t allowedFolds = std::max({maxFlipIncrease, static_cast<size_t>(before.flipped * maxFlipRatio), static_cast<size_t>(movedSinceStart() * maxFlipPerMoved)});
				const bool foldsGrew = after.flipped > before.flipped + allowedFolds;
				// The rest case has no morph to be rough, its folds tell
				// Relative to the start, but a weak morph may still get a little rougher in absolute terms
				const bool rougher = !c.rest && after.roughness > std::max(before.roughness * (1.0f + maxRoughIncrease), before.roughness + minRoughAllowance) + 1e-4f;
				// Fewer clipping vertices, or as many but less deep. More of them is worse even when the deepest got
				// shallower.
				const bool noBetter = after.newlyPenetrating > current.newlyPenetrating ||
									  (after.newlyPenetrating == current.newlyPenetrating && after.maxDepth >= current.maxDepth);
				// Fewer clipping vertices don't help when the fix pushes others deeper in
				const bool deeper = after.maxDepth > current.maxDepth + 0.05f;
				if (foldsGrew || rougher || noBetter || deeper) {
					for (auto& p : previous) {
						std::unordered_map<uint16_t, Vector3> back;
						for (size_t v = 0; v < p.second.size(); v++)
							back[static_cast<uint16_t>(v)] = p.second[v];
						mcp.Apply(ops.SetSliderDeltas(shape, p.first, back, false));
					}
					if (c.rest) {
						project.GetWorkNif()->SetVertsForShape(shape, previousBase);
						OpResult back;
						back.refresh = RefreshFlags::Meshes | RefreshFlags::PendingChanges;
						mcp.Apply(back);
					}
					reverted = true;
					entry["stopped"] = foldsGrew ? "folds" : rougher ? "roughness" : deeper ? "deeper clipping" : "no improvement";
					break;
				}

				current = after;
				used = strength;
			}

			entry["after"] = measureJson(current);
			entry["strength"] = used;
			entry["steps"] = steps;
			if (reverted)
				entry["lastStepReverted"] = true;
			if (current.newlyPenetrating < before.newlyPenetrating)
				fixedCases++;
			results.push_back(entry);

			// The other cases are measured from the fixed base
			if (c.rest) {
				project.GetWorkNif()->GetVertsForShape(shape, rest);
				sliders.Zero();
				baseline = Clipping(mcp, shape, baseShape, nearDistance);
			}
		}
	}

	// Put the user's masks back
	for (auto* shape : shapes) {
		Mesh* m = project.GetHost().GetShapeMesh(shape->name.get());
		if (!m)
			continue;
		auto& saved = savedMasks[shape->name.get()];
		frame->glView->ApplyMaskChanges({m}, [&](Mesh& mesh) {
			std::unordered_map<int, float> changes = MaskOps::ClearChanges(mesh);
			for (auto& v : saved)
				changes[v.first] = v.second;
			return changes;
		});
	}
	frame->glView->Render();

	McpToolResult result;
	result.data["checked"] = cases.size() * shapes.size();
	result.data["fixed"] = fixedCases;
	result.data["cases"] = results;
	return result;
}

McpToolResult CompareToProject(OutfitStudioMcp& mcp, const json& args) {
	std::string file = AutomationRunner::MakeAbsoluteToProject(wxString::FromUTF8(McpArgs::String(args, "file"))).ToUTF8().data();
	if (!wxFileName::FileExists(wxString::FromUTF8(file)))
		throw McpToolError("There is no file '" + file + "'.");

	std::map<std::string, std::string> shapeMap;
	if (const json* map = McpArgs::Find(args, "shapeMap")) {
		if (!map->is_object())
			McpArgs::Wrong("shapeMap", "an object of shape names here to shape names there");
		for (auto& item : map->items())
			shapeMap[item.key()] = item.value().get<std::string>();
	}

	ProjectComparison comparison;
	mcp.Apply(mcp.Ops().CompareToProject(file, McpArgs::String(args, "set"), shapeMap, comparison));

	const size_t limit = static_cast<size_t>(std::clamp(McpArgs::Int(args, "limit", 25), 1, 1000));

	auto& sliders = comparison.sliders;
	std::stable_sort(sliders.begin(), sliders.end(), [](const auto& a, const auto& b) { return a.deltas.rmsError > b.deltas.rmsError; });

	json sliderList = json::array();
	size_t identical = 0;
	for (auto& s : sliders) {
		if (s.deltas.maxError <= 1e-5f) {
			identical++;
			continue;
		}
		if (sliderList.size() >= limit)
			continue;

		json entry = {{"shape", s.shape}, {"slider", s.slider}, {"meanError", s.deltas.meanError}, {"maxError", s.deltas.maxError}, {"rmsError", s.deltas.rmsError}};
		if (s.onlyHere)
			entry["onlyHere"] = true;
		if (s.onlyThere)
			entry["onlyThere"] = true;
		sliderList.push_back(entry);
	}

	auto& weights = comparison.weights;
	std::stable_sort(weights.begin(), weights.end(), [](const auto& a, const auto& b) { return a.meanError > b.meanError; });

	json weightList = json::array();
	for (size_t i = 0; i < weights.size() && i < limit; i++) {
		auto& w = weights[i];
		weightList.push_back({{"shape", w.shape}, {"bone", w.bone}, {"vertices", w.vertices}, {"meanError", w.meanError}, {"maxError", w.maxError}});
	}

	json skipped = json::array();
	for (auto& s : comparison.skippedShapes)
		skipped.push_back({{"shape", s.first}, {"reason", s.second}});

	McpToolResult result;
	result.data["slidersCompared"] = sliders.size();
	result.data["slidersIdentical"] = identical;
	result.data["sliders"] = sliderList;
	result.data["bonesCompared"] = weights.size();
	result.data["weights"] = weightList;
	result.data["skippedShapes"] = skipped;
	return result;
}

McpToolResult ShowHeatmap(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	std::string kind = McpArgs::String(args, "kind");

	if (kind == "off") {
		frame->FillVertexColors();
		ViewState state = frame->GetViewState();
		state.display.vertexColors = McpArgs::Bool(args, "vertexColors", false);
		frame->SetViewState(state);
		return McpToolResult();
	}

	NiShape* shape = mcp.Shape(McpArgs::String(args, "shape"));
	std::vector<float> values;
	float minValue = 0.0f;
	float maxValue = 1.0f;
	McpToolResult result;

	if (kind == "clipping") {
		McpSliderValueScope sliders(project);
		if (McpArgs::Bool(args, "zeroOthers", true) && McpHasSliderValuesArg(args))
			sliders.Zero();
		sliders.Set(McpSliderValuesArg(mcp, args));

		MeshAnalysis::PenetrationStats stats = Clipping(mcp, shape, McpAgainstShape(mcp, args, shape), McpArgs::Float(args, "nearDistance", 0.1f));
		// Depth inside the reference is positive, so it shows red
		values = stats.signedDistances;
		for (auto& v : values)
			if (v != FLT_MAX)
				v = -v;
		const float range = McpArgs::Float(args, "range", 0.5f);
		minValue = -range;
		maxValue = range;
		result.data["clipping"] = PenetrationToJson(stats);
	}
	else if (kind == "slider") {
		std::vector<Vector3> deltas;
		mcp.Apply(mcp.Ops().GetSliderDeltas(shape, McpArgs::String(args, "slider"), deltas));
		for (auto& d : deltas) {
			values.push_back(d.length());
			maxValue = std::max(maxValue, values.back());
		}
		minValue = 0.0f;
		if (McpArgs::Has(args, "range"))
			maxValue = McpArgs::Float(args, "range", maxValue);
	}
	else if (kind == "weights") {
		std::string bone = McpArgs::String(args, "bone");
		values.assign(shape->GetNumVertices(), 0.0f);
		if (auto weights = project.GetWorkAnim()->GetWeightsPtr(shape->name.get(), bone))
			for (auto& w : *weights)
				if (w.first < values.size())
					values[w.first] = w.second;
	}
	else if (std::find(McpMetricNames().begin(), McpMetricNames().end(), kind) != McpMetricNames().end()) {
		json metricArgs = args;
		metricArgs["metric"] = kind;
		float threshold = 0.0f;
		values = McpVertexMetric(mcp, shape, metricArgs, &threshold);

		if (kind == "gap_change") {
			// Tighter shows blue, looser red
			const float range = McpArgs::Float(args, "range", 1.0f);
			minValue = -range;
			maxValue = range;
		}
		else {
			minValue = 0.0f;
			maxValue = 0.0f;
			for (float v : values)
				maxValue = std::max(maxValue, v);
			if (kind == "flipped" || kind == "weight_deviation" || kind == "weight_jump")
				maxValue = 1.0f;
			maxValue = std::max(McpArgs::Float(args, "range", maxValue), 1e-6f);
		}
		result.data["threshold"] = threshold;
	}
	else
		McpArgs::Wrong("kind", "clipping, slider, weights, off or one of the metrics");

	mcp.Apply(mcp.Ops().ShowHeatmap(shape, values, minValue, maxValue));

	ViewState state = frame->GetViewState();
	state.display.vertexColors = true;
	// The mask display hides vertex colors
	if (state.display.mask) {
		state.display.mask = false;
		result.data["maskDisplayTurnedOff"] = true;
	}
	frame->SetViewState(state);

	result.data["min"] = minValue;
	result.data["max"] = maxValue;
	return result;
}

const json ShapesProp = McpStringList("Outfit shapes, the selected ones by default");

} // namespace

void AddAnalysisTools(OutfitStudioMcp& mcp) {
	McpTool slider;
	slider.name = "analyze_slider";
	slider.description =
		"Morph quality of one slider on shapes, measured with only that slider: moved vertices, mean and largest delta, "
		"roughness (how much a vertex moves differently from its neighbors), normalized roughness (the same relative to "
		"the mean delta), the largest step at the edge of the morph and the triangles that flip over (folds), whose "
		"vertices withFlipped returns. Look at both roughness values: a weaker morph reads rougher when normalized.";
	slider.inputSchema = McpSchema(
		{
			{"shapes", ShapesProp},
			{"slider", McpProp("string", "Slider name")},
			{"value", McpProp("number", "Slider value the flipped triangles are counted at. Default 1.")},
			{"withFlipped", McpProp("boolean", "Also return the vertices of the flipped triangles, the folds of the morph, e.g. for frame_view, set_mask or brush_stroke. Default false.")},
		},
		{"slider"});
	slider.readOnly = true;
	slider.handler = [&mcp](const json& args) {
		std::string sliderName = McpArgs::String(args, "slider");
		const float value = McpArgs::Float(args, "value", 1.0f);
		const bool withFlipped = McpArgs::Bool(args, "withFlipped", false);

		McpToolResult result;
		for (auto* shape : mcp.Shapes(args, "shapes")) {
			MeshAnalysis::MorphStats stats;
			mcp.Apply(mcp.Ops().AnalyzeSlider(shape, sliderName, stats));
			json entry = MorphToJson(stats);

			// Folds: triangles that turn over when the morph is applied at the value
			std::vector<Vector3> before, deltas;
			mcp.Project().GetWorkNif()->GetVertsForShape(shape, before);
			mcp.Apply(mcp.Ops().GetSliderDeltas(shape, sliderName, deltas));
			std::vector<Vector3> after = before;
			for (size_t v = 0; v < after.size() && v < deltas.size(); v++)
				after[v] += deltas[v] * value;

			std::vector<Triangle> tris;
			shape->GetTriangles(tris);
			std::vector<size_t> flipped = MeshAnalysis::FlippedTriangles(before, after, tris);
			entry["flippedTriangles"] = flipped.size();

			if (withFlipped) {
				std::set<uint16_t> vertices;
				for (size_t t : flipped) {
					vertices.insert(tris[t].p1);
					vertices.insert(tris[t].p2);
					vertices.insert(tris[t].p3);
				}
				entry["flippedVertices"] = vertices;
			}

			result.data["shapes"][shape->name.get()] = entry;
		}
		return result;
	};
	mcp.AddTool(std::move(slider));

	McpTool clipping;
	clipping.name = "analyze_clipping";
	clipping.description =
		"How far shapes go into the reference shape, or into another shape with 'against' (outfit layers): penetrating vertices, vertices close to the surface, deepest and mean "
		"depth in NIF units, and the deepest vertices. Where the body pokes out through an outfit triangle between its "
		"vertices (large triangles over a curve), the triangle's vertices count as penetrating by how far the body pokes "
		"out, and pokeThrough says how many body vertices show through. Measured with the current slider values and preview pose, or with "
		"sliderValues (the other sliders at 0 unless zeroOthers is false). Use the vertices with frame_view, set_mask or "
		"brush_stroke.";
	clipping.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"sliderValues", McpNumberMap("Slider values to measure with, e.g. {\"Breasts\": 1}")},
		{"zeroOthers", McpProp("boolean", "With sliderValues: set the other sliders to 0. Default true.")},
		{"nearDistance", McpProp("number", "Distance outside the surface that still counts as close. Default 0.1.")},
		{"worstVertices", McpProp("integer", "How many of the deepest vertices to return. Default 10.")},
		{"against", McpProp("string", "Measure against this shape instead of the reference, e.g. a layer of the outfit underneath")},
	});
	const json presetProperties = McpPresetValueProperties();
	for (auto& property : presetProperties.items())
		clipping.inputSchema["properties"][property.key()] = property.value();
	clipping.readOnly = true;
	clipping.handler = [&mcp](const json& args) { return AnalyzeClipping(mcp, args); };
	mcp.AddTool(std::move(clipping));

	McpTool weights;
	weights.name = "analyze_weights";
	weights.description =
		"Weight problems of shapes: unweighted vertices, vertices with more bones than the game allows, and weights that don't "
		"add up to 1. With compareToReference, also how far the weights differ from the body under each vertex (0 the same, 1 "
		"no bone in common): outfit vertices weighted unlike the skin they sit on clip or stretch when the body moves, even "
		"when they fit at rest. mask_ops from_analysis with metric weight_deviation masks them for copy_bone_weights.";
	weights.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"influenceLimit", McpProp("integer", "Most bones per vertex. Default 4.")},
		{"compareToReference", McpProp("boolean", "Also compare the weights with the reference's (or 'against'). Default false.")},
		{"against", McpProp("string", "Shape to compare the weights with. Default the reference.")},
		{"maxDistance", McpProp("number", "Only vertices this close to the body are compared. Default 2.")},
		{"threshold", McpProp("number", "Deviation that counts as different. Default 0.25.")},
		{"worstVertices", McpProp("integer", "How many of the most different vertices to return. Default 10.")},
	});
	weights.readOnly = true;
	weights.handler = [&mcp](const json& args) {
		const int limit = McpArgs::Int(args, "influenceLimit", 4);
		McpToolResult result;
		for (auto* shape : mcp.Shapes(args, "shapes")) {
			MeshAnalysis::WeightStats stats;
			mcp.Apply(mcp.Ops().AnalyzeWeights(shape, stats, limit));
			json entry = {
				{"unweighted", stats.unweighted},
				{"overInfluenceLimit", stats.overInfluenceLimit},
				{"badSum", stats.badSum},
				{"maxSumError", stats.maxSumError},
				{"maxInfluences", stats.maxInfluences},
			};

			if (McpArgs::Bool(args, "compareToReference", false)) {
				json metricArgs = args;
				metricArgs["metric"] = "weight_deviation";
				float threshold = 0.25f;
				std::vector<float> deviation = McpVertexMetric(mcp, shape, metricArgs, &threshold);
				threshold = McpArgs::Float(args, "threshold", threshold);

				size_t over = 0;
				double sum = 0.0;
				std::vector<std::pair<float, int>> sorted;
				for (size_t v = 0; v < deviation.size(); v++) {
					sum += deviation[v];
					if (deviation[v] > threshold)
						over++;
					if (deviation[v] > 0.0f)
						sorted.emplace_back(deviation[v], static_cast<int>(v));
				}
				std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

				const size_t worst = static_cast<size_t>(std::clamp(McpArgs::Int(args, "worstVertices", 10), 0, 1000));
				json list = json::array();
				for (size_t i = 0; i < sorted.size() && i < worst; i++)
					list.push_back({{"vertex", sorted[i].second}, {"deviation", sorted[i].first}});

				entry["deviation"] = {
					{"mean", deviation.empty() ? 0.0 : sum / deviation.size()},
					{"max", sorted.empty() ? 0.0f : sorted.front().first},
					{"overThreshold", over},
					{"worst", list},
				};
			}
			result.data["shapes"][shape->name.get()] = entry;
		}
		return result;
	};
	mcp.AddTool(std::move(weights));

	McpTool sweep;
	sweep.name = "quality_sweep";
	sweep.description =
		"Checks a conversion slider by slider, and for given slider combinations: each case is measured with the other "
		"sliders at 0. For every outfit shape it reports clipping into the reference (with how many vertices the case newly "
		"pushes inside compared with all sliders at 0) and, for single sliders, the morph quality (roughness, edge steps, "
		"flipped triangles) or that the shape has no morph where the reference moves, and how the gap to the body changes "
		"where the outfit is close to it (tightened or loosened vertices). With poses, every case is checked in each pose. "
		"Returns the worst cases first. Sliders that go both ways are also checked at minus the value, and with presets each "
		"slider at the extreme values the presets set outside 0 to 1. Vertices of analysis_exclusions aren't counted. "
		"Body presets of list_presets can be checked too, at their low and high weight. Slider values are put back "
		"afterwards. Takes a few seconds per hundred sliders. Extreme presets may never look perfect on an outfit, "
		"depending on its shape, topology and mesh resolution: judge them by what they add over the other presets.";
	sweep.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"sliders", McpStringList("Sliders to check on their own. Default all morph sliders with data on the shapes or the reference; [] for none.")},
		{"combinations",
		 {{"type", "array"},
		  {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}}}, {"values", {{"type", "object"}, {"additionalProperties", {{"type", "number"}}}}}}}}},
		  {"description", "Slider combinations to check, e.g. [{\"name\": \"big\", \"values\": {\"Breasts\": 1, \"Butt\": 1}}]"}}},
		{"presets", McpStringList("Body presets to check, by name")},
		{"presetFilter", McpProp("string", "Also check every preset whose name contains this text, ignoring case")},
		{"presetWeights", {{"type", "array"}, {"items", {{"type", "string"}, {"enum", {"low", "high"}}}}, {"description", "Weights to check the presets at. Default both."}}},
		{"value", McpProp("number", "Value for the single sliders. Default 1.")},
		{"negativeValues", McpProp("boolean", "Also check sliders that go both ways (\"n|p\", \"p|n\" in the name) at minus the value. Default true.")},
		{"presetExtremes", McpProp("boolean", "With presets: also check each slider on its own at the most extreme values outside 0 to 1 that the presets set, e.g. BreastsZero at -1.5. Default true.")},
		{"poses", McpStringList("Poses of list_poses to check every case in, e.g. sitting or arms raised, or \"none\" for unposed. Default only as shown now.")},
		{"against", McpProp("string", "Measure against this shape instead of the reference, e.g. a layer of the outfit underneath")},
		{"nearDistance", McpProp("number", "Default 0.1.")},
		{"gapDistance", McpProp("number", "Only vertices this close to the body at 0 count for the gap. Default 2.")},
		{"gapThreshold", McpProp("number", "Change of the gap that counts as tightened or loosened. Default 0.3.")},
		{"limit", McpProp("integer", "How many of the worst cases to return. Default 25.")},
	});
	sweep.outputSchema = {
		{"type", "object"},
		{"properties",
		 {{"value", {{"type", "number"}}},
		  {"cases", {{"type", "integer"}}},
		  {"baseline", {{"type", "object"}}},
		  {"worst", {{"type", "array"}, {"items", {{"type", "object"}}}}},
		  {"more", {{"type", "integer"}}}}},
	};
	sweep.readOnly = true;
	sweep.handler = [&mcp](const json& args) { return QualitySweep(mcp, args); };
	mcp.AddTool(std::move(sweep));

	McpTool autoFix;
	autoFix.name = "auto_fix_clipping";
	autoFix.description =
		"Runs the masked, gentle clipping fix loop for every case that needs it: single sliders (the given ones, or all "
		"morph sliders with data, at 1 and sliders that go both ways also at -1), combinations and presets (with each "
		"slider on its own at the extreme values the presets set first). For each case with at least minNew newly clipping vertices it "
		"masks the clipping area with a soft border (vertices of analysis_exclusions stay masked), fixes at a low strength "
		"(sliders mode, or combination mode for combinations and presets), measures, and raises the strength step by step "
		"while it helps. A step that takes the folds more than maxFlippedIncrease (or maxFlippedRatio) over the start, raises the roughness by more than "
		"maxRoughnessIncrease (or minRoughnessAllowance), pushes the deepest vertex more than 0.05 deeper or doesn't improve anything is taken back, "
		"and the case stops there. Reports each case before "
		"and after. Clipping counts the body poking out between outfit vertices too (see analyze_clipping), and the fix "
		"pushes those triangles out. With rest, the clipping at sliders 0 is fixed in the base geometry first. The mask is "
		"put back afterwards. Check the result with quality_sweep and render_view.";
	autoFix.inputSchema = McpSchema({
		{"shapes", ShapesProp},
		{"sliders", McpStringList("Sliders to fix on their own. Default all morph sliders with data on the shapes, unless combinations or presets are given.")},
		{"combinations",
		 {{"type", "array"},
		  {"items", {{"type", "object"}, {"properties", {{"name", {{"type", "string"}}}, {"values", {{"type", "object"}, {"additionalProperties", {{"type", "number"}}}}}}}}},
		  {"description", "Slider combinations to fix, like quality_sweep takes them"}}},
		{"presets", McpStringList("Body presets to fix, by name")},
		{"presetFilter", McpProp("string", "Every preset whose name contains this text")},
		{"presetWeights", {{"type", "array"}, {"items", {{"type", "string"}, {"enum", {"low", "high"}}}}, {"description", "Default both."}}},
		{"rest", McpProp("boolean", "Also fix the clipping there is with all sliders at 0, in the base geometry, before the other cases. Compare with the source project first: some outfits clip at rest by design (tucked hems). Default false.")},
		{"negativeValues", McpProp("boolean", "With single sliders: also fix sliders that go both ways (\"n|p\", \"p|n\" in the name) at -1. Default true.")},
		{"presetExtremes", McpProp("boolean", "With presets: first fix each slider on its own at the most extreme values outside 0 to 1 that the presets set, e.g. BreastsZero at -1.5. Default true.")},
		{"minNew", McpProp("integer", "Fix only cases with at least this many newly clipping vertices. Default 5.")},
		{"strength", McpProp("number", "First strength. Default 0.1.")},
		{"maxStrength", McpProp("number", "Highest strength to try. Default 0.3.")},
		{"strengthStep", McpProp("number", "How much the strength rises per step. Default 0.05.")},
		{"grow", McpProp("integer", "Rings around the clipping vertices that take part. Default 2.")},
		{"feather", McpProp("integer", "Smoothing passes on the mask border. Default 4.")},
		{"maxFlippedIncrease", McpProp("integer", "Folds the fix may add over the start, at least. Default 5.")},
		{"maxFlippedRatio", McpProp("number", "Folds the fix may add relative to the start, for cases that start with many (presets). The largest allowance counts. Default 0.15.")},
		{"maxFlippedPerMoved", McpProp("number", "Folds the fix may add per vertex it has moved, for fixes over a large area. The largest allowance counts. Default 0.01.")},
		{"maxRoughnessIncrease", McpProp("number", "Relative roughness rise allowed over the start, e.g. 0.1 for 10%. Default 0.1.")},
		{"minRoughnessAllowance", McpProp("number", "Absolute roughness rise always allowed, for weak morphs where 10% is nothing. The larger of the two counts. Default 0.005.")},
	});
	autoFix.handler = [&mcp](const json& args) { return AutoFixClipping(mcp, args); };
	mcp.AddTool(std::move(autoFix), std::chrono::minutes(20));

	McpTool compare;
	compare.name = "compare_to_project";
	compare.description =
		"Compares the slider morphs and bone weights of the outfit shapes with a set of another slider set file, e.g. a "
		"finished conversion of the same outfit, to score a conversion. Shapes are matched by name or shapeMap and need the "
		"same vertex count. Returns the largest differences first.";
	compare.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "Slider set file (absolute or relative to the BodySlide folder)")},
			{"set", McpProp("string", "Set in the file")},
			{"shapeMap", {{"type", "object"}, {"additionalProperties", {{"type", "string"}}}, {"description", "Shape names here to names there, where they differ"}}},
			{"limit", McpProp("integer", "How many of the largest slider and weight differences to return. Default 25.")},
		},
		{"file", "set"});
	compare.readOnly = true;
	compare.handler = [&mcp](const json& args) { return CompareToProject(mcp, args); };
	mcp.AddTool(std::move(compare), std::chrono::minutes(10));

	McpTool heatmap;
	heatmap.name = "show_heatmap";
	heatmap.description =
		"Colors a shape in the view by a value per vertex, blue to red, and turns on vertex colors: clipping (signed "
		"distance to the reference or 'against', red inside, blue outside), slider (how far each vertex moves), weights (of a "
		"bone), or a quality metric: roughness, flipped, weight_deviation, gap_change (blue tighter, red looser), seam_gap, "
		"pose_stretch (how much the mesh stretches in a pose) or weight_jump (abrupt weight changes). \"off\" shows the "
		"shapes' own vertex colors again. render_view shows the result.";
	heatmap.inputSchema = McpSchema(
		{
			{"kind",
			 {{"type", "string"},
			  {"enum", {"clipping", "slider", "weights", "off", "roughness", "flipped", "weight_deviation", "gap_change", "seam_gap", "pose_stretch", "weight_jump"}},
			  {"description", "What to color by. roughness, flipped, weight_deviation, gap_change, seam_gap, pose_stretch and weight_jump are the metrics of mask_ops from_analysis."}}},
			{"shape", McpProp("string", "Shape to color")},
			{"slider", McpProp("string", "kind slider and the slider metrics: the slider")},
			{"pose", McpProp("string", "pose_stretch: the pose of list_poses")},
			{"value", McpProp("number", "Slider metrics: slider value to measure at. Default 1.")},
			{"against", McpProp("string", "clipping, gap_change, weight_deviation: the shape to measure against. Default the reference.")},
			{"maxDistance", McpProp("number", "gap_change, weight_deviation: how far from the body vertices count. Default 2.")},
			{"bone", McpProp("string", "kind weights: the bone")},
			{"sliderValues", McpNumberMap("kind clipping: slider values to measure with")},
			{"zeroOthers", McpProp("boolean", "kind clipping with sliderValues: set the other sliders to 0 while measuring. Default true.")},
			{"range", McpProp("number", "clipping: distance for full color (default 0.5); gap_change: change for full color (default 1); others: value for full red (default the largest)")},
			{"vertexColors", McpProp("boolean", "kind off: leave vertex colors on. Default false.")},
		},
		{"kind"});
	for (auto& property : presetProperties.items())
		heatmap.inputSchema["properties"][property.key()] = property.value();
	heatmap.handler = [&mcp](const json& args) { return ShowHeatmap(mcp, args); };
	mcp.AddTool(std::move(heatmap));
}
