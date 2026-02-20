#pragma once
#include "Application.h"
#include "Instrument.h"
#include "Engine/Renderer/RendererAPI.h"

#include <cctype>
#include <cstdlib>
#include <optional>
#include <string_view>

#ifdef SN_PLATFORM_WINDOWS

extern Syndra::Application* Syndra::CreateApplication();
extern "C" {
	__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

namespace {

	std::optional<Syndra::RendererAPI::API> ParseRendererAPI(std::string_view value)
	{
		std::string normalized;
		normalized.reserve(value.size());
		for (char c : value)
		{
			normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}

		if (normalized == "vulkan")
			return Syndra::RendererAPI::API::Vulkan;
		if (normalized == "opengl")
			return Syndra::RendererAPI::API::OpenGL;
		return std::nullopt;
	}

	void ConfigureRendererBackend(int argc, char** argv)
	{
		Syndra::RendererAPI::API requested = Syndra::RendererAPI::API::Vulkan;
		std::string source = "default";

		std::optional<std::string> cliRenderer;
		std::optional<std::string> envRendererValue;
		for (int i = 1; i < argc; ++i)
		{
			std::string_view arg = argv[i];
			constexpr std::string_view prefix = "--renderer=";
			if (arg.rfind(prefix, 0) == 0)
			{
				cliRenderer = std::string(arg.substr(prefix.size()));
				continue;
			}

			if (arg == "--renderer")
			{
				if (i + 1 < argc)
					cliRenderer = argv[++i];
				else
					SN_ERROR("Missing value for '--renderer'. Expected 'vulkan' or 'opengl'.");
			}
		}

		if (const char* envRenderer = std::getenv("SYNDRA_RENDERER"))
		{
			if (envRenderer[0] != '\0')
			{
				envRendererValue = envRenderer;
				if (auto parsed = ParseRendererAPI(*envRendererValue))
				{
					requested = *parsed;
					source = "environment variable SYNDRA_RENDERER";
				}
				else
				{
					SN_ERROR("Invalid renderer backend '{}' from SYNDRA_RENDERER. Expected 'vulkan' or 'opengl'.", *envRendererValue);
				}
			}
		}

		if (cliRenderer.has_value())
		{
			if (auto parsed = ParseRendererAPI(*cliRenderer))
			{
				requested = *parsed;
				source = "command line";
			}
			else
			{
				SN_ERROR("Invalid renderer backend '{}' from command line. Expected 'vulkan' or 'opengl'.", *cliRenderer);
			}
		}

		SN_INFO(
			"Renderer selection: source='{}', default='vulkan', env='{}', cli='{}'.",
			source,
			envRendererValue.has_value() ? *envRendererValue : "<unset>",
			cliRenderer.has_value() ? *cliRenderer : "<unset>");
		Syndra::RendererAPI::SelectAPI(requested);
	}

}

int main(int argc, char** argv)
{
	Syndra::Log::init();
	SN_WARN("HELLO! Welcome to Syndra!");
	ConfigureRendererBackend(argc, argv);

	auto app = Syndra::CreateApplication();
	SN_PROFILE_BEGIN_SESSION("Runtime", "Runtime.json");
	app->Run();
	SN_PROFILE_END_SESSION();
	delete app;
}

#endif // SN_PLATFORM_WINDOWS
