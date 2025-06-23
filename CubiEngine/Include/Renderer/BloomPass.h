#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FBloomPass : public FRenderPass
{
public:
    FBloomPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height);

    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void AddBloomPass(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR);
    void DownSampleSceneTexture(FGraphicsContext* GraphicsContext, FTexture* HDR);

    FTexture BloomResultTexture;
    std::vector<FTexture> BloomXTextures;
    std::vector<FTexture> BloomYTextures;
    std::vector<FTexture> DownSampledSceneTextures;

private:
    static constexpr int BLOOM_MAX_STEP = 4;

    FPipelineState GaussianBlurPipelineState;
    FPipelineState DownSamplePipelineState;

};