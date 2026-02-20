#pragma once

#include "Engine/Renderer/UniformBuffer.h"

#include <volk.h>

struct VmaAllocation_T;

namespace Syndra {

	using VmaAllocation = VmaAllocation_T*;

	class VulkanUniformBuffer : public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		~VulkanUniformBuffer() override;

		void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		VkBuffer GetBuffer() const { return m_Buffer; }
		uint32_t GetBinding() const { return m_Binding; }
		uint32_t GetSize() const { return m_Size; }
		static VulkanUniformBuffer* GetUniformBufferForBinding(uint32_t binding);

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = nullptr;
		void* m_MappedData = nullptr;
		uint32_t m_Size = 0;
		uint32_t m_Binding = 0;
	};

}
