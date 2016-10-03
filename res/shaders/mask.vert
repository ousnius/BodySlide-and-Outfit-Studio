#version 330
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;

uniform mat4 matProjection;
uniform mat4 matModelView;
uniform vec3 color = vec3(1.0);

struct LightSource
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 position;
};
uniform LightSource lightSource0;
uniform LightSource lightSource1;
uniform LightSource lightSource2;

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};
uniform Material material;

uniform bool bLightEnabled = true;
uniform bool bShowTexture = true;
uniform bool bShowMask = true;
uniform bool bShowWeight = false;
uniform bool bShowSegments = false;

uniform bool bWireframe;
uniform bool bPoints;
uniform bool bLighting;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexColors;
layout(location = 3) in vec2 vertexUV;

out vec4 vertexColor;

out float maskFactor;
out vec4 weightColor;
out vec4 segmentColor;
out vec2 UV;

void directionalLight(in LightSource light, in vec3 normal)
{
	// Normal dot light direction
	float nDotVP = max(0.0, dot(normal, normalize(light.position)));

	// Normal dot light half vector
	float nDotHV = max(0.0, dot(normal, normalize(light.position + vec3(0.0, 0.0, 1.0))));
	
	// Power factor
	float pf = 0.0;
	if (nDotVP != 0.0)
	{
		pf = pow(nDotHV, material.shininess);
	}
	
	Ambient += vec4(light.ambient, 1.0);
	Diffuse += vec4(light.diffuse, 1.0) * nDotVP;
	Specular += vec4(light.specular, 1.0) * pf;
}

vec4 colorRamp(in float value)
{
	float r;
	float g;
	float b;

	if (value <= 0.0f)
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

	return vec4(r, g, b, 1.0);
}

void flight(in vec3 normal, in vec4 pos)
{
	vec4 vcolor = vec4(color, 1.0);

	if (bShowTexture && !bWireframe)
	{
		vcolor = vec4(1.0);
	}

	if (bLightEnabled && bLighting)
	{
		Ambient = vec4(0.0);
		Diffuse = vec4(0.0);
		Specular = vec4(0.0);

		directionalLight(lightSource0, normal);
		directionalLight(lightSource1, normal);
		directionalLight(lightSource2, normal);

		vcolor = Ambient * vec4(material.ambient, 1.0) +
			Diffuse * vec4(material.diffuse, 1.0) +
			Specular * vec4(material.specular, 1.0);

		if (!bShowTexture)
		{
			vcolor *= vec4(color, 1.0);
		}

		vcolor += vec4(vec3(0.2), 0.0);
	}

	vcolor = clamp(vcolor, 0.0, 1.0);
	vertexColor = vcolor;
}

void main(void)
{
	// Initialization
	maskFactor = 1.0;
	weightColor = vec4(1.0, 1.0, 1.0, 1.0);
	segmentColor = vec4(1.0, 1.0, 1.0, 1.0);
	UV = vertexUV;

	// Eye-coordinate position of vertex
	vec4 pos = matProjection * matModelView * vec4(vertexPosition.x, vertexPosition.y, vertexPosition.z, 1.0);

	if (!bPoints)
	{
		// Transform and Light
		mat4 normalMatrix = transpose(inverse(matModelView));
		vec3 transformedNormal = normalize(mat3(normalMatrix) * vertexNormal);
		flight(transformedNormal, pos);
	}
	else
	{
		if (vertexColors.x > 0.0)
		{
			vertexColor = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else
		{
			vertexColor = vec4(0.0, 1.0, 0.0, 1.0);
		}
	}

	if (bShowSegments)
	{
		weightColor = colorRamp(vertexColors.g);
		segmentColor = colorRamp(vertexColors.b);
	}
	else
	{
		if (bShowMask)
		{
			maskFactor = 1.0 - vertexColors.r / 1.5;
		}
		if (bShowWeight)
		{
			weightColor = colorRamp(vertexColors.g);
		}
	}

	gl_Position = pos;
}
