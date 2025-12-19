#include "lpch.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Engine/Renderer/VertexArray.h"

namespace Syndra {

	void VulkanRendererAPI::Init()
	{
		auto* context = VulkanContext::Get();
		SN_CORE_ASSERT(context, "Vulkan context not initialized");

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &props);
		SN_CORE_WARN("Driver: {0}", props.vendorID);
		SN_CORE_WARN("Renderer: {0}", props.deviceName);
		SN_CORE_WARN("API: {0}.{1}", VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion));
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		auto* context = VulkanContext::Get();
		if (!context)
			return;

		context->PrepareFrame();
		VkViewport viewport{};
		viewport.x = static_cast<float>(x);
		viewport.y = static_cast<float>(y);
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { static_cast<int32_t>(x), static_cast<int32_t>(y) };
		scissor.extent = { width, height };

		VkCommandBuffer cmd = context->GetActiveCommandBuffer();
		if (cmd != VK_NULL_HANDLE)
		{
			vkCmdSetViewport(cmd, 0, 1, &viewport);
			vkCmdSetScissor(cmd, 0, 1, &scissor);
		}
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
		auto* context = VulkanContext::Get();
		if (context)
			context->SetClearColor(color);
	}

	void VulkanRendererAPI::Clear()
	{
		auto* context = VulkanContext::Get();
		if (!context)
			return;

		context->PrepareFrame();
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		(void)vertexArray;
		auto* context = VulkanContext::Get();
		if (!context)
			return;

		context->PrepareFrame();
		// Full Vulkan pipeline integration will bind vertex and index buffers here.
		// Dynamic rendering is active from Clear(), so defer rendering until pipeline work is added.
	}

	void VulkanRendererAPI::SetState(RenderState stateID, bool on)
	{
		(void)stateID;
		(void)on;
	}

	std::string VulkanRendererAPI::GetRendererInfo()
	{
		auto* context = VulkanContext::Get();
		if (!context)
			return "Vulkan context unavailable";

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &props);

		std::string info{};
		info += "Vendor ID: " + std::to_string(props.vendorID) + "\n";
		info += "Renderer: " + std::string(props.deviceName) + "\n";
		info += "API Version: " + std::to_string(VK_API_VERSION_MAJOR(props.apiVersion)) + "." + std::to_string(VK_API_VERSION_MINOR(props.apiVersion));
		return info;
	}

}
