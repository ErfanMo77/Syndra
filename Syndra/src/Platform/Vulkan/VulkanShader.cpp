#include "lpch.h"

#include "Platform/Vulkan/VulkanShader.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"

#include <glm/gtc/type_ptr.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

#include <cctype>
#include <fstream>
#include <set>

namespace {

	VkShaderStageFlagBits ShaderStageFromTypeString(const std::string& type)
	{
		if (type == "vertex")
			return VK_SHADER_STAGE_VERTEX_BIT;
		if (type == "fragment" || type == "pixel")
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		if (type == "compute")
			return VK_SHADER_STAGE_COMPUTE_BIT;

		SN_CORE_ASSERT(false, "Unknown Vulkan shader stage type '{}'.", type);
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	shaderc_shader_kind VulkanStageToShaderC(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_glsl_vertex_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_glsl_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_glsl_compute_shader;
		default:
			SN_CORE_ASSERT(false, "Unknown Vulkan shader stage.");
			return shaderc_glsl_infer_from_source;
		}
	}

	const char* VulkanStageToCacheExtension(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT: return ".cached_vulkan.vert";
		case VK_SHADER_STAGE_FRAGMENT_BIT: return ".cached_vulkan.frag";
		case VK_SHADER_STAGE_COMPUTE_BIT: return ".cached_vulkan.comp";
		default:
			SN_CORE_ASSERT(false, "Unknown Vulkan shader stage.");
			return "";
		}
	}

	const char* VulkanStageToString(VkShaderStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_SHADER_STAGE_VERTEX_BIT: return "VK_SHADER_STAGE_VERTEX_BIT";
		case VK_SHADER_STAGE_FRAGMENT_BIT: return "VK_SHADER_STAGE_FRAGMENT_BIT";
		case VK_SHADER_STAGE_COMPUTE_BIT: return "VK_SHADER_STAGE_COMPUTE_BIT";
		default:
			return "UNKNOWN_VK_SHADER_STAGE";
		}
	}

	std::filesystem::path GetShaderCacheDirectory()
	{
		return std::filesystem::path("assets/cache/shader/vulkan");
	}

	void CreateShaderCacheDirectoryIfNeeded()
	{
		const std::filesystem::path cacheDirectory = GetShaderCacheDirectory();
		if (!std::filesystem::exists(cacheDirectory))
			std::filesystem::create_directories(cacheDirectory);
	}

	uint64_t MakeBindingKey(uint32_t set, uint32_t binding)
	{
		return (static_cast<uint64_t>(set) << 32) | static_cast<uint64_t>(binding);
	}

	Syndra::VulkanShader::PushConstantMemberType PushConstantTypeFromSpirv(const spirv_cross::SPIRType& type)
	{
		if (type.basetype == spirv_cross::SPIRType::BaseType::Int &&
			type.columns == 1 &&
			type.vecsize == 1)
		{
			return Syndra::VulkanShader::PushConstantMemberType::Int;
		}

		if (type.basetype == spirv_cross::SPIRType::BaseType::Float)
		{
			if (type.columns == 1 && type.vecsize == 1)
				return Syndra::VulkanShader::PushConstantMemberType::Float;
			if (type.columns == 1 && type.vecsize == 3)
				return Syndra::VulkanShader::PushConstantMemberType::Float3;
			if (type.columns == 1 && type.vecsize == 4)
				return Syndra::VulkanShader::PushConstantMemberType::Float4;
			if (type.columns == 4 && type.vecsize == 4)
				return Syndra::VulkanShader::PushConstantMemberType::Mat4;
		}

		return Syndra::VulkanShader::PushConstantMemberType::Unknown;
	}

	void ReflectPushConstantMembers(
		const spirv_cross::Compiler& compiler,
		const spirv_cross::SPIRType& parentType,
		uint32_t parentTypeId,
		uint32_t baseOffset,
		const std::string& prefix,
		std::vector<Syndra::VulkanShader::PushConstantMemberInfo>& outMembers)
	{
		for (uint32_t memberIndex = 0; memberIndex < parentType.member_types.size(); ++memberIndex)
		{
			std::string memberName = compiler.get_member_name(parentTypeId, memberIndex);
			if (memberName.empty())
				memberName = "member" + std::to_string(memberIndex);

			const uint32_t memberOffset = baseOffset + static_cast<uint32_t>(compiler.type_struct_member_offset(parentType, memberIndex));
			const uint32_t memberTypeId = parentType.member_types[memberIndex];
			const auto& memberType = compiler.get_type(memberTypeId);
			const std::string fullName = prefix.empty() ? memberName : prefix + "." + memberName;

			if (memberType.basetype == spirv_cross::SPIRType::BaseType::Struct)
			{
				ReflectPushConstantMembers(
					compiler,
					memberType,
					memberTypeId,
					memberOffset,
					fullName,
					outMembers);
				continue;
			}

			Syndra::VulkanShader::PushConstantMemberInfo reflectedMember{};
			reflectedMember.Name = fullName;
			reflectedMember.Offset = memberOffset;
			reflectedMember.Size = static_cast<uint32_t>(compiler.get_declared_struct_member_size(parentType, memberIndex));
			reflectedMember.Type = PushConstantTypeFromSpirv(memberType);
			outMembers.push_back(reflectedMember);
		}
	}

}

namespace Syndra {

	const VulkanShader* VulkanShader::s_BoundShader = nullptr;

	VulkanShader::VulkanShader(const std::string& filepath)
	{
		LoadFromFile(filepath);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
		: m_Name(name)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;
		shaderSources[VK_SHADER_STAGE_VERTEX_BIT] = vertexSrc;
		shaderSources[VK_SHADER_STAGE_FRAGMENT_BIT] = fragmentSrc;
		LoadFromSources(shaderSources);
	}

	VulkanShader::~VulkanShader()
	{
		DestroyVulkanObjects();
	}

	void VulkanShader::Bind() const
	{
		s_BoundShader = this;
	}

	void VulkanShader::Unbind() const
	{
		if (s_BoundShader == this)
			s_BoundShader = nullptr;
	}

	void VulkanShader::SetInt(const std::string& name, int value)
	{
		SetPushConstantValue(name, &value, sizeof(value), PushConstantMemberType::Int);
	}

	void VulkanShader::SetIntArray(const std::string&, int*, uint32_t)
	{
	}

	void VulkanShader::SetFloat(const std::string& name, float value)
	{
		SetPushConstantValue(name, &value, sizeof(value), PushConstantMemberType::Float);
	}

	void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		SetPushConstantValue(name, glm::value_ptr(value), sizeof(float) * 3, PushConstantMemberType::Float3);
	}

	void VulkanShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{
		SetPushConstantValue(name, glm::value_ptr(value), sizeof(float) * 4, PushConstantMemberType::Float4);
	}

	void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		SetPushConstantValue(name, glm::value_ptr(value), sizeof(float) * 16, PushConstantMemberType::Mat4);
	}

	void VulkanShader::DispatchCompute(uint32_t, uint32_t, uint32_t)
	{
	}

	void VulkanShader::SetMemoryBarrier(MemoryBarrierMode)
	{
	}

	void VulkanShader::Reload()
	{
		if (m_FilePath.empty())
			return;

		LoadFromFile(m_FilePath);
	}

	VkShaderModule VulkanShader::GetShaderModule(VkShaderStageFlagBits stage) const
	{
		const auto it = m_ShaderModules.find(stage);
		if (it == m_ShaderModules.end())
			return VK_NULL_HANDLE;
		return it->second;
	}

	void VulkanShader::LoadFromFile(const std::string& filepath)
	{
		m_FilePath = filepath;

		const size_t lastSlash = filepath.find_last_of("/\\");
		const size_t start = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		const size_t lastDot = filepath.rfind('.');
		const size_t count = lastDot == std::string::npos ? filepath.size() - start : lastDot - start;
		m_Name = filepath.substr(start, count);

		const std::string source = ReadFile(filepath);
		const auto shaderSources = PreProcess(source);
		LoadFromSources(shaderSources);
	}

	void VulkanShader::LoadFromSources(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources)
	{
		VulkanRendererAPI::InvalidateShaderPipelines(this);
		DestroyVulkanObjects();
		CompileOrGetVulkanBinaries(shaderSources);
		Reflect();
		CreateShaderModules();
		BuildDescriptorSetLayoutsAndPipelineLayout();
	}

	std::string VulkanShader::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream input(filepath, std::ios::in | std::ios::binary);
		if (!input)
		{
			SN_CORE_ERROR("Could not open shader file '{}'.", filepath);
			return result;
		}

		input.seekg(0, std::ios::end);
		const std::streamoff size = input.tellg();
		if (size < 0)
		{
			SN_CORE_ERROR("Could not read shader file '{}'.", filepath);
			return result;
		}

		result.resize(static_cast<size_t>(size));
		input.seekg(0, std::ios::beg);
		input.read(result.data(), size);
		return result;
	}

	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		const size_t typeTokenLength = strlen(typeToken);
		size_t position = source.find(typeToken, 0);
		while (position != std::string::npos)
		{
			const size_t endOfLine = source.find_first_of("\r\n", position);
			SN_CORE_ASSERT(endOfLine != std::string::npos, "Invalid shader source format.");

			const size_t begin = position + typeTokenLength + 1;
			const std::string type = source.substr(begin, endOfLine - begin);
			const VkShaderStageFlagBits stage = ShaderStageFromTypeString(type);

			const size_t nextLinePosition = source.find_first_not_of("\r\n", endOfLine);
			SN_CORE_ASSERT(nextLinePosition != std::string::npos, "Invalid shader source format.");

			position = source.find(typeToken, nextLinePosition);
			shaderSources[stage] =
				(position == std::string::npos) ? source.substr(nextLinePosition) : source.substr(nextLinePosition, position - nextLinePosition);
		}

		return shaderSources;
	}

	void VulkanShader::CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources)
	{
		CreateShaderCacheDirectoryIfNeeded();
		const std::filesystem::path cacheDirectory = GetShaderCacheDirectory();

		m_VulkanSPIRV.clear();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
		options.SetTargetSpirv(shaderc_spirv_version_1_6);

		for (const auto& [stage, source] : shaderSources)
		{
			std::string cacheStem = m_Name;
			if (!m_FilePath.empty())
				cacheStem = std::filesystem::path(m_FilePath).filename().string();
			const std::filesystem::path cachedPath = cacheDirectory / (cacheStem + VulkanStageToCacheExtension(stage));

			bool useCachedBinary = false;
			if (!m_FilePath.empty() && std::filesystem::exists(cachedPath))
			{
				const std::filesystem::path sourcePath = m_FilePath;
				if (std::filesystem::exists(sourcePath))
				{
					std::error_code sourceErrorCode{};
					std::error_code cacheErrorCode{};
					const auto sourceTimestamp = std::filesystem::last_write_time(sourcePath, sourceErrorCode);
					const auto cacheTimestamp = std::filesystem::last_write_time(cachedPath, cacheErrorCode);
					if (!sourceErrorCode && !cacheErrorCode && cacheTimestamp >= sourceTimestamp)
						useCachedBinary = true;
				}
			}

			if (useCachedBinary)
			{
				std::ifstream cachedInput(cachedPath, std::ios::in | std::ios::binary);
				if (!cachedInput.is_open())
					useCachedBinary = false;
			}

			if (useCachedBinary)
			{
				std::ifstream cachedInput(cachedPath, std::ios::in | std::ios::binary);
				cachedInput.seekg(0, std::ios::end);
				const std::streamoff size = cachedInput.tellg();
				cachedInput.seekg(0, std::ios::beg);

				auto& spirv = m_VulkanSPIRV[stage];
				spirv.resize(static_cast<size_t>(size) / sizeof(uint32_t));
				cachedInput.read(reinterpret_cast<char*>(spirv.data()), size);
				continue;
			}

			const shaderc::SpvCompilationResult compilationResult =
				compiler.CompileGlslToSpv(source, VulkanStageToShaderC(stage), m_Name.c_str(), options);

			if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				SN_CORE_ERROR("Vulkan shader compile error ({}): {}", VulkanStageToString(stage), compilationResult.GetErrorMessage());
				SN_CORE_ASSERT(false, "Vulkan shader compilation failed.");
			}

			auto& spirv = m_VulkanSPIRV[stage];
			spirv.assign(compilationResult.cbegin(), compilationResult.cend());

			std::ofstream cachedOutput(cachedPath, std::ios::out | std::ios::binary);
			if (cachedOutput.is_open())
			{
				cachedOutput.write(reinterpret_cast<const char*>(spirv.data()), static_cast<std::streamsize>(spirv.size() * sizeof(uint32_t)));
				cachedOutput.flush();
			}
		}
	}

	void VulkanShader::CreateShaderModules()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
		{
			SN_CORE_WARN("VulkanShader '{}' was created without an active VulkanContext. Shader modules were not created.", m_Name);
			return;
		}

		for (const auto& [stage, spirv] : m_VulkanSPIRV)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = spirv.size() * sizeof(uint32_t);
			createInfo.pCode = spirv.data();

			VkShaderModule shaderModule = VK_NULL_HANDLE;
			const VkResult result = vkCreateShaderModule(context->GetDevice(), &createInfo, nullptr, &shaderModule);
			SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan shader module.");
			m_ShaderModules[stage] = shaderModule;
		}
	}

	void VulkanShader::Reflect()
	{
		m_ReflectedBindings.clear();
		m_ReflectedPushConstantRanges.clear();
		m_PushConstants.clear();
		m_Samplers.clear();
		m_PushConstantMembers.clear();
		m_PushConstantMemberLookup.clear();
		m_PushConstantRanges.clear();
		m_PushConstantData.clear();
		m_VertexInputLocations.clear();

		struct PendingBinding
		{
			uint32_t Set = 0;
			uint32_t Binding = 0;
			uint32_t Count = 1;
			VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkShaderStageFlags Stages = 0;
			std::string Name;
		};
		std::unordered_map<uint64_t, PendingBinding> bindingMap;
		std::unordered_map<uint64_t, ReflectedPushConstantRange> pushRangeMap;
		std::unordered_map<std::string, PushConstantMemberInfo> pushMemberMap;

		for (const auto& [stage, spirv] : m_VulkanSPIRV)
		{
			spirv_cross::Compiler compiler(spirv);
			const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			SN_CORE_INFO("VulkanShader::Reflect - {} ({})", m_Name, VulkanStageToString(stage));
			SN_CORE_TRACE("  {} uniform buffers", resources.uniform_buffers.size());
			SN_CORE_TRACE("  {} sampled images", resources.sampled_images.size());
			SN_CORE_TRACE("  {} push constant buffers", resources.push_constant_buffers.size());

			if (stage == VK_SHADER_STAGE_VERTEX_BIT)
			{
				for (const auto& input : resources.stage_inputs)
				{
					const uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
					m_VertexInputLocations.push_back(location);
				}
			}

			for (const auto& resource : resources.uniform_buffers)
			{
				const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
				const uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				const uint64_t key = MakeBindingKey(set, binding);

				auto& pending = bindingMap[key];
				pending.Set = set;
				pending.Binding = binding;
				pending.Count = 1;
				pending.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				pending.Stages |= stage;
				pending.Name = resource.name;
			}

			for (const auto& resource : resources.sampled_images)
			{
				const auto& type = compiler.get_type(resource.type_id);
				const uint32_t descriptorCount = type.array.empty() ? 1 : type.array[0];
				const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
				const uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				const uint64_t key = MakeBindingKey(set, binding);

				auto& pending = bindingMap[key];
				pending.Set = set;
				pending.Binding = binding;
				pending.Count = descriptorCount;
				pending.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				pending.Stages |= stage;
				pending.Name = resource.name;

				m_Samplers.push_back({ resource.name, set, binding, true });
			}

			for (const auto& resource : resources.storage_images)
			{
				const auto& type = compiler.get_type(resource.type_id);
				const uint32_t descriptorCount = type.array.empty() ? 1 : type.array[0];
				const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
				const uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				const uint64_t key = MakeBindingKey(set, binding);

				auto& pending = bindingMap[key];
				pending.Set = set;
				pending.Binding = binding;
				pending.Count = descriptorCount;
				pending.Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				pending.Stages |= stage;
				pending.Name = resource.name;
			}

			for (const auto& resource : resources.storage_buffers)
			{
				const auto& type = compiler.get_type(resource.type_id);
				const uint32_t descriptorCount = type.array.empty() ? 1 : type.array[0];
				const uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
				const uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				const uint64_t key = MakeBindingKey(set, binding);

				auto& pending = bindingMap[key];
				pending.Set = set;
				pending.Binding = binding;
				pending.Count = descriptorCount;
				pending.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				pending.Stages |= stage;
				pending.Name = resource.name;
			}

			for (const auto& resource : resources.push_constant_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				const uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));
				const uint32_t offset = 0;
				const uint64_t key = (static_cast<uint64_t>(offset) << 32) | size;

				auto& range = pushRangeMap[key];
				range.Offset = offset;
				range.Size = size;
				range.StageFlags |= stage;

				std::vector<PCMember> members;
				for (uint32_t memberIndex = 0; memberIndex < bufferType.member_types.size(); ++memberIndex)
				{
					std::string memberName = compiler.get_member_name(resource.base_type_id, memberIndex);
					if (memberName.empty())
						memberName = "member" + std::to_string(memberIndex);

					members.push_back({
						memberName,
						static_cast<size_t>(compiler.get_declared_struct_member_size(bufferType, memberIndex))
						});
				}

				m_PushConstants.push_back({ resource.name, size, members });

				std::vector<PushConstantMemberInfo> reflectedMembers;
				ReflectPushConstantMembers(
					compiler,
					bufferType,
					resource.base_type_id,
					offset,
					"",
					reflectedMembers);

				for (auto& reflectedMember : reflectedMembers)
				{
					reflectedMember.Name = NormalizePushConstantName(reflectedMember.Name);
					if (reflectedMember.Name.empty())
						continue;

					auto it = pushMemberMap.find(reflectedMember.Name);
					if (it == pushMemberMap.end())
					{
						pushMemberMap[reflectedMember.Name] = reflectedMember;
						continue;
					}

					if (reflectedMember.Size > it->second.Size)
						it->second = reflectedMember;
				}
			}
		}

		m_ReflectedBindings.reserve(bindingMap.size());
		for (const auto& [_, binding] : bindingMap)
		{
			m_ReflectedBindings.push_back({
				binding.Set,
				binding.Binding,
				binding.Count,
				binding.Type,
				binding.Stages,
				binding.Name
				});
		}
		std::sort(m_ReflectedBindings.begin(), m_ReflectedBindings.end(), [](const ReflectedBinding& a, const ReflectedBinding& b) {
			if (a.Set != b.Set)
				return a.Set < b.Set;
			return a.Binding < b.Binding;
			});

		m_ReflectedPushConstantRanges.reserve(pushRangeMap.size());
		for (const auto& [_, range] : pushRangeMap)
		{
			m_ReflectedPushConstantRanges.push_back(range);
		}
		std::sort(m_ReflectedPushConstantRanges.begin(), m_ReflectedPushConstantRanges.end(), [](const ReflectedPushConstantRange& a, const ReflectedPushConstantRange& b) {
			if (a.Offset != b.Offset)
				return a.Offset < b.Offset;
			return a.Size < b.Size;
			});

		std::sort(m_Samplers.begin(), m_Samplers.end(), [](const Sampler& a, const Sampler& b) {
			if (a.set != b.set)
				return a.set < b.set;
			return a.binding < b.binding;
			});

		std::sort(m_VertexInputLocations.begin(), m_VertexInputLocations.end());
		m_VertexInputLocations.erase(std::unique(m_VertexInputLocations.begin(), m_VertexInputLocations.end()), m_VertexInputLocations.end());

		for (const auto& [_, reflectedMember] : pushMemberMap)
		{
			m_PushConstantMembers.push_back(reflectedMember);
		}
		std::sort(m_PushConstantMembers.begin(), m_PushConstantMembers.end(), [](const PushConstantMemberInfo& a, const PushConstantMemberInfo& b) {
			if (a.Offset != b.Offset)
				return a.Offset < b.Offset;
			return a.Name < b.Name;
			});

		for (size_t i = 0; i < m_PushConstantMembers.size(); ++i)
			m_PushConstantMemberLookup[m_PushConstantMembers[i].Name] = i;

		InitializePushConstantStorage();
	}

	void VulkanShader::BuildDescriptorSetLayoutsAndPipelineLayout()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
			return;

		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> perSetBindings;
		for (const auto& binding : m_ReflectedBindings)
		{
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding.Binding;
			layoutBinding.descriptorType = binding.Type;
			layoutBinding.descriptorCount = binding.DescriptorCount;
			layoutBinding.stageFlags = binding.StageFlags;
			layoutBinding.pImmutableSamplers = nullptr;
			perSetBindings[binding.Set].push_back(layoutBinding);
		}

		std::vector<uint32_t> setIndices;
		setIndices.reserve(perSetBindings.size());
		for (const auto& [set, _] : perSetBindings)
			setIndices.push_back(set);
		std::sort(setIndices.begin(), setIndices.end());

		const uint32_t maxSet = setIndices.empty() ? 0 : *std::max_element(setIndices.begin(), setIndices.end());
		m_DescriptorSetLayouts.assign(setIndices.empty() ? 0 : (maxSet + 1), VK_NULL_HANDLE);
		for (uint32_t set = 0; set <= maxSet && !setIndices.empty(); ++set)
		{
			auto found = perSetBindings.find(set);
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			if (found != perSetBindings.end())
			{
				layoutBindings = found->second;
				std::sort(layoutBindings.begin(), layoutBindings.end(), [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) {
					return a.binding < b.binding;
					});
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
			layoutInfo.pBindings = layoutBindings.empty() ? nullptr : layoutBindings.data();

			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			const VkResult result = vkCreateDescriptorSetLayout(context->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
			SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan descriptor set layout.");
			m_DescriptorSetLayouts[set] = descriptorSetLayout;
		}

		if (setIndices.empty())
		{
			m_DescriptorSetLayouts.clear();
		}

		std::vector<VkPushConstantRange> pushConstantRanges;
		pushConstantRanges.reserve(m_ReflectedPushConstantRanges.size());
		for (const auto& reflectedRange : m_ReflectedPushConstantRanges)
		{
			VkPushConstantRange range{};
			range.offset = reflectedRange.Offset;
			range.size = reflectedRange.Size;
			range.stageFlags = reflectedRange.StageFlags;
			pushConstantRanges.push_back(range);
		}
		m_PushConstantRanges = pushConstantRanges;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.empty() ? nullptr : m_DescriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_PushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = m_PushConstantRanges.empty() ? nullptr : m_PushConstantRanges.data();

		const VkResult pipelineLayoutResult = vkCreatePipelineLayout(context->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
		SN_CORE_ASSERT(pipelineLayoutResult == VK_SUCCESS, "Failed to create Vulkan pipeline layout.");
	}

	void VulkanShader::InitializePushConstantStorage()
	{
		uint32_t maxSize = 0;
		for (const auto& range : m_ReflectedPushConstantRanges)
			maxSize = std::max(maxSize, range.Offset + range.Size);

		if (maxSize == 0)
		{
			m_PushConstantData.clear();
			return;
		}

		m_PushConstantData.assign(maxSize, 0);
	}

	void VulkanShader::SetPushConstantValue(const std::string& name, const void* data, uint32_t size, PushConstantMemberType valueType)
	{
		if (data == nullptr || size == 0 || m_PushConstantData.empty())
			return;

		const PushConstantMemberInfo* member = FindPushConstantMember(name);
		if (member == nullptr)
			return;

		if (member->Type != PushConstantMemberType::Unknown &&
			valueType != PushConstantMemberType::Unknown &&
			member->Type != valueType)
		{
			return;
		}

		if (member->Offset + size > m_PushConstantData.size())
			return;

		memcpy(m_PushConstantData.data() + member->Offset, data, size);
	}

	const VulkanShader::PushConstantMemberInfo* VulkanShader::FindPushConstantMember(const std::string& name) const
	{
		std::string normalizedName = NormalizePushConstantName(name);
		if (normalizedName.empty())
			return nullptr;

		const auto exactIt = m_PushConstantMemberLookup.find(normalizedName);
		if (exactIt != m_PushConstantMemberLookup.end())
			return &m_PushConstantMembers[exactIt->second];

		size_t separatorPosition = normalizedName.find_last_of('.');
		if (separatorPosition != std::string::npos)
		{
			const std::string leafName = normalizedName.substr(separatorPosition + 1);
			const auto leafIt = m_PushConstantMemberLookup.find(leafName);
			if (leafIt != m_PushConstantMemberLookup.end())
				return &m_PushConstantMembers[leafIt->second];
		}

		return nullptr;
	}

	std::string VulkanShader::NormalizePushConstantName(std::string name)
	{
		auto trimPrefix = [&](const std::string& prefix) {
			if (name.rfind(prefix, 0) == 0)
				name.erase(0, prefix.size());
		};

		trimPrefix("push.");
		trimPrefix("transform.");
		trimPrefix("pc.");

		const std::string materialPrefix = "material.";
		const size_t materialPosition = name.find(materialPrefix);
		if (materialPosition != std::string::npos)
		{
			name.erase(materialPosition, materialPrefix.size());
		}

		while (!name.empty() && name.front() == '.')
			name.erase(name.begin());
		while (!name.empty() && name.back() == '.')
			name.pop_back();

		return name;
	}

	void VulkanShader::DestroyVulkanObjects()
	{
		if (s_BoundShader == this)
			s_BoundShader = nullptr;

		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr)
		{
			m_ShaderModules.clear();
			m_DescriptorSetLayouts.clear();
			m_PipelineLayout = VK_NULL_HANDLE;
			m_PushConstantRanges.clear();
			m_PushConstantMembers.clear();
			m_PushConstantMemberLookup.clear();
			m_PushConstantData.clear();
			return;
		}

		for (auto& [_, shaderModule] : m_ShaderModules)
		{
			if (shaderModule != VK_NULL_HANDLE)
				vkDestroyShaderModule(context->GetDevice(), shaderModule, nullptr);
		}
		m_ShaderModules.clear();

		for (VkDescriptorSetLayout descriptorSetLayout : m_DescriptorSetLayouts)
		{
			if (descriptorSetLayout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(context->GetDevice(), descriptorSetLayout, nullptr);
		}
		m_DescriptorSetLayouts.clear();

		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(context->GetDevice(), m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		m_PushConstantRanges.clear();
		m_PushConstantMembers.clear();
		m_PushConstantMemberLookup.clear();
		m_PushConstantData.clear();
	}

}
