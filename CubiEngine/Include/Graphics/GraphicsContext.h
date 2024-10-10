#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;

class FGraphicsContext : public FContext
{
public:
    FGraphicsContext(FGraphicsDevice* const Device);
    void SetDescriptorHeaps() const;
    void Reset() const;

    void ClearRenderTargetView(const FTexture& InRenderTarget, std::span<const float, 4> Color);
    void ClearDepthStencilView(const FTexture& Texture);

    void SetRenderTarget(const FTexture& RenderTarget, const FTexture& DepthStencilTexture) const;
    void SetRenderTargets(const std::span<const FTexture> RenderTargets, const FTexture& DepthStencilTexture) const;

    void SetGraphicsPipelineState(const FPipelineState& PipelineState) const;

    void SetViewport(const D3D12_VIEWPORT& Viewport) const;
    void SetPrimitiveTopologyLayout(const D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology) const;

    void SetIndexBuffer(const FBuffer& Buffer) const;
    void DrawIndexedInstanced(const uint32_t IndicesCount, const uint32_t InstanceCount = 1u) const;

private:
    FGraphicsDevice* Device;
};