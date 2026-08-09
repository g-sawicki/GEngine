#pragma once

#include <array>

namespace GEngine {

struct Vertex {
    std::array<float, 4> Position{};
    std::array<float, 3> Normal{};
    std::array<float, 3> Tangent{};
    std::array<float, 2> UV{};
};

} // namespace GEngine
