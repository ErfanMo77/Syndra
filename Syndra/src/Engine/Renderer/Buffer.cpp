#include "lpch.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"


namespace Syndra {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:
			SN_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(vertices, size);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(vertices, size);
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* vertices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:
			SN_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLIndexBuffer>(vertices, count);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(vertices, count);
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

}
