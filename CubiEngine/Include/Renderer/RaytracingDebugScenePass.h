#pragma once

#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingDebugScenePass
{
public:
    FRaytracingDebugScenePass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t Width, uint32_t Height);
    
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture& GetRaytracingDebugSceneTexture() { return RaytracingDebugSceneTexture; }

private:
    const FGraphicsDevice* GraphicsDevice;

    FRaytracingPipelineState RaytracingDebugScenePassPipelineState;
    FShaderBindingTable RaytracingDebugScenePassSBT;

    FTexture RaytracingDebugSceneTexture;

    uint32_t Width;
    uint32_t Height;
};