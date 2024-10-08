#include "Graphics/GraphicsDevice.h"
#include "Graphics/PipelineState.h"
#include "Graphics/MemoryAllocator.h"
#include "Graphics/CopyContext.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"

FGraphicsDevice::FGraphicsDevice(const int Width, const int Height,
    const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle)
    : SwapchainFormat(SwapchainFormat), WindowHandle(WindowHandle)
{
    InitDeviceResources();
    InitSwapchainResources(Width, Height);
}

FGraphicsDevice::~FGraphicsDevice()
{
    DirectCommandQueue->Flush(); // flush GPU works
}

FSampler FGraphicsDevice::CreateSampler(const FSamplerCreationDesc& Desc) const
{
    FSampler Sampler{};

    Sampler.SamplerIndex = SamplerDescriptorHeap->GetCurrentDescriptorIndex();
    FDescriptorHandle Handle = SamplerDescriptorHeap->GetCurrentDescriptorHandle();

    Device->CreateSampler(&Desc.SamplerDesc, Handle.CpuDescriptorHandle);

    SamplerDescriptorHeap->OffsetCurrentHandle();

    return Sampler;
}

FTexture FGraphicsDevice::CreateTexture(const FTextureCreationDesc& TextureCreationDesc, const void* Data) const
{
    DXGI_FORMAT format = TextureCreationDesc.Format;
    DXGI_FORMAT dsFormat{};

    if (format == DXGI_FORMAT_R32_FLOAT
        || format == DXGI_FORMAT_D32_FLOAT
        || format == DXGI_FORMAT_R32_TYPELESS)
    {
        dsFormat = DXGI_FORMAT_D32_FLOAT;
        format = DXGI_FORMAT_R32_FLOAT;
    }
    else if (format == DXGI_FORMAT_D24_UNORM_S8_UINT)
    {
        FatalError("Currently, the renderer does not support depth format of the type D24_S8_UINT. Please use one of the X32 types.");
    }

    uint32_t Width, Height;
    void* TextureData{ (void*)Data };

    if (TextureCreationDesc.Usage == ETextureUsage::TextureFromData)
    {
        Width = TextureCreationDesc.Width;
        Height = TextureCreationDesc.Height;
    }
    // TODO: handle another usage
    
    FTexture Texture{};

    // GPU only memory
    Texture.Allocation = MemoryAllocator->CreateTextureResourceAllocation(TextureCreationDesc);
    Texture.Width = Width;
    Texture.Height = Height;
    
    if (TextureData) // Upload Texture Buffer
    {
        // Create upload buffer.
        const FBufferCreationDesc UploadBufferCreationDesc = {
            .Usage = EBufferUsage::UploadBuffer,
            .Name = "Upload buffer - " + std::string(TextureCreationDesc.Name),
        };

        const UINT64 UploadBufferSize = GetRequiredIntermediateSize(Texture.Allocation.Resource.Get(), 0, 1);
        const FResourceCreationDesc ResourceCreationDesc = FResourceCreationDesc::CreateBufferResourceCreationDesc(UploadBufferSize);

        FAllocation UploadAllocation =
            MemoryAllocator->CreateBufferResourceAllocation(UploadBufferCreationDesc, ResourceCreationDesc);

        D3D12_SUBRESOURCE_DATA TextureSubresourceData{};

        if (TextureCreationDesc.Usage == ETextureUsage::HDRTextureFromPath)
        {
        }
        else // TexureUsage:: TextureFromPath or TextureFromData (non HDR).
        {
            TextureSubresourceData = {
                .pData = TextureData,
                .RowPitch = Width * TextureCreationDesc.BytesPerPixel,
                .SlicePitch = Width * Height * TextureCreationDesc.BytesPerPixel,
            };
        }

        // Use the copy context and execute UpdateSubresources functions on the copy command queue.
        CopyContext->Reset();

        UpdateSubresources(CopyContext->GetCommandList(), Texture.Allocation.Resource.Get(),
            UploadAllocation.Resource.Get(), 0u, 0u, 1u, &TextureSubresourceData);

        CopyCommandQueue->ExecuteContext(CopyContext.get());
        CopyCommandQueue->Flush();

        UploadAllocation.Reset();
    }
    
    {
        // create SRV
        FSrvCreationDesc SrvCreationDesc{};

        if (TextureCreationDesc.DepthOrArraySize == 1u)
        {
            SrvCreationDesc = {
                .SrvDesc = {
                    .Format = format,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {
                        .MostDetailedMip = 0u,
                        .MipLevels = 1, // Todo:Mipmap
                    }
                }
            };
        }
        else if (TextureCreationDesc.DepthOrArraySize == 6u)
        {
            // TODO
        }

        Texture.SrvIndex = CreateSrv(SrvCreationDesc, Texture.Allocation.Resource.Get());
    }

    {
        // Create DSV (if applicable).
        if (TextureCreationDesc.Usage == ETextureUsage::DepthStencil)
        {
            const FDsvCreationDesc dsvCreationDesc = {
                .DsvDesc =
                    {
                        .Format = dsFormat,
                        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
                        .Flags = D3D12_DSV_FLAG_NONE,
                        .Texture2D{
                            .MipSlice = 0u,
                        },
                    },
            };

            Texture.DsvIndex = CreateDsv(dsvCreationDesc, Texture.Allocation.Resource.Get());
        }

        // Create RTV (if applicable).
        if (TextureCreationDesc.Usage == ETextureUsage::RenderTarget)
        {
            const FRtvCreationDesc rtvCreationDesc = {
                .RtvDesc =
                    {
                        .Format = format,
                        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                        .Texture2D{.MipSlice = 0u, .PlaneSlice = 0u},
                    },
            };

            Texture.RtvIndex = CreateRtv(rtvCreationDesc, Texture.Allocation.Resource.Get());
        }
    }

    // Todo: mipmap generation
    return Texture;
}

void FGraphicsDevice::InitDeviceResources()
{
    InitD3D12Core();
    InitCommandQueues();
    InitDescriptorHeaps();
    InitMemoryAllocator();
    InitContexts();
    InitBindlessRootSignature();
}

void FGraphicsDevice::InitSwapchainResources(const uint32_t Width, const uint32_t Height)
{
    const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width = Width,
        .Height = Height,
        .Format = SwapchainFormat,
        .Stereo = FALSE,
        .SampleDesc{ // multisample option
            .Count = 1,
            .Quality = 0,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = FRAMES_IN_FLIGHT,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = 0u,
    };

    wrl::ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(Factory->CreateSwapChainForHwnd(DirectCommandQueue->GetCommandQueue(), WindowHandle,
        &swapChainDesc, nullptr, nullptr, &swapChain1));

    // Prevent DXGI from switching to full screen state automatically while using ALT + ENTER combination.
    ThrowIfFailed(Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&SwapChain));

    CurrentFrameIndex = SwapChain->GetCurrentBackBufferIndex();

    CreateBackBufferRTVs();
}

void FGraphicsDevice::InitD3D12Core()
{
    if constexpr (DEBUG_MODE)
    {
        ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface)));
        DebugInterface->EnableDebugLayer();
        DebugInterface->SetEnableGPUBasedValidation(true);
        DebugInterface->SetEnableSynchronizedCommandQueueValidation(true);
    }

    uint32_t dxgiFactoryCreationFlags{};
    if constexpr (DEBUG_MODE)
    {
        dxgiFactoryCreationFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(::CreateDXGIFactory2(dxgiFactoryCreationFlags, IID_PPV_ARGS(&Factory)));

    // Select the adapter (in this case GPU with best performance).
    ThrowIfFailed(
        Factory->EnumAdapterByGpuPreference(0u, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&Adapter)));

    if (!Adapter)
    {
        FatalError("Failed to find D3D12 compatible adapter");
    }
    
    DXGI_ADAPTER_DESC AdapterDesc{};
    ThrowIfFailed(Adapter->GetDesc(&AdapterDesc));
    Log(std::format(L"Chosen adapter : {}.", AdapterDesc.Description));

    // Create D3D12 Device.
    ThrowIfFailed(::D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));
    Device->SetName(L"D3D12 Device");
}

void FGraphicsDevice::InitCommandQueues()
{
    DirectCommandQueue =
        std::make_unique<FCommandQueue>(Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, L"Direct Command Queue");

    CopyCommandQueue =
        std::make_unique<FCommandQueue>(Device.Get(), D3D12_COMMAND_LIST_TYPE_COPY, L"Copy Command Queue");
}

void FGraphicsDevice::InitDescriptorHeaps()
{
    // Create descriptor heaps.
    CbvSrvUavDescriptorHeap = std::make_unique<FDescriptorHeap>(
        Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GNumCbvSrvUavDescriptorHeap, L"CBV SRV UAV Descriptor Heap");

    RtvDescriptorHeap = std::make_unique<FDescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, GNumRtvDescriptorHeap,
        L"RTV Descriptor Heap");

    DsvDescriptorHeap = std::make_unique<FDescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, GNumDsvDescriptorHeap,
        L"DSV Descriptor Heap");

    SamplerDescriptorHeap = std::make_unique<FDescriptorHeap>(Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        GNumSamplerDescriptorHeap, L"Sampler Descriptor Heap");
}

void FGraphicsDevice::InitMemoryAllocator()
{
    MemoryAllocator = std::make_unique<FMemoryAllocator>(Device.Get(), Adapter.Get());
}

void FGraphicsDevice::InitContexts()
{
    // Create graphics contexts (one per frame in flight).
    for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
    {
        PerFrameGraphicsContexts[i] = std::make_unique<FGraphicsContext>(this);
    }

    CopyContext = std::make_unique<FCopyContext>(this);
}

void FGraphicsDevice::InitBindlessRootSignature()
{
    // Setup bindless root signature.
    FPipelineState::CreateBindlessRootSignature(Device.Get(), L"Shaders/Triangle.hlsl");
}

uint32_t FGraphicsDevice::CreateCbv(const FCbvCreationDesc& CbvCreationDesc) const
{
    const uint32_t Index = CbvSrvUavDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateConstantBufferView(&CbvCreationDesc.CbvDesc,
        CbvSrvUavDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    CbvSrvUavDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

uint32_t FGraphicsDevice::CreateSrv(const FSrvCreationDesc& SrvCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = CbvSrvUavDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateShaderResourceView(Resource, &SrvCreationDesc.SrvDesc,
        CbvSrvUavDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    CbvSrvUavDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

uint32_t FGraphicsDevice::CreateDsv(const FDsvCreationDesc& DsvCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = DsvDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateDepthStencilView(Resource, &DsvCreationDesc.DsvDesc,
        DsvDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    CbvSrvUavDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

uint32_t FGraphicsDevice::CreateRtv(const FRtvCreationDesc& RtvCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = RtvDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateRenderTargetView(Resource, &RtvCreationDesc.RtvDesc,
        RtvDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    CbvSrvUavDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

template<typename T>
FBuffer FGraphicsDevice::CreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data) const
{
    FBuffer Buffer{};

    // If data.size() == 0, it means that the data to fill the buffer will be passed later on (via the Update
    // functions).
    const uint32_t NumberComponents = Data.size() == 0 ? 1 : static_cast<uint32_t>(Data.size());
    uint32_t SizeInBytes = NumberComponents * sizeof(T);

    const FResourceCreationDesc ResourceCreationDesc =
        FResourceCreationDesc::CreateBufferResourceCreationDesc(SizeInBytes);

    Buffer.Allocation = MemoryAllocator->CreateBufferResourceAllocation(BufferCreationDesc, ResourceCreationDesc);
    Buffer.SizeInBytes = SizeInBytes;

    std::scoped_lock<std::recursive_mutex> LockGuard(ResourceMutex);

    // Currently, not using a backing storage for upload context's and such. Simply using D3D12MA to create a upload
    // buffer, copy the data onto the upload buffer, and then copy data from upload buffer -> GPU only buffer.
    if (Data.data())
    {
        // Create upload buffer.
        const FBufferCreationDesc UploadBufferCreationDesc = {
            .Usage = EBufferUsage::UploadBuffer,
            .Name = "Upload buffer - " + std::string(BufferCreationDesc.Name),
        };

        FAllocation UploadAllocation =
            MemoryAllocator->CreateBufferResourceAllocation(UploadBufferCreationDesc, ResourceCreationDesc);

        UploadAllocation.Update(Data.data(), SizeInBytes);

        CopyContext->Reset();

        // Get a copy command and list and execute copy resource functions on the command queue.
        CopyContext->GetCommandList()->CopyResource(Buffer.Allocation.Resource.Get(), UploadAllocation.Resource.Get()); 

        CopyCommandQueue->ExecuteContext(CopyContext.get());
        CopyCommandQueue->Flush();

        UploadAllocation.Reset();
    }

    // Create relevant descriptor's.
    if (BufferCreationDesc.Usage == EBufferUsage::StructuredBuffer)
    {
        const FSrvCreationDesc SrvCreationDesc = {
            .SrvDesc =
                {
                    .Format = DXGI_FORMAT_UNKNOWN,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer =
                        {
                            .FirstElement = 0u,
                            .NumElements = static_cast<UINT>(Data.size()),
                            .StructureByteStride = static_cast<UINT>(sizeof(T)),
                        },
                },
        };

        Buffer.SrvIndex = CreateSrv(SrvCreationDesc, Buffer.Allocation.Resource.Get());
    }
    else if (BufferCreationDesc.Usage == EBufferUsage::ConstantBuffer)
    {
        const FCbvCreationDesc CbvCreationDesc = {
            .CbvDesc =
                {
                    .BufferLocation = Buffer.Allocation.Resource->GetGPUVirtualAddress(),
                    .SizeInBytes = static_cast<UINT>(Buffer.SizeInBytes),
                },
        };

        Buffer.CbvIndex = CreateCbv(CbvCreationDesc);
    }

    return Buffer;
}

#define CREATE_BUFFER_TEMPLATE_FUNC(TYPE) \
    template FBuffer FGraphicsDevice::CreateBuffer<TYPE>( \
        const FBufferCreationDesc& BufferCreationDesc, const std::span<const TYPE> Data) const; \

template FBuffer FGraphicsDevice::CreateBuffer<interlop::MaterialBuffer>(
    const FBufferCreationDesc& BufferCreationDesc, const std::span<const interlop::MaterialBuffer> Data) const;
template FBuffer FGraphicsDevice::CreateBuffer<XMFLOAT3>(
    const FBufferCreationDesc& BufferCreationDesc, const std::span<const XMFLOAT3> Data) const;
CREATE_BUFFER_TEMPLATE_FUNC(XMFLOAT2)
CREATE_BUFFER_TEMPLATE_FUNC(uint16_t)

void FGraphicsDevice::CreateBackBufferRTVs()
{
    FDescriptorHandle RtvHandle = RtvDescriptorHeap->GetDescriptorHandleFromStart();

    // Create Backbuffer render target views.
    for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
    {
        wrl::ComPtr<ID3D12Resource> BackBuffer{};
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));

        Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RtvHandle.CpuDescriptorHandle);

        BackBuffers[i].Allocation.Resource = BackBuffer;
        BackBuffers[i].Allocation.Resource->SetName(L"SwapChain BackBuffer");
        BackBuffers[i].RtvIndex = RtvDescriptorHeap->GetDescriptorIndex(RtvHandle);

        RtvDescriptorHeap->OffsetDescriptor(RtvHandle);
    }
}

void FGraphicsDevice::BeginFrame()
{
    PerFrameGraphicsContexts[CurrentFrameIndex]->Reset();
}

void FGraphicsDevice::Present()
{
    ThrowIfFailed(SwapChain->Present(1u, 0u));
}

void FGraphicsDevice::EndFrame()
{
    FenceValues[CurrentFrameIndex].DirectQueueFenceValue = DirectCommandQueue->Signal();

    CurrentFrameIndex = SwapChain->GetCurrentBackBufferIndex();

    DirectCommandQueue->WaitForFenceValue(FenceValues[CurrentFrameIndex].DirectQueueFenceValue);
}

