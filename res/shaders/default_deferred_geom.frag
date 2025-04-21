#version 330
#extension GL_ARB_shading_language_packing : require

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

in vec3 FragPos;      // View-space position from vertex shader
in vec3 VNormal;      // View-space normal
in vec2 TexCoords;    // UVs
in mat3 TBN;          // For normal mapping

in float MaskFactor;
in float WeightFactor;
in vec3 VertexColor;
in float VertexAlpha;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform sampler2D texSpecular;
uniform samplerCube texCubemap;
uniform sampler2D texEnvMask;
uniform sampler2D texBacklight;
uniform sampler2D texLightmask;
uniform sampler2D texGlowmap;

uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matModelViewInverse;
uniform mat3 mv_normalMatrix;

uniform bool bNormalMap;
uniform bool bShowTexture;
uniform bool bModelSpace;
uniform bool bSpecular;

uniform bool bLightEnabled;
uniform bool bCubemap;
uniform bool bEnvMask;
uniform bool bEmissive;
uniform bool bRimlight;
uniform bool bSoftlight;
uniform bool bGlowmap;

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
	float rimlightPower;
	float softlighting;
};
uniform Properties prop;
uniform float alphaThreshold;

// G-buffer outputs
layout(location = 0) out vec3 gPosition;					// XYZ
layout(location = 1) out uvec4 gNormalMaskWeightRimSoft;	// Normal XYZ, Mask, Weight, Rimlight Power, Soft Lighting
layout(location = 2) out vec4 gAlbedoAlpha;					// RGBA
layout(location = 3) out vec4 gSpecular;					// RGBA
layout(location = 4) out vec3 gVertexColors;				// RGB
layout(location = 5) out vec4 gEnvironment;					// RGB cubemap, A mask
layout(location = 6) out vec4 gEmissiveRefl;				// RGB, Env Reflection
layout(location = 7) out vec3 gLightMask;					// RGB light mask for soft/rimlighting

vec2 encodeOctahedralNormal(vec3 n)
{
	n /= (abs(n.x) + abs(n.y) + abs(n.z));
	vec2 enc = n.xy;

	if (n.z < 0.0) {
		enc = (1.0 - abs(enc.yx)) * sign(enc.xy);
	}

	return enc * 0.5 + 0.5; // map from [-1,1] to [0,1]
}

uint floatBitsToUInt(float f)
{
	return uint(floatBitsToInt(f));
}


void main()
{
	vec2 uv = TexCoords * prop.uvScale + prop.uvOffset;

	// Sample the albedo texture
	vec4 diffuse = texture(texDiffuse, uv);

	if (bShowTexture)
	{
		if (alphaThreshold != -1.0f)
			if (diffuse.a <= alphaThreshold) // GL_GREATER
				discard;
	}

	diffuse.a *= prop.alpha;
	diffuse.a *= VertexAlpha;

	vec3 normal = vec3(0.0);
	float specFactor = 0.0;

	// Apply normal map and specular if enabled
	if (bShowTexture && bNormalMap)
	{
		vec4 normalMap = texture(texNormal, uv);

		if (bModelSpace)
		{
			// Model Space Normal Map
			normal = normalize(normalMap.rgb * 2.0 - 1.0);
			normal.r = -normal.r;
			normal = mat3(matView) * normal;
			normal = normalize(normal);

			if (bSpecular)
			{
				specFactor = texture(texSpecular, uv).r;
			}
		}
		else
		{
			// Tangent Space Normal Map
			normal = normalize(TBN * (normalMap.rgb * 2.0 - 1.0));

			if (bSpecular)
			{
				specFactor = normalMap.a;
			}
		}
	}
	else
	{
		// Vertex normal for shading with disabled maps
		normal = mv_normalMatrix * VNormal;
		normal = normalize(normal);
	}

	vec3 cubeMap = vec3(0.0);
	float envMask = -1.0;
	vec3 emissive = vec3(0.0);
	float rimlightPower = 0.0;
	float softlighting = 0.0;
	vec3 lightMask = normalize(vec3(0.0, 0.0, 0.5));

	if (bShowTexture)
	{
		if (bLightEnabled)
		{
			if (bCubemap)
			{
				vec3 viewDir = normalize(-FragPos);
				vec3 reflected = reflect(-viewDir, normal);
				vec3 reflectedWS = vec3(matModel * (matModelViewInverse * vec4(reflected, 0.0)));

				cubeMap = texture(texCubemap, reflectedWS).rgb;

				if (bEnvMask)
				{
					// Environment Mask
					envMask = texture(texEnvMask, uv).r;
				}
			}

			// Emissive
			if (bEmissive)
			{
				emissive = prop.emissiveColor * prop.emissiveMultiple;

				// Glowmap
				if (bGlowmap)
				{
					vec3 glowMap = texture(texGlowmap, uv).rgb;
					emissive *= glowMap;
				}
			}

			if (bRimlight || bSoftlight)
			{
				lightMask = texture(texLightmask, uv).rgb;
			}
		}
	}

	if (bLightEnabled)
	{
		if (bRimlight)
		{
			rimlightPower = prop.rimlightPower;
		}

		if (bRimlight)
		{
			softlighting = prop.softlighting;
		}
	}

	gPosition = FragPos;

	vec2 encNormal = encodeOctahedralNormal(normal);
	gNormalMaskWeightRimSoft.xy = uvec2(floatBitsToUInt(encNormal.x), floatBitsToUInt(encNormal.y));
	gNormalMaskWeightRimSoft.z = packHalf2x16(vec2(MaskFactor, WeightFactor));
	gNormalMaskWeightRimSoft.w = packHalf2x16(vec2(rimlightPower, softlighting));

	gAlbedoAlpha = diffuse;

	gSpecular.rgb = prop.specularColor * prop.specularStrength * specFactor;
	gSpecular.a = prop.shininess;

	gVertexColors.rgb = VertexColor;

	gEnvironment.rgb = cubeMap.rgb;
	gEnvironment.a = envMask;

	gEmissiveRefl = vec4(emissive.rgb, prop.envReflection);
	gLightMask = lightMask;
}
