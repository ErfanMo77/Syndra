#type vertex
#version 460

layout(location = 0) in vec3 a_pos;

layout(set = 0, binding = 3) uniform ShadowData
{
	mat4 lightViewProj;
} shadowData;

layout(push_constant) uniform Push
{
	mat4 u_trans;
	int id;
} push;

void main()
{
	gl_Position = shadowData.lightViewProj * push.u_trans * vec4(a_pos, 1.0);
}

#type fragment
#version 460

void main()
{
}
