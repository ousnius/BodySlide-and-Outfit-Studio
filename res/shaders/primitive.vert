#version 330
uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matModelView;
uniform mat3 mv_normalMatrix;
uniform vec3 color = vec3(1.0);

uniform bool bShowVertexColor;

struct DirectionalLight
{
	vec3 diffuse;
	vec3 direction;
};

uniform DirectionalLight frontal;
uniform DirectionalLight directional0;
uniform DirectionalLight directional1;
uniform DirectionalLight directional2;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 4) in vec3 vertexColors;

out vec4 vertexColor;

// Shading inputs for the primitives that ask to be lit. The lines, points and
// gizmos bind no normal attribute, so "n" stays zero for them and the fragment
// shader leaves them at their flat tint.
out vec3 n;
out vec3 viewDir;
out vec3 lightFrontal;
out vec3 lightDirectional0;
out vec3 lightDirectional1;
out vec3 lightDirectional2;

void main(void)
{
	// Eye-coordinate position of vertex
	vec3 pos = vec3(matModelView * vec4(vertexPosition, 1.0));
	gl_Position = matProjection * vec4(pos, 1.0);

	if (bShowVertexColor)
	{
		vertexColor = vec4(vertexColors, 1.0);
	}
	else
	{
		vertexColor = vec4(color, 1.0);
	}

	vertexColor = clamp(vertexColor, 0.0, 1.0);

	n = mv_normalMatrix * vertexNormal;

	viewDir = normalize(-pos);
	lightFrontal = normalize(frontal.direction);
	lightDirectional0 = normalize(mat3(matView) * directional0.direction);
	lightDirectional1 = normalize(mat3(matView) * directional1.direction);
	lightDirectional2 = normalize(mat3(matView) * directional2.direction);
}
