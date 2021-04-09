#include "lpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "glad/glad.h"

namespace Syndra {

	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         SN_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       SN_CORE_INFO(message); return;
		case GL_DEBUG_SEVERITY_LOW:          SN_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: SN_CORE_TRACE(message); return;
		}

		SN_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		SN_CORE_WARN("Driver: {0}", glGetString(GL_VENDOR));
		SN_CORE_WARN("Renderer: {0}", glGetString(GL_RENDERER));
		SN_CORE_WARN("Version: {0}", glGetString(GL_VERSION));
#ifdef SN_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		glDrawArrays(GL_TRIANGLES,0,36);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	std::string OpenGLRendererAPI::GetRendererInfo()
	{
		std::string info{};
		info += "Driver: " + std::string((char*)glGetString(GL_VENDOR)) + "\n";
		info += "Renderer: " + std::string((char*)glGetString(GL_RENDERER))+ "\n";
		info += "Version: " + std::string((char*)glGetString(GL_VERSION));
		return info;
	}

}
