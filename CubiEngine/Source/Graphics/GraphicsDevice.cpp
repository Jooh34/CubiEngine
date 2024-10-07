#include "Graphics/GraphicsDevice.h"
#include "Graphics/PipelineState.h"

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
    FTexture Texture{};

    if (TextureCreationDesc.usage == ETextureUsage::TextureFromData)
    {

    }
    
    return Texture;
}

void FGraphicsDevice::InitDeviceResources()
{
    InitD3D12Core();
    InitCommandQueues();
    InitDescriptorHeaps();
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

void FGraphicsDevice::InitContexts()
{
    // Create graphics contexts (one per frame in flight).
    for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
    {
        PerFrameGraphicsContexts[i] = std::make_unique<FGraphicsContext>(this);
    }
}

void FGraphicsDevice::InitBindlessRootSignature()
{
    // Setup bindless root signature.
    FPipelineState::CreateBindlessRootSignature(Device.Get(), L"Shaders/Triangle.hlsl");
}

void FGraphicsDevice::CreateBackBufferRTVs()
{
    FDescriptorHandle RtvHandle = RtvDescriptorHeap->GetDescriptorHandleFromStart();

    // Create Backbuffer render target views.
    for (const uint32_t i : std::views::iota(0u, FRAMES_IN_FLIGHT))
    {
        wrl::ComPtr<ID3D12Resource> BackBuffer{};
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));

        Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RtvHandle.CpuDescriptorHandle);

        BackBuffers[i].Resource = BackBuffer;
        BackBuffers[i].Resource->SetName(L"SwapChain BackBuffer");
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

