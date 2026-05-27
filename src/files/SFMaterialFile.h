/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SFMaterialGraph.h"

#include <array>
#include <cstddef>
#include <istream>
#include <string>
#include <vector>

class SFMaterialFile {
    bool failed = false;
    std::array<std::string, static_cast<size_t>(SFMaterialTextureSlot::Count)> textures;

    static bool IsTextureSlotInRange(size_t slot);

public:
    SFMaterialFile() = default;
    explicit SFMaterialFile(const std::string& fileName);
    explicit SFMaterialFile(std::istream& input);

    int Read(std::istream& input);

    bool Failed() const { return failed; }
    const std::string& GetTexture(SFMaterialTextureSlot slot) const;
    std::vector<std::string> GetTextureFiles(size_t numTextures) const;
};
