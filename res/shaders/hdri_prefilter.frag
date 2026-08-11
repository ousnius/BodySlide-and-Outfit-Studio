#version 330

// Builds one mip of the environment cube map by convolving the level above it with a GGX lobe, so
// that picking a mip by roughness gives a reflection blurred the way that roughness would blur it.
// Run once per face per level, driven by fullscreentri.vert.

uniform samplerCube texCubemap;

// Which face is being rendered, in the GL_TEXTURE_CUBE_MAP_POSITIVE_X + i order.
uniform int cubeFace;
// Roughness this level stands for. The source is the previous level rather than the sharpest one,
// so the blur accumulates down the chain and each step only has to widen it a little.
uniform float roughness;

in vec2 uv;
out vec4 color;

const float PI = 3.14159265358979323846;
const uint SAMPLE_COUNT = 64u;

vec3 faceDirection(in int face, in vec2 faceUV)
{
	vec2 p = faceUV * 2.0 - 1.0;

	if (face == 0) return vec3( 1.0,   -p.y, -p.x);		// +X
	if (face == 1) return vec3(-1.0,   -p.y,  p.x);		// -X
	if (face == 2) return vec3( p.x,    1.0,  p.y);		// +Y
	if (face == 3) return vec3( p.x,   -1.0, -p.y);		// -Y
	if (face == 4) return vec3( p.x,   -p.y,  1.0);		// +Z
	return vec3(-p.x, -p.y, -1.0);						// -Z
}

// Van der Corput radical inverse, the second dimension of a Hammersley set.
float radicalInverseVdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

// Half vector drawn from the GGX distribution around N, in world space.
vec3 importanceSampleGGX(in vec2 Xi, in float rough, in vec3 N)
{
	float a = rough * rough;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, N));
	vec3 tangentY = cross(N, tangentX);

	return tangentX * H.x + tangentY * H.y + N * H.z;
}

void main(void)
{
	// Prefiltering assumes the surface is viewed head on, which is what makes a single cube map
	// usable for every view direction. N, V and R all collapse into the texel's own direction.
	vec3 N = normalize(faceDirection(cubeFace, uv));

	vec3 sum = vec3(0.0);
	float totalWeight = 0.0;

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		vec2 Xi = vec2(float(i) / float(SAMPLE_COUNT), radicalInverseVdC(i));
		vec3 H = importanceSampleGGX(Xi, roughness, N);
		vec3 L = normalize(2.0 * dot(N, H) * H - N);

		// Samples in the lobe's far half point away from the surface and carry no light.
		float NdotL = dot(N, L);
		if (NdotL > 0.0)
		{
			sum += textureLod(texCubemap, L, 0.0).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	color = vec4(totalWeight > 0.0 ? sum / totalWeight : textureLod(texCubemap, N, 0.0).rgb, 1.0);
}
