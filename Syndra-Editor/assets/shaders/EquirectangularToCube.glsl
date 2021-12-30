// Equirectangular To Cubemap Shader
#type vertex

#version 460
	
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;

layout(location = 0) out vec3 localPos;

layout(push_constant) uniform camera{
    mat4 view;
    mat4 projection;
}cam;

void main()
{
    localPos = a_pos;  
    gl_Position =  cam.projection * cam.view * vec4(localPos, 1.0);
}

#type fragment

#version 460
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 localPos;

layout(binding = 0) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}