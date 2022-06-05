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
		int width, height, channels;
		
		stbi_uc* data = nullptr;
		if (HDR) {
			LoadHDR();
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			{
				data = stbi_load(path.c_str(), &width, &height, &channels, 0);
			}

			//SN_CORE_ASSERT(data, "Failed to load image!");
			m_Width = width;
			m_Height = height;

			GLenum internalFormat = 0, dataFormat = 0;
			if (channels == 4)
			{
				internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				dataFormat = GL_RGBA;
			}
			else if (channels == 3)
			{
				internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
				dataFormat = GL_RGB;
			}

			uint32_t mipmapLevels;
			m_InternalFormat = internalFormat;
			m_DataFormat = dataFormat;
			mipmapLevels = (GLsizei)floor(log2(std::max(width, height)));
			//SN_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

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
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* mdata = nullptr;
		if (mHeight == 0)
		{
			mdata = stbi_load_from_memory(data, mWidth, &width, &height, &channels, 0);
		}
		else
		{
			mdata = stbi_load_from_memory(data, mWidth * mHeight, &width, &height, &channels, 0);
		}

		//SN_CORE_ASSERT(data, "Failed to load image!");
		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
			dataFormat = GL_RGB;
		}

		uint32_t mipmapLevels;
		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;
		mipmapLevels = (GLsizei)floor(log2(std::max(width, height)));
		//SN_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureStorage2D(m_RendererID, mipmapLevels, internalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, mdata);
		glGenerateTextureMipmap(m_RendererID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		stbi_image_free(mdata);
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
