// Basic diffuse Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

layout(binding = 3) uniform ShadowData
{
	 mat4 lightViewProj;
} shadow;

layout(push_constant) uniform Transform
{
	mat4 u_trans;
}transform;

void main(){
	gl_Position = shadow.lightViewProj * transform.u_trans * vec4(a_pos,1.0f);
}

#type fragment

#version 460
void main()
{
}