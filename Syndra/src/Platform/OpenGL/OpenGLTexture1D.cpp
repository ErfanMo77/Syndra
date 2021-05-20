#include "lpch.h"
#include "Platform/OpenGL/OpenGLTexture1D.h"

namespace Syndra {

	OpenGLTexture1D::OpenGLTexture1D(uint32_t size)
		:m_Size(size)
	{
		glCreateTextures(GL_TEXTURE_1D, 1, &m_RendererID);
		glBindTexture(GL_TEXTURE_1D, m_RendererID);
		glTextureStorage1D(m_RendererID, 1, GL_RG32F, size);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	OpenGLTexture1D::OpenGLTexture1D(uint32_t size, void* data)
		:m_Size(size)
	{
		glCreateTextures(GL_TEXTURE_1D, 1, &m_RendererID);
		glBindTexture(GL_TEXTURE_1D, m_RendererID);
		glTextureStorage1D(m_RendererID, 1, GL_RG32F, size);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, size, GL_RG, GL_FLOAT, data);
	}

	OpenGLTexture1D::~OpenGLTexture1D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture1D::SetData(void* data, uint32_t size)
	{
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, size, GL_RG, GL_FLOAT, data);
	}

	void OpenGLTexture1D::Bind(uint32_t slot /*= 0*/) const
	{
		glBindTextureUnit(slot, m_RendererID);
	}

	std::string OpenGLTexture1D::GetPath() const
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool OpenGLTexture1D::operator==(const Texture& other) const
	{
		return this->m_RendererID == ((OpenGLTexture1D&)other).GetRendererID();
	}

}