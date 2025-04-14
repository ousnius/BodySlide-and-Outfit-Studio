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

out vec4 fragColor;

vec3 normal = vec3(0.0);

void main(void)
{
	if (pointVisible < 0.5)
		discard;

	// Make round point
	if (dot(gl_PointCoord - 0.5, gl_PointCoord - 0.5) > 0.25)
		discard;

	// Only apply full bias when very close to the camera
	float depthBiasMax = 1e-3;
	float fadeStart = 0.1;   // where fading starts (same as znear)
	float fadeEnd = 15.0;    // where bias fully fades (15 world units away)

	float fadeFactor = clamp(1.0 - (vViewDepth - fadeStart) / (fadeEnd - fadeStart), 0.0, 1.0);
	float scaledBias = depthBiasMax * fadeFactor;

	gl_FragDepth = gl_FragCoord.z - scaledBias;

	vec4 color = vColor;
	color = clamp(color, 0.0, 1.0);

	fragColor = color;

	// Visualize scaled depth bias
	//fragColor = vec4(vec3(scaledBias * 1000.0), 1.0);
}
