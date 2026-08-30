#pragma once

#include <bit>

namespace GEngine {

template <typename T>
constexpr bool is_powerof2(T v) {
    return v && ((v & (v - 1)) == 0);
}

template <auto Multiple, std::unsigned_integral T>
    requires(is_powerof2(Multiple))
constexpr T RoundUp(T number) {
    return (number + static_cast<T>(Multiple) - 1) & ~static_cast<T>(Multiple - 1);
}

template <typename T>
constexpr T DivideRoundUp(T numerator, T denominator) {
    return (numerator + denominator - 1) / denominator;
}

} // namespace GEngine
