#pragma once

#include <array>

namespace GEngine {

struct Vertex {
    std::array<float, 4> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};
};

} // namespace GEngine
