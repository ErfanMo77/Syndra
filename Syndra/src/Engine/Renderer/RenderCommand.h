#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Syndra {

	class RenderCommand
	{
	public:
		static void Init()
		{
			GetRendererAPI().Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			GetRendererAPI().SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			GetRendererAPI().SetClearColor(color);
		}

		static void Clear()
		{
			GetRendererAPI().Clear();
		}

		static void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			GetRendererAPI().DrawIndexed(vertexArray);
		}

		static void SetState(RenderState stateID, bool on)
		{
			GetRendererAPI().SetState(stateID, on);
		}

		static std::string GetInfo()
		{
			return GetRendererAPI().GetRendererInfo();
		}

		static void Shutdown();
	private:
		static RendererAPI& GetRendererAPI();

	};
}
