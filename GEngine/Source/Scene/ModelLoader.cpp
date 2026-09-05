#include "PCH.hpp"

#include "ModelLoader.hpp"

#include <cstdlib>
#include <utility>

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

    return std::move(m_Model);
}

void ModelLoader::ProcessNode(aiNode* node, const aiMatrix4x4& parentTransform) {
    const aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (uint32_t i{}; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = m_Scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, nodeTransform);
    }

    for (uint32_t i{}; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], nodeTransform);
    }
}

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiMatrix4x4& transform) {
    // Normals must be transformed by the inverse-transpose of the upper 3x3 so non-uniform scales stay correct.
    aiMatrix3x3 normalTransform(transform);
    normalTransform.Inverse().Transpose();

    Mesh& modelMesh = m_Model.Meshes.emplace_back();
    modelMesh.MaterialIndex = ProcessMaterial(m_Scene->mMaterials[mesh->mMaterialIndex]);

    // Vertices
    modelMesh.Vertices.reserve(mesh->mNumVertices);
    for (uint32_t i{}; i < mesh->mNumVertices; ++i) {
        Vertex& vertex = modelMesh.Vertices.emplace_back();

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
    }

    // Indices
    modelMesh.Indices.reserve(mesh->mNumFaces * 3);
    for (uint32_t i{}; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        for (uint32_t j{}; j < face.mNumIndices; ++j)
            modelMesh.Indices.push_back(static_cast<uint32_t>(face.mIndices[j]));
    }
}

int32_t ModelLoader::ProcessMaterial(const aiMaterial* material) {
    if (auto it = m_MaterialToIndexMap.find(material); it != m_MaterialToIndexMap.end())
        return it->second;

    int32_t materialIndex = static_cast<int32_t>(m_Model.Materials.size());
    m_Model.Materials.emplace_back(ProcessTexture(material, aiTextureType_BASE_COLOR, /*isSRGB*/ true),
                                   ProcessTexture(material, aiTextureType_NORMALS, /*isSRGB*/ false),
                                   ProcessTexture(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, /*isSRGB*/ false));
    m_MaterialToIndexMap[material] = materialIndex;
    return materialIndex;
}

int32_t ModelLoader::ProcessTexture(const aiMaterial* material, aiTextureType type, bool isSRGB) {
    aiString texturePath;
    if (aiGetMaterialTexture(material, type, 0, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr) !=
            AI_SUCCESS ||
        texturePath.length == 0)
        return -1;

    const std::string pathString = texturePath.C_Str();

    int32_t textureIndex = static_cast<int32_t>(m_Model.Textures.size());
    if (pathString[0] == '*') {
        // Embedded texture
        const int index = std::atoi(pathString.c_str() + 1);
        if (index < 0 || static_cast<unsigned int>(index) >= m_Scene->mNumTextures)
            return -1;

        const aiTexture* embedded = m_Scene->mTextures[index];
        if (auto it = m_TextureEmbeddedToIndexMap.find(embedded); it != m_TextureEmbeddedToIndexMap.end())
            return it->second;

        if (embedded->mHeight != 0) {
            // Uncompressed RGBA pixels
            const auto* pixels = reinterpret_cast<const uint8_t*>(embedded->pcData);
            const size_t pixelBytes = static_cast<size_t>(embedded->mWidth) * embedded->mHeight * 4;
            m_Model.Textures.push_back(
                TextureEmbedded{.Buffer = std::vector<uint8_t>(pixels, pixels + pixelBytes), .IsSRGB = isSRGB});
        } else {
            // Compressed data (PNG/JPEG/...)
            const auto* bytes = reinterpret_cast<const uint8_t*>(embedded->pcData);
            m_Model.Textures.push_back(
                TextureEmbedded{.Buffer = std::vector<uint8_t>(bytes, bytes + embedded->mWidth), .IsSRGB = isSRGB});
        }
        m_TextureEmbeddedToIndexMap[embedded] = textureIndex;
    } else {
        // Texture path
        if (auto it = m_TexturePathToIndexMap.find(pathString); it != m_TexturePathToIndexMap.end())
            return it->second;

        std::filesystem::path textureFile{pathString};
        if (!textureFile.is_absolute())
            textureFile = m_ModelDirectory / textureFile;

        m_Model.Textures.push_back(TexturePath{.Path = std::move(textureFile), .IsSRGB = isSRGB});
        m_TexturePathToIndexMap[pathString] = textureIndex;
    }

    return textureIndex;
}

} // namespace GEngine
