#pragma once
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/UniformBuffer.h"
#include "entt.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Syndra {

	class Entity;
	class Scene;

	class SceneRenderer 
	{
	public:
		static void Initialize();

		static void BeginScene(const PerspectiveCamera& camera);

		static void RenderScene(Scene& scene);

		static void RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc, const Ref<Shader>& shader);
		static void RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc, MaterialComponent& mat);

		static void RenderEntityID(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc);

		static void EndScene();

		static void Reload();

		static void OnViewPortResize(uint32_t width, uint32_t height);

		static void OnImGuiUpdate();

		static uint32_t GetTextureID(int index) { return s_Data->postProcFB->GetColorAttachmentRendererID(index); }

		static Ref<FrameBuffer> GetMouseFrameBuffer() { return s_Data->mouseFB; }

		static FramebufferSpecification GetMainFrameSpec() { return s_Data->mainFB->GetSpecification(); }

		static ShaderLibrary& GetShaderLibrary() { return s_Data->shaders; }

	private:

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 position;
		};

		struct Transform
		{
			glm::mat4 transform;
			glm::vec4 lightPos;
		};

		struct pointLight
		{
			glm::vec4 position;
			glm::vec4 color;
			float dist;
			glm::vec3 dummy;
		};
		
		struct spotLight {
			glm::vec4 position;
			glm::vec4 color;
			glm::vec4 direction;
			float innerCutOff;
			float outerCutOff;
			glm::vec2 dummy;
		};

		struct directionalLight
		{
			glm::vec4 position;
			glm::vec4 direction;
			glm::vec4 color;
		};

		struct ShaderData
		{
			glm::vec4 col;
		};

		struct ShadowData {
			glm::mat4 lightViewProj;
		};

		struct SceneData
		{
			CameraData CameraBuffer;
			ShaderData ShaderBuffer;
			//Light
			float exposure;
			float gamma;
			Transform TransformBuffer;
			directionalLight dirLight;
			pointLight pointLights[4];
			spotLight spotLights[4];
			Ref<UniformBuffer> CameraUniformBuffer, TransformUniformBuffer, LightsBuffer, ShadowBuffer;
			//Shadow
			Ref<FrameBuffer> shadowFB;
			glm::mat4 lightProj;
			glm::mat4 lightView;
			ShadowData shadowData;
			//Poisson
			Ref<Texture1D> distributionSampler;
			//shaders
			ShaderLibrary shaders;
			Ref<Shader> diffuse,outline,mouseShader,aa, main, depth;
			//FrameBuffers
			Ref<FrameBuffer> mainFB, mouseFB, postProcFB;
			//Scene quad VBO, VAO, EBO
			Ref<VertexArray> screenVao;
			Ref<VertexBuffer> screenVbo;
			Ref<IndexBuffer> screenEbo;
			//Clear color
			glm::vec3 clearColor;
		};

		static SceneData* s_Data;
	};

}