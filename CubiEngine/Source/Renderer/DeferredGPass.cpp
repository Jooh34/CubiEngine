#include "Renderer/DeferredGPass.h"
#include "Graphics/GraphicsDevice.h"

DeferredGPass::DeferredGPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height)
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
