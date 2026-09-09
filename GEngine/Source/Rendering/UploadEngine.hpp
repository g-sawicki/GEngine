#pragma once

#include "Core/Utility/Defines.hpp"
#include "Graphics/D3D12/Buffer.hpp"
#include "Graphics/D3D12/CommandList.hpp"
#include "Graphics/D3D12/CommandQueue.hpp"
#include "Graphics/D3D12/Fence.hpp"
#include "Graphics/D3D12/Texture.hpp"

#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace GEngine {

class UploadEngine {
  public:
    explicit UploadEngine(Device& device);
    ~UploadEngine();

    GE_NO_COPY_NO_MOVE(UploadEngine);

    void Begin();
    void Submit(bool wait = true);

    void UploadTexture(Texture& dst, std::span<const SubresourceData> subresources);
    void UploadBuffer(Buffer& dst, const void* data, UINT64 size);

  private:
    struct StagingAllocation {
        std::byte* Cpu{};
        ID3D12Resource* Resource{};
        UINT64 Offset{};
    };

    StagingAllocation AllocateStaging(UINT64 size);

    struct StagingChunk {
        std::unique_ptr<Buffer> Buffer;
        UINT64 Used{};
    };

    Device& m_Device;
    std::unique_ptr<CommandQueue> m_Queue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator;
    std::unique_ptr<CommandList> m_CommandList;
    std::unique_ptr<Fence> m_Fence;
    bool m_BatchActive{};

    std::vector<StagingChunk> m_StagingChunks;
};

} // namespace GEngine
