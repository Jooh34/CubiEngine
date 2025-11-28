#include "Renderer/TemporalAA.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Profiler.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FTemporalAAPass::FTemporalAAPass(uint32_t Width, uint32_t Height)
    : FRenderPass(Width, Height),
        HistoryFrameCount(0)
{
    FComputePipelineStateCreationDesc TemporalAAResolvePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/TemporalAAResolve.hlsl",
        },
        .PipelineName = L"TemporalAA Resolve Pipeline"
    };
    TemporalAAResolvePipelineState = RHICreatePipelineState(TemporalAAResolvePipelineDesc);

    FComputePipelineStateCreationDesc TemporalAAUpdateHistoryPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/PostProcess/TemporalAAUpdateHistory.hlsl",
        },
        .PipelineName = L"TemporalAA UpdateHistory Pipeline"
    };
    TemporalAAUpdateHistoryPipelineState = RHICreatePipelineState(TemporalAAUpdateHistoryPipelineDesc);
}

void FTemporalAAPass::InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc HistoryTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"TemporalAA History Texture",
    };

    HistoryTexture = RHICreateTexture(HistoryTextureDesc);

    FTextureCreationDesc ResolveTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"TemporalAA Resolve Texture",
    };

    ResolveTexture = RHICreateTexture(ResolveTextureDesc);
}

void FTemporalAAPass::Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, TemporalAAResolve);

    GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.VelocityTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAResolveRenderResource RenderResources = {
        .sceneTextureIndex = SceneTexture.HDRTexture->SrvIndex,
        .historyTextureIndex = HistoryTexture->SrvIndex,
        .velocityTextureIndex = SceneTexture.VelocityTexture->SrvIndex,
        .dstTextureIndex = ResolveTexture->UavIndex,
        .dstTexelSize = {1.0f / SceneTexture.Size.Width, 1.0f / SceneTexture.Size.Height},
        .historyFrameCount = HistoryFrameCount,
    };

    GraphicsContext->SetComputePipelineState(TemporalAAResolvePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(SceneTexture.Size.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(SceneTexture.Size.Height / 8.0f), 1u),
    1);
}

void FTemporalAAPass::UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene)
{
    SCOPED_NAMED_EVENT(GraphicsContext, TemporalAAUpdateHistory);

    GraphicsContext->AddResourceBarrier(ResolveTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAUpdateHistoryRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture->SrvIndex,
        .historyTextureIndex = HistoryTexture->UavIndex,
        .dstTexelSize = {1.0f / HistoryTexture->Width, 1.0f / HistoryTexture->Height},
    };

    GraphicsContext->SetComputePipelineState(TemporalAAUpdateHistoryPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(HistoryTexture->Width / 8.0f), 1u),
        max((uint32_t)std::ceil(HistoryTexture->Height / 8.0f), 1u),
    1);

    HistoryFrameCount++;
}
