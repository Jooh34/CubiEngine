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

void FGraphicsContext::Reset() const
{
    FContext::Reset();

    SetDescriptorHeaps();
}

void FGraphicsContext::ClearRenderTargetView(const Texture& InRenderTarget, std::span<const float, 4> Color)
{
    const auto rtvDescriptorHandle =
        Device->GetRtvDescriptorHeap()->GetDescriptorHandleFromIndex(InRenderTarget.RtvIndex);

    CommandList->ClearRenderTargetView(rtvDescriptorHandle.CpuDescriptorHandle, Color.data(), 0u, nullptr);
}