#include "lpch.h"
#include "Platform/Vulkan/VulkanContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Syndra {

	static std::vector<const char*> GetRequiredInstanceExtensions()
	{
		uint32_t count = 0;
		const char** extensions = glfwGetRequiredInstanceExtensions(&count);
		return std::vector<const char*>(extensions, extensions + count);
	}

	VulkanContext* VulkanContext::s_Instance = nullptr;

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		s_Instance = this;
	}

	VulkanContext::~VulkanContext()
	{
		if (m_Device != VK_NULL_HANDLE)
			vkDeviceWaitIdle(m_Device);

		for (auto fence : m_InFlightFences)
			vkDestroyFence(m_Device, fence, nullptr);
		for (auto semaphore : m_RenderFinishedSemaphores)
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		for (auto semaphore : m_ImageAvailableSemaphores)
			vkDestroySemaphore(m_Device, semaphore, nullptr);

		if (m_CommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (auto view : m_SwapchainImageViews)
			vkDestroyImageView(m_Device, view, nullptr);
		if (m_Swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		if (m_Device != VK_NULL_HANDLE)
			vkDestroyDevice(m_Device, nullptr);
		if (m_Surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		if (m_Instance != VK_NULL_HANDLE)
			vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::Init()
	{
		CreateInstance();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain();
		CreateImageViews();
		CreateCommandPool();
		AllocateCommandBuffers();
		CreateSyncObjects();
	}

	void VulkanContext::SwapBuffers()
	{
		if (m_ActiveCommandBuffer == VK_NULL_HANDLE)
			BeginFrame();
		EndFrame();
	}

	void VulkanContext::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
	}

	VulkanContext* VulkanContext::Get()
	{
		return s_Instance;
	}

	void VulkanContext::PrepareFrame()
	{
		if (m_ActiveCommandBuffer == VK_NULL_HANDLE)
			BeginFrame();
	}

	void VulkanContext::CreateInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Syndra";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		auto extensions = GetRequiredInstanceExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan instance");
	}

	void VulkanContext::CreateSurface()
	{
		VkResult result = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan surface");
	}

	uint32_t VulkanContext::FindGraphicsQueueFamily(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				return i;
		}

		return 0;
	}

	uint32_t VulkanContext::FindPresentQueueFamily(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (presentSupport)
				return i;
		}

		return 0;
	}

	void VulkanContext::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		SN_CORE_ASSERT(deviceCount > 0, "Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (auto device : devices)
		{
			uint32_t graphicsQueueFamily = FindGraphicsQueueFamily(device);
			uint32_t presentQueueFamily = FindPresentQueueFamily(device);
			if (graphicsQueueFamily != UINT32_MAX && presentQueueFamily != UINT32_MAX)
			{
				m_PhysicalDevice = device;
				return;
			}
		}

		SN_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to select a Vulkan device");
	}

	void VulkanContext::CreateLogicalDevice()
	{
		float queuePriority = 1.0f;
		uint32_t graphicsFamily = FindGraphicsQueueFamily(m_PhysicalDevice);
		uint32_t presentFamily = FindPresentQueueFamily(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::unordered_set<uint32_t> uniqueFamilies{ graphicsFamily, presentFamily };
		for (uint32_t family : uniqueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceSynchronization2Features sync2Features{};
		sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		sync2Features.synchronization2 = VK_TRUE;

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.dynamicRendering = VK_TRUE;
		dynamicRendering.pNext = &sync2Features;

		VkPhysicalDeviceFeatures deviceFeatures{};

		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.pNext = &dynamicRendering;

		VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device");

		vkGetDeviceQueue(m_Device, graphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, presentFamily, 0, &m_PresentQueue);
	}

	void VulkanContext::CreateSwapchain()
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());

		VkSurfaceFormatKHR surfaceFormat = formats[0];
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				surfaceFormat = format;
				break;
			}
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (auto mode : presentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = mode;
				break;
			}
		}

		VkExtent2D extent = capabilities.currentExtent;
		if (extent.width == UINT32_MAX)
		{
			extent.width = 1280;
			extent.height = 720;
		}

		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
			imageCount = capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { FindGraphicsQueueFamily(m_PhysicalDevice), FindPresentQueueFamily(m_PhysicalDevice) };
		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create swapchain");

		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

		m_SwapchainFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;
	}

	void VulkanContext::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());
		for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapchainFormat;
			viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_SwapchainImageViews[i]);
			SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create swapchain image view");
		}
	}

	void VulkanContext::CreateCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = FindGraphicsQueueFamily(m_PhysicalDevice);

		VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool");
	}

	void VulkanContext::AllocateCommandBuffers()
	{
		m_CommandBuffers.resize(m_SwapchainImages.size());
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data());
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffers");
	}

	void VulkanContext::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(m_SwapchainImages.size());
		m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
		m_InFlightFences.resize(m_SwapchainImages.size());

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
		{
			SN_CORE_ASSERT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS, "Failed to create semaphores");
			SN_CORE_ASSERT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS, "Failed to create semaphores");
			SN_CORE_ASSERT(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, "Failed to create fences");
		}
	}

	uint32_t VulkanContext::AcquireImage()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
		SN_CORE_ASSERT(result == VK_SUCCESS, "Failed to acquire swapchain image");
		return imageIndex;
	}

	void VulkanContext::TransitionToAttachment(VkCommandBuffer cmd, VkImage image)
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependency);
	}

	void VulkanContext::TransitionToPresent(VkCommandBuffer cmd, VkImage image)
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(cmd, &dependency);
	}

	void VulkanContext::BeginFrame()
	{
		m_ActiveImageIndex = AcquireImage();
		m_ActiveCommandBuffer = m_CommandBuffers[m_ActiveImageIndex];

		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);
		vkResetCommandBuffer(m_ActiveCommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(m_ActiveCommandBuffer, &beginInfo);

		TransitionToAttachment(m_ActiveCommandBuffer, m_SwapchainImages[m_ActiveImageIndex]);

		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = m_SwapchainImageViews[m_ActiveImageIndex];
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { { m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a } };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0,0 };
		renderingInfo.renderArea.extent = m_SwapchainExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;

		vkCmdBeginRendering(m_ActiveCommandBuffer, &renderingInfo);
	}

	void VulkanContext::EndFrame()
	{
		if (m_ActiveCommandBuffer == VK_NULL_HANDLE)
			return;

		vkCmdEndRendering(m_ActiveCommandBuffer);
		TransitionToPresent(m_ActiveCommandBuffer, m_SwapchainImages[m_ActiveImageIndex]);
		vkEndCommandBuffer(m_ActiveCommandBuffer);

		VkSemaphoreSubmitInfo waitSemaphore{};
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.semaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
		waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo signalSemaphore{};
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.semaphore = m_RenderFinishedSemaphores[m_CurrentFrame];
		signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = m_ActiveCommandBuffer;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphore;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphore;

		SN_CORE_ASSERT(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) == VK_SUCCESS, "Failed to submit Vulkan command buffer");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_ActiveImageIndex;
		vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		m_CurrentFrame = (m_CurrentFrame + 1) % static_cast<uint32_t>(m_SwapchainImages.size());
		m_ActiveCommandBuffer = VK_NULL_HANDLE;
	}

}
