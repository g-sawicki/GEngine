#include "PCH.hpp"

#include "AssetCache.hpp"

#include "Core/Utility/Image.hpp"
#include "Model.hpp"

#include <fstream>
#include <memory>

namespace GEngine {

struct ModelHeader {
    char Magic[4]{'G', 'E', 'M', 'L'};
    uint32_t MeshCount{};
    uint32_t MaterialCount{};
};

struct MeshHeader {
    uint32_t VertexCount{};
    uint32_t IndexCount{};
    uint32_t MaterialIndex{};
};

struct MaterialHeader {
    uint32_t AlbedoPathLen{};
    uint32_t NormalPathLen{};
    uint32_t RoughnessMetallicPathLen{};
    uint8_t AlbedoIsSRGB{};
    uint8_t NormalIsSRGB{};
    uint8_t RoughnessMetallicIsSRGB{};
};

AssetCache::AssetCache(const std::filesystem::path& directory) : m_Directory(directory) {}

bool AssetCache::SaveBinaryModel(const std::filesystem::path& filename, const Model& model) {
    if (!m_Directory.empty()) {
        std::filesystem::create_directories(m_Directory);
    }

    std::ofstream file(m_Directory / filename, std::ios::binary);
    if (!file)
        return false;

    ModelHeader modelHeader{
        .MeshCount = static_cast<uint32_t>(model.Meshes.size()),
        .MaterialCount = static_cast<uint32_t>(model.Materials.size()),
    };
    file.write(reinterpret_cast<const char*>(&modelHeader), sizeof(ModelHeader));

    for (const auto& mesh : model.Meshes) {
        MeshHeader meshHeader{
            .VertexCount = static_cast<uint32_t>(mesh.Vertices.size()),
            .IndexCount = static_cast<uint32_t>(mesh.Indices.size()),
            .MaterialIndex = mesh.MaterialIndex,
        };
        file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(MeshHeader));
    }

    for (const auto& mat : model.Materials) {
        std::string albedoPath = mat.Albedo ? mat.Albedo->GetPath().string() : "";
        std::string normalPath = mat.Normal ? mat.Normal->GetPath().string() : "";
        std::string rmPath = mat.RoughnessMetallic ? mat.RoughnessMetallic->GetPath().string() : "";

        const uint8_t albedoIsSRGB = mat.Albedo && mat.Albedo->IsSRGB() ? 1u : 0u;
        const uint8_t normalIsSRGB = mat.Normal && mat.Normal->IsSRGB() ? 1u : 0u;
        const uint8_t rmIsSRGB = mat.RoughnessMetallic && mat.RoughnessMetallic->IsSRGB() ? 1u : 0u;

        MaterialHeader matHeader{
            .AlbedoPathLen = static_cast<uint32_t>(albedoPath.size()),
            .NormalPathLen = static_cast<uint32_t>(normalPath.size()),
            .RoughnessMetallicPathLen = static_cast<uint32_t>(rmPath.size()),
            .AlbedoIsSRGB = albedoIsSRGB,
            .NormalIsSRGB = normalIsSRGB,
            .RoughnessMetallicIsSRGB = rmIsSRGB,
        };
        file.write(reinterpret_cast<const char*>(&matHeader), sizeof(MaterialHeader));

        if (matHeader.AlbedoPathLen > 0)
            file.write(albedoPath.data(), matHeader.AlbedoPathLen);
        if (matHeader.NormalPathLen > 0)
            file.write(normalPath.data(), matHeader.NormalPathLen);
        if (matHeader.RoughnessMetallicPathLen > 0)
            file.write(rmPath.data(), matHeader.RoughnessMetallicPathLen);
    }

    for (const auto& mesh : model.Meshes) {
        if (!mesh.Vertices.empty())
            file.write(reinterpret_cast<const char*>(mesh.Vertices.data()),
                       mesh.Vertices.size() * sizeof(mesh.Vertices[0]));

        if (!mesh.Indices.empty())
            file.write(reinterpret_cast<const char*>(mesh.Indices.data()),
                       mesh.Indices.size() * sizeof(mesh.Indices[0]));
    }

    return true;
}

std::optional<Model> AssetCache::LoadBinaryModel(const std::filesystem::path& filename) const {
    std::ifstream file(m_Directory / filename, std::ios::binary);
    if (!file)
        return std::nullopt;

    ModelHeader modelHeader{};
    file.read(reinterpret_cast<char*>(&modelHeader), sizeof(ModelHeader));

    if (!file || modelHeader.Magic[0] != 'G' || modelHeader.Magic[1] != 'E' || modelHeader.Magic[2] != 'M' ||
        modelHeader.Magic[3] != 'L') {
        return std::nullopt;
    }

    std::vector<MeshHeader> meshHeaders(modelHeader.MeshCount);
    file.read(reinterpret_cast<char*>(meshHeaders.data()), meshHeaders.size() * sizeof(MeshHeader));
    if (!file)
        return std::nullopt;

    Model model;
    model.Meshes.resize(modelHeader.MeshCount);
    model.Materials.resize(modelHeader.MaterialCount);

    for (size_t i = 0; i < modelHeader.MaterialCount; ++i) {
        MaterialHeader matHeader{};
        file.read(reinterpret_cast<char*>(&matHeader), sizeof(MaterialHeader));

        std::string albedoPath, normalPath, rmPath;

        if (matHeader.AlbedoPathLen > 0) {
            albedoPath.resize(matHeader.AlbedoPathLen);
            file.read(albedoPath.data(), matHeader.AlbedoPathLen);
        }
        if (matHeader.NormalPathLen > 0) {
            normalPath.resize(matHeader.NormalPathLen);
            file.read(normalPath.data(), matHeader.NormalPathLen);
        }
        if (matHeader.RoughnessMetallicPathLen > 0) {
            rmPath.resize(matHeader.RoughnessMetallicPathLen);
            file.read(rmPath.data(), matHeader.RoughnessMetallicPathLen);
        }

        auto& mat = model.Materials[i];
        if (!albedoPath.empty())
            mat.Albedo = std::make_shared<const Image>(albedoPath, matHeader.AlbedoIsSRGB != 0);
        if (!normalPath.empty())
            mat.Normal = std::make_shared<const Image>(normalPath, matHeader.NormalIsSRGB != 0);
        if (!rmPath.empty())
            mat.RoughnessMetallic = std::make_shared<const Image>(rmPath, matHeader.RoughnessMetallicIsSRGB != 0);
    }

    for (size_t i = 0; i < modelHeader.MeshCount; ++i) {
        const auto& meshHeader = meshHeaders[i];
        auto& mesh = model.Meshes[i];

        mesh.MaterialIndex = meshHeader.MaterialIndex;

        if (meshHeader.VertexCount > 0) {
            mesh.Vertices.resize(meshHeader.VertexCount);
            file.read(reinterpret_cast<char*>(mesh.Vertices.data()), meshHeader.VertexCount * sizeof(mesh.Vertices[0]));
        }

        if (meshHeader.IndexCount > 0) {
            mesh.Indices.resize(meshHeader.IndexCount);
            file.read(reinterpret_cast<char*>(mesh.Indices.data()), meshHeader.IndexCount * sizeof(mesh.Indices[0]));
        }
    }

    if (!file)
        return std::nullopt;

    return model;
}

} // namespace GEngine
