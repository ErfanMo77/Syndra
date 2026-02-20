#type vertex
#version 460

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

layout(push_constant) uniform Push
{
	mat4 u_trans;
	int id;
	float tiling;
	int HasAlbedoMap;
	int HasNormalMap;
	int HasRoughnessMap;
	int HasMetallicMap;
	int HasAOMap;
	vec4 color;
	float RoughnessFactor;
	float MetallicFactor;
	float AO;
} push;

struct VS_OUT
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 uv;
	mat3 tbn;
};

layout(location = 0) out VS_OUT vs_out;
layout(location = 8) out flat int v_entityID;

void main()
{
	vec4 worldPos = push.u_trans * vec4(a_pos, 1.0);
	mat3 normalMatrix = transpose(inverse(mat3(push.u_trans)));

	vec3 T = normalize(normalMatrix * a_tangent);
	vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	vs_out.worldPos = worldPos.xyz;
	vs_out.worldNormal = N;
	vs_out.uv = a_uv;
	vs_out.tbn = mat3(T, B, N);
	v_entityID = push.id;

	gl_Position = cam.u_ViewProjection * worldPos;
}

#type fragment
#version 460

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gRoughMetalAO;
layout(location = 4) out int gEntityID;

layout(set = 1, binding = 0) uniform sampler2D AlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D MetallicMap;
layout(set = 1, binding = 2) uniform sampler2D NormalMap;
layout(set = 1, binding = 3) uniform sampler2D RoughnessMap;
layout(set = 1, binding = 4) uniform sampler2D AOMap;

layout(push_constant) uniform Push
{
	mat4 u_trans;
	int id;
	float tiling;
	int HasAlbedoMap;
	int HasNormalMap;
	int HasRoughnessMap;
	int HasMetallicMap;
	int HasAOMap;
	vec4 color;
	float RoughnessFactor;
	float MetallicFactor;
	float AO;
} push;

struct VS_OUT
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 uv;
	mat3 tbn;
};

layout(location = 0) in VS_OUT fs_in;
layout(location = 8) in flat int v_entityID;

void main()
{
	vec2 uv = fs_in.uv * push.tiling;
	vec3 normal = normalize(fs_in.worldNormal);
	if (push.HasNormalMap == 1)
	{
		vec3 mapNormal = texture(NormalMap, uv).xyz * 2.0 - 1.0;
		normal = normalize(fs_in.tbn * mapNormal);
	}

	vec3 albedo = push.color.rgb;
	if (push.HasAlbedoMap == 1)
		albedo = texture(AlbedoMap, uv).rgb;

	float roughness = push.RoughnessFactor;
	if (push.HasRoughnessMap == 1)
		roughness *= texture(RoughnessMap, uv).r;

	float metallic = push.MetallicFactor;
	if (push.HasMetallicMap == 1)
		metallic *= texture(MetallicMap, uv).r;

	float ao = push.AO;
	if (push.HasAOMap == 1)
		ao *= texture(AOMap, uv).r;

	gPosition = vec4(fs_in.worldPos, 1.0);
	gNormal = vec4(normal, 1.0);
	gAlbedo = vec4(albedo, 1.0);
	gRoughMetalAO = vec4(roughness, metallic, ao, 1.0);
	gEntityID = v_entityID;
}
