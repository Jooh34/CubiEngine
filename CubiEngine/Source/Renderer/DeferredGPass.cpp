#include "Renderer/DeferredGPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Resource.h"
#include "Graphics/Profiler.h"
#include "Scene/Scene.h"
#include "Renderer/ShadowDepthPass.h"
#include "ShaderInterlop/RenderResources.hlsli"

FDeferredGPass::FDeferredGPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height)
{
    FGraphicsPipelineStateCreationDesc Desc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/RenderPass/DeferredGPass.hlsl",
                .pixelShaderPath = L"Shaders/RenderPass/DeferredGPass.hlsl",
            },
        .RtvFormats =
            {
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_R16G16_FLOAT,
            },
        .RtvCount = 4,
        .PipelineName = L"Deferred GPass Pipeline",
    };
    
    GeometryPassPipelineState = Device->CreatePipelineState(Desc);
    
    FGraphicsPipelineStateCreationDesc GeometryPassLightPipelineDesc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/RenderPass/DeferredGPassCube.hlsl",
                .pixelShaderPath = L"Shaders/RenderPass/DeferredGPassCube.hlsl",
            },
        .RtvFormats =
            {
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                DXGI_FORMAT_R16G16_FLOAT,
            },
        .RtvCount = 4,
        .PipelineName = L"Deferred GPass Cube Pipeline",
    };
    GeometryPassLightPipelineState = Device->CreatePipelineState(GeometryPassLightPipelineDesc);

    FComputePipelineStateCreationDesc LightPassPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/RenderPass/DeferredLightingPBR.hlsl",
        .PipelineName = L"LightPass Pipeline"
    };

    LightPassPipelineState = Device->CreatePipelineState(LightPassPipelineDesc);

    InitSizeDependantResource(Device, Width, Height);
}

void FDeferredGPass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
}

void FDeferredGPass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FDeferredGPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext, FSceneTexture& SceneTexture)
{
    {
        SCOPED_NAMED_EVENT(GraphicsContext, DeferredGPassModel);

        GraphicsContext->SetGraphicsPipelineState(GeometryPassPipelineState);
        std::array<FTexture, 4> Textures = {
            SceneTexture.GBufferA,
            SceneTexture.GBufferB,
            SceneTexture.GBufferC,
            SceneTexture.VelocityTexture,
        };
        GraphicsContext->SetRenderTargets(Textures, SceneTexture.DepthTexture);
        GraphicsContext->SetViewport(D3D12_VIEWPORT{
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = static_cast<float>(SceneTexture.Size.Width),
            .Height = static_cast<float>(SceneTexture.Size.Height),
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
            }, true);

        GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        // No need to clear GBuffer
        GraphicsContext->ClearRenderTargetView(SceneTexture.GBufferA, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
        GraphicsContext->ClearRenderTargetView(SceneTexture.GBufferB, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
        GraphicsContext->ClearRenderTargetView(SceneTexture.GBufferC, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
        GraphicsContext->ClearRenderTargetView(SceneTexture.HDRTexture, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});

        interlop::DeferredGPassRenderResources RenderResources{};

        Scene->RenderModels(GraphicsContext, RenderResources);
    }

    {
        SCOPED_NAMED_EVENT(GraphicsContext, DeferredGPassLight);

        GraphicsContext->SetGraphicsPipelineState(GeometryPassLightPipelineState);

        // Draw Light
        interlop::DeferredGPassCubeRenderResources RenderResources{};

        Scene->RenderLightsDeferred(GraphicsContext, RenderResources);
    }
}

void FDeferredGPass::RenderLightPass(FScene* const Scene, FGraphicsContext* const GraphicsContext,
    FShadowDepthPass* ShadowDepthPass, FSceneTexture& SceneTexture, FTexture* SSAOTexture)
{
    uint32_t Width = SceneTexture.Size.Width;
    uint32_t Height = SceneTexture.Size.Height;

    interlop::PBRRenderResources RenderResources = {
        .numCascadeShadowMap = GNumCascadeShadowMap,
        .GBufferAIndex = SceneTexture.GBufferA.SrvIndex,
        .GBufferBIndex = SceneTexture.GBufferB.SrvIndex,
        .GBufferCIndex = SceneTexture.GBufferC.SrvIndex,
        .depthTextureIndex = SceneTexture.DepthTexture.SrvIndex,
        .prefilteredEnvmapIndex = Scene->GetEnvironmentMap()->PrefilteredCubemapTexture.SrvIndex,
        .cubemapTextureIndex = Scene->GetEnvironmentMap()->CubeMapTexture.SrvIndex,
        .envBRDFTextureIndex = Scene->GetEnvironmentMap()->BRDFLutTexture.SrvIndex,
        .envIrradianceTextureIndex = Scene->GetEnvironmentMap()->IrradianceCubemapTexture.SrvIndex,
        .envMipCount = Scene->GetEnvironmentMap()->GetMipCount(),
        .shadowDepthTextureIndex = ShadowDepthPass->GetShadowDepthTexture().SrvIndex,
        .ssaoTextureIndex = SSAOTexture ? SSAOTexture->SrvIndex : INVALID_INDEX_U32,
        .outputTextureIndex = SceneTexture.HDRTexture.UavIndex,
        .dstTexelSize = {1.0f / Width, 1.0f / Height},
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex,
        .shadowBufferIndex = Scene->GetShadowBuffer().CbvIndex,
        .debugBufferIndex = Scene->GetDebugBuffer().CbvIndex,
        .bUseEnvmap = Scene->bUseEnvmap ? 1u : 0u,
        .bUseEnergyCompensation = Scene->bUseEnergyCompensation ? 1u : 0u,
        .WhiteFurnaceMethod = uint(Scene->WhiteFurnaceMethod),
        .bCSMDebug = Scene->bCSMDebug ? 1u : 0u,
        .diffuseMethod = uint(Scene->DiffuseMethod),
        .sampleBias = GFrameCount,
    };

    // Resource Barrier
    GraphicsContext->AddResourceBarrier(SceneTexture.GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.VelocityTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ShadowDepthPass->GetShadowDepthTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (SSAOTexture)
    {
        GraphicsContext->AddResourceBarrier(*SSAOTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    GraphicsContext->ExecuteResourceBarriers();

    GraphicsContext->SetComputePipelineState(LightPassPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}
