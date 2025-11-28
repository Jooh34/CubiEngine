#include "Renderer/CubeMap.h"
#include "Graphics/Resource.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "ShaderInterlop/RenderResources.hlsli"

FCubeMap::FCubeMap(const FCubeMapCreationDesc& Desc)
{
    std::unique_ptr<FTexture> EquirectangularTexture = RHICreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::HDRTextureFromPath,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .MipLevels = MipCount,
        .DepthOrArraySize = 1u,
        .Name = Desc.Name,
        .Path = Desc.EquirectangularTexturePath,
    });

    CubeMapTexture = RHICreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::CubeMap,
        .Width = GCubeMapTextureDimension,
        .Height = GCubeMapTextureDimension,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels = MipCount,
        .DepthOrArraySize = 6u,
        .Name = Desc.Name + std::wstring(L" CubeMap"),
    });

    ConvertEquirectToCubeMapPipelineState =
        RHICreatePipelineState(FComputePipelineStateCreationDesc{
            .ShaderModule =
                {
                    .computeShaderPath = L"Shaders/CubeMap/ConvertEquirectToCubeMap.hlsl",
                },
            .PipelineName = L"Convert EquirectTexture To CubeMap",
        });
    
    ScreenSpaceCubemapPipelineState =
        RHICreatePipelineState(FGraphicsPipelineStateCreationDesc{
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

    std::unique_ptr<FComputeContext> ComputeContext = RHIGetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(CubeMapTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->AddResourceBarrier(EquirectangularTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(ConvertEquirectToCubeMapPipelineState);
    
    // Process for all Mips
    for (uint32_t i = 0; i < MipCount; i++)
    {
         const interlop::ConvertEquirectToCubeMapRenderResource RenderResources = {
            .textureIndex = EquirectangularTexture->SrvIndex,
            .outputCubeMapTextureIndex = CubeMapTexture->MipUavIndex[i],
         };

        ComputeContext->Set32BitComputeConstants(&RenderResources);
        
        uint32_t Dim = GCubeMapTextureDimension >> i;
        const uint32_t numGroups = max(1u, Dim / 8u);

        ComputeContext->Dispatch(numGroups, numGroups, 6u);
    }
   
    RHIExecuteAndFlushComputeContext(std::move(ComputeContext));

    GeneratePrefilteredCubemap(Desc, MipCount);
    GenerateBRDFLut(Desc);
    GenerateIrradianceMap(Desc);
}

void FCubeMap::GeneratePrefilteredCubemap(const FCubeMapCreationDesc& Desc, uint32_t MipCount)
{
    PrefilterPipelineState =
        RHICreatePipelineState(FComputePipelineStateCreationDesc{
            .ShaderModule =
                {
                    .computeShaderPath = L"Shaders/CubeMap/GeneratePrefilteredCubemapCS.hlsl",
                },
            .PipelineName = L"GeneratePrefilteredCubemapCS Pipeline",
        });

    PrefilteredCubemapTexture = RHICreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::CubeMap,
        .Width = GPreFilteredCubeMapTextureDimension,
        .Height = GPreFilteredCubeMapTextureDimension,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels = MipCount,
        .DepthOrArraySize = 6u,
        .Name = Desc.Name + std::wstring(L" Prefiltered Cubemap"),
     });

    std::unique_ptr<FComputeContext> ComputeContext = RHIGetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(CubeMapTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ComputeContext->AddResourceBarrier(PrefilteredCubemapTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(PrefilterPipelineState);

    // Process for all Mips
    for (uint32_t i = 0; i < MipCount; i++)
    {
        uint32_t Dim = GCubeMapTextureDimension >> i;

        const interlop::GeneratePrefilteredCubemapResource RenderResources = {
           .srcMipSrvIndex = CubeMapTexture->SrvIndex,
           .dstMipUavIndex = PrefilteredCubemapTexture->MipUavIndex[i],
           .mipLevel = i,
           .totalMipLevel = MipCount,
           .texelSize = {1.0f / Dim, 1.0f / Dim},
        };

        ComputeContext->Set32BitComputeConstants(&RenderResources);

        const uint32_t numGroups = max(1u, Dim / 8u);

        ComputeContext->Dispatch(numGroups, numGroups, 6u);
    }

    ComputeContext->AddResourceBarrier(PrefilteredCubemapTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ComputeContext->ExecuteResourceBarriers();

    RHIExecuteAndFlushComputeContext(std::move(ComputeContext));
}

void FCubeMap::GenerateBRDFLut(const FCubeMapCreationDesc& Desc)
{
    BRDFLutPipelineState =
        RHICreatePipelineState(FComputePipelineStateCreationDesc{
            .ShaderModule =
                {
                    .computeShaderPath = L"Shaders/CubeMap/GenerateBRDFLutCS.hlsl",
                },
            .PipelineName = L"GenerateBRDFLutCS Pipeline",
        });

    BRDFLutTexture = RHICreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = GBRDFLutTextureDimension,
        .Height = GBRDFLutTextureDimension,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels = 1u,
        .DepthOrArraySize = 1u,
        .Name = Desc.Name + std::wstring(L" BRDFLut Texture"),
    });
    
    std::unique_ptr<FComputeContext> ComputeContext = RHIGetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(BRDFLutTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(BRDFLutPipelineState);

    const interlop::GenerateBRDFLutRenderResource RenderResources = {
        .textureUavIndex = BRDFLutTexture->UavIndex
    };

    ComputeContext->Set32BitComputeConstants(&RenderResources);

    const uint32_t numGroups = max(1u, GBRDFLutTextureDimension / 8u);
    ComputeContext->Dispatch(numGroups, numGroups, 1u);

    ComputeContext->AddResourceBarrier(BRDFLutTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ComputeContext->ExecuteResourceBarriers();

    RHIExecuteAndFlushComputeContext(std::move(ComputeContext));
}

void FCubeMap::GenerateIrradianceMap(const FCubeMapCreationDesc& Desc)
{
    GenerateIrradianceMapPipelineState =
        RHICreatePipelineState(FComputePipelineStateCreationDesc{
            .ShaderModule =
                {
                    .computeShaderPath = L"Shaders/CubeMap/GenerateIrradianceMap.hlsl",
                },
            .PipelineName = L"GenerateIrradianceMapCS Pipeline",
        });

    IrradianceCubemapTexture = RHICreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::CubeMap,
        .Width = GIrradianceCubeMapTextureDimension,
        .Height = GIrradianceCubeMapTextureDimension,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .MipLevels = 1u,
        .DepthOrArraySize = 6u,
        .Name = Desc.Name + std::wstring(L"Irradiance Cubemap"),
    });

    std::unique_ptr<FComputeContext> ComputeContext = RHIGetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(IrradianceCubemapTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(GenerateIrradianceMapPipelineState);

    const interlop::GeneratePrefilteredCubemapResource RenderResources = {
       .srcMipSrvIndex = CubeMapTexture->SrvIndex,
       .dstMipUavIndex = IrradianceCubemapTexture->UavIndex,
       .texelSize = {1.0f / IrradianceCubemapTexture->Width, 1.0f / IrradianceCubemapTexture->Height},
    };

    ComputeContext->Set32BitComputeConstants(&RenderResources);

    const uint32_t numGroups = max(1u, GIrradianceCubeMapTextureDimension / 8u);
    ComputeContext->Dispatch(numGroups, numGroups, 6u);

    ComputeContext->AddResourceBarrier(IrradianceCubemapTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    ComputeContext->ExecuteResourceBarriers();

    RHIExecuteAndFlushComputeContext(std::move(ComputeContext));
}

void FCubeMap::Render(FGraphicsContext* const GraphicsContext,
    interlop::ScreenSpaceCubeMapRenderResources& RenderResource, FTexture* Target, const FTexture* DepthBuffer)
{
    GraphicsContext->AddResourceBarrier(Target, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->ExecuteResourceBarriers();

    GraphicsContext->SetGraphicsPipelineState(ScreenSpaceCubemapPipelineState);
    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::array<const FTexture*, 1> Textures = {
        Target
    };

    GraphicsContext->SetRenderTargets(Textures, DepthBuffer);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(Target->Width),
        .Height = static_cast<float>(Target->Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
     }, true);

    GraphicsContext->SetGraphicsRoot32BitConstants(&RenderResource);

    GraphicsContext->DrawInstanced(6, 1, 0, 0);
}
