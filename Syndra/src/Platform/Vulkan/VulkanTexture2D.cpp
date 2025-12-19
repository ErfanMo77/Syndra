#include "lpch.h"
#include "Platform/Vulkan/VulkanTexture2D.h"

namespace Syndra {

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		m_Pixels.resize(static_cast<size_t>(width) * height * 4);
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path, bool sRGB, bool HDR)
	{
		(void)path;
		(void)sRGB;
		(void)HDR;
		m_Width = 1;
		m_Height = 1;
		m_Pixels.resize(4, 255);
	}

	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, const unsigned char* data, bool sRGB)
		: m_Width(width), m_Height(height)
	{
		(void)sRGB;
		m_Pixels.assign(data, data + (static_cast<size_t>(width) * height * 4));
	}

	VulkanTexture1D::VulkanTexture1D(uint32_t size)
		: m_Size(size)
	{
		m_Data.resize(size);
	}

	VulkanTexture1D::VulkanTexture1D(uint32_t size, void* data)
		: m_Size(size)
	{
		auto* bytes = static_cast<uint8_t*>(data);
		m_Data.assign(bytes, bytes + size);
	}

}
