#include "Graphics/MemoryAllocator.h"

FMemoryAllocator::FMemoryAllocator(ID3D12Device* const Device, IDXGIAdapter* const Adapter)
{
    // Create D3D12MA adapter.
    const D3D12MA::ALLOCATOR_DESC allocatorDesc = {
        .pDevice = Device,
        .pAdapter = Adapter,
    };

    ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &DmaAllocator));
}

FAllocation FMemoryAllocator::CreateTextureResourceAllocation(const FTextureCreationDesc& textureCreationDesc)
{
    FAllocation Allocation{};

    return Allocation;
}
