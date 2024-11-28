#include "Graphics/DescriptorHeap.h"

FDescriptorHeap::FDescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
    const uint32_t descriptorCount, const std::wstring_view descriptorHeapName)
{
    const D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags = (descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
        descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    const D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
        .Type = descriptorHeapType,
        .NumDescriptors = descriptorCount,
        .Flags = descriptorHeapFlags,
        .NodeMask = 0u,
    };

    NumDescriptor = descriptorCount;

    ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&DescriptorHeap)));
    DescriptorHeap->SetName(descriptorHeapName.data());

    DescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    DescriptorHandleFromHeapStart = {
        .CpuDescriptorHandle = DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        .GpuDescriptorHandle = (descriptorHeapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                                   ? DescriptorHeap->GetGPUDescriptorHandleForHeapStart()
                                   : CD3DX12_GPU_DESCRIPTOR_HANDLE{},
        .DescriptorSize = DescriptorSize,
    };

    CurrentDescriptorHandle = DescriptorHandleFromHeapStart;
}

FDescriptorHandle FDescriptorHeap::GetDescriptorHandleFromIndex(const uint32_t Index) const
{
    FDescriptorHandle Handle = GetDescriptorHandleFromStart();
    OffsetDescriptor(Handle, Index);

    return std::move(Handle);
}

void FDescriptorHeap::OffsetDescriptor(FDescriptorHandle& InHandle, const uint32_t Offset) const
{
    InHandle.CpuDescriptorHandle.ptr += DescriptorSize * static_cast<unsigned long long>(Offset);
    InHandle.GpuDescriptorHandle.ptr += DescriptorSize * static_cast<unsigned long long>(Offset);
}

void FDescriptorHeap::OffsetCurrentHandle(const uint32_t Offset)
{
    OffsetDescriptor(CurrentDescriptorHandle, Offset);
}
