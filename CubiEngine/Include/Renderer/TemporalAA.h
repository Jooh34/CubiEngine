#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FTemporalAAPass : public FRenderPass
{
public:
    FTemporalAAPass(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void Resolve(FGraphicsContext* const GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);
    void UpdateHistory(FGraphicsContext* const GraphicsContext, FScene* Scene);
    
    uint32_t HistoryFrameCount = 0;
    FPipelineState TemporalAAResolvePipelineState;
    FPipelineState TemporalAAUpdateHistoryPipelineState;

    FTexture HistoryTexture;
    FTexture ResolveTexture;
};
