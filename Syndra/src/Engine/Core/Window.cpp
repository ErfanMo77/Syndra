#include "lpch.h"
#include "Engine/Core/Window.h"

#ifdef SN_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Syndra
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
#ifdef SN_PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
#else
		//SN_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}