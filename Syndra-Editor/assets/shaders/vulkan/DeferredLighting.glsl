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

layout(set = 0, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

layout(set = 0, binding = 3) uniform ShadowData
{
	mat4 lightViewProj;
} shadowData;

struct PointLight
{
	vec4 position;
	vec4 color;
};

struct DirLight
{
	vec4 direction;
	vec4 color;
};

layout(set = 0, binding = 2) uniform Lights
{
	PointLight pLight[4];
	DirLight dLight;
} lights;

layout(push_constant) uniform Push
{
	float exposure;
	float gamma;
	float intensity;
	int useShadows;
	int useIBL;
	float iblStrength;
} push;

layout(set = 1, binding = 0) uniform sampler2D gPosition;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gAlbedo;
layout(set = 1, binding = 3) uniform sampler2D gRoughMetalAO;
layout(set = 1, binding = 4) uniform sampler2D shadowMap;
layout(set = 1, binding = 5) uniform sampler2D environmentMap;

layout(location = 0) in vec2 v_uv;

const float PI = 3.14159265358979323846;

vec2 DirectionToEquirectUV(vec3 direction)
{
	vec3 dir = normalize(direction);
	float longitude = atan(dir.z, dir.x);
	float latitude = asin(clamp(dir.y, -1.0, 1.0));
	return vec2(longitude / (2.0 * PI) + 0.5, latitude / PI + 0.5);
}

vec3 SampleEnvironment(vec3 direction)
{
	return texture(environmentMap, DirectionToEquirectUV(direction)).rgb;
}

float ComputeDirectionalShadow(vec3 worldPosition, vec3 normal, vec3 lightDirection)
{
	if (push.useShadows == 0)
		return 0.0;

	vec4 lightSpacePosition = shadowData.lightViewProj * vec4(worldPosition, 1.0);
	vec3 projected = lightSpacePosition.xyz / max(lightSpacePosition.w, 0.0001);
	vec2 shadowUV = projected.xy * 0.5 + 0.5;

	if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || projected.z < 0.0 || projected.z > 1.0)
		return 0.0;

	float closestDepth = texture(shadowMap, shadowUV).r;
	float normalAlignment = clamp(dot(normal, lightDirection), 0.0, 1.0);
	float bias = max(0.0005, 0.0025 * (1.0 - normalAlignment));
	return (projected.z - bias > closestDepth) ? 1.0 : 0.0;
}

void main()
{
	vec3 position = texture(gPosition, v_uv).xyz;
	vec3 normalSample = texture(gNormal, v_uv).xyz;
	vec3 normal = normalize(normalSample);
	vec3 albedo = texture(gAlbedo, v_uv).rgb;
	vec3 rma = texture(gRoughMetalAO, v_uv).xyz;

	// Background pixels preserve clear-color in gNormal and have a very small length.
	if (dot(normalSample, normalSample) < 0.25)
	{
		if (push.useIBL == 1)
		{
			vec2 ndc = v_uv * 2.0 - 1.0;
			vec4 clipPosition = vec4(ndc, 1.0, 1.0);
			mat4 inverseViewProjection = inverse(cam.u_ViewProjection);
			vec4 worldPosition = inverseViewProjection * clipPosition;
			vec3 viewDirection = normalize(worldPosition.xyz / max(worldPosition.w, 0.0001) - cam.cameraPos.xyz);
			vec3 environmentColor = SampleEnvironment(viewDirection) * push.iblStrength;
			vec3 mapped = vec3(1.0) - exp(-environmentColor * max(push.exposure, 0.001));
			mapped = pow(mapped, vec3(1.0 / max(push.gamma, 0.001)));
			fragColor = vec4(mapped, 1.0);
		}
		else
		{
			fragColor = vec4(0.08, 0.08, 0.10, 1.0);
		}
		return;
	}

	float roughness = clamp(rma.x, 0.02, 1.0);
	float metallic = clamp(rma.y, 0.0, 1.0);
	float ao = clamp(rma.z, 0.0, 1.0);

	vec3 V = normalize(cam.cameraPos.xyz - position);
	vec3 color = vec3(0.0);

	vec3 dirL = normalize(-lights.dLight.direction.xyz);
	float dirNdotL = max(dot(normal, dirL), 0.0);
	float shadowFactor = ComputeDirectionalShadow(position, normal, dirL);
	color += albedo * lights.dLight.color.rgb * dirNdotL * (1.0 - shadowFactor);

	for (int i = 0; i < 4; ++i)
	{
		vec3 lightPos = lights.pLight[i].position.xyz;
		vec3 L = lightPos - position;
		float distanceSqr = max(dot(L, L), 0.001);
		L = normalize(L);
		float attenuation = 1.0 / distanceSqr;
		float ndotl = max(dot(normal, L), 0.0);
		color += albedo * lights.pLight[i].color.rgb * ndotl * attenuation;
	}

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(normal, V), 0.0), 5.0);
	vec3 ambient = (1.0 - fresnel) * albedo * ao * 0.08;
	if (push.useIBL == 1)
	{
		vec3 R = reflect(-V, normal);
		float maxMip = max(float(textureQueryLevels(environmentMap) - 1), 0.0);
		float specularMip = roughness * maxMip;
		float diffuseMip = maxMip;
		vec3 envDiffuse = textureLod(environmentMap, DirectionToEquirectUV(normal), diffuseMip).rgb;
		vec3 envSpecular = textureLod(environmentMap, DirectionToEquirectUV(R), specularMip).rgb;
		vec3 kd = (1.0 - fresnel) * (1.0 - metallic);
		float specularVisibility = pow(max(1.0 - roughness, 0.0), 4.0);
		vec3 ibl = (kd * envDiffuse * albedo + envSpecular * fresnel * specularVisibility) * ao;
		ambient += ibl * push.iblStrength;
	}

	vec3 lit = (color + ambient) * push.intensity;
	vec3 mapped = vec3(1.0) - exp(-lit * max(push.exposure, 0.001));
	mapped = pow(mapped, vec3(1.0 / max(push.gamma, 0.001)));
	fragColor = vec4(mapped, 1.0);
}
