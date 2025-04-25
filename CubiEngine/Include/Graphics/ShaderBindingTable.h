#pragma once

#include "Graphics/ShaderBindingTableGenerator.h"

class FGraphicsDevice;

class FShaderBindingTable
{
public:
    FShaderBindingTable(const FGraphicsDevice* const GraphicsDevice, ComPtr<ID3D12StateObjectProperties>& RTStateObjectProps);

    D3D12_DISPATCH_RAYS_DESC CreateRayDesc(UINT Width, UINT Height);

    D3D12_GPU_VIRTUAL_ADDRESS GetSBTStorageAddress() const
    {
        return SbtResource->GetGPUVirtualAddress();
    }

private:
    ShaderBindingTableGenerator sbtGenerator;

    ComPtr<ID3D12Resource> SbtResource;
};
