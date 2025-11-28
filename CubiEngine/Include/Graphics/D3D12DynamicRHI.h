#pragma once

#include <map>
#include <string>
#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/Resource.h"
#include "Graphics/ComputeContext.h"
#include "Graphics/Profiler.h"
#include "Graphics/Query.h"

class FMemoryAllocator;
class FCopyContext;
class FGraphicsContext;
class FMipmapGenerator;
class FD3D12DynamicRHI;

struct FFenceValues
{
    uint64_t DirectQueueFenceValue{};
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps = {
    D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

static const D3D12_HEAP_PROPERTIES kUploadHeapProps = {
    D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

extern FD3D12DynamicRHI* GD3D12RHI;

void CreateRHI(
    const uint32_t Width, const uint32_t Height,
    const DXGI_FORMAT SwapchainFormat, const HWND WindowHandle
);

ID3D12Device5* RHIGetDevice();

DXGI_FORMAT RHIGetSwapChainFormat();

FGraphicsContext* RHIGetCurrentGraphicsContext();
std::unique_ptr<FComputeContext> RHIGetComputeContext();

FDescriptorHandle RHIGetCurrentCbvSrvUavDescriptorHandle();

IDXGIAdapter* RHIGetAdapter();
IDXGIAdapter1* RHIGetAdapter1();

FCommandQueue* RHIGetDirectCommandQueue();

FPipelineState RHICreatePipelineState(const FGraphicsPipelineStateCreationDesc& Desc);
FPipelineState RHICreatePipelineState(const FComputePipelineStateCreationDesc& Desc);

FDescriptorHeap* RHIGetCbvSrvUavDescriptorHeap();
FDescriptorHeap* RHIGetRtvDescriptorHeap();
FDescriptorHeap* RHIGetDsvDescriptorHeap();
FDescriptorHeap* RHIGetSamplerDescriptorHeap();

uint32_t RHICreateSrv(const FSrvCreationDesc& SrvCreationDesc, ID3D12Resource* const Resource);

FSampler RHICreateSampler(const FSamplerCreationDesc& Desc);

FGPUProfiler& RHIGetGPUProfiler();
FQueryLocation RHIAllocateQuery(D3D12_QUERY_TYPE Type);
FQueryHeap* RHIGetTimestampQueryHeap();

void RHICreateRawBuffer(
    ComPtr<ID3D12Resource>& outBuffer,
    uint64_t size,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initState,
    const D3D12_HEAP_PROPERTIES& heapProps
);

std::unique_ptr<FTexture> RHICreateTexture(const FTextureCreationDesc& InTextureCreationDesc, const void* Data = nullptr);

template <typename T>
FBuffer RHICreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data = {});
FBuffer RHICreateBuffer(const FBufferCreationDesc& BufferCreationDesc, size_t TotalBytes);

FTexture* RHIGetCurrentBackBuffer();

void RHIBeginFrame();
void RHIEndFrame();

void RHIResizeSwapchainResources(uint32_t InWidth, uint32_t InHeight);
void RHIPresent();

void RHIExecuteAndFlushComputeContext(std::unique_ptr<FComputeContext>&& ComputeContext);
void RHIFlushAllQueue();

class FD3D12DynamicRHI
{
public:
    FD3D12DynamicRHI();
    ~FD3D12DynamicRHI();

    void Init(const uint32_t InWidth, const uint32_t InHeight,
        const DXGI_FORMAT InSwapchainFormat, const HWND InWindowHandle);

    void ResizeSwapchainResources(uint32_t InWidth, uint32_t InHeight);

    FSampler CreateSampler(const FSamplerCreationDesc& Desc) const;
    std::unique_ptr<FTexture> CreateTexture(const FTextureCreationDesc& InTextureCreationDesc, const void* Data = nullptr) const;
    FPipelineState CreatePipelineState(const FGraphicsPipelineStateCreationDesc& Desc) const;
    FPipelineState CreatePipelineState(const FComputePipelineStateCreationDesc& Desc) const;

    void CreateBackBufferRTVs();

    void BeginFrame();
    void Present();
    void EndFrame();
    void FlushAllQueue();

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

    ID3D12Device5* GetDevice() const { return Device.Get(); }
    IDXGIAdapter* GetAdapter() const { return Adapter.Get(); }
    IDXGIAdapter1* GetAdapter1() const { return Adapter1.Get(); }
    FCommandQueue* GetDirectCommandQueue() const { return DirectCommandQueue.get(); }
    FDescriptorHeap* GetCbvSrvUavDescriptorHeap() const { return CbvSrvUavDescriptorHeap.get(); }
    FDescriptorHeap* GetRtvDescriptorHeap() const { return RtvDescriptorHeap.get(); }
    FDescriptorHeap* GetDsvDescriptorHeap() const { return DsvDescriptorHeap.get(); }
    FDescriptorHeap* GetSamplerDescriptorHeap() const { return SamplerDescriptorHeap.get(); }

    FGraphicsContext* GetCurrentGraphicsContext() const { return PerFrameGraphicsContexts[CurrentFrameIndex].get(); }
    FTexture* GetCurrentBackBuffer() { return BackBuffers[CurrentFrameIndex].get(); }

    FGPUProfiler& GetGPUProfiler() { return GPUProfiler; }

    void ExecuteAndFlushComputeContext(std::unique_ptr<FComputeContext>&& ComputeContext);

    template <typename T>
    FBuffer CreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data = {}) const;
    FBuffer CreateBuffer(const FBufferCreationDesc& BufferCreationDesc, size_t TotalBytes) const;

    void CreateRawBuffer(ComPtr<ID3D12Resource>& outBuffer, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps) const;

    std::unique_ptr<FComputeContext> GetComputeContext();

    void GenerateMipmap(FTexture* Texture);

    DXGI_FORMAT GetSwapChainFormat() { return SwapchainFormat; }

    void MaintainQueryHeap();
    FQueryLocation AllocateQuery(D3D12_QUERY_TYPE Type);
    FQueryHeap* GetTimestampQueryHeap() { return TimeStampQueryHeap.get(); }

    uint32_t CreateSrv(const FSrvCreationDesc& SrvCreationDesc, ID3D12Resource* const Resource) const;

    FDescriptorHandle GetCbvSrvUavHandleFromIndex(uint32_t Index) const
    {
        return CbvSrvUavDescriptorHeap->GetDescriptorHandleFromIndex(Index);
    }

    static void AppendDebugTextureKeyList(std::vector<std::string>& Keys)
    {
        Keys.reserve(Keys.size() + DebugTextureMap.size());
        for (auto& kv : DebugTextureMap) {
            if (!kv.first.empty()) {
                Keys.push_back(kv.first);
            }
        }
    }

    static FTexture* GetDebugTexture(const std::string& Name)
    {
        auto it = DebugTextureMap.find(Name);
        if (it != DebugTextureMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    static bool AddDebugTexture(const std::string& Name, FTexture* Texture)
    {
        if (Name.empty() || !Texture) {
            return false;
        }

        DebugTextureMap[Name] = Texture;
        return true;
    }

    static bool RemoveDebugTexture(const std::string& Name, const FTexture* Texture)
    {
        if (Name.empty() || !Texture) {
            return false;
        }

        auto it = DebugTextureMap.find(Name);
        if (it == DebugTextureMap.end()) {
            return false;
        }

        if (it->second != Texture) {
            return false;
        }

        DebugTextureMap.erase(it);
        return true;
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
    wrl::ComPtr<IDXGIAdapter1> Adapter1{};

    DXGI_FORMAT SwapchainFormat;
    uint64_t CurrentFrameIndex{};
    std::array<FFenceValues, FRAMES_IN_FLIGHT> FenceValues{}; // Signal for Command Queue
    std::array<std::unique_ptr<FTexture>, FRAMES_IN_FLIGHT> BackBuffers{};

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

    static std::map<std::string, FTexture*> DebugTextureMap;
};

template<typename T>
inline FBuffer RHICreateBuffer(const FBufferCreationDesc& BufferCreationDesc, const std::span<const T> Data)
{
    return GD3D12RHI->CreateBuffer(BufferCreationDesc, Data);
}
