#pragma once

#include "Engine/Renderer/Texture.h"

#include <volk.h>

struct VmaAllocation_T;

namespace Syndra {

	using VmaAllocation = VmaAllocation_T*;

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(uint32_t width, uint32_t height);
		VulkanTexture2D(const std::string& path, bool sRGB, bool HDR);
		VulkanTexture2D(uint32_t width, uint32_t height, const unsigned char* data, bool sRGB);
		~VulkanTexture2D() override;

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }
		uint32_t GetRendererID() const override { return m_RendererID; }

		void SetData(void* data, uint32_t size) override;
		bool operator==(const Texture& other) const override;
		void Bind(uint32_t slot = 0) const override;
		std::string GetPath() const override { return m_Path; }

		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkImageLayout GetImageLayout() const { return m_ImageLayout; }

		static void BindTexture(uint32_t rendererID, uint32_t slot);
		static uint32_t GetBoundTexture(uint32_t slot);
		static void ResetBoundTextures();

	private:
		void CreateTextureResources(VkFormat format, const void* initialData, uint32_t dataSize);
		void DestroyTextureResources();
		void TransitionLayout(VkImageLayout newLayout);

	private:
		std::string m_Path;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_MipLevels = 1;
		uint32_t m_RendererID = 0;

		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = nullptr;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	class VulkanTexture1D : public Texture1D
	{
	public:
		explicit VulkanTexture1D(uint32_t size);
		VulkanTexture1D(uint32_t size, void* data);
		~VulkanTexture1D() override;

		uint32_t GetWidth() const override { return m_Size; }
		uint32_t GetHeight() const override { return 1; }
		uint32_t GetRendererID() const override { return m_RendererID; }

		void SetData(void* data, uint32_t size) override;
		bool operator==(const Texture& other) const override;
		void Bind(uint32_t slot = 0) const override;
		std::string GetPath() const override { return {}; }

		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkImageLayout GetImageLayout() const { return m_ImageLayout; }

		static void BindTexture(uint32_t rendererID, uint32_t slot);
		static uint32_t GetBoundTexture(uint32_t slot);

	private:
		void CreateTextureResources(const void* initialData, uint32_t dataSize);
		void DestroyTextureResources();
		void TransitionLayout(VkImageLayout newLayout);

	private:
		uint32_t m_Size = 0;
		uint32_t m_RendererID = 0;

		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = nullptr;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

}
