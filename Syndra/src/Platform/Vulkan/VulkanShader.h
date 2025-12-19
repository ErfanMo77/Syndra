#pragma once

#include "Engine/Renderer/Shader.h"

namespace Syndra {

	class VulkanShader : public Shader
	{
	public:
		explicit VulkanShader(const std::string& path);
		VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		~VulkanShader() override = default;

		void Bind() const override {}
		void UnBind() const override {}

		void SetInt(const std::string& name, int value) override { (void)name; (void)value; }
		void SetFloat(const std::string& name, float value) override { (void)name; (void)value; }
		void SetFloat3(const std::string& name, const glm::vec3& value) override { (void)name; (void)value; }
		void SetFloat4(const std::string& name, const glm::vec4& value) override { (void)name; (void)value; }
		void SetMat4(const std::string& name, const glm::mat4& value) override { (void)name; (void)value; }

		const std::string& GetName() const override { return m_Name; }
	private:
		std::string m_Name;
	};

}
