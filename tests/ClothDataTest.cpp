#include "../src/components/ClothData.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdlib>
#include <filesystem>
#include <sstream>

using namespace nifly;

namespace {
// Bones of the reference skeleton. Like a real Havok payload, every cloth block lists them next to the bones it drives.
const std::vector<std::string> skeleton{"Root", "C_Spine"};

bool Skeleton(const std::string& bone) {
    return std::find(skeleton.begin(), skeleton.end(), bone) != skeleton.end();
}

struct ClothSpec {
    std::string label;
    std::vector<std::string> bones;
};

// A label followed by the skeleton and the driven bones, NUL separated like the string table of a tagfile
std::unique_ptr<BSClothExtraData> Cloth(const std::string& label, const std::vector<std::string>& bones = {}) {
    std::string payload = label;
    for (const auto& bone : skeleton)
        payload += '\0' + bone;
    for (const auto& bone : bones)
        payload += '\0' + bone;
    payload += '\0';

    auto block = std::make_unique<BSClothExtraData>();
    block->data.resize(static_cast<uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), block->data.begin());
    return block;
}

NiShape* AddShape(NifFile& nif, const std::string& name, const std::vector<std::string>& bones = {}) {
    const std::vector<Vector3> vertices{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    const std::vector<Triangle> triangles{{0, 1, 2}};
    const std::vector<Vector2> uvs{{0, 0}, {1, 0}, {0, 1}};
    NiShape* shape = nullptr;
    if (nif.GetHeader().GetVersion().IsSF()) {
        // nifly creates no Starfield geometry from data, so build the BSGeometry the way the game files hold it
        auto block = std::make_unique<BSGeometry>();
        auto geometry = block.get();
        shape = geometry;
        geometry->name.get() = name;
        geometry->SetInternalGeomData(true);
        auto meshSlot = geometry->AddMesh();
        meshSlot->numVerts = 3;
        meshSlot->triSize = 3;
        meshSlot->flags = 64;
        auto& mesh = meshSlot->meshData;
        mesh.version = 2;
        mesh.scale = 1.0f;
        mesh.nVertices = 3;
        mesh.Create(nif.GetHeader().GetVersion(), &vertices, nullptr, nullptr, nullptr);
        mesh.nTriIndices = 3;
        mesh.tris = triangles;
        mesh.nUV1 = 3;
        mesh.uvSets = {uvs};
        mesh.nNormals = 3;
        mesh.normals.assign(3, Vector3(0, 0, 1));
        if (!bones.empty()) {
            mesh.nWeightsPerVert = 4;
            mesh.skinWeights.resize(3, std::vector<BSGeometryMeshData::BoneWeight>(4));
            for (uint16_t v = 0; v < 3; ++v)
                mesh.skinWeights[v][0] = {static_cast<uint16_t>(v % bones.size()), 65535};
        }
        nif.GetRootNode()->childRefs.AddBlockRef(nif.GetHeader().AddBlock(std::move(block)));
    }
    else {
        shape = nif.CreateShapeFromData(name, &vertices, &triangles, &uvs);
        // Earlier games bind bones by NiNode block, Starfield by the names in SkinAttach
        for (const auto& bone : bones) {
            if (!nif.FindBlockByName<NiNode>(bone))
                nif.AddNode(bone, MatTransform());
        }
    }

    if (shape && !bones.empty()) {
        nif.CreateSkinning(shape);
        nif.SetShapeBoneList(shape, bones);
    }
    return shape;
}

NifFile Fixture(const std::string& shape, const std::vector<std::string>& bones, const std::vector<ClothSpec>& cloth, bool fallout = false) {
    NifFile nif;
    nif.Create(fallout ? NiVersion::getFO4() : NiVersion::getSF());
    REQUIRE(AddShape(nif, shape, bones));
    for (const auto& spec : cloth)
        nif.GetRootNode()->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(Cloth(spec.label, spec.bones)));
    return nif;
}

std::string Label(const BSClothExtraData& block) {
    const std::string payload(block.data.data(), block.data.size());
    return payload.substr(0, payload.find('\0'));
}

std::vector<std::string> Payloads(NifFile& nif) {
    std::vector<std::string> labels;
    for (auto* block : nif.GetChildren<BSClothExtraData>(nullptr, true))
        labels.push_back(Label(*block));
    std::sort(labels.begin(), labels.end());
    return labels;
}

std::vector<std::string> Bones(NifFile& nif, const std::string& shape) {
    std::vector<std::string> bones;
    nif.GetShapeBoneList(nif.FindBlockByName<NiShape>(shape), bones);
    return bones;
}

std::filesystem::path WorkflowDirectory() {
    const auto* directory = std::getenv("BSOS_CLOTH_DATA_DIR");
    REQUIRE(directory);
    return std::filesystem::u8path(directory);
}

const std::vector<std::string> bodyBones{"C_Spine", "Cloth_Body"};
const std::vector<std::string> coatBones{"C_Spine", "Cloth_Bone_001", "Cloth_Bone_Badge"};
const std::vector<ClothSpec> bodyCloth{{"body physics", {"Cloth_Body"}}};
const std::vector<ClothSpec> coatCloth{{"coat physics", {"Cloth_Bone_001"}}, {"badge physics", {"Cloth_Bone_Badge"}}};
} // namespace

TEST_CASE("Exported shapes retain their own cloth instead of the reference cloth", "[cloth][export]") {
    for (bool fallout : {false, true}) {
        CAPTURE(fallout);
        ClothData data;
        auto reference = Fixture("Reference", bodyBones, bodyCloth, fallout);
        auto outfit = Fixture("Coat", coatBones, coatCloth, fallout);
        data.Capture(reference, "naked.nif", reference.GetShapes(), Skeleton);
        data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
        REQUIRE(Payloads(outfit).empty());
        REQUIRE(Payloads(reference).empty());
        NifFile work(reference);
        REQUIRE(work.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));

        SECTION("Export only the outfit") {
            work.DeleteShape(work.FindBlockByName<NiShape>("Reference"));
            data.Apply(work);
            CHECK(Payloads(work) == std::vector<std::string>{"badge physics", "coat physics"});
        }
        SECTION("Include the reference") {
            data.Apply(work);
            CHECK(Payloads(work) == std::vector<std::string>{"badge physics", "body physics", "coat physics"});
        }
        SECTION("Export only the reference") {
            work.DeleteShape(work.FindBlockByName<NiShape>("Coat"));
            data.Apply(work);
            CHECK(Payloads(work) == std::vector<std::string>{"body physics"});
        }
        SECTION("Delete the outfit and reuse its name for clothless geometry") {
            data.RemoveShape("Coat");
            work.DeleteShape(work.FindBlockByName<NiShape>("Coat"));
            REQUIRE(AddShape(work, "Coat"));
            work.DeleteShape(work.FindBlockByName<NiShape>("Reference"));
            data.Apply(work);
            CHECK(Payloads(work).empty());
        }
    }
}

TEST_CASE("Cloth follows the bones it drives rather than the shared skeleton", "[cloth][export]") {
    const bool fallout = GENERATE(false, true);
    CAPTURE(fallout);
    NifFile combined;
    combined.Create(fallout ? NiVersion::getFO4() : NiVersion::getSF());
    REQUIRE(AddShape(combined, "Reference", bodyBones));
    REQUIRE(AddShape(combined, "Coat", coatBones));
    REQUIRE(AddShape(combined, "Clothless", {"C_Spine"}));
    REQUIRE(AddShape(combined, "Unskinned"));
    for (const auto& spec : {bodyCloth.front(), coatCloth.front(), coatCloth.back()})
        combined.GetRootNode()->extraDataRefs.AddBlockRef(combined.GetHeader().AddBlock(Cloth(spec.label, spec.bones)));

    SECTION("Every shape keeps the cloth of its own bones") {
        ClothData data;
        data.Capture(combined, "combined.nif", combined.GetShapes(), Skeleton);
        CHECK(data.GetShapeSources("Reference") == ClothData::Sources{"combined.nif"});
        CHECK(data.GetShapeSources("Coat") == ClothData::Sources{"combined.nif (2)", "combined.nif (3)"});
        CHECK(data.GetShapeSources("Clothless").empty());
        CHECK(data.GetShapeSources("Unskinned").empty());
    }
    SECTION("Cloth that drives only the skeleton belongs to every shape") {
        combined.GetRootNode()->extraDataRefs.AddBlockRef(combined.GetHeader().AddBlock(Cloth("skeleton physics")));
        ClothData data;
        data.Capture(combined, "combined.nif", combined.GetShapes(), Skeleton);
        for (const auto& shape : {"Reference", "Coat", "Clothless", "Unskinned"}) {
            CAPTURE(shape);
            const auto sources = data.GetShapeSources(shape);
            CHECK(std::find(sources.begin(), sources.end(), "combined.nif (4)") != sources.end());
        }
        CHECK(data.GetShapeSources("Clothless") == ClothData::Sources{"combined.nif (4)"});
    }
    SECTION("Node and HKX spellings of a bone can differ in case") {
        REQUIRE(AddShape(combined, "Lowercase", {"C_Spine", "cloth_bone_badge"}));
        ClothData data;
        data.Capture(combined, "combined.nif", combined.GetShapes(), Skeleton);
        CHECK(data.GetShapeSources("Lowercase") == ClothData::Sources{"combined.nif (3)"});
    }
    SECTION("Without a reference skeleton the shared bones hand all cloth to every skinned shape") {
        ClothData data;
        data.Capture(combined, "combined.nif", combined.GetShapes());
        CHECK(data.GetShapeSources("Reference").size() == 3);
        CHECK(data.GetShapeSources("Coat").size() == 3);
        CHECK(data.GetShapeSources("Clothless").size() == 3);
        CHECK(data.GetShapeSources("Unskinned").empty());
    }
    SECTION("A body bone the cloth rig names but the reference skeleton lacks also claims the cloth") {
        // Known limitation: names alone cannot tell a driven bone from one the cloth only collides with
        REQUIRE(AddShape(combined, "Physics Body", {"C_Spine", "L_BreastSkin"}));
        combined.GetRootNode()->extraDataRefs.AddBlockRef(
            combined.GetHeader().AddBlock(Cloth("physics coat", {"L_BreastSkin", "Cloth_Bone_Physics"})));
        REQUIRE(AddShape(combined, "Physics Coat", {"C_Spine", "L_BreastSkin", "Cloth_Bone_Physics"}));
        ClothData data;
        data.Capture(combined, "combined.nif", combined.GetShapes(), Skeleton);
        CHECK(data.GetShapeSources("Physics Coat") == ClothData::Sources{"combined.nif (4)"});
        CHECK(data.GetShapeSources("Physics Body") == ClothData::Sources{"combined.nif (4)"});
    }
}

TEST_CASE("Cloth sources survive rename duplication and delete undo", "[cloth][export]") {
    ClothData data;
    auto outfit = Fixture("Coat", coatBones, {coatCloth.front()});
    data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
    const auto sources = data.GetShapeSources("Coat");
    data.RenameShape("Coat", "Renamed");
    REQUIRE(NifFile::RenameShape(outfit.GetShapes().front(), "Renamed"));
    REQUIRE(outfit.CloneShape(outfit.GetShapes().front(), "Duplicate"));
    data.SetShapeSources("Duplicate", data.GetShapeSources("Renamed"));
    data.Apply(outfit);
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics"});
    data.RemoveShape("Renamed");
    outfit.DeleteShape(outfit.FindBlockByName<NiShape>("Renamed"));
    data.Apply(outfit);
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics"});
    data.RemoveShape("Duplicate");
    data.Apply(outfit);
    CHECK(Payloads(outfit).empty());
    data.SetShapeSources("Duplicate", sources); // Undo restores the snapshot.
    data.Apply(outfit);
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics"});
}

TEST_CASE("Reimporting the same path never replaces an earlier shape's cloth", "[cloth][export]") {
    ClothData data;
    auto first = Fixture("First", coatBones, {{"first version", {"Cloth_Bone_001"}}});
    auto second = Fixture("Second", coatBones, {{"second version", {"Cloth_Bone_001"}}});
    data.Capture(first, "outfit.nif", first.GetShapes(), Skeleton);
    data.Capture(second, "outfit.nif", second.GetShapes(), Skeleton);
    data.Apply(first);
    data.Apply(second);
    CHECK(Payloads(first) == std::vector<std::string>{"first version"});
    CHECK(Payloads(second) == std::vector<std::string>{"second version"});
}

TEST_CASE("Explicit HKX additions cannot deselect original cloth", "[cloth][export]") {
    ClothData data;
    auto outfit = Fixture("Coat", coatBones, {coatCloth.front()});
    data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
    data.AddManual("extra.hkx", Cloth("extra physics"));
    data.AddManual("duplicate.hkx", Cloth(coatCloth.front().label, coatCloth.front().bones));
    data.Apply(outfit, data.GetManualSources());
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics", "extra physics"});
    data.Apply(outfit); // Cancel or deselect all optional additions.
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics"});
}

TEST_CASE("Cloth ownership survives saving and reopening a combined NIF", "[cloth][export][roundtrip]") {
    const bool fallout = GENERATE(false, true);
    CAPTURE(fallout);
    ClothData data;
    auto reference = Fixture("Reference", bodyBones, bodyCloth, fallout);
    auto outfit = Fixture("Coat", coatBones, coatCloth, fallout);
    data.Capture(reference, "naked.nif", reference.GetShapes(), Skeleton);
    data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
    NifFile combined(reference);
    REQUIRE(combined.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    REQUIRE(AddShape(combined, "Clothless", {"C_Spine"}));
    data.Apply(combined);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(combined.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);
    REQUIRE(Bones(reloaded, "Coat") == coatBones);
    // Import can rename colliding shapes; NIF tools can reorder root blocks.
    REQUIRE(NifFile::RenameShape(reloaded.FindBlockByName<NiShape>("Coat"), "Renamed"));
    auto& refs = reloaded.GetRootNode()->extraDataRefs;
    std::reverse(refs.begin(), refs.end());

    std::string keep;
    std::vector<std::string> expected;
    bool referenceOnly = false;
    SECTION("Export only the outfit") {
        keep = "Renamed";
        expected = {"badge physics", "coat physics"};
    }
    SECTION("Export only the body") {
        keep = "Reference";
        expected = {"body physics"};
    }
    SECTION("A shape skinned to the skeleton alone must not inherit any cloth") {
        keep = "Clothless";
    }
    SECTION("Load a single reference shape from the combined file") {
        keep = "Reference";
        expected = {"body physics"};
        referenceOnly = true;
    }

    ClothData reopened;
    reopened.Capture(reloaded, "combined.nif", referenceOnly
        ? std::vector<NiShape*>{reloaded.FindBlockByName<NiShape>(keep)} : reloaded.GetShapes(), Skeleton);
    for (auto* shape : reloaded.GetShapes()) {
        if (shape->name.get() != keep)
            reloaded.DeleteShape(shape);
    }
    reopened.Apply(reloaded);
    CHECK(Payloads(reloaded) == expected);

    // Another save/reopen must retain the narrowed ownership as well.
    std::stringstream secondStream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(reloaded.Save(secondStream) == 0);
    NifFile second;
    REQUIRE(second.Load(secondStream) == 0);
    ClothData secondImport;
    secondImport.Capture(second, "outfit-only.nif", second.GetShapes(), Skeleton);
    secondImport.Apply(second);
    CHECK(Payloads(second) == expected);
}

TEST_CASE("Shared cloth stays with each owner after reopening", "[cloth][export][roundtrip]") {
    const ClothSpec shared{"shared physics", {"Cloth_Body", "Cloth_Bone_001"}};
    ClothData data;
    auto reference = Fixture("Reference", bodyBones, {bodyCloth.front(), shared});
    auto outfit = Fixture("Coat", coatBones, {coatCloth.front(), shared});
    data.Capture(reference, "naked.nif", reference.GetShapes(), Skeleton);
    data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
    REQUIRE(reference.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    data.Apply(reference);
    CHECK(Payloads(reference) == std::vector<std::string>{"body physics", "coat physics", "shared physics"});
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(reference.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);
    ClothData reopened;
    reopened.Capture(reloaded, "combined.nif", reloaded.GetShapes(), Skeleton);
    reloaded.DeleteShape(reloaded.FindBlockByName<NiShape>("Reference"));
    reopened.Apply(reloaded);
    CHECK(Payloads(reloaded) == std::vector<std::string>{"coat physics", "shared physics"});
}

TEST_CASE("Cloth edited outside Outfit Studio keeps following its bones", "[cloth][export][roundtrip]") {
    ClothData data;
    auto reference = Fixture("Reference", bodyBones, bodyCloth);
    auto outfit = Fixture("Coat", coatBones, coatCloth);
    data.Capture(reference, "naked.nif", reference.GetShapes(), Skeleton);
    data.Capture(outfit, "coat.nif", outfit.GetShapes(), Skeleton);
    NifFile combined(reference);
    REQUIRE(combined.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    REQUIRE(AddShape(combined, "Clothless", {"C_Spine"}));
    data.Apply(combined);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(combined.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);

    std::vector<std::string> coat;
    std::vector<std::string> body;
    std::vector<std::string> clothless;
    SECTION("A rebuilt payload follows the shape skinned to its bones") {
        for (auto* block : reloaded.GetChildren<BSClothExtraData>(nullptr, true)) {
            if (Label(*block) == "coat physics")
                block->data = Cloth("coat physics (rebuilt)", {"Cloth_Bone_001"})->data;
        }
        coat = {"badge physics", "coat physics (rebuilt)"};
        body = {"body physics"};
    }
    SECTION("A payload that drives no outfit bone belongs to every shape") {
        reloaded.GetRootNode()->extraDataRefs.AddBlockRef(reloaded.GetHeader().AddBlock(Cloth("new physics")));
        coat = {"badge physics", "coat physics", "new physics"};
        body = {"body physics", "new physics"};
        clothless = {"new physics"};
    }

    for (const auto& [keep, expected] : {std::pair{"Coat", coat}, std::pair{"Reference", body}, std::pair{"Clothless", clothless}}) {
        CAPTURE(keep);
        NifFile work(reloaded);
        ClothData reopened;
        reopened.Capture(work, "combined.nif", work.GetShapes(), Skeleton);
        for (auto* shape : work.GetShapes()) {
            if (shape->name.get() != keep)
                work.DeleteShape(shape);
        }
        reopened.Apply(work);
        CHECK(Payloads(work) == expected);
    }
}

TEST_CASE("Create cloth preservation workflow fixtures", "[.cloth-data-setup]") {
    auto directory = WorkflowDirectory();
    std::filesystem::create_directories(directory);
    auto reference = Fixture("Reference", bodyBones, bodyCloth);
    auto outfit = Fixture("Coat", coatBones, coatCloth);
    REQUIRE(reference.Save(directory / "reference.nif") == 0);
    REQUIRE(outfit.Save(directory / "outfit.nif") == 0);
    NifFile rig;
    rig.Create(NiVersion::getSF());
    rig.GetRootNode()->name.get() = "Root";
    REQUIRE(rig.AddNode("C_Spine", MatTransform()));
    REQUIRE(rig.Save(directory / "skeleton.nif") == 0);
}

TEST_CASE("Verify actual Outfit Studio cloth preservation exports", "[.cloth-data-verify]") {
    auto directory = WorkflowDirectory();
    for (const auto& name : {"outfit-only.nif", "edited.nif", "reopened.nif", "ShapeData/Cloth/Cloth.nif",
             "combined-reopened-outfit.nif", "project-reopened-outfit.nif"}) {
        INFO(name);
        NifFile nif;
        REQUIRE(nif.Load(directory / name) == 0);
        CHECK(nif.GetShapes().size() == 1);
        CHECK(Payloads(nif) == std::vector<std::string>{"badge physics", "coat physics"});
    }
    NifFile both;
    REQUIRE(both.Load(directory / "with-reference.nif") == 0);
    CHECK(both.GetShapes().size() == 2);
    CHECK(Payloads(both) == std::vector<std::string>{"badge physics", "body physics", "coat physics"});
    NifFile savedProject;
    REQUIRE(savedProject.Load(directory / "ShapeData/Combined/Combined.nif") == 0);
    CHECK(savedProject.GetShapes().size() == 2);
    CHECK(Payloads(savedProject) == std::vector<std::string>{"badge physics", "body physics", "coat physics"});
    NifFile merged;
    REQUIRE(merged.Load(directory / "merged.nif") == 0);
    REQUIRE(merged.GetShapes().size() == 1);
    CHECK(merged.GetShapes().front()->GetNumVertices() == 6);
    CHECK(Payloads(merged) == std::vector<std::string>{"badge physics", "body physics", "coat physics"});
}
