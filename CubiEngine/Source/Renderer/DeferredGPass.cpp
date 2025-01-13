#include "Renderer/DeferredGPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Resource.h"
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
            },
        .RtvCount = 3,
        .PipelineName = L"Deferred GPass Pipeline",
    };
    
    GeometryPassPipelineState = Device->CreatePipelineState(Desc);

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
    // TODO : re-use RTV descriptor handle
    FTextureCreationDesc GBufferADesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer A",
    };
    FTextureCreationDesc GBufferBDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer B",
    };
    FTextureCreationDesc GBufferCDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer C",
    };
    FTextureCreationDesc HDRTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"HDR",
    };

    GBuffer.GBufferA = Device->CreateTexture(GBufferADesc);
    GBuffer.GBufferB = Device->CreateTexture(GBufferBDesc);
    GBuffer.GBufferC = Device->CreateTexture(GBufferCDesc);
    HDRTexture = Device->CreateTexture(HDRTextureDesc);
}

void FDeferredGPass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitSizeDependantResource(Device, InWidth, InHeight);
}

void FDeferredGPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext, FTexture& DepthBuffer, uint32_t Width, uint32_t Height)
{
    GraphicsContext->SetGraphicsPipelineState(GeometryPassPipelineState);
    std::array<FTexture, 3> Textures = {
        GBuffer.GBufferA,
        GBuffer.GBufferB,
        GBuffer.GBufferC,
    };
    GraphicsContext->SetRenderTargets(Textures, DepthBuffer);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(Width),
        .Height = static_cast<float>(Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
        });

    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // No need to clear GBuffer
    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferA, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferB, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferC, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(HDRTexture, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});

    interlop::DeferredGPassRenderResources RenderResources{};

    Scene->RenderModels(GraphicsContext, RenderResources);
}

void FDeferredGPass::RenderLightPass(FScene* const Scene, FGraphicsContext* const GraphicsContext,
    FShadowDepthPass* ShadowDepthPass, FTexture& DepthTexture, uint32_t Width, uint32_t Height)
{
    interlop::PBRRenderResources RenderResources = {
        .lightViewProjectionMatrix = ShadowDepthPass->GetViewProjectionMatrix(),
        .GBufferAIndex = GBuffer.GBufferA.SrvIndex,
        .GBufferBIndex = GBuffer.GBufferB.SrvIndex,
        .GBufferCIndex = GBuffer.GBufferC.SrvIndex,
        .depthTextureIndex = DepthTexture.SrvIndex,
        .prefilteredEnvmapIndex = Scene->GetEnviromentMap()->PrefilteredCubemapTexture.SrvIndex,
        .cubemapTextureIndex = Scene->GetEnviromentMap()->CubeMapTexture.SrvIndex,
        .envBRDFTextureIndex = Scene->GetEnviromentMap()->BRDFLutTexture.SrvIndex,
        .shadowDepthTextureIndex = ShadowDepthPass->GetShadowDepthTexture().SrvIndex,
        .outputTextureIndex = HDRTexture.UavIndex,
        .width = Width,
        .height = Height,
        .sceneBufferIndex = Scene->GetSceneBuffer().CbvIndex,
        .lightBufferIndex = Scene->GetLightBuffer().CbvIndex,
        .iblIntensity = Scene->Light.IBLIntensity,
    };

    // Resource Barrier
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(ShadowDepthPass->GetShadowDepthTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(HDRTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();

    GraphicsContext->SetComputePipelineState(LightPassPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(Width / 8.0f), 1u),
        max((uint32_t)std::ceil(Height / 8.0f), 1u),
    1);
}
