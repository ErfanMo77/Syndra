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
#define linear 0.09
#define quadratic 0.032

struct PointLight {
    vec4 position;
    vec4 color;
	float dist;
};

layout(binding = 2) uniform pointlights
{
	PointLight light[4];
} pointLights;

struct DirLight {
    vec4 position;
    vec4 dir;
	vec4 color;
};

layout(binding = 3) uniform dirLight
{
	DirLight dirlight;
} directionalLight;

struct VS_OUT {
    vec3 v_pos;
    vec2 v_uv;
	vec3 v_normal;
};

layout(location = 0) in VS_OUT fs_in;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 col);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir,vec3 col);

void main(){	

    vec3 norm = normalize(fs_in.v_normal);
	vec3 viewDir = normalize(cam.cameraPos.rgb - fs_in.v_pos);

	vec3 color = texture(texture_diffuse, fs_in.v_uv).rgb;

	vec3 result = vec3(0);
	result += CalcDirLight(directionalLight.dirlight, norm, viewDir,color);
	for(int i = 0; i<4; i++){
		result += CalcPointLight(pointLights.light[i], norm, fs_in.v_pos, viewDir,color);
	}

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
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));  
	if(distance < light.dist){
		attenuation = 1.;
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