#include "Renderer/DebugPass.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FDebugPass::FDebugPass(FGraphicsDevice* const GraphicsDevice)
{
    FComputePipelineStateCreationDesc PipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/Copy.hlsl",
        .PipelineName = L"Debug Copy Pipeline"
    };

    CopyPipelineState = GraphicsDevice->CreatePipelineState(PipelineDesc);
}

void FDebugPass::Copy(FGraphicsContext* const GraphicsContext, const FTexture& SrcTexture, const FTexture& DstTexture, uint32_t Width, uint32_t Height)
{
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
    // GraphicsContext->Dispatch();
}
