#include "lpch.h"
#include "Engine/Core/Log.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/ostream_sink.h"

namespace Syndra {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::init() {

		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>("Syndra-Debug.log", true));

		s_CoreLogger = std::make_shared<spdlog::logger>("ENGINE", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->set_pattern("[%T] [%l] %n: %v");
		s_ClientLogger->flush_on(spdlog::level::trace);

	}

}