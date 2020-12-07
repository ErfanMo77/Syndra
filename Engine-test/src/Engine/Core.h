#pragma once
#include <memory>

#ifdef FB_PLATFORM_WINDOWS
	#ifdef ENGINE_BUILD_DLL
		#define ENGINE_API __declspec(dllexport)
	#else
		#define ENGINE_API __declspec(dllimport)
	#endif // ENGINE_BUILD_DLL
#else
#error Engine only supports windows

#endif // FB_PLATFORM_WINDOWS

#define BIT(x) (1 << x)
#include "Log.h"