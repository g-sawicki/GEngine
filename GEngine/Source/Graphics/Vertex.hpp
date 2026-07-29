#pragma once

#include <array>

namespace GEngine {

struct Vertex {
    std::array<float, 4> position{};
    std::array<float, 4> color{};
};

} // namespace GEngine
