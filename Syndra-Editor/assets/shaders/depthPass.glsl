// Depth map generator
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
}transform;


void main(){
	gl_Position = cam.u_ViewProjection * transform.u_trans * vec4(a_pos,1.0f);
}

#type fragment

#version 460
	
void main()
{
}