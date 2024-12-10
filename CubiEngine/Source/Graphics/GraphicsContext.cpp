#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/DescriptorHeap.h"

FGraphicsContext::FGraphicsContext(FGraphicsDevice* const Device)
    :Device(Device)
{
    ThrowIfFailed(Device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&CommandAllocator)));

    ThrowIfFailed(Device->GetDevice()->CreateCommandList(
        0u, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));

    SetDescriptorHeaps();

    ThrowIfFailed(CommandList->Close());
}

void FGraphicsContext::SetDescriptorHeaps() const
{
    // As all graphics context's require to set the descriptor heap before hand, the user has option to set them
    // manually (for explicitness) or just let the constructor take care of this.
    const std::array<const FDescriptorHeap* const, 2u> shaderVisibleDescriptorHeaps = {
        Device->GetCbvSrvUavDescriptorHeap(),
        Device->GetSamplerDescriptorHeap(),
    };

    std::vector<ID3D12DescriptorHeap*> descriptorHeaps{};
    descriptorHeaps.reserve(shaderVisibleDescriptorHeaps.size());
    for (const auto& heap : shaderVisibleDescriptorHeaps)
    {
        descriptorHeaps.emplace_back(heap->GetDescriptorHeap());
    };

    CommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());
}

void FGraphicsContext::Reset()
{
    FContext::Reset();

    SetDescriptorHeaps();
}

void FGraphicsContext::ClearRenderTargetView(const FTexture& InRenderTarget, std::span<const float, 4> Color)
{
    const auto rtvDescriptorHandle =
        Device->GetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(InRenderTarget.RtvIndex);

    CommandList->ClearRenderTargetView(rtvDescriptorHandle.CpuDescriptorHandle, Color.data(), 0u, nullptr);
}

void FGraphicsContext::ClearDepthStencilView(const FTexture& Texture)
{
    const auto DsvHandle =
        Device->GetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(Texture.DsvIndex);

    CommandList->ClearDepthStencilView(DsvHandle.CpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH,
        1.0f, 1u, 0u, nullptr);
}

void FGraphicsContext::SetRenderTarget(const FTexture& RenderTarget) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle =
        Device->GetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(RenderTarget.RtvIndex).CpuDescriptorHandle;

    CommandList->OMSetRenderTargets(1, &RtvHandle, FALSE, nullptr);
}

void FGraphicsContext::SetRenderTarget(const FTexture& RenderTarget, const FTexture& DepthStencilTexture) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle =
        Device->GetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(RenderTarget.RtvIndex).CpuDescriptorHandle;

    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle =
        Device->GetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(DepthStencilTexture.DsvIndex).CpuDescriptorHandle;

    CommandList->OMSetRenderTargets(1, &RtvHandle, TRUE, &DsvHandle);
}

void FGraphicsContext::SetRenderTargets(const std::span<const FTexture> RenderTargets, const FTexture& DepthStencilTexture) const
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvHandles(RenderTargets.size());
    
    for (int i=0; i<RenderTargets.size(); i++)
    {
        RtvHandles[i] = Device->GetRtvDescriptorHeap()
            ->GetDescriptorHandleFromIndex(RenderTargets[i].RtvIndex).CpuDescriptorHandle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle =
        Device->GetDsvDescriptorHeap()->GetDescriptorHandleFromIndex(DepthStencilTexture.DsvIndex).CpuDescriptorHandle;

    CommandList->OMSetRenderTargets(RtvHandles.size(), RtvHandles.data(), TRUE, &DsvHandle);
}

void FGraphicsContext::SetGraphicsPipelineState(const FPipelineState& PipelineState) const
{
    CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FGraphicsContext::SetComputePipelineState(const FPipelineState& PipelineState) const
{
    CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FGraphicsContext::SetViewport(const D3D12_VIEWPORT& Viewport) const
{
    static D3D12_RECT scissorRect{ .left = 0u, .top = 0u, .right = (LONG)Viewport.Width, .bottom = (LONG)Viewport.Height };

    CommandList->RSSetViewports(1u, &Viewport);
    CommandList->RSSetScissorRects(1u, &scissorRect);
}

void FGraphicsContext::SetPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology) const
{
    CommandList->IASetPrimitiveTopology(PrimitiveTopology);
}

void FGraphicsContext::SetIndexBuffer(const FBuffer& Buffer) const
{
    const D3D12_INDEX_BUFFER_VIEW indexBufferView = {
        .BufferLocation = Buffer.Allocation.Resource->GetGPUVirtualAddress(),
        .SizeInBytes = static_cast<UINT>(Buffer.SizeInBytes),
        .Format = DXGI_FORMAT_R16_UINT,
    };

    CommandList->IASetIndexBuffer(&indexBufferView);
}

void FGraphicsContext::DrawIndexedInstanced(const uint32_t IndicesCount, const uint32_t InstanceCount) const
{
    CommandList->DrawIndexedInstanced(IndicesCount, InstanceCount, 0u, 0u, 0u);
}

void FGraphicsContext::DrawInstanced(uint32_t VertexCountPerInstance, 
    uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation) const
{
    CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void FGraphicsContext::SetGraphicsRootSignature() const
{
    CommandList->SetGraphicsRootSignature(FPipelineState::StaticRootSignature.Get());
}

void FGraphicsContext::SetGraphicsRoot32BitConstants(const void* RenderResources) const
{
    CommandList->SetGraphicsRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FGraphicsContext::SetComputeRootSignature() const
{
    CommandList->SetComputeRootSignature(FPipelineState::StaticRootSignature.Get());
}

void FGraphicsContext::SetComputeRoot32BitConstants(const void* RenderResources) const
{
    CommandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FGraphicsContext::CopyResource(ID3D12Resource* const Destination, ID3D12Resource* const Source) const
{
    CommandList->CopyResource(Destination, Source);
}

void FGraphicsContext::Dispatch(const uint32_t ThreadGroupDimX, const uint32_t ThreadGroupDimY, const uint32_t ThreadGroupDimZ)
{
    CommandList->Dispatch(ThreadGroupDimX, ThreadGroupDimY, ThreadGroupDimZ);
}

