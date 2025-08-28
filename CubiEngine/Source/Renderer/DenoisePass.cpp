#include "Renderer/DenoisePass.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"

FDenoisePass::FDenoisePass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height)
    : FRenderPass(Device, Width, Height)
{
    OIDenoiser = std::make_unique<FOIDenoiser>();

    FOIDenoiser::FCreateDesc CreateDesc{};
    CreateDesc.D3DDevice = Device->GetDevice();
    CreateDesc.DxgiAdapter = Device->GetAdapter1();
	CreateDesc.Quality = OIDN_QUALITY_BALANCED; // FAST or BALANCED for real-time
    ;
    OIDenoiser->Initialize(CreateDesc);
}

void FDenoisePass::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc DenoisedOutputDesc{
        .Usage = ETextureUsage::UAVTexture,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"DenoisedOutput",
    };

	DenoisedOutput = Device->CreateTexture(DenoisedOutputDesc);

    bRefeshResource = true;
}

FTexture* FDenoisePass::AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR)
{
    if (bRefeshResource)
    {
		OIDenoiser->RefreshBuffers(HDR->GetResource(), nullptr, nullptr, DenoisedOutput->GetResource());
        bRefeshResource = false;
    }

	OIDenoiser->AddPass(GraphicsDevice, HDR, DenoisedOutput.get());

    return DenoisedOutput.get();
}
