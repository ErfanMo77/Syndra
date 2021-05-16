#include "lpch.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture2D.h"

namespace Syndra {



	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width, height);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool sRGB)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path,sRGB);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Syndra::Texture2D> Texture2D::Create(uint32_t width, uint32_t height,const unsigned char* data, bool sRGB)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width,height,data,sRGB);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void Texture::BindTexture(uint32_t rendererID, uint32_t slot)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
		case RendererAPI::API::OpenGL:	OpenGLTexture2D::BindTexture(rendererID,slot);
		}
	}

}