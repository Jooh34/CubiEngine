#pragma once

#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FPathTracingPass
{
public:
    FPathTracingPass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t Width, uint32_t Height);

    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture& GetPathTracingSceneTexture() { return PathTracingSceneTexture; }

private:
    const FGraphicsDevice* GraphicsDevice;

    FRaytracingPipelineState PathTracingPassPipelineState;
    FShaderBindingTable PathTracingPassSBT;

    FTexture PathTracingSceneTexture;
    FTexture FrameAccumulatedTexture;

    uint32_t Width;
    uint32_t Height;

    uint32_t CollectedFrame = 0;
    uint32_t SamplePerPixel = 8;
};
