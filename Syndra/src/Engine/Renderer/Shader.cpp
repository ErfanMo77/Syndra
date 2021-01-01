#include <lpch.h>
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "glad/glad.h"

namespace Syndra {

	Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(filepath);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}


