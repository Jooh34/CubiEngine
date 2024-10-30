#include "Renderer/DeferredGPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Resource.h"
#include "Scene/Scene.h"
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

    FTextureCreationDesc GBufferADesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer A",
    };
    FTextureCreationDesc GBufferBDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer B",
    };
    FTextureCreationDesc GBufferCDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer C",
    };
    FTextureCreationDesc HDRTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"HDR",
    };

    GeometryPassPipelineState = Device->CreatePipelineState(Desc);
    GBuffer.GBufferA = Device->CreateTexture(GBufferADesc);
    GBuffer.GBufferB = Device->CreateTexture(GBufferBDesc);
    GBuffer.GBufferC = Device->CreateTexture(GBufferCDesc);
    HDRTexture = Device->CreateTexture(HDRTextureDesc);
    
    FComputePipelineStateCreationDesc LightPassPipelineDesc = FComputePipelineStateCreationDesc
    {
        .CsShaderPath = L"Shaders/RenderPass/DeferredLightingPBR.hlsl",
        .PipelineName = L"LightPass Pipeline"
    };

    LightPassPipelineState = Device->CreatePipelineState(LightPassPipelineDesc);
}

void FDeferredGPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext, FTexture& DepthBuffer, uint32_t Width, uint32_t Height)
{
    // Resource Barrier
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferA, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferB, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferC, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->AddResourceBarrier(DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsContext->ExecuteResourceBarriers();

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

    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferA, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferB, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.GBufferC, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearDepthStencilView(DepthBuffer);

    interlop::DeferredGPassRenderResources RenderResources{};

    Scene->RenderModels(GraphicsContext, RenderResources);
}

void FDeferredGPass::RenderLightPass(FScene* const Scene, FGraphicsContext* const GraphicsContext, uint32_t Width, uint32_t Height)
{
    interlop::PBRRenderResources RenderResources = {
        .GBufferAIndex = GBuffer.GBufferA.SrvIndex,
        .GBufferBIndex = GBuffer.GBufferB.SrvIndex,
        .GBufferCIndex = GBuffer.GBufferC.SrvIndex,
        .outputTextureIndex = HDRTexture.UavIndex,
    };

    // Resource Barrier
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(GBuffer.GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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
