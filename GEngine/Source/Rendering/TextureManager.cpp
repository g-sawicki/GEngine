#include "PCH.hpp"

#include "TextureManager.hpp"

#include "Graphics/D3D12/CommandList.hpp"

namespace GEngine {

TextureManager::~TextureManager() {
    m_DefaultNormal.Reset();
    m_DefaultAlbedo.Reset();
    m_DefaultRoughnessMetallic.Reset();
}

void TextureManager::Initialize(CommandList& copyCommandList) {
    CreateDefaultTextures(copyCommandList);
}

void TextureManager::CreateDefaultTextures(CommandList& copyCommandList) {
    static constexpr uint8_t kWhitePixel[]{255, 255, 255, 255};
    static constexpr uint8_t kFlatNormalPixel[]{128, 128, 255, 255};
    static constexpr uint8_t kDefaultRoughnessMetallicPixel[]{255, 255, 0, 255};

    const Image white = Image::FromRGBA8(kWhitePixel, 1, 1);
    const Image flatNormal = Image::FromRGBA8(kFlatNormalPixel, 1, 1);
    const Image roughnessMetallic = Image::FromRGBA8(kDefaultRoughnessMetallicPixel, 1, 1);

    const TextureDesc desc{
        .Width = 1, .Height = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .Usage = TextureUsage::ShaderResource};

    const SubresourceData whiteData{white.GetData().data()};
    const SubresourceData normalData{flatNormal.GetData().data()};
    const SubresourceData roughnessMetallicData{roughnessMetallic.GetData().data()};

    Microsoft::WRL::ComPtr<ID3D12Resource> staging;
    m_DefaultAlbedo.Create(*m_Device, desc, {&whiteData, 1}, &copyCommandList, &staging);
    m_PendingUploads.push_back(std::move(staging));

    m_DefaultNormal.Create(*m_Device, desc, {&normalData, 1}, &copyCommandList, &staging);
    m_PendingUploads.push_back(std::move(staging));

    m_DefaultRoughnessMetallic.Create(*m_Device, desc, {&roughnessMetallicData, 1}, &copyCommandList, &staging);
    m_PendingUploads.push_back(std::move(staging));
}

void TextureManager::ReleasePendingUploads() noexcept {
    m_PendingUploads.clear();
}

std::unique_ptr<Texture> TextureManager::CreateTexture(const Image& image, bool isSRGB, CommandList& copyCommandList) {
    DXGI_FORMAT format = image.GetFormat();
    if (isSRGB && format == DXGI_FORMAT_R8G8B8A8_UNORM)
        format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    TextureDesc desc{.Width = image.GetWidth(),
                     .Height = image.GetHeight(),
                     .Format = format,
                     .Usage = TextureUsage::ShaderResource};
    const SubresourceData data{image.GetData().data()};
    Microsoft::WRL::ComPtr<ID3D12Resource> staging;
    auto texture = std::make_unique<Texture>();
    texture->Create(*m_Device, desc, {&data, 1}, &copyCommandList, &staging);
    m_PendingUploads.push_back(std::move(staging));
    return texture;
}

} // namespace GEngine
