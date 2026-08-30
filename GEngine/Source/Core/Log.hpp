#pragma once

#include <spdlog/spdlog.h>

namespace GEngine {

class Log {
  public:
    using Logger = spdlog::logger;

    static void Init();

    static Logger& GetCoreLogger() noexcept { return s_CoreLogger; }
    static Logger& GetClientLogger() noexcept { return s_ClientLogger; }

  private:
    static Logger s_CoreLogger;
    static Logger s_ClientLogger;
};

} // namespace GEngine

#define GE_CORE_DEBUG(...) ::GEngine::Log::GetCoreLogger().debug(__VA_ARGS__)
#define GE_CORE_INFO(...) ::GEngine::Log::GetCoreLogger().info(__VA_ARGS__)
#define GE_CORE_WARN(...) ::GEngine::Log::GetCoreLogger().warn(__VA_ARGS__)
#define GE_CORE_ERROR(...) ::GEngine::Log::GetCoreLogger().error(__VA_ARGS__)
#define GE_CORE_CRITICAL(...) ::GEngine::Log::GetCoreLogger().critical(__VA_ARGS__)

#define GE_DEBUG(...) ::GEngine::Log::GetClientLogger().debug(__VA_ARGS__)
#define GE_INFO(...) ::GEngine::Log::GetClientLogger().info(__VA_ARGS__)
#define GE_WARN(...) ::GEngine::Log::GetClientLogger().warn(__VA_ARGS__)
#define GE_ERROR(...) ::GEngine::Log::GetClientLogger().error(__VA_ARGS__)
#define GE_CRITICAL(...) ::GEngine::Log::GetClientLogger().critical(__VA_ARGS__)
