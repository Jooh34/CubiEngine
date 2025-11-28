#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RaytracingPipelineState.h"

class FQueryHeap;

class FGraphicsContext : public FContext
{
public:
    FGraphicsContext();
    void SetDescriptorHeaps() const;
    void Reset();

    void ClearRenderTargetView(const FTexture* InRenderTarget, std::span<const float, 4> Color);
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
    void SetGraphicsRoot32BitConstants(const void* RenderResources)  const;
    void SetComputeRootSignature() const;
    void SetRaytracingComputeRootSignature() const;
    void SetComputeRoot32BitConstants(const void* RenderResources)  const;
    void SetComputeRoot32BitConstants(UINT RootParameterIndex, UINT Num32BitValuesToSet, const void* RenderResources) const;

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
};