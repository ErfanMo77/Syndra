#pragma once

#include "Engine/Renderer/Texture.h"

namespace Syndra {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(uint32_t width, uint32_t height);
		VulkanTexture2D(const std::string& path, bool sRGB, bool HDR);
		VulkanTexture2D(uint32_t width, uint32_t height, const unsigned char* data, bool sRGB);
		~VulkanTexture2D() override = default;

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }

		void SetData(void* data, uint32_t size) override { (void)data; (void)size; }
		void Bind(uint32_t slot = 0) const override { (void)slot; }

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		std::vector<uint8_t> m_Pixels;
	};

	class VulkanTexture1D : public Texture1D
	{
	public:
		VulkanTexture1D(uint32_t size);
		VulkanTexture1D(uint32_t size, void* data);
		~VulkanTexture1D() override = default;

		uint32_t GetSize() override { return m_Size; }

		void SetData(void* data, uint32_t size) override { (void)data; (void)size; }
		void Bind(uint32_t slot = 0) const override { (void)slot; }
	private:
		uint32_t m_Size;
		std::vector<uint8_t> m_Data;
	};

}
