#pragma once

#include "Engine/Renderer/Buffer.h"

namespace Syndra {

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(float* vertices, uint32_t size);
		~VulkanVertexBuffer() override = default;

		void Bind() const override {}
		void UnBind() const override {}

		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
		const BufferLayout& GetLayout() const override { return m_Layout; }
	private:
		BufferLayout m_Layout;
		std::vector<float> m_Data;
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		~VulkanIndexBuffer() override = default;

		void Bind() const override {}
		void UnBind() const override {}

		uint32_t GetCount() const override { return m_Count; }
	private:
		uint32_t m_Count;
		std::vector<uint32_t> m_Data;
	};

}
