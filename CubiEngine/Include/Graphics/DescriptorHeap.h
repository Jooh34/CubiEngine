#pragma once


class FDescriptorHeap
{
public:
    FDescriptorHeap(ID3D12Device* const device, const D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType,
        const uint32_t descriptorCount, const std::wstring_view descriptorHeapName);
};
