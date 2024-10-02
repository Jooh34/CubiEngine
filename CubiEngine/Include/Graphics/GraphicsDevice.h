#pragma once

#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"

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
    void InitBindlessRootSignature();
    
    void Prensent();
    void EndFrame();

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

    FCommandQueue* GetDirectCommandQueue() const { return DirectCommandQueue.get(); }
private:
    HWND WindowHandle;

    wrl::ComPtr<ID3D12Debug3> DebugInterface{};
    wrl::ComPtr<IDXGIFactory6> Factory{};
    wrl::ComPtr<IDXGISwapChain4> SwapChain{};
    uint64_t CurrentFrameIndex{};

    wrl::ComPtr<ID3D12Device5> Device{};
    wrl::ComPtr<IDXGIAdapter> Adapter{};

    DXGI_FORMAT SwapchainFormat;
    std::array<FFenceValues, FRAMES_IN_FLIGHT> FenceValues{}; // Signal for Command Queue

    std::unique_ptr<FDescriptorHeap> CbvSrvUavDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> RtvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> DsvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> SamplerDescriptorHeap;

    std::unique_ptr<FCommandQueue> DirectCommandQueue{};
};
