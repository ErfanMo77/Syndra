#include "Engine/Core/Log.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanFrameBuffer.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "Platform/Vulkan/VulkanTexture.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"

#include "GLFW/glfw3.h"

#include <array>
#include <exception>

namespace {

	void RunVulkanResourceSmoke()
	{
		SN_CORE_INFO("Running Vulkan resource smoke test.");

		for (int iteration = 0; iteration < 3; ++iteration)
		{
			float vertices[] = {
				-1.0f, -1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
				0.0f, 1.0f, 0.0f
			};
			uint32_t indices[] = { 0, 1, 2 };
			Syndra::VulkanVertexBuffer vertexBuffer(vertices, sizeof(vertices));
			Syndra::VulkanIndexBuffer indexBuffer(indices, 3);

			Syndra::VulkanUniformBuffer cameraUniform(sizeof(float) * 16, 0);
			float matrixData[16] = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			cameraUniform.SetData(matrixData, sizeof(matrixData));

			const uint32_t texWidth = 4;
			const uint32_t texHeight = 4;
			std::array<uint8_t, texWidth * texHeight * 4> checkerData{};
			for (uint32_t y = 0; y < texHeight; ++y)
			{
				for (uint32_t x = 0; x < texWidth; ++x)
				{
					const uint32_t index = (y * texWidth + x) * 4;
					const uint8_t value = ((x + y) % 2 == 0) ? 255 : 40;
					checkerData[index + 0] = value;
					checkerData[index + 1] = value;
					checkerData[index + 2] = value;
					checkerData[index + 3] = 255;
				}
			}

			Syndra::VulkanTexture2D texture2D(texWidth, texHeight, checkerData.data(), false);
			texture2D.SetData(checkerData.data(), static_cast<uint32_t>(checkerData.size()));

			float poissonData[] = {
				0.0f, 0.0f,
				0.5f, 0.5f,
				-0.5f, 0.5f,
				0.5f, -0.5f,
				-0.5f, -0.5f,
				0.25f, -0.75f,
				-0.25f, 0.75f,
				0.75f, 0.25f
			};
			Syndra::VulkanTexture1D texture1D(8, poissonData);
			texture1D.SetData(poissonData, 8);

			Syndra::FramebufferSpecification framebufferSpec;
			framebufferSpec.Width = 640;
			framebufferSpec.Height = 360;
			framebufferSpec.Attachments = {
				Syndra::FramebufferTextureFormat::RGBA8,
				Syndra::FramebufferTextureFormat::RED_INTEGER,
				Syndra::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			framebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			Syndra::VulkanFrameBuffer framebuffer(framebufferSpec);
			framebuffer.Resize(800, 450);
			framebuffer.Resize(640, 360);

			Syndra::VulkanShader shader("Syndra-Editor/assets/shaders/vulkan/DeferredLighting.glsl");
			shader.Reload();

			SN_CORE_INFO("Vulkan resource smoke iteration {} completed.", iteration + 1);
		}

		SN_CORE_INFO("Vulkan resource smoke test completed.");
	}

}

int main()
{
	Syndra::Log::init();

	if (glfwInit() != GLFW_TRUE)
	{
		SN_CORE_ERROR("Failed to initialize GLFW for Vulkan bootstrap.");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Syndra Vulkan Bootstrap", nullptr, nullptr);
	if (window == nullptr)
	{
		SN_CORE_ERROR("Failed to create GLFW window for Vulkan bootstrap.");
		glfwTerminate();
		return -1;
	}

	int exitCode = 0;
	try
	{
		Syndra::VulkanContext context(window);
		context.Init();
		context.SetVSync(true);
		RunVulkanResourceSmoke();

		const double startTime = glfwGetTime();
		bool resizedOnce = false;

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			const double elapsed = glfwGetTime() - startTime;
			if (!resizedOnce && elapsed > 1.5)
			{
				glfwSetWindowSize(window, 1024, 640);
				resizedOnce = true;
			}
			if (elapsed > 5.0)
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}

			context.BeginFrame();
			context.EndFrame();
		}

		SN_CORE_INFO("Vulkan bootstrap completed successfully.");
	}
	catch (const std::exception& exception)
	{
		SN_CORE_ERROR("Vulkan bootstrap failed: {}", exception.what());
		exitCode = -1;
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return exitCode;
}
