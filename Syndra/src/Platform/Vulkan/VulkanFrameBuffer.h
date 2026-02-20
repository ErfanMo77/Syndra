#pragma once

#include "Engine/Renderer/FrameBuffer.h"

#include <volk.h>

struct VmaAllocation_T;

namespace Syndra {

	using VmaAllocation = VmaAllocation_T*;

	class VulkanFrameBuffer : public FrameBuffer
	{
	public:
		explicit VulkanFrameBuffer(const FramebufferSpecification& spec);
		~VulkanFrameBuffer() override;

		void Bind() override;
		void Unbind() override;

		void Resize(uint32_t width, uint32_t height) override;
		int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
		uint32_t GetRendererID() const override { return m_RendererID; }
		void ClearAttachment(uint32_t attachmentIndex, int value) override;
		void Clear(const glm::vec4& clearColor);

		uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override;
		uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachmentRendererID; }

		void BindCubemapFace(uint32_t index) const override;
		const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

		VkImageView GetColorAttachmentImageView(uint32_t index = 0) const;
		VkImageView GetDepthAttachmentImageView() const { return m_DepthImageView; }
		VkImageView GetCubemapImageView() const { return m_CubemapImageView; }
		VkFormat GetColorAttachmentFormat(uint32_t index = 0) const;
		VkFormat GetDepthAttachmentFormat() const { return m_DepthFormat; }
		uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(m_ColorAttachments.size()); }
		bool HasDepthImage() const { return m_DepthImage != VK_NULL_HANDLE; }
		void PrepareForRendering(VkCommandBuffer commandBuffer);
		void FinalizeAfterRendering(VkCommandBuffer commandBuffer);
		static VulkanFrameBuffer* GetBoundFrameBuffer() { return s_BoundFrameBuffer; }

	private:
		void Invalidate();
		void ReleaseResources();

		bool HasDepthAttachment() const { return m_DepthFormat != VK_FORMAT_UNDEFINED; }
		bool HasCubemapAttachment() const { return m_CubemapFormat != VK_FORMAT_UNDEFINED; }

	private:
		struct ColorAttachment
		{
			VkImage Image = VK_NULL_HANDLE;
			VmaAllocation Allocation = nullptr;
			VkImageView View = VK_NULL_HANDLE;
			VkFormat Format = VK_FORMAT_UNDEFINED;
			VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
			uint32_t RendererID = 0;
		};

		FramebufferSpecification m_Specification;
		uint32_t m_RendererID = 0;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;
		FramebufferTextureSpecification m_CubemapAttachmentSpecification = FramebufferTextureFormat::None;

		std::vector<ColorAttachment> m_ColorAttachments;
		std::vector<int> m_ColorAttachmentClearValues;

		VkImage m_DepthImage = VK_NULL_HANDLE;
		VmaAllocation m_DepthAllocation = nullptr;
		VkImageView m_DepthImageView = VK_NULL_HANDLE;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
		VkImageLayout m_DepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32_t m_DepthAttachmentRendererID = 0;

		VkImage m_CubemapImage = VK_NULL_HANDLE;
		VmaAllocation m_CubemapAllocation = nullptr;
		VkImageView m_CubemapImageView = VK_NULL_HANDLE;
		VkFormat m_CubemapFormat = VK_FORMAT_UNDEFINED;
		VkImageLayout m_CubemapImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32_t m_CubemapAttachmentRendererID = 0;

		VkSampler m_AttachmentSampler = VK_NULL_HANDLE;
		static VulkanFrameBuffer* s_BoundFrameBuffer;
	};

}
