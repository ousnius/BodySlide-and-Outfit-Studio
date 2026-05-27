/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <string>
#include <vector>

enum class SFMaterialTextureSlot : size_t {
    Color = 0,
    Normal,
    Opacity,
    Roughness,
    Metalness,
    AmbientOcclusion,
    Height,
    Emissive,
    Count
};

struct SFMaterialComponent {
    std::string type;
    size_t index = 0;
    std::string fileName;
    std::string linkedID;
};

struct SFMaterialGraphObject {
    std::string id;
    std::string parent;
    std::vector<SFMaterialComponent> components;
    std::vector<size_t> children;
};

class SFMaterialGraph {
    std::vector<SFMaterialGraphObject> objects;

    static constexpr const char* RootLayeredMaterial = "materials/layered/root/layeredmaterials.mat";
    static constexpr const char* LayerIDType = "BSMaterial::LayerID";
    static constexpr const char* MaterialIDType = "BSMaterial::MaterialID";
    static constexpr const char* TextureSetIDType = "BSMaterial::TextureSetID";
    static constexpr const char* MRTextureFileType = "BSMaterial::MRTextureFile";
    static constexpr const char* TextureFileType = "BSMaterial::TextureFile";

    static std::string ToForwardSlashes(const std::string& path);
    static bool EqualsInsensitive(const std::string& a, const std::string& b);
    static std::string NormalizeTexturePath(const std::string& path);
    static bool IsTextureComponent(const SFMaterialComponent& component);
    static bool IsSlotInRange(size_t slot, size_t numTextures);
    static const SFMaterialComponent* FindIndexedComponent(const SFMaterialGraphObject& object, const char* type, size_t index);
    static std::vector<size_t> GetLayerIndexes(const SFMaterialGraphObject& root);
    static bool HasAnyTexture(const std::vector<std::string>& textureFiles);

public:
    void Clear();
    bool LoadFromMaterialJson(const nlohmann::json& material);
    void AddObject(SFMaterialGraphObject object);
    void BuildChildLinks();
    bool ResolvePrimaryTextures(std::vector<std::string>& textureFiles, size_t numTextures) const;
};
