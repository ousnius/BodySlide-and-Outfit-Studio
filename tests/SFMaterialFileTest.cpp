#include "../src/files/SFMaterialFile.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    INFO(message);
    REQUIRE(condition);
}
}

TEST_CASE("Starfield material files resolve texture paths", "[SFMaterialFile]") {
    const char* materialJson = R"json(
{
    "Objects": [
        {
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": {
                        "FileName": "textures/clothes/suit_color.dds"
                    }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": {
                        "FileName": "textures/clothes/suit_normal.dds"
                    }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": {
                        "FileName": "textures/clothes/replacement_normal.dds"
                    }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 3,
                    "Data": {
                        "FileName": "textures/clothes/suit_rough.dds"
                    }
                }
            ]
        }
    ]
}
)json";

    std::istringstream input(materialJson);
    SFMaterialFile material(input);

    Require(!material.Failed(), "Valid Starfield material should parse");
    Require(material.GetTexture(SFMaterialTextureSlot::Color) == "textures/clothes/suit_color.dds", "Color texture should parse");
    Require(material.GetTexture(SFMaterialTextureSlot::Normal) == "textures/clothes/suit_normal.dds", "First normal texture should win");
    Require(material.GetTexture(SFMaterialTextureSlot::Roughness) == "textures/clothes/suit_rough.dds", "Roughness texture should parse");
    Require(material.GetTexture(SFMaterialTextureSlot::Metalness).empty(), "Missing metalness slot should be empty");

    std::vector<std::string> textures = material.GetTextureFiles(10);
    Require(textures.size() == 10, "Texture file list should preserve requested length");
    Require(textures[0] == "textures/clothes/suit_color.dds", "Texture file list should preserve color slot");
    Require(textures[1] == "textures/clothes/suit_normal.dds", "Texture file list should preserve normal slot");
    Require(textures[2].empty(), "Texture file list should preserve empty opacity slot");
    Require(textures[3] == "textures/clothes/suit_rough.dds", "Texture file list should preserve roughness slot");
    Require(textures[9].empty(), "Texture file list should preserve trailing empty slots");

    const char* layeredMaterialJson = R"json(
{
    "Objects": [
        {
            "Parent": "materials\\layered\\root\\layeredmaterials.mat",
            "Components": [
                {
                    "Type": "BSComponentDB::CTName",
                    "Index": 0,
                    "Data": { "Name": "naked_f_body" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 1,
                    "Data": { "ID": "res:SECONDARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::BlenderID",
                    "Index": 0,
                    "Data": { "ID": "res:MASK_BLENDER" }
                },
                {
                    "Type": "BSMaterial::VeryLowTextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:VERYLOW_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:SECONDARY_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:SECONDARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:SECONDARY_MATERIAL" }
                }
            ]
        },
        {
            "ID": "res:MASK_BLENDER",
            "Parent": "materials\\layered\\root\\blenders.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_mask.dds" }
                }
            ]
        },
        {
            "ID": "res:VERYLOW_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_verylow_color.dds" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_normal.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 3,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_rough.dds" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 5,
                    "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_ao.dds" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:PRIMARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:PRIMARY_MATERIAL" }
                }
            ]
        }
    ]
}
)json";

    std::istringstream layeredInput(layeredMaterialJson);
    SFMaterialFile layeredMaterial(layeredInput);
    Require(!layeredMaterial.Failed(), "Layered Starfield material should parse");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Color) == "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds", "Primary layer color texture should win");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Normal) == "textures/actors/human/naked_body/nakedbodyf_normal.dds", "Primary layer normal texture should win");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Roughness) == "textures/actors/human/naked_body/nakedbodyf_rough.dds", "Primary layer roughness texture should win");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::AmbientOcclusion) == "textures/actors/human/naked_body/nakedbodyf_ao.dds", "Primary layer AO texture should win");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Normal) != "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds", "Secondary face detail normal must not be primary normal");
    Require(layeredMaterial.GetTexture(SFMaterialTextureSlot::Color) != "textures/actors/human/naked_body/nakedbodyf_mask.dds", "Blender mask must not be primary color");

    const char* hatMaterialJson = R"json(
{
    "Objects": [
        {
            "Parent": "materials\\layered\\root\\layeredmaterials.mat",
            "Components": [
                {
                    "Type": "BSComponentDB::CTName",
                    "Index": 0,
                    "Data": { "Name": "Outfit_Colonist_QuarterPaddedVest_01_Hat" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 1,
                    "Data": { "ID": "res:DENIM_SECONDARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 2,
                    "Data": { "ID": "res:LEATHER_SECONDARY_LAYER" }
                },
                {
                    "Type": "BSMaterial::BlenderID",
                    "Index": 0,
                    "Data": { "ID": "res:HAT_MASK_BLENDER" }
                },
                {
                    "Type": "BSMaterial::LayerID",
                    "Index": 0,
                    "Data": { "ID": "res:CEA22BEC:0006E3E9:A6D9E67F" }
                }
            ]
        },
        {
            "ID": "res:DENIM_SECONDARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Shared\\denim_color.dds" }
                }
            ]
        },
        {
            "ID": "res:LEATHER_SECONDARY_TEXTURESET",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Shared\\leather_detail_normal.dds" }
                }
            ]
        },
        {
            "ID": "res:HAT_MASK_BLENDER",
            "Parent": "materials\\layered\\root\\blenders.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_mask.dds" }
                }
            ]
        },
        {
            "ID": "res:12F23154:0006A7EC:A38E3CE2",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_verylow_color.dds" }
                }
            ]
        },
        {
            "ID": "res:2AC59295:00069284:A21B614A",
            "Parent": "materials\\layered\\root\\texturesets.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 0,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_color.DDS" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 1,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_normal.DDS" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 3,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_rough.DDS" }
                },
                {
                    "Type": "BSMaterial::MRTextureFile",
                    "Index": 5,
                    "Data": { "FileName": "Data\\Textures\\Clothes\\Outfit_Colonist_QuarterPaddedVest_01\\Outfit_Colonist_QuarterPaddedVest_01_Hat_ao.dds" }
                }
            ]
        },
        {
            "ID": "res:D691332E:0005F7C7:A7F82E21",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:2AC59295:00069284:A21B614A" }
                }
            ]
        },
        {
            "ID": "res:CEA22BEC:0006E3E9:A6D9E67F",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:D691332E:0005F7C7:A7F82E21" }
                }
            ]
        },
        {
            "ID": "res:DENIM_SECONDARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:DENIM_SECONDARY_MATERIAL" }
                }
            ]
        },
        {
            "ID": "res:DENIM_SECONDARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:DENIM_SECONDARY_TEXTURESET" }
                }
            ]
        },
        {
            "ID": "res:LEATHER_SECONDARY_LAYER",
            "Parent": "materials\\layered\\root\\layers.mat",
            "Components": [
                {
                    "Type": "BSMaterial::MaterialID",
                    "Index": 0,
                    "Data": { "ID": "res:LEATHER_SECONDARY_MATERIAL" }
                }
            ]
        },
        {
            "ID": "res:LEATHER_SECONDARY_MATERIAL",
            "Parent": "materials\\layered\\root\\materials.mat",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:LEATHER_SECONDARY_TEXTURESET" }
                }
            ]
        }
    ]
}
)json";

    std::istringstream hatInput(hatMaterialJson);
    SFMaterialFile hatMaterial(hatInput);
    Require(!hatMaterial.Failed(), "Hat Starfield material should parse");
    Require(hatMaterial.GetTexture(SFMaterialTextureSlot::Color) == "Textures/Clothes/Outfit_Colonist_QuarterPaddedVest_01/Outfit_Colonist_QuarterPaddedVest_01_Hat_color.DDS", "Hat color should strip Data prefix, use forward slashes, and preserve case");
    Require(hatMaterial.GetTexture(SFMaterialTextureSlot::Normal) == "Textures/Clothes/Outfit_Colonist_QuarterPaddedVest_01/Outfit_Colonist_QuarterPaddedVest_01_Hat_normal.DDS", "Hat normal should strip Data prefix, use forward slashes, and preserve case");
    Require(hatMaterial.GetTexture(SFMaterialTextureSlot::Roughness) == "Textures/Clothes/Outfit_Colonist_QuarterPaddedVest_01/Outfit_Colonist_QuarterPaddedVest_01_Hat_rough.DDS", "Hat roughness should strip Data prefix, use forward slashes, and preserve case");
    Require(hatMaterial.GetTexture(SFMaterialTextureSlot::AmbientOcclusion) == "Textures/Clothes/Outfit_Colonist_QuarterPaddedVest_01/Outfit_Colonist_QuarterPaddedVest_01_Hat_ao.dds", "Hat AO should strip Data prefix, use forward slashes, and preserve case");
    Require(hatMaterial.GetTexture(SFMaterialTextureSlot::Color) != "Textures/Clothes/Outfit_Colonist_QuarterPaddedVest_01/Outfit_Colonist_QuarterPaddedVest_01_Hat_mask.dds", "Hat blender mask must not be primary color");

    SFMaterialFile directReadMaterial;
    std::istringstream directReadInput(materialJson);
    Require(directReadMaterial.Read(directReadInput) == 0, "Direct Read should parse valid Starfield material");
    Require(!directReadMaterial.Failed(), "Direct Read should clear failed state after valid parse");
    Require(directReadMaterial.GetTexture(SFMaterialTextureSlot::Color) == "textures/clothes/suit_color.dds", "Direct Read should preserve parsed color texture");

    std::istringstream directReadMissingObjects("{}");
    Require(directReadMaterial.Read(directReadMissingObjects) != 0, "Direct Read should reject material without Objects array");
    Require(directReadMaterial.Failed(), "Direct Read should set failed state after invalid parse");
    Require(directReadMaterial.GetTexture(SFMaterialTextureSlot::Color).empty(), "Direct Read should clear textures before invalid parse");

    std::istringstream missingObjects("{}");
    SFMaterialFile invalidMaterial(missingObjects);
    Require(invalidMaterial.Failed(), "Material without Objects array should fail");

    const char* noResolvedTexturesJson = R"json(
{
    "Objects": [
        {
            "ID": "res:EMPTY_TEXTURESET",
            "Components": [
                {
                    "Type": "BSMaterial::TextureSetID",
                    "Index": 0,
                    "Data": { "ID": "res:MISSING_TEXTURESET" }
                }
            ]
        }
    ]
}
)json";

    std::istringstream noResolvedTexturesInput(noResolvedTexturesJson);
    Require(directReadMaterial.Read(noResolvedTexturesInput) != 0, "Direct Read should reject graph materials with no resolvable textures");
    Require(directReadMaterial.Failed(), "Direct Read should fail when graph resolution finds no textures");
}
