#pragma once

#include "Core/Utility/Defines.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <dxgiformat.h>

namespace GEngine {

class Image {
  public:
    Image() = default;
    Image(const std::filesystem::path& path);
    Image(const void* data, size_t size);

    static Image FromRGBA8(const void* data, uint32_t width, uint32_t height);

    GE_NO_COPY_DEFAULT_MOVE(Image)

    [[nodiscard]] const std::vector<uint8_t>& GetData() const noexcept { return m_Data; }
    [[nodiscard]] uint32_t GetWidth() const noexcept { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }
    [[nodiscard]] DXGI_FORMAT GetFormat() const noexcept { return m_Format; }

  private:
    void AdoptDecoded(void* decoded, uint32_t width, uint32_t height, bool isHdr);

    std::vector<uint8_t> m_Data;
    uint32_t m_Width{};
    uint32_t m_Height{};
    DXGI_FORMAT m_Format{};
};

} // namespace GEngine
