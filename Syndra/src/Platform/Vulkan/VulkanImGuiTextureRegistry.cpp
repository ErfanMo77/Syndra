#include "lpch.h"

#include "Platform/Vulkan/VulkanImGuiTextureRegistry.h"

#include <unordered_map>

namespace {

	using RegistryMap = std::unordered_map<uint32_t, Syndra::VulkanImGuiTextureInfo>;

	RegistryMap& GetRegistry()
	{
		static RegistryMap registry;
		return registry;
	}

}

namespace Syndra {

	void VulkanImGuiTextureRegistry::RegisterTexture(uint32_t rendererID, VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
	{
		if (rendererID == 0 || sampler == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE)
			return;

		GetRegistry()[rendererID] = VulkanImGuiTextureInfo{ sampler, imageView, imageLayout };
	}

	void VulkanImGuiTextureRegistry::UpdateTextureLayout(uint32_t rendererID, VkImageLayout imageLayout)
	{
		if (rendererID == 0)
			return;

		auto& registry = GetRegistry();
		auto iterator = registry.find(rendererID);
		if (iterator == registry.end())
			return;

		iterator->second.ImageLayout = imageLayout;
	}

	void VulkanImGuiTextureRegistry::UnregisterTexture(uint32_t rendererID)
	{
		if (rendererID == 0)
			return;

		GetRegistry().erase(rendererID);
	}

	bool VulkanImGuiTextureRegistry::ContainsTexture(uint32_t rendererID)
	{
		if (rendererID == 0)
			return false;

		return GetRegistry().find(rendererID) != GetRegistry().end();
	}

	bool VulkanImGuiTextureRegistry::TryGetTextureInfo(uint32_t rendererID, VulkanImGuiTextureInfo& outInfo)
	{
		auto& registry = GetRegistry();
		auto iterator = registry.find(rendererID);
		if (iterator == registry.end())
			return false;

		outInfo = iterator->second;
		return true;
	}

}
