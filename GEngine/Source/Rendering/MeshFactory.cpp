#include "PCH.hpp"

#include "MeshFactory.hpp"

namespace GEngine::MeshFactory {

const Mesh& Cube() {
    static constexpr Vertex vertices[] = {
        // Front face (z = +0.5) — red
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        // Back face (z = -0.5) — green
        {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        // Left face (x = -0.5) — blue
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        // Right face (x = +0.5) — yellow
        {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
        // Top face (y = +0.5) — cyan
        {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        // Bottom face (y = -0.5) — magenta
        {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
    };

    static constexpr uint16_t indices[] = {
        0,  1,  2,  2,  3,  0,  // front
        4,  5,  6,  6,  7,  4,  // back
        8,  9,  10, 10, 11, 8,  // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20, // bottom
    };

    static const Mesh cube(std::vector<Vertex>(std::begin(vertices), std::end(vertices)),
                           std::vector<uint16_t>(std::begin(indices), std::end(indices)));
    return cube;
}

} // namespace GEngine::MeshFactory
