#include "PCH.hpp"

#include "Log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdio>

namespace GEngine {

namespace {

void AttachConsoleForStdout() {
    if (::AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
        ::AllocConsole();
    }
    FILE* stream{nullptr};
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
}

} // namespace

spdlog::logger Log::s_CoreLogger{"ENGINE"};
spdlog::logger Log::s_ClientLogger{"APP"};

void Log::Init() {
    AttachConsoleForStdout();

    wchar_t exePath[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    const std::filesystem::path logPath{std::filesystem::path(exePath).parent_path() / "GEngine.log"};

    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true));

    logSinks[0]->set_pattern("%^[%T] %n: %v%$");
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

    s_CoreLogger = spdlog::logger("ENGINE", begin(logSinks), end(logSinks));
    s_CoreLogger.set_level(spdlog::level::trace);
    s_CoreLogger.flush_on(spdlog::level::trace);

    s_ClientLogger = spdlog::logger("APP", begin(logSinks), end(logSinks));
    s_ClientLogger.set_level(spdlog::level::trace);
    s_ClientLogger.flush_on(spdlog::level::trace);
}

} // namespace GEngine
