#include "lpch.h"
#include "Platform/OpenGL/OpenGLTexture2D.h"
#include "stb_image.h"

namespace Syndra {

	//For use in compute shaders
	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		:m_Width(width), m_Height(height)
	{
		glGenTextures(1, &m_RendererID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindImageTexture(0, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, bool sRGB, bool HDR)
		: m_Path(path)
	{
		if (HDR) {
			LoadHDR();
		}
		else
		{
			int width = 0;
			int height = 0;
			int channels = 0;
			stbi_set_flip_vertically_on_load(1);
			stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

			const GLenum internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
			const GLenum dataFormat = GL_RGBA;
			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;

			if (!data || width <= 0 || height <= 0)
			{
				SN_CORE_ERROR("Failed to load texture '{}'. Using 1x1 fallback texture.", path);
				const unsigned char fallbackPixel[4] = { 255, 0, 255, 255 };
				m_Width = 1;
				m_Height = 1;

				glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
				glBindTexture(GL_TEXTURE_2D, m_RendererID);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);
				glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, fallbackPixel);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
				if (data)
					stbi_image_free(data);
				return;
			}

			m_Width = width;
			m_Height = height;
			const uint32_t mipmapLevels = static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(1, std::max(width, height)))))) + 1;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glBindTexture(GL_TEXTURE_2D, m_RendererID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTextureStorage2D(m_RendererID, mipmapLevels, internalFormat, m_Width, m_Height);

			glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
			glGenerateTextureMipmap(m_RendererID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			stbi_image_free(data);
		}
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t mWidth, uint32_t mHeight,const unsigned char* data, bool sRGB)
	{
		SN_CORE_ASSERT(mWidth > 0 && mHeight > 0, "OpenGLTexture2D dimensions must be greater than zero.");
		m_Width = static_cast<int>(mWidth);
		m_Height = static_cast<int>(mHeight);

		const GLenum internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		const GLenum dataFormat = GL_RGBA;
		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;
		const uint32_t mipmapLevels = static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(1u, std::max(mWidth, mHeight)))))) + 1;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureStorage2D(m_RendererID, mipmapLevels, internalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateTextureMipmap(m_RendererID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
		SN_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::Bind(uint32_t slot /*= 0*/) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	void OpenGLTexture2D::BindTexture(uint32_t rendererID, uint32_t slot)
	{
		//glActiveTexture(GL_TEXTURE0 + slot);
		glBindTextureUnit(slot, rendererID);
	}

	void OpenGLTexture2D::LoadHDR()
	{
		int width, height, channels;
		float* data = stbi_loadf(m_Path.c_str(), &width, &height, &channels, 0);
		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);
	}

	bool OpenGLTexture2D::operator==(const Texture& other) const
	{
		return this->m_RendererID == ((OpenGLTexture2D&)other).GetRendererID();
	}

}
