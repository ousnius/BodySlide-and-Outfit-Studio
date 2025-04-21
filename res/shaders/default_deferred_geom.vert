#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;
layout(location = 4) in vec3 vertexColors;
layout(location = 5) in float vertexAlpha;
layout(location = 6) in vec2 vertexUV;
layout(location = 7) in float vertexMask;
layout(location = 8) in float vertexWeight;

uniform mat4 matModelView;
uniform mat4 matProjection;
uniform mat3 mv_normalMatrix;

uniform vec3 color;
uniform vec3 subColor;

uniform bool bModelSpace;

uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowVertexColor;
uniform bool bShowVertexAlpha;

out vec3 FragPos;      // View-space position
out vec3 VNormal;      // Vertex normal
out vec2 TexCoords;    // UVs
out mat3 TBN;          // For normal mapping

out float MaskFactor;
out float WeightFactor;
out vec3 VertexColor;
out float VertexAlpha;


void main() {
	vec3 viewPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	FragPos = viewPos;
	VNormal = vertexNormal;

	if (!bModelSpace)
	{
		// Transform normal, tangent, bitangent to view space
		vec3 N = normalize(mv_normalMatrix * vertexNormal);
		vec3 T = normalize(mv_normalMatrix * vertexTangent);
		vec3 B = normalize(mv_normalMatrix * vertexBitangent);
		TBN = mat3(B.x, B.y, B.z,
				   T.x, T.y, T.z,
				   N.x, N.y, N.z);
	}

	TexCoords = vertexUV;

	MaskFactor = 1.0;
	WeightFactor = -1.0;
	VertexColor = vec3(1.0, 1.0, 1.0);
	VertexAlpha = 1.0;

	if (bShowVertexColor)
	{
		VertexColor = vertexColors;
	}

	if (bShowVertexAlpha)
	{
		VertexAlpha = vertexAlpha;
	}

	if (!bShowTexture)
	{
		VertexColor *= clamp(color, 0.0, 1.0);
	}

	VertexColor *= subColor;

	if (bShowMask)
	{
		MaskFactor = 1.0 - vertexMask / 1.5;
	}

	if (bShowWeight)
	{
		WeightFactor = vertexWeight;
	}

	gl_Position = matProjection * vec4(viewPos, 1.0);
}

