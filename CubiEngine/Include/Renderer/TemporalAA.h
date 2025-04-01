#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FTemporalAA
{
public:
    FTemporalAA(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene);
    
    uint32_t HistoryFrameCount = 0;
    FPipelineState TemporalAAResolvePipelineState;
    FPipelineState TemporalAAUpdateHistoryPipelineState;

    FTexture HistoryTexture;
    FTexture ResolveTexture;
};
