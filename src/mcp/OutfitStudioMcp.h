/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "McpArgs.h"
#include "McpProtocol.h"
#include "McpServer.h"
#include "../components/PoseMath.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace MeshAnalysis {
struct PenetrationStats;
}

class OutfitStudioFrame;
class OutfitProject;
class ProjectOperations;
class McpSnapshotStore;
struct OpResult;

namespace nifly {
class NiShape;
struct Vector3;
struct Triangle;
}

constexpr int McpDefaultPort = 47823;

// The MCP server of an Outfit Studio window. Tools run on the main thread against the
// window's project, the same way the window's own commands do.
class OutfitStudioMcp {
public:
	using Handler = std::function<McpToolResult(const nlohmann::json& args)>;
	using ProgressHandler = std::function<McpToolResult(const nlohmann::json& args, const McpProgress& progress)>;

	explicit OutfitStudioMcp(OutfitStudioFrame* inFrame);
	~OutfitStudioMcp();

	// Main thread only
	bool Start(int port, const std::string& token, std::string& outError);
	void Stop();

	bool IsRunning() const { return server.IsRunning(); }
	std::string GetUrl() const { return server.GetUrl(); }

	OutfitStudioFrame* GetFrame() const { return frame; }

	// Adds a tool whose handler runs on the main thread, once the window isn't busy. Messages the
	// project shows and warnings or errors it logs while the tool runs are returned with the result,
	// and the window's progress bar is reported as progress. Tools that change the project in ways the
	// window can't undo get an automatic snapshot before they run.
	void AddTool(McpTool tool, std::chrono::milliseconds timeout = std::chrono::minutes(10));

	McpSnapshotStore& Snapshots() { return *snapshots; }

	// Vertices left out of clipping measurements and protected from clipping fixes, by shape name: parts that are
	// inside the body by design, like end caps under a sleeve or cuffs past the body's open wrist. They are dropped
	// when another project is opened. Excluded returns null for a shape without any, or whose vertex count changed.
	std::map<std::string, std::vector<bool>>& Exclusions();
	const std::vector<bool>* Excluded(nifly::NiShape* shape);

	// Zap sliders that hide parts of the surface clipping is measured against, the way BodySlide builds the outfit:
	// a body zapped under a bodysuit can't show through it. Unset means the zap sliders BodySlide turns on by
	// default. Dropped with the exclusions.
	std::optional<std::vector<std::string>>& AnalysisZaps();
	// The zap sliders AnalysisZaps stands for now
	std::vector<std::string> ActiveAnalysisZaps();
	// Which of the triangles of a shape the analysis zaps remove
	std::vector<bool> HiddenSurfaceTriangles(nifly::NiShape* shape, const std::vector<nifly::Triangle>& tris);
	// The triangles of a shape the analysis zaps don't remove. outHidden receives how many were left out.
	std::vector<nifly::Triangle> SurfaceTriangles(nifly::NiShape* shape, size_t* outHidden = nullptr);

	// Helpers for tool handlers, main thread only. They throw an McpToolError when something's missing.
	OutfitProject& Project();
	ProjectOperations Ops();
	nifly::NiShape* Shape(const std::string& name);
	// The shapes named in args[name], or the selected outfit shapes if there are none
	std::vector<nifly::NiShape*> Shapes(const nlohmann::json& args, const char* name, bool allowReference = false);
	// Applies the refresh of a successful operation to the window, or throws its error
	void Apply(const OpResult& result);
	// Throws unless vertices can be moved, with the window's rules: the base shape only with all sliders
	// at zero, since the view shows the slider result, and a slider in edit mode only while it isn't zero
	void CheckPositionEdit();

private:
	OutfitStudioFrame* frame = nullptr;
	McpToolRegistry registry;
	McpProtocol protocol;
	McpServer server;
	std::unique_ptr<McpSnapshotStore> snapshots;
	std::map<std::string, std::vector<bool>> exclusions;
	std::optional<std::vector<std::string>> analysisZaps;
	uint64_t exclusionsProject = 0;
	// Shared with calls still queued for the main thread, which may run after a timeout
	std::shared_ptr<std::atomic<bool>> stopping = std::make_shared<std::atomic<bool>>(false);

	ProgressHandler OnMainThread(Handler handler, const std::string& toolName, std::chrono::milliseconds timeout, bool autoSnapshot);
};

// Tool groups, each in its own file
void AddProjectTools(OutfitStudioMcp& mcp);
void AddAutomationTools(OutfitStudioMcp& mcp);
void AddViewTools(OutfitStudioMcp& mcp);
void AddSliderTools(OutfitStudioMcp& mcp);
void AddConversionTools(OutfitStudioMcp& mcp);
void AddHistoryTools(OutfitStudioMcp& mcp);
void AddEditTools(OutfitStudioMcp& mcp);
void AddAnalysisTools(OutfitStudioMcp& mcp);
void AddArchiveTools(OutfitStudioMcp& mcp);
void AddShapeTools(OutfitStudioMcp& mcp);
void AddBoneTools(OutfitStudioMcp& mcp);
void AddSliderFileTools(OutfitStudioMcp& mcp);
void AddQualityTools(OutfitStudioMcp& mcp);
void AddFittingTools(OutfitStudioMcp& mcp);
void AddTextureTools(OutfitStudioMcp& mcp);
void AddGameTools(OutfitStudioMcp& mcp);
void AddGuideTools(OutfitStudioMcp& mcp);

struct AutomationStep;
struct ObjImportOptions;
class PresetCollection;
class PoseData;

// The body presets installed in the SliderPresets folder
PresetCollection McpLoadPresets();

// Presets named in args[name], or whose names contain args[filterName], as {preset name, values} for the
// weights in args[weightName] ("low", "high"). Throws for unknown presets.
std::vector<std::pair<std::string, std::map<std::string, float>>> McpPresetCases(OutfitStudioMcp& mcp,
																			 const nlohmann::json& args,
																			 const char* name,
																			 const char* filterName,
																			 const char* weightName);

// args["sliderValues"], or the values of the body preset in args["preset"] at args["presetWeight"] (default high)
// with sliderValues on top. Empty when neither is given.
std::map<std::string, float> McpSliderValuesArg(OutfitStudioMcp& mcp, const nlohmann::json& args);

// Whether args give slider values, by sliderValues or preset
bool McpHasSliderValuesArg(const nlohmann::json& args);

// Schemas of the preset arguments of McpSliderValuesArg, to merge into a tool's input schema
nlohmann::json McpPresetValueProperties();

// Runs automation steps the way a script does, with {variables} filled in. Returns one entry per
// step, failed is set when one failed. Main thread only. OBJ imports use objOptions, else the defaults.
nlohmann::json McpRunSteps(OutfitStudioMcp& mcp,
						   const std::vector<AutomationStep>& steps,
						   const std::map<std::string, std::string>& variables,
						   bool stopOnError,
						   bool& failed,
						   const ObjImportOptions* objOptions = nullptr);

// Runs one automation step built from JSON the way run_steps takes it, and throws when it fails
void McpRunStep(OutfitStudioMcp& mcp, const nlohmann::json& step, const ObjImportOptions* objOptions = nullptr);

// Absolute path of a file given absolute or relative to the BodySlide folder
std::string McpFilePath(const std::string& path);

// Throws when the file exists and args doesn't have overwrite: true
void McpCheckOverwrite(const std::string& path, const nlohmann::json& args);

// Sets slider values on the project for a measurement and puts the old ones back after, without
// refreshing the window, since only the project's live vertices are measured
class McpSliderValueScope {
public:
	explicit McpSliderValueScope(OutfitProject& inProject);
	~McpSliderValueScope();

	void Zero();
	// Throws for unknown sliders
	void Set(const std::map<std::string, float>& values);

private:
	OutfitProject& project;
	std::vector<float> saved;
};

// Poses the skeleton for a measurement and puts the previous pose back after, without refreshing the window
class McpPoseScope {
public:
	explicit McpPoseScope(OutfitStudioMcp& inMcp);
	~McpPoseScope();

	// A pose of list_poses, or "none" for the unposed shapes. Throws for unknown poses.
	void Set(const std::string& pose);
	// Back to the pose from before the scope
	void Restore();

private:
	OutfitStudioMcp& mcp;
	bool wasPosed = false;
	std::unique_ptr<PoseData> saved;
};

// The shape named in args["against"], or the reference shape. Throws when there is none, or it's the shape itself.
nifly::NiShape* McpAgainstShape(OutfitStudioMcp& mcp, const nlohmann::json& args, nifly::NiShape* shape);

// How many mesh units one unit of the skeleton is: Starfield's skeleton is in meters, its meshes about 70 times
// larger. Positions of bones are multiplied by it to compare them with vertices, and distances given in mesh units
// divided by it to move bones.
float McpBoneUnits(OutfitStudioMcp& mcp);

// The vertices of a shape as shown, with the current slider values and pose, in the skeleton's global space. The
// project's live vertices are in each shape's own skin space, which only matches the global space for shapes like
// bodies and outfits: a facegen head's is the head's.
std::vector<nifly::Vector3> McpGlobalLiveVerts(OutfitStudioMcp& mcp, nifly::NiShape* shape);

// The skeleton's bones with where the skin of each twist bone is, from the weights of the project's shapes
PoseMath::TwistContext McpTwistContext(OutfitStudioMcp& mcp);

// Signed distances of the vertices of a shape to the surface of another, as both are shown with the current slider
// values and pose. Negative inside, FLT_MAX where the surface is farther than searchRadius.
// The body poking out through the shape between its vertices counts too: the vertices of the triangles it goes
// through get minus the poke depth, unless they are deeper inside already.
std::vector<float> McpSignedDistances(OutfitStudioMcp& mcp, nifly::NiShape* shape, nifly::NiShape* against, float searchRadius = 5.0f);

// Distances of vertices whose closest point of the surface is on a triangle the analysis zaps remove become
// FLT_MAX: a zapped body can't show through. Measuring against only the triangles that are left would find a far
// away remnant instead.
void McpDropHiddenSurface(OutfitStudioMcp& mcp,
						  nifly::NiShape* against,
						  const std::vector<nifly::Vector3>& surfaceVerts,
						  const std::vector<nifly::Vector3>& verts,
						  std::vector<float>& distances);

// Where the surface (the body) pokes out through a shape between the shape's vertices
struct McpPokeStats {
	size_t bodyVertices = 0; // Surface vertices outside the shape where it covers them
	float maxDepth = 0.0f;
};

// Finds where the surface of 'against' pokes out through the shape, from the positions of both, and gives the
// vertices of the triangles it goes through minus the poke depth in distances, where that's deeper. Triangles
// with excluded vertices are skipped.
McpPokeStats McpMergePokeThrough(OutfitStudioMcp& mcp,
								 nifly::NiShape* shape,
								 const std::vector<nifly::Vector3>& verts,
								 nifly::NiShape* against,
								 const std::vector<nifly::Vector3>& surfaceVerts,
								 std::vector<float>& distances);

// Kinds of per-vertex measurements McpVertexMetric takes, larger is worse
const std::vector<std::string>& McpMetricNames();

// One value per vertex of a shape for the measurement in args["metric"] (see McpMetricNames), with its
// arguments (slider, value, sliderValues, against, maxDistance). Returns the default threshold for selecting
// bad vertices in outThreshold. Signed metrics (gap_change) are compared by their size.
std::vector<float> McpVertexMetric(OutfitStudioMcp& mcp, nifly::NiShape* shape, const nlohmann::json& args, float* outThreshold = nullptr);

// Argument schemas of McpVertexMetric, to merge into a tool's input schema
nlohmann::json McpMetricProperties();

// How the gap between vertices and a surface changes, from signed distances before and after: only vertices
// within maxDistance before count, and a change larger than threshold counts as tightened or loosened.
// outScore receives how bad it is, for sorting.
nlohmann::json McpGapStats(const std::vector<float>& before, const std::vector<float>& after, float maxDistance, float threshold, double* outScore);

// Leaves the excluded vertices of the shape (see OutfitStudioMcp::Excluded) out of a clipping measurement:
// their signed distance becomes FLT_MAX and the counts are made again from the distances, also when nothing is
// excluded. Returns how many were left out.
size_t McpApplyExclusions(OutfitStudioMcp& mcp, nifly::NiShape* shape, MeshAnalysis::PenetrationStats& stats, float nearDistance);

// Masks the excluded vertices of the shapes for as long as it lives, so clipping fixes leave them alone
class McpExclusionMaskScope {
public:
	McpExclusionMaskScope(OutfitStudioMcp& mcp, const std::vector<nifly::NiShape*>& shapes);
	~McpExclusionMaskScope();

private:
	std::vector<std::pair<std::string, std::unordered_map<int, float>>> saved;
	OutfitStudioMcp& mcp;
};

class Mesh;

// Mask counts of a mesh: masked (any value), fully and partly masked, and unmasked vertices
nlohmann::json McpMaskSummary(const Mesh& m);

// Masks the shapes in args by a measurement of McpVertexMetric: with target "fix" only the vertices found are left
// unmasked, so fixes change nothing else, with "protect" they are masked
McpToolResult McpMaskFromMetric(OutfitStudioMcp& mcp, const nlohmann::json& args);
