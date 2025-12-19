#include "lpch.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"
#include <cstring>

namespace Syndra {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding)
		: m_Data(size), m_Binding(binding)
	{
		(void)m_Binding;
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		if (offset + size > m_Data.size())
			m_Data.resize(offset + size);

		std::memcpy(m_Data.data() + offset, data, size);
	}

}
