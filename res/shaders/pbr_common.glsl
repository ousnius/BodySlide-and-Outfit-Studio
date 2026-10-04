/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

// Metallic-roughness lighting shared by the PBR fragment shaders (sk_truepbr.frag, sf_default.frag),
// pulled in with #include, which GLShader expands. Everything here is in linear space.
//
// The including shader declares the view direction and the texture toggle before the #include:
//   in vec3 viewDir;
//   uniform bool bShowTexture;
// and fills in the surface description below (normal, roughness, ao, f0, diffuseColor) before it
// calls directionalLight() or imageBasedLight().

uniform samplerCube texCubemap;
uniform bool bCubemap;

// Highest mip the cubemap has, which is as rough a reflection as it can describe
uniform float cubemapMaxLod;
// Sharpest mip a reflection may use. A cubemap generated from an HDRi keeps a trace of blur even at
// full gloss, which reads as more realistic than a mirror.
uniform float cubemapMinLod;
// Tints what the cubemap reflects, 1.0 for a cubemap that reflects on its own account.
uniform vec3 cubemapTint;

uniform mat4 matModel;
uniform mat4 matModelViewInverse;

uniform float ambient;

struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

const float PI = 3.14159265358979323846;

// Roughness of zero collapses the GGX lobe into a singularity, which shows up as a lone blazing
// pixel wherever a light reflects dead on. Every renderer floors it somewhere.
const float MIN_ROUGHNESS = 0.03;

vec3 normal = vec3(0.0);

// Surface description. Metalness isn't among them: it says how to split the albedo between the
// reflectance and what is left to scatter, and once that split is made nothing downstream asks about
// it again.
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
