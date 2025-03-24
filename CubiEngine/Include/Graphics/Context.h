#pragma once

#include "Graphics/Resource.h"

class FContext
{
public:
    ID3D12GraphicsCommandList1* const GetCommandList() const
    {
        return CommandList.Get();
    }

    void CopyTextureRegion(
        const D3D12_TEXTURE_COPY_LOCATION* pDst,
        UINT DstX,
        UINT DstY,
        UINT DstZ,
        const D3D12_TEXTURE_COPY_LOCATION* pSrc,
        const D3D12_BOX* pSrcBox);

    virtual void Reset();
    
    void AddResourceBarrier(FTexture& Texture, const D3D12_RESOURCE_STATES NewState);
    void AddResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier);

    void AddUAVBarrier(FTexture& Texture);

    void ExecuteResourceBarriers();
    void BeginEvent(const char* Name);
    void EndEvent(const char* Name);

protected:
    void AddResourceBarrier(ID3D12Resource* const Resource, const D3D12_RESOURCE_STATES PreviousState, const D3D12_RESOURCE_STATES NewState);

    wrl::ComPtr<ID3D12GraphicsCommandList1> CommandList{};
    wrl::ComPtr<ID3D12CommandAllocator> CommandAllocator{};

    std::vector<CD3DX12_RESOURCE_BARRIER> ResourceBarriers;
};