#version 120
uniform sampler2D texUnit0;
uniform bool bShowTexture = true;

void main (void)
{
    vec4 color;
    if(bShowTexture) {
        color = gl_Color + 0.25;
        color *= texture2D(texUnit0, gl_TexCoord[0].xy) + 0.05;
        color *= 2.6;
	    color = clamp(color, 0.0, 1.0);
    } else {
        color = gl_Color;
    }
	
    gl_FragColor = color;
	if (gl_FragColor.a < 0.1)
	{
		discard;
	}
}