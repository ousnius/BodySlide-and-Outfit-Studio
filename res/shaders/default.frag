#version 120
uniform sampler2D texUnit0;
uniform bool bShowTexture = true;

void main(void)
{
	vec4 color = gl_Color;
	
	if (bShowTexture)
	{
		color.rgb += vec3(0.25, 0.25, 0.25);
		color *= texture2D(texUnit0, gl_TexCoord[0].xy);
		color.rgb *= 2.0;
	}
	
	color = clamp(color, 0.0, 1.0);
	gl_FragColor = color;
	
	if (gl_FragColor.a < 0.1)
	{
		discard;
	}
}
