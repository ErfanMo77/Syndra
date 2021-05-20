#pragma once
#include "Engine/Renderer/Texture.h"
#include "glad/glad.h"

namespace Syndra {

	class OpenGLTexture1D : public Texture1D {

	public:
		OpenGLTexture1D(uint32_t size);
		OpenGLTexture1D(uint32_t size, void* data);
		virtual ~OpenGLTexture1D();

		virtual uint32_t GetWidth() const override { return m_Size; };
		virtual uint32_t GetHeight() const override { return m_Size; };
		virtual uint32_t GetRendererID() const override { return m_RendererID; };

		virtual void SetData(void* data, uint32_t size) override;
		virtual bool operator ==(const Texture& other) const override;

		virtual void Bind(uint32_t slot = 0) const override;


		virtual std::string GetPath() const override;

	private:
		uint32_t m_Size;
		uint32_t m_RendererID;
	};

}