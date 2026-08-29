#pragma once

#include "Core/Utility/Image.hpp"

#include <memory>

namespace GEngine {

struct Material {
    std::shared_ptr<const Image> Albedo;
    std::shared_ptr<const Image> Normal;
    std::shared_ptr<const Image> RoughnessMetallic;
};

} // namespace GEngine
