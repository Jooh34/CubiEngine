#include "Renderer/ScreenSpaceGI.h"
#include "Graphics/Profiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FScreenSpaceGI::FScreenSpaceGI(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
{
    FComputePipelineStateCreationDesc GenerateStochasticNormalPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/ScreenSpaceGI/GenerateStochasticNormal.hlsl",
        .PipelineName = L"Generate Stochastic Normal Pipeline"
    };

    GenerateStochasticNormalPipelineState = GraphicsDevice->CreatePipelineState(GenerateStochasticNormalPipelineDesc);

    FComputePipelineStateCreationDesc RaycastDiffusePipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/ScreenSpaceGI/RaycastDiffuse.hlsl",
        .PipelineName = L"RaycastDiffuse Pipeline"
    };

    RaycastDiffusePipelineState = GraphicsDevice->CreatePipelineState(RaycastDiffusePipelineDesc);

    FComputePipelineStateCreationDesc CompositionSSGIPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/ScreenSpaceGI/CompositionSSGI.hlsl",
        .PipelineName = L"CompositionSSGI Pipeline"
    };
    
    CompositionSSGIPipelineState = GraphicsDevice->CreatePipelineState(CompositionSSGIPipelineDesc);

    FComputePipelineStateCreationDesc SSGIDownSamplePipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/Common/DownSample.hlsl",
        .PipelineName = L"SSGI DownSample Pipeline"
    };
    SSGIDownSamplePipelineState = GraphicsDevice->CreatePipelineState(SSGIDownSamplePipelineDesc);

    FComputePipelineStateCreationDesc SSGIUpSamplePipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/Common/UpSample.hlsl",
        .PipelineName = L"SSGI UpSample Pipeline"
    };
    SSGIUpSamplePipelineState = GraphicsDevice->CreatePipelineState(SSGIUpSamplePipelineDesc);

    FComputePipelineStateCreationDesc SSGIResolvePipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/ScreenSpaceGI/SSGIResolve.hlsl",
        .PipelineName = L"SSGIResolve Pipeline"
    };
    SSGIResolvePipelineState = GraphicsDevice->CreatePipelineState(SSGIResolvePipelineDesc);

    FComputePipelineStateCreationDesc SSGIUpdateHistoryPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/ScreenSpaceGI/SSGIUpdateHistory.hlsl",
        .PipelineName = L"SSGIUpdateHistory Pipeline"
    };
    SSGIUpdateHistoryPipelineState = GraphicsDevice->CreatePipelineState(SSGIUpdateHistoryPipelineDesc);

    FComputePipelineStateCreationDesc SSGIGaussianBlurPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/Common/GaussianBlur.hlsl",
        .PipelineName = L"SSGI GaussianBlur Pipeline"
    };
    SSGIGaussianBlurPipelineState = GraphicsDevice->CreatePipelineState(SSGIGaussianBlurPipelineDesc);


    InitSizeDependantResource(GraphicsDevice, Width, Height);
}

void FScreenSpaceGI::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FScreenSpaceGI::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc StochasticNormalDesc {
       .Usage = ETextureUsage::RenderTarget,
       .Width = InWidth,
       .Height = InHeight,
       .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
       .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       .Name = L"SSGI Stochastic Normal",
    };

    StochasticNormalTexture = Device->CreateTexture(StochasticNormalDesc);

    FTextureCreationDesc HistoryTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI History Texture",
    };

    HistoryTexture = Device->CreateTexture(HistoryTextureDesc);

    FTextureCreationDesc ScreenSpaceGIDesc {
       .Usage = ETextureUsage::UAVTexture,
       .Width = InWidth,
       .Height = InHeight,
       .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
       .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       .Name = L"ScreenSpaceGI Texture",
    };
    ScreenSpaceGITexture = Device->CreateTexture(ScreenSpaceGIDesc);
    
    FTextureCreationDesc DenoisedScreenSpaceGIDesc{
       .Usage = ETextureUsage::UAVTexture,
       .Width = InWidth,
       .Height = InHeight,
       .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
       .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
       .Name = L"DenoisedScreenSpaceGI Texture",
    };
    DenoisedScreenSpaceGITexture = Device->CreateTexture(DenoisedScreenSpaceGIDesc);

    FTextureCreationDesc ResolveTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI Resolve Texture",
    };
    ResolveTexture = Device->CreateTexture(ResolveTextureDesc);

    FTextureCreationDesc HalfTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = uint32_t(InWidth/2.f),
        .Height = uint32_t(InHeight/2.f),
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI Resolve Half Texture",
    };
    HalfTexture = Device->CreateTexture(HalfTextureDesc);

    FTextureCreationDesc QuarterTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = uint32_t(InWidth / 4.f),
        .Height = uint32_t(InHeight / 4.f),
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI Resolve Quater Texture",
    };
    QuarterTexture = Device->CreateTexture(QuarterTextureDesc);

    FTextureCreationDesc BlurXTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = uint32_t(InWidth / 4.f),
        .Height = uint32_t(InHeight / 4.f),
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI BlurX Texture",
    };
    BlurXTexture = Device->CreateTexture(BlurXTextureDesc);
}

void FScreenSpaceGI::GenerateStochasticNormal(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* GBufferB, FTexture* GBufferC, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, GenerateStochasticNormal);

    GraphicsContext->AddResourceBarrier(*GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(*GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(StochasticNormalTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::GenerateStochasticNormalRenderResource RenderResources = {
        .srcTextureIndex = GBufferB->SrvIndex,
        .gbufferCTextureIndex = GBufferC->SrvIndex,
        .dstTextureIndex = StochasticNormalTexture.SrvIndex,
        .width = Width,
        .height = Height,
        .frameCount = GFrameCount,
        .stochasticNormalSamplingMethod = (uint32_t)Scene->StochasticNormalSamplingMethod,
    };

    GraphicsContext->SetComputePipelineState(GenerateStochasticNormalPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FScreenSpaceGI::RaycastDiffuse(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* HDR, FTexture* Depth, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, RaycastDiffuse);

    GraphicsContext->AddResourceBarrier(*HDR, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(*Depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(StochasticNormalTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ScreenSpaceGITexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();
    
    interlop::RaycastDiffuseRenderResource RenderResources = {
        .sceneColorTextureIndex = HDR->SrvIndex,
        .depthTextureIndex = Depth->SrvIndex,
        .stochasticNormalTextureIndex = StochasticNormalTexture.SrvIndex,
        .dstTextureIndex = ScreenSpaceGITexture.UavIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .width = Width,
        .height = Height,
        .rayLength = Scene->SSGIRayLength,
        .numSteps = (uint32_t)Scene->SSGINumSteps,
        .ssgiIntensity = Scene->SSGIIntensity,
        .compareToleranceScale = Scene->CompareToleranceScale,
    };

    GraphicsContext->SetComputePipelineState(RaycastDiffusePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FScreenSpaceGI::Denoise(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, Denoise);
    {
        GraphicsContext->AddResourceBarrier(ScreenSpaceGITexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(HalfTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::DownSampleResource RenderResources = {
            .srcTextureIndex = ScreenSpaceGITexture.SrvIndex,
            .dstTextureIndex = HalfTexture.UavIndex,
            .dstTexelSize = {1.0f / HalfTexture.Width, 1.0f / HalfTexture.Height},
        };

        GraphicsContext->SetComputePipelineState(SSGIDownSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
    {
        GraphicsContext->AddResourceBarrier(HalfTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::DownSampleResource RenderResources = {
            .srcTextureIndex = HalfTexture.SrvIndex,
            .dstTextureIndex = QuarterTexture.UavIndex,
            .dstTexelSize = {1.0f / QuarterTexture.Width, 1.0f / QuarterTexture.Height},
        };

        GraphicsContext->SetComputePipelineState(SSGIDownSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }

    //DenoiseGaussianBlur(GraphicsContext, Scene, Width, Height);

    {
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(DenoisedScreenSpaceGITexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::UpSampleResource RenderResources = {
            .srcTextureIndex = QuarterTexture.SrvIndex,
            .dstTextureIndex = DenoisedScreenSpaceGITexture.UavIndex,
            .dstTexelSize = {1.0f / DenoisedScreenSpaceGITexture.Width, 1.0f / DenoisedScreenSpaceGITexture.Height},
        };

        GraphicsContext->SetComputePipelineState(SSGIUpSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }

}

void FScreenSpaceGI::DenoiseGaussianBlur(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, DenoiseGaussianBlur);
    {
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(BlurXTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::GaussianBlurRenderResource RenderResources = {
            .srcTextureIndex = QuarterTexture.SrvIndex,
            .dstTextureIndex = BlurXTexture.UavIndex,
            .dstTexelSize = {1.0f / BlurXTexture.Width, 1.0f / BlurXTexture.Height},
            .bHorizontal = 1,
        };

        GraphicsContext->SetComputePipelineState(SSGIGaussianBlurPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
    {
        GraphicsContext->AddResourceBarrier(BlurXTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::GaussianBlurRenderResource RenderResources = {
            .srcTextureIndex = BlurXTexture.SrvIndex,
            .dstTextureIndex = QuarterTexture.UavIndex,
            .dstTexelSize = {1.0f / QuarterTexture.Width, 1.0f / QuarterTexture.Height},
            .bHorizontal = 0,
        };

        GraphicsContext->SetComputePipelineState(SSGIGaussianBlurPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(Width / 8.0f), 1u),
            max((uint32_t)std::ceil(Height / 8.0f), 1u),
        1);
    }
}

void FScreenSpaceGI::Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* VelocityTexture, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, ResolveSSGI);

    GraphicsContext->AddResourceBarrier(DenoisedScreenSpaceGITexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(*VelocityTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAResolveRenderResource RenderResources = {
        .sceneTextureIndex = DenoisedScreenSpaceGITexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.SrvIndex,
        .velocityTextureIndex = VelocityTexture->SrvIndex,
        .dstTextureIndex = ResolveTexture.UavIndex,
        .width = Width,
        .height = Height,
        .historyFrameCount = HistoryFrameCount,
    };

    GraphicsContext->SetComputePipelineState(SSGIResolvePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}

void FScreenSpaceGI::UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, SSGI_UpdateHistory);

    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TemporalAAUpdateHistoryRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.UavIndex,
        .width = Width,
        .height = Height,
    };

    GraphicsContext->SetComputePipelineState(SSGIUpdateHistoryPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);

    HistoryFrameCount++;
}

void FScreenSpaceGI::CompositionSSGI(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* HDR, FTexture* GBufferA, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, CompositionSSGI);

    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(*GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(*HDR, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::CompositionSSGIRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture.SrvIndex,
        .gbufferAIndex = GBufferA->SrvIndex,
        .dstTextureIndex = HDR->UavIndex,
        .width = Width,
        .height = Height,
    };

    GraphicsContext->SetComputePipelineState(CompositionSSGIPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}
