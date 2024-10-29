#pragma once

#include "Graphics/Resource.h"

class FMemoryAllocator
{
public:
    FMemoryAllocator(ID3D12Device* const Device, IDXGIAdapter* const Adapter);

    FAllocation CreateBufferResourceAllocation(const FBufferCreationDesc& BufferCreationDesc,
        const FResourceCreationDesc& ResourceCreationDesc);
    FAllocation CreateTextureResourceAllocation(const FTextureCreationDesc& TextureCreationDesc, D3D12_RESOURCE_STATES& ResourceState);

private:
    wrl::ComPtr<D3D12MA::Allocator> DmaAllocator{};
    std::recursive_mutex ResourceAllocationMutex{};
};