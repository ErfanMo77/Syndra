#pragma once
#include <Engine.h>

namespace Syndra {

	class EditorLayer : public Layer
	{

	public:
		EditorLayer();
		~EditorLayer();
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

	private:
		ShaderLibrary m_Shaders;
		Ref<VertexArray> m_VertexArray,m_QuadVA;
		Ref<VertexBuffer> m_VertexBuffer,m_QuadVB;
		Ref<FrameBuffer> m_OffScreenFB,m_PostprocFB;
		Ref<IndexBuffer> m_IndexBuffer,m_QuadIB;
		Ref<Texture2D> m_Texture,m_ScreenTex;
		PerspectiveCamera* m_Camera;

		glm::vec2 m_ViewportSize = { 200.0f,200.0f};
		glm::vec3 m_CubeColor = { 1.0f,1.0f,1.0f };
		glm::vec3 m_Scale = { 1.0f,1.0f,1.0f };
		glm::vec3 m_ClearColor = { 0.1f, 0.1f, 0.1f };
	};
}
