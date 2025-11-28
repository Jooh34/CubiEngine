#pragma once

struct FDescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle{};

    uint32_t DescriptorSize{};

    void offset()
    {
        CpuDescriptorHandle.ptr += DescriptorSize;
        GpuDescriptorHandle.ptr += DescriptorSize;
    }
};

class FDescriptorHeap
{
public:
    FDescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
        const uint32_t descriptorCount, const std::wstring_view descriptorHeapName);

    FDescriptorHandle GetDescriptorHandleFromStart() const { return DescriptorHandleFromHeapStart; }
    FDescriptorHandle GetDescriptorHandleFromIndex(const uint32_t Index) const;
    uint32_t GetDescriptorIndex(const FDescriptorHandle& InDescriptorHandle) const
    {
        uint32_t i = static_cast<uint32_t> (
            (InDescriptorHandle.GpuDescriptorHandle.ptr - DescriptorHandleFromHeapStart.GpuDescriptorHandle.ptr) /
            DescriptorSize);

        assert(i < NumDescriptor);

        return i;
    }

    uint32_t GetCurrentDescriptorIndex() const { return GetDescriptorIndex(CurrentDescriptorHandle); }
    FDescriptorHandle GetCurrentDescriptorHandle() const { return CurrentDescriptorHandle; }

    void OffsetDescriptor(FDescriptorHandle& InHandle, const uint32_t Offset = 1u) const;
    void OffsetCurrentHandle(const uint32_t Offset = 1u);

    ID3D12DescriptorHeap* const GetD3D12DescriptorHeap() const { return D3D12DescriptorHeap.Get(); }

private:
    uint32_t NumDescriptor;

    wrl::ComPtr<ID3D12DescriptorHeap> D3D12DescriptorHeap{};
    uint32_t DescriptorSize{};

    FDescriptorHandle DescriptorHandleFromHeapStart;
    FDescriptorHandle CurrentDescriptorHandle;
};
