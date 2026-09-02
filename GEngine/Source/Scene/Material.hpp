#pragma once

#include "Core/Utility/Image.hpp"

#include <filesystem>
#include <memory>

namespace GEngine {

struct TextureSource {
    std::filesystem::path Path{};
    bool IsSRGB{};
    std::shared_ptr<const Image> Decoded;
};

struct Material {
    TextureSource Albedo;
    TextureSource Normal;
    TextureSource RoughnessMetallic;
};

} // namespace GEngine
