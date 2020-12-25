#pragma once

namespace Syndra {

	enum class RendererAPI {
		NONE = 0,
		OpenGL = 1
	};

	class Renderer
	{
	public:
		inline static RendererAPI GetAPI() { return s_RendererAPI; }
		inline static void SetAPI(RendererAPI api) { s_RendererAPI = api; }

	private:
		static RendererAPI s_RendererAPI;
		
	};

}