#pragma once

#include "Graphics/Context.h"

class FCommandQueue
{
public:
    FCommandQueue(ID3D12Device5* const device, const D3D12_COMMAND_LIST_TYPE commandListType,
        const std::wstring_view name);

    ID3D12CommandQueue* const GetCommandQueue() const
    {
        return CommandQueue.Get();
    }

    uint64_t Signal();
    bool IsFenceComplete(const uint64_t InFenceValue) const;
    void WaitForFenceValue(const uint64_t InFenceValue);

    void ExecuteContext(FContext* Context);

    void Flush();

private:
    wrl::ComPtr<ID3D12CommandQueue> CommandQueue{};
    wrl::ComPtr<ID3D12Fence> Fence{};

    uint64_t CommandQueueFenceValue{};
};