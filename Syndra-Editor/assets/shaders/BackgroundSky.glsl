// Equirectangular To Cubemap Shader
#type vertex

#version 460
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;



layout(push_constant) uniform camera{
    mat4 view;
    mat4 projection;
}cam;

layout(location = 0) out vec3 WorldPos;

void main()
{
    WorldPos = a_pos;

	mat4 rotView = mat4(mat3(cam.view));
	vec4 clipPos = cam.projection * rotView * vec4(WorldPos, 1.0);

	gl_Position = clipPos.xyww;
}

#type fragment

#version 460
layout(location = 0) out vec4 FragColor;
layout(location = 0) in  vec3 WorldPos;

layout(push_constant) uniform Push{
    float intensity;
}push;

layout(binding = 0) uniform samplerCube environmentMap;
//layout(binding = 1) uniform samplerCube irradianceMap;

void main()
{		
    vec3 envColor = textureLod(environmentMap, WorldPos, push.intensity).rgb;

    // HDR tonemap and gamma correct
    vec3 mapped = vec3(1.0) - exp(-envColor * 0.7);
    // gamma correction 
	mapped = pow(mapped, vec3(1.0 / 1.9));
    
    FragColor = vec4(mapped, 1.0);
}