#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matModelView;
uniform vec3 color;
uniform vec3 subColor;

uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowVertexColor;
uniform bool bShowVertexAlpha;

uniform bool bWireframe;
uniform bool bPoints;
uniform bool bModelSpace;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;
layout(location = 4) in vec3 vertexColors;
layout(location = 5) in float vertexAlpha;
layout(location = 6) in vec2 vertexUV;

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
	vColor = vec4(1.0, 1.0, 1.0, 1.0);
	vUV = vertexUV;

	if (bShowVertexColor)
	{
		vColor.rgb = vertexColors;
	}

	if (bShowVertexAlpha)
	{
		vColor.a = vertexAlpha;
	}

	// Eye-coordinate position of vertex
	vec3 vPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	gl_Position = matProjection * vec4(vPos, 1.0);

	t = vertexTangent;
	b = vertexBitangent;
	n = vertexNormal;

	if (!bModelSpace)
	{
		mat3 mv_normalMatrix = transpose(inverse(mat3(matModelView)));
		vec3 mv_normal = normalize(mv_normalMatrix * n);
		vec3 mv_tangent = normalize(mv_normalMatrix * t);
		vec3 mv_bitangent = normalize(mv_normalMatrix * b);

		mat3 mv_tbn = mat3(mv_bitangent.x, mv_tangent.x, mv_normal.x,
                           mv_bitangent.y, mv_tangent.y, mv_normal.y,
                           mv_bitangent.z, mv_tangent.z, mv_normal.z);

		mat3 m_normalMatrix = transpose(inverse(mat3(matModel)));
		vec3 m_normal = normalize(m_normalMatrix * n);
		vec3 m_tangent = normalize(m_normalMatrix * t);
		vec3 m_bitangent = normalize(m_normalMatrix * b);

		mat3 m_tbn = mat3(m_bitangent.x, m_tangent.x, m_normal.x,
                          m_bitangent.y, m_tangent.y, m_normal.y,
                          m_bitangent.z, m_tangent.z, m_normal.z);

		viewDir = normalize(mv_tbn * -vPos);
		lightFrontal = normalize(mv_tbn * frontal.direction);
		lightDirectional0 = normalize(m_tbn * directional0.direction);
		lightDirectional1 = normalize(m_tbn * directional1.direction);
		lightDirectional2 = normalize(m_tbn * directional2.direction);
	}
	else
	{
		viewDir = normalize(-vPos);
		lightFrontal = normalize(frontal.direction);
		lightDirectional0 = normalize(mat3(matView) * directional0.direction);
		lightDirectional1 = normalize(mat3(matView) * directional1.direction);
		lightDirectional2 = normalize(mat3(matView) * directional2.direction);
	}

	if (!bPoints)
	{
		if (!bShowTexture || bWireframe)
		{
			vColor *= clamp(vec4(color, 1.0), 0.0, 1.0);
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

	if (!bPoints && !bWireframe)
	{
		vColor.rgb *= subColor;

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
