#version 330
uniform mat4 matProjection;
uniform mat4 matView;
uniform vec3 color;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;

uniform bool bWireframe;
uniform bool bPoints;
uniform bool bLighting;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 6) in vec2 vertexUV;

out vec3 N;
out vec3 v;
out vec3 vPos;
out vec2 uv;

void main(void)
{
	N = vertexNormal;
	vPos =  vec3(matView * vec4(vertexPosition, 1.0));
	v = vec3(matView * vec4(vertexUV, 0.0, 1.0));
	
	uv = vertexUV;
	gl_Position = matProjection * vec4(v, 1.0);
}
