#include "lpch.h"
#include "RendererAPI.h"

namespace Syndra {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;
	RendererAPI::API RendererAPI::s_RequestedAPI = RendererAPI::API::Vulkan;

	bool RendererAPI::IsBackendSupported(API api)
	{
		switch (api)
		{
		case API::OpenGL: return true;
		case API::Vulkan: return true;
		case API::NONE: return false;
		}

		return false;
	}

	const char* RendererAPI::APIToString(API api)
	{
		switch (api)
		{
		case API::NONE: return "none";
		case API::Vulkan: return "vulkan";
		case API::OpenGL: return "opengl";
		}

		return "unknown";
	}

	void RendererAPI::SelectAPI(API requestedAPI)
	{
		s_RequestedAPI = requestedAPI;
		if (IsBackendSupported(requestedAPI))
		{
			s_API = requestedAPI;
		}
		else
		{
			SN_WARN("Requested renderer backend '{}' is not available yet. Falling back to '{}'.",
				APIToString(requestedAPI), APIToString(API::OpenGL));
			s_API = API::OpenGL;
		}

		SN_INFO("Renderer backend requested='{}', active='{}'.",
			APIToString(s_RequestedAPI), APIToString(s_API));
	}

}
