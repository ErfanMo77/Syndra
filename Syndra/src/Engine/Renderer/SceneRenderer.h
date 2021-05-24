#pragma once
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/UniformBuffer.h"
#include "Engine/Renderer/RenderPass.h"
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
		static void UpdateLights(Scene& scene);
		static void RenderScene(Scene& scene);

		static void RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc, const Ref<Shader>& shader);
		static void RenderEntityColor(const entt::entity& entity, TransformComponent& tc, MeshComponent& mc, MaterialComponent& mat);

		static void EndScene();

		static void Reload();

		static void OnViewPortResize(uint32_t width, uint32_t height);

		static void OnImGuiUpdate();

		static uint32_t GetTextureID(int index);

		static FramebufferSpecification GetMainFrameSpec();

		static ShaderLibrary& GetShaderLibrary();


	public:

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 position;
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

		struct ShadowData {
			glm::mat4 lightViewProj;
		};

		struct DrawCall {
			entt::entity id;
			TransformComponent tc;
			MeshComponent mc;
		};

		struct SceneData
		{
			CameraData CameraBuffer;
			//Light
			float exposure;
			float gamma;
			float lightSize;
			directionalLight dirLight;
			pointLight pointLights[4];
			spotLight spotLights[4];
			Ref<UniformBuffer> CameraUniformBuffer, LightsBuffer, ShadowBuffer;
			//Shadow
			bool softShadow = false;
			float numPCF = 8;
			float numBlocker = 3;
			glm::mat4 lightProj;
			glm::mat4 lightView;
			ShadowData shadowData;
			//Poisson
			Ref<Texture1D> distributionSampler0, distributionSampler1;
			//shaders
			ShaderLibrary shaders;
			Ref<Shader> diffuse, geoShader, outline, mouseShader, aa, main, depth, deferredLighting;
			//FrameBuffers
			int textureRenderSlot=2;
			Ref<RenderPass> geoPass, mainPass, shadowPass, aaPass;
			//Scene quad VBO, VAO, EBO
			Ref<VertexArray> screenVao;
			Ref<VertexBuffer> screenVbo;
			Ref<IndexBuffer> screenEbo;
		};

	};

}