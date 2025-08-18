#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FScreenSpaceGIPass : public FRenderPass
{
public:
    FScreenSpaceGIPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;
    
    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);

    void RaycastDiffuse(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void Denoise(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void DenoiseGaussianBlur(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene);
    void CompositionSSGI(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);

    std::unique_ptr<FTexture> StochasticNormalTexture;
    std::unique_ptr<FTexture> ScreenSpaceGITexture;
    std::unique_ptr<FTexture> DenoisedScreenSpaceGITexture;

    std::unique_ptr<FTexture> HistoryTexture;
    std::unique_ptr<FTexture> HistroyNumFrameAccumulated;
    std::unique_ptr<FTexture> ResolveTexture;

    std::unique_ptr<FTexture> HalfTexture;
    std::unique_ptr<FTexture> QuarterTexture;
    std::unique_ptr<FTexture> BlurXTexture;

    
private:
    FPipelineState GenerateStochasticNormalPipelineState;
    FPipelineState RaycastDiffusePipelineState;
    FPipelineState SSGIResolvePipelineState;
    FPipelineState SSGIUpdateHistoryPipelineState;
    FPipelineState CompositionSSGIPipelineState;

    FPipelineState SSGIDownSamplePipelineState;
    FPipelineState SSGIUpSamplePipelineState;
    FPipelineState SSGIGaussianBlurWPipelineState;
    uint32_t HistoryFrameCount = 0;
};