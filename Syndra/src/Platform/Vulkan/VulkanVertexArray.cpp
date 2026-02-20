#include "lpch.h"

#include "Platform/Vulkan/VulkanVertexArray.h"

namespace Syndra {

	const VulkanVertexArray* VulkanVertexArray::s_BoundVertexArray = nullptr;

	void VulkanVertexArray::Bind() const
	{
		s_BoundVertexArray = this;
	}

	void VulkanVertexArray::Unbind() const
	{
		if (s_BoundVertexArray == this)
			s_BoundVertexArray = nullptr;
	}

	void VulkanVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		SN_CORE_ASSERT(vertexBuffer != nullptr, "Vertex buffer cannot be null.");
		SN_CORE_ASSERT(!vertexBuffer->GetLayout().GetElements().empty(), "Vertex buffer layout must be defined before adding.");
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void VulkanVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		SN_CORE_ASSERT(indexBuffer != nullptr, "Index buffer cannot be null.");
		m_IndexBuffer = indexBuffer;
	}

}
