#pragma once

#include "Engine/Renderer/Buffer.h"

#include <volk.h>

struct VmaAllocation_T;

namespace Syndra {

	using VmaAllocation = VmaAllocation_T*;

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(float* vertices, uint32_t size);
		~VulkanVertexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		VkBuffer GetBuffer() const { return m_Buffer; }
		uint32_t GetSize() const { return m_Size; }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = nullptr;
		uint32_t m_Size = 0;
		BufferLayout m_Layout;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		~VulkanIndexBuffer() override;

		void Bind() const override;
		void Unbind() const override;

		uint32_t GetCount() const override { return m_Count; }
		VkBuffer GetBuffer() const { return m_Buffer; }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = nullptr;
		uint32_t m_Count = 0;
	};

}
