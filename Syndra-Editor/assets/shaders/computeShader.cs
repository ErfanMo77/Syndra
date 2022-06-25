// Compute Shader
#type compute
#version 460

layout(local_size_x = 10, local_size_y = 10, local_size_z = 1) in;

layout(binding = 0) uniform camera
{
	mat4 u_ViewProjection;
	vec4 cameraPos;
} cam;

struct PointLight
{
	vec4 position;
	vec4 color;
	float dist;
};

struct VisibleIndex
{
	int index;
};

// Shader storage buffer objects
layout(std430, binding = 0) readonly buffer LightBuffer
{
	PointLight data [];
}
lightBuffer;

layout(std430, binding = 1) writeonly buffer VisibleLightIndicesBuffer {
	VisibleIndex data[] ;
} visibleLightIndicesBuffer;

// Uniforms
//uniform sampler2D depthMap;
//uniform mat4 view;
//uniform mat4 projection;
//uniform ivec2 screenSize;
//uniform int lightCount;

// Shared values between all the threads in the group
shared uint minDepthInt;
shared uint maxDepthInt;
shared uint visibleLightCount;
shared vec4 frustumPlanes[6] ;
// Shared local storage for visible indices, will be written out to the global buffer at the end
shared int visibleLightIndices[1024];
shared mat4 viewProjection;

layout(binding = 0) uniform sampler2D depthMap;

layout(rgba8, binding = 0) uniform image2D imgOutput;

void main() {
	vec4 value = vec4(1.0, 0.0, 0.0, 1.0);
	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	imageStore(imgOutput, texelCoord, value);
}