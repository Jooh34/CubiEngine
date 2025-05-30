#include "Graphics/GraphicsDevice.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/MemoryAllocator.h"
#include "Graphics/CopyContext.h"
#include "Core/FileSystem.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"
#include <DirectXTex.h>

FGraphicsDevice::FGraphicsDevice(const uint32_t Width, const uint32_t Height,
    const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle)
    : SwapchainFormat(SwapchainFormat), WindowHandle(WindowHandle), GPUProfiler(this)
{
    InitDeviceResources();
    InitSwapchainResources(Width, Height);

    MipmapGenerator = std::make_unique<FMipmapGenerator>(this);

    TimeStampQueryHeap = std::make_unique<FQueryHeap>(this, D3D12_QUERY_TYPE_TIMESTAMP, D3D12_QUERY_HEAP_TYPE_TIMESTAMP);;
}

FGraphicsDevice::~FGraphicsDevice()
{
    FlushAllQueue();

    // Uncomment this code if live object  warning occurs.
    // ComPtr<ID3D12DebugDevice> debugDevice;
    //if (SUCCEEDED(Device.As(&debugDevice))) {
    //    debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
    //}
}

void FGraphicsDevice::OnWindowResized(uint32_t InWidth, uint32_t InHeight)
{
    ResizeSwapchainResources(InWidth, InHeight);
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

FTexture FGraphicsDevice::CreateTexture(const FTextureCreationDesc& InTextureCreationDesc, const void* Data) const
{
    FTextureCreationDesc TextureCreationDesc = InTextureCreationDesc;

    DXGI_FORMAT dsFormat{};

    if (TextureCreationDesc.Format == DXGI_FORMAT_R32_FLOAT
        || TextureCreationDesc.Format == DXGI_FORMAT_D32_FLOAT
        || TextureCreationDesc.Format == DXGI_FORMAT_R32_TYPELESS)
    {
        dsFormat = DXGI_FORMAT_D32_FLOAT;
        TextureCreationDesc.Format = DXGI_FORMAT_R32_FLOAT;
    }
    else if (TextureCreationDesc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
    {
        FatalError("Currently, the renderer does not support depth format of the type D24_S8_UINT. Please use one of the X32 types.");
    }

    void* TextureData{ (void*)Data };

    float* HdrTextureData{ nullptr };

	DirectX::ScratchImage scratchImage;

    if (TextureCreationDesc.Usage == ETextureUsage::HDRTextureFromPath)
    {
		int32_t Width, Height;

        int ComponentCount = 4;
        std::string FullPath = FFileSystem::GetFullPath(wStringToString(TextureCreationDesc.Path));
        HdrTextureData =
            stbi_loadf(FullPath.c_str(), &Width, &Height, nullptr, ComponentCount);

        if (!HdrTextureData)
        {
            FatalError(
                std::format("Failed to load texture from path : {}.", wStringToString(TextureCreationDesc.Path)));
        }

        TextureCreationDesc.Width = Width;
        TextureCreationDesc.Height = Height;
	}
	else if (TextureCreationDesc.Usage == ETextureUsage::TextureFromPath)
	{
		int32_t Width, Height, Channels;
		std::string FullPath = FFileSystem::GetFullPath(wStringToString(TextureCreationDesc.Path));
		TextureData = stbi_load(FullPath.c_str(), &Width, &Height, &Channels, 0);

		if (!TextureData)
		{
			FatalError(
				std::format("Failed to load texture from path : {}.", wStringToString(TextureCreationDesc.Path)));
		}

		TextureCreationDesc.Width = Width;
		TextureCreationDesc.Height = Height;
	}
    else if (TextureCreationDesc.Usage == ETextureUsage::DDSTextureFromPath)
    {
        std::string FullPath = FFileSystem::GetFullPath(wStringToString(TextureCreationDesc.Path));
        std::wstring wFullPath = StringToWString(FullPath);

        HRESULT hr = DirectX::LoadFromDDSFile(wFullPath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, scratchImage);
        if (FAILED(hr)) {
            FatalError(
                std::format("LoadFromDDSFile Failed. : {}.", wStringToString(TextureCreationDesc.Path)));
        }
        const DirectX::TexMetadata& metadata = scratchImage.GetMetadata();

		TextureCreationDesc.Width = static_cast<uint32_t>(metadata.width);
		TextureCreationDesc.Height = static_cast<uint32_t>(metadata.height);
		TextureCreationDesc.DepthOrArraySize = static_cast<uint32_t>(metadata.arraySize);
		TextureCreationDesc.MipLevels = static_cast<uint32_t>(metadata.mipLevels);
		TextureCreationDesc.Format = metadata.format;
        TextureCreationDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    
	bool bUAVAllowed = FTexture::IsUAVAllowed(TextureCreationDesc.Usage, TextureCreationDesc.Format);

    FTexture Texture{};
    // GPU only memory
    D3D12_RESOURCE_STATES ResourceState;
    Texture.Allocation = MemoryAllocator->CreateTextureResourceAllocation(TextureCreationDesc, ResourceState, bUAVAllowed);
    Texture.Width = TextureCreationDesc.Width;
    Texture.Height = TextureCreationDesc.Height;
    Texture.ResourceState = ResourceState;
    Texture.Usage = TextureCreationDesc.Usage;
    Texture.Format = TextureCreationDesc.Format;
    Texture.DebugName = TextureCreationDesc.Name;
    
    if (TextureData || HdrTextureData || TextureCreationDesc.Usage == ETextureUsage::DDSTextureFromPath) // Upload Texture Buffer
    {
        std::vector<D3D12_SUBRESOURCE_DATA> TextureSubresourceData;

        uint32_t BytesPerPixel = FTexture::GetBytesPerPixel(TextureCreationDesc.Format);
        if (TextureCreationDesc.Usage == ETextureUsage::DDSTextureFromPath)
        {
            auto hr = DirectX::PrepareUpload(Device.Get(), scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), TextureSubresourceData);
            if (FAILED(hr)) {
                FatalError(
                    std::format("PrepareUpload Failed. : {}.", wStringToString(TextureCreationDesc.Path)));
            }
        }
        else if (TextureCreationDesc.Usage == ETextureUsage::HDRTextureFromPath)
        {
            TextureSubresourceData.push_back({
                .pData = HdrTextureData,
                .RowPitch = TextureCreationDesc.Width * BytesPerPixel,
                .SlicePitch = TextureCreationDesc.Width * TextureCreationDesc.Height * BytesPerPixel,
            });
        }
        else // TexureUsage:: TextureFromPath or TextureFromData (non HDR).
        {
            TextureSubresourceData.push_back({
                .pData = TextureData,
                .RowPitch = TextureCreationDesc.Width * BytesPerPixel,
                .SlicePitch = TextureCreationDesc.Width * TextureCreationDesc.Height * BytesPerPixel,
            });
        }
        
        // Create upload buffer.
        const FBufferCreationDesc UploadBufferCreationDesc = {
            .Usage = EBufferUsage::UploadBuffer,
            .Name = L"Upload buffer - " + std::wstring(TextureCreationDesc.Name),
        };

        const UINT64 UploadBufferSize = GetRequiredIntermediateSize(Texture.Allocation.Resource.Get(), 0, TextureSubresourceData.size());
        const FResourceCreationDesc ResourceCreationDesc = FResourceCreationDesc::CreateBufferResourceCreationDesc(UploadBufferSize);

        FAllocation UploadAllocation =
            MemoryAllocator->CreateBufferResourceAllocation(UploadBufferCreationDesc, ResourceCreationDesc);

        // Use the copy context and execute UpdateSubresources functions on the copy command queue.
        std::scoped_lock<std::recursive_mutex> LockGuard(ResourceMutex);

        CopyContext->Reset();

        UpdateSubresources(CopyContext->GetCommandList(), Texture.Allocation.Resource.Get(),
            UploadAllocation.Resource.Get(), 0u, 0u, TextureSubresourceData.size(), TextureSubresourceData.data());

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
                    .Format = TextureCreationDesc.Format,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {
                        .MostDetailedMip = 0u,
                        .MipLevels = TextureCreationDesc.MipLevels, // Todo:Mipmap
                    }
                }
            };
        }
        else if (TextureCreationDesc.DepthOrArraySize == 6u)
        {
            SrvCreationDesc = {
                .SrvDesc = {
                    .Format = TextureCreationDesc.Format,
                    .ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Texture2D = {
                        .MostDetailedMip = 0u,
                        .MipLevels = TextureCreationDesc.MipLevels, // Todo:Mipmap
                    }
                }
            };
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
                        .Format = TextureCreationDesc.Format,
                        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                        .Texture2D{.MipSlice = 0u, .PlaneSlice = 0u},
                    },
            };

            Texture.RtvIndex = CreateRtv(rtvCreationDesc, Texture.Allocation.Resource.Get());
        }

        // Create UAV
        if (bUAVAllowed)
        {
            if (TextureCreationDesc.DepthOrArraySize > 1u)
            {
                for (uint32_t i = 0; i < TextureCreationDesc.MipLevels; i++)
                {
                    // TODO: mips
                    const FUavCreationDesc UavCreationDesc = {
                        .UavDesc =
                            {
                                .Format = FTexture::ConvertToLinearFormat(TextureCreationDesc.Format),
                                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
                                .Texture2DArray{
                                    .MipSlice = i,
                                    .FirstArraySlice = 0u,
                                    .ArraySize = TextureCreationDesc.DepthOrArraySize
                                },
                            },
                    };
                    
                    uint32_t UavIndex = CreateUav(UavCreationDesc, Texture.Allocation.Resource.Get());
                    if (i == 0)
                    {
                        Texture.UavIndex = UavIndex;
                    }
                    Texture.MipUavIndex.push_back(UavIndex);
                }
            }
            else
            {
                for (uint32_t i = 0; i < TextureCreationDesc.MipLevels; i++)
                {
                    const FUavCreationDesc UavCreationDesc = {
                        .UavDesc =
                            {
                                .Format = FTexture::ConvertToLinearFormat(TextureCreationDesc.Format),
                                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                .Texture2D = {.MipSlice = i, .PlaneSlice = 0u},
                            },
                    };
                    uint32_t UavIndex = CreateUav(UavCreationDesc, Texture.Allocation.Resource.Get());

                    if (i == 0)
                    {
                        Texture.UavIndex = UavIndex;
                    }
                    Texture.MipUavIndex.push_back(UavIndex);
                }
            }

        }
    }

    if (bUAVAllowed && !FTexture::IsCompressedFormat(TextureCreationDesc.Format))
    {
		MipmapGenerator->GenerateMipmap(Texture);
    }

    return Texture;
}

FPipelineState FGraphicsDevice::CreatePipelineState(const FGraphicsPipelineStateCreationDesc Desc) const
{
    FPipelineState PipelineState(Device.Get(), Desc);
    return PipelineState;
}

FPipelineState FGraphicsDevice::CreatePipelineState(const FComputePipelineStateCreationDesc Desc) const
{
    FPipelineState PipelineState(Device.Get(), Desc);
    return PipelineState;
}

void FGraphicsDevice::ExecuteAndFlushComputeContext(std::unique_ptr<FComputeContext>&& ComputeContext)
{
    // Execute compute context and push to the queue.
    ComputeCommandQueue->ExecuteContext(ComputeContext.get());
    ComputeCommandQueue->Flush();

    ComputeContextQueue.emplace(std::move(ComputeContext));
}

void FGraphicsDevice::CreateRawBuffer(ComPtr<ID3D12Resource>& outBuffer, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps) const
{
    assert(outBuffer.Get() == nullptr);

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Alignment = 0;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Flags = flags;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Height = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Width = size;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // D3D Warning : Ignoring InitialState D3D12_RESOURCE_STATE_UNORDERED_ACCESS. Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON.
    if (initState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        initState = D3D12_RESOURCE_STATE_COMMON;
    }

    ThrowIfFailed(GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
        initState, nullptr, IID_PPV_ARGS(&outBuffer)));
}

std::unique_ptr<FComputeContext> FGraphicsDevice::GetComputeContext()
{
    if (ComputeContextQueue.empty())
    {
        std::unique_ptr<FComputeContext> Context = std::make_unique<FComputeContext>(this);
        return Context;
    }
    else
    {
        std::unique_ptr<FComputeContext> Ret = std::move(ComputeContextQueue.front());
        ComputeContextQueue.pop();
        return Ret;
    }
}

void FGraphicsDevice::GenerateMipmap(FTexture& Texture)
{
    MipmapGenerator->GenerateMipmap(Texture);
}

void FGraphicsDevice::MaintainQueryHeap()
{
    TimeStampQueryHeap->Maintain();
}

FQueryLocation FGraphicsDevice::AllocateQuery(D3D12_QUERY_TYPE Type)
{
    uint32_t Index = TimeStampQueryHeap->AllocateQuery();

    return FQueryLocation(
        TimeStampQueryHeap.get(),
        Index,
        Type
    );
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
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
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

void FGraphicsDevice::ResizeSwapchainResources(uint32_t InWidth, uint32_t InHeight)
{
    // Wait all GPU works to finished
    FlushAllQueue();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        BackBuffers[i].Allocation.Reset();
        BackBuffers[i] = FTexture(); // Release
    }
    
    // Resize the swap chain buffers
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    SwapChain->GetDesc(&swapChainDesc);

    ThrowIfFailed(
        SwapChain->ResizeBuffers(
            FRAMES_IN_FLIGHT,
            InWidth,
            InHeight,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        )
    );

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

    ComputeCommandQueue =
        std::make_unique<FCommandQueue>(Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, L"Compute Command Queue");
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
    FRaytracingPipelineState::CreateRaytracingRootSignature(Device.Get());
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

uint32_t FGraphicsDevice::CreateUav(const FUavCreationDesc& UavCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = CbvSrvUavDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateUnorderedAccessView(Resource, nullptr, &UavCreationDesc.UavDesc,
        CbvSrvUavDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    CbvSrvUavDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

uint32_t FGraphicsDevice::CreateDsv(const FDsvCreationDesc& DsvCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = DsvDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateDepthStencilView(Resource, &DsvCreationDesc.DsvDesc,
        DsvDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    DsvDescriptorHeap->OffsetCurrentHandle();
    return Index;
}

uint32_t FGraphicsDevice::CreateRtv(const FRtvCreationDesc& RtvCreationDesc, ID3D12Resource* const Resource) const
{
    const uint32_t Index = RtvDescriptorHeap->GetCurrentDescriptorIndex();

    Device->CreateRenderTargetView(Resource, &RtvCreationDesc.RtvDesc,
        RtvDescriptorHeap->GetCurrentDescriptorHandle().CpuDescriptorHandle);

    RtvDescriptorHeap->OffsetCurrentHandle();
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

    FResourceCreationDesc ResourceCreationDesc =
        FResourceCreationDesc::CreateBufferResourceCreationDesc(SizeInBytes);

    if (BufferCreationDesc.Usage == EBufferUsage::StructuredBufferUAV)
    {
        ResourceCreationDesc.ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    Buffer.Allocation = MemoryAllocator->CreateBufferResourceAllocation(BufferCreationDesc, ResourceCreationDesc);
    Buffer.SizeInBytes = SizeInBytes;
    Buffer.NumElement = NumberComponents;
    Buffer.ElementSizeInBytes = sizeof(T);

    std::scoped_lock<std::recursive_mutex> LockGuard(ResourceMutex);

    // Currently, not using a backing storage for upload context's and such. Simply using D3D12MA to create a upload
    // buffer, copy the data onto the upload buffer, and then copy data from upload buffer -> GPU only buffer.
    if (Data.data())
    {
        // Create upload buffer.
        const FBufferCreationDesc UploadBufferCreationDesc = {
            .Usage = EBufferUsage::UploadBuffer,
            .Name = L"Upload buffer - " + std::wstring(BufferCreationDesc.Name),
        };
        
        FResourceCreationDesc UploadResourceCreationDesc =
            FResourceCreationDesc::CreateBufferResourceCreationDesc(SizeInBytes);

        FAllocation UploadAllocation =
            MemoryAllocator->CreateBufferResourceAllocation(UploadBufferCreationDesc, UploadResourceCreationDesc);

        UploadAllocation.Update(Data.data(), SizeInBytes);

        CopyContext->Reset();

        // Get a copy command and list and execute copy resource functions on the command queue.
        CopyContext->GetCommandList()->CopyResource(Buffer.Allocation.Resource.Get(), UploadAllocation.Resource.Get()); 

        CopyCommandQueue->ExecuteContext(CopyContext.get());
        CopyCommandQueue->Flush();

        UploadAllocation.Reset();
    }

    // Create relevant descriptor's.
    if (BufferCreationDesc.Usage == EBufferUsage::StructuredBuffer || BufferCreationDesc.Usage == EBufferUsage::StructuredBufferUAV)
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

    if (BufferCreationDesc.Usage == EBufferUsage::StructuredBufferUAV)
    {
        const FUavCreationDesc UavCreationDesc = {
            .UavDesc =
                {
                    .Format = DXGI_FORMAT_UNKNOWN,
                    .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
                    .Buffer =
                        {
                            .FirstElement = 0u,
                            .NumElements = static_cast<UINT>(Data.size()),
                            .StructureByteStride = static_cast<UINT>(sizeof(T)),
                            .CounterOffsetInBytes = 0u,
                            .Flags = D3D12_BUFFER_UAV_FLAG_NONE,
                        },
                },
        };

        Buffer.UavIndex = CreateUav(UavCreationDesc, Buffer.Allocation.Resource.Get());
    }


    return Buffer;
}

#define CREATE_BUFFER_TEMPLATE_FUNC(TYPE) \
    template FBuffer FGraphicsDevice::CreateBuffer<TYPE>( \
        const FBufferCreationDesc& BufferCreationDesc, const std::span<const TYPE> Data) const; \

CREATE_BUFFER_TEMPLATE_FUNC(interlop::MaterialBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(XMFLOAT4)
CREATE_BUFFER_TEMPLATE_FUNC(XMFLOAT3)
CREATE_BUFFER_TEMPLATE_FUNC(XMFLOAT2)
CREATE_BUFFER_TEMPLATE_FUNC(UINT)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::TransformBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::DebugBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::SceneBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::LightBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::ShadowBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::SSAOKernelBuffer)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::FRaytracingGeometryInfo)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::FRaytracingMaterial)
CREATE_BUFFER_TEMPLATE_FUNC(interlop::MeshVertex)

void FGraphicsDevice::CreateBackBufferRTVs()
{
    FDescriptorHandle RtvHandle = RtvDescriptorHeap->GetDescriptorHandleFromStart();
    //FDescriptorHandle RtvHandle = RtvDescriptorHeap->GetCurrentDescriptorHandle();

    // Create Backbuffer render target views.
    for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
    {
        wrl::ComPtr<ID3D12Resource> BackBuffer{};
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));
        
        Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RtvHandle.CpuDescriptorHandle);

        BackBuffers[i].Allocation.Resource = BackBuffer;
        BackBuffers[i].Allocation.Resource->SetName(L"SwapChain BackBuffer");
        BackBuffers[i].RtvIndex = RtvDescriptorHeap->GetDescriptorIndex(RtvHandle);
        BackBuffers[i].ResourceState = D3D12_RESOURCE_STATE_PRESENT;

        RtvDescriptorHeap->OffsetDescriptor(RtvHandle);
    }

    if (!bInitialized)
    {
        RtvDescriptorHeap->OffsetCurrentHandle(FRAMES_IN_FLIGHT);
        bInitialized = true;
    }
}

void FGraphicsDevice::BeginFrame()
{
    MaintainQueryHeap();
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

void FGraphicsDevice::FlushAllQueue()
{
    DirectCommandQueue->Flush(); // flush GPU works
    CopyCommandQueue->Flush();
    ComputeCommandQueue->Flush();
}

