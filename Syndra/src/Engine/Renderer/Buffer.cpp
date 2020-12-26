#include "lpch.h"
#include "Buffer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"


namespace Syndra {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::NONE:
			SN_CORE_ASSERT(false,"RendererAPI::NONE is not supported!");
			return nullptr;
		case RendererAPI::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(vertices, size);
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* vertices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::NONE:
			SN_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		case RendererAPI::OpenGL:
			return CreateRef<OpenGLIndexBuffer>(vertices, count);
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

}