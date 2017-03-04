#version 330
uniform sampler2D texNormal;
uniform sampler2D texAlphaMask;
uniform mat4 matModelView;
uniform bool bModelSpace;
uniform bool bAlphaMask;

in vec3 N;
in vec3 v;
in vec3 vPos;
in vec2 uv;

vec4 normalMap;

out vec4 fragColor;

// simple vertex shading to produce smoothed model space normals.
vec3 NormalColor(vec3 inNormal) {
	float r = 1.0 - ( (inNormal.r + 1.0) / 2.0 );
	float g = (inNormal.g + 1.0) / 2.0;
    float b = (inNormal.b + 1.0) / 2.0;
	return vec3(r,g,b);
}

// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(in vec3 N, in vec3 p)
{
    // Get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
	
    // Solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	
    // Construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

vec3 perturb_normal(in vec3 N, in vec3 V)
{
	// Assume N, the interpolated vertex normal and V, the view vector (vertex to eye)
	vec3 map = normalMap.rgb;
	//map.r = 1.0-map.r;
	//map.g = 1.0-map.g;
	map = map * 255.0 / 127.0 - 128.0 / 127.0;
	mat3 TBN = cotangent_frame(N, -V);
	return normalize(TBN * map);
}


vec4 Light(vec3 lightPos, vec4 lightColor, float lightPower, vec3 fragNorm) {

	//vec3 xformLightPos = inverse(gl_ModelViewMatrix) * lightPos;

	vec3 lightDir = lightPos - v;
	float dist = length(lightDir);
	lightDir = normalize(lightDir);
	float NdotL = max(0.0, dot(fragNorm,lightDir));
			
	float attenuation = 1.0 / ( 1.0 +				//	 constant attenuation
								0.002 * dist +		//   linearAttenuation 
								0.0002 * dist * dist   //   quadraticAttenuation
								);
	return lightColor * lightPower * NdotL * attenuation;

}

void main(void)
{	
	vec3 norm = normalize(N);	
	if(bAlphaMask) {
		vec4 maskMap = texture(texAlphaMask,uv);
		
		if(maskMap.r == 0.0) {
			discard;
		}
	}
	
	//vec4 spec = vec4(0.0);
	//vec4 ambient =  vec4(vec3(0.1),1.0);
	//vec4 albedo = vec4(0.6, 0.6,0.6,1.0);

	//vec4 LightDiffuse1 = vec4(1.0, 1.0, 1.0, 1.0);
	//vec3 LightPos1 = vec3 (40.0, 40.0, 40.0);

	//vec4 LightDiffuse2 = vec4(1.0, 1.0, 1.0, 1.0);
	//vec3 LightPos2 = vec3 (-80.0, 10.0, 10.0);

	//vec4 LightDiffuse3 = vec4(0.0, 0.9, 0.9, 1.0);
	//vec3 LightPos3 = vec3 (00.0, 00.0, -120.0);

	//vec4 Light1 = Light (LightPos1, LightDiffuse1, 3, norm);
	//vec4 Light2 = Light (LightPos2, LightDiffuse2, 2, norm);
	//vec4 Light3 = Light (LightPos3, LightDiffuse3, 3, norm);

	//fragColor = ambient + Light1 * albedo + Light2 * albedo + Light3 * albedo;
	//fragColor = vec4(0.8, 0.0, 0.0, 1.0);
	
	normalMap = texture(texNormal,uv);
	
	mat3 normalMatrix = transpose(inverse(mat3(matModelView)));
	if(bModelSpace) {
		norm = normalize(normalMap.rgb * 2.0 - 1.0);
		norm.r = -norm.r;
	} 
	else {
		norm = normalMatrix * perturb_normal(inverse(normalMatrix) * norm, vPos);
	}
	//fragColor = normalMap;
	fragColor = vec4(NormalColor(norm) ,1.0);
}