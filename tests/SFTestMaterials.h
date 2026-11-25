/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

// Starfield material files shared by the tests

// naked_f_body.mat as the "SF naked female" mod ships it, less the components the preview ignores
inline const char* NakedBodyMaterialJson = R"json(
{
    "Objects": [
        {
            "Components": [
                { "Data": { "ID": "res:7CE127DC:0005C766:A3FB3F95" }, "Index": 0, "Type": "BSMaterial::LayerID" },
                { "Data": { "ID": "res:D45AA7CD:00067030:A5072996" }, "Index": 1, "Type": "BSMaterial::LayerID" },
                { "Data": { "ID": "res:2693C065:0006BCAF:A2633E80" }, "Index": 0, "Type": "BSMaterial::BlenderID" },
                { "Data": { "Route": "Deferred" }, "Index": 0, "Type": "BSMaterial::ShaderRouteComponent" },
                { "Data": { "FileName": "BodySkin2Layer" }, "Index": 0, "Type": "BSMaterial::ShaderModelComponent" },
                {
                    "Data": {
                        "Enabled": "true",
                        "Settings": {
                            "Data": {
                                "SSSStrength": "0.5",
                                "SSSWidth": "0.025",
                                "Thin": "false",
                                "TransmissiveScale": "0.05",
                                "TransmittanceSourceLayer": "MATERIAL_LAYER_0",
                                "UseSSS": "true"
                            },
                            "Type": "BSMaterial::TranslucencySettings"
                        }
                    },
                    "Index": 0,
                    "Type": "BSMaterial::TranslucencySettingsComponent"
                }
            ],
            "Parent": "materials\\layered\\root\\layeredmaterials.mat"
        },
        {
            "Components": [
                { "Data": { "ID": "res:0ABB9492:0004FE99:A1919594" }, "Index": 0, "Type": "BSMaterial::MaterialID" },
                { "Data": { "ID": "res:AA432480:00076793:A17F3CE9" }, "Index": 0, "Type": "BSMaterial::UVStreamID" }
            ],
            "ID": "res:7CE127DC:0005C766:A3FB3F95",
            "Parent": "materials\\layered\\root\\layers.mat"
        },
        {
            "Components": [
                { "Data": { "ID": "res:94E790C6:0004005C:A3C6B5B3" }, "Index": 0, "Type": "BSMaterial::TextureSetID" },
                {
                    "Data": { "Value": { "Data": { "w": "0", "x": "0.737255", "y": "0.737255", "z": "0.737255" }, "Type": "XMFLOAT4" } },
                    "Index": 0,
                    "Type": "BSMaterial::Color"
                },
                { "Data": { "Value": "Multiply" }, "Index": 0, "Type": "BSMaterial::MaterialOverrideColorTypeComponent" },
                { "Data": { "Value": "false" }, "Index": 0, "Type": "BSMaterial::ParamBool" }
            ],
            "ID": "res:0ABB9492:0004FE99:A1919594",
            "Parent": "materials\\layered\\root\\materials.mat"
        },
        {
            "Components": [
                { "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_sk3_color.dds" }, "Index": 0, "Type": "BSMaterial::MRTextureFile" },
                { "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_normal.dds" }, "Index": 1, "Type": "BSMaterial::MRTextureFile" },
                { "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_rough.dds" }, "Index": 3, "Type": "BSMaterial::MRTextureFile" },
                { "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_ao.dds" }, "Index": 5, "Type": "BSMaterial::MRTextureFile" },
                {
                    "Data": {
                        "Color": { "Data": { "Value": { "Data": { "w": "1", "x": "0", "y": "0", "z": "0" }, "Type": "XMFLOAT4" } }, "Type": "BSMaterial::Color" },
                        "Enabled": "true"
                    },
                    "Index": 8,
                    "Type": "BSMaterial::TextureReplacement"
                },
                { "Data": { "Value": "1" }, "Index": 0, "Type": "BSMaterial::MaterialParamFloat" }
            ],
            "ID": "res:94E790C6:0004005C:A3C6B5B3",
            "Parent": "materials\\layered\\root\\texturesets.mat"
        },
        {
            "Components": [
                { "Data": { "Value": { "Data": { "x": "1", "y": "1" }, "Type": "XMFLOAT2" } }, "Index": 0, "Type": "BSMaterial::Scale" },
                { "Data": { "Value": { "Data": { "x": "0", "y": "0" }, "Type": "XMFLOAT2" } }, "Index": 0, "Type": "BSMaterial::Offset" },
                { "Data": { "Value": "Wrap" }, "Index": 0, "Type": "BSMaterial::TextureAddressModeComponent" },
                { "Data": { "Value": "One" }, "Index": 0, "Type": "BSMaterial::Channel" }
            ],
            "ID": "res:AA432480:00076793:A17F3CE9",
            "Parent": "materials\\layered\\root\\uvstreams.mat"
        },
        {
            "Components": [
                { "Data": { "ID": "res:FB09ADC7:000587C4:A5F5FF58" }, "Index": 0, "Type": "BSMaterial::MaterialID" },
                { "Data": { "ID": "res:D9B6E8D4:000442F6:A1EF7962" }, "Index": 0, "Type": "BSMaterial::UVStreamID" }
            ],
            "ID": "res:D45AA7CD:00067030:A5072996",
            "Parent": "materials\\layered\\root\\layers.mat"
        },
        {
            "Components": [
                { "Data": { "ID": "res:556E97A9:00078196:A2617396" }, "Index": 0, "Type": "BSMaterial::TextureSetID" },
                {
                    "Data": { "Value": { "Data": { "w": "0", "x": "1", "y": "1", "z": "1" }, "Type": "XMFLOAT4" } },
                    "Index": 0,
                    "Type": "BSMaterial::Color"
                },
                { "Data": { "Value": "Lerp" }, "Index": 0, "Type": "BSMaterial::MaterialOverrideColorTypeComponent" }
            ],
            "ID": "res:FB09ADC7:000587C4:A5F5FF58",
            "Parent": "materials\\layered\\root\\materials.mat"
        },
        {
            "Components": [
                { "Data": { "FileName": "textures/actors/human/faces/facedetails/young_cheek_detail_normal.dds" }, "Index": 1, "Type": "BSMaterial::MRTextureFile" },
                { "Data": { "Value": "1" }, "Index": 0, "Type": "BSMaterial::MaterialParamFloat" }
            ],
            "ID": "res:556E97A9:00078196:A2617396",
            "Parent": "materials\\layered\\root\\texturesets.mat"
        },
        {
            "Components": [
                { "Data": { "Value": { "Data": { "x": "50", "y": "50" }, "Type": "XMFLOAT2" } }, "Index": 0, "Type": "BSMaterial::Scale" },
                { "Data": { "Value": "One" }, "Index": 0, "Type": "BSMaterial::Channel" }
            ],
            "ID": "res:D9B6E8D4:000442F6:A1EF7962",
            "Parent": "materials\\layered\\root\\uvstreams.mat"
        },
        {
            "Components": [
                { "Data": { "ID": "res:81F3933B:000725D2:A2A07BDE" }, "Index": 0, "Type": "BSMaterial::UVStreamID" },
                { "Data": { "FileName": "textures/actors/human/naked_body/nakedbodyf_mask.dds" }, "Index": 0, "Type": "BSMaterial::MRTextureFile" },
                { "Data": { "Value": "Skin" }, "Index": 0, "Type": "BSMaterial::BlendModeComponent" },
                { "Data": { "Value": "0.5" }, "Index": 4, "Type": "BSMaterial::MaterialParamFloat" },
                { "Data": { "Value": "false" }, "Index": 0, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "false" }, "Index": 1, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "false" }, "Index": 2, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "true" }, "Index": 3, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "true" }, "Index": 4, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "false" }, "Index": 5, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "true" }, "Index": 6, "Type": "BSMaterial::ParamBool" },
                { "Data": { "Value": "false" }, "Index": 7, "Type": "BSMaterial::ParamBool" }
            ],
            "ID": "res:2693C065:0006BCAF:A2633E80",
            "Parent": "materials\\layered\\root\\blenders.mat"
        },
        {
            "Components": [
                { "Data": { "Value": { "Data": { "x": "1", "y": "1" }, "Type": "XMFLOAT2" } }, "Index": 0, "Type": "BSMaterial::Scale" }
            ],
            "ID": "res:81F3933B:000725D2:A2A07BDE",
            "Parent": "materials\\layered\\root\\uvstreams.mat"
        }
    ],
    "Version": 1
}
)json";
