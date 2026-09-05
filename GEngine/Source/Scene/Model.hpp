#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <variant>
#include <vector>

namespace GEngine {

// Vertex data
struct Vertex {
    std::array<float, 4> Position{};
    std::array<float, 3> Normal{};
    std::array<float, 3> Tangent{};
    std::array<float, 2> UV{};
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

// Mesh
struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    uint32_t MaterialIndex{};
};

// Model
struct Model {
    std::vector<Mesh> Meshes;
    std::vector<Material> Materials;
    std::vector<TextureSource> Textures;
};

} // namespace GEngine
