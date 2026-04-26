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

// Density-aware point sizing inputs (only used when bAdjustPointSize is true).
uniform float pointSpacingWS;     // Average edge length in world space (0 disables density scaling).
uniform vec2  viewportSizePx;     // Framebuffer size in pixels.
uniform float pointSizeMinPx;     // Minimum point size in pixels.
uniform float pointSizeMaxPx;     // Maximum point size in pixels.
uniform float pointSizeScale;     // Multiplier on the screen-space neighbor spacing.

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
out float vBiasedDepth;


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

	// Depth bias to keep the point above the underlying wireframe / surface without
	// z-fighting. The shift is done in view space proportional to view depth, which
	// produces a roughly constant window-space offset at every zoom level (unlike a
	// fixed window-space bias, which has to fade out and creates a visible cutoff).
	float biasWS = max(vViewDepth * 0.005, 1e-3);
	vec3 vPosBiased = vec3(vPos.xy, vPos.z + biasWS); // camera looks down -Z, so +Z is toward camera
	vec4 clipBiased = matProjection * vec4(vPosBiased, 1.0);
	vBiasedDepth = (clipBiased.z / clipBiased.w) * 0.5 + 0.5;

	if (bAdjustPointSize)
	{
		// Convert pointSpacingWS (world-space neighbor distance) into a screen-space pixel
		// distance at the vertex's depth. matProjection[1][1] is the perspective Y scale
		// (cot(fovY/2)); multiplying by half the framebuffer height converts NDC to pixels.
		float screenSpacingPx = pointSpacingWS * matProjection[1][1] * (viewportSizePx.y * 0.5) / max(vViewDepth, 1e-4);

		// Fall back to the maximum size if no spacing was provided.
		float sizePx = (pointSpacingWS > 0.0) ? (screenSpacingPx * pointSizeScale) : pointSizeMaxPx;
		gl_PointSize = clamp(sizePx, pointSizeMinPx, pointSizeMaxPx);
	}

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
		vColor = vec4(subColor.rgb, 0.7);
	}
	else
	{
		vColor = vec4(color.rgb, 0.7);
	}
}
