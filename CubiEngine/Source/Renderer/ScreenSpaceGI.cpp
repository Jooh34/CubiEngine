#include "Renderer/ScreenSpaceGI.h"
#include "Graphics/Profiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "CubiMath.h"
#include "ShaderInterlop/RenderResources.hlsli"

FScreenSpaceGIPass::FScreenSpaceGIPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
	: FRenderPass(GraphicsDevice, Width, Height),
	HistoryFrameCount(0)
{
    FComputePipelineStateCreationDesc GenerateStochasticNormalPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/ScreenSpaceGI/GenerateStochasticNormal.hlsl",
        },
        .PipelineName = L"Generate Stochastic Normal Pipeline"
    };

    GenerateStochasticNormalPipelineState = GraphicsDevice->CreatePipelineState(GenerateStochasticNormalPipelineDesc);

    FComputePipelineStateCreationDesc RaycastDiffusePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/ScreenSpaceGI/RaycastDiffuse.hlsl",
        },
        .PipelineName = L"RaycastDiffuse Pipeline"
    };

    RaycastDiffusePipelineState = GraphicsDevice->CreatePipelineState(RaycastDiffusePipelineDesc);

    FComputePipelineStateCreationDesc CompositionSSGIPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/ScreenSpaceGI/CompositionSSGI.hlsl",
        },
        .PipelineName = L"CompositionSSGI Pipeline"
    };
    
    CompositionSSGIPipelineState = GraphicsDevice->CreatePipelineState(CompositionSSGIPipelineDesc);

    FComputePipelineStateCreationDesc SSGIDownSamplePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/Common/DownSample.hlsl",
        },
        .PipelineName = L"SSGI DownSample Pipeline"
    };
    SSGIDownSamplePipelineState = GraphicsDevice->CreatePipelineState(SSGIDownSamplePipelineDesc);

    FComputePipelineStateCreationDesc SSGIUpSamplePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/Common/UpSample.hlsl",
        },
        .PipelineName = L"SSGI UpSample Pipeline"
    };
    SSGIUpSamplePipelineState = GraphicsDevice->CreatePipelineState(SSGIUpSamplePipelineDesc);

    FComputePipelineStateCreationDesc SSGIResolvePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/ScreenSpaceGI/SSGIResolve.hlsl",
        },
        .PipelineName = L"SSGIResolve Pipeline"
    };
    SSGIResolvePipelineState = GraphicsDevice->CreatePipelineState(SSGIResolvePipelineDesc);

    FComputePipelineStateCreationDesc SSGIUpdateHistoryPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/ScreenSpaceGI/SSGIUpdateHistory.hlsl",
        },
        .PipelineName = L"SSGIUpdateHistory Pipeline"
    };
    SSGIUpdateHistoryPipelineState = GraphicsDevice->CreatePipelineState(SSGIUpdateHistoryPipelineDesc);

    FComputePipelineStateCreationDesc SSGIGaussianBlurWPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/Common/GaussianBlurW.hlsl",
        },
        .PipelineName = L"SSGI GaussianBlur Pipeline"
    };
    SSGIGaussianBlurWPipelineState = GraphicsDevice->CreatePipelineState(SSGIGaussianBlurWPipelineDesc);
}

void FScreenSpaceGIPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
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
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"SSGI History Texture",
    };

    HistoryTexture = Device->CreateTexture(HistoryTextureDesc);
    {
        // initialize history texture as black
        FGraphicsContext* GraphicsContext = Device->GetCurrentGraphicsContext();
        GraphicsContext->Reset();
        GraphicsContext->ClearRenderTargetView(HistoryTexture, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
        Device->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
        
        // wait immediately
        uint64_t FenceValue = Device->GetDirectCommandQueue()->Signal();
        Device->GetDirectCommandQueue()->WaitForFenceValue(FenceValue);
    }

    FTextureCreationDesc HistroyNumFrameAccumulatedDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSGI HistroyNumFrameAccumulated Texture",
    };

    HistroyNumFrameAccumulated = Device->CreateTexture(HistroyNumFrameAccumulatedDesc);
   
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

void FScreenSpaceGIPass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, ScreenSpaceGI);

    RaycastDiffuse(GraphicsContext, Scene, SceneTexture);
    Denoise(GraphicsContext, Scene, SceneTexture);
    Resolve(GraphicsContext, Scene, SceneTexture);
    UpdateHistory(GraphicsContext, Scene);
    CompositionSSGI(GraphicsContext, Scene, SceneTexture);
}

void FScreenSpaceGIPass::RaycastDiffuse(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, RaycastDiffuse);

    GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(StochasticNormalTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ScreenSpaceGITexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();
    
    interlop::RaycastDiffuseRenderResource RenderResources = {
        .sceneColorTextureIndex = SceneTexture.HDRTexture.SrvIndex,
        .depthTextureIndex = SceneTexture.DepthTexture.SrvIndex,
        .GBufferBTextureIndex = SceneTexture.GBufferB.SrvIndex,
        .dstTextureIndex = ScreenSpaceGITexture.UavIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .width = SceneTexture.Size.Width,
        .height = SceneTexture.Size.Height,
        .rayLength = Scene->SSGIRayLength,
        .numSteps = (uint32_t)Scene->SSGINumSteps,
        .ssgiIntensity = Scene->SSGIIntensity,
        .compareToleranceScale = Scene->CompareToleranceScale,
        .frameCount = GFrameCount,
        .stochasticNormalSamplingMethod = (uint32_t)Scene->StochasticNormalSamplingMethod,
        .numSamples = (uint32_t)Scene->SSGINumSamples,
    };

    GraphicsContext->SetComputePipelineState(RaycastDiffusePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(SceneTexture.Size.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(SceneTexture.Size.Height / 8.0f), 1u),
    1);
}

void FScreenSpaceGIPass::Denoise(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, Denoise);
    {
        GraphicsContext->AddResourceBarrier(ScreenSpaceGITexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(HalfTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::DownSampleRenderResource RenderResources = {
            .srcTextureIndex = ScreenSpaceGITexture.SrvIndex,
            .dstTextureIndex = HalfTexture.UavIndex,
            .dstTexelSize = {1.0f / HalfTexture.Width, 1.0f / HalfTexture.Height},
        };

        GraphicsContext->SetComputePipelineState(SSGIDownSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(SceneTexture.Size.Width / 8.0f), 1u),
            max((uint32_t)std::ceil(SceneTexture.Size.Height / 8.0f), 1u),
        1);
    }
    {
        GraphicsContext->AddResourceBarrier(HalfTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::DownSampleRenderResource RenderResources = {
            .srcTextureIndex = HalfTexture.SrvIndex,
            .dstTextureIndex = QuarterTexture.UavIndex,
            .dstTexelSize = {1.0f / QuarterTexture.Width, 1.0f / QuarterTexture.Height},
        };

        GraphicsContext->SetComputePipelineState(SSGIDownSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(QuarterTexture.Width / 8.0f), 1u),
            max((uint32_t)std::ceil(QuarterTexture.Height / 8.0f), 1u),
        1);
    }

    DenoiseGaussianBlur(GraphicsContext, Scene, SceneTexture);

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
            max((uint32_t)std::ceil(DenoisedScreenSpaceGITexture.Width / 8.0f), 1u),
            max((uint32_t)std::ceil(DenoisedScreenSpaceGITexture.Height / 8.0f), 1u),
        1);
    }

}

void FScreenSpaceGIPass::DenoiseGaussianBlur(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, DenoiseGaussianBlur);

    float GaussianBlurWeight[MAX_GAUSSIAN_KERNEL_SIZE];
    int KernelSize = Scene->SSGIGaussianKernelSize;
    float StdDev = Scene->SSGIGaussianStdDev;

    CreateGaussianBlurWeight(GaussianBlurWeight, KernelSize, StdDev);

    {
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(BlurXTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();
        
        interlop::GaussianBlurWRenderResource RenderResources = {
            .srcTextureIndex = QuarterTexture.SrvIndex,
            .dstTextureIndex = BlurXTexture.UavIndex,
            .dstTexelSize = {1.0f / BlurXTexture.Width, 1.0f / BlurXTexture.Height},
            .additiveTextureIndex = INVALID_INDEX_U32,
            .bHorizontal = 1,
            .kernelSize = (uint)KernelSize,
        };
        for (int i = 0; i < MAX_GAUSSIAN_KERNEL_SIZE / 4; i++)
        {
            RenderResources.weights[i] = XMFLOAT4(GaussianBlurWeight[i * 4],
                GaussianBlurWeight[i * 4 + 1],
                GaussianBlurWeight[i * 4 + 2],
                GaussianBlurWeight[i * 4 + 3]
            );
        }

        GraphicsContext->SetComputePipelineState(SSGIGaussianBlurWPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(BlurXTexture.Width / 8.0f), 1u),
            max((uint32_t)std::ceil(BlurXTexture.Height / 8.0f), 1u),
        1);
    }
    {
        GraphicsContext->AddResourceBarrier(BlurXTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(QuarterTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();
        
        interlop::GaussianBlurWRenderResource RenderResources = {
            .srcTextureIndex = BlurXTexture.SrvIndex,
            .dstTextureIndex = QuarterTexture.UavIndex,
            .dstTexelSize = {1.0f / QuarterTexture.Width, 1.0f / QuarterTexture.Height},
            .additiveTextureIndex = INVALID_INDEX_U32,
            .bHorizontal = 0,
            .kernelSize = (uint)KernelSize,
        };
        for (int i = 0; i < MAX_GAUSSIAN_KERNEL_SIZE / 4; i++)
        {
            RenderResources.weights[i] = XMFLOAT4(GaussianBlurWeight[i * 4],
                GaussianBlurWeight[i * 4 + 1],
                GaussianBlurWeight[i * 4 + 2],
                GaussianBlurWeight[i * 4 + 3]
            );
        }

        GraphicsContext->SetComputePipelineState(SSGIGaussianBlurWPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(QuarterTexture.Width / 8.0f), 1u),
            max((uint32_t)std::ceil(QuarterTexture.Height / 8.0f), 1u),
        1);
    }
}

void FScreenSpaceGIPass::Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, ResolveSSGI);

    GraphicsContext->AddResourceBarrier(DenoisedScreenSpaceGITexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.VelocityTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.PrevDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::SSGIResolveRenderResource RenderResources = {
        .denoisedTextureIndex = DenoisedScreenSpaceGITexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.SrvIndex,
        .velocityTextureIndex = SceneTexture.VelocityTexture.SrvIndex,
        .prevDepthTextureIndex = SceneTexture.PrevDepthTexture.SrvIndex,
        .depthTextureIndex = SceneTexture.DepthTexture.SrvIndex,
        .dstTextureIndex = ResolveTexture.UavIndex,
        .numFramesAccumulatedTextureIndex = HistroyNumFrameAccumulated.UavIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .dstTexelSize = {1.0f / ResolveTexture.Width, 1.0f / ResolveTexture.Height},
        .maxHistoryFrame = (uint)Scene->MaxHistoryFrame,
    };

    GraphicsContext->SetComputePipelineState(SSGIResolvePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(ResolveTexture.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(ResolveTexture.Height / 8.0f), 1u),
    1);
}

void FScreenSpaceGIPass::UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene)
{
    SCOPED_NAMED_EVENT(GraphicsContext, SSGI_UpdateHistory);

    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HistoryTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::SSGIUpdateHistoryRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture.SrvIndex,
        .historyTextureIndex = HistoryTexture.UavIndex,
        .dstTexelSize = {1.0f / HistoryTexture.Width, 1.0f / HistoryTexture.Height},
    };

    GraphicsContext->SetComputePipelineState(SSGIUpdateHistoryPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(HistoryTexture.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(HistoryTexture.Height / 8.0f), 1u),
    1);

    HistoryFrameCount++;
}

void FScreenSpaceGIPass::CompositionSSGI(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, CompositionSSGI);

    GraphicsContext->AddResourceBarrier(ResolveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::CompositionSSGIRenderResource RenderResources = {
        .resolveTextureIndex = ResolveTexture.SrvIndex,
        .gbufferAIndex = SceneTexture.GBufferA.SrvIndex,
        .dstTextureIndex = SceneTexture.HDRTexture.UavIndex,
        .width = SceneTexture.Size.Width,
        .height = SceneTexture.Size.Height,
    };

    GraphicsContext->SetComputePipelineState(CompositionSSGIPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(SceneTexture.Size.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(SceneTexture.Size.Height / 8.0f), 1u),
    1);
}
