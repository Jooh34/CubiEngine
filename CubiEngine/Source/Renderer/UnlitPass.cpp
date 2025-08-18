#include "Renderer/UnlitPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FUnlitPass::FUnlitPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height)
	: FRenderPass(Device, Width, Height)
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
            DXGI_FORMAT_R10G10B10A2_UNORM
        },
        .RtvCount = 1,
        .PipelineName = L"Unlit Pass Pipeline"
    };

    FTextureCreationDesc TextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_R10G10B10A2_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        .Name = L"Unlit Texture",
    };
    UnlitPipelineState = Device->CreatePipelineState(PipelineDesc);
    UnlitTexture = Device->CreateTexture(TextureDesc);
}

void FUnlitPass::InitSizeDependantResource(const FGraphicsDevice* Device, uint32_t InWidth, uint32_t InHeight)
{
}

void FUnlitPass::Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
    const FTexture* DepthBuffer, uint32_t Width, uint32_t Height)
{
    GraphicsContext->SetGraphicsPipelineState(UnlitPipelineState);
    GraphicsContext->SetRenderTarget(UnlitTexture.get(), DepthBuffer);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(Width),
        .Height = static_cast<float>(Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    }, true);

    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    GraphicsContext->ClearRenderTargetView(UnlitTexture.get(), std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
    
    interlop::UnlitPassRenderResources UnlitRenderResources{};

    Scene->RenderModels(GraphicsContext, UnlitRenderResources);
}
