#pragma once

#include "Scene/Material.hpp"

#include "Graphics/Vertex.hpp"

#include <cstdint>
#include <vector>

namespace GEngine {

struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    uint32_t MaterialIndex{};
};

struct Model {
    std::vector<Mesh> Meshes;
    std::vector<Material> Materials;
};

struct ModelHandle {
    uint32_t Id{};

    bool IsValid() const { return Id != 0; }
    bool operator==(const ModelHandle& other) const { return Id == other.Id; }
};

struct ModelComponent {
    ModelHandle Model{};
    bool CastsShadow{true};
};

} // namespace GEngine
