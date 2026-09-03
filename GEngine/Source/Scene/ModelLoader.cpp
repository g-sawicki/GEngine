#include "PCH.hpp"

#include "ModelLoader.hpp"

#include "Graphics/Vertex.hpp"

#include <cstdlib>

namespace GEngine {

ModelLoader::ModelLoader(const std::filesystem::path& filepath)
    : m_ModelPath(filepath), m_ModelDirectory(filepath.parent_path()) {}

std::expected<Model, std::string> ModelLoader::Load() {
    assert(!m_Loaded && "ModelLoader is one-shot: Load() may only be called once");
    m_Loaded = true;

    Assimp::Importer importer{};

    m_Scene = importer.ReadFile(m_ModelPath.string(), aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                                          aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);
    if (m_Scene == nullptr)
        return std::unexpected(std::string(importer.GetErrorString()));

    ProcessNode(m_Scene->mRootNode, aiMatrix4x4{});

    // Deduplicate materials (meshes may share a material) and assign each mesh its material index.
    Model model;
    std::unordered_map<const aiMaterial*, uint32_t> materialIndices;
    materialIndices.reserve(m_Scene->mNumMaterials);
    for (auto& loaded : m_LoadedMeshes) {
        const auto [it, inserted] =
            materialIndices.emplace(loaded.SourceMaterial, static_cast<uint32_t>(model.Materials.size()));
        if (inserted)
            model.Materials.push_back(std::move(loaded.Material));

        loaded.Geometry.MaterialIndex = it->second;
        model.Meshes.push_back(std::move(loaded.Geometry));
    }
    return model;
}

void ModelLoader::ProcessNode(aiNode* node, const aiMatrix4x4& parentTransform) {
    const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (uint32_t i{}; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = m_Scene->mMeshes[node->mMeshes[i]];
        m_LoadedMeshes.push_back(ProcessMesh(mesh, nodeTransform));
    }

    for (uint32_t i{}; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], nodeTransform);
    }
}

ModelLoader::LoadedMesh ModelLoader::ProcessMesh(aiMesh* mesh, const aiMatrix4x4& transform) {
    // Normals must be transformed by the inverse-transpose of the upper 3x3 so non-uniform scales stay correct.
    aiMatrix3x3 normalTransform(transform);
    normalTransform.Inverse().Transpose();

    LoadedMesh loaded{
        .SourceMaterial = m_Scene->mMaterials[mesh->mMaterialIndex],
        .Material = LoadMaterial(loaded.SourceMaterial),
    };

    loaded.Geometry.Vertices.reserve(mesh->mNumVertices);
    loaded.Geometry.Indices.reserve(mesh->mNumFaces * 3);

    for (uint32_t i{}; i < mesh->mNumVertices; ++i) {
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

    for (uint32_t i{}; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        for (uint32_t j{}; j < face.mNumIndices; ++j)
            loaded.Geometry.Indices.push_back(static_cast<uint32_t>(face.mIndices[j]));
    }

    return loaded;
}

Material ModelLoader::LoadMaterial(const aiMaterial* material) {
    Material mat;
    mat.Albedo = LoadTexture(material, aiTextureType_BASE_COLOR, /*isSRGB*/ true);
    mat.Normal = LoadTexture(material, aiTextureType_NORMALS);
    mat.RoughnessMetallic = LoadTexture(material, aiTextureType_GLTF_METALLIC_ROUGHNESS);
    return mat;
}

TextureSource ModelLoader::LoadTexture(const aiMaterial* material, aiTextureType type, bool isSRGB) {
    aiString texturePath;
    if (aiGetMaterialTexture(material, type, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) !=
        AI_SUCCESS)
        return {};

    const std::string pathString = texturePath.C_Str();
    if (pathString.empty())
        return {};

    TextureSource source;
    source.IsSRGB = isSRGB;

    std::string cacheKey;
    std::filesystem::path textureFile;
    if (pathString[0] == '*') {
        cacheKey = pathString;
    } else {
        textureFile = std::filesystem::path(pathString).is_absolute() ? std::filesystem::path(pathString)
                                                                      : m_ModelDirectory / pathString;
        cacheKey = textureFile.lexically_normal().string();
    }

    if (pathString[0] == '*') {
        // Embedded textures are referenced as "*<index>" into m_Scene->mTextures.
        const int index = std::atoi(pathString.c_str() + 1);
        if (index < 0 || static_cast<unsigned int>(index) >= m_Scene->mNumTextures)
            return {};

        const aiTexture* embedded = m_Scene->mTextures[index];
        try {
            if (embedded->mHeight != 0) {
                // Uncompressed RGBA pixels (mWidth * mHeight * 4 bytes).
                source.Decoded =
                    std::make_shared<Image>(Image::FromRGBA8(embedded->pcData, embedded->mWidth, embedded->mHeight));
            } else {
                // Compressed data (PNG/JPEG/...); mWidth bytes in pcData.
                source.Decoded = std::make_shared<Image>(embedded->pcData, embedded->mWidth);
            }
        } catch (const std::exception&) {
            return {};
        }
        return source;
    }

    source.Path = textureFile;
    if (const auto it = m_ImageCache.find(cacheKey); it != m_ImageCache.end()) {
        source.Decoded = it->second;
        return source;
    }

    try {
        source.Decoded = std::make_shared<Image>(textureFile);
        m_ImageCache.emplace(cacheKey, source.Decoded);
    } catch (const std::exception&) {
        return {};
    }
    return source;
}

} // namespace GEngine
