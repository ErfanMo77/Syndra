#include "lpch.h"

#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Syndra {



	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		if (status == 0)
		{
			SN_CORE_ERROR("Failed to initialize GLAD for OpenGL context.");
		}
	}

	void OpenGLContext::BeginFrame()
	{
	}

	void OpenGLContext::EndFrame()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	void OpenGLContext::SwapBuffers()
	{
		EndFrame();
	}

	void OpenGLContext::SetVSync(bool enabled)
	{
		glfwSwapInterval(enabled ? 1 : 0);
	}

}
