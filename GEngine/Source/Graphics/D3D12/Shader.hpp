#pragma once

#include <d3d12.h>
#include <d3dcommon.h>
#include <filesystem>
#include <wrl/client.h>

namespace GEngine {

class Shader {
  public:
    Shader() = default;
    explicit Shader(const std::filesystem::path& path) { Load(path); }

    void Load(const std::filesystem::path& path);

    [[nodiscard]] D3D12_SHADER_BYTECODE GetBytecode() const noexcept {
        return {m_Blob->GetBufferPointer(), m_Blob->GetBufferSize()};
    }

  private:
    Microsoft::WRL::ComPtr<ID3DBlob> m_Blob;
};

} // namespace GEngine
