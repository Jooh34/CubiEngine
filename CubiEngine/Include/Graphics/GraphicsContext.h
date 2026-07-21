#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RaytracingPipelineState.h"

#include <type_traits>

class FQueryHeap;

class FGraphicsContext : public FContext
{
public:
    FGraphicsContext();
    void SetDescriptorHeaps() const;
    void Reset();

    void ClearRenderTargetView(const FTexture* InRenderTarget, std::span<const float, 4> Color);
    void ClearUnorderedAccessViewFloat(const FTexture* Texture, std::span<const float, 4> Color);
    void ClearDepthStencilView(const FTexture* Texture);

    void SetRenderTarget(const FTexture* RenderTarget) const;
    void SetRenderTarget(const FTexture* RenderTarget, const FTexture* DepthStencilTexture) const;
    void SetRenderTargets(const std::span<const FTexture*> RenderTargets, const FTexture* DepthStencilTexture) const;
    void SetRenderTargetDepthOnly(const FTexture* DepthStencilTexture) const;

    void SetGraphicsPipelineState(const FPipelineState& PipelineState) const;
    void SetComputePipelineState(const FPipelineState& PipelineState) const;
    void SetRaytracingPipelineState(const FRaytracingPipelineState& PipelineState) const;

    void SetViewport(const D3D12_VIEWPORT& Viewport, bool bScissorRectAsSame = false) const;
    void SetScissorRects(const D3D12_RECT& Rect) const;
    void SetPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology) const;

    void SetIndexBuffer(const FBuffer& Buffer) const;
    void DrawIndexedInstanced(const uint32_t IndicesCount, const uint32_t InstanceCount = 1u) const;
    void DrawInstanced(uint32_t VertexCountPerInstance,
        uint32_t InstanceCount,
        uint32_t StartVertexLocation,
        uint32_t StartInstanceLocation) const;

    void SetGraphicsRootSignature() const;
    template<typename T>
    void SetGraphicsRoot32BitConstants(const T* RenderResources) const
    {
        SetRoot32BitConstants<T>(0u, RenderResources, true);
    }

    void SetComputeRootSignature() const;
    void SetRaytracingComputeRootSignature() const;
    template<typename T>
    void SetComputeRoot32BitConstants(const T* RenderResources) const
    {
        SetRoot32BitConstants<T>(0u, RenderResources, false);
    }

    template<typename T>
    void SetComputeRoot32BitConstants(UINT RootParameterIndex, const T* RenderResources) const
    {
        static_assert(sizeof(T) / sizeof(uint32_t) <= RAYTRACING_NUMBER_32_BIT_CONSTANTS,
            "Root constants exceed the raytracing root signature limit.");
        SetRoot32BitConstants<T>(RootParameterIndex, RenderResources, false);
    }

    void CopyResource(ID3D12Resource* const Destination, ID3D12Resource* const Source) const;

    void Dispatch(const uint32_t ThreadGroupDimX, const uint32_t ThreadGroupDimY, const uint32_t ThreadGroupDimZ);

    void BeginQuery(FQueryHeap* Heap, D3D12_QUERY_TYPE Type, uint32_t Index);
    void EndQuery(FQueryHeap* Heap, D3D12_QUERY_TYPE Type, uint32_t Index);
    void ResolveQueryData(FQueryHeap* Heap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, UINT64 AlignedDestinationBufferOffset = 0);

    void DispatchRays(D3D12_DISPATCH_RAYS_DESC& RayDesc) const;

    void SetComputeRootShaderResourceView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
    void SetComputeRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

private:
    static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
    static constexpr uint32_t RAYTRACING_NUMBER_32_BIT_CONSTANTS = 48;

    template<typename T>
    void SetRoot32BitConstants(UINT RootParameterIndex, const T* RenderResources, bool bGraphics) const
    {
        static_assert(std::is_trivially_copyable_v<T>, "Root constants must be trivially copyable.");
        static_assert(sizeof(T) % sizeof(uint32_t) == 0u, "Root constants must be DWORD aligned.");
        constexpr UINT Num32BitValues = static_cast<UINT>(sizeof(T) / sizeof(uint32_t));
        static_assert(Num32BitValues <= NUMBER_32_BIT_CONSTANTS, "Root constants exceed the root signature limit.");

        if (bGraphics)
        {
            D3D12CommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, RenderResources, 0u);
        }
        else
        {
            D3D12CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValues, RenderResources, 0u);
        }
    }
};
