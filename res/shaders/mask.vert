#version 120
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;
uniform bool bLightEnabled = true;
uniform bool bShowMask = true;
uniform bool bShowWeight = true;
uniform bool bShowSegments = true;
attribute float maskValue;
attribute float weightValue;
attribute float segmentValue;

varying float maskFactor;
varying vec4 weightColor;
varying vec4 segmentColor;

void directionalLight(in int i, in vec3 normal)
{
	float nDotVP;         // normal . light direction
	float nDotHV;         // normal . light half vector
	float pf;             // power factor

	nDotVP = max(0.0, dot(normal, normalize(vec3 (gl_LightSource[i].position))));
	nDotHV = max(0.0, dot(normal, vec3 (gl_LightSource[i].halfVector)));

	if (nDotVP == 0.0)
	{
		pf = 0.0;
	}
	else
	{
		pf = pow(nDotHV, gl_FrontMaterial.shininess);
	}

	Ambient  += gl_LightSource[i].ambient;
	Diffuse  += gl_LightSource[i].diffuse * nDotVP;
	Specular += gl_LightSource[i].specular * pf;
}

vec4 colorRamp(in float value)
{
	float r;
	float g;
	float b;

	if (value <= 0.0f)
	{
		r = g = b = 1.0;
	}
	else if (value <= 0.25)
	{
		r = 0.0;
		b = 1.0;
		g = value / 0.25;
	}
	else if (value <= 0.5)
	{
		r = 0.0;
		g = 1.0;
		b = 1.0 + (-1.0) * (value - 0.25) / 0.25;
	}
	else if (value <= 0.75)
	{
		r = (value - 0.5) / 0.25;
		g = 1.0;
		b = 0.0;
	}
	else
	{
		r = 1.0;
		g = 1.0 + (-1.0) * (value - 0.75) / 0.25;
		b = 0.0;
	}

	return vec4(r, g, b, 1.0);
}

void flight(in vec3 normal, in vec4 ecPosition)
{
	vec4 color;
	vec3 ecPosition3;
	vec3 eye;

	ecPosition3 = vec3(ecPosition) / ecPosition.w;
	eye = vec3(0.0, 0.0, 1.0);

	if (bLightEnabled)
	{
		Ambient = vec4(0.0);
		Diffuse = vec4(0.0);
		Specular = vec4(0.0);

		directionalLight(0, normal);
		directionalLight(1, normal);
		directionalLight(2, normal);

		color =  gl_FrontLightModelProduct.sceneColor +
			Ambient  * gl_FrontMaterial.ambient +
			Diffuse  * gl_FrontMaterial.diffuse +
			Specular * gl_FrontMaterial.specular;
	}
	else
	{
		color = gl_Color;
	}

	if (bShowMask)
	{
		maskFactor = 1.0 - maskValue / 1.5;
	}

	if (bShowWeight)
	{
		weightColor = colorRamp(weightValue);
	}
	else if (bShowSegments)
	{
		weightColor = colorRamp(weightValue);
		segmentColor = colorRamp(segmentValue);
	}

	color = clamp(color, 0.0, 1.0);
	gl_FrontColor = color;
}

void main(void)
{
	// Initialization
	maskFactor = 1.0;
	weightColor = vec4(1.0, 1.0, 1.0, 1.0);
	segmentColor = vec4(1.0, 1.0, 1.0, 1.0);

	// Eye-coordinate position of vertex, needed in various calculations
	vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;

	// Transform and Light
	gl_Position = ftransform();
	vec3 transformedNormal = normalize(gl_NormalMatrix * gl_Normal);
	flight(transformedNormal, ecPosition);

	// Gen Texture Coordinates
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
