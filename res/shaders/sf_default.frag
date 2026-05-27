#version 330

/*
 * BodySlide and Outfit Studio
 * Starfield diffuse preview shader
 */

uniform sampler2D texDiffuse;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bWireframe;

struct Properties
{
	vec2 uvOffset;
	vec2 uvScale;
	float alpha;
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

in vec3 viewNormal;
in float maskFactor;
in vec3 weightColor;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

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

void addDirectionalLight(in vec3 lightDiffuse, in vec3 lightDir, in vec3 normal, inout vec3 outDiffuse)
{
	float NdotL = max(dot(normal, lightDir), 0.0);
	outDiffuse += ambient + NdotL * lightDiffuse;
}

void main(void)
{
	vec2 uv = vUV * prop.uvScale + prop.uvOffset;
	vec4 color = vColor;

	if (!bWireframe)
	{
		if (bShowTexture)
		{
			vec4 diffuseMap = texture(texDiffuse, uv);
			color.rgb *= diffuseMap.rgb;
			color.a *= diffuseMap.a;
		}

		if (bLightEnabled)
		{
			vec3 normal = normalize(viewNormal);
			vec3 outDiffuse = vec3(0.0);

			addDirectionalLight(frontal.diffuse, lightFrontal, normal, outDiffuse);
			addDirectionalLight(directional0.diffuse, lightDirectional0, normal, outDiffuse);
			addDirectionalLight(directional1.diffuse, lightDirectional1, normal, outDiffuse);
			addDirectionalLight(directional2.diffuse, lightDirectional2, normal, outDiffuse);

			color.rgb *= outDiffuse;
			color.rgb += ambient * color.rgb;
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
			if (fragColor.a <= alphaThreshold)
				discard;

		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		gl_FragDepth = gl_FragCoord.z - 0.00001f;
	}
}