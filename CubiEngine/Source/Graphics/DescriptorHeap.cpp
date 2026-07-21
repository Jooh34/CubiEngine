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

    ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&D3D12DescriptorHeap)));
    D3D12DescriptorHeap->SetName(descriptorHeapName.data());

    DescriptorSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    DescriptorHandleFromHeapStart = {
        .CpuDescriptorHandle = D3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        .GpuDescriptorHandle = (descriptorHeapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                                   ? D3D12DescriptorHeap->GetGPUDescriptorHandleForHeapStart()
                                   : CD3DX12_GPU_DESCRIPTOR_HANDLE{},
        .DescriptorSize = DescriptorSize,
    };

    AllocatedDescriptors.resize(NumDescriptor, false);
}

FDescriptorHandle FDescriptorHeap::GetDescriptorHandleFromIndex(const uint32_t Index) const
{
    if (Index >= NumDescriptor)
    {
        FatalError("Descriptor index is out of range.");
    }

    FDescriptorHandle Handle = GetDescriptorHandleFromStart();
    OffsetDescriptor(Handle, Index);

    return Handle;
}

uint32_t FDescriptorHeap::AllocateDescriptor()
{
    std::scoped_lock Lock(AllocationMutex);

    uint32_t Index{};
    if (!FreeDescriptorIndices.empty())
    {
        Index = FreeDescriptorIndices.back();
        FreeDescriptorIndices.pop_back();
    }
    else
    {
        if (NextDescriptorIndex >= NumDescriptor)
        {
            FatalError("Descriptor heap capacity exceeded.");
        }
        Index = NextDescriptorIndex++;
    }

    assert(!AllocatedDescriptors[Index]);
    AllocatedDescriptors[Index] = true;
    return Index;
}

void FDescriptorHeap::FreeDescriptor(uint32_t Index)
{
    std::scoped_lock Lock(AllocationMutex);
    if (Index >= NumDescriptor || !AllocatedDescriptors[Index])
    {
        assert(false && "Attempted to free an invalid descriptor index.");
        return;
    }

    AllocatedDescriptors[Index] = false;
    FreeDescriptorIndices.push_back(Index);
}

void FDescriptorHeap::OffsetDescriptor(FDescriptorHandle& InHandle, const uint32_t Offset) const
{
    InHandle.CpuDescriptorHandle.ptr += DescriptorSize * static_cast<unsigned long long>(Offset);
    InHandle.GpuDescriptorHandle.ptr += DescriptorSize * static_cast<unsigned long long>(Offset);
}
