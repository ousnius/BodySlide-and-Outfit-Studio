#include "../src/files/SFMorphFile.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <string>
#include <unordered_map>

using nifly::Vector3;

namespace {

// Position deltas round-trip through an fp16 quantization at a 1/69.969 scale,
// so reloaded values are close but not bit-exact.
constexpr float HalfTolerance = 0.02f;

using OffsetMap = std::unordered_map<uint16_t, Vector3>;

// Writes to a uniquely named temp file so test cases never collide, even when
// ctest runs them as separate processes in parallel.
struct TempMorphFile {
    std::string path;

    explicit TempMorphFile(const char* name) {
        path = (std::filesystem::temp_directory_path() / name).string();
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    ~TempMorphFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

OffsetMap GetMorphOffsets(SFMorphFile& file, const std::string& morphName) {
    auto it = file.morphNamesCacheMap.find(morphName);
    REQUIRE(it != file.morphNamesCacheMap.end());
    REQUIRE(it->second < file.morphOffsetsCache.size());
    return file.morphOffsetsCache[it->second];
}

void RequireOffsetsMatch(const OffsetMap& actual, const OffsetMap& expected) {
    // Exact same vertex set: a mismatch here means morph data was attributed to
    // the wrong morph.
    REQUIRE(actual.size() == expected.size());

    for (const auto& [index, expectedDelta] : expected) {
        INFO("vertex " << index);
        auto it = actual.find(index);
        REQUIRE(it != actual.end());

        REQUIRE(std::fabs(it->second.x - expectedDelta.x) < HalfTolerance);
        REQUIRE(std::fabs(it->second.y - expectedDelta.y) < HalfTolerance);
        REQUIRE(std::fabs(it->second.z - expectedDelta.z) < HalfTolerance);
    }
}

} // namespace

TEST_CASE("Starfield morph file round-trips overlapping morphs", "[SFMorphFile]") {
    // Three morphs with overlapping vertex sets. Overlap is the regression target:
    // the key marker bits must identify each morph by its name table index, so a
    // vertex affected by multiple morphs must get each morph's own deltas back.
    // Vertex 3 is touched by all three; verts 4/5/6 are touched by morphs whose
    // index is not their position in the vertex's contributor list, which is
    // exactly what a per-vertex sequence counter mislabeled.
    OffsetMap morphA{
        {0, Vector3(1.0f, 0.0f, 0.0f)},
        {1, Vector3(0.0f, 2.0f, 0.0f)},
        {2, Vector3(0.0f, 0.0f, 3.0f)},
        {3, Vector3(1.0f, 1.0f, 1.0f)},
    };
    OffsetMap morphB{
        {2, Vector3(-1.0f, 0.0f, 0.0f)},
        {3, Vector3(0.0f, -2.0f, 0.0f)},
        {4, Vector3(0.0f, 0.0f, -3.0f)},
        {5, Vector3(-1.0f, -1.0f, -1.0f)},
    };
    OffsetMap morphC{
        {3, Vector3(4.0f, 0.0f, 0.0f)},
        {5, Vector3(0.0f, 5.0f, 0.0f)},
        {6, Vector3(0.0f, 0.0f, 6.0f)},
    };

    SFMorphFile outFile;
    outFile.SetVertexCount(8);
    REQUIRE(outFile.AddMorph("MorphA", morphA, {}, {}, {}));
    REQUIRE(outFile.AddMorph("MorphB", morphB, {}, {}, {}));
    REQUIRE(outFile.AddMorph("MorphC", morphC, {}, {}, {}));

    outFile.CacheToFileData();

    TempMorphFile temp("bsos_sfmorph_overlap.dat");
    REQUIRE(outFile.Write(temp.path));

    // Same sequence the Outfit Studio import path uses.
    SFMorphFile inFile;
    REQUIRE(inFile.Read(temp.path));
    REQUIRE(inFile.FileToCacheData());
    inFile.UpdateCachedMorphData();

    const auto names = inFile.GetMorphNames();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "MorphA");
    REQUIRE(names[1] == "MorphB");
    REQUIRE(names[2] == "MorphC");

    RequireOffsetsMatch(GetMorphOffsets(inFile, "MorphA"), morphA);
    RequireOffsetsMatch(GetMorphOffsets(inFile, "MorphB"), morphB);
    RequireOffsetsMatch(GetMorphOffsets(inFile, "MorphC"), morphC);
}

TEST_CASE("Starfield morph file round-trips a single morph", "[SFMorphFile]") {
    OffsetMap morph{
        {0, Vector3(0.5f, 0.0f, 0.0f)},
        {7, Vector3(0.0f, 0.0f, -0.5f)},
    };

    SFMorphFile outFile;
    outFile.SetVertexCount(8);
    REQUIRE(outFile.AddMorph("Morph", morph, {}, {}, {}));

    outFile.CacheToFileData();

    TempMorphFile temp("bsos_sfmorph_single.dat");
    REQUIRE(outFile.Write(temp.path));

    SFMorphFile inFile;
    REQUIRE(inFile.Read(temp.path));
    REQUIRE(inFile.FileToCacheData());
    inFile.UpdateCachedMorphData();

    REQUIRE(inFile.GetMorphCount() == 1);
    RequireOffsetsMatch(GetMorphOffsets(inFile, "Morph"), morph);
}

TEST_CASE("Starfield morph file caps shape keys at the 128-bit keyMarker width", "[SFMorphFile]") {
    // The per-vertex keyMarker is a fixed 128-bit field, so only shape keys 0-127
    // can be referenced. Exporting more must not write a bit past the 4-word
    // marker (an out-of-bounds write); the extra morphs are simply unaddressable.
    constexpr uint32_t morphCount = SFMaxShapeKeys + 2; // 130

    SFMorphFile outFile;
    outFile.SetVertexCount(morphCount);
    for (uint32_t i = 0; i < morphCount; i++) {
        const std::string name = "Morph" + std::to_string(i);
        REQUIRE(outFile.AddMorph(name, OffsetMap{{static_cast<uint16_t>(i), Vector3(1.0f, 0.0f, 0.0f)}}, {}, {}, {}));
    }

    outFile.CacheToFileData();

    TempMorphFile temp("bsos_sfmorph_limit.dat");
    REQUIRE(outFile.Write(temp.path));

    SFMorphFile inFile;
    REQUIRE(inFile.Read(temp.path));
    REQUIRE(inFile.FileToCacheData());
    inFile.UpdateCachedMorphData();

    for (uint32_t i = 0; i < SFMaxShapeKeys; i++) {
        const std::string name = "Morph" + std::to_string(i);
        INFO("addressable morph " << name);
        REQUIRE(inFile.morphNamesCacheMap.count(name) == 1);

        const uint32_t idx = inFile.morphNamesCacheMap.at(name);
        REQUIRE(inFile.morphOffsetsCache[idx].count(static_cast<uint16_t>(i)) == 1);
    }

    for (uint32_t i = SFMaxShapeKeys; i < morphCount; i++) {
        const std::string name = "Morph" + std::to_string(i);
        INFO("unaddressable morph " << name);
        // Index >= 128 cannot be encoded, so it carries no data after round-trip.
        REQUIRE(inFile.morphNamesCacheMap.count(name) == 0);
    }
}
