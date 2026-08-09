#pragma once

#include <vector>

#include "Rendering/Mesh.hpp"
#include "Scene/Material.hpp"

namespace GEngine {

struct Model {
    std::vector<Mesh> Meshes;
    std::vector<Material> Materials;
};

} // namespace GEngine
