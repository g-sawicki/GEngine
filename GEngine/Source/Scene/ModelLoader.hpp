#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Core/Utility/Image.hpp"
#include "Model.hpp"

namespace GEngine {

class ModelLoader {
  public:
    using ImageCache = std::unordered_map<std::string, std::shared_ptr<const Image>>;

    static std::expected<Model, std::string> Load(const std::filesystem::path& filepath);

  private:
    struct LoadedMesh;

    static void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform,
                            const std::filesystem::path& modelDirectory, ImageCache& imageCache,
                            std::vector<LoadedMesh>& outMeshes);
    static LoadedMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform,
                                  const std::filesystem::path& modelDirectory, ImageCache& imageCache);
    static Material LoadMaterial(const aiMaterial* material, const aiScene* scene,
                                 const std::filesystem::path& modelDirectory, ImageCache& imageCache);
    static std::shared_ptr<const Image> LoadTexture(const aiMaterial* material, aiTextureType type,
                                                    const aiScene* scene, const std::filesystem::path& modelDirectory,
                                                    ImageCache& imageCache);
};

} // namespace GEngine
