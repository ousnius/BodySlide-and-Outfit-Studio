#include "../src/components/AddOnSliders.h"
#include "../src/components/MaskOps.h"
#include "../src/components/SliderSetIndex.h"
#include "../src/program/BrushStrokeController.h"
#include "../src/program/OutfitProject.h"
#include "../src/program/ProjectOperations.h"
#include "../src/utils/ConfigurationManager.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>

#include <filesystem>
#include <map>
#include <memory>
#include <random>

extern ConfigurationManager Config;

using nifly::NifFile;
using nifly::NiShape;
using nifly::Triangle;
using nifly::Vector2;
using nifly::Vector3;

namespace {
// Flat grid in the XY plane at height z, (n + 1) x (n + 1) vertices spanning 0 to size
void MakeGrid(int n, float size, float z, std::vector<Vector3>& verts, std::vector<Triangle>& tris, std::vector<Vector2>& uvs) {
	verts.clear();
	tris.clear();
	uvs.clear();
	for (int y = 0; y <= n; y++) {
		for (int x = 0; x <= n; x++) {
			verts.emplace_back(x * size / n, y * size / n, z);
			uvs.emplace_back(static_cast<float>(x) / n, static_cast<float>(y) / n);
		}
	}

	for (int y = 0; y < n; y++) {
		for (int x = 0; x < n; x++) {
			uint16_t i = static_cast<uint16_t>(y * (n + 1) + x);
			uint16_t right = i + 1;
			uint16_t up = static_cast<uint16_t>(i + n + 1);
			tris.emplace_back(i, right, up);
			tris.emplace_back(right, static_cast<uint16_t>(up + 1), up);
		}
	}
}

// Folder of its own for each fixture, so test processes running at the same time don't share files
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

// Sets the target game for the lifetime of a fixture, also when its setup fails part way
struct TargetGameScope {
	int previous = 0;

	explicit TargetGameScope(TargetGame game) {
		previous = Config.GetIntValue("TargetGame");
		Config.SetValue("TargetGame", static_cast<int>(game));
	}

	~TargetGameScope() { Config.SetValue("TargetGame", previous); }
};

// Points the project path at a folder for the lifetime of a fixture
struct ProjectPathScope {
	std::string previousProjectPath;
	std::string previousAppDir;

	explicit ProjectPathScope(const std::filesystem::path& path) {
		previousProjectPath = Config["ProjectPath"];
		previousAppDir = Config["AppDir"];
		Config.SetValue("ProjectPath", path.u8string());
		Config.SetValue("AppDir", path.u8string());
		std::filesystem::create_directories(path / "SliderSets");
		std::filesystem::create_directories(path / "ShapeData");
	}

	~ProjectPathScope() {
		Config.SetValue("ProjectPath", previousProjectPath);
		Config.SetValue("AppDir", previousAppDir);
	}
};

std::string WriteGridNif(const std::filesystem::path& dir, const std::string& fileName, const std::string& shapeName, int n, float z) {
	std::vector<Vector3> verts;
	std::vector<Triangle> tris;
	std::vector<Vector2> uvs;
	MakeGrid(n, 10.0f, z, verts, tris, uvs);

	NifFile nif;
	nif.Create(nifly::NiVersion::getSSE());
	nif.CreateShapeFromData(shapeName, &verts, &tris, &uvs);

	auto path = dir / fileName;
	REQUIRE(nif.Save(path) == 0);
	return path.string();
}

// Unattended host that keeps a CPU mesh per shape, the way the window keeps its GL meshes
class MeshHost : public NonInteractiveHost {
public:
	OutfitProject* project = nullptr;
	std::map<std::string, std::unique_ptr<Mesh>> meshes;

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

// A body grid with a "Grow" slider that lifts it by 1 and two bones weighted along X,
// and a coarser skirt grid half a unit above it.
struct ProjectFixture {
	TargetGameScope targetGame{SKYRIMSE};
	TempFolder folder;
	MeshHost host;
	OutfitProject project{&host, false};
	EditorState editor;
	ProjectOperations ops{project, editor};
	NiShape* body = nullptr;
	NiShape* skirt = nullptr;

	explicit ProjectFixture(float skirtHeight = 0.5f) {
		host.project = &project;

		REQUIRE(project.LoadReferenceNif(WriteGridNif(folder.path, "Body.nif", "Body", 10, 0.0f), "Body") == 0);
		REQUIRE(project.ImportNIF(WriteGridNif(folder.path, "Skirt.nif", "Skirt", 6, skirtHeight), false) == 0);
		body = project.GetWorkNif()->FindBlockByName<NiShape>("Body");
		skirt = project.GetWorkNif()->FindBlockByName<NiShape>("Skirt");
		REQUIRE(body);
		REQUIRE(skirt);
		REQUIRE(project.IsBaseShape(body));

		project.AddEmptySlider("Grow");
		TargetDataDiffs grow;
		for (uint16_t i = 0; i < body->GetNumVertices(); i++)
			grow[i] = Vector3(0.0f, 0.0f, 1.0f);
		project.SetSliderFromDiff("Grow", body, grow);

		project.CreateSkinning(body);
		project.CreateSkinning(skirt);
		project.AddCustomBoneRef("BoneLeft", "", nifly::MatTransform());
		project.AddCustomBoneRef("BoneRight", "", nifly::MatTransform());

		std::vector<Vector3> verts;
		project.GetWorkNif()->GetVertsForShape(body, verts);
		std::unordered_map<uint16_t, float> left, right;
		for (uint16_t i = 0; i < verts.size(); i++) {
			right[i] = verts[i].x / 10.0f;
			left[i] = 1.0f - right[i];
		}
		project.GetWorkAnim()->SetWeights("Body", "BoneLeft", left);
		project.GetWorkAnim()->SetWeights("Body", "BoneRight", right);

		host.BuildMeshes();
	}

	// What the window does with the refresh flags, as far as the meshes are concerned
	void ApplyRefresh(RefreshFlags flags) {
		if (HasRefresh(flags, RefreshFlags::Meshes))
			host.BuildMeshes();
	}

	float Weight(NiShape* shape, const std::string& bone, uint16_t vertex) {
		auto weights = project.GetWorkAnim()->GetWeightsPtr(shape->name.get(), bone);
		if (!weights)
			return 0.0f;
		auto it = weights->find(vertex);
		return it != weights->end() ? it->second : 0.0f;
	}
};
} // namespace

TEST_CASE("Unchecked sliders don't move the shapes", "[OutfitProject]") {
	ProjectFixture f;
	REQUIRE(f.ops.SetSliderValues({{"Grow", 1.0f}}).ok);

	std::vector<Vector3> verts;
	f.project.GetMorphedVerts(f.body, verts);
	CHECK(verts[0].z == Catch::Approx(1.0f));

	REQUIRE(f.ops.SetSlidersChecked({{"Grow", false}}).ok);
	CHECK_FALSE(f.project.SliderShow("Grow"));
	f.project.GetMorphedVerts(f.body, verts);
	CHECK(verts[0].z == Catch::Approx(0.0f));

	// An unknown slider fails without changing the others
	CHECK_FALSE(f.ops.SetSlidersChecked({{"Grow", true}, {"Missing", true}}).ok);
	CHECK_FALSE(f.project.SliderShow("Grow"));

	// The slider in edit mode stays checked
	REQUIRE(f.ops.SetSlidersChecked({{"Grow", true}}).ok);
	f.editor.activeSlider = "Grow";
	f.editor.bEditSlider = true;
	CHECK_FALSE(f.ops.SetSlidersChecked({{"Grow", false}}).ok);
	CHECK(f.project.SliderShow("Grow"));
}

TEST_CASE("Meshes are built from shapes without GL", "[OutfitProject]") {
	ProjectFixture f;

	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);
	CHECK(m->nVerts == 49);
	CHECK(m->nTris == 72);
	CHECK_FALSE(m->genBuffers);
	CHECK(m->bvh);
}

TEST_CASE("Conforming carries the reference morph over to the outfit", "[OutfitProject]") {
	ProjectFixture f;

	OpResult result = f.ops.ConformSliders({f.skirt}, ConformOptions());
	REQUIRE(result.ok);

	std::vector<Vector3> deltas;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", deltas).ok);
	REQUIRE(deltas.size() == f.skirt->GetNumVertices());
	for (auto& d : deltas) {
		CHECK(d.x == Catch::Approx(0.0f).margin(1e-4));
		CHECK(d.y == Catch::Approx(0.0f).margin(1e-4));
		CHECK(d.z == Catch::Approx(1.0f).margin(1e-4));
	}
}

TEST_CASE("Bone weights are copied from the reference to unmasked vertices", "[OutfitProject]") {
	ProjectFixture f;

	// Mask the first row of the skirt, it keeps having no weights
	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);
	std::vector<float> mask(m->nVerts, 0.0f);
	for (int i = 0; i <= 6; i++)
		mask[i] = 1.0f;
	REQUIRE(MaskOps::SetMask(*m, mask));

	UndoStateProject usp;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, BoneWeightCopyOptions(), usp).ok);

	std::vector<Vector3> verts;
	f.project.GetWorkNif()->GetVertsForShape(f.skirt, verts);
	for (uint16_t i = 0; i < verts.size(); i++) {
		float left = f.Weight(f.skirt, "BoneLeft", i);
		float right = f.Weight(f.skirt, "BoneRight", i);
		if (i <= 6) {
			CHECK(left == 0.0f);
			CHECK(right == 0.0f);
			continue;
		}

		CHECK(left + right == Catch::Approx(1.0f).margin(1e-3));
		// The weights follow the reference across the grid, smoothed by the proximity blend
		CHECK(right == Catch::Approx(verts[i].x / 10.0f).margin(0.15));
	}
}

TEST_CASE("Weight copies can keep the weights of bones they don't copy", "[OutfitProject]") {
	ProjectFixture f;

	// A physics chain bone only the skirt has
	f.project.AddCustomBoneRef("Chain", "", nifly::MatTransform());
	f.project.GetWorkAnim()->RemoveShapeBone("Body", "Chain");
	std::unordered_map<uint16_t, float> chain{{10, 0.4f}, {20, 1.0f}};
	f.project.GetWorkAnim()->SetWeights("Skirt", "Chain", chain);

	BoneWeightCopyOptions options;
	options.keepOtherBones = GENERATE(false, true);
	UndoStateProject usp;
	std::map<std::string, BoneWeightCopyReport> reports;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, options, usp, &reports).ok);

	if (options.keepOtherBones) {
		CHECK(reports["Skirt"].keptBones == std::vector<std::string>{"Chain"});
		CHECK(f.Weight(f.skirt, "Chain", 10) == Catch::Approx(0.4f));
		CHECK(f.Weight(f.skirt, "Chain", 20) == Catch::Approx(1.0f));
		// The copied bones share what's left
		CHECK(f.Weight(f.skirt, "BoneLeft", 10) + f.Weight(f.skirt, "BoneRight", 10) == Catch::Approx(0.6f).margin(1e-3));
		CHECK(f.Weight(f.skirt, "BoneLeft", 20) + f.Weight(f.skirt, "BoneRight", 20) == Catch::Approx(0.0f).margin(1e-3));
	}
	else {
		// Like the window: the copied bones take the vertices over
		CHECK(reports["Skirt"].keptBones.empty());
		CHECK(f.Weight(f.skirt, "Chain", 10) == 0.0f);
		CHECK(f.Weight(f.skirt, "Chain", 20) == 0.0f);
		CHECK(f.Weight(f.skirt, "BoneLeft", 20) + f.Weight(f.skirt, "BoneRight", 20) == Catch::Approx(1.0f).margin(1e-3));
	}
}

TEST_CASE("Weight copies don't keep reference bones they leave out", "[OutfitProject]") {
	ProjectFixture f;

	// The skirt is all on BoneRight, which the copy leaves out like an excluded bone
	std::unordered_map<uint16_t, float> right;
	for (uint16_t i = 0; i < f.skirt->GetNumVertices(); i++)
		right[i] = 1.0f;
	f.project.GetWorkAnim()->SetWeights("Skirt", "BoneRight", right);

	BoneWeightCopyOptions options;
	options.keepOtherBones = true;
	options.selectedBones = {"BoneLeft"};
	UndoStateProject usp;
	std::map<std::string, BoneWeightCopyReport> reports;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, options, usp, &reports).ok);

	CHECK(reports["Skirt"].keptBones.empty());
	// The left edge of the skirt is all BoneLeft on the body, so BoneRight gives way there
	CHECK(f.Weight(f.skirt, "BoneLeft", 0) > 0.9f);
	CHECK(f.Weight(f.skirt, "BoneRight", 0) < 0.1f);
}

TEST_CASE("Weight copies only add the bones they may", "[OutfitProject]") {
	ProjectFixture f;
	f.project.GetWorkAnim()->RemoveShapeBone("Skirt", "BoneRight");

	BoneWeightCopyOptions options;
	options.mayAddBone = [](const std::string& shapeName, const std::string& bone) { return !(shapeName == "Skirt" && bone == "BoneRight"); };
	UndoStateProject usp;
	std::map<std::string, BoneWeightCopyReport> reports;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, options, usp, &reports).ok);

	CHECK(reports["Skirt"].notAdded == std::vector<std::string>{"BoneRight"});
	const auto& bones = f.project.GetWorkAnim()->shapeBones["Skirt"];
	CHECK(std::find(bones.begin(), bones.end(), "BoneRight") == bones.end());
	CHECK(f.Weight(f.skirt, "BoneLeft", 0) > 0.9f);
}

TEST_CASE("Weight copies can keep parts on one bone rigid", "[OutfitProject]") {
	ProjectFixture f;

	// The whole skirt is one part, on BoneLeft
	std::unordered_map<uint16_t, float> left;
	for (uint16_t i = 0; i < f.skirt->GetNumVertices(); i++)
		left[i] = 1.0f;
	const bool rigid = GENERATE(true, false);
	if (!rigid)
		left[24] = 0.9f;
	f.project.GetWorkAnim()->SetWeights("Skirt", "BoneLeft", left);

	BoneWeightCopyOptions options;
	options.keepRigidParts = true;
	UndoStateProject usp;
	std::map<std::string, BoneWeightCopyReport> reports;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, options, usp, &reports).ok);

	// The last skirt vertex sits at the right edge, where the body is all BoneRight
	const uint16_t corner = static_cast<uint16_t>(f.skirt->GetNumVertices() - 1);
	if (rigid) {
		REQUIRE(reports["Skirt"].rigidParts.size() == 1);
		CHECK(reports["Skirt"].rigidParts[0].first == f.skirt->GetNumVertices());
		CHECK(reports["Skirt"].rigidParts[0].second == "BoneLeft");
		CHECK(f.Weight(f.skirt, "BoneLeft", corner) == 1.0f);
	}
	else {
		CHECK(reports["Skirt"].rigidParts.empty());
		CHECK(f.Weight(f.skirt, "BoneRight", corner) > 0.5f);
	}
}

TEST_CASE("A snapshot brings back slider data, weights and masks", "[OutfitProject]") {
	ProjectFixture f;
	REQUIRE(f.ops.ConformSliders({f.skirt}, ConformOptions()).ok);

	std::vector<Vector3> before;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", before).ok);

	Mesh* m = f.host.GetShapeMesh("Skirt");
	std::vector<float> mask(m->nVerts, 0.0f);
	mask[3] = 1.0f;
	REQUIRE(MaskOps::SetMask(*m, mask));

	auto snapshot = f.ops.CaptureSnapshot();
	REQUIRE(snapshot);

	REQUIRE(f.ops.ScaleSliderDeltas(f.skirt, "Grow", 3.0f).ok);
	UndoStateProject usp;
	REQUIRE(f.ops.CopyBoneWeights({f.skirt}, BoneWeightCopyOptions(), usp).ok);
	MaskOps::SetMask(*m, std::vector<float>(m->nVerts, 0.0f));
	REQUIRE(f.Weight(f.skirt, "BoneLeft", 0) > 0.0f);

	f.ops.RestoreSnapshot(*snapshot);
	f.skirt = f.project.GetWorkNif()->FindBlockByName<NiShape>("Skirt");
	REQUIRE(f.skirt);
	f.host.BuildMeshes();
	f.ops.RestoreMasks(*snapshot);

	std::vector<Vector3> after;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", after).ok);
	REQUIRE(after.size() == before.size());
	for (size_t i = 0; i < after.size(); i++)
		CHECK(after[i].z == Catch::Approx(before[i].z));

	CHECK(f.Weight(f.skirt, "BoneLeft", 0) == 0.0f);

	m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);
	CHECK(m->mask[3] == 1.0f);
	CHECK(m->mask[4] == 0.0f);
}

TEST_CASE("Brush strokes run on meshes without input events", "[OutfitProject]") {
	ProjectFixture f;
	BrushStrokeController controller(f.project, f.editor);
	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);

	// Center of the skirt grid, and a corner far outside the brush
	const int center = 24;
	const int corner = 0;
	const bool masked = GENERATE(false, true);
	if (masked) {
		std::vector<float> mask(m->nVerts, 0.0f);
		mask[center] = 1.0f;
		REQUIRE(MaskOps::SetMask(*m, mask));
	}

	TB_Inflate inflate;
	inflate.setRadius(0.3f);
	inflate.setStrength(0.5f);

	TweakPickInfo pick = BrushStrokeController::PickAtVertex(m, center);
	controller.Begin({m}, &inflate, pick);
	REQUIRE(controller.IsActive());
	controller.Update(pick);
	f.ApplyRefresh(controller.End());
	CHECK_FALSE(controller.IsActive());
	CHECK(f.editor.undoHistory.GetCurState());

	std::vector<Vector3> verts;
	f.project.GetWorkNif()->GetVertsForShape(f.skirt, verts);
	CHECK(verts[corner].z == Catch::Approx(0.5f));
	if (masked)
		CHECK(verts[center].z == Catch::Approx(0.5f));
	else
		CHECK(std::fabs(verts[center].z - 0.5f) > 0.01f);
}

TEST_CASE("Smoothing a mask turns its border into a gradient", "[OutfitProject]") {
	ProjectFixture f;
	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);

	// Columns 0 to 2 of the 7 x 7 skirt grid are masked
	std::vector<float> mask(m->nVerts, 0.0f);
	for (int y = 0; y <= 6; y++)
		for (int x = 0; x <= 2; x++)
			mask[y * 7 + x] = 1.0f;
	REQUIRE(MaskOps::SetMask(*m, mask));

	MaskOps::ApplyChanges(*m, MaskOps::SmoothChanges(*m, 0.5f));

	const int row = 3 * 7;
	CHECK(m->mask[row + 0] == Catch::Approx(1.0f));
	CHECK(m->mask[row + 2] < 1.0f);
	CHECK(m->mask[row + 2] > 0.5f);
	CHECK(m->mask[row + 3] > 0.0f);
	CHECK(m->mask[row + 3] < 0.5f);
	CHECK(m->mask[row + 6] == Catch::Approx(0.0f));
}

TEST_CASE("Fixing clipping leaves masked vertices alone", "[OutfitProject]") {
	// The skirt lies below the body grid, so it clips everywhere
	ProjectFixture f(-0.3f);
	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);

	std::vector<Vector3> before;
	f.project.GetWorkNif()->GetVertsForShape(f.skirt, before);

	// Columns 0 to 2 masked, column 3 half masked
	std::vector<float> mask(m->nVerts, 0.0f);
	for (int y = 0; y <= 6; y++) {
		for (int x = 0; x <= 2; x++)
			mask[y * 7 + x] = 1.0f;
		mask[y * 7 + 3] = 0.5f;
	}
	REQUIRE(MaskOps::SetMask(*m, mask));

	REQUIRE(f.ops.FixClipping({f.skirt}, ClippingFixMode::Shapes, 1.0f).ok);

	std::vector<Vector3> after;
	f.project.GetWorkNif()->GetVertsForShape(f.skirt, after);

	// Out of the body is along its normal, whichever way the grid faces
	const int row = 3 * 7;
	const float fullMove = std::abs(after[row + 5].z - before[row + 5].z);
	REQUIRE(fullMove > 0.01f);
	CHECK(after[row + 1].z == Catch::Approx(before[row + 1].z));
	CHECK(std::abs(after[row + 3].z - before[row + 3].z) == Catch::Approx(fullMove * 0.5f).margin(fullMove * 0.25f));
}

TEST_CASE("Rigidifying a slider keeps the unmasked part from bending, and can be undone", "[OutfitProject]") {
	ProjectFixture f;
	REQUIRE(f.ops.ConformSliders({f.skirt}, ConformOptions()).ok);

	// A bump in the middle of the skirt's morph
	REQUIRE(f.ops.SetSliderDeltas(f.skirt, "Grow", {{24, Vector3(0.0f, 0.0f, 2.0f)}}, false).ok);

	std::vector<Vector3> before;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", before).ok);

	// The first row is masked and keeps its deltas
	Mesh* m = f.host.GetShapeMesh("Skirt");
	REQUIRE(m);
	std::vector<float> mask(m->nVerts, 0.0f);
	for (int i = 0; i <= 6; i++)
		mask[i] = 1.0f;
	REQUIRE(MaskOps::SetMask(*m, mask));

	UndoStateProject usp;
	size_t parts = 0;
	OpResult result = f.ops.RigidifySlider({f.skirt}, "Grow", RigidifyOptions(), usp, &parts);
	REQUIRE(result.ok);
	CHECK(parts == 1);
	CHECK(usp.sliderName == "Grow");
	REQUIRE(usp.usss.size() == 1);

	std::vector<Vector3> after;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", after).ok);
	for (int i = 0; i <= 6; i++)
		CHECK(after[i].z == Catch::Approx(before[i].z));

	// The rest moves as a whole: the bump is gone, the plate tilts at most a little
	CHECK(after[24].z < 1.5f);
	for (size_t i = 7; i < after.size(); i++)
		CHECK(after[i].z == Catch::Approx(after[24].z).margin(0.1));

	// Undo puts the bump back
	f.ApplyRefresh(f.ops.ApplyShapeEdits(usp, true));
	std::vector<Vector3> undone;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", undone).ok);
	for (size_t i = 0; i < undone.size(); i++)
		CHECK(undone[i].z == Catch::Approx(before[i].z).margin(1e-4));
}

TEST_CASE("Adaptive refining splits only the chosen edges and keeps the mesh closed", "[OutfitProject]") {
	ProjectFixture f;
	REQUIRE(f.ops.ConformSliders({f.skirt}, ConformOptions()).ok);

	// The skirt's edges are 1.67 long, its diagonals 2.36
	AdaptiveRefineOptions options;
	options.select.maxEdgeLength = 2.0f;

	// With columns 0 to 2 masked, only the diagonals of the last three columns of cells have both ends free
	const bool masked = GENERATE(false, true);
	if (masked) {
		Mesh* m = f.host.GetShapeMesh("Skirt");
		REQUIRE(m);
		std::vector<float> mask(m->nVerts, 0.0f);
		for (int y = 0; y <= 6; y++)
			for (int x = 0; x <= 2; x++)
				mask[y * 7 + x] = 1.0f;
		REQUIRE(MaskOps::SetMask(*m, mask));
	}
	const size_t expected = masked ? 18 : 36;

	UndoStateProject usp;
	std::vector<AdaptiveRefineShape> found;
	OpResult result = f.ops.PrepareAdaptiveRefine({f.skirt}, options, usp, &found);
	REQUIRE(result.ok);
	REQUIRE(found.size() == 1);
	CHECK(found[0].selection.edges.size() == expected);
	CHECK(found[0].edges.size() == expected);
	CHECK(found[0].newVertices == expected);
	CHECK(found[0].newTriangles == expected * 2);
	CHECK(usp.undoType == UndoType::Mesh);
	REQUIRE(usp.usss.size() == 1);

	// Nothing changes until the state is applied
	CHECK(f.skirt->GetNumVertices() == 49);

	std::vector<float> noMask;
	f.project.ApplyShapeMeshUndo(f.skirt, noMask, usp.usss[0], false);
	f.ApplyRefresh(result.refresh);
	CHECK(f.skirt->GetNumVertices() == 49 + expected);
	CHECK(f.skirt->GetNumTriangles() == 72 + expected * 2);

	// Every inner edge still has a triangle on each side: no cracks where an edge was split on one side only
	std::vector<Triangle> tris;
	f.skirt->GetTriangles(tris);
	std::map<std::pair<uint16_t, uint16_t>, int> uses;
	for (auto& t : tris)
		for (int i = 0; i < 3; i++) {
			auto e = t.GetEdge(i);
			uses[{std::min(e.p1, e.p2), std::max(e.p1, e.p2)}]++;
		}
	size_t boundary = 0;
	for (auto& u : uses) {
		CHECK(u.second <= 2);
		boundary += u.second == 1 ? 1 : 0;
	}
	CHECK(boundary == 24);

	// The new vertices are on the flat skirt and follow its morph
	std::vector<Vector3> verts, deltas;
	f.project.GetWorkNif()->GetVertsForShape(f.skirt, verts);
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", deltas).ok);
	REQUIRE(deltas.size() == verts.size());
	for (size_t v = 49; v < verts.size(); v++) {
		CHECK(verts[v].z == Catch::Approx(0.5f).margin(1e-4));
		CHECK(deltas[v].z == Catch::Approx(1.0f).margin(1e-3));
	}

	// Undoing takes them out again
	f.project.ApplyShapeMeshUndo(f.skirt, noMask, usp.usss[0], true);
	CHECK(f.skirt->GetNumVertices() == 49);
	CHECK(f.skirt->GetNumTriangles() == 72);
}

namespace {
// The fixture's project, saved as set "A" into a project folder of its own
struct SavedProjectFixture : ProjectFixture {
	TempFolder projectFolder;
	ProjectPathScope projectPath{projectFolder.path};

	std::string Save(const std::string& setName = "A",
					 const std::string& nifName = "Shared.nif",
					 SharedFilesCheck check = SharedFilesCheck::Now,
					 const SliderSetIndex* index = nullptr) {
		wxFileName setFile(wxString::FromUTF8((projectFolder.path / "SliderSets" / (setName + ".osp")).u8string()));
		return project.Save(setFile, wxString::FromUTF8(setName), "Shared", wxString::FromUTF8(nifName), "meshes", setName, false, true, false, false, "", "", check, index);
	}

	std::filesystem::path DataPath(const std::string& fileName) const { return projectFolder.path / "ShapeData" / "Shared" / fileName; }

	// A second project in its own file that uses the shared NIF, with data of its own in the shared .osd
	void AddOtherProject() {
		SliderSet other;
		other.SetName("B");
		other.SetDataFolder("Shared");
		other.SetInputFile("Shared.nif");
		other.AddShapeTarget("Skirt", "Skirt");
		size_t slider = other.CreateSlider("Flare");
		other[slider].AddDataFile("Skirt", "SkirtFlare", "Shared.osd" + std::string(PathSepStr) + "SkirtFlare");

		SliderSetFile otherFile;
		otherFile.New((projectFolder.path / "SliderSets" / "B.osp").u8string());
		otherFile.UpdateSet(other);
		REQUIRE(otherFile.Save());

		OSDataFile osd;
		REQUIRE(osd.Read(DataPath("Shared.osd").u8string()));
		osd.SetDataDiff("SkirtFlare", TargetDataDiffs{{0, Vector3(1.0f, 0.0f, 0.0f)}});
		REQUIRE(osd.Write(DataPath("Shared.osd").u8string()));
	}

	std::set<std::string> OSDDataNames() const {
		OSDataFile osd;
		REQUIRE(osd.Read(DataPath("Shared.osd").u8string()));
		std::set<std::string> names;
		for (auto& data : osd.GetDataDiffs())
			names.insert(data.first);
		return names;
	}
};
} // namespace

TEST_CASE("Saving keeps the slider data of other projects in the same .osd file", "[OutfitProject]") {
	SavedProjectFixture f;
	REQUIRE(f.Save().empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow"});

	f.AddOtherProject();
	REQUIRE(f.Save().empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow", "SkirtFlare"});

	// Without the other project, its data isn't kept
	std::filesystem::remove(f.projectFolder.path / "SliderSets" / "B.osp");
	REQUIRE(f.Save().empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow"});
}

TEST_CASE("Saving asks before a shared NIF file loses shapes that other projects use", "[OutfitProject]") {
	SavedProjectFixture f;
	REQUIRE(f.Save().empty());
	f.AddOtherProject();

	// The unattended host answers no, so nothing is written
	f.project.DeleteShape(f.skirt);
	CHECK_FALSE(f.Save().empty());

	NifFile nif;
	REQUIRE(nif.Load(f.DataPath("Shared.nif")) == 0);
	CHECK(nif.FindBlockByName<NiShape>("Skirt"));

	// A NIF of its own is fine
	CHECK(f.Save("A", "Own.nif").empty());
}

TEST_CASE("Saving without looking for other projects keeps all other data in the .osd file", "[OutfitProject]") {
	SavedProjectFixture f;
	REQUIRE(f.Save().empty());
	f.AddOtherProject();
	std::filesystem::remove(f.projectFolder.path / "SliderSets" / "B.osp");

	// Nobody uses SkirtFlare any more, but that isn't known without looking
	REQUIRE(f.Save("A", "Shared.nif", SharedFilesCheck::Skip).empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow", "SkirtFlare"});

	REQUIRE(f.Save().empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow"});

	// It doesn't ask about shapes of the NIF file either
	f.AddOtherProject();
	f.project.DeleteShape(f.skirt);
	CHECK(f.Save("A", "Shared.nif", SharedFilesCheck::Skip).empty());
}

TEST_CASE("Saving uses the index loaded before for the same files", "[OutfitProject]") {
	SavedProjectFixture f;
	REQUIRE(f.Save().empty());
	f.AddOtherProject();
	const std::string projectPath = f.projectFolder.path.u8string();

	SliderSetIndex index(projectPath, {"Shared.nif", "Shared.osd"});
	REQUIRE(f.Save("A", "Shared.nif", SharedFilesCheck::Index, &index).empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow", "SkirtFlare"});

	std::filesystem::remove(f.projectFolder.path / "SliderSets" / "B.osp");
	SliderSetIndex withoutOther(projectPath, {"Shared.nif", "Shared.osd"});
	REQUIRE(f.Save("A", "Shared.nif", SharedFilesCheck::Index, &withoutOther).empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow"});

	// An index for other files says nothing about these, so all data is kept
	f.AddOtherProject();
	std::filesystem::remove(f.projectFolder.path / "SliderSets" / "B.osp");
	SliderSetIndex otherFiles(projectPath, {"Other.nif", "Other.osd"});
	REQUIRE(f.Save("A", "Shared.nif", SharedFilesCheck::Index, &otherFiles).empty());
	CHECK(f.OSDDataNames() == std::set<std::string>{"BodyGrow", "SkirtFlare"});
}

namespace {
// A variant or add-on of the saved fixture project "A", which has "Grow" data for the body and the skirt
struct DerivedProjectFixture : SavedProjectFixture {
	NonInteractiveHost derivedHost;
	OutfitProject derived{&derivedHost, false};

	explicit DerivedProjectFixture(bool addOn) {
		TargetDataDiffs grow;
		for (uint16_t i = 0; i < skirt->GetNumVertices(); i++)
			grow[i] = Vector3(0.0f, 0.0f, 0.5f);
		project.SetSliderFromDiff("Grow", skirt, grow);
		REQUIRE(Save().empty());

		REQUIRE(derived.CreateFromBase((projectFolder.path / "SliderSets" / "A.osp").u8string(), "A", addOn, addOn ? "A Extra" : "A Variant") == 0);
	}

	std::string SaveDerived(const std::string& nifName) {
		wxFileName setFile(wxString::FromUTF8((projectFolder.path / "SliderSets" / "Derived.osp").u8string()));
		return derived.Save(setFile, derived.mOutfitName, "Shared", wxString::FromUTF8(nifName), "meshes", "Derived", false, true, false, false, "", "");
	}

	SliderSet LoadSaved(const std::string& setName) {
		SliderSetFile file((projectFolder.path / "SliderSets" / "Derived.osp").u8string());
		SliderSet set;
		REQUIRE(file.GetSet(setName, set, true, "", false) == 0);
		return set;
	}

	std::set<std::string> DataNames(const std::string& osdName) const {
		std::set<std::string> names;
		OSDataFile osd;
		if (osd.Read(DataPath(osdName).u8string()))
			for (auto& data : osd.GetDataDiffs())
				names.insert(data.first);
		return names;
	}

	NiShape* DerivedShape(const std::string& name) { return derived.GetWorkNif()->FindBlockByName<NiShape>(name); }
};

const DiffInfo* FindDataFile(SliderSet& set, const std::string& sliderName, const std::string& target) {
	for (auto& df : set[sliderName].dataFiles)
		if (df.targetName == target)
			return &df;
	return nullptr;
}
} // namespace

TEST_CASE("Variants only save the slider data they change", "[OutfitProject]") {
	DerivedProjectFixture f(false);
	REQUIRE(f.SaveDerived("A Variant.nif").empty());

	SliderSet saved = f.LoadSaved("A Variant");
	CHECK(saved.GetBaseProjectName() == "A");
	CHECK_FALSE(saved.GetBaseProjectID().empty());
	CHECK_FALSE(saved.GetProjectID().empty());
	CHECK(saved.GetProjectID() != saved.GetBaseProjectID());
	CHECK(saved["Grow"].bInherited);
	CHECK(FindDataFile(saved, "Grow", "Skirt")->bInherited);
	CHECK(f.DataNames("A Variant.osd").empty());
	CHECK(f.DataNames("Shared.osd") == std::set<std::string>{"BodyGrow", "SkirtGrow"});

	// Editing inherited data makes it an override of the variant's own
	NiShape* skirt = f.DerivedShape("Skirt");
	REQUIRE(skirt);
	f.derived.EnsureSliderDataLocal("Grow", skirt);
	f.derived.SetSliderFromDiff("Grow", skirt, TargetDataDiffs{{0, Vector3(0.0f, 0.0f, 2.0f)}});
	REQUIRE(f.SaveDerived("A Variant.nif").empty());

	saved = f.LoadSaved("A Variant");
	CHECK_FALSE(FindDataFile(saved, "Grow", "Skirt")->bInherited);
	CHECK(FindDataFile(saved, "Grow", "Body")->bInherited);
	CHECK(f.DataNames("A Variant.osd") == std::set<std::string>{"SkirtGrow"});

	std::vector<SliderDataLocation> locations;
	f.derived.GetSliderDataLocations(locations, "Grow");
	for (auto& location : locations) {
		CHECK(location.inherited == (location.key.targetName == "Body"));
		CHECK(location.overridden == (location.key.targetName == "Skirt"));
	}

	// Reverting brings back the base project's data
	EditorState editor;
	ProjectOperations ops{f.derived, editor};
	std::vector<Vector3> deltas;
	REQUIRE(ops.GetSliderDeltas(skirt, "Grow", deltas).ok);
	CHECK(deltas[0].z == Catch::Approx(2.0f));

	SliderDataKey skirtKey{"Grow", "Skirt", "SkirtGrow"};
	REQUIRE(f.derived.RevertSliderDataToBase(skirtKey));
	REQUIRE(ops.GetSliderDeltas(skirt, "Grow", deltas).ok);
	CHECK(deltas[0].z == Catch::Approx(0.5f));
	CHECK(deltas[5].z == Catch::Approx(0.5f));

	REQUIRE(f.SaveDerived("A Variant.nif").empty());
	saved = f.LoadSaved("A Variant");
	CHECK(FindDataFile(saved, "Grow", "Skirt")->bInherited);
}

TEST_CASE("A variant can't save over the NIF file of its base project", "[OutfitProject]") {
	DerivedProjectFixture f(false);
	CHECK_FALSE(f.SaveDerived("Shared.nif").empty());
	CHECK(f.SaveDerived("A Variant.nif").empty());
}

TEST_CASE("Variants hide the base sliders they delete or rename", "[OutfitProject]") {
	DerivedProjectFixture f(false);
	f.derived.AddEmptySlider("Own");
	CHECK_FALSE(f.derived.SliderFromBase("Own"));
	CHECK(f.derived.SliderFromBase("Grow"));

	// Renaming takes the slider's data from the base project
	CHECK(f.derived.RenameDisconnectsFromBase("Grow", "").size() == 2);
	CHECK(f.derived.RenameDisconnectsFromBase("", "Skirt").size() == 1);
	CHECK(f.derived.RenameDisconnectsFromBase("Own", "").empty());

	size_t growIndex = 0;
	REQUIRE(f.derived.SliderIndexFromName("Grow", growIndex));
	f.derived.SetSliderName(growIndex, "Lift");
	CHECK(f.derived.RenameDisconnectsFromBase("Lift", "").empty());
	REQUIRE(f.SaveDerived("A Variant.nif").empty());

	SliderSet saved = f.LoadSaved("A Variant");
	CHECK(saved.GetRemovedSliders() == std::vector<std::string>{"Grow"});
	CHECK_FALSE(saved["Lift"].bInherited);
	CHECK(f.DataNames("A Variant.osd") == std::set<std::string>{"BodyLift", "SkirtLift"});

	// With the base project merged in, Grow stays left out
	SliderSetFile file((f.projectFolder.path / "SliderSets" / "Derived.osp").u8string());
	SliderSet resolved;
	REQUIRE(file.GetSet("A Variant", resolved) == 0);
	CHECK_FALSE(resolved.SliderExists("Grow"));
	CHECK(resolved.SliderExists("Lift"));
}

TEST_CASE("Add-ons save their own slider data without a NIF file", "[OutfitProject]") {
	DerivedProjectFixture f(true);
	auto nifTime = std::filesystem::last_write_time(f.DataPath("Shared.nif"));

	f.derived.AddEmptySlider("Extra");
	NiShape* skirt = f.DerivedShape("Skirt");
	REQUIRE(skirt);
	f.derived.SetSliderFromDiff("Extra", skirt, TargetDataDiffs{{0, Vector3(1.0f, 0.0f, 0.0f)}});

	// The NIF name passed in doesn't matter, an add-on uses the base project's
	REQUIRE(f.SaveDerived("Ignored.nif").empty());
	CHECK_FALSE(std::filesystem::exists(f.DataPath("Ignored.nif")));
	CHECK(std::filesystem::last_write_time(f.DataPath("Shared.nif")) == nifTime);
	CHECK(f.DataNames("A Extra.osd") == std::set<std::string>{"SkirtExtra"});
	CHECK(f.DataNames("Shared.osd") == std::set<std::string>{"BodyGrow", "SkirtGrow"});

	SliderSetFile file((f.projectFolder.path / "SliderSets" / "Derived.osp").u8string());
	CHECK(file.IsAddOn("A Extra"));
	SliderSet saved = f.LoadSaved("A Extra");
	CHECK(saved.GetSliderDataFile() == "A Extra.osd");
	CHECK(saved.GetInputFile() == "Shared.nif");
	CHECK(saved.GetShapeVertexCount("Skirt") == skirt->GetNumVertices());

	// Changed shapes can't be saved with an add-on, the unattended host doesn't make it a variant
	f.derived.DeleteShape(skirt);
	CHECK_FALSE(f.SaveDerived("Ignored.nif").empty());
}

TEST_CASE("A copy of a project with the same NIF file becomes an add-on of it", "[OutfitProject]") {
	// Issue #626: the copy only added sliders, but saved everything into the original's .osd file
	DerivedProjectFixture f(false);
	std::string baseFile = (f.projectFolder.path / "SliderSets" / "A.osp").u8string();

	NonInteractiveHost copyHost;
	OutfitProject copy{&copyHost, false};
	REQUIRE(copy.LoadFromSliderSet(baseFile, "A") == 0);
	REQUIRE(copy.ReloadReferenceShape() == 0);
	copy.AddEmptySlider("Extra");
	NiShape* skirt = copy.GetWorkNif()->FindBlockByName<NiShape>("Skirt");
	copy.SetSliderFromDiff("Extra", skirt, TargetDataDiffs{{0, Vector3(1.0f, 0.0f, 0.0f)}});

	wxFileName copyFile(wxString::FromUTF8((f.projectFolder.path / "SliderSets" / "Derived.osp").u8string()));
	REQUIRE(copy.Save(copyFile, "B", "Shared", "Shared.nif", "meshes", "B", false, true, false, false, "", "").empty());
	CHECK(f.DataNames("Shared.osd") == std::set<std::string>{"BodyGrow", "SkirtGrow", "SkirtExtra"});

	CHECK(copy.BaseGeometryMatches(baseFile, "A"));
	REQUIRE(copy.SetBaseProject(baseFile, "A", true));
	CHECK(copy.SliderFromBase("Grow"));
	CHECK_FALSE(copy.SliderFromBase("Extra"));
	REQUIRE(copy.Save(copyFile, "B", "Shared", "Shared.nif", "meshes", "B", false, true, false, false, "", "").empty());
	CHECK(f.DataNames("B.osd") == std::set<std::string>{"SkirtExtra"});

	SliderSetFile savedFile(copyFile.GetFullPath().ToUTF8().data());
	CHECK(savedFile.IsAddOn("B"));
	SliderSet saved;
	REQUIRE(savedFile.GetSet("B", saved) == 0);
	CHECK(saved.GetResolveStatus().baseFound);
	CHECK(saved["Grow"].bInherited);
	CHECK(FindDataFile(saved, "Grow", "Skirt")->bInherited);
	CHECK_FALSE(FindDataFile(saved, "Extra", "Skirt")->bInherited);

	// The original no longer keeps data for the copy
	REQUIRE(f.Save().empty());
	CHECK(f.DataNames("Shared.osd") == std::set<std::string>{"BodyGrow", "SkirtGrow"});
}

TEST_CASE("Clearing the base project makes an add-on a project of its own", "[OutfitProject]") {
	DerivedProjectFixture f(true);
	f.derived.ClearBaseProject();
	CHECK_FALSE(f.derived.SliderFromBase("Grow"));
	CHECK(f.derived.mBaseFile == "A Extra.nif");

	REQUIRE(f.SaveDerived(f.derived.mBaseFile.ToUTF8().data()).empty());
	CHECK(std::filesystem::exists(f.DataPath("A Extra.nif")));
	CHECK(f.DataNames("A Extra.osd") == std::set<std::string>{"BodyGrow", "SkirtGrow"});

	SliderSetFile file((f.projectFolder.path / "SliderSets" / "Derived.osp").u8string());
	CHECK_FALSE(file.IsAddOn("A Extra"));
	SliderSet saved = f.LoadSaved("A Extra");
	CHECK_FALSE(saved.HasBaseProject());
	CHECK_FALSE(saved["Grow"].bInherited);
}

TEST_CASE("BodySlide conforms add-on sliders like Outfit Studio does", "[OutfitProject]") {
	ProjectFixture f;

	// A morph that differs per vertex, so that the weighting of the nearest vertices matters
	std::vector<Vector3> bodyVerts;
	f.project.GetWorkNif()->GetVertsForShape(f.body, bodyVerts);
	TargetDataDiffs grow;
	for (uint16_t i = 0; i < bodyVerts.size(); i++)
		grow[i] = Vector3(0.1f * bodyVerts[i].x, 0.0f, 0.5f + 0.2f * bodyVerts[i].y * bodyVerts[i].y);
	f.project.SetSliderFromDiff("Grow", f.body, grow);

	REQUIRE(f.ops.ConformSliders({f.skirt}, ConformOptions()).ok);
	std::vector<Vector3> expected;
	REQUIRE(f.ops.GetSliderDeltas(f.skirt, "Grow", expected).ok);

	// BodySlide has the body and the outfit in NIF files of their own
	NifFile bodyNif;
	NifFile skirtNif;
	REQUIRE(bodyNif.Load(f.folder.path / "Body.nif") == 0);
	REQUIRE(skirtNif.Load(f.folder.path / "Skirt.nif") == 0);
	AnimInfo bodyAnim;
	AnimInfo skirtAnim;
	LoadConformAnim(bodyAnim, bodyNif);
	LoadConformAnim(skirtAnim, skirtNif);

	DiffDataSets bodyDiffs;
	bodyDiffs.LoadSet("BodyGrow", "Body", grow);

	auto results = ConformShapesData(bodyNif, bodyNif.FindBlockByName<NiShape>("Body"), bodyAnim, bodyDiffs, skirtNif, skirtAnim, {"Skirt"}, {{"Grow", "BodyGrow"}});
	REQUIRE(results.count("Skirt"));
	const TargetDataDiffs& conformed = results["Skirt"]["Grow"];

	REQUIRE(expected.size() == f.skirt->GetNumVertices());
	for (uint16_t i = 0; i < expected.size(); i++) {
		auto diff = conformed.find(i);
		Vector3 actual = diff != conformed.end() ? diff->second : Vector3();
		CHECK(actual.x == Catch::Approx(expected[i].x).margin(1e-5));
		CHECK(actual.y == Catch::Approx(expected[i].y).margin(1e-5));
		CHECK(actual.z == Catch::Approx(expected[i].z).margin(1e-5));
	}
}

namespace {
// Saves an add-on "A Extra" of the saved fixture project "A" with an "Extra" slider for the body
std::string SaveBodyAddOn(SavedProjectFixture& f) {
	const std::string setsPath = (f.projectFolder.path / "SliderSets" / "A.osp").u8string();
	SliderSetFile baseFile(setsPath);
	REQUIRE_FALSE(baseFile.fail());
	SliderSet base;
	REQUIRE(baseFile.GetSet("A", base) == 0);

	SliderSet addOn;
	addOn.SetName("A Extra");
	addOn.SetProjectID(SliderSet::GenerateProjectID());
	addOn.SetDataFolder(base.GetDefaultDataFolder());
	addOn.SetInputFile(base.GetInputFile());
	addOn.SetOutputPath(base.GetOutputPath());
	addOn.SetOutputFile(base.GetOutputFile());
	addOn.SetAddOn(true);
	addOn.SetSliderDataFile("A Extra.osd");
	addOn.SetBaseProject("", "A", base.GetProjectID());
	for (auto shape = base.ShapesBegin(); shape != base.ShapesEnd(); ++shape)
		addOn.AddShapeTarget(shape->first, shape->second.targetShape);
	addOn[addOn.CreateSlider("Extra")].AddDataFile("Body", "BodyExtra", "A Extra.osd" + std::string(PathSepStr) + "BodyExtra");

	SliderSetFile file;
	file.New((f.projectFolder.path / "SliderSets" / "A Extra.osp").u8string());
	file.UpdateSet(addOn);
	REQUIRE(file.Save());

	OSDataFile osd;
	TargetDataDiffs extra;
	for (uint16_t i = 0; i < f.body->GetNumVertices(); i++)
		extra[i] = Vector3(0.0f, 0.0f, 2.0f);
	osd.SetDataDiff("BodyExtra", extra);
	REQUIRE(osd.Write(f.DataPath("A Extra.osd").u8string()));
	return addOn.GetProjectID();
}
} // namespace

TEST_CASE("Loading a reference can load the sliders of its add-ons", "[OutfitProject]") {
	SavedProjectFixture f;
	REQUIRE(f.Save().empty());
	const std::string addOnID = SaveBodyAddOn(f);

	ReferenceSource source;
	source.file = (f.projectFolder.path / "SliderSets" / "A.osp").u8string();
	source.set = "A";
	source.shape = "Body";
	source.mergeSliders = true;
	source.mergeZaps = true;
	source.loadAddOns = true;

	std::vector<std::string> addOnSliders;
	REQUIRE(f.ops.LoadReference(source, &addOnSliders).ok);
	f.host.BuildMeshes();
	CHECK(addOnSliders == std::vector<std::string>{"Extra"});
	REQUIRE(f.project.ValidSlider("Extra"));
	CHECK(f.project.activeSet["Extra"].addOnName == "A Extra");
	CHECK(f.project.activeSet["Extra"].addOnID == addOnID);

	NiShape* body = f.project.GetBaseShape();
	REQUIRE(body);
	std::vector<Vector3> deltas;
	REQUIRE(f.ops.GetSliderDeltas(body, "Extra", deltas).ok);
	CHECK(deltas[0].z == Catch::Approx(2.0f));

	// Conformed and saved, the slider still needs its add-on
	NiShape* skirt = f.project.GetWorkNif()->FindBlockByName<NiShape>("Skirt");
	REQUIRE(skirt);
	REQUIRE(f.ops.ConformSliders({skirt}, ConformOptions()).ok);
	REQUIRE(f.ops.GetSliderDeltas(skirt, "Extra", deltas).ok);
	CHECK(deltas[0].z == Catch::Approx(2.0f).margin(1e-4));

	REQUIRE(f.Save("B", "B.nif").empty());
	SliderSetFile savedFile((f.projectFolder.path / "SliderSets" / "B.osp").u8string());
	SliderSet saved;
	REQUIRE(savedFile.GetSet("B", saved) == 0);
	CHECK(saved["Extra"].addOnName == "A Extra");
	CHECK(saved["Extra"].addOnID == addOnID);
	CHECK_FALSE(saved["Grow"].NeedsAddOn());
}

TEST_CASE("An add-on of an outfit can load a reference with add-ons and save", "[OutfitProject]") {
	DerivedProjectFixture f(true);
	SaveBodyAddOn(f);

	EditorState editor;
	ProjectOperations ops{f.derived, editor};

	ReferenceSource source;
	source.file = (f.projectFolder.path / "SliderSets" / "A.osp").u8string();
	source.set = "A";
	source.shape = "Body";
	source.mergeSliders = true;
	source.mergeZaps = true;
	source.appendNewSliders = false;
	source.loadAddOns = true;
	REQUIRE(ops.LoadReference(source).ok);

	// Only the add-on slider stays shown, so that conforming leaves the inherited sliders alone
	REQUIRE(f.derived.ValidSlider("Extra"));
	CHECK(f.derived.SliderShow("Extra"));
	CHECK_FALSE(f.derived.SliderShow("Grow"));

	// Loading the reference isn't a change to the base project's shapes
	REQUIRE(f.SaveDerived("Ignored.nif").empty());
	SliderSet saved = f.LoadSaved("A Extra");
	CHECK(saved.IsAddOn());
	CHECK(saved["Extra"].addOnName == "A Extra");

	// A reference of another body replaces the base project's one, which isn't a change either
	REQUIRE(f.derived.LoadReferenceNif(WriteGridNif(f.folder.path, "Other.nif", "OtherBody", 8, 0.0f), "OtherBody", true, true) == 0);
	CHECK(f.SaveDerived("Ignored.nif").empty());

	// Changed outfit shapes still can't be saved with an add-on
	f.derived.DeleteShape(f.DerivedShape("Skirt"));
	CHECK_FALSE(f.SaveDerived("Ignored.nif").empty());
}
