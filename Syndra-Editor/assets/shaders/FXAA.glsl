/**
 * @license
 * Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
 *
 * TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
 * *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
 * OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
 * OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
 * CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
 * OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
 * EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

// FXAA SHADER
#type vertex
#version 460

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec4 v_uv;

layout(push_constant) uniform push{
    float width;
    float height;
}pc;

void main()
{
    gl_Position = vec4(a_pos,1.0);
    v_uv.xy = a_uv * vec2(pc.width, pc.height);

    vec2 rcpFrame = vec2(1/(pc.width),1/(pc.height));
	v_uv.wz = a_uv - (rcpFrame * (0.5));
}

#type fragment
#version 460

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_uv;

layout(binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform push{
    float width;
    float height;
}pc;

const float FXAA_REDUCE_MUL = 1.0 / 16.0;
const float FXAA_REDUCE_MIN = 1.0/ 1024.0;

void main()
{
    vec2 resolution = vec2(pc.width, pc.height);
    vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);

    vec2 v_rgbNW = (v_uv.xy + vec2(-1.0, -1.0)) * inverseVP;
	vec2 v_rgbNE = (v_uv.xy + vec2(1.0, -1.0)) * inverseVP;
	vec2 v_rgbSW = (v_uv.xy + vec2(-1.0, 1.0)) * inverseVP;
	vec2 v_rgbSE = (v_uv.xy + vec2(1.0, 1.0)) * inverseVP;
	vec2 v_rgbM = vec2(v_uv.xy * inverseVP);

    vec3 rgbNW = texture(u_Texture, v_rgbNW).xyz;
    vec3 rgbNE = texture(u_Texture, v_rgbNE).xyz;
    vec3 rgbSW = texture(u_Texture, v_rgbSW).xyz;
    vec3 rgbSE = texture(u_Texture, v_rgbSE).xyz;
    vec3 rgbM  = texture(u_Texture, v_rgbM).xyz;

    vec3  luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(32.0, 32.0),
              max(vec2(-32.0, -32.0),
              dir * rcpDirMin)) * inverseVP;

    vec3 rgbA = 0.5 * (
        texture(u_Texture, v_uv.xy * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture(u_Texture, v_uv.xy * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(u_Texture, v_uv.xy * inverseVP + dir * -0.5).xyz +
        texture(u_Texture, v_uv.xy * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    vec4 color;
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = vec4(rgbA, 1.0);
    else
        color = vec4(rgbB, 1.0);

    FragColor = color;
} 