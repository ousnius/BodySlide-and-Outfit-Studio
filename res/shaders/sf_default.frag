#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

// Starfield layered materials. A material stacks up to six layers - each a texture set with its own
// tint and UV stream - and blender n mixes layer n + 1 onto what the layers below it made, under a
// mask. The layering follows NifSkope's Starfield shader (fo76utils/nifskope, stf_default.frag),
// which is the closest anyone has come to the game's own; the lighting is the metallic-roughness
// model the True PBR shader uses, so both games look at home under the same HDRi.
//
// Everything about the material comes in through the sf* uniforms, which SFRenderData lays out on
// the CPU side. A texture reference there is 0 for nothing, -1 for the slot's replacement color
// (already decoded to what the texture would sample as) and n for textureUnits[n - 1].
//
// Not rendered: parallax from height maps, edge falloff, the detail blend mask and flipbooks.

// The sampler array takes every texture unit except the one the environment cube map sits on
#if MAX_TEXTURE_UNITS >= 32
#define SF_NUM_TEXTURES 31
#else
#define SF_NUM_TEXTURES 15
#endif

#define SF_MAX_LAYERS 6
#define SF_MAX_BLENDERS 5
#define SF_SLOTS_PER_LAYER 9

// See SFRenderData::Flag*
#define FLAG_EFFECT 1
#define FLAG_OPACITY_COMPONENT 2
#define FLAG_GLASS 4
#define FLAG_EFFECT_VERTEX_COLOR 8
#define FLAG_HAS_OPACITY 16
#define FLAG_ALPHA_VERTEX_COLOR 32
#define FLAG_DECAL 64
#define FLAG_ALPHA_TEST 128
#define FLAG_ALPHA_BLEND 256
#define FLAG_ALPHA_UV_CHANNEL_TWO 512
#define FLAG_SOFT_ADDITIVE 4096

uniform sampler2D textureUnits[SF_NUM_TEXTURES];
// Which of the units went up in an sRGB or a signed format, one bit per unit. An sRGB texture
// samples as linear already and a signed normal map as -1 to 1 already.
uniform int sfSRGBMask;
uniform int sfSignedMask;

uniform int sfNumLayers;
uniform int sfLayerTextures[SF_MAX_LAYERS * SF_SLOTS_PER_LAYER];
uniform vec4 sfLayerReplacements[SF_MAX_LAYERS * SF_SLOTS_PER_LAYER];
uniform vec4 sfLayerColors[SF_MAX_LAYERS];
// Scale in xy, offset in zw
uniform vec4 sfLayerUVs[SF_MAX_LAYERS];
// Bit 0: lerp the color instead of multiplying, bit 1: vertex color as the tint, bit 2: second UV channel
uniform int sfLayerFlags[SF_MAX_LAYERS];
uniform float sfLayerNormalScales[SF_MAX_LAYERS];

uniform int sfBlenderMasks[SF_MAX_BLENDERS];
uniform vec4 sfBlenderMaskReplacements[SF_MAX_BLENDERS];
uniform vec4 sfBlenderUVs[SF_MAX_BLENDERS];
// 0 = Linear, 1 = Additive, 2 = PositionContrast, 3 = None, 4 = CharacterCombine, 5 = Skin
uniform int sfBlenderModes[SF_MAX_BLENDERS];
uniform int sfBlenderChannels[SF_MAX_BLENDERS];
// Bit 0: blend color, 1: metalness, 2: roughness, 3: normals, 4: normals additively, 5: vertex color
// mask, 6: AO, 8: second UV channel
uniform int sfBlenderFlags[SF_MAX_BLENDERS];
// Height blend threshold, height blend factor, position, 1.0 - contrast
uniform vec4 sfBlenderParams[SF_MAX_BLENDERS];
uniform float sfBlenderIntensities[SF_MAX_BLENDERS];

uniform int sfFlags;
uniform float sfAlphaThreshold;
uniform int sfAlphaSourceLayer;
uniform int sfAlphaVertexColorChannel;
uniform vec4 sfAlphaUV;
uniform float sfMaterialAlpha;
uniform int sfOpacityLayers[3];
uniform int sfOpacityBlendModes[2];

uniform int sfEmissiveLayers[3];
uniform int sfEmissiveMasks[3];
uniform vec4 sfEmissiveTints[3];
uniform float sfEmissiveIntensity;

uniform int sfTransmissiveLayer;
uniform float sfTransmissiveScale;
uniform float sfSSSStrength;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;

uniform mat3 mv_normalMatrix;

struct Properties
{
	float alpha;
};
uniform Properties prop;
uniform float alphaThreshold;

in vec3 lightFrontal;
in vec3 lightDirectional0;
in vec3 lightDirectional1;
in vec3 lightDirectional2;

in vec3 viewDir;
in vec3 n;
in mat3 mv_tbn;

in float maskFactor;
in vec3 weightColor;

in vec4 vColor;
in vec4 vUV;

out vec4 fragColor;

#include "pbr_common.glsl"

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

// GLSL 3.30 only indexes a sampler array with a constant, so a texture reference picks its sampler
// here. The reference is a uniform, which keeps the branch the same across the whole draw.
vec4 sampleUnit(in int unit, in vec2 uv)
{
	switch (unit)
	{
		case 0: return texture(textureUnits[0], uv);
		case 1: return texture(textureUnits[1], uv);
		case 2: return texture(textureUnits[2], uv);
		case 3: return texture(textureUnits[3], uv);
		case 4: return texture(textureUnits[4], uv);
		case 5: return texture(textureUnits[5], uv);
		case 6: return texture(textureUnits[6], uv);
		case 7: return texture(textureUnits[7], uv);
		case 8: return texture(textureUnits[8], uv);
		case 9: return texture(textureUnits[9], uv);
		case 10: return texture(textureUnits[10], uv);
		case 11: return texture(textureUnits[11], uv);
		case 12: return texture(textureUnits[12], uv);
		case 13: return texture(textureUnits[13], uv);
		case 14: return texture(textureUnits[14], uv);
#if SF_NUM_TEXTURES > 15
		case 15: return texture(textureUnits[15], uv);
		case 16: return texture(textureUnits[16], uv);
		case 17: return texture(textureUnits[17], uv);
		case 18: return texture(textureUnits[18], uv);
		case 19: return texture(textureUnits[19], uv);
		case 20: return texture(textureUnits[20], uv);
		case 21: return texture(textureUnits[21], uv);
		case 22: return texture(textureUnits[22], uv);
		case 23: return texture(textureUnits[23], uv);
		case 24: return texture(textureUnits[24], uv);
		case 25: return texture(textureUnits[25], uv);
		case 26: return texture(textureUnits[26], uv);
		case 27: return texture(textureUnits[27], uv);
		case 28: return texture(textureUnits[28], uv);
		case 29: return texture(textureUnits[29], uv);
		case 30: return texture(textureUnits[30], uv);
#endif
	}

	return vec4(1.0);
}

vec2 streamUV(in vec4 scaleOffset, in bool channelTwo)
{
	return (channelTwo ? vUV.zw : vUV.xy) * scaleOffset.xy + scaleOffset.zw;
}

bool hasSlot(in int layer, in int slot)
{
	return sfLayerTextures[layer * SF_SLOTS_PER_LAYER + slot] != 0;
}

// A layer's texture slot, brought into the space the layering works in: linear color for the color
// and emissive maps, a -1 to 1 vector for the normal map, the raw values for everything else
vec4 layerTexture(in int layer, in int slot, in vec2 uv)
{
	int index = layer * SF_SLOTS_PER_LAYER + slot;
	int ref = sfLayerTextures[index];
	if (ref < 0)
		return sfLayerReplacements[index];

	int unit = ref - 1;
	vec4 value = sampleUnit(unit, uv);

	if (slot == 0 || slot == 7)
	{
		if ((sfSRGBMask & (1 << unit)) == 0)
			value.rgb = srgbToLinear(value.rgb);
	}
	else if (slot == 1)
	{
		if ((sfSignedMask & (1 << unit)) == 0)
			value.rg = value.rg * 2.0 - 1.0;
	}

	return value;
}

float blenderMask(in int blender)
{
	float mask = 1.0;
	int ref = sfBlenderMasks[blender];
	if (ref < 0)
		mask = sfBlenderMaskReplacements[blender].r;
	else if (ref > 0)
		mask = sampleUnit(ref - 1, streamUV(sfBlenderUVs[blender], (sfBlenderFlags[blender] & 256) != 0)).r;

	if ((sfBlenderFlags[blender] & 32) != 0)
		mask *= vColor[sfBlenderChannels[blender]];

	return mask * sfBlenderIntensities[blender];
}

// Light wrapping a little way past the terminator, which is how much of the look of subsurface
// scattering a single pass can give skin. Only the part beyond plain Lambert is returned, tinted the
// way light comes back out of skin: red travels the furthest.
vec3 skinScatter(in DirectionalLight light, in vec3 lightDir)
{
	float NdotL = dot(normal, lightDir);
	float wrap = sfSSSStrength * 0.5;
	float wrapped = max((NdotL + wrap) / (1.0 + wrap), 0.0);
	return (wrapped - max(NdotL, 0.0)) * vec3(1.0, 0.45, 0.3) * light.diffuse;
}

// What a thin translucent surface lets through from a light behind it
vec3 transmittedLight(in DirectionalLight light, in vec3 lightDir)
{
	return max(-dot(normal, lightDir), 0.0) * light.diffuse;
}

void main(void)
{
	vec4 color = vColor;
	float outAlpha = 1.0;

	// Without textures the shape keeps its mesh color, which the vertex stage put in vColor
	vec3 albedo = vColor.rgb;

	if (!bWireframe)
	{
		vec3 baseMap = vColor.rgb;
		vec3 normalTS = vec3(0.0, 0.0, 1.0);
		// Roughness, metalness, AO
		vec3 pbrMap = vec3(0.0, 0.0, 1.0);
		vec3 emissive = vec3(0.0);
		vec3 transmissive = vec3(0.0);

		bool isEffect = (sfFlags & FLAG_EFFECT) != 0;
		bool isGlass = (sfFlags & FLAG_GLASS) != 0;
		float baseAlpha = vColor.a;
		float alpha = 1.0;

		if (bShowTexture)
		{
			for (int i = 0; i < sfNumLayers; i++)
			{
				vec3 layerBase = vec3(0.0);
				vec3 layerNormal = vec3(0.0, 0.0, 1.0);
				vec3 layerPBR = vec3(0.0, 0.0, 1.0);

				int blendMode = i > 0 ? sfBlenderModes[i - 1] : 3;
				vec2 uv = streamUV(sfLayerUVs[i], (sfLayerFlags[i] & 4) != 0);

				if (hasSlot(i, 0))
					layerBase = layerTexture(i, 0, uv).rgb;

				vec4 tint = (sfLayerFlags[i] & 2) == 0 ? sfLayerColors[i] : vColor;
				if ((sfLayerFlags[i] & 1) == 0)
					layerBase *= tint.rgb;
				else
					layerBase = mix(layerBase, tint.rgb, tint.a);

				if (hasSlot(i, 1))
				{
					layerNormal.rg = layerTexture(i, 1, uv).rg * sfLayerNormalScales[i];
					layerNormal.b = sqrt(max(1.0 - dot(layerNormal.rg, layerNormal.rg), 0.0));
				}

				if (hasSlot(i, 3))
					layerPBR.r = layerTexture(i, 3, uv).r;
				if (hasSlot(i, 4))
					layerPBR.g = layerTexture(i, 4, uv).r;
				if (hasSlot(i, 5))
					layerPBR.b = layerTexture(i, 5, uv).r;

				float layerMask = 1.0;
				if (i == 0)
				{
					// A decal without a color map has nothing to put down
					if ((sfFlags & FLAG_DECAL) != 0 && !hasSlot(0, 0))
						discard;

					baseMap = layerBase;
					normalTS = layerNormal;
					pbrMap = layerPBR;
					baseAlpha = 1.0;
				}
				else
				{
					layerMask = blenderMask(i - 1);

					if (blendMode != 3 && !(isEffect && isGlass))
					{
						int blenderFlags = sfBlenderFlags[i - 1];
						float srcMask = layerMask;

						if (blendMode == 2)
						{
							// PositionContrast: the mask becomes a ramp around a position
							float blendPosition = sfBlenderParams[i - 1].z;
							float blendContrast = max(sfBlenderParams[i - 1].w * min(blendPosition, 1.0 - blendPosition), 0.001);
							blendPosition = (blendPosition - 0.5) * 3.17;
							blendPosition = (blendPosition * blendPosition + 1.0) * blendPosition + 0.5;
							float maskMin = blendPosition - blendContrast;
							float maskMax = blendPosition + blendContrast;
							srcMask = (srcMask - maskMin) / (maskMax - maskMin);
						}
						else if (blendMode == 4)
						{
							// CharacterCombine: color and roughness multiply (0.5 leaves them as they
							// were), metalness overlays
							layerBase = layerBase * baseMap * 2.0;
							layerPBR.r = layerPBR.r * pbrMap.r * 2.0;
							if (layerPBR.g < 0.5)
								layerPBR.g = layerPBR.g * pbrMap.g;
							else
								layerPBR.g = layerPBR.g + pbrMap.g - layerPBR.g * pbrMap.g;
						}
						else if (blendMode == 1)
						{
							layerBase += baseMap;
							layerPBR += pbrMap;
							if ((blenderFlags & 16) == 0)
								layerNormal += normalTS;
						}

						srcMask = clamp(srcMask, 0.0, 1.0);

						if ((blenderFlags & 1) != 0)
							baseMap = min(mix(baseMap, layerBase, srcMask), vec3(1.0));

						layerPBR = min(mix(pbrMap, layerPBR, srcMask), vec3(1.0));
						if ((blenderFlags & 2) != 0)
							pbrMap.g = layerPBR.g;
						if ((blenderFlags & 4) != 0)
							pbrMap.r = layerPBR.r;

						if ((blenderFlags & 8) != 0)
						{
							if ((blenderFlags & 16) != 0)
							{
								// Detail normals add their slope onto the ones below
								normalTS.rg = normalTS.rg + layerNormal.rg * srcMask;
								normalTS.b = sqrt(max(1.0 - dot(normalTS.rg, normalTS.rg), 0.0));
							}
							else
							{
								normalTS = normalize(mix(normalTS, layerNormal, srcMask));
							}
						}

						if ((blenderFlags & 64) != 0)
							pbrMap.b = layerPBR.b;
					}
				}

				if (hasSlot(i, 2))
				{
					if (isEffect)
					{
						float a = layerTexture(i, 2, uv).r;
						if ((sfFlags & FLAG_OPACITY_COMPONENT) != 0)
						{
							int opacityBlendMode = -1;
							if (i == sfOpacityLayers[0])
								baseAlpha = a;
							else if (!isGlass && i == sfOpacityLayers[1])
								opacityBlendMode = sfOpacityBlendModes[0];
							else if (!isGlass && i == sfOpacityLayers[2])
								opacityBlendMode = sfOpacityBlendModes[1];

							if (opacityBlendMode == 0)
								baseAlpha = mix(baseAlpha, a, layerMask);
							else if (opacityBlendMode == 1)
								baseAlpha = min(baseAlpha + a * layerMask, 1.0);
							else if (opacityBlendMode == 2)
								baseAlpha = max(baseAlpha - a * layerMask, 0.0);
							else if (opacityBlendMode == 3)
								baseAlpha *= a * layerMask;
						}
						else if (i == 0)
						{
							baseAlpha = a;
						}
					}
					else if ((sfFlags & FLAG_HAS_OPACITY) != 0 && i == sfAlphaSourceLayer)
					{
						baseAlpha = layerTexture(i, 2, streamUV(sfAlphaUV, (sfFlags & FLAG_ALPHA_UV_CHANNEL_TWO) != 0)).r;
					}
				}

				if (hasSlot(i, 7))
				{
					for (int e = 0; e < 3; e++)
					{
						if (sfEmissiveLayers[e] != i)
							continue;

						vec4 emissiveTint = sfEmissiveTints[e];
						int maskBlender = sfEmissiveMasks[e];
						if (maskBlender > 0 && maskBlender < sfNumLayers)
							emissiveTint.a *= blenderMask(maskBlender - 1);

						emissive += layerTexture(i, 7, uv).rgb * emissiveTint.rgb * emissiveTint.a;
						break;
					}
				}

				if (hasSlot(i, 8) && i == sfTransmissiveLayer)
					transmissive = vec3(layerTexture(i, 8, uv).r * sfTransmissiveScale);
			}

			if (isEffect)
			{
				if ((sfFlags & FLAG_EFFECT_VERTEX_COLOR) != 0)
				{
					baseMap *= vColor.rgb;
					baseAlpha *= vColor.a;
				}

				// Effects ignore their own alpha test settings and cut at 1/128
				alpha = alpha * sfMaterialAlpha * baseAlpha;
				if (!(alpha > 0.0078))
					discard;

				if ((sfFlags & FLAG_SOFT_ADDITIVE) != 0)
					baseMap *= alpha;
			}
			else
			{
				if ((sfFlags & FLAG_DECAL) != 0)
					alpha = sfMaterialAlpha;

				if ((sfFlags & FLAG_HAS_OPACITY) != 0 && (sfFlags & FLAG_ALPHA_VERTEX_COLOR) != 0)
					alpha *= vColor[sfAlphaVertexColorChannel];

				alpha *= baseAlpha;
				if ((sfFlags & FLAG_ALPHA_TEST) != 0 && !(alpha > sfAlphaThreshold))
					discard;
			}

			if ((sfFlags & FLAG_ALPHA_BLEND) != 0)
				outAlpha = alpha;

			albedo = baseMap;
		}

		// What the shape looks like with the lighting off: the base color on its own
		color.rgb = albedo;

		if (bLightEnabled)
		{
			vec3 outDiffuse = vec3(0.0);
			vec3 outSpecular = vec3(0.0);

			if (bShowTexture)
			{
				// The back of a two sided surface faces the other way
				if (!gl_FrontFacing)
					normalTS.z = -normalTS.z;

				normal = normalize(mv_tbn * normalTS);

				roughness = clamp(pbrMap.r, MIN_ROUGHNESS, 1.0);
				ao = clamp(pbrMap.b, 0.0, 1.0);

				// Metal reflects its own color and has no diffuse of its own
				float metallic = clamp(pbrMap.g, 0.0, 1.0);
				f0 = mix(vec3(0.04), albedo, metallic);
				diffuseColor = albedo * (1.0 - metallic);
			}
			else
			{
				// Vertex normal and a plain dielectric for shading with disabled maps
				normal = normalize(mv_normalMatrix * n);
				if (!gl_FrontFacing)
					normal = -normal;

				roughness = 0.6;
				ao = 1.0;
				f0 = vec3(0.04);
				diffuseColor = albedo;
			}

			directionalLight(frontal, lightFrontal, outDiffuse, outSpecular);
			directionalLight(directional0, lightDirectional0, outDiffuse, outSpecular);
			directionalLight(directional1, lightDirectional1, outDiffuse, outSpecular);
			directionalLight(directional2, lightDirectional2, outDiffuse, outSpecular);

			if (bShowTexture && sfSSSStrength > 0.0)
			{
				outDiffuse += skinScatter(frontal, lightFrontal);
				outDiffuse += skinScatter(directional0, lightDirectional0);
				outDiffuse += skinScatter(directional1, lightDirectional1);
				outDiffuse += skinScatter(directional2, lightDirectional2);
			}

			imageBasedLight(outDiffuse, outSpecular);

			color.rgb = diffuseColor * outDiffuse + outSpecular;

			if (bShowTexture)
			{
				color.rgb += emissive * sfEmissiveIntensity;

				if (sfTransmissiveLayer >= 0)
				{
					vec3 transmitted = transmittedLight(frontal, lightFrontal) + transmittedLight(directional0, lightDirectional0)
									 + transmittedLight(directional1, lightDirectional1) + transmittedLight(directional2, lightDirectional2);
					color.rgb += transmissive * albedo * ao * (transmitted + vec3(ambient));
				}
			}
		}

		// Tone mapped and encoded in one step, see sk_truepbr.frag
		color.rgb = linearToSrgb(tonemap(color.rgb) / tonemap(vec3(1.0)));
		color.a = outAlpha;

		// Interface tints go on after the encode, as in sk_truepbr.frag
		if (bShowMask)
		{
			color.rgb *= maskFactor;
		}

		if (bShowWeight)
		{
			color.rgb *= weightColor;
		}
	}
	else
	{
		color = vec4(color.rgb, 0.5);
	}

	color = clamp(color, 0.0, 1.0);

	fragColor = color;

	if (!bWireframe)
	{
		if (alphaThreshold != -1.0f)
			if (fragColor.a <= alphaThreshold) // GL_GREATER
				discard;

		fragColor.a *= prop.alpha;
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		// Minimal depth offset for wireframe to prevent z-fighting with its own mesh
		gl_FragDepth = gl_FragCoord.z - 0.00001f;
	}
}
