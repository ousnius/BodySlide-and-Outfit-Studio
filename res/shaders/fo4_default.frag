#version 330
uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texCubemap;
uniform sampler2D texEnvMask;
uniform sampler2D texSpecular;
uniform sampler2D texBacklight;

uniform mat4 matModelView;

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
};
uniform Properties prop;

struct FrontalLight
{
	vec3 diffuse;
};
struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

uniform FrontalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;
uniform float ambient;

in float maskFactor;
in vec4 weightColor;
in vec4 segmentColor;

in vec3 vPos;
smooth in vec4 vColor;
smooth in vec2 vUV;
smooth in vec3 vNormal;

out vec4 fragColor;

vec2 uv = vec2(0.0);
vec3 emissive = vec3(0.0);

vec4 baseMap = vec4(0.0);
vec4 normalMap = vec4(0.0);
vec4 specMap = vec4(0.0);
vec4 backlightMap = vec4(0.0);
vec4 envMask = vec4(0.0);

// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(in vec3 N, in vec3 p)
{
    // Get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
	
    // Solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	
    // Construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

vec3 perturb_normal(in vec3 N, in vec3 V)
{
	// Assume N, the interpolated vertex normal and V, the view vector (vertex to eye)
	vec3 map = normalMap.rgb;
	map = map * 255.0 / 127.0 - 128.0 / 127.0;
	mat3 TBN = cotangent_frame(N, -V);
	return normalize(TBN * map);
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

void directionalLight(in DirectionalLight light, in vec3 inNormal, inout vec3 diffuse, inout vec3 spec, inout vec3 cube, in bool front)
{
	mat3 normalMatrix = transpose(inverse(mat3(matModelView)));
	vec3 normal = inNormal;
	float specFactor = 0.0;
	
	vec3 lightDir;
	if (front)
	{
		lightDir = light.direction * normalMatrix;
	}
	else
	{
		lightDir = light.direction;
	}
	
	if (bShowTexture)
	{
		if (bNormalMap)
		{
			if (bModelSpace)
			{
				// No proper FO4 model space map rendering yet
				//normal = normalize(normalMap.rgb * 2.0 - 1.0);
				//normal.r = -normal.r;
				
				if (bSpecular)
				{
					// No proper FO4 specular map rendering yet
					//specFactor = specMap.r;
				}
			}
			else
			{
				// No proper FO4 tangent space map rendering yet
				//normal = normalMatrix * perturb_normal(inverse(normalMatrix) * normal, vPos);
				//specFactor = normalMap.a;
			}
		}
	}
	
	float NdotL = max(0.0, dot(normal, lightDir));
	float NdotNegL = max(0.0, dot(normal, -lightDir));
	float NdotH = max(0.0, dot(normal * inverse(normalMatrix), normalize(lightDir - vPos)));
	
	// No proper FO4 specular yet
	//spec += clamp(prop.specularColor * prop.specularStrength * specFactor * pow(NdotH, prop.shininess), 0.0, 1.0) * light.diffuse * NdotL;
	diffuse += NdotL * light.diffuse;
	
	if (bBacklight)
	{
		vec3 backlight = backlightMap.rgb;
		backlight *= NdotNegL;
		
		emissive += backlight * light.diffuse;
	}
	
	if (bCubemap)
	{
		vec3 reflected = mat3(inverse(matModelView)) * reflect(lightDir, normal);
		vec4 cubeMap = texture(texCubemap, reflected);
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
		
		cube += cubeMap.rgb;
	}
}

void frontalLight(in FrontalLight light, in vec3 inNormal, inout vec3 diffuse, inout vec3 spec, inout vec3 cube)
{
	DirectionalLight dirLight;
	dirLight.diffuse = light.diffuse;
	dirLight.direction = vec3(0.0, 0.0, 1.0);
	
	directionalLight(dirLight, inNormal, diffuse, spec, cube, true);
}

void main(void)
{
	uv = vUV * prop.uvScale + prop.uvOffset;
	vec4 color = vColor;
	vec3 albedo = vColor.rgb;
	
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
				
				if (bLightEnabled)
				{
					if (bNormalMap)
					{
						normalMap = texture(texNormal, uv);
						
						if (bModelSpace)
						{
							if (bSpecular)
							{
								// Dedicated Specular Map
								specMap = texture(texSpecular, uv);
							}
						}
					}
					
					if (bBacklight)
					{
						backlightMap = texture(texBacklight, uv);
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
				vec3 outCube = vec3(0.0);
				
				frontalLight(frontal, vNormal, outDiffuse, outSpecular, outCube);
				directionalLight(directional0, vNormal, outDiffuse, outSpecular, outCube, false);
				directionalLight(directional1, vNormal, outDiffuse, outSpecular, outCube, false);
				directionalLight(directional2, vNormal, outDiffuse, outSpecular, outCube, false);
				
				outDiffuse += ambient;
				albedo += outCube;
				
				if (bShowTexture)
				{
					// Hack for FO4 brightness
					albedo *= vec3(1.4, 1.4, 1.4);
				}
				
				if (bEmissive)
				{
					emissive += prop.emissiveColor * prop.emissiveMultiple;
				}
				
				color.rgb = albedo * (outDiffuse + emissive) + outSpecular;
			}
			else if (bShowTexture)
			{
				// Diffuse texture without lighting
				color.rgb = albedo;
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
		vec2 norm;
		norm = gl_PointCoord * 2.0 - vec2(1.0); 
		float mag = dot(norm, norm);
		if (mag > 1.0) 
			discard; // Kill pixels outside point

		color.a = 1.0 - mag;
		color = clamp(color, 0.0, 1.0);
	}
	
	fragColor = color;
	fragColor.a *= prop.alpha;
	
	if (fragColor.a < 0.1)
	{
		discard;
	}
}
