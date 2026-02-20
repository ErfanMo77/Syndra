#include "lpch.h"

#include "Platform/Vulkan/VulkanRendererAPI.h"

#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanFrameBuffer.h"
#include "Platform/Vulkan/VulkanImGuiTextureRegistry.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"
#include "Platform/Vulkan/VulkanVertexArray.h"

#include <limits>

namespace Syndra {

	namespace {

		constexpr uint32_t kDescriptorPoolSetSlack = 16;
		constexpr uint32_t kDescriptorPoolTypeSlack = 32;
		constexpr uint32_t kDescriptorPoolMinSets = 64;
		constexpr uint32_t kDescriptorPoolMinTypeCount = 128;

		struct PipelineKey
		{
			const VulkanShader* Shader = nullptr;
			VkShaderModule VertexModule = VK_NULL_HANDLE;
			VkShaderModule FragmentModule = VK_NULL_HANDLE;
			const VulkanFrameBuffer* FrameBuffer = nullptr;
			uint64_t VertexLayoutHash = 0;
			bool DepthTest = false;
			bool Blend = false;
			bool Cull = false;
			bool SRGB = false;

			bool operator==(const PipelineKey& other) const
			{
				return Shader == other.Shader &&
					VertexModule == other.VertexModule &&
					FragmentModule == other.FragmentModule &&
					FrameBuffer == other.FrameBuffer &&
					VertexLayoutHash == other.VertexLayoutHash &&
					DepthTest == other.DepthTest &&
					Blend == other.Blend &&
					Cull == other.Cull &&
					SRGB == other.SRGB;
			}
		};

		struct PipelineKeyHasher
		{
			size_t operator()(const PipelineKey& key) const
			{
				size_t hash = std::hash<const void*>{}(key.Shader);
				hash ^= std::hash<const void*>{}(key.VertexModule) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<const void*>{}(key.FragmentModule) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<const void*>{}(key.FrameBuffer) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<uint64_t>{}(key.VertexLayoutHash) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<bool>{}(key.DepthTest) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<bool>{}(key.Blend) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<bool>{}(key.Cull) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				hash ^= std::hash<bool>{}(key.SRGB) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				return hash;
			}
		};

		std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHasher>& GetPipelineCache()
		{
			static std::unordered_map<PipelineKey, VkPipeline, PipelineKeyHasher> cache;
			return cache;
		}

		void DestroyCachedPipelines(VkDevice device)
		{
			auto& pipelineCache = GetPipelineCache();
			for (auto& [_, pipeline] : pipelineCache)
			{
				if (pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline(device, pipeline, nullptr);
			}

			pipelineCache.clear();
		}

		VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
		{
			switch (type)
			{
			case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
			default:
				return VK_FORMAT_UNDEFINED;
			}
		}

		uint64_t HashVertexLayout(const BufferLayout& layout)
		{
			uint64_t hash = static_cast<uint64_t>(layout.GetStride());
			for (const auto& element : layout.GetElements())
			{
				hash ^= static_cast<uint64_t>(element.Offset + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2));
				hash ^= static_cast<uint64_t>(element.Size + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2));
				hash ^= static_cast<uint64_t>(element.GetComponentCount() + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2));
				hash ^= static_cast<uint64_t>(element.Type) + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
			}
			return hash;
		}

		VkPipeline CreateGraphicsPipeline(
			VkDevice device,
			const VulkanShader* shader,
			const VulkanFrameBuffer* frameBuffer,
			const BufferLayout& layout,
			bool depthTestEnabled,
			bool blendEnabled,
			bool cullEnabled)
		{
			const VkShaderModule vertexModule = shader->GetShaderModule(VK_SHADER_STAGE_VERTEX_BIT);
			const VkShaderModule fragmentModule = shader->GetShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT);
			if (vertexModule == VK_NULL_HANDLE || fragmentModule == VK_NULL_HANDLE)
				return VK_NULL_HANDLE;

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			shaderStages.reserve(2);

			VkPipelineShaderStageCreateInfo vertexStageInfo{};
			vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexStageInfo.module = vertexModule;
			vertexStageInfo.pName = "main";
			shaderStages.push_back(vertexStageInfo);

			VkPipelineShaderStageCreateInfo fragmentStageInfo{};
			fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentStageInfo.module = fragmentModule;
			fragmentStageInfo.pName = "main";
			shaderStages.push_back(fragmentStageInfo);

			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			attributeDescriptions.reserve(layout.GetElements().size() * 4);
			const auto& vertexInputLocations = shader->GetVertexInputLocations();
			const auto consumesLocation = [&](uint32_t shaderLocation) -> bool
			{
				if (vertexInputLocations.empty())
					return true;

				return std::find(vertexInputLocations.begin(), vertexInputLocations.end(), shaderLocation) != vertexInputLocations.end();
			};

			uint32_t location = 0;
			for (const auto& element : layout.GetElements())
			{
				if (element.Type == ShaderDataType::Mat3 || element.Type == ShaderDataType::Mat4)
				{
					const uint32_t columnCount = (element.Type == ShaderDataType::Mat3) ? 3u : 4u;
					const uint32_t columnSize = (element.Type == ShaderDataType::Mat3) ? sizeof(float) * 3 : sizeof(float) * 4;
					const VkFormat columnFormat = (element.Type == ShaderDataType::Mat3) ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
					for (uint32_t column = 0; column < columnCount; ++column)
					{
						const uint32_t shaderLocation = location++;
						if (!consumesLocation(shaderLocation))
							continue;

						VkVertexInputAttributeDescription attribute{};
						attribute.binding = 0;
						attribute.location = shaderLocation;
						attribute.format = columnFormat;
						attribute.offset = static_cast<uint32_t>(element.Offset) + column * columnSize;
						attributeDescriptions.push_back(attribute);
					}
					continue;
				}

				const VkFormat format = ShaderDataTypeToVulkanFormat(element.Type);
				const uint32_t shaderLocation = location++;
				if (format == VK_FORMAT_UNDEFINED)
					continue;
				if (!consumesLocation(shaderLocation))
					continue;

				VkVertexInputAttributeDescription attribute{};
				attribute.binding = 0;
				attribute.location = shaderLocation;
				attribute.format = format;
				attribute.offset = static_cast<uint32_t>(element.Offset);
				attributeDescriptions.push_back(attribute);
			}

			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = layout.GetStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = (layout.GetStride() > 0) ? 1u : 0u;
			vertexInputInfo.pVertexBindingDescriptions = (layout.GetStride() > 0) ? &bindingDescription : nullptr;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.empty() ? nullptr : attributeDescriptions.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer{};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = cullEnabled ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.sampleShadingEnable = VK_FALSE;

			const bool hasDepthAttachment = frameBuffer->HasDepthImage();
			VkPipelineDepthStencilStateCreateInfo depthStencil{};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = (hasDepthAttachment && depthTestEnabled) ? VK_TRUE : VK_FALSE;
			depthStencil.depthWriteEnable = (hasDepthAttachment && depthTestEnabled) ? VK_TRUE : VK_FALSE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			const uint32_t colorAttachmentCount = frameBuffer->GetColorAttachmentCount();
			std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentCount);
			for (uint32_t i = 0; i < colorAttachmentCount; ++i)
			{
				auto& blendAttachment = colorBlendAttachments[i];
				blendAttachment.colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT |
					VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT;

				const VkFormat colorFormat = frameBuffer->GetColorAttachmentFormat(i);
				if (colorFormat == VK_FORMAT_R32_SINT || colorFormat == VK_FORMAT_R32_UINT)
				{
					blendAttachment.blendEnable = VK_FALSE;
					continue;
				}

				blendAttachment.blendEnable = blendEnabled ? VK_TRUE : VK_FALSE;
				blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			}

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
			colorBlending.pAttachments = colorBlendAttachments.empty() ? nullptr : colorBlendAttachments.data();

			const std::array<VkDynamicState, 2> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			std::vector<VkFormat> colorAttachmentFormats;
			colorAttachmentFormats.reserve(colorAttachmentCount);
			for (uint32_t i = 0; i < colorAttachmentCount; ++i)
				colorAttachmentFormats.push_back(frameBuffer->GetColorAttachmentFormat(i));

			VkPipelineRenderingCreateInfo renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
			renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
			renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.empty() ? nullptr : colorAttachmentFormats.data();
			renderingInfo.depthAttachmentFormat = hasDepthAttachment ? frameBuffer->GetDepthAttachmentFormat() : VK_FORMAT_UNDEFINED;
			renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pNext = &renderingInfo;
			pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineInfo.pStages = shaderStages.data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = shader->GetPipelineLayout();
			pipelineInfo.renderPass = VK_NULL_HANDLE;
			pipelineInfo.subpass = 0;

			VkPipeline pipeline = VK_NULL_HANDLE;
			const VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
			if (result != VK_SUCCESS)
				return VK_NULL_HANDLE;

			return pipeline;
		}

	}

	VulkanRendererAPI::~VulkanRendererAPI()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context != nullptr && context->GetDevice() != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(context->GetDevice());
			DestroyCachedPipelines(context->GetDevice());
			DestroyTransientDescriptorPool(context->GetDevice());
		}
		else
		{
			GetPipelineCache().clear();
			DestroyTransientDescriptorPool(VK_NULL_HANDLE);
		}

		m_FallbackUniformBuffer = nullptr;
		m_FallbackTexture = nullptr;
	}

	void VulkanRendererAPI::InvalidateAllGraphicsPipelines()
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		const VkDevice device = (context != nullptr) ? context->GetDevice() : VK_NULL_HANDLE;
		if (device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(device);
			DestroyCachedPipelines(device);
			return;
		}

		GetPipelineCache().clear();
	}

	void VulkanRendererAPI::InvalidateShaderPipelines(const VulkanShader* shader)
	{
		if (shader == nullptr)
			return;

		auto& pipelineCache = GetPipelineCache();
		if (pipelineCache.empty())
			return;

		VulkanContext* context = VulkanContext::GetCurrent();
		const VkDevice device = (context != nullptr) ? context->GetDevice() : VK_NULL_HANDLE;
		if (device != VK_NULL_HANDLE)
			vkDeviceWaitIdle(device);

		size_t removedPipelineCount = 0;
		for (auto iterator = pipelineCache.begin(); iterator != pipelineCache.end();)
		{
			if (iterator->first.Shader != shader)
			{
				++iterator;
				continue;
			}

			if (device != VK_NULL_HANDLE && iterator->second != VK_NULL_HANDLE)
				vkDestroyPipeline(device, iterator->second, nullptr);

			iterator = pipelineCache.erase(iterator);
			++removedPipelineCount;
		}

		if (removedPipelineCount > 0)
		{
			SN_CORE_TRACE(
				"Invalidated {} cached Vulkan graphics pipelines for shader '{}'.",
				removedPipelineCount,
				shader->GetName());
		}
	}

	void VulkanRendererAPI::Init()
	{
		if (VulkanContext* context = VulkanContext::GetCurrent())
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);
			SN_CORE_INFO("Vulkan renderer API initialized on device '{}'.", properties.deviceName);

			if (!m_FallbackTexture)
			{
				const uint8_t fallbackPixel[] = { 255, 255, 255, 255 };
				m_FallbackTexture = Texture2D::Create(1, 1, fallbackPixel);
			}
			if (!m_FallbackUniformBuffer)
			{
				constexpr uint32_t fallbackUniformBinding = std::numeric_limits<uint32_t>::max();
				m_FallbackUniformBuffer = UniformBuffer::Create(256, fallbackUniformBinding);
				std::array<uint8_t, 256> zeroData{};
				m_FallbackUniformBuffer->SetData(zeroData.data(), static_cast<uint32_t>(zeroData.size()));
			}
		}
		else
		{
			SN_CORE_WARN("VulkanRendererAPI initialized without an active VulkanContext.");
		}
	}

	bool VulkanRendererAPI::EnsureTransientDescriptorPool(
		VulkanContext* context,
		uint32_t requiredSetCount,
		const std::unordered_map<VkDescriptorType, uint32_t>& requiredDescriptorCounts)
	{
		if (context == nullptr || context->GetDevice() == VK_NULL_HANDLE)
			return false;

		const uint32_t desiredMaxSets = std::max(kDescriptorPoolMinSets, requiredSetCount * kDescriptorPoolSetSlack);
		std::unordered_map<VkDescriptorType, uint32_t> desiredTypeCounts;
		desiredTypeCounts.reserve(requiredDescriptorCounts.size());
		for (const auto& [descriptorType, requiredCount] : requiredDescriptorCounts)
		{
			const uint32_t desiredCount = std::max(kDescriptorPoolMinTypeCount, requiredCount * kDescriptorPoolTypeSlack);
			desiredTypeCounts[descriptorType] = desiredCount;
		}

		bool recreatePool = (m_TransientDescriptorPool == VK_NULL_HANDLE) || (desiredMaxSets > m_TransientDescriptorPoolMaxSets);
		if (!recreatePool)
		{
			for (const auto& [descriptorType, desiredCount] : desiredTypeCounts)
			{
				const auto capIt = m_TransientDescriptorPoolTypeCaps.find(descriptorType);
				if (capIt == m_TransientDescriptorPoolTypeCaps.end() || capIt->second < desiredCount)
				{
					recreatePool = true;
					break;
				}
			}
		}

		if (recreatePool)
		{
			DestroyTransientDescriptorPool(context->GetDevice());

			std::vector<VkDescriptorPoolSize> poolSizes;
			poolSizes.reserve(desiredTypeCounts.size());
			for (const auto& [descriptorType, descriptorCount] : desiredTypeCounts)
			{
				if (descriptorCount == 0)
					continue;

				VkDescriptorPoolSize poolSize{};
				poolSize.type = descriptorType;
				poolSize.descriptorCount = descriptorCount;
				poolSizes.push_back(poolSize);
			}

			if (poolSizes.empty())
				return false;

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.maxSets = desiredMaxSets;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();

			if (vkCreateDescriptorPool(context->GetDevice(), &poolInfo, nullptr, &m_TransientDescriptorPool) != VK_SUCCESS)
				return false;

			m_TransientDescriptorPoolMaxSets = desiredMaxSets;
			m_TransientDescriptorPoolTypeCaps = std::move(desiredTypeCounts);
			return true;
		}

		return vkResetDescriptorPool(context->GetDevice(), m_TransientDescriptorPool, 0) == VK_SUCCESS;
	}

	void VulkanRendererAPI::DestroyTransientDescriptorPool(VkDevice device)
	{
		if (m_TransientDescriptorPool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device, m_TransientDescriptorPool, nullptr);

		m_TransientDescriptorPool = VK_NULL_HANDLE;
		m_TransientDescriptorPoolMaxSets = 0;
		m_TransientDescriptorPoolTypeCaps.clear();
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		m_ViewportX = x;
		m_ViewportY = y;
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
	}

	void VulkanRendererAPI::Clear()
	{
		VulkanFrameBuffer* boundFramebuffer = VulkanFrameBuffer::GetBoundFrameBuffer();
		if (boundFramebuffer != nullptr)
		{
			boundFramebuffer->Clear(m_ClearColor);
		}
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		VulkanContext* context = VulkanContext::GetCurrent();
		if (context == nullptr || vertexArray == nullptr)
			return;

		auto vkVertexArray = std::dynamic_pointer_cast<VulkanVertexArray>(vertexArray);
		if (!vkVertexArray)
			return;

		const Ref<IndexBuffer>& indexBufferRef = vkVertexArray->GetIndexBuffer();
		if (!indexBufferRef)
			return;

		auto vkIndexBuffer = std::dynamic_pointer_cast<VulkanIndexBuffer>(indexBufferRef);
		if (!vkIndexBuffer || vkIndexBuffer->GetCount() == 0)
			return;

		const VulkanShader* shader = VulkanShader::GetBoundShader();
		if (shader == nullptr)
			return;

		VulkanFrameBuffer* frameBuffer = VulkanFrameBuffer::GetBoundFrameBuffer();
		if (frameBuffer == nullptr)
			return;
		if (frameBuffer->GetColorAttachmentCount() == 0 && !frameBuffer->HasDepthImage())
			return;

		const auto& vertexBuffers = vkVertexArray->GetVertexBuffers();
		if (vertexBuffers.empty())
			return;

		auto vkVertexBuffer = std::dynamic_pointer_cast<VulkanVertexBuffer>(vertexBuffers[0]);
		if (!vkVertexBuffer)
			return;

		const BufferLayout& layout = vkVertexBuffer->GetLayout();
		if (layout.GetStride() == 0)
			return;

		const PipelineKey pipelineKey{
			shader,
			shader->GetShaderModule(VK_SHADER_STAGE_VERTEX_BIT),
			shader->GetShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT),
			frameBuffer,
			HashVertexLayout(layout),
			m_DepthTestEnabled,
			m_BlendEnabled,
			m_CullEnabled,
			m_SRGBEnabled
		};

		VkPipeline pipeline = VK_NULL_HANDLE;
		auto& pipelineCache = GetPipelineCache();
		auto pipelineIt = pipelineCache.find(pipelineKey);
		if (pipelineIt != pipelineCache.end())
		{
			pipeline = pipelineIt->second;
		}
		else
		{
			pipeline = CreateGraphicsPipeline(
				context->GetDevice(),
				shader,
				frameBuffer,
				layout,
				m_DepthTestEnabled,
				m_BlendEnabled,
				m_CullEnabled);
			if (pipeline == VK_NULL_HANDLE)
				return;

			pipelineCache.emplace(pipelineKey, pipeline);
		}

		std::vector<VkDescriptorSet> descriptorSets;
		const auto& descriptorSetLayouts = shader->GetDescriptorSetLayouts();
		const auto& reflectedBindings = shader->GetReflectedBindings();

		if (!descriptorSetLayouts.empty())
		{
			std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts;
			for (const auto& reflectedBinding : reflectedBindings)
				descriptorTypeCounts[reflectedBinding.Type] += reflectedBinding.DescriptorCount;

			if (!EnsureTransientDescriptorPool(
				context,
				static_cast<uint32_t>(descriptorSetLayouts.size()),
				descriptorTypeCounts))
				return;

			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_TransientDescriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
			allocInfo.pSetLayouts = descriptorSetLayouts.data();

			descriptorSets.resize(descriptorSetLayouts.size(), VK_NULL_HANDLE);
			if (vkAllocateDescriptorSets(context->GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
				return;

			size_t uniformBufferBindingCount = 0;
			size_t combinedImageBindingCount = 0;
			for (const auto& reflectedBinding : reflectedBindings)
			{
				if (reflectedBinding.Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
					++uniformBufferBindingCount;
				if (reflectedBinding.Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					++combinedImageBindingCount;
			}

			std::vector<VkDescriptorBufferInfo> bufferInfos;
			std::vector<VkDescriptorImageInfo> imageInfos;
			std::vector<VkWriteDescriptorSet> writes;
			bufferInfos.reserve(uniformBufferBindingCount);
			imageInfos.reserve(combinedImageBindingCount);
			writes.reserve(reflectedBindings.size());

			auto fallbackUniform = std::dynamic_pointer_cast<VulkanUniformBuffer>(m_FallbackUniformBuffer);
			for (const auto& reflectedBinding : reflectedBindings)
			{
				if (reflectedBinding.Set >= descriptorSets.size())
					continue;

				if (reflectedBinding.Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					VulkanUniformBuffer* uniformBuffer = VulkanUniformBuffer::GetUniformBufferForBinding(reflectedBinding.Binding);
					if (uniformBuffer == nullptr && fallbackUniform)
						uniformBuffer = fallbackUniform.get();
					if (uniformBuffer == nullptr)
						continue;

					VkDescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back();
					bufferInfo.buffer = uniformBuffer->GetBuffer();
					bufferInfo.offset = 0;
					bufferInfo.range = uniformBuffer->GetSize();

					VkWriteDescriptorSet& write = writes.emplace_back();
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSets[reflectedBinding.Set];
					write.dstBinding = reflectedBinding.Binding;
					write.dstArrayElement = 0;
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.descriptorCount = 1;
					write.pBufferInfo = &bufferInfo;
					continue;
				}

				if (reflectedBinding.Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					uint32_t rendererID = VulkanTexture2D::GetBoundTexture(reflectedBinding.Binding);
					if (rendererID == 0 && m_FallbackTexture)
						rendererID = m_FallbackTexture->GetRendererID();

					VulkanImGuiTextureInfo textureInfo{};
					if (!VulkanImGuiTextureRegistry::TryGetTextureInfo(rendererID, textureInfo) && m_FallbackTexture)
					{
						const uint32_t fallbackRendererID = m_FallbackTexture->GetRendererID();
						VulkanImGuiTextureRegistry::TryGetTextureInfo(fallbackRendererID, textureInfo);
					}

					if (textureInfo.Sampler == VK_NULL_HANDLE || textureInfo.ImageView == VK_NULL_HANDLE)
						continue;

					VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back();
					imageInfo.sampler = textureInfo.Sampler;
					imageInfo.imageView = textureInfo.ImageView;
					imageInfo.imageLayout = (textureInfo.ImageLayout != VK_IMAGE_LAYOUT_UNDEFINED)
						? textureInfo.ImageLayout
						: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					VkWriteDescriptorSet& write = writes.emplace_back();
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSets[reflectedBinding.Set];
					write.dstBinding = reflectedBinding.Binding;
					write.dstArrayElement = 0;
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.descriptorCount = 1;
					write.pImageInfo = &imageInfo;
				}
			}

			if (!writes.empty())
			{
				vkUpdateDescriptorSets(
					context->GetDevice(),
					static_cast<uint32_t>(writes.size()),
					writes.data(),
					0,
					nullptr);
			}
		}

		const VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();
		frameBuffer->PrepareForRendering(commandBuffer);

		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		colorAttachments.resize(frameBuffer->GetColorAttachmentCount());
		for (uint32_t i = 0; i < frameBuffer->GetColorAttachmentCount(); ++i)
		{
			VkRenderingAttachmentInfo& colorAttachment = colorAttachments[i];
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageView = frameBuffer->GetColorAttachmentImageView(i);
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		}

		VkRenderingAttachmentInfo depthAttachment{};
		VkRenderingAttachmentInfo* depthAttachmentPtr = nullptr;
		if (frameBuffer->HasDepthImage())
		{
			depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachment.imageView = frameBuffer->GetDepthAttachmentImageView();
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachmentPtr = &depthAttachment;
		}

		VkRect2D renderArea{};
		renderArea.offset = { 0, 0 };
		renderArea.extent.width = frameBuffer->GetSpecification().Width;
		renderArea.extent.height = frameBuffer->GetSpecification().Height;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = renderArea;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		renderingInfo.pColorAttachments = colorAttachments.empty() ? nullptr : colorAttachments.data();
		renderingInfo.pDepthAttachment = depthAttachmentPtr;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport{};
		viewport.x = static_cast<float>(m_ViewportX);
		viewport.y = static_cast<float>(m_ViewportY);
		viewport.width = (m_ViewportWidth > 0) ? static_cast<float>(m_ViewportWidth) : static_cast<float>(frameBuffer->GetSpecification().Width);
		viewport.height = (m_ViewportHeight > 0) ? static_cast<float>(m_ViewportHeight) : static_cast<float>(frameBuffer->GetSpecification().Height);
		viewport.width = std::min(viewport.width, static_cast<float>(frameBuffer->GetSpecification().Width));
		viewport.height = std::min(viewport.height, static_cast<float>(frameBuffer->GetSpecification().Height));
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { static_cast<int32_t>(m_ViewportX), static_cast<int32_t>(m_ViewportY) };
		scissor.extent.width = static_cast<uint32_t>(viewport.width);
		scissor.extent.height = static_cast<uint32_t>(viewport.height);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		const std::array<VkBuffer, 1> vertexBuffersVk = { vkVertexBuffer->GetBuffer() };
		const std::array<VkDeviceSize, 1> offsets = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(vertexBuffersVk.size()), vertexBuffersVk.data(), offsets.data());
		vkCmdBindIndexBuffer(commandBuffer, vkIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		if (!descriptorSets.empty())
		{
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				shader->GetPipelineLayout(),
				0,
				static_cast<uint32_t>(descriptorSets.size()),
				descriptorSets.data(),
				0,
				nullptr);
		}

		const auto& pushConstantRanges = shader->GetPushConstantRanges();
		const auto& pushConstantData = shader->GetPushConstantData();
		for (const auto& range : pushConstantRanges)
		{
			if (range.offset >= pushConstantData.size())
				continue;

			const uint32_t availableSize = static_cast<uint32_t>(pushConstantData.size() - range.offset);
			const uint32_t pushSize = std::min(range.size, availableSize);
			if (pushSize == 0)
				continue;

			vkCmdPushConstants(
				commandBuffer,
				shader->GetPipelineLayout(),
				range.stageFlags,
				range.offset,
				pushSize,
				pushConstantData.data() + range.offset);
		}

		vkCmdDrawIndexed(commandBuffer, vkIndexBuffer->GetCount(), 1, 0, 0, 0);
		vkCmdEndRendering(commandBuffer);

		frameBuffer->FinalizeAfterRendering(commandBuffer);
		context->EndSingleTimeCommands(commandBuffer);

	}

	void VulkanRendererAPI::SetState(RenderState state, bool on)
	{
		switch (state)
		{
		case RenderState::DEPTH_TEST:
			m_DepthTestEnabled = on;
			break;
		case RenderState::BLEND:
			m_BlendEnabled = on;
			break;
		case RenderState::CULL:
			m_CullEnabled = on;
			break;
		case RenderState::SRGB:
			m_SRGBEnabled = on;
			break;
		}
	}

	std::string VulkanRendererAPI::GetRendererInfo()
	{
		std::ostringstream info;
		info << "Renderer API: Vulkan";

		if (VulkanContext* context = VulkanContext::GetCurrent())
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);
			info << "\nDevice: " << properties.deviceName;
			info << "\nAPI Version: "
				<< VK_VERSION_MAJOR(properties.apiVersion) << "."
				<< VK_VERSION_MINOR(properties.apiVersion) << "."
				<< VK_VERSION_PATCH(properties.apiVersion);
		}
		else
		{
			info << "\nDevice: unavailable";
		}

		return info.str();
	}

}
