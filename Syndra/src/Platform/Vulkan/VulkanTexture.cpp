#include "lpch.h"

#include "Platform/Vulkan/VulkanTexture.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanImGuiTextureRegistry.h"
#include "stb_image.h"
#include "vk_mem_alloc.h"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace {

	std::atomic<uint32_t> s_NextTextureRendererId{ 1 };
	std::unordered_map<uint32_t, uint32_t>& GetBoundTextureSlots()
	{
		static std::unordered_map<uint32_t, uint32_t> boundTextureSlots;
		return boundTextureSlots;
	}

	uint32_t AllocateTextureRendererID()
	{
		return s_NextTextureRendererId.fetch_add(1, std::memory_order_relaxed);
	}

	VkFormat PickTextureFormat(bool sRGB)
	{
		return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	}

	uint32_t FormatBytesPerPixel(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 4;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;
		default:
			SN_CORE_ASSERT(false, "Unsupported Vulkan texture format for CPU upload size calculation.");
			return 0;
		}
	}

	uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
	{
		const uint32_t maxDimension = std::max(width, height);
		return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(1u, maxDimension))))) + 1;
	}

	void CmdImageBarrier(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask,
		uint32_t baseMipLevel,
		uint32_t levelCount)
	{
		VkImageMemoryBarrier2 imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarrier.srcStageMask = srcStageMask;
		imageBarrier.srcAccessMask = srcAccessMask;
		imageBarrier.dstStageMask = dstStageMask;
		imageBarrier.dstAccessMask = dstAccessMask;
		imageBarrier.oldLayout = oldLayout;
		imageBarrier.newLayout = newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = image;
		imageBarrier.subresourceRange.aspectMask = aspectMask;
		imageBarrier.subresourceRange.baseMipLevel = baseMipLevel;
		imageBarrier.subresourceRange.levelCount = levelCount;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imageBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	void GenerateMipmaps(
		Syndra::VulkanContext* context,
		VkImage image,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkFilter filter)
	{
		if (mipLevels <= 1)
			return;

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

		int32_t mipWidth = static_cast<int32_t>(width);
		int32_t mipHeight = static_cast<int32_t>(height);

		for (uint32_t mip = 1; mip < mipLevels; ++mip)
		{
			CmdImageBarrier(
				commandBuffer,
				image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT,
				mip - 1,
				1);

			VkImageBlit blit{};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = mip - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = mip;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { std::max(1, mipWidth / 2), std::max(1, mipHeight / 2), 1 };

			vkCmdBlitImage(
				commandBuffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&blit,
				filter);

			CmdImageBarrier(
				commandBuffer,
				image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT,
				VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
				VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
				mip - 1,
				1);

			mipWidth = std::max(1, mipWidth / 2);
			mipHeight = std::max(1, mipHeight / 2);
		}

		CmdImageBarrier(
			commandBuffer,
			image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			mipLevels - 1,
			1);

		context->EndSingleTimeCommands(commandBuffer);
	}

	void TransitionImageLayout(
		Syndra::VulkanContext* context,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t baseMipLevel = 0,
		uint32_t levelCount = 1)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for image transitions.");

		VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 srcAccessMask = 0;
		VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 dstAccessMask = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			srcAccessMask = 0;
			dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			srcAccessMask = 0;
			dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		}
		else
		{
			SN_CORE_ASSERT(false, "Unsupported Vulkan image layout transition.");
		}

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
		CmdImageBarrier(
			commandBuffer,
			image,
			aspectMask,
			oldLayout,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask,
			baseMipLevel,
			levelCount);
		context->EndSingleTimeCommands(commandBuffer);
	}

	void UploadImageData(
		Syndra::VulkanContext* context,
		VkImage image,
		uint32_t width,
		uint32_t height,
		VkDeviceSize dataSize,
		const void* data)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for image upload.");
		SN_CORE_ASSERT(data != nullptr, "Image upload data must be valid.");
		SN_CORE_ASSERT(dataSize > 0, "Image upload size must be greater than zero.");

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingAllocation = nullptr;

		VkBufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.size = dataSize;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo stagingAllocationInfo{};
		stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
		stagingAllocationInfo.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo mappedInfo{};
		const VkResult stagingResult = vmaCreateBuffer(
			context->GetAllocator(),
			&stagingBufferInfo,
			&stagingAllocationInfo,
			&stagingBuffer,
			&stagingAllocation,
			&mappedInfo);
		SN_CORE_ASSERT(stagingResult == VK_SUCCESS, "Failed to create Vulkan staging buffer.");
		SN_CORE_ASSERT(mappedInfo.pMappedData != nullptr, "Staging allocation must be mapped.");
		memcpy(mappedInfo.pMappedData, data, static_cast<size_t>(dataSize));
		vmaFlushAllocation(context->GetAllocator(), stagingAllocation, 0, dataSize);

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);

		context->EndSingleTimeCommands(commandBuffer);

		vmaDestroyBuffer(context->GetAllocator(), stagingBuffer, stagingAllocation);
	}

}

namespace Syndra {

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height), m_RendererID(AllocateTextureRendererID())
	{
		SN_CORE_ASSERT(width > 0 && height > 0, "VulkanTexture2D dimensions must be greater than zero.");
		CreateTextureResources(VK_FORMAT_R8G8B8A8_UNORM, nullptr, 0);
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path, bool sRGB, bool HDR)
		: m_Path(path), m_RendererID(AllocateTextureRendererID())
	{
		if (HDR)
		{
			int width = 0;
			int height = 0;
			int channels = 0;
			float* hdrData = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			if (hdrData == nullptr || width <= 0 || height <= 0)
			{
				SN_CORE_WARN("Failed to load HDR texture '{}'. Creating fallback 1x1 texture.", path);
				m_Width = 1;
				m_Height = 1;
				const float fallbackPixel[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
				CreateTextureResources(VK_FORMAT_R32G32B32A32_SFLOAT, fallbackPixel, sizeof(fallbackPixel));
				return;
			}

			m_Width = static_cast<uint32_t>(width);
			m_Height = static_cast<uint32_t>(height);

			std::vector<float> rgbaPixels(static_cast<size_t>(m_Width) * m_Height * 4, 1.0f);
			for (uint32_t pixelIndex = 0; pixelIndex < m_Width * m_Height; ++pixelIndex)
			{
				const uint32_t srcBase = pixelIndex * static_cast<uint32_t>(channels);
				const uint32_t dstBase = pixelIndex * 4;
				rgbaPixels[dstBase + 0] = hdrData[srcBase + 0];
				rgbaPixels[dstBase + 1] = (channels > 1) ? hdrData[srcBase + 1] : hdrData[srcBase + 0];
				rgbaPixels[dstBase + 2] = (channels > 2) ? hdrData[srcBase + 2] : hdrData[srcBase + 0];
				rgbaPixels[dstBase + 3] = 1.0f;
			}

			stbi_image_free(hdrData);
			CreateTextureResources(
				VK_FORMAT_R32G32B32A32_SFLOAT,
				rgbaPixels.data(),
				static_cast<uint32_t>(rgbaPixels.size() * sizeof(float)));
			return;
		}

		int width = 0;
		int height = 0;
		int channels = 0;
		stbi_set_flip_vertically_on_load(1);
		unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &channels, 0);
		if (imageData == nullptr || width <= 0 || height <= 0)
		{
			SN_CORE_WARN("Failed to load texture '{}'. Creating fallback 1x1 texture.", path);
			m_Width = 1;
			m_Height = 1;
			const uint8_t fallbackPixel[4] = { 255, 0, 255, 255 };
			CreateTextureResources(PickTextureFormat(sRGB), fallbackPixel, sizeof(fallbackPixel));
			return;
		}

		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);

		std::vector<uint8_t> rgbaPixels(static_cast<size_t>(m_Width) * m_Height * 4, 255);
		for (uint32_t pixelIndex = 0; pixelIndex < m_Width * m_Height; ++pixelIndex)
		{
			const uint32_t srcBase = pixelIndex * static_cast<uint32_t>(channels);
			const uint32_t dstBase = pixelIndex * 4;
			rgbaPixels[dstBase + 0] = imageData[srcBase + 0];
			rgbaPixels[dstBase + 1] = (channels > 1) ? imageData[srcBase + 1] : imageData[srcBase + 0];
			rgbaPixels[dstBase + 2] = (channels > 2) ? imageData[srcBase + 2] : imageData[srcBase + 0];
			rgbaPixels[dstBase + 3] = (channels > 3) ? imageData[srcBase + 3] : 255;
		}

		stbi_image_free(imageData);
		CreateTextureResources(
			PickTextureFormat(sRGB),
			rgbaPixels.data(),
			static_cast<uint32_t>(rgbaPixels.size()));
	}

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, const unsigned char* data, bool sRGB)
		: m_Width(width), m_Height(height), m_RendererID(AllocateTextureRendererID())
	{
		SN_CORE_ASSERT(width > 0 && height > 0, "VulkanTexture2D dimensions must be greater than zero.");
		const uint32_t dataSize = (data != nullptr) ? (width * height * 4) : 0;
		CreateTextureResources(PickTextureFormat(sRGB), data, dataSize);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		DestroyTextureResources();
	}

	void VulkanTexture2D::SetData(void* data, uint32_t size)
	{
		if (data == nullptr || size == 0)
			return;

		const uint32_t expectedSize = m_Width * m_Height * FormatBytesPerPixel(m_Format);
		SN_CORE_ASSERT(size == expectedSize, "VulkanTexture2D::SetData requires the full texture data.");

		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for texture upload.");
		const VkFormatProperties formatProperties = [&]()
		{
			VkFormatProperties properties{};
			vkGetPhysicalDeviceFormatProperties(context->GetPhysicalDevice(), m_Format, &properties);
			return properties;
		}();

		const VkFormatFeatureFlags optimalFeatures = formatProperties.optimalTilingFeatures;
		const bool canBlit = (optimalFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) &&
			(optimalFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
		const bool canLinearFilter = (optimalFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
		const VkFilter mipFilter = canLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

		TransitionImageLayout(
			context,
			m_Image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			m_ImageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0,
			m_MipLevels);
		m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		UploadImageData(context, m_Image, m_Width, m_Height, size, data);

		if (m_MipLevels > 1 && canBlit)
		{
			GenerateMipmaps(context, m_Image, m_Width, m_Height, m_MipLevels, mipFilter);
			m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			TransitionImageLayout(
				context,
				m_Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0,
				m_MipLevels);
			m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VulkanImGuiTextureRegistry::UpdateTextureLayout(m_RendererID, m_ImageLayout);
	}

	bool VulkanTexture2D::operator==(const Texture& other) const
	{
		return m_RendererID == other.GetRendererID();
	}

	void VulkanTexture2D::Bind(uint32_t slot) const
	{
		BindTexture(m_RendererID, slot);
	}

	void VulkanTexture2D::BindTexture(uint32_t rendererID, uint32_t slot)
	{
		GetBoundTextureSlots()[slot] = rendererID;
	}

	uint32_t VulkanTexture2D::GetBoundTexture(uint32_t slot)
	{
		auto& boundTextureSlots = GetBoundTextureSlots();
		const auto it = boundTextureSlots.find(slot);
		if (it == boundTextureSlots.end())
			return 0;

		return it->second;
	}

	void VulkanTexture2D::ResetBoundTextures()
	{
		GetBoundTextureSlots().clear();
	}

	void VulkanTexture2D::CreateTextureResources(VkFormat format, const void* initialData, uint32_t dataSize)
	{
		DestroyTextureResources();

		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for texture creation.");

		m_Format = format;
		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_MipLevels = 1;

		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(context->GetPhysicalDevice(), format, &formatProperties);
		const VkFormatFeatureFlags optimalFeatures = formatProperties.optimalTilingFeatures;
		const bool canBlit = (optimalFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) &&
			(optimalFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
		const bool canLinearFilter = (optimalFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
		const VkFilter mipFilter = canLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

		if (initialData != nullptr && dataSize > 0 && canBlit)
			m_MipLevels = CalculateMipLevels(m_Width, m_Height);
		else if (initialData != nullptr && dataSize > 0 && CalculateMipLevels(m_Width, m_Height) > 1)
			SN_CORE_WARN("Texture '{}' uses a single mip level because format {} does not support blit mip generation.",
				m_Path.empty() ? "<generated>" : m_Path,
				static_cast<uint32_t>(format));

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Width;
		imageInfo.extent.height = m_Height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_MipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage =
			VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		const VkResult imageResult = vmaCreateImage(
			context->GetAllocator(),
			&imageInfo,
			&allocationCreateInfo,
			&m_Image,
			&m_Allocation,
			nullptr);
		SN_CORE_ASSERT(imageResult == VK_SUCCESS, "Failed to create Vulkan texture image.");

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = m_MipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		const VkResult viewResult = vkCreateImageView(context->GetDevice(), &viewInfo, nullptr, &m_ImageView);
		SN_CORE_ASSERT(viewResult == VK_SUCCESS, "Failed to create Vulkan texture image view.");

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = canLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerInfo.minFilter = canLinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = canLinearFilter ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(m_MipLevels - 1);
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		const VkResult samplerResult = vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
		SN_CORE_ASSERT(samplerResult == VK_SUCCESS, "Failed to create Vulkan texture sampler.");

		if (initialData != nullptr && dataSize > 0)
		{
			TransitionImageLayout(
				context,
				m_Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				m_ImageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0,
				m_MipLevels);
			m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			UploadImageData(context, m_Image, m_Width, m_Height, dataSize, initialData);
			if (m_MipLevels > 1)
			{
				GenerateMipmaps(context, m_Image, m_Width, m_Height, m_MipLevels, mipFilter);
				m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else
			{
				TransitionImageLayout(
					context,
					m_Image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					0,
					m_MipLevels);
				m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
		}
		else
		{
			TransitionImageLayout(
				context,
				m_Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				m_ImageLayout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				0,
				m_MipLevels);
			m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VulkanImGuiTextureRegistry::RegisterTexture(m_RendererID, m_Sampler, m_ImageView, m_ImageLayout);
	}

	void VulkanTexture2D::DestroyTextureResources()
	{
		VulkanImGuiTextureRegistry::UnregisterTexture(m_RendererID);

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return;

		const bool hasResources =
			(m_Sampler != VK_NULL_HANDLE) ||
			(m_ImageView != VK_NULL_HANDLE) ||
			(m_Image != VK_NULL_HANDLE && m_Allocation != nullptr);
		if (hasResources)
			vkDeviceWaitIdle(context->GetDevice());

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(context->GetDevice(), m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}
		if (m_ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(context->GetDevice(), m_ImageView, nullptr);
			m_ImageView = VK_NULL_HANDLE;
		}
		if (m_Image != VK_NULL_HANDLE && m_Allocation != nullptr)
		{
			vmaDestroyImage(context->GetAllocator(), m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = nullptr;
		}

		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_MipLevels = 1;
	}

	void VulkanTexture2D::TransitionLayout(VkImageLayout newLayout)
	{
		if (newLayout == m_ImageLayout)
			return;

		VulkanContext* context = VulkanContext::GetCurrent();
		TransitionImageLayout(
			context,
			m_Image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			m_ImageLayout,
			newLayout,
			0,
			m_MipLevels);
		m_ImageLayout = newLayout;
		VulkanImGuiTextureRegistry::UpdateTextureLayout(m_RendererID, m_ImageLayout);
	}

	VulkanTexture1D::VulkanTexture1D(uint32_t size)
		: m_Size(size), m_RendererID(AllocateTextureRendererID())
	{
		SN_CORE_ASSERT(size > 0, "VulkanTexture1D size must be greater than zero.");
		CreateTextureResources(nullptr, 0);
	}

	VulkanTexture1D::VulkanTexture1D(uint32_t size, void* data)
		: m_Size(size), m_RendererID(AllocateTextureRendererID())
	{
		SN_CORE_ASSERT(size > 0, "VulkanTexture1D size must be greater than zero.");
		CreateTextureResources(data, size);
	}

	VulkanTexture1D::~VulkanTexture1D()
	{
		DestroyTextureResources();
	}

	void VulkanTexture1D::SetData(void* data, uint32_t size)
	{
		if (data == nullptr || size == 0)
			return;

		SN_CORE_ASSERT(size <= m_Size, "VulkanTexture1D::SetData size exceeds texture length.");

		const VkDeviceSize bytes = static_cast<VkDeviceSize>(size) * sizeof(float) * 2;
		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for texture upload.");

		TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		UploadImageData(context, m_Image, size, 1, bytes, data);
		TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	bool VulkanTexture1D::operator==(const Texture& other) const
	{
		return m_RendererID == other.GetRendererID();
	}

	void VulkanTexture1D::Bind(uint32_t slot) const
	{
		BindTexture(m_RendererID, slot);
	}

	void VulkanTexture1D::BindTexture(uint32_t rendererID, uint32_t slot)
	{
		GetBoundTextureSlots()[slot] = rendererID;
	}

	uint32_t VulkanTexture1D::GetBoundTexture(uint32_t slot)
	{
		return VulkanTexture2D::GetBoundTexture(slot);
	}

	void VulkanTexture1D::CreateTextureResources(const void* initialData, uint32_t dataSize)
	{
		DestroyTextureResources();

		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for texture creation.");

		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_1D;
		imageInfo.extent.width = m_Size;
		imageInfo.extent.height = 1;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		const VkResult imageResult = vmaCreateImage(
			context->GetAllocator(),
			&imageInfo,
			&allocationCreateInfo,
			&m_Image,
			&m_Allocation,
			nullptr);
		SN_CORE_ASSERT(imageResult == VK_SUCCESS, "Failed to create Vulkan 1D texture image.");

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
		viewInfo.format = VK_FORMAT_R32G32_SFLOAT;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		const VkResult viewResult = vkCreateImageView(context->GetDevice(), &viewInfo, nullptr, &m_ImageView);
		SN_CORE_ASSERT(viewResult == VK_SUCCESS, "Failed to create Vulkan 1D texture image view.");

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		const VkResult samplerResult = vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
		SN_CORE_ASSERT(samplerResult == VK_SUCCESS, "Failed to create Vulkan 1D texture sampler.");

		if (initialData != nullptr && dataSize > 0)
		{
			const VkDeviceSize byteSize = static_cast<VkDeviceSize>(dataSize) * sizeof(float) * 2;
			TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			UploadImageData(context, m_Image, dataSize, 1, byteSize, initialData);
			TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else
		{
			TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		VulkanImGuiTextureRegistry::RegisterTexture(m_RendererID, m_Sampler, m_ImageView, m_ImageLayout);
	}

	void VulkanTexture1D::DestroyTextureResources()
	{
		VulkanImGuiTextureRegistry::UnregisterTexture(m_RendererID);

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return;

		const bool hasResources =
			(m_Sampler != VK_NULL_HANDLE) ||
			(m_ImageView != VK_NULL_HANDLE) ||
			(m_Image != VK_NULL_HANDLE && m_Allocation != nullptr);
		if (hasResources)
			vkDeviceWaitIdle(context->GetDevice());

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(context->GetDevice(), m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}
		if (m_ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(context->GetDevice(), m_ImageView, nullptr);
			m_ImageView = VK_NULL_HANDLE;
		}
		if (m_Image != VK_NULL_HANDLE && m_Allocation != nullptr)
		{
			vmaDestroyImage(context->GetAllocator(), m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = nullptr;
		}

		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void VulkanTexture1D::TransitionLayout(VkImageLayout newLayout)
	{
		if (newLayout == m_ImageLayout)
			return;

		VulkanContext* context = VulkanContext::GetCurrent();
		TransitionImageLayout(context, m_Image, VK_IMAGE_ASPECT_COLOR_BIT, m_ImageLayout, newLayout);
		m_ImageLayout = newLayout;
		VulkanImGuiTextureRegistry::UpdateTextureLayout(m_RendererID, m_ImageLayout);
	}

}
