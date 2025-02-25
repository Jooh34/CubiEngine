#include "Graphics/MipmapGenerator.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/RenderResources.hlsli"

FMipmapGenerator::FMipmapGenerator(FGraphicsDevice* const Device) : Device(Device)
{
    GenerateMipmapPipelineState = Device->CreatePipelineState(FComputePipelineStateCreationDesc{
        .CsShaderPath = L"Shaders/MipMap/GenerateMipmapCS.hlsl",
        .PipelineName = L"Mipmap Generation Pipeline",
    });

    GenerateCubemapMipmapPipelineState = Device->CreatePipelineState(FComputePipelineStateCreationDesc{
        .CsShaderPath = L"Shaders/MipMap/GenerateCubemapMipmapCS.hlsl",
        .PipelineName = L"Mipmap Generation Pipeline",
    });
}

void FMipmapGenerator::GenerateMipmap(FTexture& Texture)
{
    // Todo : lock to support multithread
    D3D12_RESOURCE_DESC ResourceDesc = Texture.Allocation.Resource->GetDesc();
    if (ResourceDesc.MipLevels <= 1)
        return;

    //assert(Texture.IsPowerOfTwo());
    
    std::unique_ptr<FComputeContext> Context = Device->GetComputeContext();
    Context->Reset();

    Context->AddResourceBarrier(Texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context->ExecuteResourceBarriers();

    // Generate each mip level.
    for (uint32_t srcMipLevel = 0; srcMipLevel < ResourceDesc.MipLevels - 1u; srcMipLevel++)
    {
        uint32_t dstMipLevel = srcMipLevel + 1;
        uint32_t dstWidth = Texture.Width >> dstMipLevel;
        uint32_t dstHeight = Texture.Height >> dstMipLevel;
        bool IsSRGB = FTexture::IsSRGB(ResourceDesc.Format);

        const interlop::GenerateMipmapResource Resource = {
            .srcMipSrvIndex = Texture.SrvIndex,
            .srcMipLevel = srcMipLevel,
            .dstTexelSize = {1.0f / dstWidth, 1.0f / dstHeight},
            .dstMipIndex = Texture.MipUavIndex[dstMipLevel],
            .isSRGB = IsSRGB,
        };
        
        if (Texture.Usage == ETextureUsage::CubeMap)
        {
            Context->SetComputeRootSignatureAndPipeline(GenerateCubemapMipmapPipelineState);
            Context->Set32BitComputeConstants(&Resource);

            Context->Dispatch(
                max((uint32_t)std::ceil(dstWidth / 8.0f), 1u),
                max((uint32_t)std::ceil(dstHeight / 8.0f), 1u),
                6);
        }
        else
        {
            Context->SetComputeRootSignatureAndPipeline(GenerateMipmapPipelineState);
            Context->Set32BitComputeConstants(&Resource);

            Context->Dispatch(
                max((uint32_t)std::ceil(dstWidth / 8.0f), 1u),
                max((uint32_t)std::ceil(dstHeight / 8.0f), 1u),
                1);
        }
    }
    
    // process last one
    D3D12_RESOURCE_BARRIER SrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        Texture.Allocation.Resource.Get(),
        Texture.ResourceState,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        ResourceDesc.MipLevels - 1);
    Context->AddResourceBarrier(SrcBarrier);
    Context->ExecuteResourceBarriers();

    Device->ExecuteAndFlushComputeContext(std::move(Context));
}
