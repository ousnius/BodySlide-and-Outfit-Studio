#version 120
uniform sampler2D texUnit0;
uniform bool bShowTexture = true;

void main (void)
{
    vec4 color;
    color = gl_Color; // 1-gl_Color;
    //color.y=color.x;
    //color.z=color.x;
    if(bShowTexture) {
     color *= texture2D(texUnit0, gl_TexCoord[0].xy);
	 color *= 2.0;
	 color = clamp(color, 0.0, 1.0);
    }
	
    gl_FragColor = color;
	if (gl_FragColor.a < 0.1)
	{
		discard;
	}
}