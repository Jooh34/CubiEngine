#include "Graphics/CopyContext.h"
#include "Graphics/GraphicsDevice.h"

FCopyContext::FCopyContext(FGraphicsDevice* const Device)
{
    ThrowIfFailed(Device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
        IID_PPV_ARGS(&CommandAllocator)));

    ThrowIfFailed(Device->GetDevice()->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_COPY, CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&CommandList)));

    ThrowIfFailed(CommandList->Close());
}
