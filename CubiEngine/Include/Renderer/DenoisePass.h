#pragma once

#include "Renderer/RenderPass.h"
#include "Denoiser/OIDenoiser.h"

class FScene;
class FGraphicsContext;
class FTexture;

class FDenoisePass : public FRenderPass
{
public:
    FDenoisePass(const FGraphicsDevice* const InDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;
    FTexture* AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR);

    FTexture* GetDenoisedOutput() { return DenoisedOutput.get(); }

private:
    std::unique_ptr<FOIDenoiser> OIDenoiser{};

    std::unique_ptr<FTexture> DenoisedOutput{};

    bool bRefeshResource = false;
};