#pragma once

#include "Core/Utility/Image.hpp"

#include <optional>

namespace GEngine {

struct Material {
    std::optional<Image> Diffuse;
    std::optional<Image> Specular;
};

} // namespace GEngine
