#include "Graphics/ComputeContext.h"
#include "Graphics/D3D12DynamicRHI.h"

FComputeContext::FComputeContext()
{
    ThrowIfFailed(RHIGetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
        IID_PPV_ARGS(&D3D12CommandAllocator)));

    ThrowIfFailed(RHIGetDevice()->CreateCommandList(
        0u, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&D3D12CommandList)));

    SetDescriptorHeaps();

    ThrowIfFailed(D3D12CommandList->Close());
}

void FComputeContext::SetDescriptorHeaps() const
{
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

void FComputeContext::Reset()
{
    FContext::Reset();

    SetDescriptorHeaps();
}

void FComputeContext::SetComputeRootSignatureAndPipeline(const FPipelineState& PipelineState) const
{
    D3D12CommandList->SetComputeRootSignature(FPipelineState::StaticRootSignature.Get());
    D3D12CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FComputeContext::Set32BitComputeConstants(const void* RenderResources) const
{
    D3D12CommandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FComputeContext::Dispatch(const uint32_t ThreadGroupX, const uint32_t ThreadGroupY, const uint32_t ThreadGroupZ) const
{
    D3D12CommandList->Dispatch(ThreadGroupX, ThreadGroupY, ThreadGroupZ);
}
