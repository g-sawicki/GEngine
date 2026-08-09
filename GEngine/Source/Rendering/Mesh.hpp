#pragma once

#include "Graphics/Vertex.hpp"

#include <cstdint>
#include <vector>

namespace GEngine {

struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    uint32_t MaterialIndex{};
};

} // namespace GEngine
