#include "lpch.h"

#include "Platform/Vulkan/VulkanUniformBuffer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "vk_mem_alloc.h"

namespace Syndra {

	namespace {

		std::unordered_map<uint32_t, VulkanUniformBuffer*>& GetUniformBufferRegistry()
		{
			static std::unordered_map<uint32_t, VulkanUniformBuffer*> registry;
			return registry;
		}

	}

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding)
		: m_Size(size), m_Binding(binding)
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for uniform buffer allocation.");

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocationInfo{};
		const VkResult result = vmaCreateBuffer(
			context->GetAllocator(),
			&bufferInfo,
			&allocationCreateInfo,
			&m_Buffer,
			&m_Allocation,
			&allocationInfo);

		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan uniform buffer.");
		m_MappedData = allocationInfo.pMappedData;
		SN_CORE_ASSERT(m_MappedData != nullptr, "Uniform buffer allocation must be host-mapped.");

		GetUniformBufferRegistry()[m_Binding] = this;
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		auto& registry = GetUniformBufferRegistry();
		const auto it = registry.find(m_Binding);
		if (it != registry.end() && it->second == this)
			registry.erase(it);

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context != nullptr && context->GetAllocator() != nullptr && m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(context->GetAllocator(), m_Buffer, m_Allocation);
		}

		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = nullptr;
		m_MappedData = nullptr;
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		if (data == nullptr || size == 0)
			return;

		SN_CORE_ASSERT(offset + size <= m_Size, "Uniform buffer write out of range.");
		memcpy(static_cast<uint8_t*>(m_MappedData) + offset, data, size);

		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for uniform buffer upload.");
		vmaFlushAllocation(
			context->GetAllocator(),
			m_Allocation,
			static_cast<VkDeviceSize>(offset),
			static_cast<VkDeviceSize>(size));
	}

	VulkanUniformBuffer* VulkanUniformBuffer::GetUniformBufferForBinding(uint32_t binding)
	{
		auto& registry = GetUniformBufferRegistry();
		const auto it = registry.find(binding);
		if (it == registry.end())
			return nullptr;

		return it->second;
	}

}
