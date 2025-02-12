#include "Renderer/TemporalAA.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Profiler.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FTemporalAA::FTemporalAA(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
{
    FComputePipelineStateCreationDesc TemporalAAResolvePipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/TemporalAAResolve.hlsl",
        .PipelineName = L"TemporalAAResolve Pipeline"
    };
    TemporalAAResolvePipelineState = GraphicsDevice->CreatePipelineState(TemporalAAResolvePipelineDesc);

    FComputePipelineStateCreationDesc TemporalAAUpdateHistoryPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/TemporalAAUpdateHistory.hlsl",
        .PipelineName = L"TemporalAAUpdateHistory Pipeline"
    };
    TemporalAAUpdateHistoryPipelineState = GraphicsDevice->CreatePipelineState(TemporalAAUpdateHistoryPipelineDesc);

    InitSizeDependantResource(GraphicsDevice, Width, Height);
}

void FTemporalAA::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FTemporalAA::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc HistoryTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"TemporalAA History Texture",
    };

    HistoryTexture = Device->CreateTexture(HistoryTextureDesc);

    FTextureCreationDesc ResolveTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"TemporalAA Resolve Texture",
    };

    ResolveTexture = Device->CreateTexture(ResolveTextureDesc);
}

void FTemporalAA::Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene,
    FTexture& HDRTexture, FTexture& VelocityTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, TemporalAAResolve);

    GraphicsContext->AddResourceBarrier(HDRTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(VelocityTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAResolveRenderResource RenderResources = {
        .sceneTextureIndex = HDRTexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.SrvIndex,
        .velocityTextureIndex = VelocityTexture.SrvIndex,
        .dstTextureIndex = ResolveTexture.UavIndex,
        .width = Width,
        .height = Height,
        .historyFrameCount = HistoryFrameCount,
    };

    GraphicsContext->SetComputePipelineState(TemporalAAResolvePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FTemporalAA::UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, TemporalAAUpdateHistory);

    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAUpdateHistoryRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.UavIndex,
        .width = Width,
        .height = Height,
    };

    GraphicsContext->SetComputePipelineState(TemporalAAUpdateHistoryPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);

    HistoryFrameCount++;
}
