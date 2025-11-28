#pragma once

#include "Graphics/Context.h"

class FCommandQueue
{
public:
    FCommandQueue(ID3D12Device5* const device, const D3D12_COMMAND_LIST_TYPE commandListType,
        const std::wstring_view name);

    ID3D12CommandQueue* const GetD3D12CommandQueue() const
    {
        return D3D12CommandQueue.Get();
    }

    uint64_t Signal();
    bool IsFenceComplete(const uint64_t InFenceValue) const;
    void WaitForFenceValue(const uint64_t InFenceValue);

    void GetTimestampFrequency(UINT64* GpuFrequency);

    void ExecuteContext(FContext* Context);

    void Flush();

private:
    wrl::ComPtr<ID3D12CommandQueue> D3D12CommandQueue{};
    wrl::ComPtr<ID3D12Fence> Fence{};

    uint64_t CommandQueueFenceValue{};
};