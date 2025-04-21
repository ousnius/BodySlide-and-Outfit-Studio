#version 330
out float FragColor;

in vec2 TexCoords;

uniform int SampleCount; // Match the number of MSAA samples
uniform vec3 bgColor;

uniform sampler2DMS gPosition;
uniform usampler2DMS gNormalMaskWeightRimSoft;
uniform sampler2DMS gDepth;

uniform sampler2D texNoise;

uniform vec3 samples[128];
uniform mat4 matView;
uniform mat4 matProjection;
uniform mat3 mv_normalMatrix;

uniform vec2 screenSize;
uniform vec2 noiseScale; // = screenSize / 4

const float radius = 0.3;
const float bias = 0.01;

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

float computeOcclusion(vec3 fragPos, vec3 normal, vec3 randomVec, int sample)
{
	float occlusion = 0.0;
	for (int i = 0; i < 128; ++i)
	{
		vec3 sampleVec = samples[i];
		sampleVec = fragPos + sampleVec * radius;

		// Project to screen space (NDC -> texture space)
		vec4 offset = vec4(sampleVec, 1.0);
		offset = matProjection * vec4(sampleVec, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;  // NDC -> [0, 1]

		if (offset.x >= 0.0 && offset.x <= 1.0 &&
			offset.y >= 0.0 && offset.y <= 1.0)
		{
			ivec2 sampleTexel = ivec2(offset.xy * screenSize);
			float sampleDepth = texelFetch(gPosition, sampleTexel, sample).z;
			float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
			occlusion += (sampleDepth >= sampleVec.z + bias ? 1.0 : 0.0) * rangeCheck;
		}
	}

	occlusion = 1.0 - (occlusion / 128.0);
	return occlusion;
}


void main()
{
	vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

	ivec2 texelCoord = ivec2(gl_FragCoord.xy);

	float finalOcclusion = 0.0;

	for (int i = 0; i < SampleCount; ++i)
	{
		float depth = texelFetch(gDepth, texelCoord, i).r;
		if (depth < 1.0)
		{
			vec3 pos = texelFetch(gPosition, texelCoord, i).xyz;

			uvec4 normalMaskWeightRimSoft = texelFetch(gNormalMaskWeightRimSoft, texelCoord, i).xyzw;

			vec2 encNormal = vec2(uintBitsToFloat(normalMaskWeightRimSoft.x), uintBitsToFloat(normalMaskWeightRimSoft.y));
			vec3 norm = decodeOctahedralNormal(encNormal);

			// Perform per-sample
			float occlusion = computeOcclusion(pos, norm, randomVec, i);

			finalOcclusion += occlusion;
		}
		else
		{
			finalOcclusion += 1.0;
		}
	}

	finalOcclusion /= float(SampleCount); // Manually resolve
	FragColor = finalOcclusion;
}
