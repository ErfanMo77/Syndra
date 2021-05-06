// Basic diffuse Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(std140, binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
};

layout(binding = 1) uniform Transform
{
	 mat4 u_trans;
	 vec4 lightPos;
} transform;



struct VS_OUT {
	vec3 v_pos;
	vec2 v_uv;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
};

layout(location = 0) out VS_OUT vs_out;

void main(){

    vs_out.v_pos = vec3(transform.u_trans * vec4(a_pos, 1.0));   
    vs_out.v_uv = a_uv;

	mat3 normalMatrix = transpose(inverse(mat3(transform.u_trans)));
    vec3 T = normalize(normalMatrix * a_tangent);
    vec3 N = normalize(normalMatrix * a_normal);
	T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));

	vs_out.TangentLightPos = TBN * vec3(transform.lightPos);
    vs_out.TangentViewPos  = TBN * vec3(cameraPos);
    vs_out.TangentFragPos  = TBN * vs_out.v_pos;

	gl_Position = u_ViewProjection * transform.u_trans *vec4(a_pos,1.0);
}

#type fragment
#version 460
layout(location = 0) out vec4 fragColor;	

layout(binding = 0) uniform sampler2D texture_diffuse1;
layout(binding = 1) uniform sampler2D texture_specular1;
layout(binding = 2) uniform sampler2D texture_normal1;

struct VS_OUT {
    vec3 v_pos;
    vec2 v_uv;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
};

layout(location = 0) in VS_OUT fs_in;

void main(){	

	vec3 normal = texture(texture_normal1,fs_in.v_uv).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	//vec3 reflectDir = reflect(-lightDir,norm);

	float diff = max(dot(normal,lightDir),0);

	float spec = pow(max(dot(halfwayDir, normal), 0.0), 64);

	vec3 color = texture(texture_diffuse1,fs_in.v_uv).rgb;

	vec3 specular = spec * texture(texture_specular1,fs_in.v_uv).rgb;
	vec3 diffuse = diff * color;

	vec3 ambient = 0.1 * color;

	vec3 result = diffuse + specular + ambient;
	if(color == vec3(0)){
		result = vec3(diff);
	}
	fragColor = vec4(result,1.0);
}
