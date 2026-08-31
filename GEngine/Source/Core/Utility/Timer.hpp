#pragma once

#include "Core/Log.hpp"

#include <chrono>
#include <source_location>
#include <string>
#include <utility>

namespace GEngine {

class Timer {
  public:
    Timer() { Reset(); }

    void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

    template <typename Period = std::ratio<1>>
    float Elapsed() {
        return std::chrono::duration<float, Period>(std::chrono::high_resolution_clock::now() - m_Start).count();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start{};
};

class ScopedTimer {
  public:
    ScopedTimer(std::string message, spdlog::level::level_enum level = spdlog::level::debug,
                std::source_location location = std::source_location::current())
        : m_Message(std::move(message)), m_Level(level), m_Location(location) {
        m_Timer.Reset();
    }

    ~ScopedTimer() {
        const float elapsedMs = m_Timer.Elapsed<std::milli>();
        ::GEngine::Log::GetCoreLogger().log(
            spdlog::source_loc{m_Location.file_name(), static_cast<int>(m_Location.line()), m_Location.function_name()},
            m_Level, "{} in {:.2f} ms", m_Message, elapsedMs);
    }

  private:
    Timer m_Timer{};
    std::string m_Message{};
    spdlog::level::level_enum m_Level{spdlog::level::debug};
    std::source_location m_Location{};
};

#define GE_CONCAT_IMPL(a, b) a##b
#define GE_CONCAT(a, b) GE_CONCAT_IMPL(a, b)
#define GE_SCOPED_TIMER(...) ::GEngine::ScopedTimer GE_CONCAT(ge_scoped_timer_, __COUNTER__)(__VA_ARGS__)

} // namespace GEngine
