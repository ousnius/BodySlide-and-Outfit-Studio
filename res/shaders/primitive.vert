#version 330
uniform mat4 matProjection;
uniform mat4 matModelView;
uniform vec3 color = vec3(1.0);

uniform bool bShowVertexColor;

layout(location = 0) in vec3 vertexPosition;
layout(location = 4) in vec3 vertexColors;

out vec4 vertexColor;

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
}
