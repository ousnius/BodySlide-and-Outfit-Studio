#version 330
uniform sampler2D texDiffuse;

uniform bool bLightEnabled;
uniform bool bShowTexture;
uniform bool bShowMask;
uniform bool bShowWeight;
uniform bool bShowSegments;
uniform bool bShowSkinColor;
uniform bool bWireframe;
uniform bool bPoints;

in vec4 vertexColor;

in float maskFactor;
in vec4 weightColor;
in vec4 segmentColor;
in vec2 UV;

out vec4 fragColor;

void main(void)
{
	vec4 color = vertexColor;
	
	if (!bPoints)
	{
		if (!bWireframe)
		{
			if (bShowTexture)
			{
				color *= texture(texDiffuse, UV);
				if (bLightEnabled)
				{					
					if (!bShowSkinColor)
					{
						color *= vec4(2.0, 2.0, 2.0, 1.0);
					}
					else
					{
						color *= vec4(2.6, 2.6, 2.6, 1.0);
					}
				}
			}

			if (bShowSegments)
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
			else
			{
				if (bShowMask)
				{
					color.rgb *= maskFactor;
				}
				
				if (bShowWeight)
				{
					color *= weightColor;
				}
			}
		}
		else
		{
			color.rgb = vec3(1.0) - color.rgb;
		}
		
		color = clamp(color, 0.0, 1.0);
	}
	else
	{
		// Calculate normal from coord
		vec2 norm;
		norm = gl_PointCoord * 2.0 - vec2(1.0); 
		float mag = dot(norm, norm);
		if (mag > 1.0) 
			discard; // Kill pixels outside point

		color.a = 1.0 - mag;
		color = clamp(color, 0.0, 1.0);
	}
	
	fragColor = color;
	
	if (fragColor.a < 0.1)
	{
		discard;
	}
}
