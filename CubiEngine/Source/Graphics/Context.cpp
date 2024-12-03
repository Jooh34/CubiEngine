#include "Graphics/Context.h"

void FContext::CopyTextureRegion(
    const D3D12_TEXTURE_COPY_LOCATION* pDst,
    UINT DstX, UINT DstY, UINT DstZ,
    const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox)
{
    CommandList->CopyTextureRegion(pDst, DstX, DstY, DstZ, pSrc, pSrcBox);
}

void FContext::Reset()
{
    ThrowIfFailed(CommandAllocator->Reset());
    ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));
}

void FContext::AddResourceBarrier(ID3D12Resource* const Resource, const D3D12_RESOURCE_STATES PreviousState,
    const D3D12_RESOURCE_STATES NewState)
{
    ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(Resource, PreviousState, NewState));
}

void FContext::AddResourceBarrier(FTexture& Texture, const D3D12_RESOURCE_STATES NewState)
{
    if (Texture.ResourceState == NewState) return;

    AddResourceBarrier(Texture.GetResource(), Texture.ResourceState, NewState);
    Texture.ResourceState = NewState;
}

void FContext::AddResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier)
{
    ResourceBarriers.emplace_back(Barrier);
}

void FContext::ExecuteResourceBarriers()
{
    if (ResourceBarriers.size() == 0) return;

    CommandList->ResourceBarrier(static_cast<UINT>(ResourceBarriers.size()), ResourceBarriers.data());
    ResourceBarriers.clear();
}