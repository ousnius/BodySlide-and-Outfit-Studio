#version 330

// Projects one face of a cube map out of an equirectangular HDRi. Run once per face into a
// framebuffer with that face attached, driven by fullscreentri.vert.

uniform sampler2D texDiffuse;

// Which face is being rendered, in the GL_TEXTURE_CUBE_MAP_POSITIVE_X + i order.
uniform int cubeFace;
// Scales the file's radiance so different HDRis land in a comparable range. Worked out on the CPU
// from the image's mean luminance, since the shader only ever sees one texel at a time.
uniform float exposure;

in vec2 uv;
out vec4 color;

const float PI = 3.14159265358979323846;

// The same filmic curve the mesh shaders end on, applied here instead of at every place the cube map
// is read. Nothing downstream is colour managed: diffuse textures are sampled as if the sRGB values
// they hold were linear, and the frame is shown without an encode. So the linear radiance the file
// holds is rolled off and encoded once, here, which leaves the generated cube map in the same space
// as one that came from a DDS file and lets both be read the same way.
vec3 tonemap(in vec3 x)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Direction the texel at uv on the given face looks along, in the same world space the mesh shaders
// reflect into. Matches the cube map convention: +Y is up, faces are laid out left-handed, and V
// runs downwards.
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

void main(void)
{
	vec3 dir = normalize(faceDirection(cubeFace, uv));

	// Equirectangular mapping: longitude across, latitude down.
	vec2 sphericalUV = vec2(atan(dir.z, dir.x) / (2.0 * PI) + 0.5, acos(clamp(dir.y, -1.0, 1.0)) / PI);

	vec3 radiance = texture(texDiffuse, sphericalUV).rgb * exposure;
	vec3 mapped = clamp(tonemap(radiance) / tonemap(vec3(1.0)), 0.0, 1.0);

	color = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}
