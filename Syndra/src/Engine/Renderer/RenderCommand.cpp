#include "lpch.h"

#include "Engine/Renderer/RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"

namespace {

	Syndra::RendererAPI::API& GetActiveAPIStorage()
	{
		static Syndra::RendererAPI::API activeAPI = Syndra::RendererAPI::API::NONE;
		return activeAPI;
	}

	std::unique_ptr<Syndra::RendererAPI>& GetRendererAPIStorage()
	{
		static std::unique_ptr<Syndra::RendererAPI> rendererAPI;
		return rendererAPI;
	}

	std::unique_ptr<Syndra::RendererAPI> CreateRendererAPI(Syndra::RendererAPI::API api)
	{
		switch (api)
		{
		case Syndra::RendererAPI::API::Vulkan:
			return std::make_unique<Syndra::VulkanRendererAPI>();
		case Syndra::RendererAPI::API::OpenGL:
			return std::make_unique<Syndra::OpenGLRendererAPI>();
		case Syndra::RendererAPI::API::NONE:
		default:
			return nullptr;
		}
	}

}

namespace Syndra {

	RendererAPI& RenderCommand::GetRendererAPI()
	{
		RendererAPI::API& activeAPI = GetActiveAPIStorage();
		std::unique_ptr<RendererAPI>& rendererAPI = GetRendererAPIStorage();

		const RendererAPI::API requestedAPI = RendererAPI::GetAPI();
		if (!rendererAPI || activeAPI != requestedAPI)
		{
			rendererAPI = CreateRendererAPI(requestedAPI);
			activeAPI = requestedAPI;
		}

		SN_CORE_ASSERT(rendererAPI != nullptr, "Renderer API backend was not created.");
		return *rendererAPI;
	}

	void RenderCommand::Shutdown()
	{
		std::unique_ptr<RendererAPI>& rendererAPI = GetRendererAPIStorage();
		rendererAPI.reset();
		GetActiveAPIStorage() = RendererAPI::API::NONE;
	}

}
