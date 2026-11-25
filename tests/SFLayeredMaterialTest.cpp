#include "files/SFLayeredMaterial.h"
#include "files/SFMaterialFile.h"

#include "SFTestMaterials.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    INFO(message);
    REQUIRE(condition);
}


const std::string colorFile = "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds";
const std::string normalFile = "textures/actors/human/naked_body/nakedbodyf_normal.dds";
const std::string roughFile = "textures/actors/human/naked_body/nakedbodyf_rough.dds";
const std::string aoFile = "textures/actors/human/naked_body/nakedbodyf_ao.dds";
const std::string maskFile = "textures/actors/human/naked_body/nakedbodyf_mask.dds";
const std::string detailNormalFile = "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds";

SFLayeredMaterial ParseLayered(const char* json) {
    std::istringstream input(json);
    SFMaterialFile file(input);
    Require(!file.Failed(), "Material should parse");
    return file.GetLayeredMaterial();
}
}

TEST_CASE("Starfield layered materials read layers, blenders and settings", "[SFLayeredMaterial]") {
    const SFLayeredMaterial material = ParseLayered(NakedBodyMaterialJson);

    Require(material.layered, "A material with a layer graph should come out layered");
    Require(material.shaderModel == "BodySkin2Layer", "Shader model should be read");
    Require(material.route == SFShaderRoute::Deferred, "Shader route should be read");
    Require(material.layers.size() == 2, "Both layers should be read");
    Require(material.blenders.size() == 1, "One blender should blend the second layer");

    const SFLayer& base = material.layers[0];
    Require(base.textureSet.files[0] == colorFile, "Base layer color texture");
    Require(base.textureSet.files[1] == normalFile, "Base layer normal texture");
    Require(base.textureSet.files[2].empty(), "Base layer has no opacity texture");
    Require(base.textureSet.files[3] == roughFile, "Base layer roughness texture");
    Require(base.textureSet.files[5] == aoFile, "Base layer AO texture");
    Require((base.textureSet.replacementMask & (1u << 8)) != 0, "Transmissive replacement should be enabled");
    Require(base.textureSet.replacements[8][0] == 0.0f && base.textureSet.replacements[8][3] == 1.0f, "Transmissive replacement color should be read");
    Require(!base.colorLerp, "Base layer color should multiply");
    Require(base.color[0] == Catch::Approx(0.737255f), "Base layer color should be read");
    Require(base.color[3] == 0.0f, "Base layer color alpha should be read");
    Require(base.uvStream.scale[0] == 1.0f && !base.uvStream.channelTwo, "Base layer UV stream");

    const SFLayer& detail = material.layers[1];
    Require(detail.textureSet.files[0].empty(), "Detail layer has no color texture");
    Require(detail.textureSet.files[1] == detailNormalFile, "Detail layer normal texture");
    Require(detail.colorLerp, "Detail layer color should lerp");
    Require(detail.uvStream.scale[0] == 50.0f && detail.uvStream.scale[1] == 50.0f, "Detail layer UV scale");

    const SFBlender& blender = material.blenders[0];
    Require(blender.maskFile == maskFile, "Blender mask texture");
    Require(blender.mode == SFBlendMode::Skin, "Blender mode");
    Require(blender.floatParams[4] == 0.5f, "Blender mask intensity");
    Require(blender.floatParams[2] == 0.5f && blender.floatParams[3] == 1.0f, "Unset blender floats keep their defaults");
    const bool expectedBools[8] = {false, false, false, true, true, false, true, false};
    for (size_t i = 0; i < 8; ++i)
        Require(blender.boolParams[i] == expectedBools[i], "Blender bool parameters");

    Require(material.translucency && material.useSSS, "Translucency settings should be read");
    Require(material.sssStrength == 0.5f, "SSS strength");
    Require(material.transmissiveScale == Catch::Approx(0.05f), "Transmissive scale");
    Require(!material.twoSided, "Body skin is single sided");
    Require(!material.hasOpacity, "Body skin has no opacity");
}

TEST_CASE("Starfield layered materials agree with the primary texture resolution", "[SFLayeredMaterial]") {
    std::istringstream input(NakedBodyMaterialJson);
    SFMaterialFile file(input);
    Require(!file.Failed(), "Material should parse");

    const std::vector<std::string> primary = file.GetTextureFiles(10);
    const std::vector<std::string> fromLayers = file.GetLayeredMaterial().GetPrimaryTextureFiles(10);
    Require(primary == fromLayers, "The layered material's primary textures should match the texture grid's");
}

TEST_CASE("Starfield materials without a layer graph become a single layer", "[SFLayeredMaterial]") {
    const char* flatJson = R"json(
{
    "Objects": [
        {
            "Components": [
                { "Type": "BSMaterial::MRTextureFile", "Index": 0, "Data": { "FileName": "textures/clothes/suit_color.dds" } },
                { "Type": "BSMaterial::MRTextureFile", "Index": 1, "Data": { "FileName": "textures/clothes/suit_normal.dds" } }
            ]
        }
    ]
}
)json";

    const SFLayeredMaterial material = ParseLayered(flatJson);
    Require(!material.layered, "A flat material should not count as layered");
    Require(material.layers.size() == 1 && material.blenders.empty(), "A flat material should be a single layer");
    Require(material.layers[0].textureSet.files[0] == "textures/clothes/suit_color.dds", "Color should carry over");
    Require(material.layers[0].textureSet.files[1] == "textures/clothes/suit_normal.dds", "Normal should carry over");
    Require(material.layers[0].colorLerp && material.layers[0].color[3] == 0.0f, "Default tint should leave the texture alone");
}

TEST_CASE("Starfield layered materials apply defaults, numbers and inheritance", "[SFLayeredMaterial]") {
    const char* json = R"json(
{
    "Objects": [
        {
            "Parent": "materials\\layered\\root\\layeredmaterials.mat",
            "Components": [
                { "Type": "BSMaterial::LayerID", "Index": 0, "Data": { "ID": "res:LAYER0" } },
                { "Type": "BSMaterial::LayerID", "Index": 1, "Data": { "ID": "res:LAYER1" } },
                { "Type": "BSMaterial::LayerID", "Index": 3, "Data": { "ID": "res:LAYER0" } },
                { "Type": "BSMaterial::ShaderModelComponent", "Index": 0, "Data": { "FileName": "TwoSided1Layer" } },
                {
                    "Type": "BSMaterial::AlphaSettingsComponent",
                    "Index": 0,
                    "Data": {
                        "HasOpacity": true,
                        "AlphaTestThreshold": 0.5,
                        "OpacitySourceLayer": "MATERIAL_LAYER_1",
                        "Blender": { "Data": { "UseVertexColor": "true", "VertexColorChannel": "Alpha" }, "Type": "BSMaterial::AlphaBlenderSettings" }
                    }
                }
            ]
        },
        {
            "ID": "res:LAYER0",
            "Components": [ { "Type": "BSMaterial::MaterialID", "Index": 0, "Data": { "ID": "res:MATERIAL0" } } ]
        },
        {
            "ID": "res:MATERIAL0",
            "Components": [ { "Type": "BSMaterial::TextureSetID", "Index": 0, "Data": { "ID": "res:DERIVED_SET" } } ]
        },
        {
            "ID": "res:BASE_SET",
            "Components": [
                { "Type": "BSMaterial::MRTextureFile", "Index": 0, "Data": { "FileName": "textures/base_color.dds" } },
                { "Type": "BSMaterial::MRTextureFile", "Index": 1, "Data": { "FileName": "textures/base_normal.dds" } }
            ]
        },
        {
            "ID": "res:DERIVED_SET",
            "Parent": "res:BASE_SET",
            "Components": [
                { "Type": "BSMaterial::TextureFile", "Index": 0, "Data": { "FileName": "textures/plain_color.dds" } },
                { "Type": "BSMaterial::MRTextureFile", "Index": 1, "Data": { "FileName": "textures/derived_normal.dds" } },
                { "Type": "BSMaterial::MaterialParamFloat", "Index": 0, "Data": { "Value": 0.25 } }
            ]
        },
        {
            "ID": "res:LAYER1",
            "Components": [ ]
        }
    ]
}
)json";

    const SFLayeredMaterial material = ParseLayered(json);
    Require(material.layers.size() == 2, "Layers stop at the first gap");
    Require(material.blenders.size() == 1, "A missing blender is filled in with defaults");
    Require(material.blenders[0].mode == SFBlendMode::Linear && material.blenders[0].boolParams[0], "Default blender blends linearly");

    const SFTextureSet& textureSet = material.layers[0].textureSet;
    Require(textureSet.files[0] == "textures/base_color.dds", "An inherited MR texture wins over a plain texture file");
    Require(textureSet.files[1] == "textures/derived_normal.dds", "A derived object's texture overrides its parent's");
    Require(textureSet.floatParam == 0.25f, "JSON numbers should parse");

    Require(material.twoSided, "TwoSided1Layer renders both faces");
    Require(material.hasOpacity && material.alphaThreshold == 0.5f, "JSON booleans and numbers should parse");
    Require(material.alphaSourceLayer == 1, "Opacity source layer");
    Require(material.alphaVertexColor && material.alphaVertexColorChannel == 3, "Alpha blender settings");
}

TEST_CASE("Starfield render data references deduplicated textures", "[SFLayeredMaterial]") {
    const SFLayeredMaterial material = ParseLayered(NakedBodyMaterialJson);

    const std::vector<std::string> textures = SFRenderData::CollectTextures(material);
    const std::vector<std::string> expectedOrder = {colorFile, normalFile, roughFile, aoFile, maskFile, detailNormalFile};
    Require(textures == expectedOrder, "Textures should be collected in priority order");

    SFRenderData data = SFRenderData::Build(material, textures);
    Require(data.numLayers == 2, "Both layers render");
    Require(data.layerTextures[0] == 1 && data.layerTextures[1] == 2, "Base color and normal reference their textures");
    Require(data.layerTextures[2] == 0, "Missing opacity references nothing");
    Require(data.layerTextures[3] == 3 && data.layerTextures[5] == 4, "Roughness and AO reference their textures");
    Require(data.layerTextures[8] == -1, "Transmissive uses its replacement color");
    Require(data.layerTextures[9 + 1] == 6, "Detail normal references its texture");
    Require(data.blenderMasks[0] == 5, "Blender mask references its texture");
    Require(data.blenderModes[0] == static_cast<int>(SFBlendMode::Skin), "Blender mode carries over");
    Require(data.blenderFlags[0] == ((1 << 3) | (1 << 4) | (1 << 6)), "Blender bools become flags");
    Require(data.blenderIntensities[0] == 0.5f, "Mask intensity carries over");
    Require(data.layerUVs[4] == 50.0f, "Detail UV scale carries over");
    Require(data.layerFlags[0] == 0 && data.layerFlags[1] == 1, "Color modes become flags");
    Require(data.layerColors[0] == Catch::Approx(0.5112f).epsilon(0.01), "Material color is decoded from sRGB");
    Require(data.sssStrength == 0.5f, "SSS strength carries over");
    Require(data.transmissiveLayer == -1, "Thick translucency has no transmissive layer");
    Require(data.HasAnyTexture(), "Material has textures");

    // A texture that couldn't be loaded falls back to the replacement when there is one and to nothing
    // otherwise, and case or slash differences don't make a texture a different one
    std::vector<std::string> loaded = {"TEXTURES\\Actors\\Human\\Naked_Body\\NakedBodyF_sk3_color.dds"};
    data = SFRenderData::Build(material, loaded);
    Require(data.layerTextures[0] == 1, "Texture lookup ignores case and slashes");
    Require(data.layerTextures[1] == 0 && data.blenderMasks[0] == 0, "Unloaded textures reference nothing");
    Require(data.layerTextures[8] == -1, "Replacement stays without a texture");
}

TEST_CASE("Starfield render data decodes replacements and CharacterCombine slots", "[SFLayeredMaterial]") {
    SFLayeredMaterial material;
    SFLayer& base = material.layers.emplace_back();
    base.textureSet.files[0] = "textures/base_color.dds";
    base.textureSet.replacementMask = (1u << 1);
    base.textureSet.replacements[1] = {0.5f, 1.0f, 1.0f, 1.0f};

    SFLayer& overlay = material.layers.emplace_back();
    overlay.textureSet.files[0] = "textures/ignored_color.dds";
    overlay.textureSet.files[14] = "textures/overlay_color.dds";

    SFBlender& blender = material.blenders.emplace_back();
    blender.mode = SFBlendMode::CharacterCombine;

    const std::vector<std::string> textures = SFRenderData::CollectTextures(material);
    Require(textures.size() == 2 && textures[1] == "textures/overlay_color.dds", "CharacterCombine layers read the overlay color slot");

    const SFRenderData data = SFRenderData::Build(material, textures);
    Require(data.layerTextures[1] == -1, "Normal uses its replacement");
    Require(data.layerReplacements[1 * 4 + 0] == Catch::Approx(0.0f) && data.layerReplacements[1 * 4 + 1] == Catch::Approx(1.0f), "Normal replacement is decoded as signed");
    Require(data.layerTextures[9 + 0] == 2, "Overlay color references its texture");
    Require(data.layerTextures[9 + 3] == -1 && data.layerReplacements[(9 + 3) * 4] == 0.5f, "Missing overlay roughness multiplies by one");
}
