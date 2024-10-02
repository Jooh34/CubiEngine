#pragma once

class FContext
{
public:
    ID3D12GraphicsCommandList1* const getCommandList() const
    {
        return CommandList.Get();
    }

    void Reset() const;

protected:
    wrl::ComPtr<ID3D12GraphicsCommandList1> CommandList{};
    wrl::ComPtr<ID3D12CommandAllocator> CommandAllocator{};
};