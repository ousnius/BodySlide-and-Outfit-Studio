#version 330

/*
 * BodySlide and Outfit Studio
 * Shaders by jonwd7 and ousnius
 * https://github.com/ousnius/BodySlide-and-Outfit-Studio
 * http://www.niftools.org/
 */

uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matModelViewInverse;
uniform mat3 mv_normalMatrix;

in vec3 viewDir;
in vec3 n;
in mat3 mv_tbn;

in float maskFactor;

in float vViewDepth;
in vec4 vColor;
in float pointVisible;
in float vBiasedDepth;

out vec4 fragColor;

vec3 normal = vec3(0.0);

void main(void)
{
	if (pointVisible < 0.5)
		discard;

	// Make round point
	if (dot(gl_PointCoord - 0.5, gl_PointCoord - 0.5) > 0.25)
		discard;

	// View-space biased depth (computed in the vertex shader). Pulls the point
	// slightly toward the camera so it stays visible above the wireframe at every
	// zoom level without a hard fade cutoff.
	gl_FragDepth = vBiasedDepth;

	vec4 color = vColor;
	color = clamp(color, 0.0, 1.0);

	fragColor = color;
}
