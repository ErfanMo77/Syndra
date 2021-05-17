#pragma once
#include "Engine/Renderer/FrameBuffer.h"

namespace Syndra {

	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFrameBuffer();

		virtual void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
		
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override {
			SN_CORE_ASSERT(index < m_ColorAttachments.size(),"Framebuffer color attachment index should be less than attachments' size");
			return m_ColorAttachments[index];
		}
		virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }


		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};
}

