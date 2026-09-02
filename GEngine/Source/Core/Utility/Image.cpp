#include "PCH.hpp"

#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

namespace GEngine {

Image::Image(const std::filesystem::path& path) {
    const bool isHdr = stbi_is_hdr(path.string().c_str()) != 0;

    int width, height, channels;
    void* data = isHdr ? static_cast<void*>(stbi_loadf(path.string().c_str(), &width, &height, &channels, 4))
                       : static_cast<void*>(stbi_load(path.string().c_str(), &width, &height, &channels, 4));
    if (!data)
        throw std::runtime_error(std::format("Failed to load image: {} ({})", path.string(), stbi_failure_reason()));

    AdoptDecoded(data, static_cast<uint32_t>(width), static_cast<uint32_t>(height), isHdr);
}

Image::Image(const void* data, size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    const bool isHdr = stbi_is_hdr_from_memory(bytes, static_cast<int>(size)) != 0;

    int width, height, channels;
    void* decoded =
        isHdr ? static_cast<void*>(stbi_loadf_from_memory(bytes, static_cast<int>(size), &width, &height, &channels, 4))
              : static_cast<void*>(stbi_load_from_memory(bytes, static_cast<int>(size), &width, &height, &channels, 4));
    if (!decoded)
        throw std::runtime_error(std::format("Failed to decode in-memory image ({})", stbi_failure_reason()));

    AdoptDecoded(decoded, static_cast<uint32_t>(width), static_cast<uint32_t>(height), isHdr);
}

void Image::AdoptDecoded(void* decoded, uint32_t width, uint32_t height, bool isHdr) {
    m_Width = width;
    m_Height = height;
    m_Format = isHdr ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;

    const size_t bytesPerPixel = isHdr ? sizeof(float) * 4 : 4;
    const auto* pixels = static_cast<const uint8_t*>(decoded);
    m_Data.assign(pixels, pixels + (static_cast<size_t>(m_Width) * m_Height * bytesPerPixel));
    stbi_image_free(decoded);
}

Image Image::FromRGBA8(const void* data, uint32_t width, uint32_t height) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    Image image;
    image.m_Width = width;
    image.m_Height = height;
    image.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    image.m_Data.assign(bytes, bytes + (static_cast<size_t>(width) * height * 4));
    return image;
}

} // namespace GEngine
