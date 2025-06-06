#pragma once

#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/ComputeContext.h"
#include "Graphics/MipmapGenerator.h"
#include "Graphics/Profiler.h"
#include "Graphics/Query.h"

class FMemoryAllocator;
class FCopyContext;

struct FFenceValues
{
    uint64_t DirectQueueFenceValue{};
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps = {
    D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

static const D3D12_HEAP_PROPERTIES kUploadHeapProps = {
    D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

class FGraphicsDevice
{
public:
    FGraphicsDevice(const uint32_t Width, const uint32_t Height,
        const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle);

    FGraphicsDevice() = delete;
    
    ~FGraphicsDevice();

    void ResizeSwapchainResources(uint32_t InWidth, uint32_t InHeight);
    void OnWindowResized(uint32_t InWidth, uint32_t InHeight);

    FSampler CreateSampler(const FSamplerCreationDesc& Desc) const;
    FTexture CreateTexture(const FTextureCreationDesc& InTextureCreationDesc, const void* Data = nullptr) const;
    FPipelineState CreatePipelineState(const FGraphicsPipelineStateCreationDesc Desc) const;
    FPipelineState CreatePipelineState(const FComputePipelineStateCreationDesc Desc) const;

    void CreateBackBufferRTVs();
    
    void BeginFrame();
    void Present();
    void EndFrame();
    void FlushAllQueue();

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

    ID3D12Device5* GetDevice() const { return Device.Get(); }
    FCommandQueue* GetDirectCommandQueue() const { return DirectCommandQueue.get(); }
    FDescriptorHeap* GetCbvSrvUavDescriptorHeap() const { return CbvSrvUavDescriptorHeap.get(); }
    FDescriptorHeap* GetRtvDescriptorHeap() const { return RtvDescriptorHeap.get(); }
    FDescriptorHeap* GetDsvDescriptorHeap() const { return DsvDescriptorHeap.get(); }
    FDescriptorHeap* GetSamplerDescriptorHeap() const { return SamplerDescriptorHeap.get(); }

    FGraphicsContext* GetCurrentGraphicsContext() const { return PerFrameGraphicsContexts[CurrentFrameIndex].get(); }
    FTexture& GetCurrentBackBuffer() { return BackBuffers[CurrentFrameIndex]; }

    FGPUProfiler& GetGPUProfiler() { return GPUProfiler; }
    
    void ExecuteAndFlushComputeContext(std::unique_ptr<FComputeContext>&& ComputeContext);

    template <typename T>
    FBuffer CreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data = {}) const;

    void CreateRawBuffer(ComPtr<ID3D12Resource>& outBuffer, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps) const;
    
    std::unique_ptr<FComputeContext> GetComputeContext();

    void GenerateMipmap(FTexture& Texture);

    DXGI_FORMAT GetSwapChainFormat() { return SwapchainFormat; }

    void MaintainQueryHeap();
    FQueryLocation AllocateQuery(D3D12_QUERY_TYPE Type);
    FQueryHeap* GetTimestampQueryHeap() { return TimeStampQueryHeap.get(); }

    uint32_t CreateSrv(const FSrvCreationDesc& SrvCreationDesc, ID3D12Resource* const Resource) const;

    FDescriptorHandle GetCbvSrvUavHandleFromIndex(uint32_t Index) const
    {
        return CbvSrvUavDescriptorHeap->GetDescriptorHandleFromIndex(Index);
    }

private:
    bool bInitialized = false;

    void InitDeviceResources();
    void InitSwapchainResources(const uint32_t Width, const uint32_t Height);
    void InitD3D12Core();
    void InitCommandQueues();
    void InitDescriptorHeaps();
    void InitMemoryAllocator();
    void InitContexts();
    void InitBindlessRootSignature();
    
    uint32_t CreateCbv(const FCbvCreationDesc& CbvCreationDesc) const;
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
    std::queue<std::unique_ptr<FComputeContext>> ComputeContextQueue;

    std::unique_ptr<FDescriptorHeap> CbvSrvUavDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> RtvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> DsvDescriptorHeap;
    std::unique_ptr<FDescriptorHeap> SamplerDescriptorHeap;

    std::unique_ptr<FCommandQueue> DirectCommandQueue{};
    std::unique_ptr<FCommandQueue> ComputeCommandQueue{};
    std::unique_ptr<FCommandQueue> CopyCommandQueue{};
    std::unique_ptr<FMemoryAllocator> MemoryAllocator{};

    mutable std::recursive_mutex ResourceMutex;

    std::unique_ptr<FMipmapGenerator> MipmapGenerator{};

    FGPUProfiler GPUProfiler;

    std::unique_ptr<FQueryHeap> TimeStampQueryHeap;
};
