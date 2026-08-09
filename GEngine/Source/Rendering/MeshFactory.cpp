#include "PCH.hpp"

#include "MeshFactory.hpp"

#include <cmath>

namespace GEngine::MeshFactory {

namespace {

void ComputeTangents(Mesh& mesh) {
    std::vector<std::array<float, 3>> accumulated(mesh.Vertices.size(), {0.0f, 0.0f, 0.0f});

    for (size_t i = 0; i + 2 < mesh.Indices.size(); i += 3) {
        const uint32_t i0 = mesh.Indices[i];
        const uint32_t i1 = mesh.Indices[i + 1];
        const uint32_t i2 = mesh.Indices[i + 2];

        const auto& p0 = mesh.Vertices[i0].Position;
        const auto& p1 = mesh.Vertices[i1].Position;
        const auto& p2 = mesh.Vertices[i2].Position;

        const auto& uv0 = mesh.Vertices[i0].UV;
        const auto& uv1 = mesh.Vertices[i1].UV;
        const auto& uv2 = mesh.Vertices[i2].UV;

        const std::array<float, 3> edge1{p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
        const std::array<float, 3> edge2{p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

        const float duv1x = uv1[0] - uv0[0];
        const float duv1y = uv1[1] - uv0[1];
        const float duv2x = uv2[0] - uv0[0];
        const float duv2y = uv2[1] - uv0[1];

        const float determinant = duv1x * duv2y - duv2x * duv1y;
        if (std::abs(determinant) < 1e-7f)
            continue;

        const float f = 1.0f / determinant;
        const std::array<float, 3> tangent{
            f * (duv2y * edge1[0] - duv1y * edge2[0]),
            f * (duv2y * edge1[1] - duv1y * edge2[1]),
            f * (duv2y * edge1[2] - duv1y * edge2[2]),
        };

        for (const uint32_t index : {i0, i1, i2}) {
            accumulated[index][0] += tangent[0];
            accumulated[index][1] += tangent[1];
            accumulated[index][2] += tangent[2];
        }
    }

    for (size_t i = 0; i < mesh.Vertices.size(); ++i) {
        const float length = std::sqrt(accumulated[i][0] * accumulated[i][0] + accumulated[i][1] * accumulated[i][1] +
                                       accumulated[i][2] * accumulated[i][2]);
        if (length > 1e-7f) {
            mesh.Vertices[i].Tangent = {accumulated[i][0] / length, accumulated[i][1] / length,
                                        accumulated[i][2] / length};
        }
    }
}

} // namespace

const Mesh& Cube() {
    static constexpr Vertex vertices[] = {
        // Front face (z = +0.5)
        {.Position = {-0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 0.0f, 1.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 0.0f, 1.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 0.0f, 1.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {-0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 0.0f, 1.0f}, .UV = {0.0f, 0.0f}},
        // Back face (z = -0.5)
        {.Position = {0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 0.0f, -1.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {-0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 0.0f, -1.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {-0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 0.0f, -1.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 0.0f, -1.0f}, .UV = {0.0f, 0.0f}},
        // Left face (x = -0.5)
        {.Position = {-0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {-1.0f, 0.0f, 0.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {-0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {-1.0f, 0.0f, 0.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {-0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {-1.0f, 0.0f, 0.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {-0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {-1.0f, 0.0f, 0.0f}, .UV = {0.0f, 0.0f}},
        // Right face (x = +0.5)
        {.Position = {0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {1.0f, 0.0f, 0.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {1.0f, 0.0f, 0.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {1.0f, 0.0f, 0.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {1.0f, 0.0f, 0.0f}, .UV = {0.0f, 0.0f}},
        // Top face (y = +0.5)
        {.Position = {-0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {0.5f, 0.5f, 0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {-0.5f, 0.5f, -0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {0.0f, 0.0f}},
        // Bottom face (y = -0.5)
        {.Position = {-0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {0.0f, -1.0f, 0.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {0.5f, -0.5f, -0.5f, 1.0f}, .Normal = {0.0f, -1.0f, 0.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {0.0f, -1.0f, 0.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {-0.5f, -0.5f, 0.5f, 1.0f}, .Normal = {0.0f, -1.0f, 0.0f}, .UV = {0.0f, 0.0f}},
    };

    static constexpr uint16_t indices[] = {
        0,  1,  2,  2,  3,  0,  // front
        4,  5,  6,  6,  7,  4,  // back
        8,  9,  10, 10, 11, 8,  // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20, // bottom
    };

    static const Mesh cube = [] {
        Mesh m{.Vertices = std::vector<Vertex>(std::begin(vertices), std::end(vertices)),
               .Indices = std::vector<uint32_t>(std::begin(indices), std::end(indices))};
        ComputeTangents(m);
        return m;
    }();
    return cube;
}

const Mesh& Plane() {
    static constexpr Vertex vertices[] = {
        {.Position = {-0.5f, 0.0f, -0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {0.0f, 1.0f}},
        {.Position = {0.5f, 0.0f, -0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {1.0f, 1.0f}},
        {.Position = {0.5f, 0.0f, 0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {1.0f, 0.0f}},
        {.Position = {-0.5f, 0.0f, 0.5f, 1.0f}, .Normal = {0.0f, 1.0f, 0.0f}, .UV = {0.0f, 0.0f}},
    };

    static constexpr uint16_t indices[]{0, 2, 1, 2, 0, 3};

    static const Mesh plane = [] {
        Mesh m{.Vertices = std::vector<Vertex>(std::begin(vertices), std::end(vertices)),
               .Indices = std::vector<uint32_t>(std::begin(indices), std::end(indices))};
        ComputeTangents(m);
        return m;
    }();
    return plane;
}

} // namespace GEngine::MeshFactory
