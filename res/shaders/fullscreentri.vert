#version 330
out vec2 uv;
out vec4 vPos;
void main()
{
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    uv.x = (x+1.0)*0.5;
    uv.y = (y+1.0)*0.5;
    vPos = vec4(x, y, 0.0, 1);
}