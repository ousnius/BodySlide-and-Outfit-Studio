#version 330

in vec2 uv;
out vec3 color;
in vec4 vPos;
void main(void)
{	
	//if(uv.x==uv.y) {
	//	color = vec3(0.0,1.0,0.0);
	//} else {
		color=vec3(vPos.x, vPos.y,1.0);
	//}
}