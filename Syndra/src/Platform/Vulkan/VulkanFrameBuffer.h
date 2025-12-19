#pragma once

#include "Engine/Renderer/FrameBuffer.h"

namespace Syndra {

	class VulkanFrameBuffer : public FrameBuffer
	{
	public:
		explicit VulkanFrameBuffer(const FramebufferSpecification& spec);
		~VulkanFrameBuffer() override = default;

		void Bind() override {}
		void Unbind() override {}

		void Resize(uint32_t width, uint32_t height) override;
		int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
		uint32_t GetRendererID() const override { return 0; }
		void ClearAttachment(uint32_t attachmentIndex, int value) override;
		uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { (void)index; return 0; }
		uint32_t GetDepthAttachmentRendererID() const override { return 0; }
		void BindCubemapFace(uint32_t index) const override { (void)index; }
		const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		FramebufferSpecification m_Specification;
	};

}
