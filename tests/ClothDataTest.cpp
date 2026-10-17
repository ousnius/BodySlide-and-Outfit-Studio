#include "../src/components/ClothData.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdlib>
#include <filesystem>
#include <sstream>

using namespace nifly;

namespace {
std::unique_ptr<BSClothExtraData> Cloth(const std::string& payload) {
    auto block = std::make_unique<BSClothExtraData>();
    block->data.resize(static_cast<uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), block->data.begin());
    return block;
}

NiShape* AddShape(NifFile& nif, const std::string& name) {
    const std::vector<Vector3> vertices{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    const std::vector<Triangle> triangles{{0, 1, 2}};
    const std::vector<Vector2> uvs{{0, 0}, {1, 0}, {0, 1}};
    return nif.CreateShapeFromData(name, &vertices, &triangles, &uvs);
}

NifFile Fixture(const std::string& shape, const std::vector<std::string>& cloth, bool fallout = false) {
    NifFile nif;
    nif.Create(fallout ? NiVersion::getFO4() : NiVersion::getSF());
    REQUIRE(AddShape(nif, shape));
    for (const auto& payload : cloth)
        nif.GetRootNode()->extraDataRefs.AddBlockRef(nif.GetHeader().AddBlock(Cloth(payload)));
    return nif;
}

std::vector<std::string> Payloads(NifFile& nif) {
    std::vector<std::string> payloads;
    for (auto* block : nif.GetChildren<BSClothExtraData>(nullptr, true))
        payloads.emplace_back(block->data.begin(), block->data.end());
    std::sort(payloads.begin(), payloads.end());
    return payloads;
}

std::filesystem::path WorkflowDirectory() {
    const auto* directory = std::getenv("BSOS_CLOTH_DATA_DIR");
    REQUIRE(directory);
    return std::filesystem::u8path(directory);
}
} // namespace

TEST_CASE("Exported shapes retain their own cloth instead of the reference cloth", "[cloth][export]") {
    for (bool fallout : {false, true}) {
        ClothData data;
        auto reference = Fixture("Reference", {"body physics"}, fallout);
        auto outfit = Fixture("Coat", {"coat physics", "badge physics"}, fallout);
        data.Capture(reference, "naked.nif", reference.GetShapes());
        data.Capture(outfit, "coat.nif", outfit.GetShapes());
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

TEST_CASE("Cloth sources survive rename duplication and delete undo", "[cloth][export]") {
    ClothData data;
    auto outfit = Fixture("Coat", {"coat physics"});
    data.Capture(outfit, "coat.nif", outfit.GetShapes());
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
    auto first = Fixture("First", {"first version"});
    auto second = Fixture("Second", {"second version"});
    data.Capture(first, "outfit.nif", first.GetShapes());
    data.Capture(second, "outfit.nif", second.GetShapes());
    data.Apply(first);
    data.Apply(second);
    CHECK(Payloads(first) == std::vector<std::string>{"first version"});
    CHECK(Payloads(second) == std::vector<std::string>{"second version"});
}

TEST_CASE("Explicit HKX additions cannot deselect original cloth", "[cloth][export]") {
    ClothData data;
    auto outfit = Fixture("Coat", {"coat physics"});
    data.Capture(outfit, "coat.nif", outfit.GetShapes());
    data.AddManual("extra.hkx", Cloth("extra physics"));
    data.AddManual("duplicate.hkx", Cloth("coat physics"));
    data.Apply(outfit, data.GetManualSources());
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics", "extra physics"});
    data.Apply(outfit); // Cancel or deselect all optional additions.
    CHECK(Payloads(outfit) == std::vector<std::string>{"coat physics"});
}

TEST_CASE("Cloth ownership survives saving and reopening a combined NIF", "[cloth][export][roundtrip]") {
    const bool fallout = GENERATE(false, true);
    CAPTURE(fallout);
    ClothData data;
    auto reference = Fixture("Reference", {"body physics"}, fallout);
    auto outfit = Fixture("Coat", {"coat physics", "badge physics"}, fallout);
    data.Capture(reference, "naked.nif", reference.GetShapes());
    data.Capture(outfit, "coat.nif", outfit.GetShapes());
    NifFile combined(reference);
    REQUIRE(combined.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    REQUIRE(AddShape(combined, "Clothless"));
    data.Apply(combined);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(combined.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);
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
    SECTION("A clothless shape must not inherit any file-wide cloth") {
        keep = "Clothless";
    }
    SECTION("Load a single reference shape from the combined file") {
        keep = "Reference";
        expected = {"body physics"};
        referenceOnly = true;
    }

    ClothData reopened;
    reopened.Capture(reloaded, "combined.nif", referenceOnly
        ? std::vector<NiShape*>{reloaded.FindBlockByName<NiShape>(keep)} : reloaded.GetShapes());
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
    secondImport.Capture(second, "outfit-only.nif", second.GetShapes());
    secondImport.Apply(second);
    CHECK(Payloads(second) == expected);
}

TEST_CASE("Shared cloth stays with each owner after reopening", "[cloth][export][roundtrip]") {
    ClothData data;
    auto reference = Fixture("Reference", {"body physics", "shared physics"});
    auto outfit = Fixture("Coat", {"coat physics", "shared physics"});
    data.Capture(reference, "naked.nif", reference.GetShapes());
    data.Capture(outfit, "coat.nif", outfit.GetShapes());
    REQUIRE(reference.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    data.Apply(reference);
    CHECK(Payloads(reference) == std::vector<std::string>{"body physics", "coat physics", "shared physics"});
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(reference.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);
    ClothData reopened;
    reopened.Capture(reloaded, "combined.nif", reloaded.GetShapes());
    reloaded.DeleteShape(reloaded.FindBlockByName<NiShape>("Reference"));
    reopened.Apply(reloaded);
    CHECK(Payloads(reloaded) == std::vector<std::string>{"coat physics", "shared physics"});
}

TEST_CASE("Cloth edited outside Outfit Studio survives stale ownership records", "[cloth][export][roundtrip]") {
    ClothData data;
    auto reference = Fixture("Reference", {"body physics"});
    auto outfit = Fixture("Coat", {"coat physics", "badge physics"});
    data.Capture(reference, "naked.nif", reference.GetShapes());
    data.Capture(outfit, "coat.nif", outfit.GetShapes());
    NifFile combined(reference);
    REQUIRE(combined.CloneShape(outfit.GetShapes().front(), "Coat", &outfit));
    REQUIRE(AddShape(combined, "Clothless"));
    data.Apply(combined);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(combined.Save(stream) == 0);
    NifFile reloaded;
    REQUIRE(reloaded.Load(stream) == 0);

    std::vector<std::string> coat;
    std::vector<std::string> body;
    std::vector<std::string> clothless;
    SECTION("A replaced payload follows the shape whose record went stale") {
        for (auto* block : reloaded.GetChildren<BSClothExtraData>(nullptr, true)) {
            if (std::string(block->data.begin(), block->data.end()) == "coat physics") {
                const std::string edited = "coat physics (rebuilt)";
                block->data.resize(static_cast<uint32_t>(edited.size()));
                std::copy(edited.begin(), edited.end(), block->data.begin());
            }
        }
        coat = {"badge physics", "coat physics (rebuilt)"};
        body = {"body physics"};
    }
    SECTION("A payload added without any record belongs to every shape") {
        reloaded.GetRootNode()->extraDataRefs.AddBlockRef(reloaded.GetHeader().AddBlock(Cloth("new physics")));
        coat = {"badge physics", "coat physics", "new physics"};
        body = {"body physics", "new physics"};
        clothless = {"new physics"};
    }

    for (const auto& [keep, expected] : {std::pair{"Coat", coat}, std::pair{"Reference", body}, std::pair{"Clothless", clothless}}) {
        CAPTURE(keep);
        NifFile work(reloaded);
        ClothData reopened;
        reopened.Capture(work, "combined.nif", work.GetShapes());
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
    auto reference = Fixture("Reference", {"body physics"});
    auto outfit = Fixture("Coat", {"coat physics", "badge physics"});
    REQUIRE(reference.Save(directory / "reference.nif") == 0);
    REQUIRE(outfit.Save(directory / "outfit.nif") == 0);
    NifFile skeleton;
    skeleton.Create(NiVersion::getSF());
    skeleton.GetRootNode()->name.get() = "Root";
    REQUIRE(skeleton.Save(directory / "skeleton.nif") == 0);
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
