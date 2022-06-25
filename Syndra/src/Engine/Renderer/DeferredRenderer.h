#pragma once
#include "RenderPipeline.h"
#include "Engine/Utils/Math.h"

namespace Syndra {

	class DeferredRenderer : public RenderPipeline {
		
	public:
		virtual void Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>& env) override;

		virtual void Render() override;

		virtual void End() override;

		virtual void UpdateLights() override;

		virtual uint32_t GetFinalTextureID(int index) override;

		virtual Ref<FrameBuffer> GetMainFrameBuffer() override;

		virtual void OnImGuiRender(bool* rendererOpen, bool* environmentOpen) override;
		
		virtual void OnResize(uint32_t width, uint32_t height) override;

	public:

		struct ShadowData {
			glm::mat4 lightViewProj;
			glm::mat4 pointLightViewProj[4][6];
		};
		
		struct RenderData
		{
			//Scene object
			Ref<Scene> scene;
			//Environment
			float intensity;
			Ref<Environment> environment;
			//Anti ALiasing
			bool useFxaa = false;

			//Light and shadow
			bool softShadow = false;
			float numPCF = 16;
			float numBlocker = 2;
			float exposure, gamma, lightSize, orthoSize, lightNear, lightFar;
			Ref<LightManager> lightManager;
			Ref<UniformBuffer> ShadowBuffer;
			glm::mat4 lightProj;
			glm::mat4 lightView;
			ShadowData shadowData;
			//Poisson samplers
			Ref<Texture1D> distributionSampler0, distributionSampler1;
			//shaders
			ShaderLibrary shaders;
			Ref<Shader> geoShader, fxaa, depth, deferredLighting;
			//Render passes
			Ref<RenderPass> geoPass, shadowPass, lightingPass, aaPass;
			//Scene quad VBO
			Ref<VertexArray> screenVao;
		};

	};

}