#version 120
uniform sampler2D texUnit0;
uniform bool bShowTexture = true;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowSegments;

varying float maskFactor;
varying vec4 weightColor;
varying vec4 segmentColor;

void main(void)
{
	vec4 color = gl_Color;
	
	if (bShowTexture)
	{
		color.rgb += vec3(0.25, 0.25, 0.25);
		color *= texture2D(texUnit0, gl_TexCoord[0].xy);
		color.rgb *= 2.0;
	}
	
	if (bShowMask)
	{
		color.rgb *= maskFactor;
	}
	
	if (bShowWeight)
	{
		color *= weightColor;
	}
	else if (bShowSegments)
	{		
		if (segmentColor.r != 0.0 && segmentColor.g != 0.0 && segmentColor.b != 0.0 &&
			segmentColor.rg != normalize(segmentColor.rg) &&
			segmentColor.rb != normalize(segmentColor.rb) &&
			segmentColor.gb != normalize(segmentColor.gb))
		{
			color *= weightColor;
		}
		else
		{
			color *= segmentColor;
		}
	}
	
	color = clamp(color, 0.0, 1.0);
	gl_FragColor = color;
	
	if (gl_FragColor.a < 0.1)
	{
		discard;
	}
}
