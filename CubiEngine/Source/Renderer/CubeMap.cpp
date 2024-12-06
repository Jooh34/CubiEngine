#include "Renderer/CubeMap.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FCubeMap::FCubeMap(FGraphicsDevice* Device, const FCubeMapCreationDesc& Desc)
{
    uint32_t MipLevels = 6u;

    const FTexture EquirectangularTexture = Device->CreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::HDRTextureFromPath,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .MipLevels = MipLevels,
        .DepthOrArraySize = 1u,
        .BytesPerPixel = 16u,
        .Name = Desc.Name,
        .Path = Desc.EquirectangularTexturePath,
    });

    CubeMapTexture = Device->CreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::CubeMap,
        .Width = GCubeMapTextureDimension,
        .Height = GCubeMapTextureDimension,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels = MipLevels,
        .DepthOrArraySize = 6u,
        .Name = Desc.Name + std::wstring(L"CubeMap"),
    });

    ConvertEquirectToCubeMapPipelineState =
        Device->CreatePipelineState(FComputePipelineStateCreationDesc{
            .CsShaderPath = L"Shaders/CubeMap/ConvertEquirectToCubeMap.hlsl",
            .PipelineName = L"Convert EquirectTexture To CubeMap",
        });
    
    ScreenSpaceCubemapPipelineState =
        Device->CreatePipelineState(FGraphicsPipelineStateCreationDesc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/CubeMap/ScreenSpaceCubemap.hlsl",
                .pixelShaderPath = L"Shaders/CubeMap/ScreenSpaceCubemap.hlsl",
            },
        .RtvFormats =
            {
                DXGI_FORMAT_R16G16B16A16_FLOAT,
            },
        .RtvCount = 1,
        .DepthComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL, // only process depth = 1
        .PipelineName = L"ScreenSpaceCubemap Pipeline",
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
        });


    std::unique_ptr<FComputeContext> ComputeContext = Device->GetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(CubeMapTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(ConvertEquirectToCubeMapPipelineState);
    
    // Process for all Mips
    for (uint32_t i = 0; i < MipLevels; i++)
    {
         const interlop::ConvertEquirectToCubeMapRenderResource RenderResources = {
            .textureIndex = EquirectangularTexture.SrvIndex,
            .outputCubeMapTextureIndex = CubeMapTexture.MipUavIndex[i],
         };

        ComputeContext->Set32BitComputeConstants(&RenderResources);

        const uint32_t numGroups = max(1u, GCubeMapTextureDimension / 8u);

        ComputeContext->Dispatch(numGroups, numGroups, 6u);
    }
   
    ComputeContext->AddResourceBarrier(CubeMapTexture, D3D12_RESOURCE_STATE_COMMON);
    ComputeContext->ExecuteResourceBarriers();

    Device->ExecuteAndFlushComputeContext(std::move(ComputeContext));
}

void FCubeMap::Render(FGraphicsContext* const GraphicsContext,
    interlop::ScreenSpaceCubeMapRenderResources& RenderResource, const FTexture& Target, const FTexture& DepthBuffer)
{
    GraphicsContext->SetGraphicsPipelineState(ScreenSpaceCubemapPipelineState);
    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::array<FTexture, 1> Textures = {
        Target
    };

    GraphicsContext->SetRenderTargets(Textures, DepthBuffer);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(Target.Width),
        .Height = static_cast<float>(Target.Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
     });

    GraphicsContext->SetGraphicsRoot32BitConstants(&RenderResource);

    GraphicsContext->DrawInstanced(6, 1, 0, 0);
}
