#pragma once
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Syndra {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void SwapBuffers() override;
		virtual void SetVSync(bool enabled) override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}
