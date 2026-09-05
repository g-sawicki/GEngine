#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Device.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/Texture.hpp"
#include "Rendering/Components.hpp"
#include "Rendering/MeshBuffer.hpp"
#include "Rendering/TextureManager.hpp"
#include "Scene/Model.hpp"

#include <wrl/client.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace GEngine {

struct GPUModelHandle {
    uint32_t Id{};

    [[nodiscard]] bool IsValid() const noexcept { return Id != 0; }
    bool operator==(const GPUModelHandle& other) const noexcept { return Id == other.Id; }
};

struct GPUSubmesh {
    std::unique_ptr<MeshBuffer> Mesh{};
    MaterialGPU Material{};
};

class GPUResourceManager {
  public:
    GPUResourceManager(Device& device, CommandQueue& uploadQueue);

    GE_NO_COPY_NO_MOVE(GPUResourceManager);

    void BeginCopyPass();
    [[nodiscard]] GPUModelHandle StageModelToVRAM(const Model& cpuModel);
    void EndAndSubmitCopyPass();

    [[nodiscard]] std::unique_ptr<MeshBuffer> StageMeshBuffer(const Mesh& mesh);
    [[nodiscard]] std::unique_ptr<Texture> StageTexture(const Image& image, bool isSRGB);

    void RegisterModelHandle(ModelHandle cpuHandle, GPUModelHandle gpuHandle);
    [[nodiscard]] GPUModelHandle GetGPUHandle(ModelHandle cpuHandle) const;

    [[nodiscard]] uint32_t GetSubmeshCount(GPUModelHandle handle) const;
    [[nodiscard]] const GPUSubmesh& GetSubmesh(GPUModelHandle handle, uint32_t submeshIndex) const;

    void Shutdown();

    [[nodiscard]] TextureManager& GetTextureManager() noexcept { return m_Textures; }

  private:
    void StageBuffer(Buffer& destination, const void* data, UINT64 size);

    struct ModelRecord {
        std::vector<GPUSubmesh> Submeshes;
        std::vector<std::unique_ptr<Texture>> OwnedTextures;
    };

    Device& m_Device;
    CommandQueue& m_UploadQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CopyAllocator;
    std::unique_ptr<CommandList> m_CopyCommandList;
    std::unique_ptr<Fence> m_CopyFence;
    uint64_t m_CopyFenceValue{};
    bool m_CopyPassActive{};

    TextureManager m_Textures;

    std::vector<Buffer> m_StagingBuffers;
    std::unordered_map<uint32_t, ModelRecord> m_GPUModels;
    std::unordered_map<uint32_t, GPUModelHandle> m_ModelToGPU;
    uint32_t m_NextModelId{1u};
};

} // namespace GEngine
