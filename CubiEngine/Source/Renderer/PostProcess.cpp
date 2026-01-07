#include "Renderer/PostProcess.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Profiler.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FPostProcess::FPostProcess(uint32_t Width, uint32_t Height)
    : FRenderPass(Width, Height)
{
    FComputePipelineStateCreationDesc TonemappingPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/Tonemapping.hlsl",
        },
        .PipelineName = L"Tonemapping Pipeline"
    };
    TonemappingPipelineState = RHICreatePipelineState(TonemappingPipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/DebugVisualize.hlsl",
        },
        .PipelineName = L"DebugVisualize Pipeline"
    };
    DebugVisualizePipeline = RHICreatePipelineState(DebugVisualizePipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizeDepthPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/DebugVisualizeDepth.hlsl",
        },
        .PipelineName = L"DebugVisualizeDepth Pipeline"
    };
    DebugVisualizeDepthPipeline = RHICreatePipelineState(DebugVisualizeDepthPipelineDesc);
}

void FPostProcess::InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight)
{
}

void FPostProcess::Tonemapping(FGraphicsContext* const GraphicsContext, FScene* Scene,
    FTexture* HDRTexture, FTexture* LDRTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, Tonemapping);

    GraphicsContext->AddResourceBarrier(HDRTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(LDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TonemappingRenderResources RenderResources = {
        .srcTextureIndex = HDRTexture->SrvIndex,
        .dstTextureIndex = LDRTexture->UavIndex,
        .bloomTextureIndex = INVALID_INDEX_U32,
        .width = Width,
        .height = Height,
        .toneMappingMethod = (uint)Scene->GetRenderSettings().ToneMappingMethod,
        .bGammaCorrection = Scene->GetRenderSettings().bGammaCorrection,
        .averageLuminanceBufferIndex = INVALID_INDEX_U32,
    };

    GraphicsContext->SetComputePipelineState(TonemappingPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FPostProcess::DebugVisualize(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* SrcTexture, FTexture* TargetTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, DebugVisualize);

    GraphicsContext->AddResourceBarrier(TargetTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();
    
    if (SrcTexture->IsDepthFormat())
    {
        interlop::DebugVisualizeDepthRenderResources RenderResources = {
            .srcTextureIndex = SrcTexture->SrvIndex,
            .dstTextureIndex = TargetTexture->UavIndex,
            .visDebugMin = Scene->GetRenderSettings().VisualizeDebugMin,
            .visDebugMax = Scene->GetRenderSettings().VisualizeDebugMax,
        };

        GraphicsContext->SetComputePipelineState(DebugVisualizeDepthPipeline);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
    else
    {
        interlop::DebugVisualizeRenderResources RenderResources = {
            .srcTextureIndex = SrcTexture->SrvIndex,
            .dstTextureIndex = TargetTexture->UavIndex,
            .visDebugMin = Scene->GetRenderSettings().VisualizeDebugMin,
            .visDebugMax = Scene->GetRenderSettings().VisualizeDebugMax,
        };

        GraphicsContext->SetComputePipelineState(DebugVisualizePipeline);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
}
