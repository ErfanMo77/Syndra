// Deferred Lighting Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;

void main(){
	v_uv = a_uv;
	gl_Position = vec4(a_pos,1.0f);
}

#type fragment

#version 460
layout(location = 0) out vec4 fragColor;	


//GBuffer samplers
layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;
layout(binding = 6) uniform sampler2D gRoughMetalAO;

//Shadow related samplers
layout(binding = 3) uniform sampler2D shadowMap;
layout(binding = 4) uniform sampler1D distribution0;
layout(binding = 5) uniform sampler1D distribution1;

//-----------------------------------------------UNIFORM BUFFERS-----------------------------------------//
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

//-----------------------------------------------PUSH CONSTANT-----------------------------------------//
layout(push_constant) uniform pushConstants{
	float exposure;
	float gamma;
	float size;
	float near;
	int numPCFSamples;
	int numBlockerSearchSamples;
	int softShadow;
} pc;

//-----------------------------------------Shadow calculation functions-------------------------------//

vec2 RandomDirection(sampler1D distribution, float u)
{
   return texture(distribution, u).xy * 2 - vec2(1);
}

//////////////////////////////////////////////////////////////////////////
float SearchWidth(float uvLightSize, float receiverDistance)
{
	return uvLightSize * (receiverDistance - pc.near) / receiverDistance;
}

//////////////////////////////////////////////////////////////////////////
float FindBlockerDistance_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize,float bias)
{
	int blockers = 0;
	float avgBlockerDistance = 0;
	float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
	for (int i = 0; i < pc.numBlockerSearchSamples; i++)
	{
		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution0, i / float(pc.numBlockerSearchSamples)) * searchWidth).r;
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
	for (int i = 0; i < pc.numPCFSamples; i++)
	{
		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution1, i / float(pc.numPCFSamples)) * uvRadius).r;
		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;
	}
	return sum / pc.numPCFSamples;
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
	float uvRadius = penumbraWidth * uvLightSize * pc.near / shadowCoords.z;
	return PCF_DirectionalLight(shadowCoords, shadowMap, uvRadius, bias);
}

//////////////////////////////////////////////////////////////////////////
float SoftShadow(vec4 fragPosLightSpace, float bias)
{

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
	if(projCoords.z > 1.0)
        return 0.0;
	float shadow = PCSS_DirectionalLight(projCoords,shadowMap,pc.size,bias);

    return shadow;
}

float HardShadow(vec4 fragPosLightSpace, float bias)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
	if(projCoords.z > 1.0)
        return 0.0;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	//pcf
	for (int i = 0; i < pc.numPCFSamples; i++)
	{
		float z = texture(shadowMap, projCoords.xy + RandomDirection(distribution1, i / float(pc.numPCFSamples)) * texelSize * pc.numBlockerSearchSamples).r;
		shadow += (z < (projCoords.z - bias)) ? 1 : 0;
	}

	shadow /= pc.numPCFSamples;
	//shadow +=0.1;

    return shadow;
}

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
///////////////////////////////////////////////////PBR////////////////////////////////////////
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// ----------------------------------------------------------------------------
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}   

// ----------------------------------------------------------------------------
vec3 CalculateLo(vec3 L, vec3 N, vec3 V, vec3 Ra, vec3 F0, float R, float M, vec3 A)
{
	vec3 H = normalize(V + L); //Halfway Vector

	//Cook-Torrance BRDF
	float D = DistributionGGX(N, H, R);
	float G = GeometrySmith(N, V, L, R);
	vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 Nominator    = D * G * F;
	float Denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
	vec3 Specular	  = Nominator / Denominator;

	vec3 Ks = F;
	vec3 Kd = vec3(1.0) - Ks;
	Kd *= 1.0 - M;

	float NDotL = max(dot(N, L), 0.0);
	return (Kd * A / PI + Specular) * Ra * NDotL;
}

layout(location = 0) in vec2 v_uv;

void main()
{
	vec3 fragPos = texture(gPosition, v_uv).rgb;
	vec3 N = texture(gNormal, v_uv).rgb;
	vec4 albedospec = texture(gAlbedoSpec, v_uv);
	vec3 Albedo = albedospec.rgb;
	float Roughness = texture(gRoughMetalAO,  v_uv).r;
	float Metallic  = texture(gRoughMetalAO,  v_uv).g;
	float AO		= texture(gRoughMetalAO,  v_uv).b;

	vec4 FragPosLightSpace = shadow.lightViewProj * vec4(fragPos, 1.0);

	vec3 V = normalize(cam.cameraPos.rgb - fragPos);

	vec3 R = reflect(-V, N);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, Albedo, Metallic);

	vec3 Lo = vec3(0.0);

	vec3 lightDir = normalize(-lights.dLight.dir.rgb);
    float bias = max(0.01 * (1.0 - dot(N, lightDir)), 0.001); 

	Lo += CalculateLo(lightDir, N, V, lights.dLight.color.rgb, F0, Roughness, Metallic, Albedo);

	float shadow =0;
	if(pc.softShadow == 1){
		shadow = SoftShadow(FragPosLightSpace, bias);
	} else
	{
		shadow = HardShadow(FragPosLightSpace, bias);
	}
	vec3 ambient = vec3(0.03) * Albedo * AO;

	vec3 result = vec3(0);
	result = (1-shadow) * Lo + ambient;

    vec3 hdrColor = result;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * pc.exposure);
    // gamma correction 
	mapped = pow(mapped, vec3(1.0 / pc.gamma));

	fragColor = vec4(mapped, 1.0);
}