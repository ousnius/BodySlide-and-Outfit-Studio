#include "../src/files/SFResourceHash.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}
}

int main() {
    const SFResourceID defaultId{};
    Require(defaultId.dir == 0 && defaultId.file == 0 && defaultId.ext == 0,
            "Default resource ID should have zero fields");

    Require(SFGetCrc("") == 0, "Empty CRC should be zero");
    Require(SFGetCrc("materials/actors/human") == SFGetCrc("MATERIALS\\ACTORS\\HUMAN"),
            "CRC should normalize case and path separators");
    Require(SFGetCrc("materials/actors/human/naked_body/female") == 0x3114EC57u,
            "Directory CRC should match Starfield reference hash");
    Require(SFGetCrc("naked_f_body") == 0x70F49A6Eu,
            "Filename CRC should match Starfield reference hash");

    const std::string materialPath = "materials/actors/human/naked_body/female/naked_f_body.mat";
    const SFResourceID expected{0x3114EC57u, 0x70F49A6Eu, 0x0074616Du};
    const SFResourceID lowerId = SFGetResourceIdFromPath(materialPath);
    Require(lowerId == expected, "Resource ID should split directory, filename, and extension");

    const SFResourceID variantId = SFGetResourceIdFromPath("Materials\\Actors\\Human\\Naked_Body\\Female\\Naked_F_Body.MAT");
    Require(variantId == expected, "Resource ID should normalize slash and case variants");

    std::unordered_set<SFResourceID, SFResourceIDHash> ids;
    ids.insert(lowerId);
    Require(ids.find(variantId) != ids.end(), "SFResourceIDHash should support unordered lookup");

    return 0;
}
