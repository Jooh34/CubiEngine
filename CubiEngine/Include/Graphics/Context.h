#pragma once

#include "Graphics/Resource.h"

class FContext
{
public:
    ID3D12GraphicsCommandList1* const GetCommandList() const
    {
        return CommandList.Get();
    }

    void Reset() const;

    void AddResourceBarrier(FTexture& Texture, const D3D12_RESOURCE_STATES NewState);

    void ExecuteResourceBarriers();

protected:
    void AddResourceBarrier(ID3D12Resource* const Resource, const D3D12_RESOURCE_STATES PreviousState, const D3D12_RESOURCE_STATES NewState);

    wrl::ComPtr<ID3D12GraphicsCommandList1> CommandList{};
    wrl::ComPtr<ID3D12CommandAllocator> CommandAllocator{};

    std::vector<CD3DX12_RESOURCE_BARRIER> ResourceBarriers;
};