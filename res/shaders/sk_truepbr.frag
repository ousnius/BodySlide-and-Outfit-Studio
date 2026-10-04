#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

// Community Shaders "True PBR" for Skyrim. A shape reaches this shader by carrying Shader Flags 2
// bit 23, which NifSkope calls "Unused 01"; see GLSurface::AddMeshFromNif. Its texture slots do not
// mean what they do in vanilla: slot 5 holds an RMAOS map rather than an environment mask, slot 2 an
// emissive color rather than a glow map, and slot 4 is left empty, so the only thing there is to
// reflect is the cube map generated from the HDRi.
//
// Unlike the other mesh shaders this one lights in linear space - the sRGB maps are decoded when
// they are sampled and the frame is encoded on the way out. A metallic-roughness BRDF only balances
// if its energy is in the right space; skipping the conversion leaves metal that never darkens and
// roughness that barely reads.

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
// Emissive color, texture slot 2. Vanilla puts a glow map there, so it goes by its own sampler name.
uniform sampler2D texEmissive;
// Roughness in red, metalness in green, ambient occlusion in blue and the specular level in alpha,
// texture slot 5. Vanilla puts the environment mask there, hence the separate name again.
uniform sampler2D texRMAOS;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;

uniform bool bNormalMap;
uniform bool bRMAOS;
uniform bool bEmissive;
uniform bool bPBREmissive;
// Whether the base color and emissive maps were uploaded in an sRGB format (a DDS saved as _SRGB),
// which the GPU decodes as it samples. Decoding those again here would square away every dark color.
uniform bool bDiffuseSRGB;
uniform bool bEmissiveSRGB;

uniform mat3 mv_normalMatrix;

struct Properties
{
	vec2 uvOffset;
	vec2 uvScale;
	// Specular Level under True PBR: the reflectance of the surface where it isn't metal. Scales the
	// alpha of the RMAOS map. 0.04 describes most materials, 0.08 a specular map authored for Unreal.
	float shininess;
	// Roughness Scale under True PBR: scales the red channel of the RMAOS map. Usually left at 1.
	float specularStrength;
	vec3 emissiveColor;
	float emissiveMultiple;
	float alpha;
};
// The skin and hair tint colors have no entry here: True PBR needs the default or the multi layer
// parallax shader type, and a tint only ever comes from the skin tint or hair tint types.
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
in vec2 vUV;

out vec4 fragColor;

#include "pbr_common.glsl"

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

vec2 uv = vec2(0.0);
vec3 albedo = vec3(0.0);

void readMaterial(void)
{
	if (!bShowTexture)
	{
		// No maps to read a material out of, so the shape is shaded as a plain dielectric rather than
		// as the fully rough metal an all-white RMAOS would otherwise describe.
		roughness = 0.6;
		ao = 1.0;
		f0 = vec3(0.04);
		diffuseColor = albedo;
		return;
	}

	// A slot the shape didn't fill reads as white, the same default Community Shaders substitutes.
	// That describes a fully rough, fully metallic surface, which renders as a dull grey - a shape
	// that looks like that is a shape whose RMAOS map is missing.
	vec4 rmaos = vec4(1.0);
	if (bRMAOS)
		rmaos = texture(texRMAOS, uv);

	roughness = clamp(rmaos.r * prop.specularStrength, MIN_ROUGHNESS, 1.0);
	ao = clamp(rmaos.b, 0.0, 1.0);

	// Metal reflects its own color and has no diffuse of its own, so the albedo becomes the
	// reflectance and what is left to scatter goes to black.
	float metallic = clamp(rmaos.g, 0.0, 1.0);
	float dielectricF0 = clamp(rmaos.a * prop.shininess, 0.0, 1.0);
	f0 = mix(vec3(dielectricF0), albedo, metallic);
	diffuseColor = albedo * (1.0 - metallic);
}

void main(void)
{
	uv = vUV * prop.uvScale + prop.uvOffset;
	vec4 color = vColor;

	// Vertex colors are left as they are rather than decoded: True PBR spends them on ambient
	// occlusion, which is already the linear quantity the lighting below wants.
	albedo = vColor.rgb;

	if (!bWireframe)
	{
		if (bShowTexture)
		{
			// Base color, which is authored in sRGB and lit in linear
			vec4 baseMap = texture(texDiffuse, uv);
			albedo *= bDiffuseSRGB ? baseMap.rgb : srgbToLinear(baseMap.rgb);
			color.a *= baseMap.a;
		}

		// What the shape looks like with the lighting off: the base color on its own
		color.rgb = albedo;

		if (bLightEnabled)
		{
			vec3 outDiffuse = vec3(0.0);
			vec3 outSpecular = vec3(0.0);
			vec3 emissive = vec3(0.0);

			if (bShowTexture && bNormalMap)
			{
				// Tangent space normal map. True PBR keeps all three channels here - the alpha holds
				// nothing, unlike the vanilla map where it is the specular factor.
				vec4 normalMap = texture(texNormal, uv);
				normal = normalize(mv_tbn * (normalMap.rgb * 2.0 - 1.0));
			}
			else
			{
				// Vertex normal for shading with disabled maps
				normal = normalize(mv_normalMatrix * n);
			}

			readMaterial();

			directionalLight(frontal, lightFrontal, outDiffuse, outSpecular);
			directionalLight(directional0, lightDirectional0, outDiffuse, outSpecular);
			directionalLight(directional1, lightDirectional1, outDiffuse, outSpecular);
			directionalLight(directional2, lightDirectional2, outDiffuse, outSpecular);

			imageBasedLight(outDiffuse, outSpecular);

			// Emissive
			if (bEmissive)
			{
				emissive += prop.emissiveColor * prop.emissiveMultiple;

				// Emissive map
				if (bPBREmissive && bShowTexture)
				{
					vec3 emissiveMap = texture(texEmissive, uv).rgb;
					emissive *= bEmissiveSRGB ? emissiveMap : srgbToLinear(emissiveMap);
				}
			}

			color.rgb = diffuseColor * outDiffuse + outSpecular + emissive;
		}

		// Rolled off and encoded in one step, which is the only place this shader parts ways with
		// the others: they hand the frame over in whatever space their textures were in, while
		// everything above here is linear and has to be put back.
		color.rgb = linearToSrgb(tonemap(color.rgb) / tonemap(vec3(1.0)));

		// The mask and weight tints go on after the encode rather than before it, which is the other
		// way round from the shaders that never leave display space. They are interface, not light:
		// dimming to a third of the brightness has to look like a third here as well, and a third of
		// the linear value would come back out of the encode as about two thirds.
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
