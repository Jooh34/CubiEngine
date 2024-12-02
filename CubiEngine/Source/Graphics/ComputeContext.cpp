#include "Graphics/ComputeContext.h"
#include "Graphics/GraphicsDevice.h"

FComputeContext::FComputeContext(FGraphicsDevice* const Device)
    :Device(Device)
{
    ThrowIfFailed(Device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
        IID_PPV_ARGS(&CommandAllocator)));

    ThrowIfFailed(Device->GetDevice()->CreateCommandList(
        0u, D3D12_COMMAND_LIST_TYPE_COMPUTE, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));

    SetDescriptorHeaps();

    ThrowIfFailed(CommandList->Close());
}

void FComputeContext::SetDescriptorHeaps() const
{
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

void FComputeContext::SetComputeRootSignatureAndPipeline(const FPipelineState& PipelineState) const
{
    CommandList->SetComputeRootSignature(FPipelineState::StaticRootSignature.Get());
    CommandList->SetPipelineState(PipelineState.PipelineStateObject.Get());
}

void FComputeContext::Set32BitComputeConstants(const void* RenderResources) const
{
    CommandList->SetComputeRoot32BitConstants(0u, NUMBER_32_BIT_CONSTANTS, RenderResources, 0u);
}

void FComputeContext::Dispatch(const uint32_t ThreadGroupX, const uint32_t ThreadGroupY, const uint32_t ThreadGroupZ) const
{
    CommandList->Dispatch(ThreadGroupX, ThreadGroupY, ThreadGroupZ);

}
