#pragma once
#include <memory>

#ifdef SN_PLATFORM_WINDOWS
	#if SN_DYNAMIC_LINK
		#ifdef SYNDRA_BUILD_DLL
			#define ENGINE_API __declspec(dllexport)
		#else
			#define ENGINE_API __declspec(dllimport)
		#endif // ENGINE_BUILD_DLL
#else
	#define ENGINE_API
#endif
#else
#error Engine only supports windows

#endif // FB_PLATFORM_WINDOWS

#define BIT(x) (1 << x)
#ifdef SN_DEBUG
#if defined(SN_PLATFORM_WINDOWS)
#define SN_DEBUGBREAK() __debugbreak()
#elif defined(HZ_PLATFORM_LINUX)
#include <signal.h>
#define SN_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#define SN_ENABLE_ASSERTS
#else
#define SN_DEBUGBREAK()
#endif

#define SN_EXPAND_MACRO(x) x
#define SN_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define SN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Syndra {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Log.h"
