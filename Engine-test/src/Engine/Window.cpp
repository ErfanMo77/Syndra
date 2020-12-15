#include "lpch.h"
#include "Engine/Window.h"

#ifdef FB_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Engine
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
#ifdef FB_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
#else
		EG_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}