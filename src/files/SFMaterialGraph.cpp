/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFMaterialGraph.h"

#include "../utils/StringStuff.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace {
// Values in material JSON are strings ("0.5", "true", "Skin") in both loose files and what the CDB
// reader produces, but a hand edited file may well use JSON numbers and booleans.
bool ReadFloat(const nlohmann::json* value, float& out) {
    if (!value)
        return false;

    if (value->is_number()) {
        out = value->get<float>();
        return true;
    }

    if (value->is_string()) {
        const std::string& str = value->get_ref<const std::string&>();
        char* end = nullptr;
        const float parsed = std::strtof(str.c_str(), &end);
        if (end == str.c_str())
            return false;

        out = parsed;
        return true;
    }

    return false;
}

bool ReadBool(const nlohmann::json* value, bool& out) {
    if (!value)
        return false;

    if (value->is_boolean()) {
        out = value->get<bool>();
        return true;
    }

    if (value->is_string()) {
        const std::string& str = value->get_ref<const std::string&>();
        if (StringsEqualInsens(str.c_str(), "true")) {
            out = true;
            return true;
        }

        if (StringsEqualInsens(str.c_str(), "false")) {
            out = false;
            return true;
        }
    }

    return false;
}

bool ReadString(const nlohmann::json* value, std::string& out) {
    if (!value || !value->is_string())
        return false;

    out = value->get<std::string>();
    return true;
}

// Index of a string in a list of names, for the enumerations the game stores by name
template<typename T, size_t N>
bool ReadEnum(const nlohmann::json* value, const char* const (&names)[N], T& out) {
    std::string str;
    if (!ReadString(value, str))
        return false;

    for (size_t i = 0; i < N; ++i) {
        if (StringsEqualInsens(str.c_str(), names[i])) {
            out = static_cast<T>(i);
            return true;
        }
    }

    return false;
}

// "MATERIAL_LAYER_n" and "BLEND_LAYER_n"
bool ReadIndexedName(const nlohmann::json* value, const char* prefix, size_t count, uint8_t& out) {
    std::string str;
    if (!ReadString(value, str))
        return false;

    const size_t prefixLength = std::strlen(prefix);
    if (str.size() != prefixLength + 1 || !StringsEqualNInsens(str.c_str(), prefix, static_cast<int>(prefixLength)))
        return false;

    const int index = str[prefixLength] - '0';
    if (index < 0 || static_cast<size_t>(index) >= count)
        return false;

    out = static_cast<uint8_t>(index);
    return true;
}

bool ReadLayerName(const nlohmann::json* value, uint8_t& out) {
    return ReadIndexedName(value, "MATERIAL_LAYER_", SFLayeredMaterial::MaxLayers, out);
}

const nlohmann::json* Field(const nlohmann::json* object, const char* name) {
    if (!object || !object->is_object())
        return nullptr;

    const auto it = object->find(name);
    if (it == object->end())
        return nullptr;

    return &*it;
}

// Nested structures come wrapped as { "Data": {...}, "Type": "..." }
const nlohmann::json* Unwrap(const nlohmann::json* value) {
    while (value && value->is_object()) {
        const nlohmann::json* data = Field(value, "Data");
        if (!data)
            break;

        value = data;
    }

    return value;
}

const nlohmann::json* StructField(const nlohmann::json* object, const char* name) {
    return Unwrap(Field(object, name));
}

// An XMFLOAT2/3/4 either as is or inside the "Value" of a BSMaterial::Color or a Scale and Offset
template<size_t N>
bool ReadVector(const nlohmann::json* value, float (&out)[N]) {
    value = Unwrap(value);
    if (const nlohmann::json* inner = StructField(value, "Value"))
        value = inner;

    if (!value || !value->is_object())
        return false;

    static constexpr const char* names[4] = {"x", "y", "z", "w"};
    float result[N];
    for (size_t i = 0; i < N; ++i)
        if (!ReadFloat(Field(value, names[i]), result[i]))
            return false;

    for (size_t i = 0; i < N; ++i)
        out[i] = std::clamp(result[i], -1.0e6f, 1.0e6f);

    return true;
}

bool ReadColor(const nlohmann::json* value, float (&out)[4]) {
    if (!ReadVector(value, out))
        return false;

    for (float& c : out)
        c = std::clamp(c, 0.0f, 1.0f);

    return true;
}

constexpr const char* const blendModeNames[] = {"Linear", "Additive", "PositionContrast", "None", "CharacterCombine", "Skin"};
constexpr const char* const colorChannelNames[] = {"Red", "Green", "Blue", "Alpha"};
constexpr const char* const addressModeNames[] = {"Wrap", "Clamp", "Mirror", "Border"};
constexpr const char* const channelNames[] = {"Zero", "One", "Two", "Three"};
constexpr const char* const routeNames[] = {"Deferred", "Effect", "PlanetaryRing", "PrecomputedScattering", "Water"};
constexpr const char* const opacityBlendModeNames[] = {"Lerp", "Additive", "Subtractive", "Multiplicative"};
constexpr const char* const maskBlenderNames[] = {"None", "Blender1", "Blender2", "Blender3"};
constexpr const char* const effectBlendModeNames[] = {"AlphaBlend", "Additive", "SourceSoftAdditive", "Multiply", "DestinationSoftAdditive", "DestinationInvertedSoftAdditive", "TakeSmaller", "None"};
constexpr const char* const overrideColorNames[] = {"Multiply", "Lerp"};

// Shader models that render both faces regardless of the material's own two sided setting, the
// ones NifSkope marks: TranslucentTwoSided1Layer, TwoSided1Layer, VegetationTranslucent1/2Layer and
// both water models
bool IsTwoSidedShaderModel(const std::string& shaderModel) {
    static constexpr const char* const twoSidedModels[] = {
        "TranslucentTwoSided1Layer", "TwoSided1Layer", "VegetationTranslucent1Layer", "VegetationTranslucent2Layer", "Water", "Water1Layer"};

    for (const char* model : twoSidedModels)
        if (shaderModel == model)
            return true;

    return false;
}

void ReadEmissiveLayer(const nlohmann::json* data, const char* layerField, const char* tintField, const char* maskField, SFEmissiveLayer& layer) {
    ReadLayerName(Field(data, layerField), layer.layer);
    ReadColor(Field(data, tintField), layer.tint);
    ReadEnum(Field(data, maskField), maskBlenderNames, layer.maskBlender);
}

void ReadEmittance(const nlohmann::json* data, SFEmittance& emittance) {
    ReadBool(Field(data, "AdaptiveEmittance"), emittance.adaptive);
    ReadBool(Field(data, "EnableAdaptiveLimits"), emittance.adaptiveLimits);
    ReadFloat(Field(data, "LuminousEmittance"), emittance.luminousEmittance);
    ReadFloat(Field(data, "ExposureOffset"), emittance.exposureOffset);
    ReadFloat(Field(data, "MaxOffsetEmittance"), emittance.maxOffset);
    ReadFloat(Field(data, "MinOffsetEmittance"), emittance.minOffset);
}
}

std::string SFMaterialGraph::ToForwardSlashes(const std::string& path) {
    std::string normalized(path);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool SFMaterialGraph::EqualsInsensitive(const std::string& a, const std::string& b) {
    return StringsEqualInsens(a.c_str(), b.c_str());
}

std::string SFMaterialGraph::NormalizeTexturePath(const std::string& path) {
    std::string normalized = ToForwardSlashes(path);

    constexpr const char* dataPrefix = "Data/";
    if (normalized.size() >= 5 && StringsEqualNInsens(normalized.c_str(), dataPrefix, 5))
        normalized.erase(0, 5);

    return normalized;
}

bool SFMaterialGraph::IsTextureComponent(const SFMaterialComponent& component) {
    return component.type == MRTextureFileType || component.type == TextureFileType;
}

bool SFMaterialGraph::IsSlotInRange(size_t slot, size_t numTextures) {
    return slot < numTextures && slot < static_cast<size_t>(SFMaterialTextureSlot::Count);
}

const SFMaterialComponent* SFMaterialGraph::FindIndexedComponent(const SFMaterialGraphObject& object, const char* type, size_t index) {
    for (const auto& component : object.components)
        if (component.type == type && component.index == index && !component.linkedID.empty())
            return &component;

    return nullptr;
}

std::vector<size_t> SFMaterialGraph::GetLayerIndexes(const SFMaterialGraphObject& root) {
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

bool SFMaterialGraph::HasAnyTexture(const std::vector<std::string>& textureFiles) {
    return std::any_of(textureFiles.begin(), textureFiles.end(), [](const std::string& texture) {
        return !texture.empty();
    });
}

const SFMaterialGraphObject* SFMaterialGraph::FindObject(const std::string& id) const {
    const auto object = objectIndexes.find(id);
    if (object == objectIndexes.end())
        return nullptr;

    return &objects[object->second];
}

const SFMaterialGraphObject* SFMaterialGraph::FindRoot() const {
    // The material itself has no ID of its own, unlike the LOD materials the CDB reader lists after it
    // that derive from the same root template
    for (const auto& object : objects)
        if (object.id.empty() && EqualsInsensitive(ToForwardSlashes(object.parent), RootLayeredMaterial))
            return &object;

    for (const auto& object : objects)
        if (EqualsInsensitive(ToForwardSlashes(object.parent), RootLayeredMaterial))
            return &object;

    for (const auto& object : objects)
        if (object.id.empty() && object.parent.empty())
            return &object;

    return nullptr;
}

const SFMaterialGraphObject* SFMaterialGraph::FindLinkedObject(const SFMaterialGraphObject& object, const char* type, size_t index) const {
    for (const SFMaterialComponent* component : GetComposedComponents(object))
        if (component->type == type && component->index == index && !component->linkedID.empty())
            return FindObject(component->linkedID);

    return nullptr;
}

std::vector<const SFMaterialComponent*> SFMaterialGraph::GetComposedComponents(const SFMaterialGraphObject& object) const {
    // Parents first, so the object's own components come later and win. The chain is short, but a
    // malformed file could make it a loop.
    std::vector<const SFMaterialGraphObject*> chain;
    for (const SFMaterialGraphObject* current = &object; current && chain.size() < 16; current = FindObject(current->parent)) {
        if (std::find(chain.begin(), chain.end(), current) != chain.end())
            break;

        chain.push_back(current);
    }

    std::vector<const SFMaterialComponent*> components;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        for (const auto& component : (*it)->components) {
            auto same = std::find_if(components.begin(), components.end(), [&](const SFMaterialComponent* existing) {
                return existing->type == component.type && existing->index == component.index;
            });

            if (same != components.end())
                *same = &component;
            else
                components.push_back(&component);
        }
    }

    return components;
}

void SFMaterialGraph::Clear() {
    objects.clear();
    objectIndexes.clear();
}

bool SFMaterialGraph::LoadFromMaterialJson(const nlohmann::json& material) {
    Clear();

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

                    component.data = *data;
                }

                object.components.push_back(std::move(component));
            }
        }

        AddObject(std::move(object));
    }

    BuildChildLinks();
    return true;
}

void SFMaterialGraph::AddObject(SFMaterialGraphObject object) {
    objects.push_back(std::move(object));
}

void SFMaterialGraph::BuildChildLinks() {
    objectIndexes.clear();
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

    auto resolveLayer = [&](const SFMaterialGraphObject& root, size_t layerIndex, std::vector<std::string>& output) {
        const auto layerComponent = FindIndexedComponent(root, LayerIDType, layerIndex);
        if (!layerComponent)
            return false;

        const auto layer = FindObject(layerComponent->linkedID);
        if (!layer)
            return false;

        const auto materialComponent = FindIndexedComponent(*layer, MaterialIDType, 0);
        if (!materialComponent)
            return false;

        const auto material = FindObject(materialComponent->linkedID);
        if (!material)
            return false;

        const auto textureSetComponent = FindIndexedComponent(*material, TextureSetIDType, 0);
        if (!textureSetComponent)
            return false;

        const auto textureSet = FindObject(textureSetComponent->linkedID);
        if (!textureSet)
            return false;

        collectTextures(*textureSet, output);
        for (size_t childIndex : textureSet->children)
            collectTextures(objects[childIndex], output);

        return HasAnyTexture(output);
    };

    const SFMaterialGraphObject* root = FindRoot();
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

bool SFMaterialGraph::ResolveLayeredMaterial(SFLayeredMaterial& material) const {
    material = SFLayeredMaterial();

    const SFMaterialGraphObject* root = FindRoot();
    if (!root)
        return false;

    // The game only renders a run of layers from layer 0 without gaps
    for (size_t i = 0; i < SFLayeredMaterial::MaxLayers; ++i) {
        const SFMaterialGraphObject* layerObject = FindLinkedObject(*root, LayerIDType, i);
        if (!layerObject)
            break;

        ReadLayer(*layerObject, material.layers.emplace_back());
    }

    if (material.layers.empty())
        return false;

    // A missing blender blends with the defaults, which is a full linear blend of everything
    for (size_t i = 0; i + 1 < material.layers.size(); ++i) {
        SFBlender& blender = material.blenders.emplace_back();
        if (const SFMaterialGraphObject* blenderObject = FindLinkedObject(*root, BlenderIDType, i))
            ReadBlender(*blenderObject, blender);
    }

    ReadRootComponents(*root, material);
    material.layered = true;
    return true;
}

void SFMaterialGraph::ReadRootComponents(const SFMaterialGraphObject& root, SFLayeredMaterial& material) const {
    for (const SFMaterialComponent* component : GetComposedComponents(root)) {
        const nlohmann::json* data = &component->data;
        const std::string& type = component->type;

        if (type == "BSMaterial::ShaderModelComponent") {
            ReadString(Field(data, "FileName"), material.shaderModel);
        }
        else if (type == "BSMaterial::ShaderRouteComponent") {
            ReadEnum(Field(data, "Route"), routeNames, material.route);
        }
        else if (type == "BSMaterial::ParamBool" && component->index == 0) {
            ReadBool(Field(data, "Value"), material.twoSided);
        }
        else if (type == "BSMaterial::AlphaSettingsComponent") {
            ReadBool(Field(data, "HasOpacity"), material.hasOpacity);
            if (ReadFloat(Field(data, "AlphaTestThreshold"), material.alphaThreshold))
                material.alphaThreshold = std::clamp(material.alphaThreshold, 0.0f, 1.0f);
            ReadLayerName(Field(data, "OpacitySourceLayer"), material.alphaSourceLayer);

            const nlohmann::json* blender = StructField(data, "Blender");
            ReadBool(Field(blender, "UseVertexColor"), material.alphaVertexColor);
            ReadEnum(Field(blender, "VertexColorChannel"), colorChannelNames, material.alphaVertexColorChannel);

            std::string uvStreamID;
            if (ReadString(Field(StructField(blender, "OpacityUVStream"), "ID"), uvStreamID)) {
                if (const SFMaterialGraphObject* uvStreamObject = FindObject(uvStreamID)) {
                    ReadUVStream(*uvStreamObject, material.alphaUVStream);
                    material.hasAlphaUVStream = true;
                }
            }
        }
        else if (type == "BSMaterial::DecalSettingsComponent") {
            ReadBool(Field(data, "IsDecal"), material.isDecal);
            ReadFloat(Field(data, "MaterialOverallAlpha"), material.decalAlpha);
        }
        else if (type == "BSMaterial::EffectSettingsComponent") {
            material.isEffect = true;
            ReadBool(Field(data, "IsGlass"), material.effectGlass);
            ReadBool(Field(data, "VertexColorBlend"), material.effectVertexColorBlend);
            ReadBool(Field(data, "IsAlphaTested"), material.effectAlphaTested);
            ReadFloat(Field(data, "MaterialOverallAlpha"), material.effectAlpha);
            ReadEnum(Field(data, "BlendingMode"), effectBlendModeNames, material.effectBlendMode);

            bool emissiveOnly = false;
            bool emissiveOnlyAuto = false;
            ReadBool(Field(data, "EmissiveOnlyEffect"), emissiveOnly);
            ReadBool(Field(data, "EmissiveOnlyAutomaticallyApplied"), emissiveOnlyAuto);
            material.effectEmissiveOnly = emissiveOnly || emissiveOnlyAuto;
        }
        else if (type == "BSMaterial::OpacityComponent") {
            material.hasOpacityComponent = true;
            ReadLayerName(Field(data, "FirstLayerIndex"), material.opacityLayers[0]);
            ReadBool(Field(data, "SecondLayerActive"), material.opacityLayerActive[1]);
            ReadLayerName(Field(data, "SecondLayerIndex"), material.opacityLayers[1]);
            ReadEnum(Field(data, "FirstBlenderMode"), opacityBlendModeNames, material.opacityBlendModes[0]);
            ReadBool(Field(data, "ThirdLayerActive"), material.opacityLayerActive[2]);
            ReadLayerName(Field(data, "ThirdLayerIndex"), material.opacityLayers[2]);
            ReadEnum(Field(data, "SecondBlenderMode"), opacityBlendModeNames, material.opacityBlendModes[1]);
        }
        else if (type == "BSMaterial::EmissiveSettingsComponent") {
            bool enabled = false;
            ReadBool(Field(data, "Enabled"), enabled);

            // An enabled emissive settings component wins over the layered one, which the game only
            // uses for materials without it
            if (enabled) {
                const nlohmann::json* settings = StructField(data, "Settings");
                material.emissive[0] = SFEmissiveLayer();
                material.emissive[0].active = true;
                ReadEmissiveLayer(settings, "EmissiveSourceLayer", "EmissiveTint", "EmissiveMaskSourceBlender", material.emissive[0]);
                material.emissive[1] = SFEmissiveLayer();
                material.emissive[2] = SFEmissiveLayer();

                material.emittance = SFEmittance();
                material.emittance.maxOffset = 9999.0f;
                ReadEmittance(settings, material.emittance);
                material.layeredEmissivity = false;
            }
        }
        else if (type == "BSMaterial::LayeredEmissivityComponent") {
            bool enabled = false;
            ReadBool(Field(data, "Enabled"), enabled);

            if (enabled && !material.emissive[0].active) {
                material.layeredEmissivity = true;
                for (uint8_t i = 0; i < 3; ++i) {
                    material.emissive[i] = SFEmissiveLayer();
                    material.emissive[i].layer = i;
                }

                material.emissive[0].active = true;
                ReadEmissiveLayer(data, "FirstLayerIndex", "FirstLayerTint", "FirstLayerMaskIndex", material.emissive[0]);
                ReadBool(Field(data, "SecondLayerActive"), material.emissive[1].active);
                ReadEmissiveLayer(data, "SecondLayerIndex", "SecondLayerTint", "SecondLayerMaskIndex", material.emissive[1]);
                ReadBool(Field(data, "ThirdLayerActive"), material.emissive[2].active);
                ReadEmissiveLayer(data, "ThirdLayerIndex", "ThirdLayerTint", "ThirdLayerMaskIndex", material.emissive[2]);

                material.emittance = SFEmittance();
                ReadEmittance(data, material.emittance);
            }
        }
        else if (type == "BSMaterial::TranslucencySettingsComponent") {
            ReadBool(Field(data, "Enabled"), material.translucency);

            const nlohmann::json* settings = StructField(data, "Settings");
            ReadBool(Field(settings, "Thin"), material.translucencyThin);
            ReadBool(Field(settings, "UseSSS"), material.useSSS);
            ReadFloat(Field(settings, "SSSStrength"), material.sssStrength);
            ReadFloat(Field(settings, "SSSWidth"), material.sssWidth);
            ReadFloat(Field(settings, "TransmissiveScale"), material.transmissiveScale);
            ReadLayerName(Field(settings, "TransmittanceSourceLayer"), material.transmissiveLayer);
        }
        else if (type == "BSMaterial::HairSettingsComponent") {
            ReadBool(Field(data, "Enabled"), material.isHair);
            ReadFloat(Field(data, "Roughness"), material.hairRoughness);
        }
    }

    if (IsTwoSidedShaderModel(material.shaderModel))
        material.twoSided = true;
}

void SFMaterialGraph::ReadLayer(const SFMaterialGraphObject& layerObject, SFLayer& layer) const {
    if (const SFMaterialGraphObject* uvStreamObject = FindLinkedObject(layerObject, UVStreamIDType, 0))
        ReadUVStream(*uvStreamObject, layer.uvStream);

    const SFMaterialGraphObject* materialObject = FindLinkedObject(layerObject, MaterialIDType, 0);
    if (!materialObject)
        return;

    for (const SFMaterialComponent* component : GetComposedComponents(*materialObject)) {
        const nlohmann::json* data = &component->data;
        const std::string& type = component->type;

        if (type == "BSMaterial::Color")
            ReadColor(data, layer.color);
        else if (type == "BSMaterial::MaterialOverrideColorTypeComponent")
            ReadEnum(Field(data, "Value"), overrideColorNames, layer.colorLerp);
        else if (type == "BSMaterial::ParamBool" && component->index == 0)
            ReadBool(Field(data, "Value"), layer.vertexColorTint);
    }

    const SFMaterialGraphObject* textureSetObject = FindLinkedObject(*materialObject, TextureSetIDType, 0);
    if (!textureSetObject)
        return;

    SFTextureSet& textureSet = layer.textureSet;
    bool hasMRTexture[SFTextureSet::MaxSlots] = {};

    for (const SFMaterialComponent* component : GetComposedComponents(*textureSetObject)) {
        const nlohmann::json* data = &component->data;
        const std::string& type = component->type;
        const size_t slot = component->index;

        if (IsTextureComponent(*component) && slot < SFTextureSet::MaxSlots) {
            // The MR (material resolution) file of a slot wins over a plain texture file
            const bool isMR = type == MRTextureFileType;
            if (!isMR && hasMRTexture[slot])
                continue;

            hasMRTexture[slot] |= isMR;
            textureSet.files[slot] = NormalizeTexturePath(component->fileName);
        }
        else if (type == "BSMaterial::TextureReplacement" && slot < SFTextureSet::MaxSlots) {
            bool enabled = false;
            if (ReadBool(Field(data, "Enabled"), enabled)) {
                if (enabled)
                    textureSet.replacementMask |= 1u << slot;
                else
                    textureSet.replacementMask &= ~(1u << slot);
            }

            float color[4];
            if (ReadColor(Field(data, "Color"), color))
                std::copy(color, color + 4, textureSet.replacements[slot].begin());
        }
        else if (type == "BSMaterial::MaterialParamFloat" && slot == 0) {
            if (ReadFloat(Field(data, "Value"), textureSet.floatParam))
                textureSet.floatParam = std::clamp(textureSet.floatParam, 0.0f, 1.0f);
        }
    }
}

void SFMaterialGraph::ReadBlender(const SFMaterialGraphObject& blenderObject, SFBlender& blender) const {
    if (const SFMaterialGraphObject* uvStreamObject = FindLinkedObject(blenderObject, UVStreamIDType, 0))
        ReadUVStream(*uvStreamObject, blender.uvStream);

    for (const SFMaterialComponent* component : GetComposedComponents(blenderObject)) {
        const nlohmann::json* data = &component->data;
        const std::string& type = component->type;
        const size_t index = component->index;

        if (IsTextureComponent(*component) && index == 0) {
            blender.maskFile = NormalizeTexturePath(component->fileName);
        }
        else if (type == "BSMaterial::TextureReplacement") {
            ReadBool(Field(data, "Enabled"), blender.maskReplacementEnabled);
            ReadColor(Field(data, "Color"), blender.maskReplacement);
        }
        else if (type == "BSMaterial::BlendModeComponent") {
            ReadEnum(Field(data, "Value"), blendModeNames, blender.mode);
        }
        else if (type == "BSMaterial::ColorChannelTypeComponent") {
            ReadEnum(Field(data, "Value"), colorChannelNames, blender.colorChannel);
        }
        else if (type == "BSMaterial::MaterialParamFloat" && index < 5) {
            if (ReadFloat(Field(data, "Value"), blender.floatParams[index]))
                blender.floatParams[index] = std::clamp(blender.floatParams[index], 0.0f, 1.0f);
        }
        else if (type == "BSMaterial::ParamBool" && index < 8) {
            ReadBool(Field(data, "Value"), blender.boolParams[index]);
        }
    }
}

void SFMaterialGraph::ReadUVStream(const SFMaterialGraphObject& uvStreamObject, SFUVStream& uvStream) const {
    for (const SFMaterialComponent* component : GetComposedComponents(uvStreamObject)) {
        const nlohmann::json* data = &component->data;
        const std::string& type = component->type;

        if (type == "BSMaterial::Scale") {
            ReadVector(data, uvStream.scale);
        }
        else if (type == "BSMaterial::Offset") {
            ReadVector(data, uvStream.offset);
        }
        else if (type == "BSMaterial::TextureAddressModeComponent") {
            ReadEnum(Field(data, "Value"), addressModeNames, uvStream.addressMode);
        }
        else if (type == "BSMaterial::Channel") {
            uint8_t channel = 1;
            if (ReadEnum(Field(data, "Value"), channelNames, channel))
                uvStream.channelTwo = channel > 1;
        }
    }
}
