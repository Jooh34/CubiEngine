#include "Renderer/ShadowDepthPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FShadowDepthPass::FShadowDepthPass(FGraphicsDevice* const Device)
{
    FTextureCreationDesc Desc{
        .Usage = ETextureUsage::DepthStencil,
        .Width = GShadowDepthDimension,
        .Height = GShadowDepthDimension,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"ShadowDepth Texture",
    };

    ShadowDepthTexture = Device->CreateTexture(Desc);

    FGraphicsPipelineStateCreationDesc PipelineStateDesc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
                .pixelShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
            },
        .RtvFormats = {},
        .RtvCount = 0,
        .PipelineName = L"ShadowDepthPass Pipeline",
    };

    ShadowDepthPassPipelineState = Device->CreatePipelineState(PipelineStateDesc);
}

void FShadowDepthPass::Render(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    uint32_t DIndex = 0u;

    ViewProjectionMatrix = Scene->GetCamera().GetDirectionalShadowViewProjMatrix(
        Scene->Light.LightBufferData.lightPosition[DIndex],
        Scene->Light.LightBufferData.maxDistance[DIndex]
    );

    GraphicsContext->SetGraphicsPipelineState(ShadowDepthPassPipelineState);
    GraphicsContext->SetRenderTargetDepthOnly(ShadowDepthTexture);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(ShadowDepthTexture.Width),
        .Height = static_cast<float>(ShadowDepthTexture.Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    });
    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsContext->ClearDepthStencilView(ShadowDepthTexture);

    interlop::ShadowDepthPassRenderResource RenderResources{
        .lightViewProjectionMatrix = ViewProjectionMatrix,
    };

    Scene->RenderModels(GraphicsContext, RenderResources);
}