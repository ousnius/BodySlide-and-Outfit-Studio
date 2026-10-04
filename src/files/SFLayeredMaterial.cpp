/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SFLayeredMaterial.h"

#include "../utils/StringStuff.h"

#include <algorithm>
#include <cmath>

namespace {
// What a slot holds decides how its replacement color has to be decoded to sample like the texture
// would: color and emissive are sRGB, the normal map is signed, the rest is plain linear data.
enum class ReplacementMode {
    Linear,
    SRGB,
    Signed
};

ReplacementMode GetReplacementMode(size_t shaderSlot) {
    switch (shaderSlot) {
        case 0:
        case 7: return ReplacementMode::SRGB;
        case 1: return ReplacementMode::Signed;
        default: return ReplacementMode::Linear;
    }
}

float SRGBToLinear(float value) {
    return std::pow(std::max(value, 0.0f), 2.2f);
}

void DecodeReplacement(const float* color, ReplacementMode mode, float* out) {
    for (size_t i = 0; i < 4; ++i) {
        float value = color[i];
        if (mode == ReplacementMode::SRGB && i < 3)
            value = SRGBToLinear(value);
        else if (mode == ReplacementMode::Signed)
            value = value * 2.0f - 1.0f;

        out[i] = value;
    }
}

// Texture slot a shader slot reads for a layer. CharacterCombine layers take their color, roughness
// and metalness from the overlay slots, which is where the game keeps the maps meant for multiplying.
size_t GetTextureSlot(size_t shaderSlot, bool characterCombine) {
    if (characterCombine) {
        switch (shaderSlot) {
            case 0: return 14;
            case 3: return 15;
            case 4: return 16;
        }
    }

    return shaderSlot;
}

std::string GetTextureKey(const std::string& file) {
    std::string key = ToLower(file);
    std::replace(key.begin(), key.end(), '\\', '/');
    return key;
}

int FindTexture(const std::vector<std::string>& textures, const std::string& file) {
    if (file.empty())
        return -1;

    const std::string key = GetTextureKey(file);
    for (size_t i = 0; i < textures.size(); ++i)
        if (GetTextureKey(textures[i]) == key)
            return static_cast<int>(i);

    return -1;
}

void CopyUVStream(const SFUVStream& uvStream, float* out) {
    out[0] = uvStream.scale[0];
    out[1] = uvStream.scale[1];
    out[2] = uvStream.offset[0];
    out[3] = uvStream.offset[1];
}

size_t GetNumLayers(const SFLayeredMaterial& material) {
    // The eye model renders two layers at most and Skin5Layer five, whatever the material lists
    size_t maxLayers = SFLayeredMaterial::MaxLayers;
    if (material.shaderModel == "Eye1Layer")
        maxLayers = 2;
    else if (material.shaderModel == "Skin5Layer")
        maxLayers = 5;

    return std::min(material.layers.size(), maxLayers);
}

bool IsCharacterCombine(const SFLayeredMaterial& material, size_t layerIndex) {
    return layerIndex > 0 && layerIndex - 1 < material.blenders.size() && material.blenders[layerIndex - 1].mode == SFBlendMode::CharacterCombine;
}

bool IsEffectRoute(const SFLayeredMaterial& material) {
    return material.isEffect && material.route != SFShaderRoute::Deferred;
}

bool IsEmissiveSource(const SFLayeredMaterial& material, size_t layerIndex) {
    for (const auto& source : material.emissive)
        if (source.active && source.layer == layerIndex)
            return true;

    return false;
}

// Shader slots a layer is sampled at, which leaves out the height map: the preview does no parallax.
bool IsShaderSlotUsed(const SFLayeredMaterial& material, size_t layerIndex, size_t shaderSlot) {
    switch (shaderSlot) {
        case 2: return IsEffectRoute(material) || (material.hasOpacity && material.alphaSourceLayer == layerIndex);
        case 6: return false;
        case 7: return IsEmissiveSource(material, layerIndex);
        case 8: return material.translucency && material.translucencyThin && material.transmissiveLayer == layerIndex;
        default: return true;
    }
}

// sqrt(lux / 100) is how NifSkope brings the game's emittance into display range. The adaptive kind
// is relative to the scene's exposure, which stands at roughly 100 lux for the viewport's lights.
float GetEmissiveIntensity(const SFEmittance& emittance) {
    float luminance = emittance.luminousEmittance;
    if (emittance.adaptive) {
        luminance = 100.0f * std::exp2(emittance.exposureOffset);
        if (emittance.adaptiveLimits)
            luminance = std::clamp(luminance, emittance.minOffset, emittance.maxOffset);
    }

    return std::sqrt(std::max(luminance, 0.0f) * 0.01f);
}
}

std::array<std::array<float, 4>, SFTextureSet::MaxSlots> SFTextureSet::DefaultReplacements() {
    // The root template's replacement colors: black for most maps, a flat normal, opaque, unoccluded,
    // and mid grey for the overlay and mask slots
    constexpr float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    constexpr float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    constexpr float grey[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    constexpr float flatNormal[4] = {0.5f, 0.5f, 1.0f, 1.0f};

    const float* defaults[MaxSlots] = {
        black, flatNormal, white, black, black, white, black, black, black, grey, black, grey,
        black, black, grey, grey, grey, black, black, white, grey};

    std::array<std::array<float, 4>, MaxSlots> replacements{};
    for (size_t i = 0; i < MaxSlots; ++i)
        std::copy(defaults[i], defaults[i] + 4, replacements[i].begin());

    return replacements;
}

bool SFLayeredMaterial::IsEye() const {
    return shaderModel == "Eye1Layer" || shaderModel == "EyeWetness1Layer";
}

bool SFLayeredMaterial::IsMouth() const {
    return shaderModel == "1LayerMouth";
}

std::vector<std::string> SFLayeredMaterial::GetPrimaryTextureFiles(size_t numTextures) const {
    std::vector<std::string> textureFiles(numTextures);
    constexpr size_t numSlots = 8;

    for (const auto& layer : layers) {
        const auto& files = layer.textureSet.files;
        if (std::none_of(files.begin(), files.begin() + numSlots, [](const std::string& file) { return !file.empty(); }))
            continue;

        for (size_t i = 0; i < std::min(numTextures, numSlots); ++i)
            textureFiles[i] = files[i];

        break;
    }

    return textureFiles;
}

void SFLayeredMaterial::ResolveTexturePaths(const std::function<std::string(const std::string&)>& resolve) {
    for (auto& layer : layers)
        for (auto& file : layer.textureSet.files)
            if (!file.empty())
                file = resolve(file);

    for (auto& blender : blenders)
        if (!blender.maskFile.empty())
            blender.maskFile = resolve(blender.maskFile);
}

SFLayeredMaterial SFLayeredMaterial::FromTextureFiles(const std::vector<std::string>& textureFiles) {
    SFLayeredMaterial material;

    SFLayer& layer = material.layers.emplace_back();
    for (size_t i = 0; i < std::min<size_t>(textureFiles.size(), 8); ++i)
        layer.textureSet.files[i] = textureFiles[i];

    return material;
}

std::vector<std::string> SFRenderData::CollectTextures(const SFLayeredMaterial& material) {
    std::vector<std::string> textures;
    auto add = [&](const std::string& file) {
        if (!file.empty() && FindTexture(textures, file) == -1)
            textures.push_back(file);
    };

    const size_t numLayers = GetNumLayers(material);
    auto addLayer = [&](size_t layerIndex, const auto& shaderSlots) {
        const bool characterCombine = IsCharacterCombine(material, layerIndex);
        for (size_t shaderSlot : shaderSlots)
            if (IsShaderSlotUsed(material, layerIndex, shaderSlot))
                add(material.layers[layerIndex].textureSet.files[GetTextureSlot(shaderSlot, characterCombine)]);
    };

    constexpr std::array<size_t, 5> surfaceSlots = {0, 1, 3, 4, 5};
    constexpr std::array<size_t, 3> otherSlots = {2, 7, 8};

    if (numLayers > 0)
        addLayer(0, surfaceSlots);

    for (size_t i = 1; i < numLayers; ++i)
        if (i - 1 < material.blenders.size())
            add(material.blenders[i - 1].maskFile);

    for (size_t i = 1; i < numLayers; ++i)
        addLayer(i, surfaceSlots);

    for (size_t i = 0; i < numLayers; ++i)
        addLayer(i, otherSlots);

    return textures;
}

SFRenderData SFRenderData::Build(const SFLayeredMaterial& material, const std::vector<std::string>& availableTextures) {
    SFRenderData data;
    data.textures = availableTextures;

    const size_t numLayers = GetNumLayers(material);
    data.numLayers = static_cast<int>(numLayers);

    const bool isHair = material.isHair || material.shaderModel == "Hair1Layer";

    for (size_t i = 0; i < numLayers; ++i) {
        const SFLayer& layer = material.layers[i];
        const SFTextureSet& textureSet = layer.textureSet;
        const bool characterCombine = IsCharacterCombine(material, i);

        for (size_t shaderSlot = 0; shaderSlot < SlotsPerLayer; ++shaderSlot) {
            const size_t textureSlot = GetTextureSlot(shaderSlot, characterCombine);
            const size_t index = i * SlotsPerLayer + shaderSlot;
            float* replacement = &data.layerReplacements[index * 4];

            const int texture = IsShaderSlotUsed(material, i, shaderSlot) ? FindTexture(data.textures, textureSet.files[textureSlot]) : -1;
            if (texture >= 0) {
                data.layerTextures[index] = texture + 1;
            }
            else if (textureSet.replacementMask & (1u << textureSlot)) {
                data.layerTextures[index] = -1;
                DecodeReplacement(textureSet.replacements[textureSlot].data(), GetReplacementMode(shaderSlot), replacement);
            }
            else if (characterCombine && (shaderSlot == 0 || shaderSlot == 3 || shaderSlot == 4)) {
                // A multiplier of 0.5 doubled by the blend leaves the layers below as they were
                data.layerTextures[index] = -1;
                std::fill(replacement, replacement + 4, 0.5f);
            }
            else if (shaderSlot == 3 && isHair && material.shaderModel == "Hair1Layer") {
                // Hair without a roughness map takes the hair settings' roughness, remapped the way
                // NifSkope does
                const float r = material.hairRoughness;
                data.layerTextures[index] = -1;
                std::fill(replacement, replacement + 4, ((r - 2.0f) * r + 2.0f) * r);
            }
        }

        // The color is authored in sRGB, its alpha (the lerp weight) is linear
        for (size_t c = 0; c < 3; ++c)
            data.layerColors[i * 4 + c] = SRGBToLinear(layer.color[c]);
        data.layerColors[i * 4 + 3] = layer.color[3];

        CopyUVStream(layer.uvStream, &data.layerUVs[i * 4]);
        data.layerNormalScales[i] = textureSet.floatParam;

        int flags = 0;
        if (layer.colorLerp)
            flags |= 1;
        // The mouth model ignores the vertex color tint
        if (layer.vertexColorTint && !material.IsMouth())
            flags |= 2;
        if (layer.uvStream.channelTwo)
            flags |= 4;
        data.layerFlags[i] = flags;
    }

    for (size_t i = 0; i + 1 < numLayers; ++i) {
        const SFBlender defaultBlender;
        const SFBlender& blender = i < material.blenders.size() ? material.blenders[i] : defaultBlender;

        const int mask = FindTexture(data.textures, blender.maskFile);
        if (mask >= 0) {
            data.blenderMasks[i] = mask + 1;
        }
        else if (blender.maskReplacementEnabled) {
            data.blenderMasks[i] = -1;
            std::copy(blender.maskReplacement, blender.maskReplacement + 4, &data.blenderMaskReplacements[i * 4]);
        }

        CopyUVStream(blender.uvStream, &data.blenderUVs[i * 4]);
        data.blenderModes[i] = static_cast<int>(blender.mode);
        data.blenderChannels[i] = blender.colorChannel;

        int flags = 0;
        for (size_t b = 0; b < 8; ++b)
            if (blender.boolParams[b])
                flags |= 1 << b;
        if (blender.uvStream.channelTwo)
            flags |= 1 << 8;
        data.blenderFlags[i] = flags;

        std::copy(blender.floatParams, blender.floatParams + 4, &data.blenderParams[i * 4]);
        data.blenderIntensities[i] = blender.floatParams[4];
    }

    const bool isEffect = IsEffectRoute(material);
    int flags = 0;

    if (isEffect) {
        flags |= FlagEffect | FlagAlphaBlend;
        if (material.effectAlphaTested)
            flags |= FlagAlphaTest;
        if (material.effectGlass)
            flags |= FlagGlass;
        if (material.effectVertexColorBlend)
            flags |= FlagEffectVertexColor;
        if (material.effectBlendMode == 2 && !material.effectEmissiveOnly)
            flags |= FlagSoftAdditive;

        data.materialAlpha = material.effectAlpha;

        if (material.hasOpacityComponent) {
            flags |= FlagOpacityComponent;
            for (size_t i = 0; i < 3; ++i)
                data.opacityLayers[i] = (i == 0 || material.opacityLayerActive[i]) ? material.opacityLayers[i] : -1;
            data.opacityBlendModes[0] = material.opacityBlendModes[0];
            data.opacityBlendModes[1] = material.opacityBlendModes[1];
        }
    }
    else {
        if (material.isDecal) {
            flags |= FlagDecal | FlagAlphaBlend;
            data.materialAlpha = material.decalAlpha;
        }

        if (material.hasOpacity) {
            flags |= FlagHasOpacity;
            if (material.alphaThreshold > 0.0f)
                flags |= FlagAlphaTest;
            if (material.alphaVertexColor)
                flags |= FlagAlphaVertexColor;

            data.alphaThreshold = material.alphaThreshold;
            data.alphaSourceLayer = material.alphaSourceLayer;
            data.alphaVertexColorChannel = material.alphaVertexColorChannel;

            const SFUVStream alphaUVStream = material.hasAlphaUVStream ? material.alphaUVStream : SFUVStream();
            CopyUVStream(alphaUVStream, data.alphaUV);
            if (alphaUVStream.channelTwo)
                flags |= FlagAlphaUVChannelTwo;
        }
    }

    if (isHair)
        flags |= FlagHair;
    if (material.IsEye())
        flags |= FlagEye;

    data.flags = flags;

    bool hasEmissive = false;
    for (size_t i = 0; i < 3; ++i) {
        const SFEmissiveLayer& source = material.emissive[i];
        if (!source.active || source.layer >= numLayers)
            continue;

        data.emissiveLayers[i] = source.layer;
        data.emissiveMasks[i] = source.maskBlender;
        for (size_t c = 0; c < 3; ++c)
            data.emissiveTints[i * 4 + c] = SRGBToLinear(source.tint[c]);
        data.emissiveTints[i * 4 + 3] = source.tint[3];
        hasEmissive = true;
    }

    if (hasEmissive)
        data.emissiveIntensity = GetEmissiveIntensity(material.emittance);

    if (material.translucency) {
        if (material.translucencyThin && material.transmissiveLayer < numLayers) {
            data.transmissiveLayer = material.transmissiveLayer;
            data.transmissiveScale = material.transmissiveScale;
        }

        if (material.useSSS)
            data.sssStrength = material.sssStrength;
    }

    return data;
}

bool SFRenderData::HasAnyTexture() const {
    for (int i = 0; i < numLayers * static_cast<int>(SlotsPerLayer); ++i)
        if (layerTextures[i] != 0)
            return true;

    return false;
}
