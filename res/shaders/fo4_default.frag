#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texCubemap;
uniform sampler2D texEnvMask;
uniform sampler2D texSpecular;
uniform sampler2D texGreyscale;
uniform sampler2D texGlowmap;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;

uniform bool bNormalMap;
uniform bool bModelSpace;
uniform bool bCubemap;
uniform bool bEnvMask;
uniform bool bSpecular;
uniform bool bEmissive;
uniform bool bBacklight;
uniform bool bRimlight;
uniform bool bSoftlight;
uniform bool bGlowmap;
uniform bool bGreyscaleColor;

uniform mat4 matModel;
uniform mat4 matModelViewInverse;

struct Properties
{
	vec2 uvOffset;
	vec2 uvScale;
	vec3 specularColor;
	float specularStrength;
	float shininess;
	float envReflection;
	vec3 emissiveColor;
	float emissiveMultiple;
	float alpha;
	float backlightPower;
	float rimlightPower;
	float subsurfaceRolloff;
	float fresnelPower;
	float paletteScale;
};
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
in mat3 mv_tbn;

in float maskFactor;
in vec3 weightColor;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

vec3 normal = vec3(0.0);
float specGloss = 1.0;
float specFactor = 1.0;

vec2 uv = vec2(0.0);
vec3 albedo = vec3(0.0);
vec3 emissive = vec3(0.0);

vec4 baseMap = vec4(0.0);
vec4 normalMap = vec4(0.0);
vec4 specMap = vec4(0.0);
vec4 envMask = vec4(0.0);

#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif

#define FLT_EPSILON 1.192092896e-07F // smallest such that 1.0 + FLT_EPSILON != 1.0

float OrenNayarFull(vec3 L, vec3 V, vec3 N, float roughness, float NdotL)
{
	//float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float LdotV = dot(L, V);

	float angleVN = acos(max(NdotV, FLT_EPSILON));
	float angleLN = acos(max(NdotL, FLT_EPSILON));

	float alpha = max(angleVN, angleLN);
	float beta = min(angleVN, angleLN);
	float gamma = LdotV - NdotL * NdotV;

	float roughnessSquared = roughness * roughness;
	float roughnessSquared9 = (roughnessSquared / (roughnessSquared + 0.09));

	// C1, C2, and C3
	float C1 = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.33));
	float C2 = 0.45 * roughnessSquared9;

	if( gamma >= 0.0 )
		C2 *= sin(alpha);
	else
		C2 *= (sin(alpha) - pow((2.0 * beta) / M_PI, 3.0));

	float powValue = (4.0 * alpha * beta) / (M_PI * M_PI);
	float C3 = 0.125 * roughnessSquared9 * powValue * powValue;

	// Avoid asymptote at pi/2
	float asym = M_PI / 2.0;
	float lim1 = asym + 0.01;
	float lim2 = asym - 0.01;

	float ab2 = (alpha + beta) / 2.0;

	if (beta >= asym && beta < lim1)
		beta = lim1;
	else if (beta < asym && beta >= lim2)
		beta = lim2;

	if (ab2 >= asym && ab2 < lim1)
		ab2 = lim1;
	else if (ab2 < asym && ab2 >= lim2)
		ab2 = lim2;

	// Reflection
	float A = gamma * C2 * tan(beta);
	float B = (1.0 - abs(gamma)) * C3 * tan(ab2);

	float L1 = max(FLT_EPSILON, NdotL) * (C1 + A + B);

	// Interreflection
	float twoBetaPi = 2.0 * beta / M_PI;
	float L2 = 0.17 * max(FLT_EPSILON, NdotL) * (roughnessSquared / (roughnessSquared + 0.13)) * (1.0 - gamma * twoBetaPi * twoBetaPi);

	return L1 + L2;
}

// Schlick's Fresnel approximation
float fresnelSchlick(float VdotH, float F0)
{
	float base = 1.0 - VdotH;
	float exp = pow(base, prop.fresnelPower);
	return clamp(exp + F0 * (1.0 - exp), 0.0, 1.0);
}

// The Torrance-Sparrow visibility factor, G
float VisibDiv(float NdotL, float NdotV, float VdotH, float NdotH)
{
	float denom = max(VdotH, FLT_EPSILON);
	float numL = min(NdotV, NdotL);
	float numR = 2.0 * NdotH;
	if (denom >= (numL * numR))
	{
		numL = (numL == NdotV) ? 1.0 : (NdotL / NdotV);
		return (numL * numR) / denom;
	}
	return 1.0 / NdotV;
}

// this is a normalized Phong model used in the Torrance-Sparrow model
vec3 TorranceSparrow(float NdotL, float NdotH, float NdotV, float VdotH, vec3 color, float power, float F0)
{
	// D: Normalized phong model
	float D = ((power + 2.0) / (2.0 * M_PI)) * pow(NdotH, power);

	// G: Torrance-Sparrow visibility term divided by NdotV
	float G_NdotV = VisibDiv(NdotL, NdotV, VdotH, NdotH);

	// F: Schlick's approximation
	float F = fresnelSchlick(VdotH, F0);

	// Torrance-Sparrow:
	// (F * G * D) / (4 * NdotL * NdotV)
	// Division by NdotV is done in VisibDiv()
	// and division by NdotL is removed since
	// outgoing radiance is determined by:
	// BRDF * NdotL * L()
	float spec = (F * G_NdotV * D) / 4.0;

	return color * spec * M_PI;
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

void directionalLight(in DirectionalLight light, in vec3 lightDir, inout vec3 outDiffuse, inout vec3 outSpec)
{
	vec3 halfDir = normalize(lightDir + viewDir);
	float NdotL = dot(normal, lightDir);
	float NdotL0 = max(NdotL, FLT_EPSILON);
	float NdotH = max(dot(normal, halfDir), FLT_EPSILON);
	float NdotV = max(dot(normal, viewDir), FLT_EPSILON);
	float VdotH = max(dot(viewDir, halfDir), FLT_EPSILON);

	// Temporary diffuse
	vec3 diff = ambient + NdotL0 * light.diffuse;

	// Specularity
	float smoothness = 1.0;
	float roughness = 0.0;
	float specMask = 1.0;
	if (bSpecular && bShowTexture)
	{
		smoothness = specGloss * prop.shininess;
		roughness = 1.0 - smoothness;
		float fSpecularPower = exp2(smoothness * 10.0 + 1.0);
		specMask = specFactor * prop.specularStrength;

		outSpec += TorranceSparrow(NdotL0, NdotH, NdotV, VdotH, vec3(specMask), fSpecularPower, 0.2) * NdotL0 * light.diffuse * prop.specularColor;
		outSpec += ambient * specMask * fresnelSchlick(VdotH, 0.2) * (1.0 - NdotV) * light.diffuse;
	}

	// Environment
	if (bCubemap && bShowTexture)
	{
		vec3 reflected = reflect(viewDir, normal);
		vec3 reflectedWS = vec3(matModel * (matModelViewInverse * vec4(reflected, 0.0)));

		vec4 cube = textureLod(texCubemap, reflectedWS, 8.0 - smoothness * 8.0);
		cube.rgb *= prop.envReflection * prop.specularStrength;
		if (bEnvMask)
		{
			cube.rgb *= envMask.r;
		}
		else
		{
			// No env mask, use specular factor
			cube.rgb *= specFactor;
		}

		outSpec += cube.rgb * diff;
	}

	// Back lighting not really useful for the current light setup of multiple directional lights
	//if (bBacklight)
	//{
	//	float NdotNegL = max(dot(normal, -lightDir), FLT_EPSILON);
	//	vec3 backlight = albedo * NdotNegL * clamp(prop.backlightPower, 0.0, 1.0);
	//	emissive += backlight * light.diffuse;
	//}

	// Rim lighting not really useful for the current light setup of multiple directional lights
	//if (bRimlight)
	//{
	//	vec3 rim = vec3(pow((1.0 - NdotV), prop.rimlightPower));
	//	rim *= smoothstep(-0.2, 1.0, dot(-lightDir, viewDir));
	//	emissive += rim * light.diffuse * specMask;
	//}

	// Diffuse
	diff = vec3(OrenNayarFull(lightDir, viewDir, normal, roughness, NdotL0));
	outDiffuse += diff * light.diffuse;

	// Soft Lighting
	if (bSoftlight)
	{
		float wrap = (NdotL + prop.subsurfaceRolloff) / (1.0 + prop.subsurfaceRolloff);
		vec3 soft = albedo * max(0.0, wrap) * smoothstep(1.0, 0.0, sqrt(diff));
		outDiffuse += soft;
	}
}

vec4 colorLookup(in float x, in float y)
{
	return texture(texGreyscale, vec2(clamp(x, 0.0, 1.0), clamp(y, 0.0, 1.0)));
}

void main(void)
{
	uv = vUV * prop.uvScale + prop.uvOffset;
	vec4 color = vColor;
	albedo = vColor.rgb;

	if (!bWireframe)
	{
		if (bShowTexture)
		{
			// Diffuse Texture
			baseMap = texture(texDiffuse, uv);
			albedo *= baseMap.rgb;
			color.a *= baseMap.a;

			// Diffuse texture without lighting
			color.rgb = albedo;

			if (bLightEnabled)
			{
				if (bNormalMap)
				{
					normalMap = texture(texNormal, uv);

					if (bSpecular)
					{
						// Specular Map
						specMap = texture(texSpecular, uv);
						specGloss = specMap.g;
						specFactor = specMap.r;
					}
				}

				if (bCubemap)
				{
					if (bEnvMask)
					{
						// Environment Mask
						envMask = texture(texEnvMask, uv);
					}
				}
			}
		}

		if (bLightEnabled)
		{
			// Lighting with or without textures
			vec3 outDiffuse = vec3(0.0);
			vec3 outSpecular = vec3(0.0);

			// Start off neutral
			normal = normalize(mv_tbn * vec3(0.0, 0.0, 0.5));

			if (bShowTexture)
			{
				if (bNormalMap)
				{
					if (bModelSpace)
					{
						// No proper FO4 model space map rendering yet
						//normal = normalize(normalMap.rgb * 2.0 - 1.0);
						//normal.r = -normal.r;
					}
					else
					{
						normal = (normalMap.rgb * 2.0 - 1.0);

						// Calculate missing blue channel
						normal.b = sqrt(1.0 - dot(normal.rg, normal.rg));

						// Tangent space map
						normal = normalize(mv_tbn * normal);
					}
				}

				if (bGreyscaleColor)
				{
					vec4 luG = colorLookup(baseMap.g, prop.paletteScale - (1.0 - vColor.r));
					albedo = luG.rgb;
				}
			}

			directionalLight(frontal, lightFrontal, outDiffuse, outSpecular);
			directionalLight(directional0, lightDirectional0, outDiffuse, outSpecular);
			directionalLight(directional1, lightDirectional1, outDiffuse, outSpecular);
			directionalLight(directional2, lightDirectional2, outDiffuse, outSpecular);

			// Emissive
			if (bEmissive)
			{
				emissive += prop.emissiveColor * prop.emissiveMultiple;

				// Glowmap
				if (bGlowmap)
				{
					vec4 glowMap = texture(texGlowmap, uv);
					emissive *= glowMap.rgb;
				}
			}

			color.rgb = outDiffuse * albedo;
			color.rgb += outSpecular;
			color.rgb += emissive;
			color.rgb += ambient * albedo;
		}

		if (bShowMask)
		{
			color.rgb *= maskFactor;
		}

		if (bShowWeight)
		{
			color.rgb *= weightColor;
		}

		color.rgb = tonemap(color.rgb) / tonemap(vec3(1.0));
	}
	else
	{
		color = vec4(color.rgb, 0.5);
	}

	color = clamp(color, 0.0, 1.0);

	fragColor = color;

	if (!bWireframe)
	{
		fragColor.a *= prop.alpha;

		if (alphaThreshold != -1.0f)
			if (fragColor.a <= alphaThreshold) // GL_GREATER
				discard;
	}
}
