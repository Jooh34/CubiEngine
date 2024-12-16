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

    void Tonemapping(FGraphicsContext* const GraphicsContext,
        FTexture& SrcTexture, uint32_t Width, uint32_t Height);

    void DebugVisualize(FGraphicsContext* const GraphicsContext, FTexture& SrcTexture, uint32_t Width, uint32_t Height);

    FPipelineState TonemappingPipelineState;
    FTexture LDRTexture;

    FPipelineState DebugVisualizeCubeMapPipeline;
    FPipelineState DebugVisualizeDepthPipeline;
};
