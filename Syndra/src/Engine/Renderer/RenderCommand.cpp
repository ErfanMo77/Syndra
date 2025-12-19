#include "lpch.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"

namespace Syndra {

	RendererAPI* RenderCommand::s_RendererAPI = []() -> RendererAPI*
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return new VulkanRendererAPI();
		case RendererAPI::API::OpenGL:
		default:
			return new OpenGLRendererAPI();
		}
	}();

}
