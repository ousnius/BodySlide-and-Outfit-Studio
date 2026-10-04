/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// A Starfield layered material (.mat or materialsbeta.cdb entry), reduced to what the preview renders.
// The model follows the game's component database: a root material stacks up to six layers, each with
// a material (tint and texture set) and a UV stream, and blender n mixes layer n + 1 onto the result of
// the layers below it. Field meanings and defaults follow NifSkope's CE2Material, which matches what the
// root templates in materials/layered/root/ define - a loose .mat only stores what differs from those.

enum class SFBlendMode : uint8_t {
    Linear = 0,
    Additive,
    PositionContrast,
    None,
    CharacterCombine,
    Skin
};

enum class SFShaderRoute : uint8_t {
    Deferred = 0,
    Effect,
    PlanetaryRing,
    PrecomputedScattering,
    Water
};

struct SFUVStream {
    float scale[2] = {1.0f, 1.0f};
    float offset[2] = {0.0f, 0.0f};
    // 0 = Wrap, 1 = Clamp, 2 = Mirror, 3 = Border
    uint8_t addressMode = 0;
    bool channelTwo = false;
};

struct SFTextureSet {
    // Texture slots as the game numbers them. 0-8 are the ones every layer reads (color, normal,
    // opacity, roughness, metalness, AO, height, emissive, transmissive); CharacterCombine layers read
    // their color, roughness and metalness from the overlay slots 14-16 instead.
    static constexpr size_t MaxSlots = 21;

    std::array<std::string, MaxSlots> files;
    // Constant RGBA standing in for the slot when its replacement is enabled and no texture is there
    // (or it can't be loaded). Stored as authored, 0-1 per channel, not decoded yet.
    std::array<std::array<float, 4>, MaxSlots> replacements = DefaultReplacements();
    uint32_t replacementMask = 0;
    // Normal map intensity
    float floatParam = 1.0f;

    static std::array<std::array<float, 4>, MaxSlots> DefaultReplacements();
};

struct SFLayer {
    SFTextureSet textureSet;
    float color[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    // MaterialOverrideColorTypeComponent: Lerp mixes the color over the texture by its alpha,
    // Multiply tints the texture with it
    bool colorLerp = true;
    // ParamBool 0 of the material: the vertex color replaces the material color as the tint
    bool vertexColorTint = false;
    SFUVStream uvStream;
};

struct SFBlender {
    std::string maskFile;
    float maskReplacement[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool maskReplacementEnabled = false;
    SFUVStream uvStream;
    SFBlendMode mode = SFBlendMode::Linear;
    // Vertex color channel the mask is multiplied with when boolParams[5] is set: 0-3 = RGBA
    uint8_t colorChannel = 0;
    // 0: height blend threshold, 1: height blend factor, 2: position, 3: 1.0 - contrast, 4: mask intensity
    float floatParams[5] = {0.5f, 0.5f, 0.5f, 1.0f, 1.0f};
    // 0: blend color, 1: blend metalness, 2: blend roughness, 3: blend normals, 4: blend normals
    // additively, 5: multiply the mask with a vertex color channel, 6: blend AO, 7: use the detail mask
    bool boolParams[8] = {true, true, true, true, true, false, true, false};
};

struct SFEmissiveLayer {
    bool active = false;
    uint8_t layer = 0;
    float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    // 0 = no mask, n = the mask of blender n - 1
    uint8_t maskBlender = 0;
};

struct SFEmittance {
    bool adaptive = false;
    bool adaptiveLimits = false;
    float luminousEmittance = 100.0f;
    float exposureOffset = 0.0f;
    float maxOffset = 1.0f;
    float minOffset = 0.0f;
};

struct SFLayeredMaterial {
    static constexpr size_t MaxLayers = 6;
    static constexpr size_t MaxBlenders = MaxLayers - 1;

    std::string shaderModel = "BaseMaterial";
    SFShaderRoute route = SFShaderRoute::Deferred;

    // Layers 0 to n - 1 without gaps, the way the game only renders a contiguous run from layer 0.
    // Blender i blends layer i + 1, so there is always one fewer of them.
    std::vector<SFLayer> layers;
    std::vector<SFBlender> blenders;

    bool twoSided = false;
    bool isHair = false;
    float hairRoughness = 0.38f;

    // AlphaSettingsComponent (deferred route)
    bool hasOpacity = false;
    float alphaThreshold = 1.0f / 3.0f;
    uint8_t alphaSourceLayer = 0;
    bool hasAlphaUVStream = false;
    SFUVStream alphaUVStream;
    bool alphaVertexColor = false;
    uint8_t alphaVertexColorChannel = 0;

    // DecalSettingsComponent
    bool isDecal = false;
    float decalAlpha = 1.0f;

    // EffectSettingsComponent (effect route)
    bool isEffect = false;
    bool effectGlass = false;
    bool effectVertexColorBlend = false;
    bool effectAlphaTested = false;
    bool effectEmissiveOnly = false;
    float effectAlpha = 1.0f;
    // 0 = AlphaBlend, 1 = Additive, 2 = SourceSoftAdditive, 3 = Multiply, 4 = DestinationSoftAdditive,
    // 5 = DestinationInvertedSoftAdditive, 6 = TakeSmaller, 7 = None
    uint8_t effectBlendMode = 0;

    // OpacityComponent (effect route): which layers make up the opacity, and how
    bool hasOpacityComponent = false;
    uint8_t opacityLayers[3] = {0, 1, 2};
    bool opacityLayerActive[3] = {true, false, false};
    // 0 = Lerp, 1 = Additive, 2 = Subtractive, 3 = Multiplicative
    uint8_t opacityBlendModes[2] = {0, 0};

    // EmissiveSettingsComponent fills emissive[0]; LayeredEmissivityComponent up to all three
    SFEmissiveLayer emissive[3];
    SFEmittance emittance;
    bool layeredEmissivity = false;

    // TranslucencySettingsComponent
    bool translucency = false;
    bool translucencyThin = false;
    bool useSSS = false;
    float sssStrength = 0.5f;
    float sssWidth = 0.025f;
    float transmissiveScale = 0.05f;
    uint8_t transmissiveLayer = 0;

    // Whether the layers came out of a layered material at all, rather than out of a flat list of
    // texture files (see FromTextureFiles)
    bool layered = false;

    // Whether the shader model is one of the eye models or the 1LayerMouth one
    bool IsEye() const;
    bool IsMouth() const;

    // The texture files of the slots that the texture grid and the other games' material
    // pipeline call 0-7, from the first layer that has any.
    std::vector<std::string> GetPrimaryTextureFiles(size_t numTextures) const;
    // Replaces every texture file that is set with what resolve makes of it, which is how the paths the
    // material stores relative to the data folder become the absolute ones the texture loader takes.
    void ResolveTexturePaths(const std::function<std::string(const std::string&)>& resolve);

    // A single layer material made of the given slots 0-7, for a shape whose material couldn't be
    // read and for textures chosen by hand.
    static SFLayeredMaterial FromTextureFiles(const std::vector<std::string>& textureFiles);
};

// Everything the shader needs to render a layered material, laid out the way sf_default.frag declares
// its uniforms: flat arrays it can take in one call each.
struct SFRenderData {
    static constexpr size_t SlotsPerLayer = 9;

    // Texture files to bind, in unit order. Every reference below indexes this list.
    std::vector<std::string> textures;

    int numLayers = 0;
    // Per layer and slot: 0 = nothing, -1 = the replacement color, n > 0 = textures[n - 1]
    int layerTextures[SFLayeredMaterial::MaxLayers * SlotsPerLayer] = {};
    // Replacement colors, already decoded to what the texture would sample as
    float layerReplacements[SFLayeredMaterial::MaxLayers * SlotsPerLayer * 4] = {};
    float layerColors[SFLayeredMaterial::MaxLayers * 4] = {};
    // Scale xy, offset zw
    float layerUVs[SFLayeredMaterial::MaxLayers * 4] = {};
    // Bit 0 = lerp the color, bit 1 = vertex color tint, bit 2 = second UV channel
    int layerFlags[SFLayeredMaterial::MaxLayers] = {};
    float layerNormalScales[SFLayeredMaterial::MaxLayers] = {};

    int blenderMasks[SFLayeredMaterial::MaxBlenders] = {};
    float blenderMaskReplacements[SFLayeredMaterial::MaxBlenders * 4] = {};
    float blenderUVs[SFLayeredMaterial::MaxBlenders * 4] = {};
    int blenderModes[SFLayeredMaterial::MaxBlenders] = {};
    int blenderChannels[SFLayeredMaterial::MaxBlenders] = {};
    // Bits 0-7 = boolParams, bit 8 = second UV channel
    int blenderFlags[SFLayeredMaterial::MaxBlenders] = {};
    // floatParams 0-3, then the mask intensity (floatParams 4) on its own
    float blenderParams[SFLayeredMaterial::MaxBlenders * 4] = {};
    float blenderIntensities[SFLayeredMaterial::MaxBlenders] = {};

    // See the Flag* constants
    int flags = 0;
    float alphaThreshold = 0.0f;
    // Opacity source layer of the deferred route, and the vertex color channel that scales it
    int alphaSourceLayer = 0;
    int alphaVertexColorChannel = 0;
    float alphaUV[4] = {1.0f, 1.0f, 0.0f, 0.0f};
    // Decal alpha or the effect's overall alpha
    float materialAlpha = 1.0f;
    // Effect route opacity: layers 0-2 (-1 = inactive), then the two blend modes
    int opacityLayers[3] = {0, -1, -1};
    int opacityBlendModes[2] = {0, 0};

    // Emissive sources: layer (-1 = inactive), mask blender (0 = none), tint, plus their overall scale
    int emissiveLayers[3] = {-1, -1, -1};
    int emissiveMasks[3] = {};
    float emissiveTints[3 * 4] = {};
    float emissiveIntensity = 0.0f;

    // Thin translucency: source layer (-1 = none) and scale. SSS strength drives a wrapped diffuse.
    int transmissiveLayer = -1;
    float transmissiveScale = 0.0f;
    float sssStrength = 0.0f;

    static constexpr int FlagEffect = 1 << 0;
    static constexpr int FlagOpacityComponent = 1 << 1;
    static constexpr int FlagGlass = 1 << 2;
    static constexpr int FlagEffectVertexColor = 1 << 3;
    static constexpr int FlagHasOpacity = 1 << 4;
    static constexpr int FlagAlphaVertexColor = 1 << 5;
    static constexpr int FlagDecal = 1 << 6;
    static constexpr int FlagAlphaTest = 1 << 7;
    static constexpr int FlagAlphaBlend = 1 << 8;
    static constexpr int FlagAlphaUVChannelTwo = 1 << 9;
    static constexpr int FlagHair = 1 << 10;
    static constexpr int FlagEye = 1 << 11;
    static constexpr int FlagSoftAdditive = 1 << 12;

    // Texture files the material references, without duplicates and in the order they matter most
    // in: the first layer's color, normal and PBR maps, then the blender masks, then the later layers,
    // then everything else. A list longer than the shader has samplers for is cut from the end.
    static std::vector<std::string> CollectTextures(const SFLayeredMaterial& material);

    // Builds the shader data out of a material. availableTextures lists the textures that could be
    // loaded, in unit order; a slot whose texture isn't among them falls back to its replacement color
    // when it has one and to nothing otherwise.
    static SFRenderData Build(const SFLayeredMaterial& material, const std::vector<std::string>& availableTextures);

    // Whether any slot of any layer resolved to a texture or a replacement color, so that the material
    // has anything to show when textures are on.
    bool HasAnyTexture() const;
};
