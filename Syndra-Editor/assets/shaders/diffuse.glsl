// Basic diffuse Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(push_constant) uniform Transform
{
	mat4 u_trans;
}transform;

layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

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
layout(binding = 4) uniform sampler1D distribution0;
layout(binding = 5) uniform sampler1D distribution1;


layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

#define constant 1.0
#define linear 0.022
#define quadratic 0.0019
const int NEAR = 2;

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

layout(push_constant) uniform pc
{
	float size;
	int numPCFSamples;
	int numBlockerSearchSamples;
	int softShadow;
} push;


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

//////////////////////////////////////////////////////////////////////////
vec2 RandomDirection(sampler1D distribution, float u)
{
   return texture(distribution, u).xy * 2 - vec2(1);
}

//////////////////////////////////////////////////////////////////////////
float SearchWidth(float uvLightSize, float receiverDistance)
{
	return uvLightSize * (receiverDistance - NEAR) / receiverDistance;
}

//////////////////////////////////////////////////////////////////////////
float FindBlockerDistance_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize,float bias)
{
	int blockers = 0;
	float avgBlockerDistance = 0;
	float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
	for (int i = 0; i < push.numBlockerSearchSamples; i++)
	{
		//vec3 uvc =  vec3(shadowCoords.xy + RandomDirection(distribution0, i / float(numPCFSamples)) * uvLightSize,(shadowCoords.z - bias));
		//float z = texture(shadowMap, uvc);
		//if (z>0.5)
		//{
		//	blockers++;
			//avgBlockerDistance += z;
		//}

		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution0, i / float(push.numBlockerSearchSamples)) * searchWidth).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}
	if (blockers > 0)
		return avgBlockerDistance / blockers;
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////////
float PCF_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvRadius, float bias)
{
	float sum = 0;
	for (int i = 0; i < push.numPCFSamples; i++)
	{
		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * uvRadius).r;
		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;

		//vec3 uvc =  vec3(shadowCoords.xy + RandomDirection(distribution1, i / float(numPCFSamples)) * uvRadius,(shadowCoords.z - bias));
		//float z = texture(shadowMap, uvc);
		//sum += z;
	}
	return sum / push.numPCFSamples;
}

//////////////////////////////////////////////////////////////////////////
float PCSS_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize, float bias)
{
	// blocker search
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowCoords, shadowMap, uvLightSize, bias);
	if (blockerDistance == -1)
		return 0;		

	// penumbra estimation
	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	// percentage-close filtering
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;
	return PCF_DirectionalLight(shadowCoords, shadowMap, uvRadius, bias);
}

//////////////////////////////////////////////////////////////////////////
float SoftShadow(vec4 fragPosLightSpace)
{

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

	vec3 normal = normalize(fs_in.v_normal);
    vec3 lightDir = normalize(-lights.dLight.dir.rgb);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001); 

    float currentDepth = projCoords.z;
	if(projCoords.z > 1.0)
        return 0.0;
	float shadow = PCSS_DirectionalLight(projCoords,shadowMap,push.size,bias);

    return shadow;
}

float HardShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

	vec3 normal = normalize(fs_in.v_normal);
    vec3 lightDir = normalize(-lights.dLight.dir.rgb);
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001); 

    float currentDepth = projCoords.z;
	if(projCoords.z > 1.0)
        return 0.0;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	//pcf
	for (int i = 0; i < push.numPCFSamples; i++)
	{
		float z = texture(shadowMap, projCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * texelSize * push.numBlockerSearchSamples).r;
		shadow += (z < (projCoords.z - bias)) ? 1 : 0;
	}

	shadow /= push.numPCFSamples;
	//shadow +=0.1;

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
	for(int i=0; i<1; i++){
		result += CalcSpotLight(lights.sLight[i], norm, fs_in.v_pos, viewDir, color.rgb);
	}
	float shadow =0;
	if(push.softShadow == 1){
		shadow = SoftShadow(fs_in.FragPosLightSpace);
	} else
	{
		shadow = HardShadow(fs_in.FragPosLightSpace);
	}
	result = (1-shadow) * result + color.rgb * 0.1;
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