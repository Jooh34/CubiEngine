#include "Graphics/CopyContext.h"
#include "Graphics/D3D12DynamicRHI.h"

FCopyContext::FCopyContext()
{
    ThrowIfFailed(RHIGetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
        IID_PPV_ARGS(&D3D12CommandAllocator)));

    ThrowIfFailed(RHIGetDevice()->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_COPY, D3D12CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&D3D12CommandList)));

    ThrowIfFailed(D3D12CommandList->Close());
}
