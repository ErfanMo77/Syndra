#include "lpch.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Syndra {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}