#pragma once

#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsContext.h"

class FMemoryAllocator;
class FCopyContext;

struct FFenceValues
{
    uint64_t DirectQueueFenceValue{};
};

class FGraphicsDevice
{
public:
    FGraphicsDevice(const uint32_t Width, const uint32_t Height,
        const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle);

    FGraphicsDevice() = delete;
    
    ~FGraphicsDevice();

    FSampler CreateSampler(const FSamplerCreationDesc& Desc) const;
    FTexture CreateTexture(const FTextureCreationDesc& TextureCreationDesc, const void* Data = nullptr) const;
    FPipelineState CreatePipelineState(const FGraphicsPipelineStateCreationDesc Desc) const;
    FPipelineState CreatePipelineState(const FComputePipelineStateCreationDesc Desc) const;

    void CreateBackBufferRTVs();
    
    void BeginFrame();
    void Present();
    void EndFrame();

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

    ID3D12Device5* GetDevice() const { return Device.Get(); }
    FCommandQueue* GetDirectCommandQueue() const { return DirectCommandQueue.get(); }
    FDescriptorHeap* GetCbvSrvUavDescriptorHeap() const { return CbvSrvUavDescriptorHeap.get(); }
    FDescriptorHeap* GetRtvDescriptorHeap() const { return RtvDescriptorHeap.get(); }
    FDescriptorHeap* GetDsvDescriptorHeap() const { return DsvDescriptorHeap.get(); }
    FDescriptorHeap* GetSamplerDescriptorHeap() const { return SamplerDescriptorHeap.get(); }
    
    FGraphicsContext* GetCurrentGraphicsContext() const { return PerFrameGraphicsContexts[CurrentFrameIndex].get(); }
    FTexture& GetCurrentBackBuffer() { return BackBuffers[CurrentFrameIndex]; }

    template <typename T>
    FBuffer CreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data = {}) const;

private:
    void InitDeviceResources();
    void InitSwapchainResources(const uint32_t Width, const uint32_t Height);
    void InitD3D12Core();
    void InitCommandQueues();
    void InitDescriptorHeaps();
    void InitMemoryAllocator();
    void InitContexts();
    void InitBindlessRootSignature();
    
    uint32_t CreateCbv(const FCbvCreationDesc& CbvCreationDesc) const;
    uint32_t CreateSrv(const FSrvCreationDesc& SrvCreationDesc, ID3D12Resource* const Resource) const;
    uint32_t CreateUav(const FUavCreationDesc& UavCreationDesc, ID3D12Resource* const Resource) const;
    uint32_t CreateDsv(const FDsvCreationDesc& DsvCreationDesc, ID3D12Resource* const Resource) const;
    uint32_t CreateRtv(const FRtvCreationDesc& RtvCreationDesc, ID3D12Resource* const Resource) const;

    HWND WindowHandle;

    wrl::ComPtr<ID3D12Debug3> DebugInterface{};
    wrl::ComPtr<IDXGIFactory6> Factory{};
    wrl::ComPtr<IDXGISwapChain4> SwapChain{};

    wrl::ComPtr<ID3D12Device5> Device{};
    wrl::ComPtr<IDXGIAdapter> Adapter{};

    DXGI_FORMAT SwapchainFormat;
    uint64_t CurrentFrameIndex{};
    std::array<FFenceValues, FRAMES_IN_FLIGHT> FenceValues{}; // Signal for Command Queue
    std::array<FTexture, FRAMES_IN_FLIGHT> BackBuffers{};

    std::array<std::unique_ptr<FGraphicsContext>, FRAMES_IN_FLIGHT> PerFrameGraphicsContexts{};
    std::unique_ptr<FCopyContext> CopyContext;

    std::unique_ptr<FDescriptorHeap> CbvSrvUavDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> RtvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> DsvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> SamplerDescriptorHeap;

    std::unique_ptr<FCommandQueue> DirectCommandQueue{};
    std::unique_ptr<FCommandQueue> CopyCommandQueue{};
    std::unique_ptr<FMemoryAllocator> MemoryAllocator{};

    mutable std::recursive_mutex ResourceMutex;
};
