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
	int id;
}transform;

layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;


struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

layout(location = 0) out VS_OUT vs_out;
layout(location = 8) out flat int id;

void main()
{
	vs_out.v_pos = vec3(transform.u_trans*vec4(a_pos,1.0));

	mat3 normalMatrix = transpose(inverse(mat3(transform.u_trans)));
    vec3 T = normalize(normalMatrix * a_tangent);
    vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	vs_out.v_normal = N;

	vs_out.TBN = mat3(T, B, N);

	vs_out.v_uv = a_uv;

	id = transform.id;

	gl_Position = cam.u_ViewProjection * transform.u_trans * vec4(a_pos, 1.0);
}

#type fragment

#version 460
layout(location = 0) out vec3 gPosistion;	
layout(location = 1) out vec3 gNormal;	
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec3 gRoughMetalAO;
layout(location = 4) out int  gEntityID;


layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D metallicMap;
layout(binding = 2) uniform sampler2D NormalMap;
layout(binding = 3) uniform sampler2D RoughnessMap;
layout(binding = 4) uniform sampler2D AmbientOcclusionMap;

struct Material
{
	vec4 color;
	float RoughnessFactor;
	float MetallicFactor;
	float AO;
};

layout(push_constant) uniform Push{
	Material material;
	int id;
	float tiling;
	int HasAlbedoMap;
	int HasNormalMap;
	int HasRoughnessMap;
	int HasMetallicMap;
	int HasAOMap;
}push;

struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

layout(location = 0) in VS_OUT fs_in;
layout(location = 8) in	flat int id;

void main()
{
	//////////////////////////////////////POSITION//////////////////////////////////////////
	gPosistion = fs_in.v_pos;
	vec2 uv = fs_in.v_uv * push.tiling;

	//////////////////////////////////////ALBEDO////////////////////////////////////////////
	if(push.HasAlbedoMap==1){
		gAlbedoSpec.rgb = texture(AlbedoMap, uv).rgb;
	}else {
		gAlbedoSpec.rgb = push.material.color.rgb;
	}
	gAlbedoSpec.a = 1.0;

	//////////////////////////////////////NORMAL////////////////////////////////////////////
	if(push.HasNormalMap==1){
		vec3 normal = texture(NormalMap, uv).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		gNormal = normalize(fs_in.TBN * normal);
	}
	else{
		gNormal = fs_in.v_normal;
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

	gRoughMetalAO = vec3(Roughness, Metallic, AO);

	//////////////////////////////////ENTITY ID////////////////////////////////////////////
	gEntityID = id;
}