// Forward shading light accumulation shader
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

layout(push_constant) uniform Transform
{
	mat4 u_trans;
	int id;
}transform;


struct VS_OUT {
	vec3 v_pos;
	vec2 v_uv;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
};

layout(location = 0) out VS_OUT vs_out;
layout(location = 8) out flat int id;

void main(){

    vs_out.v_pos = vec3(transform.u_trans * vec4(a_pos, 1.0));   
    vs_out.v_uv = a_uv;

	mat3 normalMatrix = transpose(inverse(mat3(transform.u_trans)));
    vec3 T = normalize(normalMatrix * a_tangent);
    vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));

	id = transform.id;

	//vs_out.TangentLightPos = TBN * vec3(transform.lightPos);
    vs_out.TangentViewPos  = TBN * vec3(cam.cameraPos);
    vs_out.TangentFragPos  = TBN * vs_out.v_pos;
	gl_Position = cam.u_ViewProjection * transform.u_trans *vec4(a_pos, 1.0);
}

#type fragment
#version 460

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lightDebug;
layout(location = 2) out int entityID;

layout(binding = 0) uniform sampler2D texture_diffuse;
layout(binding = 3) uniform sampler2D shadowMap;
layout(binding = 4) uniform sampler1D distribution0;
layout(binding = 5) uniform sampler1D distribution1;


layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

layout(binding = 3) uniform ShadowData
{
	 mat4 lightViewProj;
} shadow;

#define constant 1.0
#define linear 0.022
#define quadratic 0.0019
const int NEAR = 2;

struct PointLight {
    vec4 position;
    vec4 color;
	vec4 paddingAndRadius;
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

struct VisibleIndex
{
	int index;
};


layout(binding = 2) uniform Lights
{
	DirLight dLight;
} lights;


layout(push_constant) uniform pushConstants{
	float exposure;
	float gamma;
	float size;
	float near;
	float intensity;
	int numPCFSamples;
	int numBlockerSearchSamples;
	int softShadow;
	int numberOfTilesX;
} pc;

// Shader storage buffer objects
layout(std430, binding = 0) readonly buffer LightBuffer {
	PointLight data[];
} lightBuffer;

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[];
} visibleLightIndicesBuffer;

struct VS_OUT {
    vec3 v_pos;
    vec2 v_uv;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
};

layout(location = 0) in VS_OUT fs_in;
layout(location = 8) in	flat int id;

//////////////////////////////////////////////////////////////////////////
//vec2 RandomDirection(sampler1D distribution, float u)
//{
//   return texture(distribution, u).xy * 2 - vec2(1);
//}
//
////////////////////////////////////////////////////////////////////////////
//float SearchWidth(float uvLightSize, float receiverDistance)
//{
//	return uvLightSize * (receiverDistance - NEAR) / receiverDistance;
//}

//////////////////////////////////////////////////////////////////////////
//float FindBlockerDistance_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize,float bias)
//{
//	int blockers = 0;
//	float avgBlockerDistance = 0;
//	float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
//	for (int i = 0; i < push.numBlockerSearchSamples; i++)
//	{
//		//vec3 uvc =  vec3(shadowCoords.xy + RandomDirection(distribution0, i / float(numPCFSamples)) * uvLightSize,(shadowCoords.z - bias));
//		//float z = texture(shadowMap, uvc);
//		//if (z>0.5)
//		//{
//		//	blockers++;
//			//avgBlockerDistance += z;
//		//}
//
//		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution0, i / float(push.numBlockerSearchSamples)) * searchWidth).r;
//		if (z < (shadowCoords.z - bias))
//		{
//			blockers++;
//			avgBlockerDistance += z;
//		}
//	}
//	if (blockers > 0)
//		return avgBlockerDistance / blockers;
//	else
//		return -1;
//}

//////////////////////////////////////////////////////////////////////////
//float PCF_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvRadius, float bias)
//{
//	float sum = 0;
//	for (int i = 0; i < push.numPCFSamples; i++)
//	{
//		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * uvRadius).r;
//		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;
//
//		//vec3 uvc =  vec3(shadowCoords.xy + RandomDirection(distribution1, i / float(numPCFSamples)) * uvRadius,(shadowCoords.z - bias));
//		//float z = texture(shadowMap, uvc);
//		//sum += z;
//	}
//	return sum / push.numPCFSamples;
//}

//////////////////////////////////////////////////////////////////////////
//float PCSS_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize, float bias)
//{
//	// blocker search
//	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowCoords, shadowMap, uvLightSize, bias);
//	if (blockerDistance == -1)
//		return 0;		
//
//	// penumbra estimation
//	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;
//
//	// percentage-close filtering
//	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;
//	return PCF_DirectionalLight(shadowCoords, shadowMap, uvRadius, bias);
//}

//////////////////////////////////////////////////////////////////////////
//float SoftShadow(vec4 fragPosLightSpace)
//{
//
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//
//    projCoords = projCoords * 0.5 + 0.5;
//
//	vec3 normal = normalize(fs_in.v_normal);
//    vec3 lightDir = normalize(-lights.dLight.dir.rgb);
//    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001); 
//
//    float currentDepth = projCoords.z;
//	if(projCoords.z > 1.0)
//        return 0.0;
//	float shadow = PCSS_DirectionalLight(projCoords,shadowMap,push.size,bias);
//
//    return shadow;
//}

//float HardShadow(vec4 fragPosLightSpace)
//{
//    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
//
//    projCoords = projCoords * 0.5 + 0.5;
//
//	vec3 normal = normalize(fs_in.v_normal);
//    vec3 lightDir = normalize(-lights.dLight.dir.rgb);
//    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.001); 
//
//    float currentDepth = projCoords.z;
//	if(projCoords.z > 1.0)
//        return 0.0;
//
//	float shadow = 0.0;
//	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
//	//pcf
//	for (int i = 0; i < push.numPCFSamples; i++)
//	{
//		float z = texture(shadowMap, projCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * texelSize * push.numBlockerSearchSamples).r;
//		shadow += (z < (projCoords.z - bias)) ? 1 : 0;
//	}
//
//	shadow /= push.numPCFSamples;
//	//shadow +=0.1;
//
//    return shadow;
//}


void main(){	

    //vec3 norm = normalize(fs_in.v_normal);
	//vec3 viewDir = normalize(cam.cameraPos.rgb - fs_in.v_pos);
	//
	//vec4 color = texture(texture_diffuse, fs_in.v_uv);
	//if(color.a < .2){
	//	discard;
	//}
	//
	//vec3 result = vec3(0);
	//result += CalcDirLight(lights.dLight, norm, viewDir,color.rgb);
	//
	//float shadow =0;
	//if(push.softShadow == 1){
	//	shadow = SoftShadow(fs_in.FragPosLightSpace);
	//} else
	//{
	//	shadow = HardShadow(fs_in.FragPosLightSpace);
	//}
	//result = (1-shadow) * result + color.rgb * 0.1;

	// Determine which tile this pixel belongs to
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tileID = location / ivec2(16, 16);
	uint index = tileID.y * pc.numberOfTilesX + tileID.x;

	//uint offset = index * 1024;
	//for (uint i = 0; i < 1024 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++) {
	//uint lightIndex = visibleLightIndicesBuffer.data[offset + i].index;
	//PointLight light = lightBuffer.data[lightIndex];
	//
	//vec4 lightColor = light.color;
	//vec3 tangentLightPosition = fragment_in.TBN * light.position.xyz;
	//float lightRadius = light.paddingAndRadius.w;
	//
	//// Calculate the light attenuation on the pre-normalized lightDirection
	//vec3 lightDirection = tangentLightPosition - fragment_in.tangentFragmentPosition;
	//float attenuation = attenuate(lightDirection, lightRadius);
	//
	//// Normalize the light direction and calculate the halfway vector
	//lightDirection = normalize(lightDirection);
	//vec3 halfway = normalize(lightDirection + viewDirection);
	//
	//// Calculate the diffuse and specular components of the irradiance, then irradiance, and accumulate onto color
	//float diffuse = max(dot(lightDirection, normal), 0.0);
	//// How do I change the material propery for the spec exponent? is it the alpha of the spec texture?
	//float specular = pow(max(dot(normal, halfway), 0.0), 32.0);
	//
	//// Hacky fix to handle issue where specular light still effects scene once point light has passed into an object
	//if (diffuse == 0.0) {
	//	specular = 0.0;
	//}


	uint offset = index * 512;
	uint i;
	for (i = 0; i < 512 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++);

	float ratio = float(i) / float(2.0);

	fragColor = vec4(vec3(1), 1.0);
	lightDebug = vec4(vec3(ratio),1.0);
	entityID = id;
	//fragColor = vec4(1.0);

}
