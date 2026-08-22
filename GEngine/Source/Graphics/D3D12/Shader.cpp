#include "PCH.hpp"

#include "Shader.hpp"

#include "Graphics/D3D12/D3D12Common.hpp"

#include <d3dcompiler.h>

namespace GEngine {

void Shader::Load(const std::filesystem::path& path) {
    ThrowIfFailed(D3DReadFileToBlob(path.c_str(), &m_Blob));
}

} // namespace GEngine
