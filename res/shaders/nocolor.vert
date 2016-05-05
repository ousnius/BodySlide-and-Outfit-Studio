#version 120
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;
uniform bool bLightEnabled = true;

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

void flight(in vec3 normal, in vec4 ecPosition)
{
    vec4 color;
    vec3 ecPosition3;
    vec3 eye;

    ecPosition3 = (vec3 (ecPosition)) / ecPosition.w;
    eye = vec3 (0.0, 0.0, 1.0);
	if(bLightEnabled) {
	
		directionalLight(0, normal);
	
		directionalLight(1, normal);

		directionalLight(2, normal);

		color =  gl_FrontLightModelProduct.sceneColor +
		  Ambient  * gl_FrontMaterial.ambient +
		  Diffuse  * gl_FrontMaterial.diffuse +
		  Specular * gl_FrontMaterial.specular;
	} else {	
	  color = gl_Color;
	}
	color = clamp( color, 0.0, 1.0 );
    gl_FrontColor = color;

}

void main (void)
{
    vec3  transformedNormal;

    // Eye-coordinate position of vertex, needed in various calculations
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;

    // Transform and Light
    gl_Position = ftransform();
    transformedNormal = normalize(gl_NormalMatrix * gl_Normal);
    flight(transformedNormal, ecPosition);

	// Gen Texture Coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
}