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
    GraphicsContext->SetScissorRects(D3D12_RECT{
        .left = 0u,
        .top = 0u,
        .right = (LONG)ShadowDepthTexture.Width,
        .bottom = (LONG)ShadowDepthTexture.Height,
    });

    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsContext->ClearDepthStencilView(ShadowDepthTexture);

    if (GNumCascadeShadowMap == 1)
    {
        interlop::ShadowDepthPassRenderResource RenderResources{
            .lightViewProjectionMatrix = Scene->Light.LightBufferData.lightViewProjectionMatrix[0],
        };

        Scene->RenderModels(GraphicsContext, RenderResources);
    }
    else
    {
        for (int CascadeIndex = 0; CascadeIndex < GNumCascadeShadowMap; CascadeIndex++)
        {
            float ViewportWidth = ShadowDepthTexture.Width / 2.f;
            float ViewportHeight = ShadowDepthTexture.Height / 2.f;
            float TopLeftX = 0.f;
            float TopLeftY = 0.f;
            if (CascadeIndex == 0)
            {
                TopLeftX = 0.f; TopLeftY = 0.f;
            }
            else if (CascadeIndex == 1)
            {
                TopLeftX = ViewportWidth; TopLeftY = 0.f;
            }
            else if (CascadeIndex == 2)
            {
                TopLeftX = 0.f; TopLeftY = ViewportHeight;
            }
            else if (CascadeIndex == 3)
            {
                TopLeftX = ViewportWidth; TopLeftY = ViewportHeight;
            }

            GraphicsContext->SetViewport(D3D12_VIEWPORT{
                .TopLeftX = TopLeftX,
                .TopLeftY = TopLeftY,
                .Width = ViewportWidth,
                .Height = ViewportHeight,
                .MinDepth = 0.0f,
                .MaxDepth = 1.0f,
            });
            
            D3D12_RECT scissorRect;
            scissorRect.left = static_cast<LONG>(TopLeftX);
            scissorRect.top = static_cast<LONG>(TopLeftY);
            scissorRect.right = static_cast<LONG>(TopLeftX + ViewportWidth);
            scissorRect.bottom = static_cast<LONG>(TopLeftY + ViewportHeight);
            GraphicsContext->SetScissorRects(scissorRect);

            interlop::ShadowDepthPassRenderResource RenderResources{
                .lightViewProjectionMatrix = Scene->Light.LightBufferData.lightViewProjectionMatrix[CascadeIndex],
            };

            Scene->RenderModels(GraphicsContext, RenderResources);
        }
    }
}