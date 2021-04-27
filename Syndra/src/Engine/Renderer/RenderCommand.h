#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Syndra {

	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		static void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

		static void SetState(RenderState stateID, bool on) 
		{
			s_RendererAPI->SetState(stateID, on);
		}

		static std::string GetInfo() 
		{
			return s_RendererAPI->GetRendererInfo();
		}
	private:
		static RendererAPI* s_RendererAPI;

	};
}