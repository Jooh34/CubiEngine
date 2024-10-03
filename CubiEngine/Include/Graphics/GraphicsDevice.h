#pragma once

#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsContext.h"

struct FFenceValues
{
    uint64_t DirectQueueFenceValue{};
};

class FGraphicsDevice
{
public:
    FGraphicsDevice(const int Width, const int Height,
        const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle);

    FGraphicsDevice() = delete;
    
    ~FGraphicsDevice();

    void InitDeviceResources();

    void InitSwapchainResources(const uint32_t Width, const uint32_t Height);

    void InitD3D12Core();
    void InitCommandQueues();
    void InitDescriptorHeaps();
    void InitContexts();
    void InitBindlessRootSignature();

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
    Texture& GetCurrentBackBuffer() { return BackBuffers[CurrentFrameIndex]; }

private:
    HWND WindowHandle;

    wrl::ComPtr<ID3D12Debug3> DebugInterface{};
    wrl::ComPtr<IDXGIFactory6> Factory{};
    wrl::ComPtr<IDXGISwapChain4> SwapChain{};

    wrl::ComPtr<ID3D12Device5> Device{};
    wrl::ComPtr<IDXGIAdapter> Adapter{};

    DXGI_FORMAT SwapchainFormat;
    uint64_t CurrentFrameIndex{};
    std::array<FFenceValues, FRAMES_IN_FLIGHT> FenceValues{}; // Signal for Command Queue
    std::array<Texture, FRAMES_IN_FLIGHT> BackBuffers{};

    std::array<std::unique_ptr<FGraphicsContext>, FRAMES_IN_FLIGHT> PerFrameGraphicsContexts{};


    std::unique_ptr<FDescriptorHeap> CbvSrvUavDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> RtvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> DsvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> SamplerDescriptorHeap;

    std::unique_ptr<FCommandQueue> DirectCommandQueue{};
};
