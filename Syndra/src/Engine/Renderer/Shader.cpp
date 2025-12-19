#include <lpch.h>
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace Syndra {

	Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:    SN_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(filepath);
		case RendererAPI::API::Vulkan:  return CreateRef<VulkanShader>(filepath);
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
		case RendererAPI::API::Vulkan:  return CreateRef<VulkanShader>(name, vertexSrc, fragmentSrc);
		}

		SN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	//==================================Shader Library====================================\\

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		SN_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		SN_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	void ShaderLibrary::DeleteShader(const Ref<Shader>& shader)
	{
		m_Shaders.erase(shader->GetName());
	}

	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

}

