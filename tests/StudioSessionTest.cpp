#include "components/PoseData.h"
#include "components/RefTemplates.h"
#include "mcp/OutfitStudioMcp.h"
#include "project/AutomationRunner.h"
#include "project/OutfitProject.h"
#include "project/ProjectOperations.h"
#include "project/StudioSession.h"
#include "render/ViewState.h"
#include "utils/ConfigurationManager.h"
#include "utils/MainThreadDispatcher.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

using nlohmann::json;
using nifly::NifFile;
using nifly::NiShape;
using nifly::Triangle;
using nifly::Vector2;
using nifly::Vector3;

namespace {
// Folder of its own for each test, so test processes running at the same time don't share files
struct TempFolder {
	std::filesystem::path path;

	TempFolder() {
		std::random_device random;
		path = std::filesystem::temp_directory_path() / ("BSOSTests-" + std::to_string(random()));
		std::filesystem::create_directories(path);
	}

	~TempFolder() {
		std::error_code error;
		std::filesystem::remove_all(path, error);
	}
};

struct TargetGameScope {
	int previous = 0;

	explicit TargetGameScope(TargetGame game) {
		previous = Config.GetIntValue("TargetGame");
		Config.SetValue("TargetGame", static_cast<int>(game));
	}

	~TargetGameScope() { Config.SetValue("TargetGame", previous); }
};

// Flat grid at height z, (n + 1) x (n + 1) vertices spanning 0 to 10, saved as a NIF with one shape
std::string WriteGridNif(const std::filesystem::path& dir, const std::string& fileName, const std::string& shapeName, int n, float z) {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	for (int y = 0; y <= n; y++)
		for (int x = 0; x <= n; x++) {
			verts.emplace_back(x * 10.0f / n, y * 10.0f / n, z);
			uvs.emplace_back(static_cast<float>(x) / n, static_cast<float>(y) / n);
		}
	for (int y = 0; y < n; y++)
		for (int x = 0; x < n; x++) {
			uint16_t i = static_cast<uint16_t>(y * (n + 1) + x);
			tris.emplace_back(i, static_cast<uint16_t>(i + 1), static_cast<uint16_t>(i + n + 1));
			tris.emplace_back(static_cast<uint16_t>(i + 1), static_cast<uint16_t>(i + n + 2), static_cast<uint16_t>(i + n + 1));
		}

	NifFile nif;
	nif.Create(nifly::NiVersion::getSSE());
	nif.CreateShapeFromData(shapeName, &verts, &tris, &uvs);

	auto path = dir / fileName;
	REQUIRE(nif.Save(path) == 0);
	return path.string();
}

// Unattended host with a CPU mesh per shape, the way the window keeps its GL meshes
class MeshHost : public SwitchableHost {
public:
	OutfitProject* project = nullptr;
	std::map<std::string, std::unique_ptr<Mesh>> meshes;

	MeshHost() { SetInteractive(false); }

	Mesh* GetShapeMesh(const std::string& shapeName) override {
		auto it = meshes.find(shapeName);
		return it != meshes.end() ? it->second.get() : nullptr;
	}

	void BuildMeshes() {
		meshes.clear();
		for (auto& name : project->GetWorkNif()->GetShapeNames())
			meshes[name] = Mesh::CreateFromShape(project->GetWorkNif(), name);
	}
};

// A session without a window or a view: what the MCP tools and the automation runner need to run headless
class TestSession : public IStudioSession {
public:
	AnimSkeleton skeleton;
	MeshHost host;
	OutfitProject project{skeleton, &host, false};
	EditorState editor;
	PoseDataCollection poses;
	std::vector<RefTemplate> refTemplates;
	MainThreadDispatcher dispatcher{[] {}};
	std::vector<std::string> selectedShapes;
	bool pendingChanges = false;
	int progressDepth = 0;

	TestSession() { host.project = &project; }

	OutfitProject* GetProject() override { return &project; }
	EditorState& Editor() override { return editor; }
	SwitchableHost& Host() override { return host; }
	PoseDataCollection& Poses() override { return poses; }
	AnimSkeleton& Skeleton() override { return skeleton; }
	const std::vector<RefTemplate>& GetRefTemplates() override { return refTemplates; }
	void UpdateReferenceTemplates() override {}

	bool HasPendingChanges() override { return pendingChanges; }
	void SetPendingChanges(bool pending = true) override { pendingChanges = pending; }
	void UpdateTitle() override {}

	MainThreadDispatcher& Dispatcher() override { return dispatcher; }
	std::string GetBusyReason() override { return {}; }
	bool InProgress() override { return progressDepth > 0; }
	void StartProgress(const std::string&) override { progressDepth++; }
	void EndAllProgress() override { progressDepth = 0; }
	void SetProgressListener(std::function<void(int, const std::string&)>) override {}

	bool LoadProject(const std::string&, const std::string& = "") override { return false; }
	void ResetProject() override {}
	bool LoadMeshFromArchive(const std::string&, const std::string&, bool) override { return false; }
	int ImportOBJWithDialog(const std::string&) override { return 1; }
#ifdef USE_FBXSDK
	int ImportFBXWithDialog(const std::string&) override { return 1; }
#endif
	void RefreshArchiveBrowser() override {}
	void RetryMissingTextures() override {}

	NiShape* GetActiveShape() override { return nullptr; }
	std::vector<std::string> GetSelectedShapeNames() override { return selectedShapes; }
	bool SelectShapes(const std::vector<std::string>& shapeNames) override {
		selectedShapes = shapeNames;
		return true;
	}
	void SetBaseShape() override {}
	void UpdateShapeReference(NiShape*, NiShape*) override {}
	void RefreshGUIFromProj(bool = true, bool = true) override { host.BuildMeshes(); }
	void MeshFromProj(NiShape*, bool = false) override { host.BuildMeshes(); }
	void MeshesFromProj(bool = false) override { host.BuildMeshes(); }
	void RefreshSegmentTree() override {}
	void RefreshPartitionTree() override {}

	void CreateSetSliders() override {}
	std::string NewSlider(const std::string& suggestedName = "", bool = false) override { return suggestedName; }
	void SetSliderValue(size_t index, int val) override { project.SliderValue(index) = val / 100.0f; }
	void SetSliderValue(const std::string& name, int val) override { project.SliderValue(name) = val / 100.0f; }
	void ApplySliders(bool = true) override {}
	void EnterSliderEdit(const std::string& = "") override {}
	void ExitSliderEdit() override {}

	void GetNormalizeBones(std::vector<std::string>*, std::vector<std::string>*) override {}
	void RefreshGUIWeightColors() override {}
	void UpdateAnimationGUI() override {}
	void UpdateBones() override {}
	void PoseToGUI() override {}
	std::string SaveCurrentPose(const std::string&) override { return "No poses without a window"; }
	std::string SaveCurrentPoseFile(const std::string&) override { return "No poses without a window"; }
	void PoseToMesh() override {}

	Mesh* GetMesh(const std::string& shapeName) override { return host.GetShapeMesh(shapeName); }
	void RenameMesh(const std::string& shapeName, const std::string& newShapeName) override {
		auto it = host.meshes.find(shapeName);
		if (it == host.meshes.end())
			return;
		auto mesh = std::move(it->second);
		host.meshes.erase(it);
		mesh->shapeName = newShapeName;
		host.meshes[newShapeName] = std::move(mesh);
	}
	void GetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override {
		if (Mesh* m = GetMesh(shapeName))
			for (int i = 0; i < m->nVerts; i++)
				if (m->mask[i] != 0.0f)
					mask[i] = m->mask[i];
	}
	void GetShapeUnmasked(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override {
		if (Mesh* m = GetMesh(shapeName))
			for (int i = 0; i < m->nVerts; i++)
				if (m->mask[i] == 0.0f)
					mask[i] = m->mask[i];
	}
	void SetShapeMask(std::unordered_map<uint16_t, float>& mask, const std::string& shapeName) override {
		if (Mesh* m = GetMesh(shapeName))
			for (int i = 0; i < m->nVerts; i++)
				m->mask[i] = mask.count(i) ? mask[i] : 0.0f;
	}
	void InvertMaskTris(std::unordered_map<uint16_t, float>&, const std::string&) override {}
	std::unordered_map<std::string, std::vector<float>> StashMasks() override { return {}; }
	void UnstashMasks(const std::unordered_map<std::string, std::vector<float>>&) override {}
	void ApplyMaskChanges(const std::vector<Mesh*>& meshes, const std::function<std::unordered_map<int, float>(Mesh&)>& changesForMesh) override {
		for (Mesh* m : meshes)
			for (auto& change : changesForMesh(*m))
				m->mask[change.first] = change.second;
	}
	void FillVertexColors() override {}

	void ApplyUndoState(UndoStateProject*, bool, bool = true) override {}
	void ActiveShapesUpdated(UndoStateProject*, bool = false) override {}
	bool UndoStroke() override { return false; }
	bool RedoStroke() override { return false; }
	void UpdateUndoTools() override {}
	void RestoreEditSnapshot(const EditSnapshot& snapshot) override {
		editor.undoHistory.ClearHistory();
		ProjectOperations ops(project, editor);
		ApplyRefresh(ops.RestoreSnapshot(snapshot));
		ops.RestoreMasks(snapshot);
	}
	void ApplyRefresh(RefreshFlags flags) override {
		if (HasRefresh(flags, RefreshFlags::ShapeList) || HasRefresh(flags, RefreshFlags::Meshes))
			host.BuildMeshes();
	}
	bool GetSegmentMode() override { return false; }

	GLSurface* View() override { return nullptr; }
	void Render() override {}
	bool RenderViewImage(int, int, wxImage&) override { return false; }
	ViewState GetViewState() override { return {}; }
	void SetViewState(const ViewState&) override {}
	bool GetShapeDisplayMode(const std::string&, ShapeDisplayMode&) override { return false; }
	bool SetShapeDisplayMode(const std::string&, ShapeDisplayMode) override { return false; }
	bool FrameShapes(const std::vector<std::string>&) override { return false; }
	bool FrameVertices(const std::string&, const std::vector<int>&) override { return false; }
	bool FrameBone(const std::string&, float = 0.1f) override { return false; }
	bool IsOverlayShown(ViewOverlay) override { return false; }
	void ShowOverlay(ViewOverlay, bool) override {}
	Physics::XmlEditSession* PhysicsXmlSession() override { return nullptr; }
	void PhysicsXmlChanged(const std::string&) override {}
};

// A session with a body grid as the reference, with a "Grow" slider, and a skirt grid above it
struct SessionFixture {
	TargetGameScope targetGame{SKYRIMSE};
	TempFolder folder;
	TestSession session;

	SessionFixture() {
		OutfitProject& project = session.project;
		REQUIRE(project.LoadReferenceNif(WriteGridNif(folder.path, "Body.nif", "Body", 10, 0.0f), "Body") == 0);
		REQUIRE(project.ImportNIF(WriteGridNif(folder.path, "Skirt.nif", "Skirt", 6, 0.5f), false) == 0);

		NiShape* body = project.GetWorkNif()->FindBlockByName<NiShape>("Body");
		REQUIRE(body);
		project.AddEmptySlider("Grow");
		TargetDataDiffs grow;
		for (uint16_t i = 0; i < body->GetNumVertices(); i++)
			grow[i] = Vector3(0.0f, 0.0f, 1.0f);
		project.SetSliderFromDiff("Grow", body, grow);

		session.host.BuildMeshes();
	}

	bool HasShape(const std::string& name) { return session.project.GetWorkNif()->FindBlockByName<NiShape>(name) != nullptr; }
};
} // namespace

TEST_CASE("Automation steps run on a session without a window", "[StudioSession]") {
	SessionFixture f;
	AutomationRunner runner(f.session);
	runner.SetHeadless(true);
	runner.BeginRun({});

	AutomationStep rename;
	rename.type = AutomationStepType::RenameShape;
	rename.renameOldName = "Skirt";
	rename.renameNewName = "Dress";
	REQUIRE(runner.RunStep(rename) == 0);
	CHECK(f.HasShape("Dress"));
	CHECK_FALSE(f.HasShape("Skirt"));
	CHECK(f.session.GetMesh("Dress"));

	AutomationStep deleteSlider;
	deleteSlider.type = AutomationStepType::DeleteSlider;
	deleteSlider.deleteSliderNames = {"Grow"};
	REQUIRE(runner.RunStep(deleteSlider) == 0);
	CHECK(f.session.project.SliderCount() == 0);
}

TEST_CASE("MCP tools read and tidy physics XMLs", "[StudioSession][PhysicsXml]") {
	SessionFixture f;
	OutfitStudioMcp mcp(f.session);

	auto write = [&](const std::string& name, const std::string& xml) {
		const std::filesystem::path path = f.folder.path / name;
		std::ofstream(path, std::ios::binary) << xml;
		return path.u8string();
	};

	// The constraint creates both its bones from the first default, so the later <bone> for one of them is skipped
	const std::string hair = write("hair.xml", R"(<system>
	<bone-default><mass>1</mass></bone-default>
	<generic-constraint bodyA="Hair 1" bodyB="NPC Head [Head]"/>
	<bone name="Hair 1"><mass>2</mass></bone>
	<bone-default><mass>3</mass></bone-default>
	<bone name="Hair 2"/>
	<per-vertex-shape name="Hair"><tag>hair</tag><no-collide-with-tag>hair</no-collide-with-tag></per-vertex-shape>
	<per-triangle-shape name="Head"><tag>head</tag></per-triangle-shape>
</system>)");

	McpToolResult inspect = mcp.CallTool("inspect_physics_xml", {{"xml", json::array({hair})}});
	REQUIRE_FALSE(inspect.isError);
	const json& file = inspect.data["files"][0];
	CHECK(file["needsTidy"] == true);
	CHECK(file["ignoredBoneElements"] == json::array({"Hair 1"}));
	REQUIRE(file["implicitBones"].size() == 2);
	CHECK(file["implicitBones"][0]["name"] == "Hair 1");
	CHECK(file["implicitBones"][0]["values"]["mass"] == 1.0f);
	REQUIRE(file["bones"].size() == 1);
	CHECK(file["bones"][0]["name"] == "Hair 2");
	CHECK(file["bones"][0]["profile"] == "Default 2");
	CHECK(file["bones"][0]["values"]["mass"] == 3.0f);

	McpToolResult collisions = mcp.CallTool("physics_collisions", {{"xml", json::array({hair})}});
	REQUIRE_FALSE(collisions.isError);
	REQUIRE(collisions.data["pairs"].size() == 1);
	CHECK(collisions.data["pairs"][0]["collides"] == true);

	// The shapes create their bones from the second default, the constraint from the first
	McpToolResult refused = mcp.CallTool("tidy_physics_xml", {{"xml", hair}, {"overwrite", true}});
	REQUIRE_FALSE(refused.isError);
	CHECK(refused.data["ok"] == false);

	const std::string simple = write("simple.xml", R"(<system>
	<bone name="Root"/>
	<bone-default><mass>1</mass></bone-default>
	<bone name="Tip"/>
</system>)");
	CHECK(mcp.CallTool("tidy_physics_xml", {{"xml", simple}}).isError);

	McpToolResult tidied = mcp.CallTool("tidy_physics_xml", {{"xml", simple}, {"overwrite", true}});
	REQUIRE_FALSE(tidied.isError);
	CHECK(tidied.data["ok"] == true);

	McpToolResult after = mcp.CallTool("inspect_physics_xml", {{"xml", json::array({simple})}});
	REQUIRE_FALSE(after.isError);
	const json& tidiedFile = after.data["files"][0];
	CHECK_FALSE(tidiedFile.contains("needsTidy"));
	CHECK(tidiedFile["bones"][0]["profile"] == "Schema defaults");
	CHECK(tidiedFile["bones"][1]["values"]["mass"] == 1.0f);
}

TEST_CASE("MCP tools run on a session without a window", "[StudioSession]") {
	SessionFixture f;
	OutfitStudioMcp mcp(f.session);

	McpToolResult status = mcp.CallTool("get_status", json::object());
	REQUIRE_FALSE(status.isError);
	CHECK(status.data["outfitShapes"] == json::array({"Skirt"}));
	CHECK(status.data["reference"]["shape"] == "Body");
	CHECK(status.data["sliderCount"] == 1);

	CHECK(mcp.CallTool("no_such_tool", json::object()).isError);
	CHECK(mcp.CallTool("rename_shape", {{"shape", "Missing"}, {"newName", "Dress"}}).isError);

	// A tool the window can't undo takes an automatic snapshot first, which brings the shape back
	McpToolResult rename = mcp.CallTool("rename_shape", {{"shape", "Skirt"}, {"newName", "Dress"}});
	REQUIRE_FALSE(rename.isError);
	CHECK(f.HasShape("Dress"));
	CHECK(f.session.GetMesh("Dress"));
	REQUIRE(rename.data["autoSnapshot"].is_string());

	McpToolResult restore = mcp.CallTool("snapshot_restore", {{"name", rename.data["autoSnapshot"]}});
	REQUIRE_FALSE(restore.isError);
	CHECK(f.HasShape("Skirt"));
	CHECK_FALSE(f.HasShape("Dress"));
	CHECK(f.session.GetMesh("Skirt"));
}
