#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matModelView;
uniform mat3 mv_normalMatrix;
uniform vec3 color;
uniform vec3 subColor;

uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowVertexColor;
uniform bool bShowVertexAlpha;

uniform bool bWireframe;
uniform bool bModelSpace;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;
layout(location = 4) in vec3 vertexColors;
layout(location = 5) in float vertexAlpha;
layout(location = 6) in vec2 vertexUV;
layout(location = 7) in float vertexMask;
layout(location = 8) in float vertexWeight;
layout(location = 10) in vec4 vertexPoseRotation;

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
out vec3 n;
out mat3 mv_tbn;

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

mat3 rotationFromQuaternion(in vec4 q)
{
	q = normalize(q);

	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;

	return mat3(1.0 - 2.0 * (yy + zz), 2.0 * (xy + wz), 2.0 * (xz - wy),
				2.0 * (xy - wz), 1.0 - 2.0 * (xx + zz), 2.0 * (yz + wx),
				2.0 * (xz + wy), 2.0 * (yz - wx), 1.0 - 2.0 * (xx + yy));
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

	n = vertexNormal;

	if (!bModelSpace)
	{
		vec3 mv_normal = mv_normalMatrix * n;
		vec3 mv_tangent = mv_normalMatrix * vertexTangent;
		vec3 mv_bitangent = mv_normalMatrix * vertexBitangent;

		mv_tbn = mat3(mv_bitangent.x, mv_bitangent.y, mv_bitangent.z,
                           mv_tangent.x, mv_tangent.y, mv_tangent.z,
                           mv_normal.x, mv_normal.y, mv_normal.z);
	}
	else
	{
		// Model space normal maps hold the normals of the unposed shape, so they are
		// turned along with the bones the pose moved this vertex with
		mv_tbn = mv_normalMatrix * rotationFromQuaternion(vertexPoseRotation);
	}
	viewDir = normalize(-vPos);
	lightFrontal = normalize(frontal.direction);
	lightDirectional0 = normalize(mat3(matView) * directional0.direction);
	lightDirectional1 = normalize(mat3(matView) * directional1.direction);
	lightDirectional2 = normalize(mat3(matView) * directional2.direction);

	if (!bShowTexture || bWireframe)
	{
		vColor *= clamp(vec4(color, 1.0), 0.0, 1.0);
	}

	if (!bWireframe)
	{
		vColor.rgb *= subColor;

		if (bShowMask)
		{
			maskFactor = 1.0 - vertexMask / 1.5;
		}

		if (bShowWeight)
		{
			weightColor = colorRamp(vertexWeight);
		}
	}
}
