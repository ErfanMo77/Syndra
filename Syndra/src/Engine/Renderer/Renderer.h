#pragma once
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Renderer/OrthographicCamera.h"

namespace Syndra {

	class Renderer
	{
	public:
		static void BeginScene(const PerspectiveCamera& camera);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray);

		static void OnWindowResize(uint32_t width, uint32_t height);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};

}