#include "Renderer/BloomPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Profiler.h"
#include "CubiMath.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FBloomPass::FBloomPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height)
	: FRenderPass(Device, Width, Height)
{
    FComputePipelineStateCreationDesc GaussianBlurPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/Common/GaussianBlurW.hlsl",
        },
        .PipelineName = L"GaussianBlurW Pipeline"
    };
    GaussianBlurPipelineState = Device->CreatePipelineState(GaussianBlurPipelineDesc);

    FComputePipelineStateCreationDesc DownSamplePipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule
        {
            .computeShaderPath = L"Shaders/Common/DownSample.hlsl",
        },
        .PipelineName = L"DownSample Pipeline"
    };
    DownSamplePipelineState = Device->CreatePipelineState(DownSamplePipelineDesc);
}

void FBloomPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc BloomResultTextureDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = uint32_t(InWidth / 2.f),
        .Height = uint32_t(InHeight / 2.f),
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"BloomResult Texture",
    };
    BloomResultTexture = Device->CreateTexture(BloomResultTextureDesc);
    
    BloomXTextures.clear();
    BloomXTextures.reserve(BLOOM_MAX_STEP);
    BloomYTextures.clear();
    BloomYTextures.reserve(BLOOM_MAX_STEP);

    for (int BloomIndex = 0; BloomIndex < BLOOM_MAX_STEP; BloomIndex++)
    {
        float Denom = pow(2, BloomIndex + 1);
        uint32_t Width = uint32_t(InWidth / Denom);
        uint32_t Height = uint32_t(InHeight / Denom);

        wchar_t NameBuffer[100];
        swprintf(NameBuffer, 100, L"BloomX Texture 1/%d", (int)Denom);
        FTextureCreationDesc BloomXTextureDesc{
            .Usage = ETextureUsage::UAVTexture,
            .Width = Width,
            .Height = Height,
            .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            .Name = NameBuffer,
        };
        BloomXTextures.push_back(Device->CreateTexture(BloomXTextureDesc));

        swprintf(NameBuffer, 100, L"BloomY Texture 1/%d", (int)Denom);
        FTextureCreationDesc BloomYTextureDesc{
            .Usage = ETextureUsage::UAVTexture,
            .Width = Width,
            .Height = Height,
            .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            .Name = NameBuffer,
        };
        BloomYTextures.push_back(Device->CreateTexture(BloomYTextureDesc));

        swprintf(NameBuffer, 100, L"DownSampled Scene Texture 1/%d", (int)Denom);
        FTextureCreationDesc DownSampledSceneTextureDesc{
            .Usage = ETextureUsage::UAVTexture,
            .Width = Width,
            .Height = Height,
            .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            .Name = NameBuffer,
        };
        DownSampledSceneTextures.push_back(Device->CreateTexture(DownSampledSceneTextureDesc));
    }
}

void FBloomPass::AddBloomPass(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR)
{
    SCOPED_NAMED_EVENT(GraphicsContext, BloomPass);

    DownSampleSceneTexture(GraphicsContext, HDR);

    float GaussianBlurWeight[MAX_GAUSSIAN_KERNEL_SIZE];
    int KernelSize = 16;
    float StdDev = 0.8f;

    int BloomStep = BLOOM_MAX_STEP;
    FTexture* InputTexture;
    FTexture* OutputTexture;
    for (int BloomIndex = BloomStep-1; BloomIndex >= 0; BloomIndex--)
    {
        CreateGaussianBlurWeight(GaussianBlurWeight, KernelSize, StdDev);

        // -------------- BloomX --------------
        InputTexture = &DownSampledSceneTextures[BloomIndex];
        OutputTexture = &BloomXTextures[BloomIndex];

        GraphicsContext->AddResourceBarrier(*InputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(*OutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::GaussianBlurWRenderResource RenderResources = {
            .srcTextureIndex = InputTexture->SrvIndex,
            .dstTextureIndex = OutputTexture->UavIndex,
            .dstTexelSize = {1.0f / OutputTexture->Width, 1.0f / OutputTexture->Height},
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

        GraphicsContext->SetComputePipelineState(GaussianBlurPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);
        
        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(OutputTexture->Width / 8.0f), 1u),
            max((uint32_t)std::ceil(OutputTexture->Height / 8.0f), 1u),
        1);
        
        // -------------- BloomY --------------
        InputTexture = &BloomXTextures[BloomIndex];
        OutputTexture = (BloomIndex ==0) ? &BloomResultTexture : &BloomYTextures[BloomIndex];
        FTexture* AdditiveTexture = (BloomIndex == BloomStep - 1) ? nullptr : &BloomYTextures[BloomIndex + 1];

        GraphicsContext->AddResourceBarrier(*InputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(*OutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        if (AdditiveTexture)
        {
            GraphicsContext->AddResourceBarrier(*AdditiveTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        GraphicsContext->ExecuteResourceBarriers();

        interlop::GaussianBlurWRenderResource RenderResourcesY = {
            .srcTextureIndex = InputTexture->SrvIndex,
            .dstTextureIndex = OutputTexture->UavIndex,
            .dstTexelSize = {1.0f / OutputTexture->Width, 1.0f / OutputTexture->Height},
            .additiveTextureIndex = AdditiveTexture ? AdditiveTexture->SrvIndex : INVALID_INDEX_U32,
            .bHorizontal = 0,
            .kernelSize = (uint)KernelSize,
        };
        for (int i = 0; i < MAX_GAUSSIAN_KERNEL_SIZE/4; i++)
        {
            RenderResourcesY.weights[i] = XMFLOAT4(GaussianBlurWeight[i * 4] * Scene->BloomTint[BloomIndex],
                GaussianBlurWeight[i * 4+1] * Scene->BloomTint[BloomIndex],
                GaussianBlurWeight[i * 4+2] * Scene->BloomTint[BloomIndex],
                GaussianBlurWeight[i * 4+3] * Scene->BloomTint[BloomIndex]
            );
        }

        GraphicsContext->SetComputePipelineState(GaussianBlurPipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResourcesY);

        // shader (8,8,1)
        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(OutputTexture->Width / 8.0f), 1u),
            max((uint32_t)std::ceil(OutputTexture->Height / 8.0f), 1u),
        1);

        // downscale kernel
        KernelSize = KernelSize / 2.f;
    }
}

void FBloomPass::DownSampleSceneTexture(FGraphicsContext* GraphicsContext, FTexture* HDR)
{
    SCOPED_NAMED_EVENT(GraphicsContext, DownSampleSceneTexture);
    
    FTexture* InputTexture = HDR;
    FTexture* OutputTexture = &DownSampledSceneTextures[0];
    for (int i = 0; i < BLOOM_MAX_STEP; i++)
    {
        GraphicsContext->AddResourceBarrier(*InputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        GraphicsContext->AddResourceBarrier(*OutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        GraphicsContext->ExecuteResourceBarriers();

        interlop::DownSampleRenderResource RenderResources = {
            .srcTextureIndex = InputTexture->SrvIndex,
            .dstTextureIndex = OutputTexture->UavIndex,
            .dstTexelSize = {1.0f / OutputTexture->Width, 1.0f / OutputTexture->Height},
        };

        GraphicsContext->SetComputePipelineState(DownSamplePipelineState);
        GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

        // shader (8,8,1)

        GraphicsContext->Dispatch(
            max((uint32_t)std::ceil(OutputTexture->Width / 8.0f), 1u),
            max((uint32_t)std::ceil(OutputTexture->Height / 8.0f), 1u),
        1);

        if (i != BLOOM_MAX_STEP - 1)
        {
            InputTexture = &DownSampledSceneTextures[i];
            OutputTexture = &DownSampledSceneTextures[i+1];
        }
    }
}
