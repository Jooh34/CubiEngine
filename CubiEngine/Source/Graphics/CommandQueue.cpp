#include "Graphics/CommandQueue.h"

FCommandQueue::FCommandQueue(ID3D12Device5* const device, const D3D12_COMMAND_LIST_TYPE commandListType,
    const std::wstring_view name)
    : CommandQueueFenceValue(0)
{
    // Create the command queue based on list type.
    const D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
        .Type = commandListType,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0u,
    };

    ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&CommandQueue)));
    CommandQueue->SetName(name.data());

    // Create the fence (used for synchronization of CPU and GPU).
    ThrowIfFailed(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    Fence->SetName(name.data());
}

uint64_t FCommandQueue::Signal()
{
    CommandQueueFenceValue++;
    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), CommandQueueFenceValue));

    return CommandQueueFenceValue;
}

bool FCommandQueue::IsFenceComplete(const uint64_t InFenceValue) const
{
    return Fence->GetCompletedValue() >= InFenceValue;
}

void FCommandQueue::WaitForFenceValue(const uint64_t InFenceValue)
{
    if (!IsFenceComplete(InFenceValue))
    {
        ThrowIfFailed(Fence->SetEventOnCompletion(InFenceValue, nullptr));
    }
}

void FCommandQueue::Execute(ID3D12CommandList& CmdList)
{
    CommandQueue->ExecuteCommandLists(1u, (ID3D12CommandList *const*)&CmdList);
}

void FCommandQueue::Flush()
{
    const uint64_t FenceValue = Signal();
    WaitForFenceValue(FenceValue);
}
