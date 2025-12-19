#include "lpch.h"
#include "Platform/Vulkan/VulkanFrameBuffer.h"

namespace Syndra {

	VulkanFrameBuffer::VulkanFrameBuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{
	}

	void VulkanFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
	}

	int VulkanFrameBuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		(void)attachmentIndex;
		(void)x;
		(void)y;
		return 0;
	}

	void VulkanFrameBuffer::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		(void)attachmentIndex;
		(void)value;
	}

}
