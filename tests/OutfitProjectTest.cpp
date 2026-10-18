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

	ProjectFixture() {
		host.project = &project;

		REQUIRE(project.LoadReferenceNif(WriteGridNif(folder.path, "Body.nif", "Body", 10, 0.0f), "Body") == 0);
		REQUIRE(project.ImportNIF(WriteGridNif(folder.path, "Skirt.nif", "Skirt", 6, 0.5f), false) == 0);
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
