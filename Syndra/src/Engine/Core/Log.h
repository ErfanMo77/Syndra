#pragma once

#include "Core.h"
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Syndra {
	class Log
	{
	public:

		static void init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};
}

// core log macros
#define SN_CORE_ERROR(...)  ::Syndra::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SN_CORE_WARN(...)	::Syndra::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SN_CORE_TRACE(...)	::Syndra::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SN_CORE_INFO(...)	::Syndra::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SN_CORE_FATAL(...)	::Syndra::Log::GetCoreLogger()->fatal(__VA_ARGS__)
			 
// client log macros		 
#define SN_ERROR(...)		::Syndra::Log::GetClientLogger()->error(__VA_ARGS__)
#define SN_WARN(...)		::Syndra::Log::GetClientLogger()->warn(__VA_ARGS__)
#define SN_TRACE(...)		::Syndra::Log::GetClientLogger()->trace(__VA_ARGS__)
#define SN_INFO(...)		::Syndra::Log::GetClientLogger()->info(__VA_ARGS__)
#define SN_FATAL(...)		::Syndra::Log::GetClientLogger()->fatal(__VA_ARGS__)



