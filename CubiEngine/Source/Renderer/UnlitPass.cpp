#include "Renderer/UnlitPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FUnlitPass::FUnlitPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height)
{
    FGraphicsPipelineStateCreationDesc PipelineDesc = FGraphicsPipelineStateCreationDesc
    {
        .ShaderModule =
        {
            .vertexShaderPath = L"Shaders/RenderPass/UnlitPass.hlsl",
            .pixelShaderPath = L"Shaders/RenderPass/UnlitPass.hlsl",
        },
        .RtvFormats =
        {
            DXGI_FORMAT_R16G16B16A16_FLOAT
        },
        .RtvCount = 1,
        .PipelineName = L"Unlit Pass Pipeline"
    };

    FTextureCreationDesc TextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        .Name = "Unlit Texture",
    };

    UnlitPipelineState = Device->CreatePipelineState(PipelineDesc);
    UnlitTexture = Device->CreateTexture(TextureDesc);
}

void FUnlitPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
    const FTexture& DepthBuffer, uint32_t Width, uint32_t Height)
{
    GraphicsContext->SetGraphicsPipelineState(UnlitPipelineState);
    GraphicsContext->SetRenderTarget(UnlitTexture, DepthBuffer);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(Width),
        .Height = static_cast<float>(Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
        });

    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    GraphicsContext->ClearRenderTargetView(UnlitTexture, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    
    interlop::UnlitPassRenderResources UnlitRenderResources{};

    Scene->RenderModels(GraphicsContext, UnlitRenderResources);
}
