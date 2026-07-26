#pragma once

namespace GEngine {

inline void ThrowIfFailed(HRESULT hr, const std::source_location& loc = std::source_location::current()) {
    if (FAILED(hr))
        throw std::runtime_error(
            std::format("{}:{} HRESULT: 0x{:08X}", loc.file_name(), loc.line(), static_cast<unsigned>(hr)));
}

} // namespace GEngine
