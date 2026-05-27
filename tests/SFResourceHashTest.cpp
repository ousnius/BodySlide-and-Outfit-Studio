#include "../src/files/SFResourceHash.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <unordered_set>

TEST_CASE("Starfield resource hashes normalize paths", "[SFResourceHash]") {
    const SFResourceID defaultId{};
        REQUIRE(defaultId.dir == 0);
        REQUIRE(defaultId.file == 0);
        REQUIRE(defaultId.ext == 0);

        REQUIRE(SFGetCrc("") == 0);
        REQUIRE(SFGetCrc("materials/actors/human") == SFGetCrc("MATERIALS\\ACTORS\\HUMAN"));
        REQUIRE(SFGetCrc("materials/actors/human/naked_body/female") == 0x3114EC57u);
        REQUIRE(SFGetCrc("naked_f_body") == 0x70F49A6Eu);

    const std::string materialPath = "materials/actors/human/naked_body/female/naked_f_body.mat";
    const SFResourceID expected{0x3114EC57u, 0x70F49A6Eu, 0x0074616Du};
    const SFResourceID lowerId = SFGetResourceIdFromPath(materialPath);
        REQUIRE(lowerId == expected);

    const SFResourceID variantId = SFGetResourceIdFromPath("Materials\\Actors\\Human\\Naked_Body\\Female\\Naked_F_Body.MAT");
        REQUIRE(variantId == expected);

    std::unordered_set<SFResourceID, SFResourceIDHash> ids;
    ids.insert(lowerId);
        REQUIRE(ids.find(variantId) != ids.end());
}
