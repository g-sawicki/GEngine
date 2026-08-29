#include "PCH.hpp"

#include "ModelLoader.hpp"

#include "Graphics/Vertex.hpp"

#include <cstdlib>
#include <unordered_map>

namespace GEngine {

struct ModelLoader::LoadedMesh {
    Mesh Geometry;
    const aiMaterial* SourceMaterial{}; // used to deduplicate materials across meshes
    Material Material;
};

std::expected<Model, std::string> ModelLoader::Load(const std::filesystem::path& filepath) {
    Assimp::Importer importer;

    const aiScene* pScene =
        importer.ReadFile(filepath.string(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                                 aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);
    if (pScene == nullptr)
        return std::unexpected(std::string(importer.GetErrorString()));

    const std::filesystem::path modelDirectory = filepath.parent_path();

    ImageCache imageCache;
    std::vector<LoadedMesh> loadedMeshes;
    ProcessNode(pScene->mRootNode, pScene, aiMatrix4x4{}, modelDirectory, imageCache, loadedMeshes);

    // Deduplicate materials (meshes may share a material) and assign each mesh its material index.
    Model model;
    std::unordered_map<const aiMaterial*, uint32_t> materialIndices;
    materialIndices.reserve(pScene->mNumMaterials);
    for (auto& loaded : loadedMeshes) {
        const auto [it, inserted] =
            materialIndices.emplace(loaded.SourceMaterial, static_cast<uint32_t>(model.Materials.size()));
        if (inserted)
            model.Materials.push_back(std::move(loaded.Material));

        loaded.Geometry.MaterialIndex = it->second;
        model.Meshes.push_back(std::move(loaded.Geometry));
    }
    return model;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform,
                              const std::filesystem::path& modelDirectory, ModelLoader::ImageCache& imageCache,
                              std::vector<LoadedMesh>& outMeshes) {
    const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (UINT i{}; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        outMeshes.push_back(ProcessMesh(mesh, scene, nodeTransform, modelDirectory, imageCache));
    }

    for (UINT i{}; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, nodeTransform, modelDirectory, imageCache, outMeshes);
    }
}

ModelLoader::LoadedMesh ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform,
                                                 const std::filesystem::path& modelDirectory,
                                                 ModelLoader::ImageCache& imageCache) {
    // Normals must be transformed by the inverse-transpose of the upper 3x3 so non-uniform scales stay correct.
    aiMatrix3x3 normalTransform(transform);
    normalTransform.Inverse().Transpose();

    LoadedMesh loaded;
    loaded.SourceMaterial = scene->mMaterials[mesh->mMaterialIndex];
    loaded.Material = LoadMaterial(loaded.SourceMaterial, scene, modelDirectory, imageCache);

    loaded.Geometry.Vertices.reserve(mesh->mNumVertices);
    loaded.Geometry.Indices.reserve(mesh->mNumFaces * 3);

    for (UINT i{}; i < mesh->mNumVertices; ++i) {
        Vertex vertex{};

        aiVector3D position = mesh->mVertices[i];
        position *= transform;
        vertex.Position = {position.x, position.y, position.z, 1.0f};

        if (mesh->mNormals) {
            aiVector3D normal = mesh->mNormals[i];
            normal *= normalTransform;
            normal.Normalize();
            vertex.Normal = {normal.x, normal.y, normal.z};
        }

        if (mesh->mTangents) {
            aiVector3D tangent = mesh->mTangents[i];
            vertex.Tangent = {tangent.x, tangent.y, tangent.z};
        }

        if (mesh->mTextureCoords[0]) {
            vertex.UV = {static_cast<float>(mesh->mTextureCoords[0][i].x),
                         static_cast<float>(mesh->mTextureCoords[0][i].y)};
        }

        loaded.Geometry.Vertices.push_back(vertex);
    }

    for (UINT i{}; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        for (UINT j{}; j < face.mNumIndices; ++j)
            loaded.Geometry.Indices.push_back(static_cast<uint32_t>(face.mIndices[j]));
    }

    return loaded;
}

Material ModelLoader::LoadMaterial(const aiMaterial* material, const aiScene* scene,
                                   const std::filesystem::path& modelDirectory, ModelLoader::ImageCache& imageCache) {
    Material mat;
    mat.Albedo = LoadTexture(material, aiTextureType_BASE_COLOR, scene, modelDirectory, imageCache);
    mat.Normal = LoadTexture(material, aiTextureType_NORMALS, scene, modelDirectory, imageCache);
    mat.RoughnessMetallic =
        LoadTexture(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, scene, modelDirectory, imageCache);
    return mat;
}

std::shared_ptr<const Image> ModelLoader::LoadTexture(const aiMaterial* material, aiTextureType type,
                                                      const aiScene* scene, const std::filesystem::path& modelDirectory,
                                                      ModelLoader::ImageCache& imageCache) {
    aiString texturePath;
    if (aiGetMaterialTexture(material, type, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) !=
        AI_SUCCESS)
        return nullptr;

    const std::string pathString = texturePath.C_Str();
    if (pathString.empty())
        return nullptr;

    std::string cacheKey;
    std::filesystem::path textureFile;
    if (pathString[0] == '*') {
        cacheKey = pathString;
    } else {
        textureFile = std::filesystem::path(pathString).is_absolute() ? std::filesystem::path(pathString)
                                                                      : modelDirectory / pathString;
        cacheKey = textureFile.lexically_normal().string();
    }

    if (const auto it = imageCache.find(cacheKey); it != imageCache.end())
        return it->second;

    try {
        std::shared_ptr<const Image> image;
        if (pathString[0] == '*') {
            // Embedded textures are referenced as "*<index>" into scene->mTextures.
            const int index = std::atoi(pathString.c_str() + 1);
            if (index < 0 || static_cast<unsigned int>(index) >= scene->mNumTextures)
                return nullptr;

            const aiTexture* embedded = scene->mTextures[index];
            if (embedded->mHeight != 0) {
                // Uncompressed RGBA pixels (mWidth * mHeight * 4 bytes).
                image =
                    std::make_shared<Image>(Image::FromRawRGBA(embedded->pcData, embedded->mWidth, embedded->mHeight));
            } else {
                // Compressed data (PNG/JPEG/...); mWidth bytes in pcData.
                image = std::make_shared<Image>(Image(embedded->pcData, embedded->mWidth));
            }
        } else {
            // External file: resolve against the directory containing the model.
            image = std::make_shared<Image>(textureFile);
        }
        imageCache.emplace(cacheKey, image);
        return image;
    } catch (const std::exception&) {
        return nullptr;
    }
}

} // namespace GEngine
