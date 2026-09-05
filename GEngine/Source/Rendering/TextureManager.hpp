#pragma once

#include "Core/Utility/Defines.hpp"
#include "Core/Utility/Image.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Scene/Model.hpp"

#include <vector>
#include <wrl/client.h>

namespace GEngine {

class TextureManager {
  public:
    explicit TextureManager(Device& device) : m_Device(&device) {}
    ~TextureManager();

    GE_NO_COPY_NO_MOVE(TextureManager);

    void Initialize(CommandList& copyCommandList);

    void ReleasePendingUploads() noexcept;

    std::unique_ptr<Texture> CreateTexture(const Image& image, bool isSRGB, CommandList& copyCommandList);

    [[nodiscard]] uint32_t GetDefaultAlbedoIndex() const noexcept { return m_DefaultAlbedo.GetSrvIndex(); }
    [[nodiscard]] uint32_t GetDefaultNormalIndex() const noexcept { return m_DefaultNormal.GetSrvIndex(); }
    [[nodiscard]] uint32_t GetDefaultRoughnessMetallicIndex() const noexcept {
        return m_DefaultRoughnessMetallic.GetSrvIndex();
    }

  private:
    void CreateDefaultTextures(CommandList& copyCommandList);

    Device* m_Device{};

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_PendingUploads;

    Texture m_DefaultAlbedo;
    Texture m_DefaultNormal;
    Texture m_DefaultRoughnessMetallic;
};

} // namespace GEngine
