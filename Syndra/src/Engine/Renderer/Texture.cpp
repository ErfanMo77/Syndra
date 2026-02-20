#include "lpch.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Utils/AssetPath.h"
#include "Platform/OpenGL/OpenGLTexture2D.h"
#include "Platform/OpenGL/OpenGLTexture1D.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace Syndra {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture2D>(width, height);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width, height);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool sRGB)
	{
		const std::string resolvedPath = AssetPath::ResolveTexturePath(path);
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture2D>(resolvedPath, sRGB, false);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(resolvedPath, sRGB, false);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Syndra::Texture2D> Texture2D::Create(uint32_t width, uint32_t height,const unsigned char* data, bool sRGB)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture2D>(width, height, data, sRGB);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width,height,data,sRGB);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::CreateHDR(const std::string& path, bool sRGB, bool HDR)
	{
		const std::string resolvedPath = AssetPath::ResolveTexturePath(path);
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture2D>(resolvedPath, sRGB, HDR);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(resolvedPath, sRGB, HDR);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	void Texture::BindTexture(uint32_t rendererID, uint32_t slot)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:
			SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return;
		case RendererAPI::API::Vulkan:
			VulkanTexture2D::BindTexture(rendererID, slot);
			return;
		case RendererAPI::API::OpenGL:
			OpenGLTexture2D::BindTexture(rendererID,slot);
			return;
		}
	}

	Ref<Texture1D> Texture1D::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture1D>(size);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture1D>(size);
		}
		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Syndra::Texture1D> Texture1D::Create(uint32_t size, void* data)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::Vulkan: return CreateRef<VulkanTexture1D>(size, data);
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture1D>(size,data);
		}
		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
