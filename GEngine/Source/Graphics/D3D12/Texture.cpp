#include "PCH.hpp"

#include "Texture.hpp"

#include "CommandList.hpp"
#include "CommandQueue.hpp"
#include "Core/Utility/Common.hpp"
#include "Core/Utility/Image.hpp"
#include "D3D12Common.hpp"

namespace GEngine {

Texture::Texture(Device& device, CommandQueue& commandQueue, DescriptorHandle descriptorHandle, const TextureDesc& desc,
                 const Image& image)
    : m_Format(desc.Format) {
    assert(descriptorHandle.cpuHandle.ptr != 0);
    assert(descriptorHandle.gpuHandle.ptr != 0);

    const auto width = static_cast<UINT64>(desc.Width);
    const auto height = static_cast<UINT>(desc.Height);

    UINT bytesPerPixel = 4; // Default to RGBA
    if (desc.Format == DXGI_FORMAT_R8_UNORM)
        bytesPerPixel = 1;
    else if (desc.Format == DXGI_FORMAT_R8G8_UNORM)
        bytesPerPixel = 2;
    else if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
        bytesPerPixel = 4;

    const UINT64 srcRowPitch = image.GetRowPitch();
    const UINT64 expectedRowPitch = width * bytesPerPixel;
    const UINT64 alignedRowPitch = RoundUp<D3D12_TEXTURE_DATA_PITCH_ALIGNMENT>(expectedRowPitch);
    const UINT64 uploadSize = alignedRowPitch * height;

    // Create the default-heap texture resource.
    {
        const CD3DX12_RESOURCE_DESC resourceDesc{CD3DX12_RESOURCE_DESC::Tex2D(m_Format, width, height, 1, 1)};
        const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};

        ThrowIfFailed(device.Get()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                            IID_PPV_ARGS(&m_Resource)));
    }

    // Create the SRV.
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = m_Format,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {.MipLevels = 1},
        };
        device.Get()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, descriptorHandle.cpuHandle);
        m_SRV = descriptorHandle.gpuHandle;
    }

    // Create upload buffer and copy pixel data.
    const CD3DX12_HEAP_PROPERTIES uploadHeapProps{D3D12_HEAP_TYPE_UPLOAD};
    const CD3DX12_RESOURCE_DESC uploadDesc{CD3DX12_RESOURCE_DESC::Buffer(uploadSize)};

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    ThrowIfFailed(device.Get()->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        IID_PPV_ARGS(&uploadBuffer)));

    {
        void* mapped = nullptr;
        uploadBuffer->Map(0, nullptr, &mapped);
        auto* dst = static_cast<uint8_t*>(mapped);
        const auto* src = image.GetData().data();
        const uint8_t imageBytesPerPixel = image.GetChannelCount();

        // Handle channel expansion (3->4 bytes per pixel)
        if (imageBytesPerPixel == 3 && bytesPerPixel == 4) {
            for (UINT row = 0; row < height; ++row) {
                for (UINT col = 0; col < width; ++col) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = 0xFF;
                    dst += 4;
                    src += 3;
                }
                dst += alignedRowPitch - width * 4;
            }
        } else {
            // Direct copy for matching formats
            for (UINT row = 0; row < height; ++row) {
                std::memcpy(dst, src, srcRowPitch);
                dst += alignedRowPitch;
                src += srcRowPitch;
            }
        }
        uploadBuffer->Unmap(0, nullptr);
    }

    // Upload to GPU via the renderer's command queue.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadAllocator;
    ThrowIfFailed(device.Get()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadAllocator)));

    CommandList uploadCmdList{device, uploadAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT};
    uploadCmdList.Reset(uploadAllocator.Get());

    D3D12_TEXTURE_COPY_LOCATION dstLocation{
        .pResource = m_Resource.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };
    D3D12_TEXTURE_COPY_LOCATION srcLocation{
        .pResource = uploadBuffer.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint =
            {
                .Offset = 0,
                .Footprint =
                    {
                        .Format = m_Format,
                        .Width = static_cast<UINT>(width),
                        .Height = height,
                        .Depth = 1,
                        .RowPitch = static_cast<UINT>(alignedRowPitch),
                    },
            },
    };

    auto* cmdList = uploadCmdList.GetHandle();
    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    const CD3DX12_RESOURCE_BARRIER barrier{CD3DX12_RESOURCE_BARRIER::Transition(
        m_Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)};
    cmdList->ResourceBarrier(1, &barrier);

    uploadCmdList.Close();

    // Execute upload and wait for completion.
    {
        ID3D12CommandList* lists[] = {uploadCmdList.GetHandle()};
        commandQueue.GetHandle()->ExecuteCommandLists(1, lists);

        Microsoft::WRL::ComPtr<ID3D12Fence> uploadFence;
        ThrowIfFailed(device.Get()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence)));

        const HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        commandQueue.GetHandle()->Signal(uploadFence.Get(), 1);
        uploadFence->SetEventOnCompletion(1, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }
}

} // namespace GEngine
