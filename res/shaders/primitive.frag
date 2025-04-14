#version 330

struct Properties
{
	float alpha;
};
uniform Properties prop;

in vec4 vertexColor;
out vec4 fragColor;

void main(void)
{
	vec4 color = vertexColor;

	color.a *= prop.alpha;
	color = clamp(color, 0.0, 1.0);

	fragColor = color;
}
