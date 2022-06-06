#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform sampler2D texDiffuse;
uniform sampler2D texGlowmap;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;
uniform bool bPoints;

uniform bool bGlowmap;

uniform mat4 matModelView;

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
float specFactor = 0.0;

vec2 uv = vec2(0.0);
vec3 albedo = vec3(0.0);
vec3 emissive = vec3(0.0);

vec4 baseMap = vec4(0.0);

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
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotH = max(dot(normal, halfDir), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);

	outDiffuse += ambient + NdotL * light.diffuse;
	outSpec += clamp(prop.specularColor * prop.specularStrength * specFactor * pow(NdotH, prop.shininess), 0.0, 1.0) * light.diffuse;
}

void main(void)
{
	uv = vUV * prop.uvScale + prop.uvOffset;
	vec4 color = vColor;
	albedo = vColor.rgb;

	if (!bPoints)
	{
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
			}

			if (bLightEnabled)
			{
				// Lighting with or without textures
				vec3 outDiffuse = vec3(0.0);
				vec3 outSpecular = vec3(0.0);

				// Start off neutral
				normal = normalize(mv_tbn * vec3(0.0, 0.0, 0.5));
				specFactor = 0.0;

				directionalLight(frontal, lightFrontal, outDiffuse, outSpecular);
				directionalLight(directional0, lightDirectional0, outDiffuse, outSpecular);
				directionalLight(directional1, lightDirectional1, outDiffuse, outSpecular);
				directionalLight(directional2, lightDirectional2, outDiffuse, outSpecular);

				// Emissive
				emissive += prop.emissiveColor * prop.emissiveMultiple;

				// Glowmap
				if (bGlowmap)
				{
					vec4 glowMap = texture(texGlowmap, uv);
					emissive *= glowMap.rgb;
				}

				color.rgb = albedo * (outDiffuse + emissive) + outSpecular;
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
	}
	else
	{
		// Calculate normal from point coord
		vec2 norm = gl_PointCoord * 2.0 - vec2(1.0);
		float mag = dot(norm, norm);
		if (mag > 1.0)
			discard; // Kill pixels outside point

		color.a = 1.0 - mag;
		color = clamp(color, 0.0, 1.0);
	}

	fragColor = color;

	if (!bPoints && !bWireframe)
	{
		if (alphaThreshold != -1.0f)
			if (fragColor.a <= alphaThreshold) // GL_GREATER
				discard;

		fragColor.a *= prop.alpha;
	}
}
