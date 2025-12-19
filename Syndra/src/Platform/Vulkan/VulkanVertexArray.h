#pragma once

#include "Engine/Renderer/VertexArray.h"

namespace Syndra {

	class VulkanVertexArray : public VertexArray
	{
	public:
		VulkanVertexArray() = default;
		~VulkanVertexArray() override = default;

		void Bind() const override {}
		void UnBind() const override {}

		void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override { m_VertexBuffers.push_back(vertexBuffer); }
		void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override { m_IndexBuffer = indexBuffer; }

		const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer;
	};

}
