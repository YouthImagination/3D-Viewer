#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace viewer
{
	std::shared_ptr<spdlog::logger> Log::m_CoreLogger;
	void Log::Init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");

		m_CoreLogger = spdlog::stdout_color_mt("3D-Viewer");
		m_CoreLogger->set_level(spdlog::level::trace);
	}
}