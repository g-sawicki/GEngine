#include "PCH.hpp"

#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace GEngine {

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

Image::Image(const std::filesystem::path& path) {
    m_Format = DetermineImageFormat(path);
    static constexpr uint8_t kChannels = 4;
    int width, height, channels;
    void* data = stbi_load(path.string().c_str(), &width, &height, &channels, kChannels);
    if (!data) {
        throw std::runtime_error("Failed to load image: " + path.string());
    }
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
    m_Channels = kChannels;
    m_Data = std::vector<uint8_t>(static_cast<uint8_t*>(data),
                                  static_cast<uint8_t*>(data) + (m_Width * m_Height * m_Channels));
    stbi_image_free(data);
}

DXGI_FORMAT Image::GetDXGIFormat() const noexcept {
    switch (m_Channels) {
    case 1:
        return DXGI_FORMAT_R8_UNORM;
    case 2:
        return DXGI_FORMAT_R8G8_UNORM;
    case 4:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
        // Unsupported channel count; fall back to RGBA8.
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

} // namespace GEngine
