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
} transform;

struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

layout(location = 0) out VS_OUT vs_out;
layout(location = 8) out flat int id;

void main(){

    vs_out.v_pos = vec3(transform.u_trans * vec4(a_pos, 1.0));   

	mat3 normalMatrix = transpose(inverse(mat3(transform.u_trans)));
    vec3 T = normalize(normalMatrix * a_tangent);
    vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	vs_out.v_normal = N;
	vs_out.v_uv = a_uv;
	vs_out.TBN = mat3(T, B, N);

	id = transform.id;
	//vs_out.TangentLightPos = TBN * vec3(transform.lightPos);
    //vs_out.TangentViewPos  = TBN * vec3(cam.cameraPos);
    //vs_out.TangentFragPos  = TBN * vs_out.v_pos;
	gl_Position = cam.u_ViewProjection * transform.u_trans *vec4(a_pos, 1.0);
}

#type fragment
#version 460

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lightDebug;
layout(location = 2) out int entityID;

//Textures of a mesh
layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D metallicMap;
layout(binding = 2) uniform sampler2D NormalMap;
layout(binding = 3) uniform sampler2D RoughnessMap;
layout(binding = 4) uniform sampler2D AmbientOcclusionMap;

//Shadow related samplers
layout(binding = 5) uniform sampler2D shadowMap;
layout(binding = 6) uniform sampler1D distribution0;
layout(binding = 10) uniform sampler1D distribution1;

//IBL
layout(binding = 7) uniform samplerCube irradianceMap;
layout(binding = 8) uniform samplerCube prefilterMap;
layout(binding = 9) uniform sampler2D   brdfLUT;  



struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

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

struct Material
{
	vec4 color;
	float RoughnessFactor;
	float MetallicFactor;
	float AO;
};

//All of the uniform variables
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
	Material material;
	int id;
	float tiling;
	int HasAlbedoMap;
	int HasNormalMap;
	int HasRoughnessMap;
	int HasMetallicMap;
	int HasAOMap;
} push;

// Shader storage buffer objects
layout(std430, binding = 0) readonly buffer LightBuffer {
	PointLight data[];
} lightBuffer;

layout(std430, binding = 1) readonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[];
} visibleLightIndicesBuffer;

layout(location = 0) in VS_OUT fs_in;
layout(location = 8) in	flat int id;

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

////////////////////////////////////////////////////////////////////////////
float FindBlockerDistance_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvLightSize,float bias)
{
	int blockers = 0;
	float avgBlockerDistance = 0;
	float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
	for (int i = 0; i < push.numBlockerSearchSamples; i++)
	{
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

////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
float PCF_DirectionalLight(vec3 shadowCoords, sampler2D shadowMap, float uvRadius, float bias)
{
	float sum = 0;
	for (int i = 0; i < push.numPCFSamples; i++)
	{
		float z = texture(shadowMap, shadowCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * uvRadius).r;
		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;
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
	float uvRadius = penumbraWidth * uvLightSize * push.near / shadowCoords.z;
	return PCF_DirectionalLight(shadowCoords, shadowMap, uvRadius, bias);
}

////////////////////////////////////////////////////////////////////////////
float SoftShadow(vec4 fragPosLightSpace, float bias)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
	if(projCoords.z > 1.0)
        return 0.0;
	float shadow = PCSS_DirectionalLight(projCoords,shadowMap,push.size,bias);

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
	for (int i = 0; i < push.numPCFSamples; i++)
	{
		float z = texture(shadowMap, projCoords.xy + RandomDirection(distribution1, i / float(push.numPCFSamples)) * texelSize * push.numBlockerSearchSamples).r;
		shadow += (z < (projCoords.z - bias)) ? 1 : 0;
	}

	shadow /= push.numPCFSamples;
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

// Attenuate the point light intensity
float attenuate(vec3 lightDirection, float radius) {
	float cutoff = 0.5;
	float attenuation = dot(lightDirection, lightDirection) / (100.0 * radius);
	attenuation = 1.0 / (attenuation * 15.0 + 1.0);
	attenuation = (attenuation - cutoff) / (1.0 - cutoff);

	return clamp(attenuation, 0.0, 1.0);
}

void main(){	

	//UV
	vec2 uv = fs_in.v_uv * push.tiling;

	//////////////////////////////////////ALBEDO////////////////////////////////////////////
	vec3 albedo;
	if(push.HasAlbedoMap==1){
		albedo = pow(texture(AlbedoMap, uv).rgb, vec3(2.2));
	}else {
		albedo = push.material.color.rgb;
	}

	//////////////////////////////////////NORMAL////////////////////////////////////////////
	vec3 N;
	if(push.HasNormalMap==1){
		vec3 normal = texture(NormalMap, uv).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		N = normalize(fs_in.TBN * normal);
	}
	else{
		N = fs_in.v_normal;
	}

	///////////////////////////////////ROUGHNESS////////////////////////////////////////////
	float Roughness;
	if(push.HasRoughnessMap == 1)
	{
		Roughness = texture(RoughnessMap, uv).r * push.material.RoughnessFactor;
	}
	else
	{
		Roughness = push.material.RoughnessFactor;
	}

	///////////////////////////////////METALLIC/////////////////////////////////////////////
	float Metallic;
	if(push.HasMetallicMap == 1)
	{
		Metallic =  texture(metallicMap, uv).r * push.material.MetallicFactor;
	}
	else
	{
		Metallic = push.material.MetallicFactor;
	}

	///////////////////////////////////AMBIENT OCCLUSION///////////////////////////////////
	float AO;
	if(push.HasAOMap == 1)
	{
		AO =  texture(AmbientOcclusionMap, uv).r * push.material.AO;
	}
	else
	{
		AO = push.material.AO;
	}

	vec3 fragPos = fs_in.v_pos;
	vec4 FragPosLightSpace = shadow.lightViewProj * vec4(fragPos, 1.0);

	vec3 V = normalize(cam.cameraPos.rgb - fragPos);

	vec3 R = reflect(-V, N);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, Metallic);

	vec3 Lo = vec3(0.0);

	vec3 lightDir = normalize(-lights.dLight.dir.rgb);
    float bias = max(0.01 * (1.0 - dot(N, lightDir)), 0.001); 

	Lo += CalculateLo(lightDir, N, V, lights.dLight.color.rgb, F0, Roughness, Metallic, albedo);
	float shadow =0;
	//directional light shadow
	if(push.softShadow == 1){
		shadow = SoftShadow(FragPosLightSpace, bias);
	} else
	{
		shadow = HardShadow(FragPosLightSpace, bias);
	}

	vec3 tangentFragmentPos = fs_in.TBN * fragPos;
	// Determine which tile this pixel belongs to
	ivec2 location = ivec2(gl_FragCoord.xy);
	ivec2 tileID = location / ivec2(16, 16);
	uint index = tileID.y * push.numberOfTilesX + tileID.x;


	//Calculating point lights contribution
	uint offset = index * 512;
	for (uint i = 0; i < 512 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++) {
		uint lightIndex = visibleLightIndicesBuffer.data[offset + i].index;
		PointLight light = lightBuffer.data[lightIndex];
		
		vec3 lightColor = light.color.rgb;
		vec3 tangentLightPosition = fs_in.TBN * light.position.xyz;
		float lightRadius = light.paddingAndRadius.w;
		
		// Calculate the light attenuation on the pre-normalized lightDirection
		vec3 lightDirection = tangentLightPosition - tangentFragmentPos;
		float attenuation = attenuate(lightDirection, lightRadius);
		lightDirection = normalize((light.position.xyz - fragPos));
		vec3 pointLO = CalculateLo(lightDirection, N, V, lightColor, F0, Roughness, Metallic, albedo) * attenuation; 
		Lo += pointLO;
	}

	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, Roughness);

	vec3 Ks = F;
	vec3 Kd = 1.0 - Ks;
	Kd *= 1.0 - Metallic;

	vec3 irradiance = texture(irradianceMap, N).rgb * push.intensity;
	vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  Roughness * MAX_REFLECTION_LOD).rgb; 
    vec2 BRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), Roughness)).rg;
    vec3 specular = prefilteredColor * (F * BRDF.x + BRDF.y);

	vec3 ambient = (Kd * diffuse + specular) * AO; 

	vec3 result = vec3(0);
	result = (1-shadow) * Lo + ambient;
    vec3 hdrColor = result;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * push.exposure);
    // gamma correction 
	mapped = pow(mapped, vec3(1.0 / push.gamma));

	uint i;
	for (i = 0; i < 512 && visibleLightIndicesBuffer.data[offset + i].index != -1; i++);

	float ratio = float(i) / float(512.0);

	fragColor = vec4(mapped, 1.0);
	lightDebug = vec4(vec3(ratio),1.0);
	entityID = id;
}
