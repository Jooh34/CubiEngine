#include "Graphics/MemoryAllocator.h"

FMemoryAllocator::FMemoryAllocator(ID3D12Device* const Device, IDXGIAdapter* const Adapter)
{
    // Create D3D12MA adapter.
    const D3D12MA::ALLOCATOR_DESC allocatorDesc = {
        .pDevice = Device,
        .pAdapter = Adapter,
    };

    ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &DmaAllocator));
}

FAllocation FMemoryAllocator::CreateBufferResourceAllocation(const FBufferCreationDesc& BufferCreationDesc, const FResourceCreationDesc& ResourceCreationDesc)
{
    FAllocation Allocation{};

    D3D12_RESOURCE_STATES ResourceState{};
    D3D12_HEAP_TYPE HeapType{};
    bool bCpuVisible = false;

    switch (BufferCreationDesc.Usage)
    {
        case EBufferUsage::UploadBuffer:
        case EBufferUsage::ConstantBuffer:
        {
            // GenericRead implies readable data from the GPU memory. Required resourceState for upload heaps.
            // UploadHeap : CPU writable access, GPU readable access.
            ResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            HeapType = D3D12_HEAP_TYPE_UPLOAD;
            bCpuVisible = true;
        }
        break;

        case EBufferUsage::IndexBuffer:
        case EBufferUsage::StructuredBuffer:
        {
            ResourceState = D3D12_RESOURCE_STATE_COMMON;
            HeapType = D3D12_HEAP_TYPE_DEFAULT;
            bCpuVisible = false;
        }
        break;
        case EBufferUsage::StructuredBufferUAV:
        {
            ResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            HeapType = D3D12_HEAP_TYPE_DEFAULT;
            bCpuVisible = false;
        }
        break;
    };

    const D3D12MA::ALLOCATION_DESC AllocationDesc = { .HeapType = HeapType };
    
    std::lock_guard<std::recursive_mutex> LockGuard(ResourceAllocationMutex);

    ThrowIfFailed(DmaAllocator->CreateResource(&AllocationDesc, &ResourceCreationDesc.ResourceDesc, ResourceState,
        nullptr, &Allocation.DmaAllocation, IID_PPV_ARGS(&Allocation.Resource)));

    if (bCpuVisible)
    {
        // Give the mapped pointer some value to hold.
        Allocation.MappedPointer = nullptr;
        ThrowIfFailed(Allocation.Resource->Map(0u, nullptr, &Allocation.MappedPointer.value()));
    }

    Allocation.Resource->SetName((LPCWSTR)BufferCreationDesc.Name.data());
    Allocation.DmaAllocation->SetResource(Allocation.Resource.Get());

    return Allocation;
}

FAllocation FMemoryAllocator::CreateTextureResourceAllocation(const FTextureCreationDesc& TextureCreationDesc, D3D12_RESOURCE_STATES& ResourceState, bool bUAVAllowed)
{
    ResourceState = D3D12_RESOURCE_STATE_COMMON;

    FAllocation Allocation{};

    DXGI_FORMAT format = TextureCreationDesc.Format;
    DXGI_FORMAT dsFormat{};

    switch (TextureCreationDesc.Format)
    {
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS: {
        dsFormat = DXGI_FORMAT_D32_FLOAT;
        format = DXGI_FORMAT_R32_FLOAT;
    }
    break;
    }

    FResourceCreationDesc ResourceCreationDesc = {
        .ResourceDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Alignment = 0u,
                .Width = TextureCreationDesc.Width,
                .Height = TextureCreationDesc.Height,
                .DepthOrArraySize = static_cast<UINT16>(TextureCreationDesc.DepthOrArraySize),
                .MipLevels = static_cast<UINT16>(TextureCreationDesc.MipLevels),
                .Format = format,
                .SampleDesc =
                    {
                        .Count = 1u,
                        .Quality = 0u,
                    },
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            },
    };

    constexpr D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12MA::ALLOCATION_DESC AllocationDesc = {
        .HeapType = heapType,
    };

    if (bUAVAllowed)
    {
		ResourceCreationDesc.ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    switch (TextureCreationDesc.Usage)
    {
    case ETextureUsage::DepthStencil: {
        ResourceCreationDesc.ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        ResourceCreationDesc.ResourceDesc.Format = dsFormat;
        AllocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
        ResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    break;

    case ETextureUsage::RenderTarget: {
        ResourceCreationDesc.ResourceDesc.Flags =
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        AllocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
        AllocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
        ResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    break;
    };

    std::optional<D3D12_CLEAR_VALUE> ClearValue{};
    if (TextureCreationDesc.Usage == ETextureUsage::RenderTarget)
    {
        ClearValue = { .Format = format, .Color = { 0.0f, 0.0f, 0.0f, 1.0f, } };
    }
    else if (TextureCreationDesc.Usage == ETextureUsage::DepthStencil)
    {
        constexpr D3D12_DEPTH_STENCIL_VALUE dsValue = {
            .Depth = 0.0f, // ReversedZ
            .Stencil = 0u,
        };
        ClearValue = { .Format = dsFormat, .DepthStencil = dsValue };
    }

    std::lock_guard<std::recursive_mutex> Guard(ResourceAllocationMutex);
    
    if (TextureCreationDesc.InitialState != D3D12_RESOURCE_STATE_COMMON)
    {
        ResourceState = TextureCreationDesc.InitialState;
    }

    ThrowIfFailed(
        DmaAllocator->CreateResource(&AllocationDesc, &ResourceCreationDesc.ResourceDesc, ResourceState,
            ClearValue.has_value() ? &ClearValue.value() : nullptr,
            &Allocation.DmaAllocation, IID_PPV_ARGS(&Allocation.Resource)));

    Allocation.Resource->SetName((LPCWSTR)TextureCreationDesc.Name.data());
    Allocation.DmaAllocation->SetResource(Allocation.Resource.Get());

    return Allocation;
}
