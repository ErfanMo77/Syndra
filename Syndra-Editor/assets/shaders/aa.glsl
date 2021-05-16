// Basic diffuse Shader
#type vertex

#version 460
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;

void main(){
	v_uv = a_uv;
	gl_Position = vec4(a_pos,1.0f);
}

#type fragment

#version 460
layout(location = 0) out vec4 fragColor;	

layout(binding = 0) uniform sampler2DMS u_Texture;

layout(push_constant) uniform pushConstants{
	float exposure;
	float gamma;
} pc;

layout(location = 0) in vec2 v_uv;

void main()
{
	ivec2 texturePosition = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	vec4 colorSample0 = texelFetch(u_Texture, texturePosition, 0);
	vec4 colorSample1 = texelFetch(u_Texture, texturePosition, 1);
	vec4 colorSample2 = texelFetch(u_Texture, texturePosition, 2);
	vec4 colorSample3 = texelFetch(u_Texture, texturePosition, 3);

	vec4 antialiased = (colorSample0 + colorSample1 + colorSample2 + colorSample3) / 4.0f;

    vec3 hdrColor = antialiased.rgb;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * pc.exposure);
    // gamma correction 
	mapped = pow(mapped, vec3(1.0 / pc.gamma));
	
	fragColor = vec4(mapped, 1.0);
}