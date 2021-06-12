#pragma once
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/UniformBuffer.h"
#include "Engine/Renderer/Environment.h"
#include "Engine/Renderer/LightManager.h"
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

		static void Reload(const Ref<Shader>& shader);

		static void OnViewPortResize(uint32_t width, uint32_t height);

		static void OnImGuiUpdate();

		static uint32_t GetTextureID(int index);

		static FramebufferSpecification GetMainFrameSpec();
		static Ref<FrameBuffer> GetGeoFrameBuffer();

		static ShaderLibrary& GetShaderLibrary();


	public:

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 position;
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
			//Environment
			float intensity;
			Ref<Environment> environment;
			//Light
			Ref<LightManager> lightManager;
			float exposure;
			float gamma;
			float lightSize;
			float orthoSize;
			float lightNear;
			float lightFar;
			Ref<UniformBuffer> CameraUniformBuffer, ShadowBuffer;
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
			Ref<Shader> diffuse, geoShader, outline, mouseShader, fxaa, main, depth, deferredLighting, hdrToCubeShader;
			//Render passes
			Ref<RenderPass> geoPass, shadowPass, lightingPass, aaPass;
			//Scene quad VBO
			Ref<VertexArray> screenVao;
		};

	};

}