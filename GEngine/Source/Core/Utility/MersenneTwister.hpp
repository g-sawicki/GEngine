#pragma once

#include <random>

namespace GEngine {

class MersenneTwister {
  public:
    MersenneTwister() = delete;

    static void Seed(uint32_t seed) { m_Generator.seed(seed); }

    template <typename T>
    static T GetRandom(T min, T max)
        requires std::is_integral_v<T>
    {
        return std::uniform_int_distribution<T>(min, max)(m_Generator);
    }

    template <typename T>
    static T GetRandom(T min, T max)
        requires std::is_floating_point_v<T>
    {
        return std::uniform_real_distribution<T>(min, max)(m_Generator);
    }

  private:
    static inline std::mt19937 m_Generator{};
};

} // namespace GEngine
