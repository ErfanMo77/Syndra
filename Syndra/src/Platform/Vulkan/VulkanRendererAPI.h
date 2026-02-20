#pragma once

#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/UniformBuffer.h"

#include <volk.h>

#include <unordered_map>

namespace Syndra {

	class VulkanContext;
	class VulkanShader;

	class VulkanRendererAPI : public RendererAPI
	{
public:
		~VulkanRendererAPI() override;
		void Init() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;
		void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
		void SetState(RenderState stateID, bool on) override;
		std::string GetRendererInfo() override;

		static void InvalidateAllGraphicsPipelines();
		static void InvalidateShaderPipelines(const VulkanShader* shader);

	private:
		bool EnsureTransientDescriptorPool(
			VulkanContext* context,
			uint32_t requiredSetCount,
			const std::unordered_map<VkDescriptorType, uint32_t>& requiredDescriptorCounts);
		void DestroyTransientDescriptorPool(VkDevice device);

	private:
		glm::vec4 m_ClearColor = { 0.07f, 0.07f, 0.09f, 1.0f };
		uint32_t m_ViewportX = 0;
		uint32_t m_ViewportY = 0;
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;
		bool m_DepthTestEnabled = true;
		bool m_BlendEnabled = false;
		bool m_CullEnabled = false;
		bool m_SRGBEnabled = false;

		Ref<Texture2D> m_FallbackTexture;
		Ref<UniformBuffer> m_FallbackUniformBuffer;
		VkDescriptorPool m_TransientDescriptorPool = VK_NULL_HANDLE;
		uint32_t m_TransientDescriptorPoolMaxSets = 0;
		std::unordered_map<VkDescriptorType, uint32_t> m_TransientDescriptorPoolTypeCaps;
	};

}
