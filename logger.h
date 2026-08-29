#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <sstream>

#include <imgui.h>

enum class LogSeverity {
	Info,
	Warning,
	Error,
	Debug
};

struct LogEntry {
	LogSeverity severity;
	std::string text;
};

struct AppLog {
	std::vector<LogEntry> Items;
	bool ScrollToBottom = false;
	std::mutex logMutex;

	void Clear() {
		std::lock_guard<std::mutex> lock(logMutex);
		Items.clear();
	}

	void AddLog(LogSeverity severity, const std::string& msg) {
#ifdef NDEBUG
		if (severity == LogSeverity::Debug) return;
#endif
		std::lock_guard<std::mutex> lock(logMutex);
		Items.push_back({ severity, msg });
		ScrollToBottom = true;
	}

	void Draw(const char* title, bool* p_open) {
		ImGui::SetNextWindowSize(ImVec2(600, 350), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title, p_open)) {
			ImGui::End();
			return;
		}

		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (copy) ImGui::LogToClipboard();

		std::lock_guard<std::mutex> lock(logMutex);
		for (const auto& item : Items) {
			ImVec4 color;
			switch (item.severity) {
			case LogSeverity::Info:
				color = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); // Green
				break;
			case LogSeverity::Warning:
				color = ImVec4(0.9f, 0.9f, 0.0f, 1.0f); // Yellow
				break;
			case LogSeverity::Error:
				color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red
				break;
			case LogSeverity::Debug:
				color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); // Blue
				break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(item.text.c_str());
			ImGui::PopStyleColor();
		}

		if (ScrollToBottom) {
			ImGui::SetScrollHereY(1.0f);
			ScrollToBottom = false;
		}

		ImGui::EndChild();
		ImGui::End();
	}
};

extern AppLog appLog;
inline void logMsg(LogSeverity severity, const char* fmt, ...) {
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;
	va_end(args);
	appLog.AddLog(severity, buf);
}

class LogStream {
public:
	LogStream(LogSeverity severity) : severity_(severity) {}
	~LogStream() {
		appLog.AddLog(severity_, stream_.str());
	}

	template<typename T>
	LogStream& operator<<(const T& value) {
		stream_ << value;
		return *this;
	}

	LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
		manip(stream_);
		return *this;
	}

private:
	LogSeverity severity_;
	std::ostringstream stream_;
};

#define LOG_INFO LogStream(LogSeverity::Info)
#define LOG_WARN LogStream(LogSeverity::Warning)
#define LOG_ERROR LogStream(LogSeverity::Error)
#ifdef NDEBUG
#define LOG_DEBUG if(false) LogStream(LogSeverity::Debug)
#else
#define LOG_DEBUG LogStream(LogSeverity::Debug)
#endif