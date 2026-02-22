#type vertex
#version 460

layout(location = 0) in vec3 a_pos;

layout(set = 0, binding = 3) uniform ShadowData
{
	mat4 dirLightViewProj;
	mat4 spotLightViewProj;
} shadowData;

layout(push_constant) uniform Push
{
	mat4 u_trans;
	int id;
	int useSpot;
} push;

void main()
{
	mat4 lightViewProj = (push.useSpot == 1) ? shadowData.spotLightViewProj : shadowData.dirLightViewProj;
	gl_Position = lightViewProj * push.u_trans * vec4(a_pos, 1.0);
}

#type fragment
#version 460

void main()
{
}
