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

struct VS_OUT {
	vec3 v_pos;
	vec2 v_uv;
	vec3 v_normal;
};

layout(location = 0) out VS_OUT vs_out;

void main(){
	vs_out.v_normal = mat3(transpose(inverse(transform.u_trans)))*a_normal;
	vs_out.v_pos = vec3(transform.u_trans*vec4(a_pos,1.0));
	vs_out.v_uv = a_uv;
	gl_Position = cam.u_ViewProjection * transform.u_trans *vec4(a_pos,1.0);
}

#type fragment
#version 460
layout(location = 0) out vec4 fragColor;	

layout(binding = 0) uniform sampler2D texture_diffuse;
layout(binding = 1) uniform sampler2D texture_specular;

layout(binding = 1) uniform Transform
{
	 mat4 u_trans;
	 vec4 lightPos;
} transform;

struct VS_OUT {
    vec3 v_pos;
    vec2 v_uv;
	vec3 v_normal;
};

layout(location = 0) in VS_OUT fs_in;

void main(){	
    vec3 norm = normalize(fs_in.v_normal);
	vec3 lightDir = normalize(transform.lightPos.rgb - fs_in.v_pos);
	vec3 viewDir = normalize(transform.lightPos.rgb - fs_in.v_pos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	//vec3 reflectDir = reflect(-lightDir,norm);

	float diff = max(dot(norm,lightDir),0);

	float spec = pow(max(dot(halfwayDir, norm), 0.0), 4);

	vec3 color = texture(texture_diffuse, fs_in.v_uv).rgb;

	vec3 specular = spec * texture(texture_specular, fs_in.v_uv).rgb;
	vec3 diffuse = diff * color;

	vec3 result = diffuse + specular + vec3(0.06);
	if(color == vec3(0)){
		result = vec3(diff);
	}

	fragColor = vec4(vec3(diff),1.0);
}