#pragma once

#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/UniformBuffer.h"

#include <volk.h>

#include <limits>
#include <vector>
#include <unordered_map>

namespace Syndra {

	class VulkanContext;
	class VulkanFrameBuffer;
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
		void Flush() override;
		void WaitForIdle() override;
		std::string GetRendererInfo() override;

		static void InvalidateAllGraphicsPipelines();
		static void InvalidateShaderPipelines(const VulkanShader* shader);

	private:
		struct TransientDescriptorPoolState
		{
			VkDescriptorPool Pool = VK_NULL_HANDLE;
			uint32_t MaxSets = 0;
			std::unordered_map<VkDescriptorType, uint32_t> TypeCaps;
			bool NeedsReset = true;
		};

		struct DescriptorBindingEntry
		{
			uint32_t Set = 0;
			uint32_t Binding = 0;
			VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkBuffer Buffer = VK_NULL_HANDLE;
			VkDeviceSize BufferOffset = 0;
			VkDeviceSize BufferRange = 0;
			VkSampler Sampler = VK_NULL_HANDLE;
			VkImageView ImageView = VK_NULL_HANDLE;
			VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			bool operator==(const DescriptorBindingEntry& other) const
			{
				return Set == other.Set &&
					Binding == other.Binding &&
					Type == other.Type &&
					Buffer == other.Buffer &&
					BufferOffset == other.BufferOffset &&
					BufferRange == other.BufferRange &&
					Sampler == other.Sampler &&
					ImageView == other.ImageView &&
					ImageLayout == other.ImageLayout;
			}
		};

		struct DescriptorSetCacheKey
		{
			const VulkanShader* Shader = nullptr;
			VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
			std::vector<DescriptorBindingEntry> Bindings;

			bool operator==(const DescriptorSetCacheKey& other) const
			{
				return Shader == other.Shader &&
					PipelineLayout == other.PipelineLayout &&
					Bindings == other.Bindings;
			}
		};

		struct DescriptorSetCacheKeyHasher
		{
			size_t operator()(const DescriptorSetCacheKey& key) const
			{
				size_t hash = std::hash<const void*>{}(key.Shader);
				hash ^= std::hash<VkPipelineLayout>{}(key.PipelineLayout) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				for (const DescriptorBindingEntry& binding : key.Bindings)
				{
					hash ^= std::hash<uint32_t>{}(binding.Set) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<uint32_t>{}(binding.Binding) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<uint32_t>{}(static_cast<uint32_t>(binding.Type)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<VkBuffer>{}(binding.Buffer) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<uint64_t>{}(binding.BufferOffset) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<uint64_t>{}(binding.BufferRange) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<VkSampler>{}(binding.Sampler) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<VkImageView>{}(binding.ImageView) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					hash ^= std::hash<uint32_t>{}(static_cast<uint32_t>(binding.ImageLayout)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				}

				return hash;
			}
		};

		bool EnsureTransientDescriptorPool(
			VulkanContext* context,
			uint32_t frameIndex,
			uint32_t requiredSetCount,
			const std::unordered_map<VkDescriptorType, uint32_t>& requiredDescriptorCounts,
			bool forceReset);
		void DestroyTransientDescriptorPools(VkDevice device);

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
		std::vector<TransientDescriptorPoolState> m_TransientDescriptorPools;
		std::vector<std::unordered_map<DescriptorSetCacheKey, std::vector<VkDescriptorSet>, DescriptorSetCacheKeyHasher>> m_DescriptorSetCache;
		uint64_t m_LastDescriptorPoolFrameSerial = std::numeric_limits<uint64_t>::max();
		uint64_t m_LastDescriptorCacheFrameSerial = std::numeric_limits<uint64_t>::max();
		VkCommandBuffer m_ActiveRenderingCommandBuffer = VK_NULL_HANDLE;
		VulkanFrameBuffer* m_ActiveRenderingFrameBuffer = nullptr;
	};

}
