#include "PCH.hpp"

#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace GEngine {

static ImageFormat SniffFormat(const uint8_t* data, size_t size) {
    if (size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        return ImageFormat::PNG;
    if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8)
        return ImageFormat::JPEG;
    if (size >= 2 && data[0] == 'B' && data[1] == 'M')
        return ImageFormat::BMP;
    if (size >= 4 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F' && data[3] == '8')
        return ImageFormat::GIF;
    if (size >= 10 && std::memcmp(data, "#?RADIANCE", 10) == 0)
        return ImageFormat::HDR;
    return ImageFormat::PNG;
}

static ImageFormat DetermineImageFormat(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".png") {
        return ImageFormat::PNG;
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return ImageFormat::JPEG;
    } else if (extension == ".bmp") {
        return ImageFormat::BMP;
    } else if (extension == ".tga") {
        return ImageFormat::TGA;
    } else if (extension == ".gif") {
        return ImageFormat::GIF;
    } else if (extension == ".hdr") {
        return ImageFormat::HDR;
    } else {
        throw std::runtime_error("Unsupported image format: " + extension);
    }
}

Image::Image(const std::filesystem::path& path, bool isSRGB)
    : m_Path(path), m_Format(DetermineImageFormat(path)), m_IsSRGB(isSRGB) {
    static constexpr uint8_t kChannels = 4;
    int width, height, channels;
    void* data;
    if (m_Format == ImageFormat::HDR)
        data = stbi_loadf(path.string().c_str(), &width, &height, &channels, kChannels);
    else
        data = stbi_load(path.string().c_str(), &width, &height, &channels, kChannels);
    if (!data) {
        throw std::runtime_error("Failed to load image: " + path.string());
    }
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
    m_Channels = kChannels;
    m_Data = std::vector<uint8_t>(static_cast<uint8_t*>(data),
                                  static_cast<uint8_t*>(data) +
                                      (static_cast<size_t>(m_Width) * m_Height * GetBytesPerPixel()));
    stbi_image_free(data);
}

Image::Image(const void* data, size_t size, bool isSRGB)
    : m_Format(SniffFormat(static_cast<const uint8_t*>(data), size)), m_IsSRGB(isSRGB) {
    const auto* bytes = static_cast<const uint8_t*>(data);

    static constexpr uint8_t kChannels = 4;
    int width, height, channels;
    void* decoded;
    if (m_Format == ImageFormat::HDR)
        decoded = stbi_loadf_from_memory(bytes, static_cast<int>(size), &width, &height, &channels, kChannels);
    else
        decoded = stbi_load_from_memory(bytes, static_cast<int>(size), &width, &height, &channels, kChannels);
    if (!decoded) {
        throw std::runtime_error("Failed to decode in-memory image.");
    }
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
    m_Channels = kChannels;
    m_Data = std::vector<uint8_t>(static_cast<uint8_t*>(decoded),
                                  static_cast<uint8_t*>(decoded) +
                                      (static_cast<size_t>(m_Width) * m_Height * GetBytesPerPixel()));
    stbi_image_free(decoded);
}

Image Image::FromRawRGBA(const void* data, uint32_t width, uint32_t height, bool isSRGB) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    Image image;
    image.m_Width = width;
    image.m_Height = height;
    image.m_Channels = 4;
    image.m_Format = ImageFormat::PNG;
    image.m_IsSRGB = isSRGB;
    image.m_Data.assign(bytes, bytes + (static_cast<size_t>(width) * height * 4));
    return image;
}

DXGI_FORMAT Image::GetDXGIFormat() const noexcept {
    if (m_Format == ImageFormat::HDR)
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    switch (m_Channels) {
    case 1:
        return DXGI_FORMAT_R8_UNORM;
    case 2:
        return DXGI_FORMAT_R8G8_UNORM;
    case 4:
        return m_IsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
        // Unsupported channel count; fall back to RGBA8.
        return m_IsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

} // namespace GEngine
