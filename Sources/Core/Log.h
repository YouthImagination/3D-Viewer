#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace viewer
{
	class Log {
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
	private:
		static std::shared_ptr<spdlog::logger> m_CoreLogger;
	};
}

#define LogE(...)		::viewer::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LogW(...)		::viewer::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LogI(...)		::viewer::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LogT(...)		::viewer::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LogC(...)		::viewer::Log::GetCoreLogger()->critical(__VA_ARGS__)
#define LogA(x, ...)    {if (!(x)){ LogE("Assertion Failed: {0}", __VA_ARGS__); __debugbreak();} }