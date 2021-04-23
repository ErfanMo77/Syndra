#pragma once
#include "Engine/Renderer/RendererAPI.h"

namespace Syndra {

	class OpenGLRendererAPI : public RendererAPI {

	public:


		virtual void Init() override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetState(int stateID, bool on) override;

		virtual std::string GetRendererInfo() override;

	};

}