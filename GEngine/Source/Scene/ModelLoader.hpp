#pragma once

#include "Core/Utility/Defines.hpp"
#include "Model.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <expected>
#include <filesystem>
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
    void ProcessNode(aiNode* node, const aiMatrix4x4& parentTransform);
    void ProcessMesh(aiMesh* mesh, const aiMatrix4x4& transform);
    int32_t ProcessMaterial(const aiMaterial* material);
    int32_t ProcessTexture(const aiMaterial* material, aiTextureType type, bool isSRGB);

    std::unordered_map<const aiMaterial*, int32_t> m_MaterialToIndexMap;
    std::unordered_map<const aiTexture*, int32_t> m_TextureEmbeddedToIndexMap;
    std::unordered_map<std::string, int32_t> m_TexturePathToIndexMap;
    const std::filesystem::path m_ModelPath{};
    const std::filesystem::path m_ModelDirectory{};
    Model m_Model{};
    const aiScene* m_Scene{};
    bool m_Loaded{};
};

} // namespace GEngine
