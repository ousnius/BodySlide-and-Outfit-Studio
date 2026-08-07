#version 330

struct Properties
{
	float alpha;
};
uniform Properties prop;

uniform bool bLighting;
uniform float ambient;

struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

in vec4 vertexColor;

in vec3 n;
in vec3 viewDir;
in vec3 lightFrontal;
in vec3 lightDirectional0;
in vec3 lightDirectional1;
in vec3 lightDirectional2;

out vec4 fragColor;

// A tighter, brighter highlight than the meshes get, because a primitive has no
// maps to read a specular factor out of and the highlight is what tells the eye
// which way a curved one is turned.
const float specularStrength = 0.3;
const float shininess = 40.0;

vec3 normal = vec3(0.0);
vec3 eyeDir = vec3(0.0);

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

// The same accumulation the mesh shaders do, so a lit primitive sits in the same
// light as the shapes around it instead of looking like it came from elsewhere.
void directionalLight(in DirectionalLight light, in vec3 lightDir, inout vec3 outDiffuse, inout vec3 outSpec)
{
	vec3 halfDir = normalize(lightDir + eyeDir);
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotH = max(dot(normal, halfDir), 0.0);

	outDiffuse += ambient + NdotL * light.diffuse;
	outSpec += specularStrength * pow(NdotH, shininess) * light.diffuse;
}

void main(void)
{
	vec4 color = vertexColor;

	// Only the primitives built with normals are shaded, so a round one reads as a
	// solid body with a near and a far side rather than as a flat silhouette. The
	// rest bind no normal attribute and keep the plain tint they are drawn in.
	if (bLighting && dot(n, n) > 0.0)
	{
		normal = normalize(n);
		eyeDir = normalize(viewDir);

		vec3 outDiffuse = vec3(0.0);
		vec3 outSpecular = vec3(0.0);

		directionalLight(frontal, lightFrontal, outDiffuse, outSpecular);
		directionalLight(directional0, lightDirectional0, outDiffuse, outSpecular);
		directionalLight(directional1, lightDirectional1, outDiffuse, outSpecular);
		directionalLight(directional2, lightDirectional2, outDiffuse, outSpecular);

		color.rgb = color.rgb * outDiffuse + outSpecular;
		color.rgb = tonemap(color.rgb) / tonemap(vec3(1.0));
	}

	color.a *= prop.alpha;
	color = clamp(color, 0.0, 1.0);

	fragColor = color;
}
