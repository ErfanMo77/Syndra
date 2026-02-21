#pragma once

#include "RenderPipeline.h"

namespace Syndra {

	class VulkanDeferredRenderer : public RenderPipeline
	{
	public:
		void Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>& env) override;
		void Render() override;
		void End() override;
		void ShutDown() override;
		void UpdateLights() override;
		uint32_t GetFinalTextureID(int index) override;
		uint32_t GetMouseTextureID() override;
		Ref<FrameBuffer> GetMainFrameBuffer() override;
		void OnResize(uint32_t width, uint32_t height) override;
		void OnImGuiRender(bool* rendererOpen, bool* environmentOpen) override;

	public:
		struct RenderData
		{
			struct PointLightData
			{
				glm::vec4 position = glm::vec4(0.0f);
				glm::vec4 color = glm::vec4(0.0f);
			};

			struct DirLightData
			{
				glm::vec4 direction = glm::vec4(0.0f);
				glm::vec4 color = glm::vec4(0.0f);
			};

			struct LightsData
			{
				PointLightData pLight[4];
				DirLightData dLight;
			};

			Ref<Scene> scene;
			ShaderLibrary shaders;
			bool useFxaa = false;
			bool useShadows = true;
			bool useIBL = true;
			bool useFrustumCulling = true;
			uint32_t directionalLightCount = 0;
			uint32_t pointLightCount = 0;
			uint32_t visibleMeshEntityCount = 0;
			uint32_t culledMeshEntityCount = 0;
			uint32_t shadowMapSize = 2048;
			float exposure = 1.0f;
			float gamma = 2.2f;
			float intensity = 1.0f;
			float iblStrength = 1.0f;
			float shadowOrthoSize = 35.0f;
			float shadowNearPlane = 1.0f;
			float shadowFarPlane = 120.0f;
			Ref<Texture2D> environmentMap;

			Ref<Shader> geometryShader;
			Ref<Shader> lightingShader;
			Ref<Shader> shadowShader;
			Ref<Shader> fxaaShader;
			Ref<UniformBuffer> lightsUniformBuffer;
			Ref<UniformBuffer> shadowUniformBuffer;
			LightsData lightsData;
			glm::mat4 shadowViewProjection = glm::mat4(1.0f);

			Ref<RenderPass> shadowPass;
			Ref<RenderPass> geometryPass;
			Ref<RenderPass> lightingPass;
			Ref<RenderPass> aaPass;

			Ref<VertexArray> screenVao;
		};
	};

}
