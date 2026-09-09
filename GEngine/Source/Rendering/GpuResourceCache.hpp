#pragma once

#include "Core/Utility/Defines.hpp"
#include "Rendering/Components.hpp"
#include "Scene/Model.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace GEngine {

class Device;
class Texture;
class UploadEngine;
class Image;

class GpuResourceCache {
  public:
    GpuResourceCache(Device& device, UploadEngine& uploads);

    GE_NO_COPY_NO_MOVE(GpuResourceCache)

    [[nodiscard]] MeshGPU StageMesh(const Mesh& mesh);
    void StageModel(const Model& model, uint32_t cpuModelId);

    [[nodiscard]] const GpuModel* FindModel(uint32_t cpuModelId) const noexcept;
    [[nodiscard]] bool HasModel(uint32_t cpuModelId) const noexcept;

    [[nodiscard]] uint32_t GetDefaultAlbedoIndex() const noexcept;
    [[nodiscard]] uint32_t GetDefaultNormalIndex() const noexcept;
    [[nodiscard]] uint32_t GetDefaultRoughnessMetallicIndex() const noexcept;

  private:
    void EnsureDefaultTextures();
    void StageMeshGeometry(MeshGPU& outGeometry, const Mesh& mesh);
    std::shared_ptr<Texture> CreateTextureFromSource(const TextureSource& source);
    std::shared_ptr<Texture> CreateTexture(const Image& image, bool isSRGB);
    uint32_t ResolveTextureIndex(int32_t sourceIndex, uint32_t fallbackIndex, const std::vector<TextureSource>& sources,
                                 std::unordered_map<int32_t, uint32_t>& inModelDedup);

    Device& m_Device;
    UploadEngine& m_UploadEngine;

    std::unordered_map<uint32_t, GpuModel> m_Models;
    // Keyed by "<path>#<srgb|linear>"; dedups texture uploads across models.
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;

    std::shared_ptr<Texture> m_DefaultAlbedo;
    std::shared_ptr<Texture> m_DefaultNormal;
    std::shared_ptr<Texture> m_DefaultRoughnessMetallic;
};

} // namespace GEngine
