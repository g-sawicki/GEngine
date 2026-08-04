#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <dxgiformat.h>

namespace GEngine {

enum class ImageFormat { PNG, JPEG, BMP, TGA, GIF, HDR };

class Image {
  public:
    Image(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<uint8_t>& GetData() const noexcept { return m_Data; }
    [[nodiscard]] uint32_t GetWidth() const noexcept { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }
    [[nodiscard]] uint8_t GetChannelCount() const noexcept { return m_Channels; }
    [[nodiscard]] ImageFormat GetFormat() const noexcept { return m_Format; }

    [[nodiscard]] DXGI_FORMAT GetDXGIFormat() const noexcept;

    [[nodiscard]] UINT64 GetRowPitch() const noexcept { return static_cast<UINT64>(m_Width) * m_Channels; }

  private:
    std::vector<uint8_t> m_Data;
    uint32_t m_Width{};
    uint32_t m_Height{};
    uint8_t m_Channels{};
    ImageFormat m_Format{};
};

} // namespace GEngine
