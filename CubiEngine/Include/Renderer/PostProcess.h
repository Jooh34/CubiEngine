#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FPostProcess
{
public:
   FPostProcess(FGraphicsDevice* const GraphicsDevice, uint32_t Width, uint32_t Height);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void Tonemapping(FGraphicsContext* const GraphicsContext, FScene* Scene,
        FTexture& SrcTexture, FTexture& LDRTexture, uint32_t Width, uint32_t Height);

    void DebugVisualize(FGraphicsContext* const GraphicsContext, FScene* Scene, FTexture& SrcTexture, FTexture& TargetTexture, uint32_t Width, uint32_t Height);

    FPipelineState TonemappingPipelineState;

    FPipelineState DebugVisualizePipeline;
    FPipelineState DebugVisualizeDepthPipeline;
};
