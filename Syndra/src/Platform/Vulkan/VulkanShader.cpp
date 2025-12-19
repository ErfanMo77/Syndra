#include "lpch.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace Syndra {

	VulkanShader::VulkanShader(const std::string& path)
	{
		auto lastSlash = path.find_last_of("/\\");
		auto lastDot = path.rfind('.');
		lastDot = lastDot == std::string::npos ? path.size() : lastDot;
		m_Name = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		(void)vertexSrc;
		(void)fragmentSrc;
		m_Name = name;
	}

}
