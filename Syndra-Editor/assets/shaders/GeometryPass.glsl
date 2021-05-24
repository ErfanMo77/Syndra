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

struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

layout(location = 0) out VS_OUT vs_out;

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

	gl_Position = cam.u_ViewProjection * transform.u_trans * vec4(a_pos, 1.0);
}

#type fragment

#version 460
layout(location = 0) out vec3 gPosistion;	
layout(location = 1) out vec3 gNormal;	
layout(location = 2) out vec4 gAlbedoSpec;

layout(binding = 0) uniform sampler2D DiffuseMap;
layout(binding = 1) uniform sampler2D SpecularMap;
layout(binding = 2) uniform sampler2D NormalMap;

struct VS_OUT
{
	vec3 v_pos;
	vec3 v_normal;
	vec2 v_uv;
	mat3 TBN;
};

layout(location = 0) in VS_OUT fs_in;

void main()
{
	gPosistion = fs_in.v_pos;

	gAlbedoSpec.rgb = texture(DiffuseMap, fs_in.v_uv).rgb;

	gAlbedoSpec.a = texture(SpecularMap, fs_in.v_uv).r;

	vec3 normal = texture(NormalMap, fs_in.v_uv).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(fs_in.TBN * normal);
	
	normal = fs_in.v_normal;

	gNormal = normal; 
}