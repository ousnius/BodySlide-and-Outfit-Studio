/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialFile.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <utility>

namespace {
bool IsTextureSlotInRange(size_t slot) {
    return slot < static_cast<size_t>(SFMaterialTextureSlot::Count);
}

bool BuildMaterialGraph(const nlohmann::json& material, SFMaterialGraph& graph) {
    graph.Clear();

    const auto jsonObjects = material.find("Objects");
    if (jsonObjects == material.end() || !jsonObjects->is_array())
        return false;

    for (const auto& jsonObject : *jsonObjects) {
        SFMaterialGraphObject object;

        const auto id = jsonObject.find("ID");
        if (id != jsonObject.end() && id->is_string())
            object.id = id->get<std::string>();

        const auto parent = jsonObject.find("Parent");
        if (parent != jsonObject.end() && parent->is_string())
            object.parent = parent->get<std::string>();

        const auto components = jsonObject.find("Components");
        if (components != jsonObject.end() && components->is_array()) {
            for (const auto& jsonComponent : *components) {
                SFMaterialComponent component;

                const auto type = jsonComponent.find("Type");
                if (type != jsonComponent.end() && type->is_string())
                    component.type = type->get<std::string>();

                const auto index = jsonComponent.find("Index");
                if (index != jsonComponent.end() && index->is_number_unsigned())
                    component.index = index->get<size_t>();

                const auto data = jsonComponent.find("Data");
                if (data != jsonComponent.end() && data->is_object()) {
                    const auto fileName = data->find("FileName");
                    if (fileName != data->end() && fileName->is_string())
                        component.fileName = fileName->get<std::string>();

                    const auto linkedID = data->find("ID");
                    if (linkedID != data->end() && linkedID->is_string())
                        component.linkedID = linkedID->get<std::string>();
                }

                object.components.push_back(std::move(component));
            }
        }

        graph.AddObject(std::move(object));
    }

    graph.BuildChildLinks();
    return true;
}
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
    if (!BuildMaterialGraph(material, graph)) {
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
