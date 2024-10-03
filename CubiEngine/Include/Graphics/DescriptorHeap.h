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
        return static_cast<uint32_t> (
            (InDescriptorHandle.GpuDescriptorHandle.ptr - DescriptorHandleFromHeapStart.GpuDescriptorHandle.ptr) /
            DescriptorSize);
    }

    uint32_t GetCurrentDescriptorIndex() const { return GetDescriptorIndex(CurrentDescriptorHandle); }

    void OffsetDescriptor(FDescriptorHandle& InHandle, const uint32_t Offset = 1u) const;
    ID3D12DescriptorHeap* const GetDescriptorHeap() const { return DescriptorHeap.Get(); }

private:
    wrl::ComPtr<ID3D12DescriptorHeap> DescriptorHeap{};
    uint32_t DescriptorSize{};

    FDescriptorHandle DescriptorHandleFromHeapStart;
    FDescriptorHandle CurrentDescriptorHandle;
};
