#include "Renderer/PostProcess.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FPostProcess::FPostProcess(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
{
    FComputePipelineStateCreationDesc TonemappingPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/Tonemapping.hlsl",
        .PipelineName = L"Tonemapping Pipeline"
    };
    TonemappingPipelineState = GraphicsDevice->CreatePipelineState(TonemappingPipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizeCubeMapPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/DebugVisualizeCubeMap.hlsl",
        .PipelineName = L"DebugVisualizeCubeMap Pipeline"
    };
    DebugVisualizeCubeMapPipeline = GraphicsDevice->CreatePipelineState(DebugVisualizeCubeMapPipelineDesc);
    
    FComputePipelineStateCreationDesc DebugVisualizeDepthPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/PostProcess/DebugVisualizeDepth.hlsl",
        .PipelineName = L"DebugVisualizeDepth Pipeline"
    };
    DebugVisualizeDepthPipeline = GraphicsDevice->CreatePipelineState(DebugVisualizeDepthPipelineDesc);

    InitSizeDependantResource(GraphicsDevice, Width, Height);
}

void FPostProcess::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc TextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R10G10B10A2_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"LDR Texture",
    };

    LDRTexture = Device->CreateTexture(TextureDesc);
}

void FPostProcess::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FPostProcess::Tonemapping(FGraphicsContext* const GraphicsContext,
    FTexture& HDRTexture, uint32_t Width, uint32_t Height)
{
    //GraphicsContext->AddResourceBarrier(SrcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
    GraphicsContext->AddResourceBarrier(LDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TonemappingRenderResources RenderResources = {
        .srcTextureIndex = HDRTexture.SrvIndex,
        .dstTextureIndex = LDRTexture.UavIndex,
        .width = Width,
        .height = Height,
    };

    GraphicsContext->SetComputePipelineState(TonemappingPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FPostProcess::DebugVisualize(FGraphicsContext* const GraphicsContext, FTexture& SrcTexture, uint32_t Width, uint32_t Height)
{
    GraphicsContext->AddResourceBarrier(LDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    if (SrcTexture.Usage == ETextureUsage::CubeMap)
    {
        interlop::DebugVisualizeCubeMapRenderResources RenderResources = {
            .srcTextureIndex = SrcTexture.SrvIndex,
            .dstTextureIndex = LDRTexture.UavIndex,
        };

        GraphicsContext->SetComputePipelineState(DebugVisualizeCubeMapPipeline);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,6)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
    else if (SrcTexture.Usage == ETextureUsage::DepthStencil)
    {
        interlop::DebugVisualizeDepthRenderResources RenderResources = {
            .srcTextureIndex = SrcTexture.SrvIndex,
            .dstTextureIndex = LDRTexture.UavIndex,
        };

        GraphicsContext->SetComputePipelineState(DebugVisualizeDepthPipeline);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,6)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
}
