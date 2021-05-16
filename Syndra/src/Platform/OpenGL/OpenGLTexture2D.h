#pragma once
#include "Engine/Renderer/Texture.h"
#include "glad/glad.h"

namespace Syndra {

	class OpenGLTexture2D : public Texture2D {

	public:
		OpenGLTexture2D(uint32_t width, uint32_t height);
		OpenGLTexture2D(const std::string& path, bool sRGB);
		OpenGLTexture2D(uint32_t mWidth, uint32_t mHeight,const unsigned char* data, bool sRGB);
		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; };
		virtual uint32_t GetHeight() const override { return m_Height; };
		virtual uint32_t GetRendererID() const override { return m_RendererID; };

		virtual void SetData(void* data, uint32_t size) override;
		virtual bool operator ==(const Texture& other) const override;

		virtual std::string GetPath() const override { return m_Path; }

		virtual void Bind(uint32_t slot = 0) const override;

		static void BindTexture(uint32_t rendererID, uint32_t slot);

	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

}
