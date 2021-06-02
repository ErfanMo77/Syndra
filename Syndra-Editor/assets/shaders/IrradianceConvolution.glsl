// Convoluting cubemap 
#type vertex

#version 460
	
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout(location = 0) out vec3 worldPos;

layout(push_constant) uniform camera{
    mat4 view;
    mat4 projection;
}cam;

void main()
{
    worldPos = a_pos;  
    gl_Position =  cam.projection * cam.view * vec4(worldPos, 1.0);
}

#type fragment

#version 460
layout(location = 0) out vec4 FragColor;
layout(location = 0) in  vec3 worldPos;

layout(binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{		
    vec3 N = normalize(worldPos);

    vec3 irradiance = vec3(0.0);   
    
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 
            vec3 color = texture(environmentMap, sampleVec).rgb;
            float r = min(color.r, 200);
            float g = min(color.g, 200);
            float b = min(color.b, 200);
            irradiance += vec3(r,g,b) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    FragColor = vec4(irradiance, 1.0);
}