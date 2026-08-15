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
uniform samplerCube texCubemap;
// Roughness in red, metalness in green, ambient occlusion in blue and the specular level in alpha,
// texture slot 5. Vanilla puts the environment mask there, hence the separate name again.
uniform sampler2D texRMAOS;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;

uniform bool bNormalMap;
uniform bool bCubemap;
uniform bool bRMAOS;
uniform bool bEmissive;
uniform bool bPBREmissive;

// Highest mip the cubemap has, which is as rough a reflection as it can describe
uniform float cubemapMaxLod;
// Sharpest mip a reflection may use. A cubemap generated from an HDRi keeps a trace of blur even at
// full gloss, which reads as more realistic than a mirror.
uniform float cubemapMinLod;
// Tints what the cubemap reflects, 1.0 for a cubemap that reflects on its own account.
uniform vec3 cubemapTint;

uniform mat4 matModel;
uniform mat4 matModelViewInverse;
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

uniform float ambient;

struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

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

const float PI = 3.14159265358979323846;

// Roughness of zero collapses the GGX lobe into a singularity, which shows up as a lone blazing
// pixel wherever a light reflects dead on. Every renderer floors it somewhere.
const float MIN_ROUGHNESS = 0.03;

vec2 uv = vec2(0.0);
vec3 normal = vec3(0.0);
vec3 albedo = vec3(0.0);

// Surface description, filled in by readMaterial() below. Metalness isn't among them: it says how to
// split the albedo between the reflectance and what is left to scatter, and once that split is made
// nothing downstream asks about it again.
float roughness = 1.0;
float ao = 1.0;
vec3 f0 = vec3(0.04);
vec3 diffuseColor = vec3(0.0);

vec3 srgbToLinear(in vec3 c)
{
	return pow(max(c, vec3(0.0)), vec3(2.2));
}

vec3 linearToSrgb(in vec3 c)
{
	return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 tonemap(in vec3 x)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Trowbridge-Reitz (GGX) normal distribution
float distributionGGX(in float NdotH)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 1e-7);
}

// Height correlated Smith visibility, which is the geometry term already divided by the
// 4 * NdotL * NdotV the microfacet BRDF would otherwise be divided by
float visibilitySmith(in float NdotL, in float NdotV)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
	float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
	return 0.5 / max(lambdaV + lambdaL, 1e-7);
}

vec3 fresnelSchlick(in float VdotH)
{
	return f0 + (vec3(1.0) - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// Karis' analytic fit of the split sum environment BRDF, which saves carrying a lookup table around
// for the one term that would need it
vec2 envBRDFApprox(in float NdotV)
{
	const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);

	vec4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
	return vec2(-1.04, 1.04) * a004 + r.zw;
}

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

void directionalLight(in DirectionalLight light, in vec3 lightDir, inout vec3 outDiffuse, inout vec3 outSpec)
{
	vec3 halfDir = normalize(lightDir + viewDir);
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotV = max(dot(normal, viewDir), 1e-4);
	float NdotH = max(dot(normal, halfDir), 0.0);
	float VdotH = max(dot(viewDir, halfDir), 0.0);

	if (NdotL <= 0.0)
		return;

	vec3 F = fresnelSchlick(VdotH);
	vec3 spec = distributionGGX(NdotH) * visibilitySmith(NdotL, NdotV) * F;

	// Both terms are scaled by pi against the textbook BRDF, which cancels the 1/pi of the Lambert
	// diffuse and leaves it at the brightness every other shader here gives a lit surface. Scaling
	// the specular by the same amount is what keeps the two in the ratio the BRDF asks for.
	outDiffuse += (vec3(1.0) - F) * NdotL * light.diffuse;
	outSpec += PI * spec * NdotL * light.diffuse;
}

void imageBasedLight(inout vec3 outDiffuse, inout vec3 outSpec)
{
	float NdotV = max(dot(normal, viewDir), 1e-4);

	// The ambient light stands for light arriving evenly from every direction, which is the same thing
	// the image based terms describe, so it belongs here rather than with the directional lights. It is
	// added to whatever the environment supplies instead of standing in for it, so the viewport's
	// ambient setting keeps working once an HDRi is loaded - and it is the only light left holding a
	// metal up when there is no environment at all.
	vec3 irradiance = vec3(ambient);
	vec3 radiance = vec3(ambient);

	if (bCubemap && bShowTexture)
	{
		vec3 normalWS = vec3(matModel * (matModelViewInverse * vec4(normal, 0.0)));
		vec3 reflectedWS = vec3(matModel * (matModelViewInverse * vec4(reflect(-viewDir, normal), 0.0)));

		// The cubemap was tone mapped and encoded when it was generated, the same way one from a DDS
		// file already is, so it has to be brought back to linear before it can stand for radiance.
		// The tone map isn't invertible, but undoing the encode puts it in the right range.
		// Its roughest mip stands in for the diffuse irradiance: the chain was convolved with a GGX
		// lobe, and the widest of those is close enough to a cosine one at this size.
		irradiance += srgbToLinear(textureLod(texCubemap, normalWS, cubemapMaxLod).rgb) * cubemapTint;
		radiance += srgbToLinear(textureLod(texCubemap, reflectedWS, mix(cubemapMinLod, cubemapMaxLod, roughness)).rgb) * cubemapTint;
	}

	vec2 ab = envBRDFApprox(NdotV);

	outDiffuse += irradiance * ao;
	outSpec += radiance * (f0 * ab.x + ab.y) * ao;
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
			albedo *= srgbToLinear(baseMap.rgb);
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
					emissive *= srgbToLinear(texture(texEmissive, uv).rgb);
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
