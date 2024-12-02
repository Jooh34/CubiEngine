#include "Renderer/CubeMap.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FCubeMap::FCubeMap(FGraphicsDevice* Device, const FCubeMapCreationDesc& Desc)
{
    const FTexture EquirectangularTexture = Device->CreateTexture(FTextureCreationDesc{
        .Usage = ETextureUsage::HDRTextureFromPath,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .MipLevels = 1,
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
        .MipLevels = 1u,
        .DepthOrArraySize = 6u,
        .Name = Desc.Name + std::wstring(L"CubeMap"),
    });

    ConvertEquirectToCubeMapPipelineState =
        Device->CreatePipelineState(FComputePipelineStateCreationDesc{
            .CsShaderPath = L"Shaders/CubeMap/ConvertEquirectToCubeMap.hlsl",
            .PipelineName = L"Convert EquirectTexture To CubeMap",
        });

    std::unique_ptr<FComputeContext> ComputeContext = Device->GetComputeContext();
    ComputeContext->Reset();

    ComputeContext->AddResourceBarrier(CubeMapTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ComputeContext->ExecuteResourceBarriers();
    ComputeContext->SetComputeRootSignatureAndPipeline(ConvertEquirectToCubeMapPipelineState);

    const interlop::ConvertEquirectToCubeMapRenderResource RenderResources = {
        .textureIndex = EquirectangularTexture.SrvIndex,
        .outputCubeMapTextureIndex = CubeMapTexture.UavIndex,
    };

    ComputeContext->Set32BitComputeConstants(&RenderResources);

    const uint32_t numGroups = max(1u, GCubeMapTextureDimension / 8u);

    ComputeContext->Dispatch(numGroups, numGroups, 6u);

    ComputeContext->AddResourceBarrier(CubeMapTexture, D3D12_RESOURCE_STATE_COMMON);
    ComputeContext->ExecuteResourceBarriers();

    Device->ExecuteAndFlushComputeContext(std::move(ComputeContext));
}
