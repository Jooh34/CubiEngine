#include "Graphics/Context.h"
#include "WinPixEventRuntime/pix3.h"

void FContext::CopyTextureRegion(
    const D3D12_TEXTURE_COPY_LOCATION* pDst,
    UINT DstX, UINT DstY, UINT DstZ,
    const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox)
{
    D3D12CommandList->CopyTextureRegion(pDst, DstX, DstY, DstZ, pSrc, pSrcBox);
}

void FContext::Reset()
{
    ThrowIfFailed(D3D12CommandAllocator->Reset());
    ThrowIfFailed(D3D12CommandList->Reset(D3D12CommandAllocator.Get(), nullptr));
}

void FContext::AddResourceBarrier(ID3D12Resource* const Resource, const D3D12_RESOURCE_STATES PreviousState,
    const D3D12_RESOURCE_STATES NewState)
{
    ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource, PreviousState, NewState));
}

void FContext::AddResourceBarrier(FTexture* Texture, const D3D12_RESOURCE_STATES NewState)
{
    if (Texture->ResourceState == NewState) return;

    AddResourceBarrier(Texture->GetResource(), Texture->ResourceState, NewState);
    Texture->ResourceState = NewState;
}

void FContext::AddResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier)
{
    ResourceBarriers.emplace_back(Barrier);
}

void FContext::AddUAVBarrier(FTexture* Texture)
{
    ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(Texture->GetResource()));
}

void FContext::AddUAVBarrier(FBuffer& Buffer)
{
    ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(Buffer.Allocation.Resource.Get()));
}

void FContext::ExecuteResourceBarriers()
{
    if (ResourceBarriers.size() == 0) return;

    D3D12CommandList->ResourceBarrier(static_cast<UINT>(ResourceBarriers.size()), ResourceBarriers.data());
    ResourceBarriers.clear();
}

void FContext::BeginEvent(const char* Name)
{
    PIXBeginEvent(D3D12CommandList.Get(), PIX_COLOR(255, 255, 255), Name);
}

void FContext::EndEvent(const char* Name)
{
    PIXEndEvent(D3D12CommandList.Get());
}
