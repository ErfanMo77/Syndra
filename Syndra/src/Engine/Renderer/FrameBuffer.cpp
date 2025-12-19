#include "lpch.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"
#include "Platform/Vulkan/VulkanFrameBuffer.h"

namespace Syndra {

	Ref<FrameBuffer> FrameBuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:
			SN_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLFrameBuffer>(spec);
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanFrameBuffer>(spec);
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

}
