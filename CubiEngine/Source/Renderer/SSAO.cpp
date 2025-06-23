#include "Renderer/SSAO.h"
#include "Graphics/Profiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "CubiMath.h"
#include "ShaderInterlop/RenderResources.hlsli"

FSSAOPass::FSSAOPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height)
	: FRenderPass(GraphicsDevice, Width, Height)
{
    SSAOKernelBuffer = GraphicsDevice->CreateBuffer<interlop::SSAOKernelBuffer>(FBufferCreationDesc{
        .Usage = EBufferUsage::ConstantBuffer,
        .Name = L"SSAO Kernel Buffer",
    });
    GenerateSSAOKernel();

    FComputePipelineStateCreationDesc SSAOPipelineDesc = FComputePipelineStateCreationDesc
    {
        .ShaderModule =
        {
            .computeShaderPath = L"Shaders/SSAO/SSAO.hlsl",
        },
        .PipelineName = L"SSAO Pipeline"
    };
    SSAOPipelineState = GraphicsDevice->CreatePipelineState(SSAOPipelineDesc);
}

void FSSAOPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    SSAOTexture = Device->CreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"SSAO Texture",
    });
}

void FSSAOPass::GenerateSSAOKernel()
{
    int KernelSize = 64;
    float RndFloats[MAX_SSAO_KERNEL_SIZE * 3];
    CreateRandomFloats(RndFloats, KernelSize * 3);
    
    interlop::SSAOKernelBuffer SSAOKernelBufferData{};

    for (int i = 0; i < KernelSize; i++)
    {
        XMVECTOR Sample = XMVectorSet(
            RndFloats[i * 3] * 2.0f - 1.0f,
            RndFloats[i * 3 + 1] * 2.0f - 1.0f,
            RndFloats[i * 3 + 2],
            1.0f
        );

        Sample = XMVector3Normalize(Sample);

        // https://learnopengl.com/Advanced-Lighting/SSAO
        float scale = (float)i / 64.0;
        scale = std::lerp(0.1f, 1.0f, scale * scale);
        XMVECTOR Scaled = XMVectorScale(Sample, scale);

        // save
        XMStoreFloat4(&SSAOKernelBufferData.kernel[i], Scaled);
    }

    SSAOKernelBuffer.Update(&SSAOKernelBufferData);
}

void FSSAOPass::AddSSAOPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture)
{
    SCOPED_NAMED_EVENT(GraphicsContext, SSAO);

    GraphicsContext->AddResourceBarrier(SceneTexture.GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SSAOTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    interlop::SSAORenderResource RenderResources = {
        .GBufferBIndex = SceneTexture.GBufferB.SrvIndex,
        .depthTextureIndex = SceneTexture.DepthTexture.SrvIndex,
        .dstTextureIndex = SSAOTexture.UavIndex,
        .SSAOKernelBufferIndex = SSAOKernelBuffer.CbvIndex,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .frameCount = GFrameCount,
        .kernelSize = (uint)Scene->SSAOKernelSize,
        .kernelRadius = Scene->SSAOKernelRadius,
        .depthBias = Scene->SSAODepthBias,
        .bUseRangeCheck = Scene->SSAOUseRangeCheck ? 1u : 0u,
    };

    GraphicsContext->SetComputePipelineState(SSAOPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(SSAOTexture.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(SSAOTexture.Height / 8.0f), 1u),
    1);
}
