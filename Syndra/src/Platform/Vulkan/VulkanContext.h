#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace Syndra {

	class VulkanContext : public GraphicsContext
	{
	public:
		explicit VulkanContext(GLFWwindow* windowHandle);
		~VulkanContext();

		void Init() override;
		void SwapBuffers() override;

		VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveCommandBuffer; }
		void SetClearColor(const glm::vec4& color);
		void PrepareFrame();
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkFormat GetSwapchainFormat() const { return m_SwapchainFormat; }
		VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }

		static VulkanContext* Get();

	private:
		void CreateInstance();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();
		void CreateImageViews();
		void CreateCommandPool();
		void AllocateCommandBuffers();
		void CreateSyncObjects();
		void BeginFrame();
		void EndFrame();
		uint32_t FindGraphicsQueueFamily(VkPhysicalDevice device);
		uint32_t FindPresentQueueFamily(VkPhysicalDevice device);
		uint32_t AcquireImage();
		void TransitionToAttachment(VkCommandBuffer cmd, VkImage image);
		void TransitionToPresent(VkCommandBuffer cmd, VkImage image);

	private:
		GLFWwindow* m_WindowHandle = nullptr;

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_SwapchainFormat{};
		VkExtent2D m_SwapchainExtent{};

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_ActiveImageIndex = 0;
		VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;
		glm::vec4 m_ClearColor{ 0.f };

		static VulkanContext* s_Instance;
	};

}
