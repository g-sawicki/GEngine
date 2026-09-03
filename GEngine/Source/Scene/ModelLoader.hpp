#pragma once

#include "Core/Utility/Defines.hpp"
#include "Core/Utility/Image.hpp"
#include "Model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace GEngine {

class ModelLoader {
  public:
    explicit ModelLoader(const std::filesystem::path& filepath);

    GE_NO_COPY_NO_MOVE(ModelLoader);

    std::expected<Model, std::string> Load();

  private:
    struct LoadedMesh {
        Mesh Geometry;
        const aiMaterial* SourceMaterial{};
        Material Material;
    };

    void ProcessNode(aiNode* node, const aiMatrix4x4& parentTransform);
    LoadedMesh ProcessMesh(aiMesh* mesh, const aiMatrix4x4& transform);
    Material LoadMaterial(const aiMaterial* material);
    TextureSource LoadTexture(const aiMaterial* material, aiTextureType type, bool isSRGB = false);

    const std::filesystem::path m_ModelPath{};
    const std::filesystem::path m_ModelDirectory{};
    std::unordered_map<std::string, std::shared_ptr<const Image>> m_ImageCache;
    std::vector<LoadedMesh> m_LoadedMeshes;
    const aiScene* m_Scene{};
    bool m_Loaded{};
};

} // namespace GEngine
