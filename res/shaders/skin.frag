#version 120
uniform sampler2D texUnit0;
uniform bool bShowTexture = true;
uniform bool bShowMask = true;
uniform bool bShowWeight = true;
uniform bool bShowSegments = true;

in float maskFactor;
in vec4 weightColor;
in vec4 segmentColor;

void main(void)
{
	vec4 color = gl_Color;
	color.rgb += vec3(0.25, 0.25, 0.25);
	
	if (bShowTexture)
	{
		color *= texture2D(texUnit0, gl_TexCoord[0].xy);
		color.rgb *= 2.6;
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
			segmentColor = vec4(1.0, 1.0, 1.0, 1.0);
		}
		else
		{
			weightColor = vec4(1.0, 1.0, 1.0, 1.0);
		}
		
		color *= weightColor;
		color *= segmentColor;
	}
	
	color = clamp(color, 0.0, 1.0);
	gl_FragColor = color;
	
	if (gl_FragColor.a < 0.1)
	{
		discard;
	}
}
