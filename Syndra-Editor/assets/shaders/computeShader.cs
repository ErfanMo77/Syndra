// Compute Shader
#type compute
#version 460

layout(local_size_x = 10, local_size_y = 10, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform image2D imgOutput;

void main() {
	vec4 value = vec4(1.0, 0.0, 0.0, 1.0);
	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	imageStore(imgOutput, texelCoord, value);
}