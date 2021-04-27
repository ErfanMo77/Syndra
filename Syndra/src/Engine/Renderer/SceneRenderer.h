#pragma once
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "entt.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Syndra {

	class Entity;

	class SceneRenderer 
	{
	public:
		static void Initialize();

		static void BeginScene(const PerspectiveCamera& camera);

		static void RenderEntity(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc);

		static void EndScene();

		static void OnViewPortResize(uint32_t width, uint32_t height);

		static uint32_t GetTextureID(int index) { return s_Data->postProcFB->GetColorAttachmentRendererID(index); }

		static Ref<FrameBuffer> GetMouseFrameBuffer() { return s_Data->mouseFB; }

		static FramebufferSpecification GetMainFrameSpec() { return s_Data->mainFB->GetSpecification(); }

	private:
		struct SceneData
		{
			PerspectiveCamera camera;
			ShaderLibrary shaders;
			Ref<Shader> diffuse,outline,mouseShader,aa;
			Ref<FrameBuffer> mainFB, mouseFB, postProcFB;
			Ref<VertexArray> screenVao;
			glm::vec3 clearColor;
		};

		static SceneData* s_Data;
	};

}