#include "Renderer/ShadowDepthPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FShadowDepthPass::FShadowDepthPass(FGraphicsDevice* const Device)
{
    FTextureCreationDesc ShadowDepthTextureDesc{
        .Usage = ETextureUsage::DepthStencil,
        .Width = GShadowDepthDimension,
        .Height = GShadowDepthDimension,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"ShadowDepth Texture",
    };

    ShadowDepthTexture = Device->CreateTexture(ShadowDepthTextureDesc);
    
    FTextureCreationDesc MomentTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = GShadowDepthDimension,
        .Height = GShadowDepthDimension,
        .Format = DXGI_FORMAT_R32G32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"VSM Moment Texture",
    };

    MomentTexture = Device->CreateTexture(MomentTextureDesc);

    FGraphicsPipelineStateCreationDesc PipelineStateDesc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
                .pixelShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
            },
        .RtvFormats = {DXGI_FORMAT_R32G32_FLOAT},
        .RtvCount = 1,
        .PipelineName = L"ShadowDepthPass Pipeline",
    };

    ShadowDepthPassPipelineState = Device->CreatePipelineState(PipelineStateDesc);

    FComputePipelineStateCreationDesc MomentPassPipelineStateDesc{
        .ShaderModule =
        {
            .computeShaderPath = L"Shaders/Shadow/VSMMomentPass.hlsl",
        },
        .PipelineName = L"VSM Moment Pipeline"
    };

    MomentPassPipelineState = Device->CreatePipelineState(MomentPassPipelineStateDesc);
}

void FShadowDepthPass::Render(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    // todo : shader permutation to use vsm or not.
    GraphicsContext->AddResourceBarrier(ShadowDepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsContext->AddResourceBarrier(MomentTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->ExecuteResourceBarriers();
    GraphicsContext->ClearDepthStencilView(ShadowDepthTexture);
    GraphicsContext->ClearRenderTargetView(MomentTexture, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    

    GraphicsContext->SetGraphicsPipelineState(ShadowDepthPassPipelineState);
    //GraphicsContext->SetRenderTargetDepthOnly(ShadowDepthTexture);
    GraphicsContext->SetRenderTarget(MomentTexture, ShadowDepthTexture);

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

    if (GNumCascadeShadowMap == 1)
    {
        interlop::ShadowDepthPassRenderResource RenderResources{
            .lightViewProjectionMatrix = Scene->Light.ShadowBufferData.lightViewProjectionMatrix[0],
        };

        Scene->RenderModels(GraphicsContext, RenderResources);
    }
    else
    {
        for (int CascadeIndex = 0; CascadeIndex < GNumCascadeShadowMap; CascadeIndex++)
        {
            float ViewportWidth = ShadowDepthTexture.Width / 2.f;
            float ViewportHeight = ShadowDepthTexture.Height / 2.f;

            int gridX = CascadeIndex % 2;  // 0, 1 (가로 방향)
            int gridY = CascadeIndex / 2;  // 0, 1 (세로 방향)

            float TopLeftX = gridX * ViewportWidth;
            float TopLeftY = gridY * ViewportHeight;

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
            //scissorRect.left = 0; 
            //scissorRect.top = 0;
            //scissorRect.right = static_cast<LONG>(ShadowDepthTexture.Width);
            //scissorRect.bottom = static_cast<LONG>(ShadowDepthTexture.Height);
            GraphicsContext->SetScissorRects(scissorRect);

            interlop::ShadowDepthPassRenderResource RenderResources{
                .lightViewProjectionMatrix = Scene->Light.ShadowBufferData.lightViewProjectionMatrix[CascadeIndex],
            };

            Scene->RenderModels(GraphicsContext, RenderResources);
        }
    }

    if (Scene->bUseVSM)
    {
        //AddVSMPass(GraphicsContext, Scene);
    }
}

void FShadowDepthPass::AddVSMPass(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    // currently not using

    SCOPED_NAMED_EVENT(GraphicsContext, VSMPass);

    GraphicsContext->AddResourceBarrier(ShadowDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    GraphicsContext->AddResourceBarrier(MomentTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    GraphicsContext->ExecuteResourceBarriers();


    interlop::VSMMomentPassRenderResource RenderResources = {
        .srcTextureIndex = ShadowDepthTexture.SrvIndex,
        .momentTextureIndex = MomentTexture.UavIndex,
        .dstTexelSize = {1.0f / MomentTexture.Width, 1.0f / MomentTexture.Height},
    };

    GraphicsContext->SetComputePipelineState(MomentPassPipelineState);
    GraphicsContext->SetComputeRoot32BitConstants(&RenderResources);

    // shader (8,8,1)
    GraphicsContext->Dispatch(
        max((uint32_t)std::ceil(MomentTexture.Width / 8.0f), 1u),
        max((uint32_t)std::ceil(MomentTexture.Height / 8.0f), 1u),
    1);
}
