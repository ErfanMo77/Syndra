#pragma once

#include <volk.h>

#include <cstdint>

namespace Syndra {

	struct VulkanImGuiTextureInfo
	{
		VkSampler Sampler = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	class VulkanImGuiTextureRegistry
	{
	public:
		static void RegisterTexture(uint32_t rendererID, VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);
		static void UpdateTextureLayout(uint32_t rendererID, VkImageLayout imageLayout);
		static void UnregisterTexture(uint32_t rendererID);
		static bool ContainsTexture(uint32_t rendererID);
		static bool TryGetTextureInfo(uint32_t rendererID, VulkanImGuiTextureInfo& outInfo);
	};

}
