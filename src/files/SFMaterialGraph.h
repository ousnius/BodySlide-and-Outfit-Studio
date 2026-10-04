/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "SFLayeredMaterial.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
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
    // The component's whole Data object, for the values beyond file names and links
    nlohmann::json data;
};

struct SFMaterialGraphObject {
    std::string id;
    std::string parent;
    std::vector<SFMaterialComponent> components;
    std::vector<size_t> children;
};

class SFMaterialGraph {
    std::vector<SFMaterialGraphObject> objects;
    std::unordered_map<std::string, size_t> objectIndexes;

    static constexpr const char* RootLayeredMaterial = "materials/layered/root/layeredmaterials.mat";
    static constexpr const char* LayerIDType = "BSMaterial::LayerID";
    static constexpr const char* BlenderIDType = "BSMaterial::BlenderID";
    static constexpr const char* MaterialIDType = "BSMaterial::MaterialID";
    static constexpr const char* TextureSetIDType = "BSMaterial::TextureSetID";
    static constexpr const char* UVStreamIDType = "BSMaterial::UVStreamID";
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

    const SFMaterialGraphObject* FindObject(const std::string& id) const;
    const SFMaterialGraphObject* FindRoot() const;
    const SFMaterialGraphObject* FindLinkedObject(const SFMaterialGraphObject& object, const char* type, size_t index) const;
    // The object's components after those of the objects it derives from in the same file, in the order
    // they apply in: a later one of the same type and index overrides an earlier one.
    std::vector<const SFMaterialComponent*> GetComposedComponents(const SFMaterialGraphObject& object) const;

    void ReadRootComponents(const SFMaterialGraphObject& root, SFLayeredMaterial& material) const;
    void ReadLayer(const SFMaterialGraphObject& layerObject, SFLayer& layer) const;
    void ReadBlender(const SFMaterialGraphObject& blenderObject, SFBlender& blender) const;
    void ReadUVStream(const SFMaterialGraphObject& uvStreamObject, SFUVStream& uvStream) const;

public:
    void Clear();
    bool LoadFromMaterialJson(const nlohmann::json& material);
    void AddObject(SFMaterialGraphObject object);
    void BuildChildLinks();
    bool ResolvePrimaryTextures(std::vector<std::string>& textureFiles, size_t numTextures) const;
    // Reads the layers, blenders and material settings of a layered material. Fails for a material
    // without a root object that links at least its first layer.
    bool ResolveLayeredMaterial(SFLayeredMaterial& material) const;
};
