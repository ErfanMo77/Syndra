#include "lpch.h"

#include "Platform/Vulkan/VulkanFrameBuffer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanImGuiTextureRegistry.h"
#include "vk_mem_alloc.h"

#include <atomic>

namespace {

	constexpr uint32_t kMaxFramebufferSize = 8192;
	std::atomic<uint32_t> s_NextFramebufferRendererId{ 1000000 };
	bool s_LoggedCubemapFaceUnsupported = false;

	uint32_t AllocateFramebufferRendererID()
	{
		return s_NextFramebufferRendererId.fetch_add(1, std::memory_order_relaxed);
	}

	bool IsDepthFormat(Syndra::FramebufferTextureFormat format)
	{
		switch (format)
		{
		case Syndra::FramebufferTextureFormat::DEPTH24STENCIL8:
		case Syndra::FramebufferTextureFormat::DEPTH32:
			return true;
		default:
			return false;
		}
	}

	bool IsCubemapFormat(Syndra::FramebufferTextureFormat format)
	{
		return format == Syndra::FramebufferTextureFormat::Cubemap;
	}

	VkFormat FramebufferFormatToVulkan(Syndra::FramebufferTextureFormat format)
	{
		switch (format)
		{
		case Syndra::FramebufferTextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case Syndra::FramebufferTextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Syndra::FramebufferTextureFormat::RED_INTEGER: return VK_FORMAT_R32_SINT;
		case Syndra::FramebufferTextureFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Syndra::FramebufferTextureFormat::DEPTH32: return VK_FORMAT_D32_SFLOAT;
		case Syndra::FramebufferTextureFormat::Cubemap: return VK_FORMAT_D32_SFLOAT;
		default:
			SN_CORE_ASSERT(false, "Unsupported framebuffer texture format for Vulkan.");
			return VK_FORMAT_UNDEFINED;
		}
	}

	VkImageAspectFlags AspectMaskFromFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	void CreateImage(
		Syndra::VulkanContext* context,
		uint32_t width,
		uint32_t height,
		uint32_t arrayLayers,
		VkImageCreateFlags imageFlags,
		VkImageType imageType,
		VkFormat format,
		VkImageUsageFlags usage,
		VkImage& outImage,
		Syndra::VmaAllocation& outAllocation)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for framebuffer image allocation.");

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.flags = imageFlags;
		imageInfo.imageType = imageType;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = arrayLayers;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		const VkResult result = vmaCreateImage(
			context->GetAllocator(),
			&imageInfo,
			&allocationInfo,
			&outImage,
			&outAllocation,
			nullptr);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan framebuffer image.");
	}

	VkImageView CreateImageView(
		Syndra::VulkanContext* context,
		VkImage image,
		VkFormat format,
		VkImageViewType viewType,
		VkImageAspectFlags aspectMask,
		uint32_t layerCount)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = viewType;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = layerCount;

		VkImageView imageView = VK_NULL_HANDLE;
		const VkResult result = vkCreateImageView(context->GetDevice(), &viewInfo, nullptr, &imageView);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan framebuffer image view.");
		return imageView;
	}

	void WriteImageBarrier(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask,
		uint32_t layerCount)
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = srcStageMask;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstStageMask = dstStageMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = aspectMask;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	void InitializeColorAttachmentImage(
		Syndra::VulkanContext* context,
		VkImage image,
		VkFormat format,
		const VkClearColorValue& clearColorValue,
		uint32_t layerCount)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required to initialize color attachments.");

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
		WriteImageBarrier(
			commandBuffer,
			image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE,
			0,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			layerCount);

		VkImageSubresourceRange clearRange{};
		clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		clearRange.baseMipLevel = 0;
		clearRange.levelCount = 1;
		clearRange.baseArrayLayer = 0;
		clearRange.layerCount = layerCount;
		vkCmdClearColorImage(
			commandBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&clearColorValue,
			1,
			&clearRange);

		WriteImageBarrier(
			commandBuffer,
			image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			layerCount);
		context->EndSingleTimeCommands(commandBuffer);
	}

	void TransitionDepthImageToReadOnly(
		Syndra::VulkanContext* context,
		VkImage image,
		VkImageAspectFlags aspectMask,
		uint32_t layerCount)
	{
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required to initialize depth attachments.");

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
		WriteImageBarrier(
			commandBuffer,
			image,
			aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE,
			0,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			layerCount);
		context->EndSingleTimeCommands(commandBuffer);
	}

	VkClearColorValue BuildClearColorValue(VkFormat format, const glm::vec4& clearColor, int integerValue)
	{
		VkClearColorValue value{};
		if (format == VK_FORMAT_R32_SINT)
		{
			value.int32[0] = integerValue;
			value.int32[1] = 0;
			value.int32[2] = 0;
			value.int32[3] = 0;
		}
		else
		{
			value.float32[0] = clearColor.r;
			value.float32[1] = clearColor.g;
			value.float32[2] = clearColor.b;
			value.float32[3] = clearColor.a;
		}

		return value;
	}

	void TransitionTrackedImageLayout(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout& currentLayout,
		VkImageLayout newLayout,
		uint32_t layerCount)
	{
		if (currentLayout == newLayout)
			return;

		VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 srcAccessMask = 0;
		switch (currentLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			srcAccessMask = 0;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			srcAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		default:
			srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
			break;
		}

		VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 dstAccessMask = 0;
		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		default:
			dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
			break;
		}

		WriteImageBarrier(
			commandBuffer,
			image,
			aspectMask,
			currentLayout,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask,
			layerCount);
		currentLayout = newLayout;
	}

}

namespace Syndra {

	VulkanFrameBuffer* VulkanFrameBuffer::s_BoundFrameBuffer = nullptr;

	VulkanFrameBuffer::VulkanFrameBuffer(const FramebufferSpecification& spec)
		: m_Specification(spec), m_RendererID(AllocateFramebufferRendererID())
	{
		for (const auto& attachment : m_Specification.Attachments.Attachments)
		{
			if (IsDepthFormat(attachment.TextureFormat))
				m_DepthAttachmentSpecification = attachment;
			else if (IsCubemapFormat(attachment.TextureFormat))
				m_CubemapAttachmentSpecification = attachment;
			else
				m_ColorAttachmentSpecifications.push_back(attachment);
		}

		Invalidate();
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		ReleaseResources();
	}

	void VulkanFrameBuffer::Bind()
	{
		s_BoundFrameBuffer = this;
	}

	void VulkanFrameBuffer::Unbind()
	{
		if (s_BoundFrameBuffer == this)
			s_BoundFrameBuffer = nullptr;
	}

	void VulkanFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > kMaxFramebufferSize || height > kMaxFramebufferSize)
		{
			SN_CORE_ERROR("Attempted to resize Vulkan framebuffer to {0}, {1}", width, height);
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;
		Invalidate();
	}

	int VulkanFrameBuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		if (attachmentIndex >= m_ColorAttachments.size())
			return -1;

		if (x < 0 || y < 0 ||
			x >= static_cast<int>(m_Specification.Width) ||
			y >= static_cast<int>(m_Specification.Height))
		{
			return -1;
		}

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return -1;

		ColorAttachment& attachment = m_ColorAttachments[attachmentIndex];

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingAllocation = nullptr;
		VmaAllocationInfo allocationInfo{};

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(int32_t);
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocationCreateInfo.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		const VkResult stagingResult = vmaCreateBuffer(
			context->GetAllocator(),
			&bufferInfo,
			&allocationCreateInfo,
			&stagingBuffer,
			&stagingAllocation,
			&allocationInfo);
		if (stagingResult != VK_SUCCESS || allocationInfo.pMappedData == nullptr)
			return -1;

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
		TransitionTrackedImageLayout(
			commandBuffer,
			attachment.Image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			attachment.Layout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			1);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { x, y, 0 };
		region.imageExtent = { 1, 1, 1 };

		vkCmdCopyImageToBuffer(
			commandBuffer,
			attachment.Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			stagingBuffer,
			1,
			&region);

		TransitionTrackedImageLayout(
			commandBuffer,
			attachment.Image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			attachment.Layout,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			1);
		VulkanImGuiTextureRegistry::UpdateTextureLayout(attachment.RendererID, attachment.Layout);
		context->EndSingleTimeCommands(commandBuffer);

		vmaInvalidateAllocation(context->GetAllocator(), stagingAllocation, 0, sizeof(int32_t));
		const int32_t pixelData = *reinterpret_cast<const int32_t*>(allocationInfo.pMappedData);
		vmaDestroyBuffer(context->GetAllocator(), stagingBuffer, stagingAllocation);
		return pixelData;
	}

	void VulkanFrameBuffer::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		if (attachmentIndex >= m_ColorAttachments.size())
			return;

		m_ColorAttachmentClearValues[attachmentIndex] = value;

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return;

		ColorAttachment& attachment = m_ColorAttachments[attachmentIndex];
		const VkClearColorValue clearColor = BuildClearColorValue(attachment.Format, m_Specification.ClearColor, value);
		InitializeColorAttachmentImage(context, attachment.Image, attachment.Format, clearColor, 1);
		attachment.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VulkanImGuiTextureRegistry::UpdateTextureLayout(attachment.RendererID, attachment.Layout);
	}

	void VulkanFrameBuffer::Clear(const glm::vec4& clearColor)
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return;

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

		for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			ColorAttachment& attachment = m_ColorAttachments[i];
			TransitionTrackedImageLayout(
				commandBuffer,
				attachment.Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				attachment.Layout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1);

			const int integerClearValue = (i < m_ColorAttachmentClearValues.size()) ? m_ColorAttachmentClearValues[i] : 0;
			const VkClearColorValue clearValue = BuildClearColorValue(attachment.Format, clearColor, integerClearValue);

			VkImageSubresourceRange clearRange{};
			clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			clearRange.baseMipLevel = 0;
			clearRange.levelCount = 1;
			clearRange.baseArrayLayer = 0;
			clearRange.layerCount = 1;
			vkCmdClearColorImage(
				commandBuffer,
				attachment.Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&clearValue,
				1,
				&clearRange);

			TransitionTrackedImageLayout(
				commandBuffer,
				attachment.Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				attachment.Layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				1);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(attachment.RendererID, attachment.Layout);
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
			const VkImageAspectFlags depthAspect = AspectMaskFromFormat(m_DepthFormat);
			TransitionTrackedImageLayout(
				commandBuffer,
				m_DepthImage,
				depthAspect,
				m_DepthImageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1);

			VkImageSubresourceRange depthRange{};
			depthRange.aspectMask = depthAspect;
			depthRange.baseMipLevel = 0;
			depthRange.levelCount = 1;
			depthRange.baseArrayLayer = 0;
			depthRange.layerCount = 1;

			VkClearDepthStencilValue depthClear{};
			depthClear.depth = 1.0f;
			depthClear.stencil = 0;
			vkCmdClearDepthStencilImage(
				commandBuffer,
				m_DepthImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&depthClear,
				1,
				&depthRange);

			TransitionTrackedImageLayout(
				commandBuffer,
				m_DepthImage,
				depthAspect,
				m_DepthImageLayout,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
				1);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(m_DepthAttachmentRendererID, m_DepthImageLayout);
		}

		if (m_CubemapImage != VK_NULL_HANDLE)
		{
			const VkImageAspectFlags cubemapAspect = AspectMaskFromFormat(m_CubemapFormat);
			TransitionTrackedImageLayout(
				commandBuffer,
				m_CubemapImage,
				cubemapAspect,
				m_CubemapImageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				6);

			VkImageSubresourceRange cubemapRange{};
			cubemapRange.aspectMask = cubemapAspect;
			cubemapRange.baseMipLevel = 0;
			cubemapRange.levelCount = 1;
			cubemapRange.baseArrayLayer = 0;
			cubemapRange.layerCount = 6;

			VkClearDepthStencilValue depthClear{};
			depthClear.depth = 1.0f;
			depthClear.stencil = 0;
			vkCmdClearDepthStencilImage(
				commandBuffer,
				m_CubemapImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				&depthClear,
				1,
				&cubemapRange);

			TransitionTrackedImageLayout(
				commandBuffer,
				m_CubemapImage,
				cubemapAspect,
				m_CubemapImageLayout,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
				6);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(m_CubemapAttachmentRendererID, m_CubemapImageLayout);
		}

		context->EndSingleTimeCommands(commandBuffer);
		m_Specification.ClearColor = clearColor;
	}

	void VulkanFrameBuffer::PrepareForRendering(VkCommandBuffer commandBuffer)
	{
		for (auto& attachment : m_ColorAttachments)
		{
			TransitionTrackedImageLayout(
				commandBuffer,
				attachment.Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				attachment.Layout,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				1);
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
			TransitionTrackedImageLayout(
				commandBuffer,
				m_DepthImage,
				AspectMaskFromFormat(m_DepthFormat),
				m_DepthImageLayout,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				1);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(m_DepthAttachmentRendererID, m_DepthImageLayout);
		}
	}

	void VulkanFrameBuffer::FinalizeAfterRendering(VkCommandBuffer commandBuffer)
	{
		for (auto& attachment : m_ColorAttachments)
		{
			TransitionTrackedImageLayout(
				commandBuffer,
				attachment.Image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				attachment.Layout,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				1);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(attachment.RendererID, attachment.Layout);
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
			TransitionTrackedImageLayout(
				commandBuffer,
				m_DepthImage,
				AspectMaskFromFormat(m_DepthFormat),
				m_DepthImageLayout,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
				1);
			VulkanImGuiTextureRegistry::UpdateTextureLayout(m_DepthAttachmentRendererID, m_DepthImageLayout);
		}
	}

	uint32_t VulkanFrameBuffer::GetColorAttachmentRendererID(uint32_t index) const
	{
		if (m_ColorAttachments.empty())
		{
			return m_CubemapAttachmentRendererID;
		}

		SN_CORE_ASSERT(index < m_ColorAttachments.size(), "Vulkan framebuffer color attachment index out of range.");
		return m_ColorAttachments[index].RendererID;
	}

	void VulkanFrameBuffer::BindCubemapFace(uint32_t) const
	{
		if (!s_LoggedCubemapFaceUnsupported)
		{
			SN_CORE_WARN("VulkanFrameBuffer::BindCubemapFace is not required with Vulkan dynamic rendering and is currently a no-op.");
			s_LoggedCubemapFaceUnsupported = true;
		}
	}

	VkImageView VulkanFrameBuffer::GetColorAttachmentImageView(uint32_t index) const
	{
		if (m_ColorAttachments.empty())
		{
			return m_CubemapImageView;
		}

		SN_CORE_ASSERT(index < m_ColorAttachments.size(), "Vulkan framebuffer color attachment index out of range.");
		return m_ColorAttachments[index].View;
	}

	VkFormat VulkanFrameBuffer::GetColorAttachmentFormat(uint32_t index) const
	{
		if (m_ColorAttachments.empty())
		{
			return m_CubemapFormat;
		}

		SN_CORE_ASSERT(index < m_ColorAttachments.size(), "Vulkan framebuffer color attachment index out of range.");
		return m_ColorAttachments[index].Format;
	}

	void VulkanFrameBuffer::Invalidate()
	{
		ReleaseResources();

		VulkanContext* context = VulkanContext::GetCurrent();
		SN_CORE_ASSERT(context != nullptr, "Vulkan context is required for framebuffer creation.");
		SN_CORE_ASSERT(m_Specification.Width > 0 && m_Specification.Height > 0, "Framebuffer dimensions must be valid.");

		if (m_AttachmentSampler == VK_NULL_HANDLE)
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;

			const VkResult samplerResult = vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &m_AttachmentSampler);
			SN_CORE_ASSERT(samplerResult == VK_SUCCESS, "Failed to create Vulkan framebuffer attachment sampler.");
		}

		m_ColorAttachments.reserve(m_ColorAttachmentSpecifications.size());
		m_ColorAttachmentClearValues.clear();
		m_ColorAttachmentClearValues.reserve(m_ColorAttachmentSpecifications.size());
		for (const auto& colorSpec : m_ColorAttachmentSpecifications)
		{
			ColorAttachment attachment;
			attachment.Format = FramebufferFormatToVulkan(colorSpec.TextureFormat);
			attachment.RendererID = AllocateFramebufferRendererID();

			CreateImage(
				context,
				m_Specification.Width,
				m_Specification.Height,
				1,
				0,
				VK_IMAGE_TYPE_2D,
				attachment.Format,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				attachment.Image,
				attachment.Allocation);

			attachment.View = CreateImageView(
				context,
				attachment.Image,
				attachment.Format,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_ASPECT_COLOR_BIT,
				1);

			VulkanImGuiTextureRegistry::RegisterTexture(
				attachment.RendererID,
				m_AttachmentSampler,
				attachment.View,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			const int clearValue = (colorSpec.TextureFormat == FramebufferTextureFormat::RED_INTEGER) ? -1 : 0;
			const VkClearColorValue clearColor = BuildClearColorValue(attachment.Format, m_Specification.ClearColor, clearValue);
			InitializeColorAttachmentImage(context, attachment.Image, attachment.Format, clearColor, 1);
			attachment.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			m_ColorAttachmentClearValues.push_back(clearValue);

			m_ColorAttachments.push_back(attachment);
		}

		m_DepthFormat = VK_FORMAT_UNDEFINED;
		m_DepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_DepthAttachmentRendererID = 0;
		if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
		{
			m_DepthFormat = FramebufferFormatToVulkan(m_DepthAttachmentSpecification.TextureFormat);
			m_DepthAttachmentRendererID = AllocateFramebufferRendererID();

			CreateImage(
				context,
				m_Specification.Width,
				m_Specification.Height,
				1,
				0,
				VK_IMAGE_TYPE_2D,
				m_DepthFormat,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				m_DepthImage,
				m_DepthAllocation);

			m_DepthImageView = CreateImageView(
				context,
				m_DepthImage,
				m_DepthFormat,
				VK_IMAGE_VIEW_TYPE_2D,
				AspectMaskFromFormat(m_DepthFormat),
				1);
			TransitionDepthImageToReadOnly(context, m_DepthImage, AspectMaskFromFormat(m_DepthFormat), 1);
			m_DepthImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			VulkanImGuiTextureRegistry::RegisterTexture(
				m_DepthAttachmentRendererID,
				m_AttachmentSampler,
				m_DepthImageView,
				m_DepthImageLayout);
		}

		m_CubemapFormat = VK_FORMAT_UNDEFINED;
		m_CubemapImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_CubemapAttachmentRendererID = 0;
		if (m_CubemapAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
		{
			m_CubemapFormat = FramebufferFormatToVulkan(m_CubemapAttachmentSpecification.TextureFormat);
			m_CubemapAttachmentRendererID = AllocateFramebufferRendererID();

			CreateImage(
				context,
				m_Specification.Width,
				m_Specification.Height,
				6,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
				VK_IMAGE_TYPE_2D,
				m_CubemapFormat,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				m_CubemapImage,
				m_CubemapAllocation);

			m_CubemapImageView = CreateImageView(
				context,
				m_CubemapImage,
				m_CubemapFormat,
				VK_IMAGE_VIEW_TYPE_CUBE,
				AspectMaskFromFormat(m_CubemapFormat),
				6);
			TransitionDepthImageToReadOnly(context, m_CubemapImage, AspectMaskFromFormat(m_CubemapFormat), 6);
			m_CubemapImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			VulkanImGuiTextureRegistry::RegisterTexture(
				m_CubemapAttachmentRendererID,
				m_AttachmentSampler,
				m_CubemapImageView,
				m_CubemapImageLayout);
		}
	}

	void VulkanFrameBuffer::ReleaseResources()
	{
		if (s_BoundFrameBuffer == this)
			s_BoundFrameBuffer = nullptr;

		for (const auto& attachment : m_ColorAttachments)
		{
			VulkanImGuiTextureRegistry::UnregisterTexture(attachment.RendererID);
		}
		VulkanImGuiTextureRegistry::UnregisterTexture(m_DepthAttachmentRendererID);
		VulkanImGuiTextureRegistry::UnregisterTexture(m_CubemapAttachmentRendererID);

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
		{
			m_ColorAttachments.clear();
			m_ColorAttachmentClearValues.clear();
			m_DepthFormat = VK_FORMAT_UNDEFINED;
			m_DepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			m_CubemapFormat = VK_FORMAT_UNDEFINED;
			m_CubemapImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			m_DepthAttachmentRendererID = 0;
			m_CubemapAttachmentRendererID = 0;
			return;
		}

		bool hasResources = false;
		for (const auto& attachment : m_ColorAttachments)
		{
			hasResources = hasResources ||
				(attachment.View != VK_NULL_HANDLE) ||
				(attachment.Image != VK_NULL_HANDLE && attachment.Allocation != nullptr);
		}
		hasResources = hasResources ||
			(m_DepthImageView != VK_NULL_HANDLE) ||
			(m_DepthImage != VK_NULL_HANDLE && m_DepthAllocation != nullptr) ||
			(m_CubemapImageView != VK_NULL_HANDLE) ||
			(m_CubemapImage != VK_NULL_HANDLE && m_CubemapAllocation != nullptr) ||
			(m_AttachmentSampler != VK_NULL_HANDLE);
		if (hasResources)
			vkDeviceWaitIdle(context->GetDevice());

		for (auto& attachment : m_ColorAttachments)
		{
			if (attachment.View != VK_NULL_HANDLE)
			{
				vkDestroyImageView(context->GetDevice(), attachment.View, nullptr);
				attachment.View = VK_NULL_HANDLE;
			}
			if (attachment.Image != VK_NULL_HANDLE && attachment.Allocation != nullptr)
			{
				vmaDestroyImage(context->GetAllocator(), attachment.Image, attachment.Allocation);
				attachment.Image = VK_NULL_HANDLE;
				attachment.Allocation = nullptr;
			}
		}
		m_ColorAttachments.clear();
		m_ColorAttachmentClearValues.clear();

		if (m_DepthImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(context->GetDevice(), m_DepthImageView, nullptr);
			m_DepthImageView = VK_NULL_HANDLE;
		}
		if (m_DepthImage != VK_NULL_HANDLE && m_DepthAllocation != nullptr)
		{
			vmaDestroyImage(context->GetAllocator(), m_DepthImage, m_DepthAllocation);
			m_DepthImage = VK_NULL_HANDLE;
			m_DepthAllocation = nullptr;
		}
		m_DepthFormat = VK_FORMAT_UNDEFINED;
		m_DepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_DepthAttachmentRendererID = 0;

		if (m_CubemapImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(context->GetDevice(), m_CubemapImageView, nullptr);
			m_CubemapImageView = VK_NULL_HANDLE;
		}
		if (m_CubemapImage != VK_NULL_HANDLE && m_CubemapAllocation != nullptr)
		{
			vmaDestroyImage(context->GetAllocator(), m_CubemapImage, m_CubemapAllocation);
			m_CubemapImage = VK_NULL_HANDLE;
			m_CubemapAllocation = nullptr;
		}
		m_CubemapFormat = VK_FORMAT_UNDEFINED;
		m_CubemapImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_CubemapAttachmentRendererID = 0;

		if (m_AttachmentSampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(context->GetDevice(), m_AttachmentSampler, nullptr);
			m_AttachmentSampler = VK_NULL_HANDLE;
		}
	}

}
