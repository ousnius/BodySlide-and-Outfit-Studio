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

out vec3 lightFrontal;
out vec3 lightDirectional0;
out vec3 lightDirectional1;
out vec3 lightDirectional2;

out vec3 viewDir;
out vec3 t;
out vec3 b;
out vec3 n;

out float maskFactor;
out vec3 weightColor;
out vec3 segmentColor;

out vec4 vColor;
out vec2 vUV;

vec3 colorRamp(in float value)
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

	return vec3(r, g, b);
}

void main(void)
{
	// Initialization
	maskFactor = 1.0;
	weightColor = vec3(1.0, 1.0, 1.0);
	segmentColor = vec3(1.0, 1.0, 1.0);
	vColor = vec4(1.0, 1.0, 1.0, 1.0);
	vUV = vertexUV;
	
	// Eye-coordinate position of vertex
	vec3 vPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	gl_Position = matProjection * vec4(vPos, 1.0);
	
	t = vertexTangent;
	b = vertexBitangent;
	n = vertexNormal;

	mat3 normalMatrix = transpose(inverse(mat3(matModelView)));
	vec3 n_normal = normalize(normalMatrix * n);
	vec3 n_tangent = normalize(normalMatrix * t);
	vec3 n_bitangent = normalize(normalMatrix * b);
	
	mat3 tbn = mat3(n_tangent.x, n_bitangent.x, n_normal.x,
                    n_tangent.y, n_bitangent.y, n_normal.y,
                    n_tangent.z, n_bitangent.z, n_normal.z);
			   
	viewDir = normalize(tbn * -vPos);
	lightFrontal = normalize(tbn * frontal.direction);
	
	mat3 tbnDir = mat3(t.x, b.x, n.x,
                       t.y, b.y, n.y,
                       t.z, b.z, n.z);
					
	lightDirectional0 = normalize(tbnDir * directional0.direction);
	lightDirectional1 = normalize(tbnDir * directional1.direction);
	lightDirectional2 = normalize(tbnDir * directional2.direction);

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
