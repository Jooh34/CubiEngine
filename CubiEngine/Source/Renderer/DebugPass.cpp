#include "Renderer/DebugPass.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FDebugPass::FDebugPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
	:FRenderPass(GraphicsDevice, Width, Height)
{
    FComputePipelineStateCreationDesc PipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/Copy.hlsl",
        },
        .PipelineName = L"Debug Copy Pipeline"
    };

    CopyPipelineState = GraphicsDevice->CreatePipelineState(PipelineDesc);
}

void FDebugPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc TextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R10G10B10A2_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        .Name = L"Copy Texture",
    };

    TextureForCopy = Device->CreateTexture(TextureDesc);
}

void FDebugPass::Copy(FGraphicsContext* const GraphicsContext, FTexture& SrcTexture, FTexture& DstTexture, uint32_t Width, uint32_t Height)
{
    // Resource Barrier
    GraphicsContext->AddResourceBarrier(SrcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
    GraphicsContext->AddResourceBarrier(DstTexture, D3D12_RESOURCE_STATE_COPY_DEST);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::CopyRenderResources RenderResources = {
        .srcTextureIndex = SrcTexture.SrvIndex,
        .dstTextureIndex = DstTexture.UavIndex,
    };

    GraphicsContext->SetComputePipelineState(CopyPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}
