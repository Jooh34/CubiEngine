#include "Renderer/EyeAdaptationPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FEyeAdaptationPass::FEyeAdaptationPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height)
	: FRenderPass(Device, Width, Height)
{
    std::vector<UINT> Data(256, 0);

    HistogramBuffer = Device->CreateBuffer<UINT>(
        FBufferCreationDesc{
            .Usage = EBufferUsage::StructuredBufferUAV,
            .Name = L"Histogram buffer ",
        },
        Data);

    // 4 element to align 16 byte
    std::vector<XMFLOAT4> AverageLuminanceData(1, XMFLOAT4{ 0,0,0,0 });
    AverageLuminanceBuffer = Device->CreateBuffer<XMFLOAT4>(
        FBufferCreationDesc{
            .Usage = EBufferUsage::StructuredBufferUAV,
            .Name = L"AverageLuminance buffer ",
        }, AverageLuminanceData);
    
    FComputePipelineStateCreationDesc GenerateHistogramPipelineStateDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/EyeAdaptation/GenerateHistogram.hlsl",
        },
        .PipelineName = L"GenerateHistogram Pipeline"
    };
    
    GenerateHistogramPipelineState = Device->CreatePipelineState(GenerateHistogramPipelineStateDesc);

    FComputePipelineStateCreationDesc CalculateAverageLuminancePipelineStateDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/EyeAdaptation/CalculateAverageLuminance.hlsl",
        },
        .PipelineName = L"CalculateAverageLuminance Pipeline"
    };

    CalcuateAverageLuminancePipelineState = Device->CreatePipelineState(CalculateAverageLuminancePipelineStateDesc);

    EyeAdaptationTonemappingPipelineState = Device->CreatePipelineState(FComputePipelineStateCreationDesc{
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/EyeAdaptation/Tonemapping.hlsl",
        },
        .PipelineName = L"EyeAdaptation Tonemapping Pipeline",
    });
}

void FEyeAdaptationPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
}

void FEyeAdaptationPass::GenerateHistogram(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR)
{
    SCOPED_NAMED_EVENT(GraphicsContext, GenerateHistogram);

    GraphicsContext->AddResourceBarrier(*HDR, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->ExecuteResourceBarriers();
    
    float inverseLogLuminanceRange = 1.f / (Scene->HistogramLogMax - Scene->HistogramLogMin);

    interlop::GenerateHistogramRenderResource RenderResources = {
        .sceneTextureIndex = HDR->SrvIndex,
        .histogramBufferIndex = HistogramBuffer.UavIndex,
        .minLogLuminance = Scene->HistogramLogMin,
        .inverseLogLuminanceRange = inverseLogLuminanceRange,
    };

    GraphicsContext->SetComputePipelineState(GenerateHistogramPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (16,16,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(HDR->Width / 16.0f), 1u),
        max((uint32_t)std::ceil(HDR->Height / 16.0f), 1u),
    1);

    GraphicsContext->AddUAVBarrier(HistogramBuffer);
    GraphicsContext->ExecuteResourceBarriers();
}

void FEyeAdaptationPass::CalculateAverageLuminance(FGraphicsContext* GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height)
{
    SCOPED_NAMED_EVENT(GraphicsContext, CalculateAverageLuminance);
    
    float logLuminanceRange = Scene->HistogramLogMax - Scene->HistogramLogMin;

    interlop::CalculateAverageLuminanceRenderResource RenderResources = {
        .histogramBufferIndex = HistogramBuffer.UavIndex,
        .averageLuminanceBufferIndex = AverageLuminanceBuffer.UavIndex,
        .minLogLuminance = Scene->HistogramLogMin,
        .logLumRange = logLuminanceRange,
        .timeCoeff = Scene->TimeCoeff,
        .numPixels = Width * Height,
    };

    GraphicsContext->SetComputePipelineState(CalcuateAverageLuminancePipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // 256 histogram
    GraphicsContext->Dispatch(1, 1, 1);
}

void FEyeAdaptationPass::ToneMapping(FGraphicsContext* GraphicsContext, FScene* Scene,
    FTexture* HDRTexture, FTexture* LDRTexture, FTexture* BloomTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, EyeAdaptationTonemapping);

    GraphicsContext->AddResourceBarrier(*LDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->AddResourceBarrier(*HDRTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (BloomTexture)
    {
        GraphicsContext->AddResourceBarrier(*BloomTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    GraphicsContext->ExecuteResourceBarriers();

    interlop::TonemappingRenderResources RenderResources = {
        .srcTextureIndex = HDRTexture->SrvIndex,
        .dstTextureIndex = LDRTexture->UavIndex,
        .bloomTextureIndex = BloomTexture ? BloomTexture->SrvIndex : INVALID_INDEX_U32,
        .width = LDRTexture->Width,
        .height = LDRTexture->Height,
        .toneMappingMethod = (uint)Scene->ToneMappingMethod,
        .bGammaCorrection = Scene->bGammaCorrection,
        .averageLuminanceBufferIndex = Scene->bUseEyeAdaptation ? AverageLuminanceBuffer.SrvIndex : INVALID_INDEX_U32,
    };

    GraphicsContext->SetComputePipelineState(EyeAdaptationTonemappingPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(LDRTexture->Width / 8.0f), 1u),
        max((uint32_t)std::ceil(LDRTexture->Height / 8.0f), 1u),
        1);
}
