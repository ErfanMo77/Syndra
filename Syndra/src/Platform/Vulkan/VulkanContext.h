#pragma once

#include "Engine/Renderer/GraphicsContext.h"

#include <volk.h>

#include <array>
#include <functional>
#include <optional>
#include <vector>

struct GLFWwindow;
struct VmaAllocator_T;

namespace Syndra {

	using VmaAllocator = VmaAllocator_T*;

	class VulkanContext : public GraphicsContext
	{
	public:
		explicit VulkanContext(GLFWwindow* windowHandle);
		~VulkanContext() override;

		void Init() override;
		void BeginFrame() override;
		void EndFrame() override;
		void SwapBuffers() override;
		void SetVSync(bool enabled) override;

		VkInstance GetInstance() const { return m_Instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
		uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }
		VkFormat GetSwapchainImageFormat() const { return m_SwapchainImageFormat; }
		VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }
		uint32_t GetSwapchainImageCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }
		uint32_t GetSwapchainMinImageCount() const { return m_SwapchainMinImageCount; }
		VkCommandPool GetCommandPool() const { return m_CommandPool; }
		VkCommandBuffer GetActiveFrameCommandBuffer() const { return m_FrameInProgress ? m_CommandBuffers[m_CurrentFrame] : VK_NULL_HANDLE; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
		uint32_t GetFramesInFlight() const { return static_cast<uint32_t>(m_CommandBuffers.size()); }
		uint64_t GetFrameNumber() const { return m_FrameNumber; }
		VmaAllocator GetAllocator() const { return m_Allocator; }
		void SetOverlayRenderCallback(const std::function<void(VkCommandBuffer, uint32_t)>& callback) { m_OverlayRenderCallback = callback; }

		VkCommandBuffer BeginSingleTimeCommands() const;
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

		static VulkanContext* GetCurrent() { return s_CurrentContext; }

	private:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> GraphicsFamily;
			std::optional<uint32_t> PresentFamily;

			bool IsComplete() const
			{
				return GraphicsFamily.has_value() && PresentFamily.has_value();
			}
		};

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities{};
			std::vector<VkSurfaceFormatKHR> Formats;
			std::vector<VkPresentModeKHR> PresentModes;
		};

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateAllocator();
		void CreateSwapchain();
		void CreateImageViews();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreateRenderFinishedSemaphores();
		void DestroyRenderFinishedSemaphores();

		void CleanupSwapchain();
		void RecreateSwapchain();
		void WaitForValidFramebufferSize() const;
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void TransitionSwapchainImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;
		bool CheckValidationLayerSupport() const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
		bool IsDeviceSuitable(VkPhysicalDevice device) const;
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		GLFWwindow* m_WindowHandle = nullptr;
		bool m_Initialized = false;
		bool m_FramebufferResized = false;
		bool m_VSync = true;
		bool m_FrameInProgress = false;
		uint32_t m_AcquiredImageIndex = 0;
		uint32_t m_CurrentFrame = 0;
		uint64_t m_FrameNumber = 0;

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		uint32_t m_GraphicsQueueFamily = 0;
		uint32_t m_PresentQueueFamily = 0;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		std::vector<VkImageLayout> m_SwapchainImageLayouts;
		uint32_t m_SwapchainMinImageCount = 2;
		VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_SwapchainExtent{};

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, 2> m_CommandBuffers{};

		std::array<VkSemaphore, 2> m_ImageAvailableSemaphores{};
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::array<VkFence, 2> m_InFlightFences{};
		std::vector<VkFence> m_ImagesInFlight;
		VmaAllocator m_Allocator = nullptr;
		std::function<void(VkCommandBuffer, uint32_t)> m_OverlayRenderCallback;

		static VulkanContext* s_CurrentContext;
	};

}
