#version 330
#extension GL_ARB_shading_language_packing : require

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

out vec4 FragColor;

in vec2 TexCoords;

uniform int SampleCount; // Match the number of MSAA samples
uniform int DebugGBuffer;
uniform vec3 bgColor;

uniform sampler2DMS gPosition;					// XYZ
uniform usampler2DMS gNormalMaskWeightRimSoft;	// Normal XYZ, Mask, Weight, Rimlight Power, Soft Lighting
uniform sampler2DMS gAlbedoAlpha;				// RGBA
uniform sampler2DMS gSpecular;					// RGBA
uniform sampler2DMS gVertexColors;				// RGB
uniform sampler2DMS gEnvironment;				// RGB cubemap, A mask
uniform sampler2DMS gEmissiveRefl;				// RGB, Env Reflection
uniform sampler2DMS gLightMask;					// RGB light mask for soft/rimlighting
uniform sampler2DMS gDepth;						// Depth

vec4 computeColor(vec3 pos, vec3 norm, vec4 albedoAlpha, vec4 specular, vec3 vertexColors, vec3 emissive, float maskFactor, float weightFactor)
{
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	if (DebugGBuffer == 0)
	{
		// Display Position Buffer
		color = vec4(pos, 1.0);
	}
	else if (DebugGBuffer == 1)
	{
		// Display Normal Buffer
		color = vec4(norm * 0.5 + 0.5, 1.0);
	}
	else if (DebugGBuffer == 2)
	{
		// Display Albedo (Diffuse) Color Buffer
		color = vec4(albedoAlpha.rgb, 1.0);
	}
	else if (DebugGBuffer == 3)
	{
		// Display Specular Buffer
		color = vec4(specular.a, specular.a, specular.a, 1.0);
	}
	else if (DebugGBuffer == 4)
	{
		// Display Vertex Color Buffer
		color = vec4(vertexColors, 1.0);
	}
	else if (DebugGBuffer == 5)
	{
		// Display Emissive Buffer
		color = vec4(emissive, 1.0);
	}
	else if (DebugGBuffer == 6)
	{
		// Display Mask Factor Buffer
		color = vec4(maskFactor, maskFactor, maskFactor, 1.0);
	}
	else if (DebugGBuffer == 7)
	{
		// Display Weight Factor Buffer
		color = vec4(weightFactor, weightFactor, weightFactor, 1.0);
	}

	return color;
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

float uintBitsToFloat(uint u)
{
	return intBitsToFloat(int(u));
}


void main(void)
{
	ivec2 texelCoord = ivec2(gl_FragCoord.xy);

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

			vec4 albedoAlpha = texelFetch(gAlbedoAlpha, texelCoord, i).rgba;
			vec4 specular = texelFetch(gSpecular, texelCoord, i).rgba;
			vec3 vertexColors = texelFetch(gVertexColors, texelCoord, i).rgb;
			vec4 environment = texelFetch(gEnvironment, texelCoord, i).rgba;
			vec4 emissiveRefl = texelFetch(gEmissiveRefl, texelCoord, i).rgba;
			vec3 lightMask = texelFetch(gLightMask, texelCoord, i).rgb;

			vec3 emissive = emissiveRefl.rgb;

			// Perform per-sample
			vec4 color = computeColor(pos, norm, albedoAlpha, specular, vertexColors, emissive, maskFactor, weightFactor);

			finalColor += color;
		}
		else
		{
			finalColor += vec4(bgColor.rgb, 1.0);
		}
	}

	finalColor /= float(SampleCount); // Manually resolve
	FragColor = finalColor;
}
