/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialFile.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

bool SFMaterialFile::IsTextureSlotInRange(size_t slot) {
    return slot < static_cast<size_t>(SFMaterialTextureSlot::Count);
}

SFMaterialFile::SFMaterialFile(const std::string& fileName) {
    std::ifstream input(fileName);
    if (!input) {
        failed = true;
        return;
    }

    Read(input);
}

SFMaterialFile::SFMaterialFile(std::istream& input) {
    if (!input) {
        failed = true;
        return;
    }

    Read(input);
}

int SFMaterialFile::Read(std::istream& input) {
    textures.fill(std::string());
    failed = false;

    nlohmann::json material = nlohmann::json::parse(input, nullptr, false, true);
    if (material.is_discarded()) {
        failed = true;
        return 1;
    }

    SFMaterialGraph graph;
    if (!graph.LoadFromMaterialJson(material)) {
        failed = true;
        return 1;
    }

    std::vector<std::string> textureFiles;
    if (!graph.ResolvePrimaryTextures(textureFiles, textures.size())) {
        failed = true;
        return 1;
    }

    for (size_t i = 0; i < textures.size(); ++i)
        textures[i] = textureFiles[i];

    return 0;
}

const std::string& SFMaterialFile::GetTexture(SFMaterialTextureSlot slot) const {
    static const std::string empty;

    const size_t index = static_cast<size_t>(slot);
    if (!IsTextureSlotInRange(index))
        return empty;

    return textures[index];
}

std::vector<std::string> SFMaterialFile::GetTextureFiles(size_t numTextures) const {
    std::vector<std::string> textureFiles(numTextures);
    const size_t numSlots = std::min(numTextures, textures.size());

    for (size_t i = 0; i < numSlots; ++i)
        textureFiles[i] = textures[i];

    return textureFiles;
}
