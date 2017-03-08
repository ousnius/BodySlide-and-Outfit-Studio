#version 330

uniform sampler2D texDiffuse;
in vec2 uv;
out vec4 color;

vec4 blur(in float texel) {
	vec2 offs[9] = vec2[]( 
		vec2(-texel, -texel), vec2(0.0, -texel), vec2(texel,-texel),		
		vec2(-texel, 0.0),	  vec2(0.0, 0.0),    vec2(texel,0.0),		
		vec2(-texel, texel),  vec2(0.0, texel),  vec2(texel,texel)
	);
	
	vec4[9] sample;
	for(int i=0;i<9;i++) {
		sample[i] = texture(texDiffuse, uv + offs[i]);
	}
	return (sample[0] + (2.0*sample[1]) + sample[2] + 
              (2.0*sample[3]) + sample[4] + (2.0*sample[5]) + 
              sample[6] + (2.0*sample[7]) + sample[8]) / 13.0;

} 

vec4 maxSample(in float texel) {
	vec2 offs[9] = vec2[]( 
		vec2(-texel, -texel), vec2(0.0, -texel), vec2(texel,-texel),		
		vec2(-texel, 0.0),	  vec2(0.0, 0.0),    vec2(texel,0.0),		
		vec2(-texel, texel),  vec2(0.0, texel),  vec2(texel,texel)
	);
	vec4[9] sample;
	vec4 maxValue = vec4(0.0);
	for(int i=0;i<9;i++) {
		sample[i] = texture(texDiffuse, uv + offs[i]);
		maxValue = max(sample[i], maxValue);
	}

	return maxValue;
}

vec4 minDistSample(in float texel, in int range) {

	vec2 offs[8] = vec2[] ( vec2(-1.0,0.0), vec2(1.0,0.0), vec2(0.0,1.0), vec2(0.0,-1.0), vec2(-1.0,1.0), vec2(1.0,1.0), vec2(1.0,-1.0), vec2(-1.0,-1.0));
	vec4 sample = texture(texDiffuse, uv);
	if(sample.a != 0.0) {
		return sample;
	}

	for(int i=0;i<range;i++) {
		for(int j=0;j<8;j++) {
			vec4 remoteSample = texture(texDiffuse, uv + offs[j]*(texel*i));
			if(remoteSample.a !=0.0) {
				return remoteSample;
			}
		}
	}
	return sample;
}

void main(void)
{	
	float texel = 1.0 / 1024.0;

	// blur filter
	// color = blur(texel);

	// simple maximum -- greyscale dilation (Does not work for color)
	// color = maxSample(texel);

	// nearest non-alpha sample, works as a basic color dilation filter
	color = minDistSample (texel, 10);
	
	// passthrough
	// color = texture(texDiffuse,uv);

	// uvramp
	// color=vec4(uv.x, uv.y,0.0,1.0);
	
}