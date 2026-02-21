#include "lpch.h"

#include "Platform/Vulkan/VulkanContext.h"

#include "Engine/Core/Instrument.h"
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"

#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>

namespace {

	constexpr uint32_t kMaxFramesInFlight = 2;
	constexpr VkClearColorValue kDefaultClearColor = { { 0.07f, 0.07f, 0.09f, 1.0f } };

#ifdef SN_DEBUG
	constexpr bool kEnableValidationLayers = true;
#else
	constexpr bool kEnableValidationLayers = false;
#endif

	const std::array<const char*, 1> kValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::array<const char*, 1> kDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void ValidateVulkanResult(VkResult result, const char* operation)
	{
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(std::string(operation) + " failed with error code " + std::to_string(result));
		}
	}

	void ValidateVmaResult(VkResult result, const char* operation)
	{
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(std::string(operation) + " failed with error code " + std::to_string(result));
		}
	}

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	}

}

namespace Syndra {

	VulkanContext* VulkanContext::s_CurrentContext = nullptr;

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	VulkanContext::~VulkanContext()
	{
		if (!m_Initialized)
			return;

		if (m_Device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device);
		}

		CleanupSwapchain();

		for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
		{
			if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			if (m_InFlightFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}

		if (m_CommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		if (m_Allocator != nullptr)
		{
			vmaDestroyAllocator(m_Allocator);
			m_Allocator = nullptr;
		}

		if (m_Device != VK_NULL_HANDLE)
			vkDestroyDevice(m_Device, nullptr);

		if (m_Surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

		if (kEnableValidationLayers && m_DebugMessenger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

		if (m_Instance != VK_NULL_HANDLE)
			vkDestroyInstance(m_Instance, nullptr);

		if (s_CurrentContext == this)
			s_CurrentContext = nullptr;
	}

	void VulkanContext::Init()
	{
		if (m_Initialized)
			return;

		if (volkInitialize() != VK_SUCCESS)
			throw std::runtime_error("volkInitialize failed");

		CreateInstance();
		volkLoadInstance(m_Instance);

		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateAllocator();
		CreateSwapchain();
		CreateImageViews();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();

		s_CurrentContext = this;
		m_Initialized = true;
		SN_CORE_INFO("Vulkan context initialized (API 1.4, dynamic rendering + synchronization2).");
	}

	VkCommandBuffer VulkanContext::BeginSingleTimeCommands() const
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		ValidateVulkanResult(vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer), "vkAllocateCommandBuffers(singleTime)");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		ValidateVulkanResult(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer(singleTime)");

		return commandBuffer;
	}

	void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
	{
		ValidateVulkanResult(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer(singleTime)");

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = commandBuffer;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;

		ValidateVulkanResult(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit2(singleTime)");
		ValidateVulkanResult(vkQueueWaitIdle(m_GraphicsQueue), "vkQueueWaitIdle(singleTime)");

		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	}

	void VulkanContext::BeginFrame()
	{
		SN_PROFILE_SCOPE("VulkanContext::BeginFrame");
		if (!m_Initialized)
			return;

		if (m_FrameInProgress)
			return;

		if (glfwWindowShouldClose(m_WindowHandle))
			return;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);

		if (width <= 0 || height <= 0)
			return;

		if (static_cast<uint32_t>(width) != m_SwapchainExtent.width || static_cast<uint32_t>(height) != m_SwapchainExtent.height)
			m_FramebufferResized = true;

		{
			SN_PROFILE_SCOPE("vkWaitForFences(frame)");
			ValidateVulkanResult(vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX), "vkWaitForFences");
		}

		VkResult acquireResult = VK_SUCCESS;
		{
			SN_PROFILE_SCOPE("vkAcquireNextImageKHR");
			acquireResult = vkAcquireNextImageKHR(
				m_Device,
				m_Swapchain,
				UINT64_MAX,
				m_ImageAvailableSemaphores[m_CurrentFrame],
				VK_NULL_HANDLE,
				&m_AcquiredImageIndex);
		}

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			if (!glfwWindowShouldClose(m_WindowHandle))
				RecreateSwapchain();
			return;
		}
		if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		{
			ValidateVulkanResult(acquireResult, "vkAcquireNextImageKHR");
		}

		if (m_ImagesInFlight[m_AcquiredImageIndex] != VK_NULL_HANDLE)
		{
			SN_PROFILE_SCOPE("vkWaitForFences(image)");
			ValidateVulkanResult(vkWaitForFences(m_Device, 1, &m_ImagesInFlight[m_AcquiredImageIndex], VK_TRUE, UINT64_MAX), "vkWaitForFences(image)");
		}

		m_ImagesInFlight[m_AcquiredImageIndex] = m_InFlightFences[m_CurrentFrame];
		{
			SN_PROFILE_SCOPE("vkResetFences");
			ValidateVulkanResult(vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]), "vkResetFences");
		}
		{
			SN_PROFILE_SCOPE("vkResetCommandBuffer");
			ValidateVulkanResult(vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0), "vkResetCommandBuffer");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		{
			SN_PROFILE_SCOPE("vkBeginCommandBuffer(frame)");
			ValidateVulkanResult(vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrame], &beginInfo), "vkBeginCommandBuffer(frame)");
		}

		m_FrameInProgress = true;
		++m_FrameNumber;
	}

	void VulkanContext::EndFrame()
	{
		SN_PROFILE_SCOPE("VulkanContext::EndFrame");
		if (!m_Initialized || !m_FrameInProgress)
			return;

		{
			SN_PROFILE_SCOPE("RecordCommandBuffer");
			RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], m_AcquiredImageIndex);
		}

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = m_CommandBuffers[m_CurrentFrame];

		VkSemaphoreSubmitInfo waitSemaphoreInfo{};
		waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreInfo.semaphore = m_ImageAvailableSemaphores[m_CurrentFrame];
		waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		waitSemaphoreInfo.deviceIndex = 0;

		VkSemaphoreSubmitInfo signalSemaphoreInfo{};
		signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreInfo.semaphore = m_RenderFinishedSemaphores[m_AcquiredImageIndex];
		signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		signalSemaphoreInfo.deviceIndex = 0;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

		{
			SN_PROFILE_SCOPE("vkQueueSubmit2(frame)");
			ValidateVulkanResult(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]), "vkQueueSubmit2");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_AcquiredImageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_AcquiredImageIndex;

		VkResult presentResult = VK_SUCCESS;
		{
			SN_PROFILE_SCOPE("vkQueuePresentKHR");
			presentResult = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		}
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
		{
			if (!glfwWindowShouldClose(m_WindowHandle))
				RecreateSwapchain();
		}
		else if (presentResult != VK_SUCCESS)
		{
			ValidateVulkanResult(presentResult, "vkQueuePresentKHR");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
		m_FrameInProgress = false;
	}

	void VulkanContext::SwapBuffers()
	{
		BeginFrame();
		EndFrame();
	}

	void VulkanContext::SetVSync(bool enabled)
	{
		if (m_VSync == enabled)
			return;

		m_VSync = enabled;

		if (m_Initialized)
		{
			RecreateSwapchain();
		}
	}

	void VulkanContext::CreateInstance()
	{
		if (kEnableValidationLayers && !CheckValidationLayerSupport())
			throw std::runtime_error("Requested Vulkan validation layers are not available.");

		uint32_t requiredExtensionCount = 0;
		const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
		if (requiredExtensions == nullptr)
			throw std::runtime_error("GLFW did not report required Vulkan instance extensions.");

		std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionCount);
		if (kEnableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Syndra";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "Syndra";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (kEnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			createInfo.ppEnabledLayerNames = kValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			debugCreateInfo.pfnUserCallback = DebugCallback;
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		ValidateVulkanResult(vkCreateInstance(&createInfo, nullptr, &m_Instance), "vkCreateInstance");
	}

	void VulkanContext::SetupDebugMessenger()
	{
		if (!kEnableValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = this;

		ValidateVulkanResult(vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger),
			"vkCreateDebugUtilsMessengerEXT");
	}

	void VulkanContext::CreateSurface()
	{
		ValidateVulkanResult(static_cast<VkResult>(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface)),
			"glfwCreateWindowSurface");
	}

	void VulkanContext::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		ValidateVulkanResult(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr), "vkEnumeratePhysicalDevices(count)");

		if (deviceCount == 0)
			throw std::runtime_error("No Vulkan physical device found.");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		ValidateVulkanResult(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()), "vkEnumeratePhysicalDevices(list)");

		int bestScore = -1;
		for (const VkPhysicalDevice device : devices)
		{
			if (!IsDeviceSuitable(device))
				continue;

			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);

			int score = 0;
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score += 1000;
			score += static_cast<int>(properties.limits.maxImageDimension2D);

			if (score > bestScore)
			{
				bestScore = score;
				m_PhysicalDevice = device;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("No suitable Vulkan 1.4 physical device found.");

		VkPhysicalDeviceProperties selectedProperties{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &selectedProperties);
		SN_CORE_INFO("Selected Vulkan device: {}", selectedProperties.deviceName);
	}

	void VulkanContext::CreateLogicalDevice()
	{
		const QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		m_GraphicsQueueFamily = indices.GraphicsFamily.value();
		m_PresentQueueFamily = indices.PresentFamily.value();

		std::set<uint32_t> uniqueQueueFamilies = {
			m_GraphicsQueueFamily,
			m_PresentQueueFamily
		};

		const float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(uniqueQueueFamilies.size());
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceVulkan13Features vulkan13Features{};
		vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;

		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.separateDepthStencilLayouts = VK_TRUE;

		VkPhysicalDeviceVulkan14Features vulkan14Features{};
		vulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		vulkan14Features.pNext = &vulkan13Features;
		vulkan13Features.pNext = &vulkan12Features;

		VkPhysicalDeviceFeatures2 deviceFeatures{};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures.features.independentBlend = VK_TRUE;
		deviceFeatures.pNext = &vulkan14Features;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &deviceFeatures;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

		if (kEnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			createInfo.ppEnabledLayerNames = kValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		ValidateVulkanResult(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice");
		volkLoadDevice(m_Device);

		vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);
	}

	void VulkanContext::CreateAllocator()
	{
		VmaVulkanFunctions vulkanFunctions{};

		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_Device;
		allocatorInfo.instance = m_Instance;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;

		ValidateVmaResult(vmaImportVulkanFunctionsFromVolk(&allocatorInfo, &vulkanFunctions), "vmaImportVulkanFunctionsFromVolk");
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;

		ValidateVmaResult(vmaCreateAllocator(&allocatorInfo, &m_Allocator), "vmaCreateAllocator");
	}

	void VulkanContext::CreateSwapchain()
	{
		const SwapchainSupportDetails support = QuerySwapchainSupport(m_PhysicalDevice);
		const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.Formats);
		const VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.PresentModes);
		const VkExtent2D extent = ChooseSwapExtent(support.Capabilities);

		const uint32_t requestedMinImageCount = std::max(2u, support.Capabilities.minImageCount);
		uint32_t imageCount = requestedMinImageCount + 1;
		if (support.Capabilities.maxImageCount > 0 && imageCount > support.Capabilities.maxImageCount)
			imageCount = support.Capabilities.maxImageCount;
		m_SwapchainMinImageCount = std::min(requestedMinImageCount, imageCount);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const uint32_t queueFamilyIndices[] = { m_GraphicsQueueFamily, m_PresentQueueFamily };
		if (m_GraphicsQueueFamily != m_PresentQueueFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = support.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		ValidateVulkanResult(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

		uint32_t swapchainImageCount = 0;
		ValidateVulkanResult(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchainImageCount, nullptr), "vkGetSwapchainImagesKHR(count)");
		m_SwapchainImages.resize(swapchainImageCount);
		ValidateVulkanResult(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &swapchainImageCount, m_SwapchainImages.data()), "vkGetSwapchainImagesKHR(list)");

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;
		m_SwapchainImageLayouts.assign(m_SwapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);

		SN_CORE_INFO("Created Vulkan swapchain: {}x{}, images={}", m_SwapchainExtent.width, m_SwapchainExtent.height, swapchainImageCount);
	}

	void VulkanContext::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SwapchainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			ValidateVulkanResult(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]), "vkCreateImageView");
		}
	}

	void VulkanContext::CreateCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

		ValidateVulkanResult(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
	}

	void VulkanContext::CreateCommandBuffers()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = kMaxFramesInFlight;

		ValidateVulkanResult(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()), "vkAllocateCommandBuffers");
	}

	void VulkanContext::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
		{
			ValidateVulkanResult(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "vkCreateSemaphore(imageAvailable)");
			ValidateVulkanResult(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
		}

		CreateRenderFinishedSemaphores();
		m_ImagesInFlight.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);
	}

	void VulkanContext::CreateRenderFinishedSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		m_RenderFinishedSemaphores.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);
		for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
		{
			ValidateVulkanResult(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
				"vkCreateSemaphore(renderFinished)");
		}
	}

	void VulkanContext::DestroyRenderFinishedSemaphores()
	{
		for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
				vkDestroySemaphore(m_Device, semaphore, nullptr);
		}
		m_RenderFinishedSemaphores.clear();
	}

	void VulkanContext::CleanupSwapchain()
	{
		DestroyRenderFinishedSemaphores();

		for (VkImageView imageView : m_SwapchainImageViews)
		{
			vkDestroyImageView(m_Device, imageView, nullptr);
		}
		m_SwapchainImageViews.clear();

		if (m_Swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;
		}

		m_SwapchainImages.clear();
		m_SwapchainImageLayouts.clear();
	}

	void VulkanContext::RecreateSwapchain()
	{
		if (glfwWindowShouldClose(m_WindowHandle))
			return;

		WaitForValidFramebufferSize();
		if (glfwWindowShouldClose(m_WindowHandle))
			return;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		if (width <= 0 || height <= 0)
			return;

		vkDeviceWaitIdle(m_Device);

		CleanupSwapchain();
		CreateSwapchain();
		CreateImageViews();
		CreateRenderFinishedSemaphores();
		m_ImagesInFlight.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);

		m_FramebufferResized = false;
		SN_CORE_INFO("Recreated Vulkan swapchain.");
	}

	void VulkanContext::WaitForValidFramebufferSize() const
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		while ((width == 0 || height == 0) && !glfwWindowShouldClose(m_WindowHandle))
		{
			glfwWaitEvents();
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		}
	}

	void VulkanContext::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		SN_PROFILE_SCOPE("VulkanContext::RecordCommandBuffer");
		const VkImageLayout oldLayout = m_SwapchainImageLayouts[imageIndex];
		TransitionSwapchainImage(commandBuffer, m_SwapchainImages[imageIndex], oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = m_SwapchainImageViews[imageIndex];
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = kDefaultClearColor;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = m_SwapchainExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		if (m_OverlayRenderCallback)
		{
			m_OverlayRenderCallback(commandBuffer, imageIndex);
		}
		vkCmdEndRendering(commandBuffer);

		TransitionSwapchainImage(commandBuffer, m_SwapchainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		m_SwapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		ValidateVulkanResult(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer(frame)");
	}

	void VulkanContext::TransitionSwapchainImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 srcAccessMask = 0;
		VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 dstAccessMask = 0;

		if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) &&
			newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			srcAccessMask = 0;
			dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_2_NONE;
			dstAccessMask = 0;
		}
		else
		{
			throw std::runtime_error("Unsupported swapchain layout transition.");
		}

		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = srcStageMask;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstStageMask = dstStageMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			const VkQueueFamilyProperties& queueFamily = queueFamilies[i];

			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				indices.GraphicsFamily = i;

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (presentSupport == VK_TRUE)
				indices.PresentFamily = i;

			if (indices.IsComplete())
				break;
		}

		return indices;
	}

	VulkanContext::SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device) const
	{
		SwapchainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount > 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
		if (presentModeCount > 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	bool VulkanContext::CheckValidationLayerSupport() const
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : kValidationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}

	bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device) const
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device, &properties);
		if (properties.apiVersion < VK_API_VERSION_1_4)
			return false;

		VkPhysicalDeviceVulkan13Features vulkan13Features{};
		vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

		VkPhysicalDeviceVulkan14Features vulkan14Features{};
		vulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		vulkan14Features.pNext = &vulkan13Features;
		vulkan13Features.pNext = &vulkan12Features;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &vulkan14Features;

		vkGetPhysicalDeviceFeatures2(device, &features2);
		if (vulkan13Features.dynamicRendering != VK_TRUE ||
			vulkan13Features.synchronization2 != VK_TRUE ||
			vulkan12Features.separateDepthStencilLayouts != VK_TRUE ||
			features2.features.independentBlend != VK_TRUE)
		{
			return false;
		}

		const QueueFamilyIndices indices = FindQueueFamilies(device);
		if (!indices.IsComplete())
			return false;

		if (!CheckDeviceExtensionSupport(device))
			return false;

		const SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);
		return !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();
	}

	VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
	{
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		return formats[0];
	}

	VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
	{
		if (!m_VSync)
		{
			for (const VkPresentModeKHR presentMode : presentModes)
			{
				if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
					return presentMode;
			}

			for (const VkPresentModeKHR presentMode : presentModes)
			{
				if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
					return presentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			SN_CORE_ERROR("Vulkan validation: {}", pCallbackData->pMessage);
		}
		else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			SN_CORE_WARN("Vulkan validation: {}", pCallbackData->pMessage);
		}
		else
		{
			SN_CORE_TRACE("Vulkan validation: {}", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

}
