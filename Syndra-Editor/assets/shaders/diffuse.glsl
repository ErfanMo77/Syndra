// Basic diffuse Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

layout(binding = 1) uniform Transform
{
	 mat4 u_trans;
	 vec4 lightPos;
} transform;

layout(binding = 3) uniform ShadowData
{
	 mat4 lightViewProj;
} shadow;

struct VS_OUT {
	vec3 v_pos;
	vec2 v_uv;
	vec3 v_normal;
	vec4 FragPosLightSpace;
};

layout(location = 0) out VS_OUT vs_out;

void main(){
	vs_out.v_normal = mat3(transpose(inverse(transform.u_trans)))*a_normal;
	vs_out.v_pos = vec3(transform.u_trans*vec4(a_pos,1.0));
	vs_out.v_uv = a_uv;
	vs_out.FragPosLightSpace = shadow.lightViewProj * vec4(vs_out.v_pos, 1.0);
	gl_Position = cam.u_ViewProjection * transform.u_trans *vec4(a_pos,1.0);
}

#type fragment
#version 460
layout(location = 0) out vec4 fragColor;	

layout(binding = 0) uniform sampler2D texture_diffuse;
layout(binding = 3) uniform sampler2D shadowMap;

layout(binding = 1) uniform Transform
{
	 mat4 u_trans;
	 vec4 lightPos;
} transform;

layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

#define constant 1.0
#define linear 0.022
#define quadratic 0.0019

struct PointLight {
    vec4 position;
    vec4 color;
	float dist;
};

struct DirLight {
    vec4 position;
    vec4 dir;
	vec4 color;
};

struct SpotLight {
    vec4 position;
	vec4 color;
    vec4 direction;
    float cutOff;
    float outerCutOff;      
};

layout(binding = 2) uniform Lights
{
	PointLight pLight[4];
	SpotLight sLight[4];
	DirLight dLight;
} lights;


struct VS_OUT {
    vec3 v_pos;
    vec2 v_uv;
	vec3 v_normal;
	vec4 FragPosLightSpace;
};

layout(location = 0) in VS_OUT fs_in;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 col);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir,vec3 col);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 col);

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	vec3 normal = normalize(fs_in.v_normal);
    vec3 lightDir = normalize(-lights.dLight.dir.rgb);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001); 

	float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	//pcf
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
 
	if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main(){	

    vec3 norm = normalize(fs_in.v_normal);
	vec3 viewDir = normalize(cam.cameraPos.rgb - fs_in.v_pos);

	vec4 color = texture(texture_diffuse, fs_in.v_uv);
	if(color.a < .2){
		discard;
	}

	vec3 result = vec3(0);
	result += CalcDirLight(lights.dLight, norm, viewDir,color.rgb);
	for(int i = 0; i<4; i++){
		result += CalcPointLight(lights.pLight[i], norm, fs_in.v_pos, viewDir,color.rgb);
	}
	for(int i=0; i<4; i++){
		result += CalcSpotLight(lights.sLight[i], norm, fs_in.v_pos, viewDir, color.rgb);
	}
	float shadow = ShadowCalculation(fs_in.FragPosLightSpace); 
	result = (1- shadow) * result + color.rgb * 0.1;
	fragColor = vec4(result,1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir,vec3 col){
    vec3 lightDir = normalize(-light.dir.rgb);
    float diff = max(dot(normal, lightDir), 0.0);
	vec3 color;
	if(col == vec3(0)){
		color = vec3(diff)* light.color.rgb;
	}else {
		color = col * diff * light.color.rgb;
	}
	return color;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,vec3 col)
{
    vec3 lightDir = normalize(light.position.rgb - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position.rgb - fragPos);
    float attenuation = 1.0 / (constant + quadratic * (distance * distance));  
	if(distance > light.dist){
		attenuation = 0;
	}
	vec3 color;
	if(col == vec3(0)){
		color = vec3(diff)* light.color.rgb;
	}else {
		color = col * diff * light.color.rgb;
	}

    color *= attenuation;
    return color;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 col){

	vec3 lightDir = normalize(light.position.rgb - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position.rgb - fragPos);
    float attenuation = 1.0 / (constant + quadratic * (distance * distance));  

	vec3 color;
	if(col == vec3(0)){
		color = vec3(diff)* light.color.rgb;
	}else {
		color = col * diff * light.color.rgb;
	}
	float theta = dot(lightDir, normalize(-light.direction.rgb)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    color *= attenuation * intensity;

	return color;

}