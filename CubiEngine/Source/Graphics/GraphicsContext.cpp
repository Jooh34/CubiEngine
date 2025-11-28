#include "Graphics/GraphicsContext.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/DescriptorHeap.h"

FGraphicsContext::FGraphicsContext()
{
    ThrowIfFailed(RHIGetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&D3D12CommandAllocator)));
    
    wrl::ComPtr<ID3D12GraphicsCommandList> D3D12CommandListBase;

    ThrowIfFailed(RHIGetDevice()->CreateCommandList(
        0u, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&D3D12CommandListBase)));

    if (!SUCCEEDED(D3D12CommandListBase.As(&D3D12CommandList))) {
        // not supported
        FatalError("current device does not support ID3D12GraphicsD3D12CommandList4!");
    }

    SetDescriptorHeaps();

    ThrowIfFailed(D3D12CommandList->Close());
}

void FGraphicsContext::SetDescriptorHeaps() const
{
    // As all graphics context's require to set the descriptor heap before hand, the user has option to set them
    // manually (for explicitness) or just let the constructor take care of this.
    const std::array<const FDescriptorHeap* const, 2u> shaderVisibleDescriptorHeaps = {
        RHIGetCbvSrvUavDescriptorHeap(),
        RHIGetSamplerDescriptorHeap(),
    };

    std::vector<ID3D12DescriptorHeap*> descriptorHeaps{};
    descriptorHeaps.reserve(shaderVisibleDescriptorHeaps.size());
    for (const auto& DescriptorHeap : shaderVisibleDescriptorHeaps)
    {
        descriptorHeaps.emplace_back(DescriptorHeap->GetD3D12DescriptorHeap());
    };

    D3D12CommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
}

void FGraphicsContext::Reset()
{
    FContext::Reset();

    SetDescriptorHeaps();
}

void FGraphicsContext::ClearRenderTargetView(const FTexture* InRenderTarget, std::span<const float, 4> Color)
{
    const auto rtvDescriptorHandle =
        RHIGetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(InRenderTarget->RtvIndex);

    D3D12CommandList->ClearRenderTargetView(rtvDescriptorHandle.CpuDescriptorHandle, Color.data(), 0u, nullptr);
}

void FGraphicsContext::ClearDepthStencilView(const FTexture* Texture)
{
    const auto DsvHandle =
        RHIGetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(Texture->DsvIndex);

    D3D12CommandList->ClearDepthStencilView(DsvHandle.CpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH,
        0.0f, 1u, 0u, nullptr); // ReversedZ
}

void FGraphicsContext::SetRenderTarget(const FTexture* RenderTarget) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle =
        RHIGetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(RenderTarget->RtvIndex).CpuDescriptorHandle;

    D3D12CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
}

void FGraphicsContext::SetRenderTarget(const FTexture* RenderTarget, const FTexture* DepthStencilTexture) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle =
        RHIGetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(RenderTarget->RtvIndex).CpuDescriptorHandle;

    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle =
        RHIGetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(DepthStencilTexture->DsvIndex).CpuDescriptorHandle;

    D3D12CommandList->OMSetRenderTargets(1, &RtvHandle, TRUE, &DsvHandle);
}

void FGraphicsContext::SetRenderTargets(const std::span<const FTexture*> RenderTargets, const FTexture* DepthStencilTexture) const
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvHandles(RenderTargets.size());
    
    for (int i=0; i<RenderTargets.size(); i++)
    {
        RtvHandles[i] = RHIGetRtvDescriptorHeap()
            ->GetDescriptorHandleFromIndex(RenderTargets[i]->RtvIndex).CpuDescriptorHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle =
        RHIGetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(DepthStencilTexture->DsvIndex).CpuDescriptorHandle;

    D3D12CommandList->OMSetRenderTargets(RtvHandles.size(), RtvHandles.data(), TRUE, &DsvHandle);
}

void FGraphicsContext::SetRenderTargetDepthOnly(const FTexture* DepthStencilTexture) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle =
        RHIGetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(DepthStencilTexture->DsvIndex).CpuDescriptorHandle;

    D3D12CommandList->OMSetRenderTargets(0, nullptr, FALSE, &DsvHandle);
}

void FGraphicsContext::SetGraphicsPipelineState(const FPipelineState& PipelineState) const
{
    D3D12CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FGraphicsContext::SetComputePipelineState(const FPipelineState& PipelineState) const
{
    D3D12CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FGraphicsContext::SetRaytracingPipelineState(const FRaytracingPipelineState& PipelineState) const
{
    D3D12CommandList->SetPipelineState1(PipelineState.GetRTStateObjectPtr());
}

void FGraphicsContext::SetViewport(const D3D12_VIEWPORT& Viewport, bool bScissorRectAsSame) const
{
    D3D12CommandList->RSSetViewports(1u, &Viewport);
    if (bScissorRectAsSame)
    {
        D3D12_RECT ScissorRect{ .left = 0u, .top = 0u, .right = (LONG)Viewport.Width, .bottom = (LONG)Viewport.Height };
        SetScissorRects(ScissorRect);
    }
}

void FGraphicsContext::SetScissorRects(const D3D12_RECT& Rect) const
{
    D3D12CommandList->RSSetScissorRects(1u, &Rect);
}

void FGraphicsContext::SetPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology) const
{
    D3D12CommandList->IASetPrimitiveTopology(PrimitiveTopology);
}

void FGraphicsContext::SetIndexBuffer(const FBuffer& Buffer) const
{
    const D3D12_INDEX_BUFFER_VIEW indexBufferView = {
        .BufferLocation = Buffer.Allocation.Resource->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(Buffer.SizeInBytes),
        .Format = DXGI_FORMAT_R32_UINT,
    };

    D3D12CommandList->IASetIndexBuffer(&indexBufferView);
}

void FGraphicsContext::DrawIndexedInstanced(const uint32_t IndicesCount, const uint32_t InstanceCount) const
{
    D3D12CommandList->DrawIndexedInstanced(IndicesCount, InstanceCount, 0u, 0u, 0u);
}

void FGraphicsContext::DrawInstanced(uint32_t VertexCountPerInstance, 
    uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation) const
{
    D3D12CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FGraphicsContext::SetGraphicsRootSignature() const
{
    D3D12CommandList->SetGraphicsRootSignature(FPipelineState::StaticRootSignature.Get());
}

void FGraphicsContext::SetGraphicsRoot32BitConstants(const void* RenderResources) const
{
    D3D12CommandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FGraphicsContext::SetComputeRootSignature() const
{
    D3D12CommandList->SetComputeRootSignature(FPipelineState::StaticRootSignature.Get());
}

void FGraphicsContext::SetRaytracingComputeRootSignature() const
{
    D3D12CommandList->SetComputeRootSignature(FRaytracingPipelineState::StaticGlobalRootSignature.Get());
}

void FGraphicsContext::SetComputeRoot32BitConstants(const void* RenderResources) const
{
    D3D12CommandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FGraphicsContext::SetComputeRoot32BitConstants(UINT RootParameterIndex, UINT Num32BitValuesToSet, const void* RenderResources) const
{
    D3D12CommandList->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, RenderResources, 0u);
}

void FGraphicsContext::CopyResource(ID3D12Resource* const Destination, ID3D12Resource* const Source) const
{
    D3D12CommandList->CopyResource(Destination, Source);
}

void FGraphicsContext::Dispatch(const uint32_t ThreadGroupDimX, const uint32_t ThreadGroupDimY, const uint32_t ThreadGroupDimZ)
{
    D3D12CommandList->Dispatch(ThreadGroupDimX, ThreadGroupDimY, ThreadGroupDimZ);
}

void FGraphicsContext::BeginQuery(FQueryHeap* Heap, D3D12_QUERY_TYPE Type, uint32_t Index)
{
    D3D12CommandList->BeginQuery(Heap->GetD3D12QueryHeap(), Type, Index);
}

void FGraphicsContext::EndQuery(FQueryHeap* Heap, D3D12_QUERY_TYPE Type, uint32_t Index)
{
    D3D12CommandList->EndQuery(Heap->GetD3D12QueryHeap(), Type, Index);
}

void FGraphicsContext::ResolveQueryData(
    FQueryHeap* Heap,
    D3D12_QUERY_TYPE Type,
    UINT StartIndex,
    UINT NumQueries,
    UINT64 AlignedDestinationBufferOffset
)
{
    D3D12CommandList->ResolveQueryData(Heap->GetD3D12QueryHeap(), Type, StartIndex, NumQueries, Heap->GetQueryReadbackBuffer(), AlignedDestinationBufferOffset);
}

void FGraphicsContext::DispatchRays(D3D12_DISPATCH_RAYS_DESC& RayDesc) const
{
    D3D12CommandList->DispatchRays(&RayDesc);
}

void FGraphicsContext::SetComputeRootShaderResourceView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation)
{
    D3D12CommandList->SetComputeRootShaderResourceView(RootParameterIndex, BufferLocation);
}

void FGraphicsContext::SetComputeRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    D3D12CommandList->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}
