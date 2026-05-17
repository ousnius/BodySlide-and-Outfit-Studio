/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

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

public:
    void Clear();
    void AddObject(SFMaterialGraphObject object);
    void BuildChildLinks();
    bool ResolvePrimaryTextures(std::vector<std::string>& textureFiles, size_t numTextures) const;
};
