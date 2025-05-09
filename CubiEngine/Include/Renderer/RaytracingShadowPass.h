#pragma once

#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderBindingTable.h"

class FScene;
class FGraphicsContext;

class FRaytracingShadowPass
{
public:
    FRaytracingShadowPass(const FGraphicsDevice* const Device, FScene* Scene, uint32_t Width, uint32_t Height);

    void OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    void AddPass(FGraphicsContext* GraphicsContext, FScene* Scene, FSceneTexture& SceneTexture);

    FTexture& GetRaytracingShadowTexture() { return RaytracingShadowTexture; }

private:
    const FGraphicsDevice* GraphicsDevice;

    FRaytracingPipelineState RaytracingShadowPipelineState;
    FShaderBindingTable RaytracingShadowPassSBT;

    FTexture RaytracingShadowTexture;

    uint32_t Width;
    uint32_t Height;
};