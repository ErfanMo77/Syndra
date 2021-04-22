#pragma once
#include <glm/glm.hpp>
#include "Engine/Renderer/VertexArray.h"

namespace Syndra {

	class RendererAPI {

	public:
		enum class API {
			NONE = 0,
			OpenGL = 1
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4 & color) = 0;
		virtual void Clear() = 0;
		virtual void DrawIndexed(const Ref<VertexArray>&vertexArray) = 0;
		virtual void SetState(int stateID, bool on) = 0;

		virtual std::string GetRendererInfo() = 0;

		static API GetAPI() { return s_API; }
		//static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}