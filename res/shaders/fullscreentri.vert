#version 330
out vec2 uv;

void main()
{
	uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);							//  0.0, 0.0     2.0, 0.0   0.0, 2.0 
	gl_Position = vec4(uv * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);		//  -1.0, -1.0   3.0, -1.0  -1.0, 3.0 
}