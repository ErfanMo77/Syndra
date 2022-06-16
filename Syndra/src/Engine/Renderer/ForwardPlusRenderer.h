#pragma once
#include "RenderPipeline.h"
#include "Engine/Utils/Math.h"

namespace Syndra {

	class Entity;
	class Scene;

	class ForwardPlusRenderer : public RenderPipeline {

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
			
		};
	};
}