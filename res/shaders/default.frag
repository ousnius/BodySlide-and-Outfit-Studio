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
uniform sampler2D texBacklight;
uniform sampler2D texLightmask;
uniform sampler2D texGlowmap;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowSegments;
uniform bool bWireframe;
uniform bool bPoints;

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

in DirectionalLight lightFrontal;
in DirectionalLight lightDirectional0;
in DirectionalLight lightDirectional1;
in DirectionalLight lightDirectional2;

in vec3 viewDir;
in vec3 t;
in vec3 b;
in vec3 n;

in float maskFactor;
in vec4 weightColor;
in vec4 segmentColor;

in vec3 vPos;
smooth in vec4 vColor;
smooth in vec2 vUV;

out vec4 fragColor;

vec3 normal = vec3(0.0);
float specFactor = 0.0;

vec2 uv = vec2(0.0);
vec3 albedo = vec3(0.0);
vec3 emissive = vec3(0.0);

vec4 baseMap = vec4(0.0);
vec4 normalMap = vec4(0.0);
vec4 specMap = vec4(0.0);
vec4 envMask = vec4(0.0);
vec4 backlightMap = vec4(0.0);

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

void directionalLight(in DirectionalLight light, inout vec3 outDiffuse, inout vec3 outSpec)
{
	vec3 half = normalize(light.direction + viewDir);
	float NdotL = max(dot(normal, light.direction), 0.0);
	float NdotH = max(dot(normal, half), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);
	
	outDiffuse += ambient + NdotL * light.diffuse;
	outSpec += clamp(prop.specularColor * prop.specularStrength * specFactor * pow(NdotH, prop.shininess), 0.0, 1.0) * light.diffuse;
	
	// Back lighting not really useful for the current light setup of multiple directional lights
	//if (bBacklight && bShowTexture)
	//{
	//	float NdotNegL = max(dot(normal, -light.direction), 0.0);
	//	vec3 backlight = backlightMap.rgb * NdotNegL * light.diffuse;
	//	emissive += backlight;
	//}
	
	vec3 lightMask = normalize(vec3(0.0, 0.0, 0.5));
	if (bRimlight || bSoftlight)
	{
		lightMask = texture(texLightmask, uv).rgb;
	}
	
	// Rim lighting not really useful for the current light setup of multiple directional lights
	//if (bRimlight)
	//{
	//	vec3 rim = lightMask * pow(vec3(1.0 - NdotV), vec3(prop.rimlightPower));
	//	rim *= smoothstep(-0.2, 1.0, dot(-light.direction, viewDir));
	//	emissive += rim * light.diffuse;
	//}
	
	// Soft Lighting
	if (bSoftlight)
	{
		float wrap = (dot(normal, light.direction) + prop.softlighting) / (1.0 + prop.softlighting);
		vec3 soft = max(wrap, 0.0) * lightMask * smoothstep(1.0, 0.0, NdotL);
		soft *= sqrt(clamp(prop.softlighting, 0.0, 1.0));
		emissive += soft * light.diffuse;
	}
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
				
				if (bLightEnabled)
				{
					if (bNormalMap)
					{
						normalMap = texture(texNormal, uv);
						
						if (bModelSpace && bSpecular)
						{
							// Dedicated Specular Map
							specMap = texture(texSpecular, uv);
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
					
					if (bBacklight)
					{
						backlightMap = texture(texBacklight, uv);
					}
				}
			}
			
			if (bLightEnabled)
			{
				// Lighting with or without textures
				vec3 outDiffuse = vec3(0.0);
				vec3 outSpecular = vec3(0.0);
				
				// Start off neutral
				normal = normalize(vec3(0.0, 0.0, 0.5));
				specFactor = 0.0;
				
				if (bModelSpace)
				{
					// Vertex normal for shading with disabled maps
					normal = mat3(matModelView) * n;
				}
				
				if (bShowTexture)
				{
					if (bNormalMap)
					{
						if (bModelSpace)
						{
							// Model Space Normal Map
							normal = normalize(normalMap.rgb * 2.0 - 1.0);
							normal.r = -normal.r;
							normal = mat3(matModelView) * normal;
							
							if (bSpecular)
							{
								specFactor = specMap.r;
							}
						}
						else
						{
							// Tangent Space Normal Map
							normal = normalize(normalMap.rgb * 2.0 - 1.0);
							
							if (bSpecular)
							{
								specFactor = normalMap.a;
							}
						}
					}
				}
				
				directionalLight(lightFrontal, outDiffuse, outSpecular);
				directionalLight(lightDirectional0, outDiffuse, outSpecular);
				directionalLight(lightDirectional1, outDiffuse, outSpecular);
				directionalLight(lightDirectional2, outDiffuse, outSpecular);
				
				if (bCubemap && bShowTexture)
				{
					vec3 reflected = reflect(-viewDir, normal);
					vec3 reflectedVS = b * reflected.x + t * reflected.y + n * reflected.z;
					vec3 reflectedWS = mat3(matModelView) * reflectedVS;
					
					vec4 cubeMap = texture(texCubemap, reflectedWS);
					cubeMap.rgb *= prop.envReflection;
					
					if (bEnvMask)
					{
						cubeMap.rgb *= envMask.r;
					}
					else
					{
						// No env mask, use specular factor (0.0 if no normal map either)
						cubeMap.rgb *= specFactor;
					}
					
					albedo += cubeMap.rgb;
				}
				
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
				
				color.rgb = albedo * (outDiffuse + emissive) + outSpecular;
			}
			
			if (bShowSegments)
			{
				if (segmentColor.r != 0.0 && segmentColor.g != 0.0 && segmentColor.b != 0.0 &&
					segmentColor.rg != normalize(segmentColor.rg) &&
					segmentColor.rb != normalize(segmentColor.rb) &&
					segmentColor.gb != normalize(segmentColor.gb))
				{
					color *= weightColor;
				}
				else
				{
					color *= segmentColor;
				}
			}
			else
			{
				if (bShowMask)
				{
					color.rgb *= maskFactor;
				}
				
				if (bShowWeight)
				{
					color *= weightColor;
				}
			}
		}
		else
		{
			color.rgb = vec3(1.0) - color.rgb;
		}
		
		color.rgb = tonemap(color.rgb) / tonemap(vec3(1.0));
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
	fragColor.a *= prop.alpha;
	
	if (fragColor.a < alphaThreshold)
		discard;
}
