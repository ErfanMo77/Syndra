#include "lpch.h"

#include "Platform/Vulkan/VulkanBuffer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "vk_mem_alloc.h"

namespace {

	void CreateHostVisibleBuffer(
		Syndra::VulkanContext* context,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkBuffer& outBuffer,
		Syndra::VmaAllocation& outAllocation,
		VmaAllocationInfo& outAllocationInfo)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required to allocate buffers.");

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		const VkResult result = vmaCreateBuffer(
			context->GetAllocator(),
			&bufferInfo,
			&allocationCreateInfo,
			&outBuffer,
			&outAllocation,
			&outAllocationInfo);

		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer.");
	}

}

namespace Syndra {

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
		: m_Size(size)
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		VmaAllocationInfo allocationInfo{};
		CreateHostVisibleBuffer(
			context,
			size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			m_Buffer,
			m_Allocation,
			allocationInfo);

		if (vertices != nullptr && size > 0)
		{
			SN_CORE_ASSERT(allocationInfo.pMappedData != nullptr, "Mapped buffer allocation was expected for vertex buffer.");
			memcpy(allocationInfo.pMappedData, vertices, static_cast<size_t>(size));
			vmaFlushAllocation(context->GetAllocator(), m_Allocation, 0, size);
		}
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context != nullptr && context->GetAllocator() != nullptr && m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(context->GetAllocator(), m_Buffer, m_Allocation);
		}

		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = nullptr;
	}

	void VulkanVertexBuffer::Bind() const
	{
	}

	void VulkanVertexBuffer::Unbind() const
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		const VkDeviceSize size = static_cast<VkDeviceSize>(count) * sizeof(uint32_t);
		VulkanContext* context = VulkanContext::GetCurrent();
		VmaAllocationInfo allocationInfo{};
		CreateHostVisibleBuffer(
			context,
			size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			m_Buffer,
			m_Allocation,
			allocationInfo);

		if (indices != nullptr && size > 0)
		{
			SN_CORE_ASSERT(allocationInfo.pMappedData != nullptr, "Mapped buffer allocation was expected for index buffer.");
			memcpy(allocationInfo.pMappedData, indices, static_cast<size_t>(size));
			vmaFlushAllocation(context->GetAllocator(), m_Allocation, 0, size);
		}
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context != nullptr && context->GetAllocator() != nullptr && m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(context->GetAllocator(), m_Buffer, m_Allocation);
		}

		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = nullptr;
	}

	void VulkanIndexBuffer::Bind() const
	{
	}

	void VulkanIndexBuffer::Unbind() const
	{
	}

}
