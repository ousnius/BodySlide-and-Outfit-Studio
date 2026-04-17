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
uniform bool bAdjustPointSize;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;
layout(location = 4) in vec3 vertexColors;
layout(location = 5) in float vertexAlpha;
layout(location = 6) in vec2 vertexUV;
layout(location = 7) in float vertexMask;
layout(location = 8) in float vertexWeight;

out vec3 viewDir;
out vec3 n;
out mat3 mv_tbn;

out float maskFactor;

out vec4 vColor;
out vec2 vUV;
out float pointVisible;
out float vViewDepth;


void main(void)
{
	// Initialization
	maskFactor = 1.0;
	vColor = vec4(1.0, 1.0, 1.0, 1.0);
	vUV = vertexUV;

	// Eye-coordinate position of vertex
	vec3 vPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	vViewDepth = -vPos.z; // positive depth (camera looks down -Z)

	gl_Position = matProjection * vec4(vPos, 1.0);

	if (bAdjustPointSize)
		gl_PointSize = clamp(30.0 / vViewDepth, 2.0, 12.0); // Shrinks with distance

	n = vertexNormal;

	vec3 mv_normal = mv_normalMatrix * n;
	vec3 mv_tangent = mv_normalMatrix * vertexTangent;
	vec3 mv_bitangent = mv_normalMatrix * vertexBitangent;

	mv_tbn = mat3(mv_bitangent.x, mv_bitangent.y, mv_bitangent.z,
					   mv_tangent.x, mv_tangent.y, mv_tangent.z,
					   mv_normal.x, mv_normal.y, mv_normal.z);

	viewDir = normalize(-vPos);
	pointVisible = step(0.0, dot(mv_normal, viewDir)); // 1 if front-facing, 0 if back

	if (vertexMask > 0.0)
	{
		vColor = vec4(subColor.rgb, 0.5);
	}
	else
	{
		vColor = vec4(color.rgb, 0.5);
	}
}
