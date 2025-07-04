#include "Renderer/PostProcess.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Profiler.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FPostProcess::FPostProcess(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
    : FRenderPass(GraphicsDevice, Width, Height)
{
    FComputePipelineStateCreationDesc TonemappingPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/Tonemapping.hlsl",
        },
        .PipelineName = L"Tonemapping Pipeline"
    };
    TonemappingPipelineState = GraphicsDevice->CreatePipelineState(TonemappingPipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/DebugVisualize.hlsl",
        },
        .PipelineName = L"DebugVisualize Pipeline"
    };
    DebugVisualizePipeline = GraphicsDevice->CreatePipelineState(DebugVisualizePipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizeDepthPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/DebugVisualizeDepth.hlsl",
        },
        .PipelineName = L"DebugVisualizeDepth Pipeline"
    };
    DebugVisualizeDepthPipeline = GraphicsDevice->CreatePipelineState(DebugVisualizeDepthPipelineDesc);
}

void FPostProcess::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
}

void FPostProcess::Tonemapping(FGraphicsContext* const GraphicsContext, FScene* Scene,
    FTexture& HDRTexture, FTexture& LDRTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, Tonemapping);

    GraphicsContext->AddResourceBarrier(HDRTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(LDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TonemappingRenderResources RenderResources = {
        .srcTextureIndex = HDRTexture.SrvIndex,
        .dstTextureIndex = LDRTexture.UavIndex,
        .bloomTextureIndex = INVALID_INDEX_U32,
        .width = Width,
        .height = Height,
        .toneMappingMethod = (uint)Scene->ToneMappingMethod,
        .bGammaCorrection = Scene->bGammaCorrection,
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

void FPostProcess::DebugVisualize(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture& SrcTexture, FTexture& TargetTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, DebugVisualize);

    GraphicsContext->AddResourceBarrier(TargetTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();
    
    if (SrcTexture.IsDepthFormat())
    {
        interlop::DebugVisualizeDepthRenderResources RenderResources = {
            .srcTextureIndex = SrcTexture.SrvIndex,
            .dstTextureIndex = TargetTexture.UavIndex,
            .visDebugMin = Scene->VisualizeDebugMin,
            .visDebugMax = Scene->VisualizeDebugMax,
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
            .srcTextureIndex = SrcTexture.SrvIndex,
            .dstTextureIndex = TargetTexture.UavIndex,
            .visDebugMin = Scene->VisualizeDebugMin,
            .visDebugMax = Scene->VisualizeDebugMax,
        };

        GraphicsContext->SetComputePipelineState(DebugVisualizePipeline);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
}
