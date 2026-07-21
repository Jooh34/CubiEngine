#pragma once

#include "Renderer/RenderPass.h"
#include "Denoiser/OIDenoiser.h"

class FScene;
class FGraphicsContext;
class FTexture;

class FDenoisePass : public FRenderPass
{
public:
    FDenoisePass(uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) override;
    FTexture* AddPass(FGraphicsContext* GraphicsContext,
        FTexture* HDR,
        FTexture* Albedo = nullptr,
        FTexture* Normal = nullptr
    );

    FTexture* GetDenoisedOutput() { return DenoisedOutput.get(); }

private:
    std::unique_ptr<FOIDenoiser> OIDenoiser{};

    std::unique_ptr<FTexture> DenoisedOutput{};

    FTexture* BoundHDR = nullptr;
    FTexture* BoundAlbedo = nullptr;
    FTexture* BoundNormal = nullptr;
    FTexture* BoundOutput = nullptr;
};
