#include <Engine.h>
#include "Engine/Core/EntryPoint.h"
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"

class DummyLayer : public Syndra::Layer {

public:
	DummyLayer() :Layer("Dummy") 
	{
	}
	
	~DummyLayer() {
		delete m_Camera;
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("test");
		ImGui::Text("simple text");
		ImGui::End();
	}

	virtual void OnEvent(Syndra::Event& event) override {
	}


	virtual void OnAttach() override
	{

		
	}

	virtual void OnUpdate(Syndra::Timestep ts) override
	{

	}


private:
	Syndra::ShaderLibrary m_Shaders;

	Syndra::Ref<Syndra::VertexArray> m_VertexArray;
	Syndra::Ref<Syndra::VertexBuffer> m_VertexBuffer;
	Syndra::Ref<Syndra::IndexBuffer> m_IndexBuffer;
	Syndra::Ref<Syndra::Texture2D> m_Texture;
	Syndra::PerspectiveCamera* m_Camera;
};


class Sandbox : public Syndra::Application {

public:

	Sandbox() {
		PushLayer(new DummyLayer());
	}

	~Sandbox() {

	}

};

Syndra::Application* Syndra::CreateApplication() {
	return new Sandbox();
}
