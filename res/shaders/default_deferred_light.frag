#version 330
#extension GL_ARB_shading_language_packing : require

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

// G-buffer inputs
uniform sampler2DMS gPosition;					// XYZ
uniform usampler2DMS gNormalMaskWeightRimSoft;	// Normal XYZ, Mask, Weight, Rimlight Power, Soft Lighting
uniform sampler2DMS gAlbedoAlpha;				// RGBA
uniform sampler2DMS gSpecular;					// RGBA
uniform sampler2DMS gVertexColors;				// RGB
uniform sampler2DMS gEnvironment;				// RGB cubemap, A mask
uniform sampler2DMS gEmissiveRefl;				// RGB, Env Reflection
uniform sampler2DMS gLightMask;					// RGB light mask for soft/rimlighting
uniform sampler2DMS gDepth;						// Depth

uniform sampler2DMS ssaoTexture;				// SSAO Texture

uniform int SampleCount; // Match the number of MSAA samples
uniform vec3 bgColor;
uniform vec2 screenSize;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;

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

in vec2 quadUV;

out vec4 FragColor;

vec3 viewDir = vec3(0.0);
vec3 normal = vec3(0.0);

vec3 colorRamp(in float value)
{
	float r;
	float g;
	float b;

	if (value <= 0.0)
	{
		r = g = b = 1.0;
	}
	else if (value <= 0.25)
	{
		r = 0.0;
		b = 1.0;
		g = value / 0.25;
	}
	else if (value <= 0.5)
	{
		r = 0.0;
		g = 1.0;
		b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
	}
	else if (value <= 0.75)
	{
		r = (value - 0.5) / 0.25;
		g = 1.0;
		b = 0.0;
	}
	else
	{
		r = 1.0;
		g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
		b = 0.0;
	}

	return vec3(r, g, b);
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

vec3 decodeOctahedralNormal(vec2 f)
{
	f = f * 2.0 - 1.0; // remap from [0,1] to [-1,1]
	vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));

	if (n.z < 0.0) {
		vec2 signFix = vec2((n.x >= 0.0) ? 1.0 : -1.0,
							(n.y >= 0.0) ? 1.0 : -1.0);
		n.xy = (1.0 - abs(n.yx)) * signFix;
	}

	return normalize(n);
}

void directionalLight(in DirectionalLight light, in vec3 lightDir, in vec3 specular, in float shininess, in float rimlightPower, in float softlighting, in vec3 lightMask, in float ssao, inout vec3 outDiffuse, inout vec3 outSpec, inout vec3 outEmissive)
{
	vec3 halfDir = normalize(lightDir + viewDir);
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotH = max(dot(normal, halfDir), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);

	outDiffuse += ambient * ssao + NdotL * light.diffuse;

	outSpec += clamp(specular.rgb * pow(NdotH, shininess), 0.0, 1.0) * light.diffuse;

	if (bShowTexture)
	{
		// Back lighting not really useful for the current light setup of multiple directional lights
		//if (bBacklight)
		//{
		//	float NdotNegL = max(dot(normal, -lightDir), 0.0);
		//	vec3 backlight = backlightMap.rgb * NdotNegL * light.diffuse;
		//	outEmissive += backlight;
		//}

		// Rim lighting not really useful for the current light setup of multiple directional lights
		//if (rimlightPower != 0.0)
		//{
		//	vec3 rim = lightMask * pow(vec3(1.0 - NdotV), vec3(rimlightPower));
		//	rim *= smoothstep(-0.2, 1.0, dot(-lightDir, viewDir));
		//	outEmissive += rim * light.diffuse;
		//}

		// Soft Lighting
		if (softlighting != 0.0)
		{
			float wrap = (dot(normal, lightDir) + softlighting) / (1.0 + softlighting);
			vec3 soft = max(wrap, 0.0) * lightMask * smoothstep(1.0, 0.0, NdotL);
			soft *= sqrt(clamp(softlighting, 0.0, 1.0));
			outEmissive += soft * light.diffuse;
		}
	}
}

vec4 computeLighting(vec3 pos, vec3 norm, float maskFactor, float weightFactor, vec4 albedoAlpha, vec4 specular, vec3 vertexColors, vec4 environment, vec3 emissive, vec3 lightMask, float rimlightPower, float softlighting, float envReflection, float ssao)
{
	viewDir = normalize(-pos);
	normal = norm;

	vec4 color = vec4(vertexColors, 1.0);
	vec3 albedo = color.rgb;

	if (bShowTexture)
	{
		// Diffuse Texture
		albedo *= albedoAlpha.rgb;

		// Diffuse texture without lighting
		color.rgb = albedo;
	}

	if (bLightEnabled)
	{
		// Lighting with or without textures
		vec3 outDiffuse = vec3(0.0);
		vec3 outSpecular = vec3(0.0);
		vec3 outEmissive = vec3(0.0);

		float shininess = specular.a;
		directionalLight(frontal, lightFrontal, specular.rgb, shininess, rimlightPower, softlighting, lightMask, ssao, outDiffuse, outSpecular, outEmissive);
		directionalLight(directional0, lightDirectional0, specular.rgb, shininess, rimlightPower, softlighting, lightMask, ssao, outDiffuse, outSpecular, outEmissive);
		directionalLight(directional1, lightDirectional1, specular.rgb, shininess, rimlightPower, softlighting, lightMask, ssao, outDiffuse, outSpecular, outEmissive);
		directionalLight(directional2, lightDirectional2, specular.rgb, shininess, rimlightPower, softlighting, lightMask, ssao, outDiffuse, outSpecular, outEmissive);

		if (bShowTexture)
		{
			vec3 cubeMap = environment.rgb;
			cubeMap *= envReflection;

			if (environment.a != -1.0)
			{
				cubeMap *= environment.a; // Env Mask
			}
			else
			{
				// No env mask, use specular factor (0.0 if no normal map either)
				cubeMap *= specular.rgb;
			}

			albedo += cubeMap;
		}

		// Emissive
		outEmissive += emissive;

		color.rgb = albedo * (outDiffuse + outEmissive) + outSpecular;
	}

	color.rgb = tonemap(color.rgb) / tonemap(vec3(1.0));

	if (bShowMask)
	{
		color.rgb *= maskFactor;
	}

	if (bShowWeight)
	{
		if (weightFactor != -1.0)
		{
			color.rgb *= colorRamp(weightFactor);
		}
	}

	color = clamp(color, 0.0, 1.0);
	return color;
}

float uintBitsToFloat(uint u)
{
	return intBitsToFloat(int(u));
}


void main(void)
{
	vec2 texelSize = 1.0 / screenSize;
	ivec2 texelCoord = ivec2(gl_FragCoord.xy);

	// 5x5 Gaussian kernel
	vec3 samples[25] = vec3[25](
		vec3(-2.0,  2.0, 1.0), vec3(-1.0,  2.0, 1.0), vec3( 0.0,  2.0, 1.0), vec3( 1.0,  2.0, 1.0), vec3( 2.0,  2.0, 1.0),
		vec3(-2.0,  1.0, 1.0), vec3(-1.0,  1.0, 1.0), vec3( 0.0,  1.0, 1.0), vec3( 1.0,  1.0, 1.0), vec3( 2.0,  1.0, 1.0),
		vec3(-2.0,  0.0, 1.0), vec3(-1.0,  0.0, 1.0), vec3( 0.0,  0.0, 1.0), vec3( 1.0,  0.0, 1.0), vec3( 2.0,  0.0, 1.0),
		vec3(-2.0, -1.0, 1.0), vec3(-1.0, -1.0, 1.0), vec3( 0.0, -1.0, 1.0), vec3( 1.0, -1.0, 1.0), vec3( 2.0, -1.0, 1.0),
		vec3(-2.0, -2.0, 1.0), vec3(-1.0, -2.0, 1.0), vec3( 0.0, -2.0, 1.0), vec3( 1.0, -2.0, 1.0), vec3( 2.0, -2.0, 1.0)
	);

	vec4 finalColor = vec4(0.0);

	for (int i = 0; i < SampleCount; ++i)
	{
		float depth = texelFetch(gDepth, texelCoord, i).r;
		if (depth < 1.0)
		{
			vec3 pos = texelFetch(gPosition, texelCoord, i).xyz;

			uvec4 normalMaskWeightRimSoft = texelFetch(gNormalMaskWeightRimSoft, texelCoord, i).xyzw;

			vec2 encNormal = vec2(uintBitsToFloat(normalMaskWeightRimSoft.x), uintBitsToFloat(normalMaskWeightRimSoft.y));
			vec3 norm = decodeOctahedralNormal(encNormal);

			vec2 maskWeight = unpackHalf2x16(normalMaskWeightRimSoft.z);
			float maskFactor = maskWeight.x;
			float weightFactor = maskWeight.y;

			vec2 rimSoft = unpackHalf2x16(normalMaskWeightRimSoft.w);
			float rimlightPower = rimSoft.x;
			float softlighting = rimSoft.y;

			vec4 albedoAlpha = texelFetch(gAlbedoAlpha, texelCoord, i).rgba;
			vec4 specular = texelFetch(gSpecular, texelCoord, i).rgba;
			vec3 vertexColors = texelFetch(gVertexColors, texelCoord, i).rgb;
			vec4 environment = texelFetch(gEnvironment, texelCoord, i).rgba;
			vec4 emissiveRefl = texelFetch(gEmissiveRefl, texelCoord, i).rgba;
			vec3 lightMask = texelFetch(gLightMask, texelCoord, i).rgb;

			vec3 emissive = emissiveRefl.rgb;
			float envReflection = emissiveRefl.a;

			float ssao = 0.0;
			for (int j = 0; j < 25; ++j)
			{
				ssao += texelFetch(ssaoTexture, texelCoord + ivec2(samples[j].xy * texelSize), i).r * samples[j].z;
			}
			ssao /= 25.0;
			ssao = clamp(ssao, 0.0, 1.0);

			// Perform lighting per-sample
			vec4 lighting = computeLighting(pos, norm, maskFactor, weightFactor, albedoAlpha, specular, vertexColors, environment, emissive, lightMask, rimlightPower, softlighting, envReflection, ssao);

			finalColor += lighting;
		}
		else
		{
			finalColor += vec4(bgColor.rgb, 1.0);
		}
	}

	finalColor /= float(SampleCount); // Manually resolve
	FragColor = finalColor;
}
