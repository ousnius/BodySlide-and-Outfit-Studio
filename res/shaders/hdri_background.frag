#version 330

// Draws the environment cube map as the viewport background, driven by fullscreentri.vert.

uniform samplerCube texCubemap;

uniform mat4 matProjection;
uniform mat4 matView;

in vec2 uv;
out vec4 color;

void main(void)
{
	// Unproject the far plane to get the view ray, then rotate it into the world space the cube map
	// lives in. The projection handed in here is always a perspective one - an orthographic one has
	// no divide, so every ray would come out parallel and the background would be one flat colour.
	vec4 viewPos = inverse(matProjection) * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
	vec3 viewDir = normalize(viewPos.xyz / viewPos.w);
	vec3 worldDir = transpose(mat3(matView)) * viewDir;

	// The cube map was tone mapped and encoded when it was generated, the same way a cube map from a
	// DDS file already is, so what it holds goes straight to the screen.
	color = vec4(textureLod(texCubemap, worldDir, 0.0).rgb, 1.0);
}
