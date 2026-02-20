#type vertex
#version 460

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

layout(location = 0) out vec2 v_uv;

void main()
{
	v_uv = a_uv;
	gl_Position = vec4(a_pos, 1.0);
}

#type fragment
#version 460

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform Push
{
	float width;
	float height;
} push;

void main()
{
	vec2 invResolution = vec2(1.0 / max(push.width, 1.0), 1.0 / max(push.height, 1.0));

	vec3 rgbM = texture(u_Texture, v_uv).rgb;
	vec3 rgbN = texture(u_Texture, v_uv + vec2(0.0, invResolution.y)).rgb;
	vec3 rgbS = texture(u_Texture, v_uv - vec2(0.0, invResolution.y)).rgb;
	vec3 rgbE = texture(u_Texture, v_uv + vec2(invResolution.x, 0.0)).rgb;
	vec3 rgbW = texture(u_Texture, v_uv - vec2(invResolution.x, 0.0)).rgb;

	vec3 lumaWeights = vec3(0.299, 0.587, 0.114);
	float lumaM = dot(rgbM, lumaWeights);
	float lumaN = dot(rgbN, lumaWeights);
	float lumaS = dot(rgbS, lumaWeights);
	float lumaE = dot(rgbE, lumaWeights);
	float lumaW = dot(rgbW, lumaWeights);

	float edgeHorizontal = abs(lumaE - lumaW);
	float edgeVertical = abs(lumaN - lumaS);
	float edge = max(edgeHorizontal, edgeVertical);
	float blend = smoothstep(0.02, 0.12, edge);

	vec3 blur = (rgbN + rgbS + rgbE + rgbW) * 0.25;
	vec3 finalColor = mix(rgbM, blur, blend * 0.6);
	fragColor = vec4(finalColor, 1.0);
}
