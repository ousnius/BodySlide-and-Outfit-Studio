/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <vector>

struct ShaderFlagDef {
	const char* name;
	int bit;
};

inline std::vector<ShaderFlagDef> GetFO3ShaderFlags1() {
	return {
		{"Specular", 0}, {"Skinned", 1}, {"Low Detail", 2}, {"Vertex Alpha", 3},
		{"Unknown 1", 4}, {"Single Pass", 5}, {"Empty", 6}, {"Environment Mapping", 7},
		{"Alpha Texture", 8}, {"Unknown 2", 9}, {"FaceGen", 10}, {"Parallax", 11},
		{"Unknown 3", 12}, {"Non-Projective Shadows", 13}, {"Unknown 4", 14}, {"Refraction", 15},
		{"Fire Refraction", 16}, {"Eye Environment Mapping", 17}, {"Hair", 18}, {"Dynamic Alpha", 19},
		{"Localmap Hide Secret", 20}, {"Window Environment Mapping", 21}, {"Tree Billboard", 22}, {"Shadow Frustum", 23},
		{"Multiple Textures", 24}, {"Remappable Textures", 25}, {"Decal", 26}, {"Dynamic Decal", 27},
		{"Parallax Occlusion", 28}, {"External Emittance", 29}, {"Shadow Map", 30}, {"ZBuffer Test", 31}
	};
}

inline std::vector<ShaderFlagDef> GetFO3ShaderFlags2() {
	return {
		{"ZBuffer Write", 0}, {"LOD Landscape", 1}, {"LOD Building", 2}, {"No Fade", 3},
		{"Refraction Tint", 4}, {"Vertex Colors", 5}, {"Unknown 1", 6}, {"1st Point Light", 7},
		{"2nd Light", 8}, {"3rd Light", 9}, {"Vertex Lighting", 10}, {"Uniform Scale", 11},
		{"Fit Slope", 12}, {"Billboard Envmap Light Fade", 13}, {"No LOD Land Blend", 14}, {"Envmap Light Fade", 15},
		{"Wireframe", 16}, {"VATS Selection", 17}, {"Show in Local Map", 18}, {"Premult Alpha", 19},
		{"Skip Normal Maps", 20}, {"Alpha Decal", 21}, {"No Transparency Multisampling", 22}, {"Unknown 2", 23},
		{"Unknown 3", 24}, {"Unknown 4", 25}, {"Unknown 5", 26}, {"Unknown 6", 27},
		{"Unknown 7", 28}, {"Unknown 8", 29}, {"Unknown 9", 30}, {"Unknown 10", 31}
	};
}

inline std::vector<ShaderFlagDef> GetFO4ShaderFlags1() {
	return {
		{"Specular", 0}, {"Skinned", 1}, {"Temp Refraction", 2}, {"Vertex Alpha", 3},
		{"GreyscaleToPalette Color", 4}, {"GreyscaleToPalette Alpha", 5}, {"Use Falloff", 6}, {"Environment Mapping", 7},
		{"RGB Falloff", 8}, {"Cast Shadows", 9}, {"Face", 10}, {"UI Mask Rects", 11},
		{"Model Space Normals", 12}, {"Non-Projective Shadows", 13}, {"Landscape", 14}, {"Refraction", 15},
		{"Fire Refraction", 16}, {"Eye Environment Mapping", 17}, {"Hair", 18}, {"Screendoor Alpha Fade", 19},
		{"Localmap Hide Secret", 20}, {"Skin Tint", 21}, {"Own Emit", 22}, {"Projected UV", 23},
		{"Multiple Textures", 24}, {"Tessellate", 25}, {"Decal", 26}, {"Dynamic Decal", 27},
		{"Character Lighting", 28}, {"External Emittance", 29}, {"Soft Effect", 30}, {"ZBuffer Test", 31}
	};
}

inline std::vector<ShaderFlagDef> GetFO4ShaderFlags2() {
	return {
		{"ZBuffer Write", 0}, {"LOD Landscape", 1}, {"LOD Objects", 2}, {"No Fade", 3},
		{"Double Sided", 4}, {"Vertex Colors", 5}, {"Glow Map", 6}, {"Transform Changed", 7},
		{"Dismemberment Meatcuff", 8}, {"Tint", 9}, {"Grass Vertex Lighting", 10}, {"Grass Uniform Scale", 11},
		{"Grass Fit Slope", 12}, {"Grass Billboard", 13}, {"No LOD Land Blend", 14}, {"Dismemberment", 15},
		{"Wireframe", 16}, {"Weapon Blood", 17}, {"Hide On Local Map", 18}, {"Premult Alpha", 19},
		{"VATS Target", 20}, {"Anisotropic Lighting", 21}, {"Skew Specular Alpha", 22}, {"Menu Screen", 23},
		{"Multi Layer Parallax", 24}, {"Alpha Test", 25}, {"Gradient Remap", 26}, {"VATS Target Draw All", 27},
		{"Pipboy Screen", 28}, {"Tree Anim", 29}, {"Effect Lighting", 30}, {"Refraction Writes Depth", 31}
	};
}

inline std::vector<ShaderFlagDef> GetSkyrimShaderFlags1() {
	return {
		{"Specular", 0}, {"Skinned", 1}, {"Temp Refraction", 2}, {"Vertex Alpha", 3},
		{"GreyscaleToPalette Color", 4}, {"GreyscaleToPalette Alpha", 5}, {"Use Falloff", 6}, {"Environment Mapping", 7},
		{"Receive Shadows", 8}, {"Cast Shadows", 9}, {"Facegen Detail Map", 10}, {"Parallax", 11},
		{"Model Space Normals", 12}, {"Non-Projective Shadows", 13}, {"Landscape", 14}, {"Refraction", 15},
		{"Fire Refraction", 16}, {"Eye Environment Mapping", 17}, {"Hair Soft Lighting", 18}, {"Screendoor Alpha Fade", 19},
		{"Localmap Hide Secret", 20}, {"FaceGen RGB Tint", 21}, {"Own Emit", 22}, {"Projected UV", 23},
		{"Multiple Textures", 24}, {"Remappable Textures", 25}, {"Decal", 26}, {"Dynamic Decal", 27},
		{"Parallax Occlusion", 28}, {"External Emittance", 29}, {"Soft Effect", 30}, {"ZBuffer Test", 31}
	};
}

inline std::vector<ShaderFlagDef> GetSkyrimShaderFlags2() {
	return {
		{"ZBuffer Write", 0}, {"LOD Landscape", 1}, {"LOD Objects", 2}, {"No Fade", 3},
		{"Double Sided", 4}, {"Vertex Colors", 5}, {"Glow Map", 6}, {"Assume Shadowmask", 7},
		{"Packed Tangent", 8}, {"Multi Index Snow", 9}, {"Vertex Lighting", 10}, {"Uniform Scale", 11},
		{"Fit Slope", 12}, {"Billboard", 13}, {"No LOD Land Blend", 14}, {"EnvMap Light Fade", 15},
		{"Wireframe", 16}, {"Weapon Blood", 17}, {"Hide On Local Map", 18}, {"Premult Alpha", 19},
		{"Cloud LOD", 20}, {"Anisotropic Lighting", 21}, {"No Transparency Multisampling", 22}, {"Unused 01", 23},
		{"Multi Layer Parallax", 24}, {"Soft Lighting", 25}, {"Rim Lighting", 26}, {"Back Lighting", 27},
		{"Unused 02", 28}, {"Tree Anim", 29}, {"Effect Lighting", 30}, {"HD LOD Objects", 31}
	};
}
