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

    FTextureCreationDesc AlbedoDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer Albedo",
    };
    FTextureCreationDesc NormalDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer Albedo",
    };
    FTextureCreationDesc AoMetalicRoughnessDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer Albedo",
    };

    PipelineState = Device->CreatePipelineState(Desc);
    GBuffer.Albedo = Device->CreateTexture(AlbedoDesc);
    GBuffer.NormalEmissive = Device->CreateTexture(NormalDesc);
    GBuffer.AoMetalicRoughness = Device->CreateTexture(AoMetalicRoughnessDesc);
}

void FDeferredGPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext, const FTexture& DepthBuffer, uint32_t Width, uint32_t Height)
{
    GraphicsContext->SetGraphicsPipelineState(PipelineState);
    std::array<FTexture, 3> Textures = {
        GBuffer.Albedo,
        GBuffer.NormalEmissive,
        GBuffer.AoMetalicRoughness,
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

    GraphicsContext->ClearRenderTargetView(GBuffer.Albedo, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.NormalEmissive, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    GraphicsContext->ClearRenderTargetView(GBuffer.AoMetalicRoughness, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});

    interlop::DeferredGPassRenderResources RenderResources{};

    Scene->RenderModels(GraphicsContext, RenderResources);
}
