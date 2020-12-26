#include "lpch.h"
#include "Engine/Renderer/VertexArray.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Engine/Renderer/Renderer.h"

namespace Syndra {

	Ref<VertexArray> VertexArray::Create() {
		switch (Renderer::GetAPI())
		{
			case RendererAPI::NONE: SN_CORE_ASSERT(false, "RendererAPI::NONE is not supported yet!"); return nullptr;
			case RendererAPI::OpenGL: return CreateRef<OpenGLVertexArray>();
		}

		SN_CORE_ASSERT(false, "Unknown API!");
		return nullptr;
	}

}