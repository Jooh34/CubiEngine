#include "Renderer/DenoisePass.h"
#include "Graphics/Resource.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Scene/Scene.h"

FDenoisePass::FDenoisePass(uint32_t Width, uint32_t Height)
    : FRenderPass(Width, Height)
{
    OIDenoiser = std::make_unique<FOIDenoiser>();

    FOIDenoiser::FCreateDesc CreateDesc{};
    CreateDesc.D3DDevice = RHIGetDevice();
    CreateDesc.DxgiAdapter =RHIGetAdapter1();
	CreateDesc.Quality = OIDN_QUALITY_BALANCED; // FAST or BALANCED for real-time
    ;
    OIDenoiser->Initialize(CreateDesc);
}

void FDenoisePass::InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc DenoisedOutputDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"DenoisedOutput",
    };

	DenoisedOutput = RHICreateTexture(DenoisedOutputDesc);

    bRefeshResource = true;
}

FTexture* FDenoisePass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR, FTexture* Albedo, FTexture* Normal)
{
    if (bRefeshResource)
    {
		ID3D12Resource* AlbedoResource = Scene->GetRenderSettings().bDenoiserAlbedoNormal ? Albedo->GetResource() : nullptr;
		ID3D12Resource* NormalResource = Scene->GetRenderSettings().bDenoiserAlbedoNormal ? Normal->GetResource() : nullptr;

		OIDenoiser->RefreshBuffers(HDR->GetResource(), AlbedoResource, NormalResource, DenoisedOutput->GetResource());
        bRefeshResource = false;
    }

	OIDenoiser->AddPass(HDR, Albedo, Normal, DenoisedOutput.get());

    return DenoisedOutput.get();
}
