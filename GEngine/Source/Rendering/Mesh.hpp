#pragma once

#include "Graphics/Vertex.hpp"

#include <cstdint>
#include <vector>

namespace GEngine {

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

} // namespace GEngine
