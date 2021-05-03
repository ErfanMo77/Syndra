#include "lpch.h"
#include "Engine/Renderer/UniformBuffer.h"

#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"

namespace Syndra {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLUniformBuffer>(size, binding);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}