/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialGraph.h"

#include "../utils/StringStuff.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace {
constexpr const char* RootLayeredMaterial = "materials/layered/root/layeredmaterials.mat";
constexpr const char* LayerIDType = "BSMaterial::LayerID";
constexpr const char* MaterialIDType = "BSMaterial::MaterialID";
constexpr const char* TextureSetIDType = "BSMaterial::TextureSetID";
constexpr const char* MRTextureFileType = "BSMaterial::MRTextureFile";
constexpr const char* TextureFileType = "BSMaterial::TextureFile";

std::string ToForwardSlashes(const std::string& path) {
    std::string normalized(path);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool EqualsInsensitive(const std::string& a, const std::string& b) {
    return StringsEqualInsens(a.c_str(), b.c_str());
}

std::string NormalizeTexturePath(const std::string& path) {
    std::string normalized = ToForwardSlashes(path);

    constexpr const char* dataPrefix = "Data/";
    if (normalized.size() >= 5 && StringsEqualNInsens(normalized.c_str(), dataPrefix, 5))
        normalized.erase(0, 5);

    return normalized;
}

bool IsTextureComponent(const SFMaterialComponent& component) {
    return component.type == MRTextureFileType || component.type == TextureFileType;
}

bool IsSlotInRange(size_t slot, size_t numTextures) {
    return slot < numTextures && slot < static_cast<size_t>(SFMaterialTextureSlot::Count);
}

const SFMaterialComponent* FindIndexedComponent(const SFMaterialGraphObject& object, const char* type, size_t index) {
    for (const auto& component : object.components)
        if (component.type == type && component.index == index && !component.linkedID.empty())
            return &component;

    return nullptr;
}

std::vector<size_t> GetLayerIndexes(const SFMaterialGraphObject& root) {
    std::vector<size_t> indexes;

    for (const auto& component : root.components) {
        if (component.type != LayerIDType || component.linkedID.empty())
            continue;

        if (std::find(indexes.begin(), indexes.end(), component.index) == indexes.end())
            indexes.push_back(component.index);
    }

    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

bool HasAnyTexture(const std::vector<std::string>& textureFiles) {
    return std::any_of(textureFiles.begin(), textureFiles.end(), [](const std::string& texture) {
        return !texture.empty();
    });
}
}

void SFMaterialGraph::Clear() {
    objects.clear();
}

void SFMaterialGraph::AddObject(SFMaterialGraphObject object) {
    objects.push_back(std::move(object));
}

void SFMaterialGraph::BuildChildLinks() {
    std::unordered_map<std::string, size_t> objectIndexes;
    for (size_t i = 0; i < objects.size(); ++i) {
        objects[i].children.clear();

        if (!objects[i].id.empty())
            objectIndexes.emplace(objects[i].id, i);
    }

    for (size_t i = 0; i < objects.size(); ++i) {
        const auto parent = objectIndexes.find(objects[i].parent);
        if (parent != objectIndexes.end())
            objects[parent->second].children.push_back(i);
    }
}

bool SFMaterialGraph::ResolvePrimaryTextures(std::vector<std::string>& textureFiles, size_t numTextures) const {
    textureFiles.assign(numTextures, std::string());

    auto collectTextures = [&](const SFMaterialGraphObject& object, std::vector<std::string>& output) {
        for (const auto& component : object.components) {
            if (!IsTextureComponent(component) || component.fileName.empty() || !IsSlotInRange(component.index, numTextures))
                continue;

            if (output[component.index].empty())
                output[component.index] = NormalizeTexturePath(component.fileName);
        }
    };

    std::unordered_map<std::string, size_t> objectIndexes;
    for (size_t i = 0; i < objects.size(); ++i)
        if (!objects[i].id.empty())
            objectIndexes.emplace(objects[i].id, i);

    auto findObject = [&](const std::string& id) -> const SFMaterialGraphObject* {
        const auto object = objectIndexes.find(id);
        if (object == objectIndexes.end())
            return nullptr;

        return &objects[object->second];
    };

    auto findRoot = [&]() -> const SFMaterialGraphObject* {
        for (const auto& object : objects)
            if (EqualsInsensitive(ToForwardSlashes(object.parent), RootLayeredMaterial))
                return &object;

        for (const auto& object : objects)
            if (object.id.empty() && object.parent.empty())
                return &object;

        return nullptr;
    };

    auto resolveLayer = [&](const SFMaterialGraphObject& root, size_t layerIndex, std::vector<std::string>& output) {
        const auto layerComponent = FindIndexedComponent(root, LayerIDType, layerIndex);
        if (!layerComponent)
            return false;

        const auto layer = findObject(layerComponent->linkedID);
        if (!layer)
            return false;

        const auto materialComponent = FindIndexedComponent(*layer, MaterialIDType, 0);
        if (!materialComponent)
            return false;

        const auto material = findObject(materialComponent->linkedID);
        if (!material)
            return false;

        const auto textureSetComponent = FindIndexedComponent(*material, TextureSetIDType, 0);
        if (!textureSetComponent)
            return false;

        const auto textureSet = findObject(textureSetComponent->linkedID);
        if (!textureSet)
            return false;

        collectTextures(*textureSet, output);
        for (size_t childIndex : textureSet->children)
            collectTextures(objects[childIndex], output);

        return HasAnyTexture(output);
    };

    const SFMaterialGraphObject* root = findRoot();
    const bool hasLayerGraph = root && std::any_of(root->components.begin(), root->components.end(), [](const SFMaterialComponent& component) {
        return component.type == LayerIDType && !component.linkedID.empty();
    });

    if (root && hasLayerGraph) {
        if (resolveLayer(*root, 0, textureFiles))
            return true;

        for (size_t layerIndex : GetLayerIndexes(*root)) {
            if (layerIndex == 0)
                continue;

            std::vector<std::string> fallbackTextures(numTextures);
            if (resolveLayer(*root, layerIndex, fallbackTextures)) {
                textureFiles = std::move(fallbackTextures);
                return true;
            }
        }

        return false;
    }

    for (const auto& object : objects)
        collectTextures(object, textureFiles);

    return HasAnyTexture(textureFiles);
}
