#pragma once

#include <DirectXCollision.h>

#include <cstdint>
#include <filesystem>
#include <variant>
#include <vector>

namespace GEngine {

// Vertex data
struct Vertex {
    DirectX::XMFLOAT3 Position{};
    DirectX::XMFLOAT3 Normal{};
    DirectX::XMFLOAT3 Tangent{};
    DirectX::XMFLOAT2 UV{};
};

// Texture
struct TexturePath {
    std::filesystem::path Path{};
    bool IsSRGB{};
};

struct TextureEmbedded {
    std::vector<uint8_t> Buffer{};
    bool IsSRGB{};
};

using TextureSource = std::variant<std::monostate, TexturePath, TextureEmbedded>;

// Material
struct Material {
    int32_t BaseColorTextureIndex{-1};
    int32_t NormalTextureIndex{-1};
    int32_t RoughnessMetallic{-1};
};

[[nodiscard]] inline DirectX::BoundingBox ComputeMeshBounds(const std::vector<Vertex>& vertices) noexcept {
    DirectX::BoundingBox bounds{};
    if (!vertices.empty())
        DirectX::BoundingBox::CreateFromPoints(bounds, vertices.size(), &vertices[0].Position, sizeof(Vertex));
    return bounds;
}

// Mesh
struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    DirectX::BoundingBox BoundingBox{};
    uint32_t MaterialIndex{};
};

// Model
struct Model {
    std::vector<Mesh> Meshes;
    std::vector<Material> Materials;
    std::vector<TextureSource> Textures;
};

} // namespace GEngine
