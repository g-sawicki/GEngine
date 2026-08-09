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
        importer.ReadFile(filepath.string(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_FlipUVs |
                                                 aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                                                 aiProcess_GenSmoothNormals);
    if (pScene == nullptr)
        return std::unexpected(std::string(importer.GetErrorString()));

    const std::filesystem::path modelDirectory = filepath.parent_path();

    std::vector<LoadedMesh> loadedMeshes;
    ProcessNode(pScene->mRootNode, pScene, aiMatrix4x4{}, modelDirectory, loadedMeshes);

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
                              const std::filesystem::path& modelDirectory, std::vector<LoadedMesh>& outMeshes) {
    const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (UINT i{}; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        outMeshes.push_back(ProcessMesh(mesh, scene, nodeTransform, modelDirectory));
    }

    for (UINT i{}; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, nodeTransform, modelDirectory, outMeshes);
    }
}

ModelLoader::LoadedMesh ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform,
                                                 const std::filesystem::path& modelDirectory) {
    // Normals must be transformed by the inverse-transpose of the upper 3x3 so non-uniform scales stay correct.
    aiMatrix3x3 normalTransform(transform);
    normalTransform.Inverse().Transpose();

    LoadedMesh loaded;
    loaded.SourceMaterial = scene->mMaterials[mesh->mMaterialIndex];
    loaded.Material = LoadMaterial(loaded.SourceMaterial, scene, modelDirectory);

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
                                   const std::filesystem::path& modelDirectory) {
    Material mat;

    // glTF base-color maps to BASE_COLOR in recent Assimp; fall back to the classic DIFFUSE slot.
    mat.Diffuse = LoadTexture(material, aiTextureType_BASE_COLOR, scene, modelDirectory);
    if (!mat.Diffuse.has_value())
        mat.Diffuse = LoadTexture(material, aiTextureType_DIFFUSE, scene, modelDirectory);

    mat.Specular = LoadTexture(material, aiTextureType_SPECULAR, scene, modelDirectory);
    mat.Normal = LoadTexture(material, aiTextureType_NORMALS, scene, modelDirectory);
    return mat;
}

std::optional<Image> ModelLoader::LoadTexture(const aiMaterial* material, aiTextureType type, const aiScene* scene,
                                              const std::filesystem::path& modelDirectory) {
    aiString texturePath;
    if (aiGetMaterialTexture(material, type, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) !=
        AI_SUCCESS)
        return std::nullopt;

    const std::string pathString = texturePath.C_Str();
    if (pathString.empty())
        return std::nullopt;

    // Embedded textures are referenced as "*<index>" into scene->mTextures.
    if (pathString[0] == '*') {
        const int index = std::atoi(pathString.c_str() + 1);
        if (index < 0 || static_cast<unsigned int>(index) >= scene->mNumTextures)
            return std::nullopt;

        const aiTexture* embedded = scene->mTextures[index];
        try {
            if (embedded->mHeight != 0) {
                // Uncompressed RGBA pixels (mWidth * mHeight * 4 bytes).
                return Image::FromRawRGBA(embedded->pcData, embedded->mWidth, embedded->mHeight);
            }
            // Compressed data (PNG/JPEG/...); mWidth bytes in pcData.
            return Image(embedded->pcData, embedded->mWidth);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    // External file: resolve against the directory containing the model.
    const std::filesystem::path textureFile = std::filesystem::path(pathString).is_absolute()
                                                  ? std::filesystem::path(pathString)
                                                  : modelDirectory / pathString;
    try {
        return Image(textureFile);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace GEngine
