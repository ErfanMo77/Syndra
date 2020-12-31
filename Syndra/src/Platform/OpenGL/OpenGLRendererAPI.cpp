#include "lpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "glad/glad.h"

namespace Syndra {

	void OpenGLRendererAPI::Init()
	{
		SN_CORE_WARN("Driver: {0}", glGetString(GL_VENDOR));
		SN_CORE_WARN("Renderer: {0}", glGetString(GL_RENDERER));
		SN_CORE_WARN("Version: {0}", glGetString(GL_VERSION));
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
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
	}

}
