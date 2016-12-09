#version 330
uniform mat4 matProjection;
uniform mat4 matModelView;
uniform vec3 color;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowSegments;

uniform bool bWireframe;
uniform bool bPoints;
uniform bool bLighting;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec3 vertexColors;
layout(location = 3) in vec2 vertexUV;

out float maskFactor;
out vec4 weightColor;
out vec4 segmentColor;

out vec3 vPos;
smooth out vec4 vColor;
smooth out vec2 vUV;
smooth out vec3 vNormal;

vec4 colorRamp(in float value)
{
	float r;
	float g;
	float b;

	if (value <= 0.0f)
	{
		r = g = b = 1.0;
	}
	else if (value <= 0.25)
	{
		r = 0.0;
		b = 1.0;
		g = value / 0.25;
	}
	else if (value <= 0.5)
	{
		r = 0.0;
		g = 1.0;
		b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
	}
	else if (value <= 0.75)
	{
		r = (value - 0.5) / 0.25;
		g = 1.0;
		b = 0.0;
	}
	else
	{
		r = 1.0;
		g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
		b = 0.0;
	}

	return vec4(r, g, b, 1.0);
}

void main(void)
{
	// Initialization
	maskFactor = 1.0;
	weightColor = vec4(1.0, 1.0, 1.0, 1.0);
	segmentColor = vec4(1.0, 1.0, 1.0, 1.0);
	vColor = vec4(1.0, 1.0, 1.0, 1.0);
	vUV = vertexUV;
	vNormal = vertexNormal;
	
	// Eye-coordinate position of vertex
	vPos = vec3(matModelView * vec4(vertexPosition, 1.0));
	gl_Position = matProjection * vec4(vPos, 1.0);

	if (!bPoints)
	{
		if (!bShowTexture || bWireframe)
		{
			vColor = clamp(vec4(color, 1.0), 0.0, 1.0);
		}
	}
	else
	{
		if (vertexColors.x > 0.0)
		{
			vColor = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else
		{
			vColor = vec4(0.0, 1.0, 0.0, 1.0);
		}
	}

	if (bShowSegments)
	{
		weightColor = colorRamp(vertexColors.g);
		segmentColor = colorRamp(vertexColors.b);
	}
	else
	{
		if (bShowMask)
		{
			maskFactor = 1.0 - vertexColors.r / 1.5;
		}
		if (bShowWeight)
		{
			weightColor = colorRamp(vertexColors.g);
		}
	}
}
