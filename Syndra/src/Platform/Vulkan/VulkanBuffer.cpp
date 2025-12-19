#include "lpch.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace Syndra {

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
	{
		m_Data.assign(vertices, vertices + size / sizeof(float));
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		m_Data.assign(indices, indices + count);
	}

}
