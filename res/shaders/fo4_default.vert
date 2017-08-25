#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform mat4 matProjection;
uniform mat4 matModelView;
uniform vec3 color;

uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowSegments;

uniform bool bWireframe;
uniform bool bPoints;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;
layout(location = 4) in vec3 vertexColors;
layout(location = 5) in vec2 vertexUV;

struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

out DirectionalLight lightFrontal;
out DirectionalLight lightDirectional0;
out DirectionalLight lightDirectional1;
out DirectionalLight lightDirectional2;

out vec3 viewDir;

out float maskFactor;
out vec4 weightColor;
out vec4 segmentColor;

out vec3 vPos;
smooth out vec4 vColor;
smooth out vec2 vUV;

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

void main(void)
{
	// Initialization
	maskFactor = 1.0;
	weightColor = vec4(1.0, 1.0, 1.0, 1.0);
	segmentColor = vec4(1.0, 1.0, 1.0, 1.0);
	vColor = vec4(1.0, 1.0, 1.0, 1.0);
	vUV = vertexUV;
	lightFrontal = frontal;
	lightDirectional0 = directional0;
	lightDirectional1 = directional1;
	lightDirectional2 = directional2;
	
	// Eye-coordinate position of vertex
	vPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	gl_Position = matProjection * vec4(vPos, 1.0);

	mat3 normalMatrix = transpose(inverse(mat3(matModelView)));
	vec3 normal = normalize(normalMatrix * vertexNormal);
	vec3 tangent = normalize(normalMatrix * vertexTangent);
	vec3 bitangent = normalize(normalMatrix * vertexBitangent);
	
	mat3 tbn = mat3(tangent.x, bitangent.x, normal.x,
                    tangent.y, bitangent.y, normal.y,
                    tangent.z, bitangent.z, normal.z);
			   
	viewDir = normalize(tbn * -vPos);
	lightFrontal.direction = normalize(tbn * lightFrontal.direction);
	
	mat3 tbnDir = mat3(vertexTangent.x, vertexBitangent.x, vertexNormal.x,
                       vertexTangent.y, vertexBitangent.y, vertexNormal.y,
                       vertexTangent.z, vertexBitangent.z, vertexNormal.z);
					
	lightDirectional0.direction = normalize(tbnDir * lightDirectional0.direction);
	lightDirectional1.direction = normalize(tbnDir * lightDirectional1.direction);
	lightDirectional2.direction = normalize(tbnDir * lightDirectional2.direction);

	if (!bPoints)
	{
		if (!bShowTexture || bWireframe)
		{
			vColor = clamp(vec4(color, 1.0), 0.0, 1.0);
		}
	}
	else
	{
		if (vertexColors.x > 0.0)
		{
			vColor = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else
		{
			vColor = vec4(0.0, 1.0, 0.0, 1.0);
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
}
