#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FScreenSpaceGI
{
public:
    FScreenSpaceGI(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    
    void GenerateStochasticNormal(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* GBufferB, uint32_t Width, uint32_t Height);
    void RaycastDiffuse(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* HDR, FTexture* Depth, uint32_t Width, uint32_t Height);
    void Denoise(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height);
    void Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* VelocityTexture, uint32_t Width, uint32_t Height);
    void UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height);
    void CompositionSSGI(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture* HDR, FTexture* GBufferA, uint32_t Width, uint32_t Height);

    FTexture StochasticNormalTexture;
    FTexture ScreenSpaceGITexture;
    FTexture DenoisedScreenSpaceGITexture;

    FTexture HistoryTexture;
    FTexture ResolveTexture;

    FTexture HalfTexture;
    FTexture QuarterTexture;

    
private:
    FPipelineState GenerateStochasticNormalPipelineState;
    FPipelineState RaycastDiffusePipelineState;
    FPipelineState SSGIDownSamplePipelineState;
    FPipelineState SSGIUpSamplePipelineState;
    FPipelineState SSGIResolvePipelineState;
    FPipelineState SSGIUpdateHistoryPipelineState;
    FPipelineState CompositionSSGIPipelineState;

    uint32_t HistoryFrameCount = 0;
};