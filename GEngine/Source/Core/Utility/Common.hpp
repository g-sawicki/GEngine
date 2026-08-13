#pragma once

namespace GEngine {

template <typename T> constexpr bool is_powerof2(T v) {
    return v && ((v & (v - 1)) == 0);
}

template <auto Multiple, typename T>
    requires(is_powerof2(Multiple))
constexpr T RoundUp(T number) {
    return (number + Multiple - 1) & ~(Multiple - 1);
}

} // namespace GEngine
