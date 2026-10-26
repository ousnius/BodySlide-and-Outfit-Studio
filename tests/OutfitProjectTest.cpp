#include "../src/components/MaskOps.h"
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
