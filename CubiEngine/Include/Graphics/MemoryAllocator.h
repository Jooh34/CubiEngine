#pragma once

#include "Graphics/Resource.h"

class FMemoryAllocator
{
public:
    FMemoryAllocator(ID3D12Device* const Device, IDXGIAdapter* const Adapter);

    FAllocation CreateTextureResourceAllocation(const FTextureCreationDesc& textureCreationDesc);

private:
    wrl::ComPtr<D3D12MA::Allocator> DmaAllocator{};
};