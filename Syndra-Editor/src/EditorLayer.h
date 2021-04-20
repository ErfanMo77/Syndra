#pragma once
#include <Engine.h>
#include "UI/ScenePanel.h"

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
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveSceneAs();

	private:
		ShaderLibrary m_Shaders;

		Ref<Scene> m_ActiveScene;
		Entity m_CubeEntity;
		Entity m_CubeEntity2;

		Ref<VertexArray> m_VertexArray,m_QuadVA;
		Ref<VertexBuffer> m_VertexBuffer,m_QuadVB;
		Ref<FrameBuffer> m_OffScreenFB,m_PostprocFB, m_MousePickFB;
		Ref<IndexBuffer> m_IndexBuffer,m_QuadIB;
		Ref<Texture2D> m_Texture,m_ScreenTex;
		Ref<ScenePanel> m_ScenePanel;
		PerspectiveCamera* m_Camera;

		int m_GizmoType = -1;
		bool altIsDown=false;
		std::string m_Info;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;


		glm::vec2 m_ViewportBounds[2];

		glm::vec2 m_ViewportSize = { 200.0f,200.0f};
		glm::vec3 m_CubeColor = { 1.0f,1.0f,1.0f };
		glm::vec3 m_Scale = { 1.0f,1.0f,1.0f };
		glm::vec3 m_ClearColor = { 0.196f, 0.196f, 0.196f };
	};
}
