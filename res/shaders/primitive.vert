#version 330
uniform mat4 matProjection;
uniform mat4 matModelView;
uniform vec3 color = vec3(1.0);

layout(location = 0) in vec3 vertexPosition;

out vec4 vertexColor;

void main(void)
{
	// Eye-coordinate position of vertex
	vec4 pos = matProjection * matModelView * vec4(vertexPosition.x, vertexPosition.y, vertexPosition.z, 1.0);

	vertexColor = vec4(color, 1.0);
	vertexColor = clamp(vertexColor, 0.0, 1.0);
	gl_Position = pos;
}
