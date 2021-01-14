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
		Ref<VertexArray> m_VertexArray;
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<FrameBuffer> m_Framebuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<Texture2D> m_Texture;
		PerspectiveCamera* m_Camera;
	};
}

