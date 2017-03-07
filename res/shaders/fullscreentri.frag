#version 330

uniform sampler2D texDiffuse;
in vec2 uv;
out vec4 color;

void main(void)
{	
	float texel = 1.0 / 1024.0;
	vec2 offs[9] = vec2[]( 
		vec2(-texel, -texel), vec2(0.0, -texel), vec2(texel,-texel),		
		vec2(-texel, 0.0),	  vec2(0.0, 0.0),    vec2(texel,0.0),		
		vec2(-texel, texel),  vec2(0.0, texel),  vec2(texel,texel)
	);

	vec4 sample[9];
	vec4 maxValue = vec4(0.0);

	for(int i=0;i<9;i++) {
		sample[i] = texture(texDiffuse, uv + offs[i]);
		maxValue = max(sample[i], maxValue);
	}

	color = (sample[0] + (2.0*sample[1]) + sample[2] + 
                    (2.0*sample[3]) + sample[4] + (2.0*sample[5]) + 
                    sample[6] + (2.0*sample[7]) + sample[8]) / 13.0;

	if(sample[4].r == 0.0 && sample[4].g == 0.0 && sample[4].b == 0.0 ) {
		color = maxValue;
	} else {
		color = sample[4]; //vec4(1.0,0.5,1.0,1.0);
	}
	//vec4 tex = texture(texDiffuse,uv);
	//if(uv.x==uv.y) {
		//color = vec4(0.0,1.0,0.0, 1.0);
	//} else {
		//color = tex;
	   // color=vec4(uv.x, uv.y,1.0,1.0);
	//}
}