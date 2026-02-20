#pragma once

#include "Engine/Renderer/Shader.h"

#include <volk.h>

namespace Syndra {

	class VulkanShader : public Shader
	{
	public:
		enum class PushConstantMemberType
		{
			Unknown = 0,
			Int,
			Float,
			Float3,
			Float4,
			Mat4
		};

		struct ReflectedBinding
		{
			uint32_t Set = 0;
			uint32_t Binding = 0;
			uint32_t DescriptorCount = 1;
			VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkShaderStageFlags StageFlags = 0;
			std::string Name;
		};

		struct PushConstantMemberInfo
		{
			std::string Name;
			uint32_t Offset = 0;
			uint32_t Size = 0;
			PushConstantMemberType Type = PushConstantMemberType::Unknown;
		};

		explicit VulkanShader(const std::string& filepath);
		VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		~VulkanShader() override;

		void Bind() const override;
		void Unbind() const override;

		void SetInt(const std::string& name, int value) override;
		void SetIntArray(const std::string& name, int* values, uint32_t count) override;
		void SetFloat(const std::string& name, float value) override;
		void SetFloat3(const std::string& name, const glm::vec3& value) override;
		void SetFloat4(const std::string& name, const glm::vec4& value) override;
		void SetMat4(const std::string& name, const glm::mat4& value) override;

		void DispatchCompute(uint32_t x, uint32_t y, uint32_t z) override;
		void SetMemoryBarrier(MemoryBarrierMode mode) override;

		std::vector<PushConstant> GetPushConstants() override { return m_PushConstants; }
		std::vector<Sampler> GetSamplers() override { return m_Samplers; }

		const std::string& GetName() const override { return m_Name; }
		void Reload() override;

		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		const std::vector<ReflectedBinding>& GetReflectedBindings() const { return m_ReflectedBindings; }
		const std::vector<VkPushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }
		const std::vector<uint8_t>& GetPushConstantData() const { return m_PushConstantData; }
		const std::vector<uint32_t>& GetVertexInputLocations() const { return m_VertexInputLocations; }
		VkShaderModule GetShaderModule(VkShaderStageFlagBits stage) const;
		static const VulkanShader* GetBoundShader() { return s_BoundShader; }

	private:
		struct ReflectedPushConstantRange
		{
			uint32_t Offset = 0;
			uint32_t Size = 0;
			VkShaderStageFlags StageFlags = 0;
		};

		void LoadFromFile(const std::string& filepath);
		void LoadFromSources(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources);
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);

		void CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources);
		void CreateShaderModules();
		void Reflect();
		void BuildDescriptorSetLayoutsAndPipelineLayout();
		void DestroyVulkanObjects();
		void InitializePushConstantStorage();
		void SetPushConstantValue(const std::string& name, const void* data, uint32_t size, PushConstantMemberType valueType);
		const PushConstantMemberInfo* FindPushConstantMember(const std::string& name) const;
		static std::string NormalizePushConstantName(std::string name);

	private:
		std::string m_FilePath;
		std::string m_Name;

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_VulkanSPIRV;
		std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_ShaderModules;
		std::vector<ReflectedBinding> m_ReflectedBindings;
		std::vector<ReflectedPushConstantRange> m_ReflectedPushConstantRanges;
		std::vector<VkPushConstantRange> m_PushConstantRanges;
		std::vector<PushConstantMemberInfo> m_PushConstantMembers;
		std::unordered_map<std::string, size_t> m_PushConstantMemberLookup;
		std::vector<uint8_t> m_PushConstantData;
		std::vector<uint32_t> m_VertexInputLocations;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<PushConstant> m_PushConstants;
		std::vector<Sampler> m_Samplers;

		static const VulkanShader* s_BoundShader;
	};

}
